// Przykład dydaktyczny: różne rodzaje pętli głównej serwera TCP.
//
// gcc -std=c99 -pedantic -Wall -pthread rot13_server.c -lrt -o rot13_server
//
// Opcja -pthread jest konieczna bo przykład korzysta z POSIX-owych wątków;
// -lrt dołącza bibliotekę, która w starszych wersjach glibc była niezbędna
// aby móc korzystać z zegarów wysokiej rozdzielczości.

// Zdefiniowanie dwóch poniższych makr sygnalizowałoby, że kod napisano
// używając funkcji z POSIX.1-2008 z rozszerzeniem X/Open System Interface.
//   #define _POSIX_C_SOURCE 200809L
//   #define _XOPEN_SOURCE 700
// Niestety, POSIX nie definiuje liczbowych id wątków ani ulepszonych wersji
// funkcji poll. W przykładzie obie te rzeczy były potrzebne, kod wykracza
// więc poza standard POSIX i używa funkcji specyficznych dla Linuksa. Stąd
// użycie makra _GNU_SOURCE, znaczącego mniej więcej "chcę mieć dostęp do
// wszystkiego, co oferuje mój system operacyjny".
//
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

// Te dwa pliki nagłówkowe są specyficzne dla Linuksa z biblioteką glibc:
#include <sys/epoll.h>
#include <sys/syscall.h>

// Standardowa procedura tworząca nasłuchujące gniazdko TCP.

int listening_socket_tcp_ipv4(in_port_t port)
{
    int s;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);

    if (bind(s, (struct sockaddr *) &a, sizeof(a)) == -1) {
        perror("bind");
        goto close_and_fail;
    }

    if (listen(s, 10) == -1) {
        perror("listen");
        goto close_and_fail;
    }

    return s;

close_and_fail:
    close(s);
    return -1;
}

// Pomocnicze funkcje do drukowania na ekranie komunikatów uzupełnianych
// o znacznik czasu oraz identyfikatory procesu/wątku. Będą używane do
// raportowania przebiegu pozostałych operacji we-wy.

void log_printf(const char * fmt, ...)
{
    // bufor na przyrostowo budowany komunikat, len mówi ile już znaków ma
    char buf[1024];
    int len = 0;

    struct timespec date_unix;
    struct tm date_human;
    long int usec;
    if (clock_gettime(CLOCK_REALTIME, &date_unix) == -1) {
        perror("clock_gettime");
        return;
    }
    if (localtime_r(&date_unix.tv_sec, &date_human) == NULL) {
        perror("localtime_r");
        return;
    }
    usec = date_unix.tv_nsec / 1000;

    // getpid() i gettid() zawsze wykonują się bezbłędnie
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    len = snprintf(buf, sizeof(buf), "%02i:%02i:%02i.%06li PID=%ji TID=%ji ",
                date_human.tm_hour, date_human.tm_min, date_human.tm_sec,
                usec, (intmax_t) pid, (intmax_t) tid);
    if (len < 0) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    int i = vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
    va_end(ap);
    if (i < 0) {
        return;
    }
    len += i;

    // zamień \0 kończące łańcuch na \n i wyślij całość na stdout, czyli na
    // deskryptor nr 1; bez obsługi błędów aby nie komplikować przykładu
    buf[len] = '\n';
    write(1, buf, len + 1);
}

void log_perror(const char * msg)
{
    log_printf("%s: %s", msg, strerror(errno));
}

void log_error(const char * msg, int errnum)
{
    log_printf("%s: %s", msg, strerror(errnum));
}

// Pomocnicze funkcje wykonujące pojedynczą operację we-wy oraz wypisujące
// szczegółowe komunikaty o jej działaniu.

int accept_verbose(int srv_sock)
{
    struct sockaddr_in a;
    socklen_t a_len = sizeof(a);

    log_printf("calling accept() on descriptor %i", srv_sock);
    int rv = accept(srv_sock, (struct sockaddr *) &a, &a_len);
    if (rv == -1) {
        log_perror("accept");
    } else {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &a.sin_addr, buf, sizeof(buf)) == NULL) {
            log_perror("inet_ntop");
            strcpy(buf, "???");
        }
        log_printf("new client %s:%u, new descriptor %i",
                        buf, (unsigned int) ntohs(a.sin_port), rv);
    }
    return rv;
}

ssize_t read_verbose(int fd, void * buf, size_t nbytes)
{
    log_printf("calling read() on descriptor %i", fd);
    ssize_t rv = read(fd, buf, nbytes);
    if (rv == -1) {
        log_perror("read");
    } else {
        log_printf("received %zi bytes on descriptor %i", rv, fd);
    }
    return rv;
}

ssize_t write_verbose(int fd, void * buf, size_t nbytes)
{
    log_printf("calling write() on descriptor %i", fd);
    ssize_t rv = write(fd, buf, nbytes);
    if (rv == -1) {
        log_perror("write");
    } else if (rv < nbytes) {
        log_printf("partial write on %i, wrote only %zi of %zu bytes",
                        fd, rv, nbytes);
    } else {
        log_printf("wrote %zi bytes on descriptor %i", rv, fd);
    }
    return rv;
}

int close_verbose(int fd)
{
    log_printf("closing descriptor %i", fd);
    int rv = close(fd);
    if (rv == -1) {
        log_perror("close");
    }
    return rv;
}

// Procedury przetwarzające pojedynczą porcję danych przysłaną przez klienta.

void rot13(char * data, size_t data_len)
{
    char * p = data;
    char * e = data + data_len;
    for (; p < e; ++p) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (*p - 'a' + 13) % 26 + 'a';
        } else if (*p >= 'A' && *p <= 'Z') {
            *p = (*p - 'A' + 13) % 26 + 'A';
        }
    }
}

ssize_t read_rot13_write(int sock)
{
    char buf[4096];

    ssize_t bytes_read = read_verbose(sock, buf, sizeof(buf));
    if (bytes_read < 0) {
        return -1;
    }

    rot13(buf, bytes_read);

    char * data = buf;
    size_t data_len = bytes_read;
    while (data_len > 0) {
        ssize_t bytes_written = write_verbose(sock, data, data_len);
        if (bytes_written < 0) {
            return -1;
        }
        data = data + bytes_written;
        data_len = data_len - bytes_written;
    }

    return bytes_read;
}

// Teraz implementacje różnych typów głównej pętli serwera. Na początek
// serwer iteracyjny, potrafiący obsługiwać tylko jednego klienta na raz.

void iterative_loop(int srv_sock)
{
    while (true) {
        int s = accept_verbose(srv_sock);
        if (s == -1) {
            break;
        }

        while (read_rot13_write(s) > 0) {
            ;
        }
        close_verbose(s);
    }
}

// Serwer forkujący odrębny proces potomny dla każdego klienta.
//
// Podczas forkowania deskryptory są duplikowane. Jeśli oryginalny proces miał
// otwarte gniazdko TCP, to aby je zamknąć trzeba zamknąć odnoszący się do
// niego deskryptor i w procesie-rodzicu, i w procesie-dziecku.
//
// Domyślnie po zakończeniu wykonywania się procesu-dziecka staje się on tzw.
// "procesem zombie", dalej mającym wpis w ogólnosystemowej tablicy procesów.
// Dzięki temu proces-rodzic może poprosić jądro o informacje dot. sposobu,
// w jaki dziecko zakończyło działanie, i dopiero wtedy zombie jest usuwany.

void forking_loop(int srv_sock)
{
    // daj znać jądru że nie będziemy prosić o statusy dzieci; od razu po
    // zakończeniu działania można usunąć ich wpisy w tablicy procesów
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        log_perror("signal");
        return;
    }

    while (true) {
        int s = accept_verbose(srv_sock);
        if (s == -1) {
            break;
        }

        log_printf("attempting to fork a child");
        pid_t pid = fork();
        if (pid == -1) {
            // instrukcje do wykonania w razie nieudanego forkowania
            log_perror("fork");
        } else if (pid == 0) {
            // instrukcje do wykonania w procesie-dziecku
            log_printf("child process started");
            close_verbose(srv_sock);

            while (read_rot13_write(s) > 0) {
                ;
            }
            close_verbose(s);

            log_printf("child exits");
            exit(0);
        } else {
            // instrukcje do wykonania w oryginalnym procesie-rodzicu
            ;
        }
        // poniższe close() wykona się i w rodzicu, i w razie błędu fork()
        close_verbose(s);
    }
}

// Serwer wielowątkowy, dla każdego klienta tworzony jest nowy wątek.
//
// Wątki współdzielą zasoby procesu, w tym deskryptory. W odróżnieniu od
// wersji forkującej deskryptor połączenia TCP jest więc tu zamykany tylko
// w wątku obsługującym danego klienta.
//
// Podobnie jak w przypadku procesów zakończone wątki normalnie stają się
// "wątkami zombie", zajmującymi miejsce w tablicy wątków do chwili aż ktoś
// poprosi jądro o wynik przez nie zwrócony. Jeśli tego nie chcemy, trzeba
// wątkom ustawić atrybut "detached".

void * client_thread(void * arg)
{
    log_printf("thread started");
    int s = *( (int *) arg );

    while (read_rot13_write(s) > 0) {
        ;
    }
    close_verbose(s);

    log_printf("thread exits");
    // nie zapomnij o zwolnieniu argumentu zaalokowanego przez główny wątek
    free(arg);
    return NULL;
}

void multithreaded_loop(int srv_sock)
{
    pthread_attr_t attr;
    int errnum;

    // funkcje obsługujące wątki nie zapisują kodów błędu w zmiennej errno,
    // lecz zwracają te kody jako swój wynik
    errnum = pthread_attr_init(&attr);
    if (errnum != 0) {
        log_error("pthread_attr_init", errnum);
        return;
    }
    errnum = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (errnum != 0) {
        log_error("pthread_attr_setdetachstate", errnum);
        goto cleanup_attr;
    }

    while (true) {
        int s = accept_verbose(srv_sock);
        if (s == -1) {
            break;
        }

        int * p = (int *) malloc(sizeof(int));
        if (p == NULL) {
            log_printf("cannot allocate memory for thread argument");
            goto cleanup_sock;
        }
        *p = s;

        log_printf("attempting to create a thread");
        pthread_t thr;
        errnum = pthread_create(&thr, &attr, client_thread, p);
        if (errnum != 0) {
            log_error("pthread_create", errnum);
            goto cleanup_sock;
        }

        // wątek klienta uruchomiony, można wrócić na początek pętli
        continue;

cleanup_sock:
        // nie udało się stworzyć wątku do obsługi odebranego połączenia
        close_verbose(s);
    }

cleanup_attr:
    errnum = pthread_attr_destroy(&attr);
    if (errnum != 0) {
        log_error("pthread_attr_destroy", errnum);
    }
}

// Serwery wieloprocesowe i wielowątkowe są ograniczone maksymalną liczbą
// procesów/wątków, które potrafi obsłużyć jądro systemu. Traci się też czas
// na tzw. przełączanie kontekstu pomiędzy procesami/wątkami. Wymyślono więc
// różne podejścia pozwalające jednemu wątkowi obsługiwać wiele gniazdek.
//
// Wszystkie one w jakiś sposób muszą ominąć to, że operacje we-wy domyślnie
// są blokujące. Wywołanie read() na gniazdku może zawiesić wątek na nieznany
// z góry czas w oczekiwaniu na dane; w ciągu tego okresu serwer będzie
// bezczynny nawet jeśli w międzyczasie pojawią się dane na innych gniazdkach.
//
// Poniższa procedura używa funkcji select(), pozwalającej sprawdzić na
// których deskryptorach z podanego jej zbioru czekają gotowe do odczytu
// dane, czy też gotowe do odebrania nowe połączenia. Moża wtedy wywołać
// read() bądź accept() wiedząc, że nie zablokują procesu/wątku.
//
// Ograniczeniem select() jest pojemność zbiorów. Potrafią one przechowywać
// liczby od 0 do FD_SETSIZE - 1. W dawnych czasach FD_SETSIZE mogło wynosić
// np. 64 lub 256, we współczesnych dystrybucjach Linuksa jest równe 1024.

void select_loop(int srv_sock)
{
    fd_set sock_fds;    // zbiór deskryptorów otwartych gniazdek
    int max_fd;         // największy użyty numer deskryptora

    // na początku zbiór zawiera tylko gniazdko odbierające połączenia
    FD_ZERO(&sock_fds);
    FD_SET(srv_sock, &sock_fds);
    max_fd = srv_sock;

    while (true) {
        log_printf("calling select()");
        // select() modyfikuje zawartość przekazanego mu zbioru, zostaną
        // w nim tylko deskryptory mające gotowe do odczytu dane
        fd_set read_ready_fds = sock_fds;
        int num = select(max_fd + 1, &read_ready_fds, NULL, NULL, NULL);
        if (num == -1) {
            log_perror("select");
            break;
        }
        log_printf("number of ready to read descriptors = %i", num);

        for (int fd = 0; fd <= max_fd; ++fd) {
            if (! FD_ISSET(fd, &read_ready_fds)) {
                continue;
            }

            if (fd == srv_sock) {

                int s = accept_verbose(srv_sock);
                if (s == -1) {
                    goto break_out_of_main_loop;
                } else if (s >= FD_SETSIZE) {
                    log_printf("%i cannot be added to a fd_set", s);
                    // tego klienta nie da się obsłużyć, więc zamknij gniazdko
                    close_verbose(s);
                    continue;
                }
                FD_SET(s, &sock_fds);
                if (s > max_fd) {
                    max_fd = s;
                }

            } else {    // fd != srv_sock

                if (read_rot13_write(fd) <= 0) {
                    // druga strona zamknęła połączenie lub wystąpił błąd
                    FD_CLR(fd, &sock_fds);
                    close_verbose(fd);
                }

            }
        }
    }
break_out_of_main_loop:

    // zamknij wszystkie połączenia z klientami
    for (int fd = 0; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &sock_fds) && fd != srv_sock) {
            close_verbose(fd);
        }
    }
}

// Zamiast select() można użyć poll(), która nie ma ograniczenia na liczbę
// sprawdzanych deskryptorów. Funkcja poll() jako argument bierze wskaźnik
// na początek tablicy, w której kolejnych elementach podane są deskryptory
// i typy zdarzeń, na które program chce reagować. Tablica może być dowolnie
// długa, można więc obsługiwać wiele tysięcy równoległych połączeń.
//
// Przy tak długiej tablicy problemem staje się czas, który trzeba poświęcić
// na jej przeglądanie, oraz na ewentualne manipulacje w strukturach jądra
// związanych z deskryptorami będącymi obiektem zainteresowania programu.
// Przy każdym wywołaniu poll() te manipulacje trzeba wykonywać od początku.

// Różne systemy operacyjne wprowadziły różne mechanizmy ulepszające poll.
// Opisy kilku z nich można znaleźć na http://www.kegel.com/c10k.html gdzie
// zebrano materiały dotyczące "problemu C10k", czyli problemu równoczenej
// obsługi 10 tysięcy połączeń.
//
// Linux ma epoll, które przenosi do wnętrza jądra całą strukturę danych
// reprezentującą zbiór interesujących nas deskryptorów. Kod wywołujący
// epoll_wait() musi tylko podać identyfikator/deskryptor tej struktury,
// i adres bufora w którym jądro zapisze te elementy zbioru, gdzie pojawiły
// się interesujące nas zdarzenia.

int add_fd_to_epoll(int fd, int epoll_fd)
{
    log_printf("adding descriptor %i to epoll instance %i", fd, epoll_fd);
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;    // rodzaj interesujących nas zdarzeń
    ev.data.fd = fd;        // dodatkowe dane, jądro nie zwraca na nie uwagi
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rv == -1) {
        log_perror("epoll_ctl(ADD)");
    }
    return rv;
}

int remove_fd_from_epoll(int fd, int epoll_fd)
{
    log_printf("removing descriptor %i from epoll instance %i", fd, epoll_fd);
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (rv == -1) {
        log_perror("epoll_ctl(DEL)");
    }
    return rv;
}

#define MAX_EVENTS 8

void epoll_loop(int srv_sock)
{
    // w dzisiejszych czasach wartość argumentu nie ma znaczenia
    int epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        log_perror("epoll_create");
        return;
    }
    log_printf("epoll instance created, descriptor %i", epoll_fd);

    if (add_fd_to_epoll(srv_sock, epoll_fd) == -1) {
        goto cleanup_epoll;
    }

    while (true) {
        log_printf("calling epoll()");
        struct epoll_event events[MAX_EVENTS];
        // timeout równy -1 oznacza czekanie w nieskończoność
        int num = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num == -1) {
            log_perror("epoll");
            break;
        }
        // epoll wypełniła num początkowych elementów tablicy events
        log_printf("number of events = %i", num);

        for (int i = 0; i < num; ++i) {
            // deskryptor został wcześniej zapisany jako dodatkowe dane
            int fd = events[i].data.fd;
            // typ zgłoszonego zdarzenia powinien zawierać "gotów do odczytu"
            if ((events[i].events & EPOLLIN) == 0) {
                // niby nigdy nie powinno się zdarzyć, ale...
                log_printf("descriptor %i isn't ready to read", fd);
                continue;
            }

            if (fd == srv_sock) {

                int s = accept_verbose(srv_sock);
                if (s == -1) {
                    goto cleanup_epoll;
                }
                if (add_fd_to_epoll(s, epoll_fd) == -1) {
                    goto cleanup_epoll;
                }

            } else {    // fd != srv_sock

                if (read_rot13_write(fd) <= 0) {
                    // druga strona zamknęła połączenie lub wystąpił błąd
                    remove_fd_from_epoll(fd, epoll_fd);
                    close_verbose(fd);
                }

            }
        }
    }

cleanup_epoll:
    close_verbose(epoll_fd);

    // W tym miejscu należałoby zamknąć otwarte połączenia z klientami, ale
    // nie dysponujemy żadną listą ani zbiorem z numerami ich deskryptorów.
}

// Powyższa funkcja używała epoll w kontekście jednego wątku. W rzeczywistych
// serwerach, takich jak Apache czy nginx, liczba wątków zazwyczaj jest
// proporcjonalna do liczby rdzeni obliczeniowych w komputerze. Pojawiają
// się wtedy dodatkowe problemy związane z synchronizacją, ze sprawiedliwym
// podziałem pracy pomiędzy wątki, itd.
//
// I oczywiście w rzeczywistych serwerach obliczenia wykonywane na danych są
// znacznie bardziej skomplikowane. Niezbędne dane wejściowe prawdopodobnie
// przyjdą w wielu porcjach, wygenerowana odpowiedź może być zbyt duża aby
// dało się ją jednym wywołaniem write() odesłać bez blokowania, itd.
//
// Osobom na serio zainteresowanym tym, jak działają rzeczywiste serwery
// obsługujące setki/tysiące połączeń na sekundę polecam więc przestudiowanie
// ich kodu źródłowego.

// Do napisania została jeszcze tylko funkcja main, i to będzie wszystko.

const struct {
    const char * name;
    void (*func_ptr)(int);
} srv_modes[] = {
    { "iterative", iterative_loop },
    { "forking", forking_loop },
    { "multithreaded", multithreaded_loop },
    { "select", select_loop },
    { "epoll", epoll_loop },
};

int main(int argc, char * argv[])
{
    long int srv_port;
    int srv_sock;
    void (*main_loop)(int);

    // Przetwórz argumenty podane w linii komend, ustaw na ich podstawie
    // wartości zmiennych srv_port i main_loop.

    if (argc != 3) {
        goto bad_args;
    }

    main_loop = NULL;
    for (int i = 0; i < sizeof(srv_modes) / sizeof(srv_modes[0]); ++i) {
        if (strcmp(argv[1], srv_modes[i].name) == 0) {
            main_loop = srv_modes[i].func_ptr;
            break;
        }
    }
    if (main_loop == NULL) {
        goto bad_args;
    }

    char * p;
    errno = 0;
    srv_port = strtol(argv[2], &p, 10);
    if (errno != 0 || *p != '\0' || srv_port < 1024 || srv_port > 65535) {
        goto bad_args;
    }

    // Stwórz gniazdko i uruchom pętlę odbierającą przychodzące połączenia.

    if ((srv_sock = listening_socket_tcp_ipv4(srv_port)) == -1) {
        goto fail;
    }

    log_printf("starting main loop");
    main_loop(srv_sock);
    log_printf("main loop done");

    if (close(srv_sock) == -1) {
        log_perror("close");
        goto fail;
    }

    return 0;

bad_args:
    fprintf(stderr, "Usage: %s mode port\n", argv[0]);
    fprintf(stderr, "available server modes:\n");
    for (int i = 0; i < sizeof(srv_modes) / sizeof(srv_modes[0]); ++i) {
        fprintf(stderr, "    %s\n", srv_modes[i].name);
    }
    fprintf(stderr, "listening port number range: 1024-65535\n");
fail:
    return 1;
}

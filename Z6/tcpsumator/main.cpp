#include <cerrno>
#include <stdarg.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <sys/epoll.h>
#include <sys/syscall.h>
#include "Automat.h"
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

int accept_verbose(int srv_sock)
{
    struct sockaddr_in a;
    socklen_t a_len = sizeof(a);

    int rv = accept(srv_sock, (struct sockaddr *) &a, &a_len);
    if (rv == -1) {
        perror("accept");
    } else {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &a.sin_addr, buf, sizeof(buf)) == NULL) {
            perror("inet_ntop");
            strcpy(buf, "???");
        }
	std::cout<<"new client"<<std::endl;
    }
    return rv;
}

ssize_t read_verbose(int fd, void * buf, size_t nbytes)
{
    ssize_t rv = read(fd, buf, nbytes);
    if (rv == -1) {
        perror("read");
    }
    return rv;
}

ssize_t write_verbose(int fd, void * buf, size_t nbytes)
{
    ssize_t rv = write(fd, buf, nbytes);
    if (rv == -1){ 
        perror("write");
    }
    return rv;
}

int close_verbose(int fd)
{
    int rv = close(fd);
    if (rv == -1) {
        perror("close");
    }
    return rv;
}
ssize_t read_data(int sock)
{
    char buf[4096];

    ssize_t bytes_read = read_verbose(sock, buf, sizeof(buf));
    if (bytes_read < 0) {
        return -1;
    }
    Automat a;
    ssize_t msgSize = 0;
    Automat::States s;
		for(int j=0; j<bytes_read;j++){
			s=a.parse(buf[j]);
			if(/*s==Automat::States::error||*/s==Automat::States::endline){ //Jesli tylko endline jest sprawdzany, to ERROR zostanie zwrócony tylko po otrzymaniu kończ linii, z error, to przy kazdym bledzie
				msgSize=a.getFinalMessage(buf);
			}else if(j+1==bytes_read){
				j=-1;
				bytes_read = read_verbose(sock, buf, sizeof(buf));
				if (bytes_read < 0) {
					return -1;
				}
			}
		}

    char * data = buf;
    size_t data_len = msgSize;
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
int add_fd_to_epoll(int fd, int epoll_fd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;    // rodzaj interesujących nas zdarzeń
    ev.data.fd = fd;        // dodatkowe dane, jądro nie zwraca na nie uwagi
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rv == -1) {
        perror("epoll_ctl(ADD)");
    }
    return rv;
}

int remove_fd_from_epoll(int fd, int epoll_fd)
{
    int rv = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (rv == -1) {
        perror("epoll_ctl(DEL)");
    }
    return rv;
}

#define MAX_EVENTS 8

void epoll_loop(int srv_sock)
{
    int epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        perror("epoll_create");
        return;
    }

    if (add_fd_to_epoll(srv_sock, epoll_fd) == -1) {
        goto cleanup_epoll;
    }

    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int num = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num == -1) {
            perror("epoll");
            break;
        }

        for (int i = 0; i < num; ++i) {
            int fd = events[i].data.fd;
            if ((events[i].events & EPOLLIN) == 0) {
                printf("descriptor %i isn't ready to read", fd);
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

            } else {

                if (read_data(fd) <= 0) {
                    remove_fd_from_epoll(fd, epoll_fd);
                    close_verbose(fd);
                }

            }
        }
    }

cleanup_epoll:
    close_verbose(epoll_fd);

}
int main(int argc, char * argv[])
{
    long int srv_port;
    int srv_sock;

    if (argc != 2) {
        goto bad_args;
    }


    char * p;
    errno = 0;
    srv_port = strtol(argv[1], &p, 10);
    if (errno != 0 || *p != '\0' || srv_port < 1024 || srv_port > 65535) {
        goto bad_args;
    }

    if ((srv_sock = listening_socket_tcp_ipv4(srv_port)) == -1) {
        goto fail;
    }

    epoll_loop(srv_sock);

    if (close(srv_sock) == -1) {
        perror("close");
        goto fail;
    }

    return 0;

bad_args:
    fprintf(stderr, "Usage: %s mode port\n", argv[0]);
    fprintf(stderr, "listening port number range: 1024-65535\n");
fail:
    return 1;
}

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
int main(int argc, const char** argv){
	if(argc!=2){
		return -1;
	}
	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in adres;
	memset(&adres,0,sizeof(adres));
	adres.sin_family = AF_INET;
	adres.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, "127.0.0.1",&adres.sin_addr);
	bind(serv_fd, (const struct sockaddr*)&adres,sizeof(adres));
	listen(serv_fd, 3);
	while (1) {
		int fd = accept(serv_fd, NULL, 0);
		char* buf="Hello, world!\r\n";
		send(fd,buf,strlen(buf),0);
		close(fd);
	}
}

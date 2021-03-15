#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
bool drukowalne(const void *buf, int len);
int main(int argc,char* argv[]){
	if(argc!=3){
		printf("Wrong numberr of arguments");
		printf("\n%s ip port",argv[0]);
		return -1;
	}
	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);	
	struct sockaddr_in adres;
	memset(&adres,0,sizeof(adres));
	adres.sin_family = AF_INET;
	adres.sin_port = htons(atoi(argv[2]));
	int pton_ret=inet_pton(AF_INET, argv[1], &adres.sin_addr);
	if(pton_ret!=1){
		printf("Bad IP format");
		return -1;
	}
	if(connect(serv_fd,(const struct sockaddr*)&adres,sizeof(adres))==-1){
		perror("connect");
		return -1;
	}
	char buf[100];
	int recv_len;
	while((recv_len=recv(serv_fd,buf,100,MSG_WAITALL))!=0){
		//if(drukowalne(buf,recv_len))
			printf("%s",buf);	
	}
	close(serv_fd);
}

bool drukowalne(const void *buf, int len)
{
	const char *tmp = (const char *)buf;
	for (int i = 0; i < len; i++)
	{
		if (tmp[i] < 31 || tmp[i] > 126)
		{
			return false;
		}
	}
	return true;
}

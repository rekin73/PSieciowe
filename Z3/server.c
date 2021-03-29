#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#define BUFFL 512
void report_error(char *s);
int processData(char *buf,int buflen,char *outbuf);
bool detectOverflow(long int a,long int b,long int* sum);
int main(int argc, char * argv[]){
	struct sockaddr_in adres,src_addr;
	int serv_fd = socket(AF_INET, SOCK_DGRAM,0);
	unsigned int addrlen=sizeof(src_addr);
	if((serv_fd)==-1){
		report_error("socket");
	}
	char buf[BUFFL];
	memset(&adres,0,sizeof(adres));
	adres.sin_family = AF_INET;
	adres.sin_port = htons(2020);
	inet_pton(AF_INET,"0" ,&adres.sin_addr);
	if((bind(serv_fd,(const struct sockaddr*)&adres,sizeof(adres)))==-1){
		report_error("bind");
	}
	memset(&src_addr,0,sizeof(src_addr));
	int recvlen,sendlen;
	char sendbuf[BUFFL];
	buf[BUFFL-1]='\0';
	while(1){
		printf("Waiting for data...\n");
		//fflush(stdout);
		if((recvlen=recvfrom(serv_fd,buf,BUFFL-1,0,(struct sockaddr *) &src_addr,&addrlen))==-1){
			report_error("recvfrom");
		}

		sendlen=processData(buf,recvlen,sendbuf);

		if((sendto(serv_fd,sendbuf,sendlen,0,(struct sockaddr*) &src_addr,addrlen))==-1){
			report_error("sendto");
		}

	}	
	if((close(serv_fd))==-1){
		report_error("close");
	}
}



void report_error(char *s){
	perror(s);
	exit(1);
}
int processData(char *buf,int buflen,char *outbuf){
	char *pch;
	long int suma=0,w;
	pch = strtok (buf," ");
	bool error = false;

	while (pch != NULL && !error)
	{
		printf ("%i\n",*pch);
		//LONG_MAX=9223372036854775807
		if((w=strtol(pch,NULL,10))==0 && strcmp(pch,"0")){
			sprintf(outbuf,"ERROR BAD_CHAR\n");
			error = true;
			break;
		}
		if(/*(w==LONG_MIN || w==LONG_MAX)&&*/errno==ERANGE){
			sprintf(outbuf,"ERROR CON_OVERFLOW\n");
			error = true;
			break;
		}
		if(detectOverflow(suma,w,&suma)){
			sprintf(outbuf,"ERROR SUM_OVERFLOW\n");
			error = true;
			break;

		}
		//suma+=w;
		pch = strtok (NULL, " ");
	}
	if(!error)
		sprintf(outbuf,"%ld\n",suma);
	errno=0;
	return strlen(outbuf);
}
bool detectOverflow(long int a,long int b,long int* sum){
	(*sum)=a+b;
	if(a>0 && b>0 && (*sum)<=0){
		return true;
	}
	if(a<0 && b<0 && (*sum)>=0){
		return true;
	}
	return false;
}

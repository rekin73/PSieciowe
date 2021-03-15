#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int copy(const char *file1,const char *file2);
int main(int argc, char *argv[]){
	if(argc!=3){
		return -1;
	}
	if(copy(argv[1],argv[2])==-1){
		perror("Error occured");
		return -1;
	}
	return 0;  
}
int copy(const char *file1,const char *file2){
	int f1,f2;
	if((f1=open(file1,O_RDONLY))==-1){
		return -1;
	}
	if((f2=open(file2,O_CREAT|O_WRONLY,0666))==-1){
		close(f1);
		return -1;
	}
	int rsize;
	char buffer[100];
	while((rsize=read(f1,buffer,100))>0){
		if(write(f2,buffer,rsize)<rsize){
			rsize=-1;
			break;
		}
	}
	close(f1);
	close(f2);
	if(rsize==-1){
		return -1;
	}
	return 0;
}

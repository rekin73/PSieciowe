#include <string.h>
#include <stdbool.h>
#include <stdio.h>
bool drukowalne(const void *buf, int len);
bool drukowalne2(const void *buf, int len);
bool drukowalne3(const char * buf);
bool drukowalne4(const char * buf);
int main(int argc, char **argv)
{
	char chars[127];
	for (int i = 0; i < 127; i++)
	{
		chars[i]=i;
		//printf("%i=%c",i,chars[i]);
	}

	//drukowalne(chars+i,1)?printf("%i=%c\n",i,chars[i]):printf("%i nie drukowalne\n",i);
	//drukowalne2(chars,127)?printf("tak\n"):printf("nie\n");
	chars[0]=1;
	//drukowalne3(chars+32)?printf("tak\n"):printf("nie\n");
	drukowalne4(chars+32)?printf("tak\n"):printf("nie\n");

}
bool drukowalne(const void *buf, int len)
{
	const char *tmp = (const char *)buf;
	for (int i = 0; i < len; i++)
	{
		if (tmp[i] < 32 || tmp[i] > 126)
		{
			return false;
		}
	}
	return true;
}
bool drukowalne2(const void *buf, int len)
{
	char *tmp = (char *)buf;
	for (; tmp < (char *)buf+len; tmp++)
	{
		//printf("%li\n",(unsigned long int)buf-(unsigned long int)tmp);
		if (*tmp < 32 || *tmp > 126)
		{
			return false;
		}
	}
	return true;
}
bool drukowalne3(const char * buf){
	while(*buf!=0)
	{
		if(*buf<32 || *buf>126)
		{

			return false;
		}
		buf++;
	}
	return true;
}
bool drukowalne4(const char * buf){
	int i=0;
	while(buf[i]!=0)
	{
		if(buf[i]<32 || buf[i]>126)
		{

			return false;
		}
		i++;
	}
	return true;

}

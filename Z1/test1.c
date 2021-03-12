#include <stdio.h>
int main(int argc, char** argv){
    int liczby[50];
    int tmp,count=0;
    for(int i=0;i<50;i++){
       scanf("%i",&tmp);
       if(tmp==0){
        break;
       }

        *(liczby+i)=tmp;
        count++;
    }
    for (int i = 0; i < count; i++)
    {
        if(*(liczby+i)>10 && *(liczby+i)<100){
            printf("%i\n",*(liczby+i));
        }
    }
    
}
#include "Automat.h"
#include <iostream>
#include <limits>
#include <string>
Automat::Automat(){
	sum=0;
	digit=0;
	errorline=false;
	state=Automat::States::start;
	//std::cout<<"init"<<(int)state<<std::endl;
}
Automat::~Automat(){}
Automat::States Automat::parse(char input){
	if((input>='0' && input<='9') || input==' ' || input=='\r' || input=='\n')
		switch(state){
			case Automat::States::start:
				if(input>='0' && input<='9'){

					state=Automat::States::cyfra;
					addDigit(input);
				}else{
					state=Automat::States::error;
				}
				break;
			case Automat::States::cyfra:
				if(input>='0' && input<='9'){
					addDigit(input);
				}else if(input==' '){
				state=Automat::States::spacja;	
				addSum();
				}else if(input=='\r'){
					state=Automat::States::returnCaret;
					addSum();
				}else{
					state=Automat::States::error;
				}
				break;
			case Automat::States::spacja:
				if(input<'0' || input>'9')
					state=Automat::States::error;
				else{
					state=Automat::States::cyfra;
					addDigit(input);
				}
				break;
			case Automat::States::error:
				if(input=='\r'){
					state=Automat::States::returnCaret;
				}	
				else if(input == '\n'){
					state=Automat::States::endline;
				}
				break;
			case Automat::States::endline:
				
				break;
			case Automat::States::returnCaret:
				if(input=='\n'){
					state=Automat::States::endline;
				}else{
					state=Automat::States::error;
				}
				break;
		}else{
			state=Automat::States::error;
		}
	if(state==Automat::States::error)
		errorline=true;
	return state;
}
int Automat::getFinalMessage(char buffor[]){
	if(!errorline){
	int tmp = sum;
	int nOfDigits=0;
	do{
		nOfDigits++;
		tmp/=10;
					
	}while(tmp);
	
	sprintf(buffor,"%d",sum);
	buffor[nOfDigits+1]='\n';
	buffor[nOfDigits]='\r';
	sum=0;
	digit=0;
	errorline=false;
	state=Automat::States::start;
	return nOfDigits+2;
	}else{
	sprintf(buffor,"ERROR\r\n");
	sum=0;
	digit=0;
	errorline=false;
	state=Automat::States::start;
	return 7;
	}
}

void Automat::addDigit(char d){
	int c = d - '0';
	if((float)digit>(float)(std::numeric_limits<int>::max()-c)/(float)10){
		state=Automat::States::error;
	}else{
		digit=(10*digit)+c;
	}
}
void Automat::addSum(){
	if((float)sum>(float)(std::numeric_limits<int>::max()-digit)){
		state=Automat::States::error;
	}else{
		sum+=digit;
	}
	digit=0;
	
}

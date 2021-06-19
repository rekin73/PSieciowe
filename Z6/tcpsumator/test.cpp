#include "Automat.h"
#include <iostream>
#include <vector>
int main(){
	std::vector<std::vector<char>> tab={
		{'4','2',' ','1','0','\r','\n',
		'1',' ','2',' ','3','\r','\n'},
		{'1',' ',' ','2',' ','3','\r','\n'}

	};
	Automat a;
	Automat::States s;
	char buff[1024];
	int buffsize;
	for(int i=0; i<(int)(tab.size());i++){

		for(int j=0; j<(int)(tab[i].size());j++){
			s=a.parse(tab[i][j]);
			//std::cout<<(int)s<<" "<<tab[i][j]<<std::endl;
			if(/*s==Automat::States::error||*/s==Automat::States::endline){
				buffsize=a.getFinalMessage(buff);
				for(int k=0; k<buffsize;k++){
					std::cout<<buff[k];
					//buff[k]=0;
				}
			}
		}


	}

}

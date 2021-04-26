#ifndef TCPSUM_AUTOMAT_H
#define TCPSUM_AUTOMAT_H
class Automat{
	private:
		char state;
		int sum;
		int cyfra;
	public:
		static const enum struct States:char {
		start,
		cyfra,
		spacja,
		error,
		}States;
		Automat();
		~Automat();
		char parse(char input);//returns current state
		int getFinalMessage(char buffor[]);//returns size of buffor or -1
	private:

	
};
#endif

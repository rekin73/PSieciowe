#ifndef TCPSUM_AUTOMAT_H
#define TCPSUM_AUTOMAT_H
class Automat{
	public:
		enum struct States:char {
			start,
			cyfra,
			spacja,
			error,
			endline,
			returnCaret,
		};
	private:
		States state;
		int sum;
		int digit;
		bool errorline;
	public:
		Automat();
		~Automat();
		States parse(char input);//returns current state
		int getFinalMessage(char buffor[]);//returns size of buffor or -1
	private:
		void addDigit(char d);
		void addSum();
	
};
#endif

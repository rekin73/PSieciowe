CXXFLAGS := -pedantic -Wall -std=c++17 

LDFLAGS := -lrt -pthread 

.PHONY: all clean test

all:sumator.out
	
test:test.out 
	./test.out
%.o: %.cpp %.h
		$(CXX) $(CXXFLAGS) -c -o $@ $<

Automat.o:

test.out:Automat.o test.o

sumator.out: Automat.o main.o
	
%.out:%.o
	$(CXX) -o $@ $^ $(LDFLAGS) 
clean:
	rm *.o

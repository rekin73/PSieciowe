CXXFLAGS := -pedantic -Wall -std=c++17 

LDFLAGS := -lrt -pthread 

.PHONY: all clean test

all: Automat.o
	
test:test.out 
	./test.out
%.o: %.cpp %.h
		$(CXX) $(CXXFLAGS) -c -o $@ $<

Automat.o:

test.out:Automat.o test.o

%.out:%.o
	$(CXX) -o $@ $^ $(LDFLAGS) 
clean:
	rm *.o

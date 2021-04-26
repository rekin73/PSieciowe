CXXFLAGS := -pedantic -Wall -std=c++17

.PHONY: all clean test

all: Automat.o
	
test:test.out 
	./test.out
%.o: %.cpp %.h
		$(CXX) $(CXXFLAGS) -c -o $@ $<

Automat.o:

test.out:Automat.o test.o

%.out:%.o
	$(CXX) $(LDFLAGS) -o $@ $^
clean:
	rm *.o

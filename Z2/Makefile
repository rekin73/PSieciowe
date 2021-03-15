CFLAGS	:= -pedantic -Wall -std=c99
#CFLAGS	+= -Wall
LDFLAGS := -s 
.PHONY: all clean tar servklient
all: servklient 
sources = main procinfo
objects=$(addsuffix .o,$(sources))
autor=HubertKalinowski
katalog=`basename $(CURDIR)`
archiwum=$(autor)_$(katalog)

#'basename $(CURDIR)'
#main.o:	main.c	procinfo.h
#	gcc -o main.o main.c

#	$(CC) $(CFLAGS) -c -o $@ $<
servklient: server.x client.x

server.x:

client.x:


#program.x: $(addsuffix .o,$(sources))

%.o: %.c
	$(CC) $(CFLAGS) $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

%.x: %.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(wildcard *.[ox])


tar: clean
	(cd ../; tar -cvzf $(archiwum).tar.gz  $(katalog) )


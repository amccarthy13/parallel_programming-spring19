CC = gcc
CFLAGS = -O3 -Wall -Werror -pthread

all: fw testing

fw: fw.o
	$(CC) $(CFLAGS) -o fw fw.o

fw.o: fw.c
	$(CC) $(CFLAGS) -c fw.c

testing: testing.o
	$(CC) $(CFLAGS) -o testing testing.o

testing.o: testing.c
	$(CC) $(CFLAGS) -c testing.c

clean: 
	rm fw testing *.o



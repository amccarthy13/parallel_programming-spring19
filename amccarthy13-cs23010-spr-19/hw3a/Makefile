CC = gcc
CFLAGS = -O3 -Wall -Werror -pthread

all: counter

counter: counter.o clhlock.o alock.o taslock.o stopwatch.o
	$(CC) $(CFLAGS) -o counter counter.o clhlock.o alock.o taslock.o stopwatch.o -lm

counter.o: counter.c clhlock.h alock.h taslock.h utils/stopwatch.h
	$(CC) $(CFLAGS) -c counter.c

clhlock.o: clhlock.c clhlock.h
	$(CC) $(CFLAGS) -c clhlock.c

alock.o: alock.c alock.h
	$(CC) $(CFLAGS) -c alock.c

taslock.o: taslock.c taslock.h
	$(CC) $(CFLAGS) -c taslock.c

stopwatch.o: utils/stopwatch.c utils/stopwatch.h
	$(CC) $(CFLAGS) -c utils/stopwatch.c

clean:
	rm counter *.o
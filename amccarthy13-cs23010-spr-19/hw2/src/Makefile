CC = gcc
CFLAGS = -O3 -Wall -Werror -pthread -Wno-error=strict-aliasing

all: lamport

lamport: main.o lamport.o packetsource.o fingerprint.o generators.o crc32.o stopwatch.o
	$(CC) $(CFLAGS) -o lamport main.o fingerprint.o packetsource.o lamport.o generators.o crc32.o stopwatch.o -lm

main.o: main.c atomic.h packetsource.h fingerprint.h lamport.h
	$(CC) $(CFLAGS) -c main.c

fingerprint.o: fingerprint.c fingerprint.h
	$(CC) $(CFLAGS) -c fingerprint.c

packetsource.o: packetsource.c packetsource.h
	$(CC) $(CFLAGS) -c packetsource.c

crc32.o: crc32.c crc32.h
	$(CC) $(CFLAGS) -c crc32.c

generators.o: generators.c generators.h
	$(CC) $(CFLAGS) -c generators.c

lamport.o: lamport.c lamport.h atomic.h
	$(CC) $(CFLAGS) -c lamport.c

stopwatch.o: stopwatch.c stopwatch.h
	$(CC) $(CFLAGS) -c stopwatch.c

clean: 
	rm lamport *.o

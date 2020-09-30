CC = gcc
CFLAGS = -O3 -Wall -Werror -pthread -Wno-error=strict-aliasing

all: packet

packet: packet.o clhlock.o packetsource.o fingerprint.o generators.o crc32.o stopwatch.o
	$(CC) $(CFLAGS) -o packet packet.o clhlock.o fingerprint.o packetsource.o generators.o crc32.o stopwatch.o -lm

packet.o: packet.c clhlock.h utils/stopwatch.h  utils/fingerprint.h  utils/stopwatch.h  utils/packetsource.h
	$(CC) $(CFLAGS) -c packet.c

clhlock.o: clhlock.c clhlock.h
	$(CC) $(CFLAGS) -c clhlock.c

fingerprint.o:  utils/fingerprint.c  utils/fingerprint.h
	$(CC) $(CFLAGS) -c  utils/fingerprint.c

packetsource.o: utils/packetsource.c  utils/packetsource.h
	$(CC) $(CFLAGS) -c  utils/packetsource.c

crc32.o:  utils/crc32.c  utils/crc32.h
	$(CC) $(CFLAGS) -c  utils/crc32.c

generators.o:  utils/generators.c  utils/generators.h
	$(CC) $(CFLAGS) -c  utils/generators.c

stopwatch.o: utils/stopwatch.c utils/stopwatch.h
	$(CC) $(CFLAGS) -c utils/stopwatch.c

clean:
	rm packet *.o
CC=gcc
CFLAGS=-g -pthread

dchat1: dchat1.o
	$(CC) -o dchat1 $(CFLAGS) dchat1.o 
	
dchat1.o: dchat1.c dchat.h
	$(CC) -c dchat1.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat1
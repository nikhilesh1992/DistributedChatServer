CC=gcc
CFLAGS=-g -pthread

dchat: dchat.o
	$(CC) -o dchat $(CFLAGS) dchat.o 
	
dchat.o: dchat.c dchat.h
	$(CC) -c dchat.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat
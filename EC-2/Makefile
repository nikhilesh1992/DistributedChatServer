CC=gcc
CFLAGS=-g -pthread

dchat2: dchat2.o
	$(CC) -o dchat2 $(CFLAGS) dchat2.o 
	
dchat.o: dchat2.c dchat.h
	$(CC) -c dchat2.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat2
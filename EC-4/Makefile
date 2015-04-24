CC=gcc
CFLAGS=-g -lpthread

dchat4: dchat4.o
	$(CC) -o dchat4 $(CFLAGS) dchat4.o 
	
dchat4.o: dchat4.c dchat4.h
	$(CC) -c dchat4.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat4
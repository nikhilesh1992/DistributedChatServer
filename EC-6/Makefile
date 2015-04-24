CC=gcc
CFLAGS=-g -lpthread

dchat6: dchat6.o
	$(CC) -o dchat6 $(CFLAGS) dchat6.o 
	
dchat6.o: dchat6.c dchat6.h
	$(CC) -c dchat6.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat6
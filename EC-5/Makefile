CC=gcc
CFLAGS=-g -pthread

dchat5: dchat5.o
	$(CC) -o dchat5 $(CFLAGS) dchat5.o 
	
dchat5.o: dchat5.c dchat.h
	$(CC) -c dchat5.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat5
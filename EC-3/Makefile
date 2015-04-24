CC=gcc
CFLAGS=-g -pthread

dchat3: dchat3.o
	$(CC) -o dchat3 $(CFLAGS) dchat3.o 
	
dchat3.o: dchat3.c dchat.h
	$(CC) -c dchat3.c $(CFLAGS)
	
clean:
	rm -f *.o *.exe dchat3
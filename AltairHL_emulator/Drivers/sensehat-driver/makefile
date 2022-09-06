CFLAGS=-c -Wall -O2
CC=gcc
CPP=g++
OBJ=sense_hat.o
LIB=libsense.a

all: 
	$(CC) $(CFLAGS) src/sense_hat.c
	ar -rc $(LIB) $(OBJ)
	sudo cp $(LIB) /usr/local/lib
	sudo cp src/sense_hat.h /usr/local/include

example:
	$(CC) $(CFLAGS) demo/demo.c
	$(CC) demo.o -lsense -lpthread -lm -o sense_demo

tests:
	$(CPP) test/test_main.cpp test/test_sense_hat.cpp -o test_sense -lgtest -lpthread -lsense

remove:
	sudo rm /usr/local/lib/$(LIB)
	sudo rm /usr/local/include/sense_hat.h

clean:
	rm *.o *.a
#makefile
CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -pedantic 
.PHONY : clean

all : main consumer producer

main: main.c makefile
	$(CC) $(CFLAGS) main.c general_func.c main_func.c -o main.exe
	
consumer: consumer.c makefile
	$(CC) $(CFLAGS) consumer.c general_func.c -o consumer.exe

producer: producer.c makefile
	$(CC) $(CFLAGS) producer.c general_func.c -o producer.exe

clean:
	rm -f main.exe consumer.exe producer.exe
run:
	./main.exe
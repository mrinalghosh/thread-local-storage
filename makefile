CC=gcc -Werror -Wall -g 
all: threadlib main
	$(CC) -o main main.o tls.o

main: main.c
	$(CC) -c -o main.o main.c

threadlib: tls.c
	$(CC) -c -lpthread -o tls.o tls.c

clean:
	rm tls.o main.o main

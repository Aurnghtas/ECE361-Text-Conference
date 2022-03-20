CC=gcc
LDFLAGS=-pthread
all: server ../client/client
server: server.o
client: ../client/client.o
clean:
		rm -f *.o server 
		rm -f *.o ../client/client
CC=gcc
LDFLAGS=-pthread
all: ../server/server client
server: ../server/server.o
client: client.o
clean:
		rm -f *.o ../server/server 
		rm -f *.o client
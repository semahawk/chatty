CC = gcc
CFLAGS := $(CFLAGS) -std=gnu99 -W -Werror -Wextra

SERVER_OBJS = server.o
CLIENT_OBJS = client.o

.PHONY: all
all: chatty

chatty: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client

server.o: server.c
client.o: client.c

clean:
	rm -rf *.o

distclean: clean
	rm -rf bin/*


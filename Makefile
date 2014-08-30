CC = gcc
CFLAGS := $(CFLAGS) -g -std=c99 -W -Werror -Wextra
LIBS = -pthread

SRCS = chatty.c client.c server.c
OBJS = $(SRCS:.c=.o)

.PHONY: clean distclean

chatty: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	rm -rf *.o

distclean: clean
	rm -rf chatty


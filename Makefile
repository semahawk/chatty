CC = gcc
CFLAGS := $(CFLAGS) -g -std=gnu99 -W -Werror -Wextra
LIBS = -pthread

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean distclean
chatty: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o chatty $(LIBS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o

distclean: clean
	rm -rf chatty


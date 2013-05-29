CC = gcc
CFLAGS := $(CFLAGS) -std=gnu99 -W -Werror -Wextra

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean distclean
chatty: $(OBJS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o

distclean: clean
	rm -rf chatty


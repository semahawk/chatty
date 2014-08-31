/*
 *
 * chatty.h
 *
 * Created at:  Tue 28 May 2013 22:25:07 CEST 22:25:07
 *
 * Author:  Szymon Urbaś <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef CHATTY_H
#define CHATTY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <curses.h>

/* version of the Chatty */
#define VERSION "0.1.0"
/* the port clients will connect to (unchangeable, unless hard-coded-ingly) */
#define PORT "1337"
/* max connections waiting in a queue */
#define MAX_PENDING 5
#define THREADS	1
/* maximum size of a data packet */
#define MAX_BUFFER_SIZE 256
/* maximum size of the username */
#define MAX_NAME_SIZE 32

struct client {
  /* socket fd */
  int fd;
  /* obviously */
  char *nick;
  /* date at which the client has connected */
  time_t since;
  /* next element on the list */
  struct client *next;
};

/* `packet_type` */
#define PACKET_MSG         0x01
#define PACKET_CMD         0x02

/* `cmd_type` */
#define CMD_JOIN           0x01
#define CMD_LIST           0x02

struct packet {
  unsigned char type;
  union {
    struct {
      char username[MAX_NAME_SIZE];
      /* the minus 1 is because of `data_type` */
      char message[MAX_BUFFER_SIZE - MAX_NAME_SIZE - 1];
    } msg;

    struct {
      unsigned char type;
      /* `MAX_BUFFER_SIZE` minus `data_type` minus `cmd_type` */
      char args[MAX_BUFFER_SIZE - 2];
    } cmd;
  };
};

int server(void);
int client(void);

void *get_in_addr(struct sockaddr *);

#endif /* CHATTY_H */


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

/* version of the Chatty */
#define VERSION "0.1.0"
/* the port clients will connect to (unchangeable, unless hard-coded-ingly) */
#define PORT "1337"
/* max connections waiting in a queue */
#define MAX_PENDING 5
#define THREADS	1

/*
 * Here are all the special messages.
 *
 * If the first character of a recieved message is one of the following, then
 * it's a special message.
 *
 * If the first character of a recieved message is NOT one of the following,
 * then it's just a regular message
 */
#define MSG_JOIN 0x01
#define MSG_TEST 0x02

struct clients {
  /* socket fd */
  int fd;
  /* obviously */
  char *nick;
  /* date at which the client has connected */
  time_t since;
  /* next element on the list */
  struct clients *next;
};

int server(void);
int client(void);

void *handle_client(void *arg);
void out(const char *msg, ...);
void dispatch(int fd, char *msg);
void add_client(int fd, char *name);
char *get_nick_by_fd(int fd);
void *get_in_addr(struct sockaddr *);

#endif /* CHATTY_H */


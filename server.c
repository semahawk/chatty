/*
 *
 * server.c
 *
 * Created at:  Thu 30 May 2013 11:43:31 CEST 11:43:31
 *
 * Authors: Szymon Urbaś <szymon.urbas@aol.com>
 *          Tomek K.     <tomicode@gmail.com>
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

#include "chatty.h"

/* the clients list */
static struct clients *clients = NULL;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int server(void)
{
  struct sigaction sa;
  /* socket file descriptor */
  int sockfd;
  struct addrinfo hints;
  /* will point to the results */
  struct addrinfo *servinfo;
  /* multipurpose */
  int status;
  /* IP number of the connector */
  char ip[INET6_ADDRSTRLEN];

  /* clear out the hints struct */
  memset(&hints, 0, sizeof hints);
  /* don't care if IPv4 or IPv6 */
  hints.ai_family =  AF_UNSPEC;
  /* TCP stream sockets */
  hints.ai_socktype = SOCK_STREAM;
  /* fill in my IP for me */
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }

  /* servinfo now points to a linked list of 1 or more struct addrinfos */
  /* get the socket descriptor */
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* bind to the port we passed in to getaddrinfo */
  if ((bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /* reap all the dead processes */
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  printf(" \e[1;34m*\e[0;0m listening on port %s\n", PORT);

  /* listen to incoming connections */
  while (listen(sockfd, 10) != -1){
    /* the connectors address */
    struct sockaddr_storage their_addr;
    /* the connectors file socket descriptor */
    int newfd;
    /* size of the connectors address */
    socklen_t addr_size = sizeof their_addr;
    /* accept the connection */
    if ((newfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
      perror("accept");
      exit(EXIT_FAILURE);
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ip, sizeof ip);
    /* print some output */
    printf(" \e[1;33m*\e[0;0m got a connection from %s\n", ip);

    /* this is the child's process */
    if (!fork()){
      char msg[64];
      char welcome_msg[64];
      close(sockfd);
      /* recieving the join message */
      if (recv(newfd, msg, 64, 0) == -1){
        perror("recv");
        /*exit(EXIT_FAILURE);*/
      }
      /* dispatch the message, eg. do what it tells to */
      dispatch(newfd, msg);
      /* create the welcome message */
      sprintf(welcome_msg, "Siedem, %s!", msg);
      if (send(newfd, welcome_msg, strlen(welcome_msg), 0) == -1){
        perror("send");
        /*exit(EXIT_FAILURE);*/
      }
      /*close(newfd);*/
      exit(EXIT_FAILURE);
    }
    /*close(newfd);*/
  }

  /* free the linked list */
  freeaddrinfo(servinfo);

  return 0;
}

/*
 * Looks at the messages first char and does what is supposed to
 *
 * fd  - socket file descriptor of the client that has sent the message
 * msg - the message the client has sent
 */
void dispatch(int fd, char *msg)
{
  /* type of the message (eg. JOIN, regular) */
  char type;
  /* the message without the first char */
  char *rest;
  /* do anything only when the message is NOT empty */
  if (strcmp(msg, "")){
    type = msg[0];
    rest = msg++;
    switch (type){
      case MSG_JOIN:
        /* print some output */
        out("user %s has joined", rest);
        /* add the client to the clients list */
        add_client(fd, msg);
        break;
      default:
        /* TODO: forward the message to every client connected here */
        break;
    }
  }
}

/*
 * Adds a client of a given <fd> and <nick> to the clients list.
 */
void add_client(int fd, char *nick)
{
  struct clients *new = malloc(sizeof(struct clients));
  if (!new){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* initialize */
  new->fd = fd;
  new->nick = strdup(nick);
  new->since = time(NULL);

  /* append to the list */
  new->next = clients;
  clients = new;
}

/*
 * A handy nice little function that prints the <msg> with this little colorful
 * asterisk prepended and a newline appended.
 */
void out(const char *msg, ...)
{
  va_list vl;
  va_start(vl, msg);
  printf(" \e[1;32m*\e[0;0m ");
  vprintf(msg, vl);
  printf("\n");
  va_end(vl);
}


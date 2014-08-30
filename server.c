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
/* server's socket file descriptor */
static int sockfd;
/* will point to the results */
static struct addrinfo *servinfo;

/* forward */
static void send_to_fd(int fd, char *msg, ...);

/* SIGINT */
void sigint_handler(int s)
{
  /* freeing the clients list */
  struct clients *list;
  struct clients *next;

  if (clients != NULL){
    printf("\n");
    out("freeing the users list");
  }

  for (list = clients; list != NULL; list = next){
    next = list->next;
    out("closing %s's socket", list->nick);
    close(list->fd);
    free(list);
  }

  /* close the server's socket */
  close(sockfd);
  /* free the linked list */
  freeaddrinfo(servinfo);

  exit(EXIT_FAILURE);
}

/* SIGCHLD */
void sigchld_handler(int s)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

int server(void)
{
  /* the SIGCHLD signal */
  struct sigaction sigchld;
  /* the SIGINT signal */
  struct sigaction sigint;
  struct addrinfo hints;
  /* multipurpose */
  int status;
  /* IP number of the connector */
  char ip[INET6_ADDRSTRLEN];
  /* the connectors address */
  struct sockaddr_storage their_addr;
  /* the connectors file socket descriptor */
  int newfd;
  /* size of the connectors address */
  socklen_t addr_size = sizeof their_addr;
  /* thread for the client */
  pthread_t client_thd;

  /* clear out the hints struct */
  memset(&hints, 0, sizeof hints);
  /* use IPv4 (screw IPv6 (at least for now)) */
  hints.ai_family =  AF_INET;
  /* TCP stream sockets */
  hints.ai_socktype = SOCK_STREAM;
  /* fill in my IP for me */
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    return 1;
  }

  /* servinfo now points to a linked list of 1 or more struct addrinfos */
  /* get the socket descriptor */
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
    perror("socket");
    return 1;
  }

  /* bind to the port we passed in to getaddrinfo */
  if ((bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
    perror("bind");
    return 1;
  }

  /* reap all the dead processes */
  sigchld.sa_handler = sigchld_handler;
  sigemptyset(&sigchld.sa_mask);
  sigchld.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sigchld, NULL) == -1){
    perror("sigaction");
    return 1;
  }

  /* set the SIGINT handler */
  sigint.sa_handler = sigint_handler;
  sigemptyset(&sigint.sa_mask);
  sigint.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sigint, NULL) == -1){
    perror("sigint");
    return 1;
  }

  /* listen to incoming connections */
  if (listen(sockfd, MAX_PENDING) == -1){
    perror("listen");
    return 1;
  }

  printf(" \e[1;34m*\e[0;0m listening on port %s\n", PORT);

  while (1){
    /* accept the connection */
    if ((newfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1){
      perror("accept");
    }
    /* get the clients IP number */
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ip, sizeof ip);
    /* print some output */
    printf(" \e[1;33m*\e[0;0m got a connection from %s\n", ip);

    /* create a thread for the client */
    if (pthread_create(&client_thd, 0, handle_client, (void *)&newfd) != 0){
      perror("pthread_create");
      return 1;
    }
  }

  /* close the server's socket */
  close(sockfd);
  /* free the linked list */
  freeaddrinfo(servinfo);

  return 0;
}

/*
 * Thread function to recieve and send messages to the client
 */
void *handle_client(void *arg)
{
  int fd = *(int *)arg;
  char recv_buffer[MAX_BUFFER_SIZE];

  while (1){
    /* zero out the recieving buffer */
    memset(recv_buffer, 0, MAX_BUFFER_SIZE);
    /* recieve the message */
    if (recv(fd, recv_buffer, MAX_BUFFER_SIZE - 1, 0) != -1){
      dispatch(fd, recv_buffer);
    } else {
      perror("recv");
      exit(EXIT_FAILURE);
    }
  }

  /* close the clients socket */
  close(fd);

  return NULL;
}

/*
 * Checks if the message is a command and does what it is told to do
 *
 * fd  - socket file descriptor of the client that has sent the message
 * msg - the message the client has sent
 */
void dispatch(int fd, char *msg)
{
  /* copy of msg */
  char *copy = strdup(msg);
  /* the command, like join, list */
  char *cmd;
  /* some more strtok'd tokens */
  char *token;
  /* message delimiters for strtok */
  const char *delims = " ";

  /* don't do anything when the message is empty */
  if (!strcmp(msg, ""))
    goto END;

  /* if a message starts with a slash, it's a command */
  if (!strncmp(msg, "/", 1)){
    /* get the first word, which is the command */
    /* copy + 1 so to skip over the slash */
    cmd = strtok(copy + 1, delims);
    /* dispatch the command */
    /* XXX /join */
    if (!strcmp(cmd, "join")){
      /* get the user's nick */
      token = strtok(NULL, delims);
      /* there must be nick after the /join */
      if (!token){
        send_to_fd(fd, "must supply a nick!");
        goto END;
      }
      /* print some output */
      out("user %s has joined on socket %d", token, fd);
      /* add the client to the clients list */
      add_client(fd, token);
    }
    /* XXX /list */
    else if (!strcmp(cmd, "list")){
      /* iterate through all the connected clients */
      for (struct clients *client = clients; client != NULL; client = client->next){
        send_to_fd(client->fd, client->nick);
      }
    }
  }
  /* it's just a regular message */
  else {
    /* send the message to every client connected */
    for (struct clients *client = clients; client != NULL; client = client->next){
      send_to_fd(client->fd, msg);
    }
    /* and output to the servers... output */
    out("%s: %s", get_nick_by_fd(fd), msg);
  }

END:
  free(copy);
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
 * Returns a nick connected with the given <fd>
 */
char *get_nick_by_fd(int fd)
{
  for (struct clients *p = clients; p != NULL; p = p->next)
    if (p->fd == fd)
      return p->nick;
}

/*
 * Send a <msg> to the client of a given <fd>
 */
static void send_to_fd(int fd, char *msg, ...)
{
  va_list vl;
  char send_buffer[MAX_BUFFER_SIZE];

  va_start(vl, msg);
  vsprintf(send_buffer, msg, vl);

  if (send(fd, send_buffer, strlen(send_buffer), 0) == -1){
    perror("send");
  }

  va_end(vl);
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


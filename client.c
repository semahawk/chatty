/*
 *
 * client.c
 *
 * Created at:  Thu 30 May 2013 11:46:28 CEST 11:46:28
 *
 * Authors: Tomek K.     <tomicode@gmail.com>
 *          Szymon Urbaś <szymon.urbas@aol.com>
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

static bool recivethread_active = true;

void close_handler(int sig){
  recivethread_active = false;
}

void *reciveThread(void *sid){
	char recv_buffer[MAX_BUFFER_SIZE];
	int byte_count, sockid;

	sockid = *(int *)sid;
	while(recivethread_active){
    usleep(100000);

    if(!recivethread_active)
      break; // To aviod error on closed socket.

		if((byte_count = recv(sockid, recv_buffer, sizeof(recv_buffer), MSG_DONTWAIT)) == -1){
			if(errno == EWOULDBLOCK){ // When no data is on the input stream.
        continue;
      }
      perror("recv");
      break;
		}
    printw("Message: %s\n", recv_buffer);
    refresh();

    memset(&recv_buffer, 0 , sizeof(recv_buffer));
	}
  pthread_exit(NULL);
}

int client(void)
{
  WINDOW * client_wnd; // Main window variable for ncurses init.

  char join_msg[MAX_BUFFER_SIZE];
  char nick[MAX_NAME_SIZE];
  char ip[INET6_ADDRSTRLEN], buffer[MAX_BUFFER_SIZE];
  char *input;

  int status, sockid, byte_count;
  
  struct addrinfo hints;
  struct addrinfo *servinfo;

  //pthread_attr_t attr;
  pthread_t thread;

  signal(SIGINT, close_handler);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family			= AF_UNSPEC;
  hints.ai_socktype		= SOCK_STREAM;

  if((client_wnd = initscr()) == NULL){ // Init the curses window.
    perror("initscr");
    exit(EXIT_FAILURE);
  }

  keypad(client_wnd, true); // Get all function keys from the keyboard.

  //pthread_attr_init(&attr);
  //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  //printf("Client Mode - Connect\nPlease input the server IP:");
  addstr("Chatty - Client Mode\nServer IP: ");
  refresh();
  //scanf("%s", ip);
  getstr(ip); // Curses function for geting char table.
  if((status = getaddrinfo(ip, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }
  addstr("Nickname: ");
  refresh();
  getstr(nick);
  //printf("Choose your nick: ");
  //scanf("%s", nick);

  if((sockid = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if((status = connect(sockid, servinfo->ai_addr, servinfo->ai_addrlen)) != 0){
      perror("connect");
      exit(EXIT_FAILURE);
  }

  /* send the JOIN message */
  /* 6 for "/join " and 1 for the NULL byte */
  // #TomiCode: Why, not use a single byte to determine the connection type? //

  char join[6 + strlen(nick) + 1];
  sprintf(join, "/join %s", nick);
  if (send(sockid, join, strlen(join), 0) == -1){
    perror("send");
    exit(EXIT_FAILURE);
  }

	if(pthread_create(&thread, NULL, reciveThread, (void *) &sockid)){
    perror("pthread");
    exit(EXIT_FAILURE);
  }
	input = malloc(sizeof(char) * MAX_BUFFER_SIZE);
	while(true){
		memset(input, 0, MAX_BUFFER_SIZE);
		//input = fgets(input, MAX_BUFFER_SIZE - 1, stdin);
    getstr(input);
		//input[strlen(input)] = '\0';
    if(strcmp(input, "/q") == 0){
      break;
    }
		if(strlen(input)){
			byte_count = send(sockid, input, strlen(input), 0);
			//printf("Sent %lu bytes [%s]\n", strlen(input), input);
      printw("Send %lu bytes [%s]\n", strlen(input), input);
      refresh();
		}
	}

  recivethread_active = false; // Close reciving thread.
  close(sockid); // And close the socket.
	//pthread_detach(thread);
	//pthread_join(thread, NULL);

	//printf(" \e[0;34mClient Closing..\e[0m\n");
  addstr("\nClosing..");
  refresh();

  delwin(client_wnd);
  endwin();

	free(input);
  freeaddrinfo(servinfo);
  return 0;
}


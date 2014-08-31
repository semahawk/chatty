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
  struct packet recv_packet;
	int byte_count, sockid;

	sockid = *(int *)sid;
	while(recivethread_active){
    usleep(100000);

    if(!recivethread_active)
      break; // To aviod error on closed socket.

		if((byte_count = recv(sockid, &recv_packet, sizeof(struct packet), MSG_DONTWAIT)) == -1){
			if(errno == EWOULDBLOCK){ // When no data is on the input stream.
        continue;
      }
      perror("recv");
      break;
		}
    printw("%s: %s\n", recv_packet.msg.username, recv_packet.msg.message);
    refresh();

    memset(&recv_packet, 0 , sizeof(struct packet));
	}
  pthread_exit(NULL);
}

int client(void)
{
  WINDOW * client_wnd; // Main window variable for ncurses init.
  
  struct packet client_packet;

  char join_msg[MAX_BUFFER_SIZE];
  char nick[MAX_NAME_SIZE];
  char ip[INET6_ADDRSTRLEN], buffer[MAX_BUFFER_SIZE];

  int status, sockid, byte_count;
  
  struct addrinfo hints;
  struct addrinfo *servinfo;

  //pthread_attr_t attr;
  pthread_t thread;

  signal(SIGINT, close_handler);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family			= AF_INET; /* use IPv4 */
  hints.ai_socktype		= SOCK_STREAM;

  if((client_wnd = initscr()) == NULL){ // Init the curses window.
    perror("initscr");
    exit(EXIT_FAILURE);
  }

  keypad(client_wnd, true); // Get all function keys from the keyboard.

  addstr("Chatty - Client Mode\nServer IP: ");
  refresh();
  getstr(ip); // Curses function for geting char table.
  if((status = getaddrinfo(ip, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }
  addstr("Nickname: ");
  refresh();
  getstr(nick);

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
  // Client packet struct //
  client_packet.type = PACKET_CMD;
  client_packet.cmd.type = CMD_JOIN;

  strcpy(client_packet.cmd.args, nick); 
  if (send(sockid, &client_packet, sizeof(struct packet), 0) == -1){
    perror("send");
    exit(EXIT_FAILURE);
  }

	if(pthread_create(&thread, NULL, reciveThread, (void *) &sockid)){
    perror("pthread");
    exit(EXIT_FAILURE);
  }
	while(true){
    memset(&client_packet, 0, sizeof(struct packet));

    client_packet.type = PACKET_MSG;
    getstr(client_packet.msg.message);
    strcpy(client_packet.msg.username, nick);
    if(strcmp(client_packet.msg.message, "/q") == 0){
      memset(&client_packet, 0, sizeof(struct packet));
      break;
    }
		if(strlen(client_packet.msg.message)){
			byte_count = send(sockid, &client_packet, sizeof(struct packet), 0);
      printw("Send %lu bytes [%d]\n", sizeof(struct packet), byte_count);
      refresh();
		}
	}

  recivethread_active = false; // Close reciving thread.
  close(sockid); // And close the socket.
  addstr("\nClosing..");
  refresh();

  delwin(client_wnd);
  endwin();

	//free(input);
  freeaddrinfo(servinfo);
  return 0;
}


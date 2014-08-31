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
static WINDOW * message_wnd;
static WINDOW * input_wnd;
static WINDOW * status_wnd;

// Function prototypes.
int client_handler(void);

void close_handler(int sig)
{
  recivethread_active = false;
}

void *reciveThread(void *sid)
{
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
    wprintw(message_wnd, "%s: %s\n", recv_packet.msg.username, recv_packet.msg.message);
    wrefresh(message_wnd);

    memset(&recv_packet, 0 , sizeof(struct packet));
  }
  pthread_exit(NULL);
}

int client(void)
{
  WINDOW * client_wnd; // Main window variable.
  int lines, cols;
  int result;

  signal(SIGINT, close_handler);

  if((client_wnd = initscr()) == NULL){
    perror("initscr");
    exit(EXIT_FAILURE);
  }
  getmaxyx(client_wnd, lines, cols); // Get the size of the current terminal window.
  input_wnd = newwin(1, cols, lines - 1, 0);
  status_wnd = newwin(1, cols, lines - 2, 0);
  message_wnd = newwin(lines - 4, cols, 0, 0);

  result = client_handler();

  // Remove all curses windows.
  delwin(message_wnd);
  delwin(status_wnd);
  delwin(input_wnd);
  delwin(client_wnd);
  endwin();

  if(result == EXIT_FAILURE){
    perror("chatty");
    exit(result);
  }

  return result;
}

bool init_colors()
{
  if(has_colors() == FALSE){
    return FALSE;
  }
  start_color();
  // For default color handling of the terminal.
  use_default_colors();
  // Initialize color pair sets.
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  return TRUE;
}

int client_handler()
{
  struct packet client_packet; // Main client packet for data sending purpose.
  struct addrinfo *servinfo;
  struct addrinfo hints;
  char nick[MAX_NAME_SIZE];
  char ip[INET6_ADDRSTRLEN];
  int status, sockid, byte_count;
  pthread_t thread;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family     = AF_INET; /* use IPv4 */
  hints.ai_socktype   = SOCK_STREAM;

  keypad(input_wnd, true); // Get all function keys from the keyboard.
  init_colors();

  wbkgd(status_wnd, COLOR_PAIR(1));
  waddstr(input_wnd, "Server IP: ");
  wgetstr(input_wnd, ip); // Curses function for geting char table.
  if((status = getaddrinfo(ip, PORT, &hints, &servinfo)) != 0){
    return EXIT_FAILURE;
  }
  
  werase(input_wnd);
  waddstr(input_wnd, "Username: ");
  wgetstr(input_wnd, nick);
  if((sockid = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) == -1){
    return EXIT_FAILURE;
  }

  if((status = connect(sockid, servinfo->ai_addr, servinfo->ai_addrlen)) != 0){
    return EXIT_FAILURE;
  }

  wattron(status_wnd, A_BOLD);
  wprintw(status_wnd, " %s ", nick);

  wattroff(status_wnd, A_BOLD);
  wprintw(status_wnd, " IP: %s", ip);
  wrefresh(status_wnd);
  // Client packet for connecting the server.
  client_packet.type = PACKET_CMD;
  client_packet.cmd.type = CMD_JOIN;

  strcpy(client_packet.cmd.args, nick); 
  if (send(sockid, &client_packet, sizeof(struct packet), 0) == -1){
    return EXIT_FAILURE;
  }

  if(pthread_create(&thread, NULL, reciveThread, (void *) &sockid)){
    return EXIT_FAILURE;
  }
  while(true){
    memset(&client_packet, 0, sizeof(struct packet));
    client_packet.type = PACKET_MSG;
    wgetstr(input_wnd, client_packet.msg.message);
    werase(input_wnd);

    strcpy(client_packet.msg.username, nick);
    if(strcmp(client_packet.msg.message, "/q") == 0){
      memset(&client_packet, 0, sizeof(struct packet));
      break;
    }
    if(strlen(client_packet.msg.message)){
      byte_count = send(sockid, &client_packet, sizeof(struct packet), 0);
      wprintw(message_wnd, "Send %lu bytes [%d]\n", sizeof(struct packet), byte_count);
      wrefresh(message_wnd);
    }
  }

  recivethread_active = false; // Close reciving thread.
  close(sockid); // And close the socket.
  freeaddrinfo(servinfo);
  return 0;
}


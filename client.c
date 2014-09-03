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

#define CLIENT_IDLE 0x01
#define CLIENT_INSERT 0x02
#define CLIENT_COMMAND 0x03
#define KEY_ESCAPE 27

#include "chatty.h"
#include <locale.h>
#include <pwd.h>

typedef struct {
  int lines;
  int cols;
} WINDOW_SIZE;

static bool recivethread_active = true;
static WINDOW * message_wnd;
static WINDOW * input_wnd;
static WINDOW * status_wnd;

// Function prototypes.
bool read_client_console(unsigned char * buffer, bool command);
int client_handler(void);
int client_connect(char * ip_address, char * username);
int message_to_server(int socket, char * username, char * message);
int command_expand(char * buffer, char * command, char * arg);

void close_handler(int sig)
{
  recivethread_active = false;
}

void switch_client_mode(unsigned char mode)
{
  wprintw(status_wnd, " [%d] ", mode);
  if(mode == CLIENT_INSERT | mode == CLIENT_COMMAND){
    echo(); // Show what user is typping in.
    curs_set(1);
    if(mode == CLIENT_COMMAND){
      wprintw(input_wnd, ":"); // Add semicolon to the input box.
    }
  }else{
    noecho(); // Turn again echo off.
    curs_set(0);
    werase(input_wnd); // Clear the input box.
  }
  wrefresh(status_wnd);
  wrefresh(input_wnd); 
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
      wprintw(message_wnd, "Error on recv: %s [%d]\n", strerror(errno), sockid);
      wrefresh(message_wnd);
      wrefresh(input_wnd);
      //perror("recv");
      break;
    }
    wprintw(message_wnd, "%s: %s\n", recv_packet.msg.username, recv_packet.msg.message);
    wrefresh(message_wnd);
    wrefresh(input_wnd); // Go back to the input box.
    memset(&recv_packet, 0 , sizeof(struct packet));
  }
  pthread_exit(NULL);
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
  init_pair(2, COLOR_BLUE, -1);
  init_pair(3, -1, COLOR_RED);
  init_pair(4, -1, COLOR_GREEN);
  return TRUE;
}

int client(void)
{
  setlocale(LC_ALL, "");
  int result;
  WINDOW_SIZE size;

  signal(SIGINT, close_handler);
  initscr();
  cbreak();
  noecho();
  curs_set(0); // Remove cursor on start.
  getmaxyx(stdscr, size.lines, size.cols); // Get the size of the current terminal window.
  input_wnd = newwin(1, size.cols, size.lines - 1, 0);
  status_wnd = newwin(1, size.cols, size.lines - 2, 0);
  message_wnd = newwin(size.lines - 2, size.cols, 0, 0);
  scrollok(message_wnd, TRUE); // Enable the scrolling of the message window.
  keypad(input_wnd, true); // Get all function keys from the keyboard.
  set_escdelay(1);
  init_colors(); // Initialize all color pairs and other functions.
  wbkgd(status_wnd, COLOR_PAIR(1)); // Set the background and foreground color.

  result = client_handler();
  delwin(message_wnd);
  delwin(status_wnd);
  delwin(input_wnd);
  endwin(); // Close curses and return to normal mode.
  if(result == EXIT_FAILURE){
    perror("chatty");
    exit(result);
  }
  return result;
}

void client_msg_start()
{
  wprintw(status_wnd, " [Not connected]");
  wattron(message_wnd, COLOR_PAIR(2));
  wprintw(message_wnd,
          "      _           _   _\n"
          "  ___| |__   __ _| |_| |_ _   _\n"
          " / __| '_ \\ / _` | __| __| | | |\n"
          "| (__| | | | (_| | |_| |_| |_| |\n"
          " \\___|_| |_|\\__,_|\\__|\\__|\\__, |\n"
          "                          |___/\n"
          " ver: %s \n\n"
          "type :connect <ip address> to connect to a server.\n"
          "type :q to exit the application.\n\n", VERSION);
  wattroff(message_wnd, COLOR_PAIR(2));
  wrefresh(status_wnd);
  wrefresh(message_wnd);
  wrefresh(input_wnd);
}

int client_handler()
{
  struct packet client_packet; // Main client packet for data sending purpose.
  char nick[MAX_NAME_SIZE];
  unsigned char buffer[MAX_BUFFER_SIZE]; // Fixme: but later.. XD
  char ip[INET6_ADDRSTRLEN];
  bool function_escape;
  int socket, byte_count;
  struct passwd * pwd;
  pthread_t thread;
  unsigned char client_mode;

  {
    pwd = getpwuid(geteuid());
    strncpy(nick, pwd->pw_name, MAX_NAME_SIZE); // Copy the name from the passwd pointer.
  }
  client_mode = CLIENT_IDLE; // Client should now wait for commands.
  client_msg_start(); // Display the f*cking awesome ASCII art ;) 
  socket = -1;

  while(true){
    if(client_mode == CLIENT_IDLE){
      switch(wgetch(input_wnd)){
        case 'i':
          if(socket == -1){
            waddstr(message_wnd, "You are not connected to a server!\n");;
            wrefresh(message_wnd);
            wrefresh(input_wnd);
            break;
          }
          client_mode = CLIENT_INSERT;
          switch_client_mode(client_mode);
          break;
        case ':':
          client_mode = CLIENT_COMMAND;
          switch_client_mode(client_mode);
          break;
        default:
          continue;
      }
    }
    else if(client_mode == CLIENT_COMMAND){
      char command[32], args[32];
      int result;
      memset(buffer, 0, MAX_BUFFER_SIZE);
      if(read_client_console(buffer, true)){
        if((result = command_expand(buffer, command, args)) != -1){
          if(strcmp(command, "q") == 0 || strcmp(command, "quit") == 0){
            break;
          }
          if(strcmp(command, "connect") == 0){
            if((socket = client_connect(args, nick)) == -1){
              wprintw(message_wnd, "Error occured: %s\n", strerror(errno));
            }else{
              pthread_create(&thread, NULL, reciveThread, (void *) &socket);
              wprintw(message_wnd, "Connected to: %s\n", args);
            }
            wrefresh(message_wnd);
            wrefresh(input_wnd);
          }
        }
      }
      client_mode = CLIENT_IDLE;
      switch_client_mode(client_mode);
    }else if(client_mode == CLIENT_INSERT){
      memset(buffer, 0, MAX_BUFFER_SIZE);
      while(read_client_console(buffer, false)){
        byte_count = message_to_server(socket, nick, buffer);
        wprintw(message_wnd, " Send: %d bytes\n", byte_count);
        wrefresh(message_wnd);
        wrefresh(input_wnd);
        memset(buffer, 0, MAX_BUFFER_SIZE);
      }
      client_mode = CLIENT_IDLE;
      switch_client_mode(client_mode);
    }
  }
  recivethread_active = false; // Close reciving thread.
  if(socket != -1){
    close(socket);
  }
  return 0;
}

bool read_client_console(unsigned char * buffer, bool command)
{
  int char_num = 0, key_char = ' ';

  while((key_char = wgetch(input_wnd)) != KEY_ESCAPE){
    if(key_char == '\n' || char_num >= MAX_BUFFER_SIZE - 1){
      werase(input_wnd);
      return true;
    }
    else if(key_char == KEY_BACKSPACE){
      if(char_num > 0){
        char_num--;
        *(buffer + char_num) = 0x0;
        wdelch(input_wnd);
        if(*(buffer + char_num - 1) >= 0xC2 && *(buffer + char_num - 1) <= 0xDF){
          char_num--;
          *(buffer + char_num) = 0x0;
        }

      }else if(command && char_num == 0){
        wmove(input_wnd, 0, 1); // Fix: Remove semicolon from console window.
      }
    }
    else{
      *(buffer + char_num) = key_char;
      char_num++;
      if(key_char >= 0xC2 && key_char <= 0xDF) { // Chatch a multi-byte character.
        *(buffer + char_num) = wgetch(input_wnd);
        char_num++;
      }
    }
  }
  return false;
}

int command_expand(char * buffer, char * command, char * arg)
{
  bool current_cmd = true;
  int arg_begining = 0;
  if(strlen(buffer) == 0){
    return -1;
  }
  memset(command, 0, strlen(command));
  memset(arg, 0, strlen(arg));

  for(unsigned int i = 0; i < strlen(buffer); i++){
    if(current_cmd == true && *(buffer + i) == ' '){
      current_cmd = false;
      arg_begining = i + 1;
      continue;
    }
    if(current_cmd){
      *(command + i) = *(buffer + i);
    }else{
      *(arg + (i - arg_begining)) = *(buffer + i);
    }
  }
  if(current_cmd){
    return 0;
  }else{
    return 1;
  }
}
 
int message_to_server(int socket, char * username, char * message)
{
  struct packet message_packet;
 
  if(strlen(message) == 0){
    return -1;
  }
  message_packet.type = PACKET_MSG;
  strcpy(message_packet.msg.username, username);
  strncpy(message_packet.msg.message, message, MAX_BUFFER_SIZE - MAX_NAME_SIZE - 1);

  return send(socket, &message_packet, sizeof(struct packet), 0);
}

int client_connect(char * ip_address, char * username)
{
  struct packet client_packet; 
  struct addrinfo *servinfo;
  struct addrinfo hints;
  int socket_id;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family     = AF_INET; /* use IPv4 */
  hints.ai_socktype   = SOCK_STREAM;

  if(getaddrinfo(ip_address, PORT, &hints, &servinfo) != 0){
    return -1;
  }
  if((socket_id = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) == -1){
    return -1;
  }
  if(connect(socket_id, servinfo->ai_addr, servinfo->ai_addrlen) != 0){
    return -1;
  }

  // Client packet for connecting the server.
  memset(&client_packet, 0, sizeof(struct packet));
  client_packet.type = PACKET_CMD;
  client_packet.cmd.type = CMD_JOIN;

  strcpy(client_packet.cmd.args, username); 
  if (send(socket_id, &client_packet, sizeof(struct packet), 0) == -1){
    return -1;
  }

  freeaddrinfo(servinfo);
  return socket_id;
}

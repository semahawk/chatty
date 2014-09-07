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
#define MAX_DATA_SIZE 32

#include "chatty.h"
#include <locale.h>
#include <pwd.h>

typedef struct {
  int lines;
  int cols;
} WINDOW_SIZE;

struct client_str {
  char nick[MAX_NAME_SIZE];
  char ip[INET_ADDRSTRLEN];
  unsigned char mode;
  int socket;
};

static bool recivethread_active = true;
static WINDOW * message_wnd;
static WINDOW * input_wnd;
static WINDOW * status_wnd;
static WINDOW * system_wnd;

// Function prototypes.
bool read_client_console(unsigned char * buffer, bool command);
int client_handler(void);
int client_connect(char * ip_address, char * username);
int command_expand(char * buffer, char * arg);
int message_to_server(int socket, char * username, char * message);

void close_handler(int sig)
{
  recivethread_active = false;
}

void update_status_window(struct client_str * client)
{ 
  if(client->mode == CLIENT_INSERT){
    wbkgd(status_wnd, COLOR_PAIR(13)); 
  }else{
    wbkgd(status_wnd, COLOR_PAIR(10)); 
  }
  werase(status_wnd);
  if(client->mode == CLIENT_INSERT){
    wattron(status_wnd, COLOR_PAIR(15) | A_BOLD);
    waddstr(status_wnd, " INSERT ");
    wattroff(status_wnd, COLOR_PAIR(15) | A_BOLD);
    wattron(status_wnd, COLOR_PAIR(14) | A_BOLD);
  }else if(client->mode == CLIENT_COMMAND || client->mode == CLIENT_IDLE){
    wattron(status_wnd, COLOR_PAIR(12) | A_BOLD);
    waddstr(status_wnd, " NORMAL ");
    wattroff(status_wnd, COLOR_PAIR(12) | A_BOLD);
    wattron(status_wnd, COLOR_PAIR(11) | A_BOLD);
  }
  wprintw(status_wnd, " %s ", client->nick);
  if(client->mode == CLIENT_INSERT){
    wattroff(status_wnd, COLOR_PAIR(11) | A_BOLD);
  }else{
    wattroff(status_wnd, COLOR_PAIR(14) | A_BOLD);
  }
  
  if(strlen(client->ip) > 0){
    wprintw(status_wnd, " connected: %s ", client->ip);
  }else{
    waddstr(status_wnd, " not connected ");
  }
  wrefresh(status_wnd);
  wrefresh(input_wnd);
}

void switch_client_mode(struct client_str * client)
{
  if(client->mode == CLIENT_INSERT | client->mode == CLIENT_COMMAND){
    echo(); // Show what user is typping in.
    curs_set(1);
    if(client->mode == CLIENT_COMMAND){
      wprintw(input_wnd, ":"); // Add semicolon to the input box.
    }
  }else{
    noecho(); // Turn again echo off.
    curs_set(0);
    werase(input_wnd); // Clear the input box.
  }
  update_status_window(client);
}

void print_line(int color_p, char * out, char * format, ...)
{
  char msg[64];
  int char_num;
  va_list args;
  
  int a, b;

  va_start(args, format);
  char_num = vsnprintf(msg, 64, format, args);
  wattron(message_wnd, COLOR_PAIR(color_p) | A_BOLD);
  if(out == NULL){
    wprintw(message_wnd, " chatty: ");
  }else{
    wprintw(message_wnd, " %s: ", out);
  }
  wattroff(message_wnd, A_BOLD);
  wprintw(message_wnd, "%s\n", msg);
  wattroff(message_wnd, COLOR_PAIR(color_p));
  wrefresh(message_wnd);
  wrefresh(input_wnd);

  va_end(args);
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
      print_line(3, "recv", "Error: %s, socket: %d", strerror(errno));
      break;
    }
    if(recv_packet.type == PACKET_SRV){
      if(recv_packet.srv.error == SRV_NOERR){
        print_line(5, "-", recv_packet.srv.message); 
      }else if(recv_packet.srv.error == SRV_ERR){
        print_line(3, "-", recv_packet.srv.message);
      }
    }else{
      print_line(0, recv_packet.msg.username, recv_packet.msg.message);
    }
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
  init_pair(0, COLOR_WHITE, -1);
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  init_pair(2, COLOR_BLUE, -1);
  init_pair(3, COLOR_RED, -1);
  init_pair(4, COLOR_GREEN, -1);
  init_pair(5, 242, -1);

  init_pair(10, 244, 236);
  init_pair(11, COLOR_WHITE, 242);
  init_pair(12, 22, 148);

  init_pair(13, 80, 25);
  init_pair(14, COLOR_WHITE, 32);
  init_pair(15, 25, 255);
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
  message_wnd = newwin(size.lines - 3, size.cols, 1, 0);
  system_wnd = newwin(1, size.cols, 0, 0);
  scrollok(message_wnd, TRUE);
  keypad(input_wnd, true); // Get all function keys from the keyboard.
  set_escdelay(1);
  init_colors(); // Initialize all color pairs and other functions.
  result = client_handler();
  delwin(message_wnd);
  delwin(status_wnd);
  delwin(input_wnd);
  delwin(system_wnd);
  endwin(); // Close curses and return to normal mode.
  if(result == EXIT_FAILURE){
    perror("chatty");
    exit(result);
  }
  return result;
}

void client_msg_start()
{
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
  wrefresh(message_wnd);
  wrefresh(input_wnd);
}

int client_handler()
{
  struct packet client_packet; // Main client packet for data sending purpose.
  struct client_str * client; // The main client struct.. with the nickname and socket id.
  unsigned char buffer[MAX_BUFFER_SIZE]; // Fixme: but later.. XD
  bool function_escape;
  int byte_count;
  struct passwd * pwd;
  pthread_t thread;
  unsigned char client_mode;

  client = malloc(sizeof(struct client_str));
  {
    pwd = getpwuid(geteuid());
    strncpy(client->nick, pwd->pw_name, MAX_NAME_SIZE); // Copy the name from the passwd pointer.
  }

  client_msg_start(); // Display the f*cking awesome ASCII art ;) 
  client->mode = CLIENT_IDLE; // Client should now wait for commands.
  client->socket = -1;
  update_status_window(client);

  while(true){
    if(client->mode == CLIENT_IDLE){
      switch(wgetch(input_wnd)){
        case 'i':
          if(client->socket == -1){
            print_line(5, NULL, "You are not connected to a server!");
            break;
          }
          client->mode = CLIENT_INSERT;
          switch_client_mode(client);
          break;
        case ':':
          client->mode = CLIENT_COMMAND;
          switch_client_mode(client);
          break;
        default:
          continue;
      }
    }
    else if(client->mode == CLIENT_COMMAND){
      char args[MAX_DATA_SIZE];
      int result;
      memset(buffer, 0, MAX_BUFFER_SIZE);
      if(read_client_console(buffer, true)){
        if((result = command_expand(buffer, args)) != -1){
          if(strcmp(buffer, "q") == 0 || strcmp(buffer, "quit") == 0){
            break;
          }
          else if(strcmp(buffer, "connect") == 0){
            if((client->socket = client_connect(args, client->nick)) == -1){
              print_line(3, "Error occured", "%s, %s", strerror(errno), args);
            }else{
              strcpy(client->ip, args);
              pthread_create(&thread, NULL, reciveThread, (void *) &client->socket);
              print_line(3, NULL, "Connected to %s", args);
            }
          }
          else{
            print_line(3, NULL, "unknown command '%s'.", buffer);
          }
        }
      }
      client->mode = CLIENT_IDLE;
      switch_client_mode(client);
    }else if(client->mode == CLIENT_INSERT){
      memset(buffer, 0, MAX_BUFFER_SIZE);
      while(read_client_console(buffer, false)){
        byte_count = message_to_server(client->socket, client->nick, buffer);
        memset(buffer, 0, MAX_BUFFER_SIZE);
      }
      client->mode = CLIENT_IDLE;
      switch_client_mode(client);
    }
  }
  recivethread_active = false; // Close reciving thread.
  if(client->socket != -1){
    close(client->socket);
  }
  free(client);
  return 0;
}

bool read_client_console(unsigned char * buffer, bool command)
{
  int char_num = 0, key_char = ' ';
  while((key_char = wgetch(input_wnd)) != KEY_ESCAPE){
    if(key_char == '\n' || char_num >= MAX_BUFFER_SIZE - 1){
      *(buffer + (char_num)) = 0x0;
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

int command_expand(char * buffer, char * arg)
{
  bool current_cmd = true;
  int arg_begining = 0;
  unsigned int buffer_length;
  if((buffer_length = strlen(buffer)) == 0){
    return -1;
  }
  memset(arg, 0, MAX_DATA_SIZE);
  for(unsigned int i = 0; i < buffer_length; i++){
    if(current_cmd == true && *(buffer + i) == ' '){
      current_cmd = false;
      arg_begining = i + 1;
      *(buffer + i) = '\0';
      continue;
    }
    if(current_cmd){
      continue;
    }else{
      *(arg + (i - arg_begining)) = *(buffer + i);
      *(buffer + i) = '\0';
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

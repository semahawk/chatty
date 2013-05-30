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

int client(void)
{
  char nick[64];
  char ip[INET6_ADDRSTRLEN], buffer[256];
  int status, sockid, byte_count;
  struct addrinfo hints;
  struct addrinfo *servinfo;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family			= AF_UNSPEC;
  hints.ai_socktype		= SOCK_STREAM;

  printf("Client Mode - Connect\nPlease input the server IP: ");
  scanf("%s", ip);
  if((status = getaddrinfo(ip, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }

  printf("Choose your nick: ");
  scanf("%s", nick);

  if((sockid = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if((status = connect(sockid, servinfo->ai_addr, servinfo->ai_addrlen)) != 0){
    perror("connect");
    exit(EXIT_FAILURE);
  }
  memset(&buffer, 0, sizeof(buffer));
  if (send(sockid, nick, strlen(nick), 0) == -1){
    perror("send");
    exit(EXIT_FAILURE);
  }

  if((byte_count = recv(sockid, buffer, sizeof(buffer), 0)) == -1){
    perror("recv");
    exit(EXIT_FAILURE);
  }

  printf("Recived data: %s\n", buffer);

  freeaddrinfo(servinfo);
  return 0;
}


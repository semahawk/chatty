/*
 *
 * client.c
 *
 * Created at:  Thu 30 May 2013 11:46:28 CEST 11:46:28
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

#include "chatty.h"

static bool mainLoop = true;

void closeHandler(int sig){
	mainLoop = false;
}

void *reciveThread(void *sid){
	char recv_buffer[256];
	int byte_count, *sockid;
	int obj_count = 5; //FOR debug only;
	sockid = (int *) sid;
	memset(&recv_buffer, 0 , sizeof(recv_buffer));
	while(obj_count > 0){
		if((byte_count = recv(*sockid, recv_buffer, sizeof(recv_buffer), 0)) == -1){
			perror("recv");
		}
		printf("\e[0;34mMessage:\e[0;0m%s\n", recv_buffer);
		obj_count--;
	}
}

int client(void)
{
  char ip[INET6_ADDRSTRLEN], buffer[256];
  int status, sockid, byte_count;
	int thread_stat;
	char *input;
  struct addrinfo hints;
  struct addrinfo *servinfo;
	pthread_t threads[THREADS];

	signal(SIGINT, closeHandler);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family			= AF_UNSPEC;
  hints.ai_socktype		= SOCK_STREAM;

  printf("Client Mode - Connect\nPlease input the server IP:");
  scanf("%s", ip);
  if((status = getaddrinfo(ip, PORT, &hints, &servinfo)) != 0){
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }

  if((sockid = socket(servinfo->ai_family, servinfo->ai_socktype, 0)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if((status = connect(sockid, servinfo->ai_addr, servinfo->ai_addrlen)) != 0){
    perror("connect");
    exit(EXIT_FAILURE);
  }
	thread_stat = pthread_create(&threads[0], NULL, reciveThread, (void *) &sockid);
	input = malloc(sizeof(char) * 129);
	while(mainLoop){
		memset(input, 0, sizeof(input));
		input = fgets(input, 128, stdin);
		input[strlen(input) - 1] = '\0';
		if(strlen(input) > 1){
			byte_count = send(sockid, input, sizeof(input), 0);
			printf(" Sended %d bytes [%s]\n", sizeof(input), input);
		}
	}
	printf(" \e[0;34mClient Closing..\e[0m\n");
  freeaddrinfo(servinfo);
  return 0;
}




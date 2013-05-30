/*
 *
 * chatty.c
 *
 * Created at:  Tue 28 May 2013 22:37:04 CEST 22:37:04
 *
 * Author:  Szymon Urbaś <szymon.urbas@aol.com> and Tomek K. <tomicode@gmail.com>
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

int main(int argc, char *argv[])
{
  /* used for getopt */
  int c;
  /* -s flag */
  short run_as_server;

  while ((c = getopt(argc, argv, "sv")) != -1){
    switch (c){
      case 's':
        run_as_server = 1;
        break;
      case 'v':
        fprintf(stdout, "Chatty v" VERSION " " __DATE__ " " __TIME__ "\n");
        return EXIT_SUCCESS;
      case '?':
        return EXIT_FAILURE;
      default:
        abort();
    }
  }

  /* doing it server style! */
  if (run_as_server){
    return server();
  }
  return client();
}

/*
 * Returns the IP number
 */
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET){
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


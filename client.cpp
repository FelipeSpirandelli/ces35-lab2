/*
File use: ./client <hostname> <filename>
Example: ./client localhost teste.html
*/

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree */
#define BUFSIZE 4096     /* block transfer size */
#define REQUESTSIZE 100

// argv -> host name, filename
int main(int argc, char **argv)
{
  int c, s, bytes;
  char request[REQUESTSIZE];
  char buf[BUFSIZE];          /* buffer for incoming file */
  struct hostent *h;          /* info about server */
  struct sockaddr_in channel; /* holds IP address */
  if (argc < 4)
  {
    printf("Usage: client <server-name> <user-id> <options>\n");
    exit(-1);
  }
  h = gethostbyname(argv[1]); /* look up host’s IP address */
  if (!h)
  {
    printf("gethostbyname failed to locate %s", argv[1]);
    exit(-1);
  }

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0)
  {
    printf("socket call failed");
    exit(-1);
  }
  memset(&channel, 0, sizeof(channel));
  channel.sin_family = AF_INET;
  memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
  channel.sin_port = htons(SERVER_PORT);
  c = connect(s, (struct sockaddr *)&channel, sizeof(channel));
  if (c < 0)
  {
    printf("connect failed");
    exit(-1);
  }
  /* Connection is now established. Send file name including 0 byte at end. */

  // Now send the request
  if (!strcmp(argv[3], "-f")) { // myget request
    strcpy(request, "MyGet");
    strcat(request, "\nID ");
    strcat(request, argv[2]);
    strcat(request, "\nURL ");
    strcat(request, argv[4]);
    write(1, "Sending MyGet Request: \n\n", 25);
    write(1, request, strlen(request));
  }
  if (!strcmp(argv[3], "-n")) {
    strcpy(request, "MyLastAccess\n");
    strcat(request, "ID ");
    strcat(request, argv[2]);
    write(1, "Sending MyLastAccess Request: \n\n", 33);
    write(1, request, strlen(request));
  }
  write(s, request, strlen(request));
  write(1, "\n\nRequest successfully sent\n\n", 30);

  /* Go get the file and write it to standard output.*/
  while (1)
  {
    bytes = read(s, buf, BUFSIZE); /* read from socket */
    if (bytes <= 0)
    {
      exit(0); /* check for end of file */
    }
    write(1, "Received Response from server:\n\n", 33);
    write(1, buf, bytes); /* write to standard output */
    write(1,"\n",2);
  }
}

// TODOS are marked as TODO
// TODO - ORGANIZE CODE BETTER
// TODO - create route handler

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore.h>
#include <pthread.h>

#define SERVER_PORT 8080 /* arbitrary, but client & server must agree*/
#define BUF_SIZE 4096    /* block transfer size */
#define QUEUE_SIZE 10
#define N_THREADS 5

sem_t mutex, spots;
int possibleThreadIndex[N_THREADS];
int lastAccess = -1;

struct my_get_args
{
   int sa;
   char *filename;
   int tidx;
};

int getLastThreadIndex()
{
   // Only get mutex if spots is possible.
   // Order is important, otherwise get mutex and deadlock
   sem_wait(&spots);
   sem_wait(&mutex);
   // critical section
   int aux = -1;
   for (int i = 0; i < N_THREADS; i++)
   {
      if (possibleThreadIndex[i] != -1)
      {
         aux = possibleThreadIndex[i];
         possibleThreadIndex[i] = -1;
         break;
      }
   }
   sem_post(&mutex);
   return aux;
}

void returnThreadIndex(int idx)
{
   sem_wait(&mutex);

   for (int i = 0; i < N_THREADS; i++)
   {
      if (possibleThreadIndex[i] == -1)
      {
         possibleThreadIndex[i] = idx;
         break;
      }
   }
   sem_post(&spots);
   sem_post(&mutex);
}

void *myGet(void *arguments)
{
   my_get_args *args = (my_get_args *)arguments;
   char buf[BUF_SIZE];
   /* Get and return the file. */

   char buffer[50];
   int length = sprintf(buffer, "Reading file %s\n", args->filename);
   write(1, buffer, length);

   int fd = open(args->filename, O_RDONLY); /* open the file to be sent back */
   if (fd < 0)
      printf("open failed");
   write(1, "Open of file was successful\n", 28);
   while (1)
   {
      int bytes = read(fd, buf, BUF_SIZE); /* read from file */
      if (bytes <= 0)
         break; /* check for end of file */
      write(1, "Wrote bytes to socket\n", 23);
      write(args->sa, buf, bytes); /* write bytes to socket */
   }
   close(fd); /* close file */
   returnThreadIndex(args->tidx);
   close(args->sa);
   pthread_exit(NULL);
}

void init_threads()
{
   // Create possible indexes
   for (int i = 0; i < N_THREADS; i++)
   {
      possibleThreadIndex[i] = i;
   }
   // Create mutex
   int n = sem_init(&mutex, 0, 1);
   if (n != 0)
   {
      printf("mutex init failed");
      exit(-1);
   }
   n = sem_init(&spots, 0, N_THREADS);
   if (n != 0)
   {
      printf("mutex init failed");
      exit(-1);
   }
}

int main(int argc, char *argv[])
{
   int s, b, l, sa, on = 1;
   pthread_t file_thread[N_THREADS];
   init_threads();
   char buf[BUF_SIZE];         /* buffer for outgoing file */
   struct sockaddr_in channel; /* holds IP address */
   /* Build address structure to bind to socket. */
   memset(&channel, 0, sizeof(channel));
   /* zero channel */
   channel.sin_family = AF_INET;
   channel.sin_addr.s_addr = htonl(INADDR_ANY);
   channel.sin_port = htons(SERVER_PORT);
   /* Passive open. Wait for connection. */
   s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
   if (s < 0)
   {
      printf("socket call failed");
      exit(-1);
   }
   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

   b = bind(s, (struct sockaddr *)&channel, sizeof(channel));
   if (b < 0)
   {
      printf("bind failed");
      exit(-1);
   }

   l = listen(s, QUEUE_SIZE); /* specify queue size */
   if (l < 0)
   {
      printf("listen failed");
      exit(-1);
   }
   /* Socket is now set up and bound. Wait for connection and process it. */
   while (1)
   {
      write(1, "Accepting connections\n", 22);
      sa = accept(s, 0, 0); /* block for connection request */
      if (sa < 0)
      {
         printf("accept failed\n");
         exit(-1);
      }

      write(1, "Connection accepted\n", 21);

      read(sa, buf, BUF_SIZE); /* read file name from socket */
      int idx = getLastThreadIndex();
      my_get_args *arguments = (my_get_args *)malloc(sizeof(my_get_args));
      if (arguments != NULL)
      {
         // Initialize the structure
         arguments->sa = sa;
         // TODO - CHANGE FILENAME TO A SEPARATE BUFFER, OTHERWISE PARALLELISM IS IMPOSSIBLE
         arguments->filename = buf;
         arguments->tidx = idx;
      }
      pthread_create(&file_thread[idx], NULL, myGet, (void *)arguments);
   }
}

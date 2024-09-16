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
   //char *request;
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
   char request_buf[BUF_SIZE];
   char response_buf[BUF_SIZE];
   /* Get and return the file. */

   read(args->sa, request_buf, BUF_SIZE); /* read file name from socket */

   char buffer[100];
   strncpy(buffer, request_buf, sizeof(buffer) - 1);
   buffer[sizeof(buffer) - 1] = '\0';

   char* token = strtok(buffer, " \n");
   if (token == NULL) {
      write(args->sa, "Bad Request", 12);
      returnThreadIndex(args->tidx);
      close(args->sa);
      pthread_exit(NULL);
   }

   //HANDLING THE PROTOCOLS
   if (!strcmp(token, "MyLastAccess")) {
      write(1, "Handling LastAccess Request...\n", 32);
      token = strtok(NULL, " \n");
      if (token == NULL) {
         write(args->sa, "Bad Request, no user ID\n", 25);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }
      if (strcmp(token, "ID")) {
         write(args->sa, "Bad Request, no ID field\n", 26);
      }
      token = strtok(NULL, " \n");
      if (token == NULL) {
         write(args->sa, "Bad Request, no user ID\n", 25);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }

      //TODO: Write the accessCount instead of the ID
      write(args->sa, token, strlen(token));
      returnThreadIndex(args->tidx);
      close(args->sa);
      pthread_exit(NULL);
   }

   else if (!strcmp(token, "MyGet")) {
      write(1,"Handling MyGet Request...\n", 27);
      token = strtok(NULL, " \n"); //get the next word
      if (token == NULL) {
         write(args->sa, "Bad MyGet Request, no URL field\n", 33);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }
      if (strcmp(token, "ID")) {
         write(args->sa, "Bad MyGet Request, no ID field\n", 32);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }
      token = strtok(NULL, " \n");
      // Here, token should be the ID. TODO: Increment the counter for that ID
      if (token == NULL) {
         write(args->sa, "Bad MyGet Request, no user ID\n", 31);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }

      token = strtok(NULL, " \n");

      if (!strcmp(token, "URL")) {
         write(1, "Reading URL field...\n", 21);
         token = strtok(NULL, " \n");
         if (token == NULL) {
            write(args->sa, "Bad MyGet Request, no URL provided in the URL field\n", 53);
            returnThreadIndex(args->tidx);
            close(args->sa);
            pthread_exit(NULL);
         }

         // find the file
         int fd = open(token, O_RDONLY); /* open the file to be sent back */
         if (fd < 0) {
            write(1, "Failed to locate the file, closing the connection\n", 51);
            write(args->sa, "File was not found\n", 20);
            returnThreadIndex(args->tidx);
            close(args->sa);
            pthread_exit(NULL);
         }

         write(1, "File successfully opened\n", 26);
         while (1) {
            int bytes = read(fd, response_buf, BUF_SIZE); /* read from file */
            if (bytes <= 0)
               break; /* check for end of file */
            write(1, "Wrote bytes to socket\n", 23);
            write(args->sa, response_buf, bytes); /* write bytes to socket */
         }
         close(fd);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      } else {
         write(args->sa, "Unexpected field\n", 18);
         returnThreadIndex(args->tidx);
         close(args->sa);
         pthread_exit(NULL);
      }
   }

   else { // NO PROTOCOL MATCH
      write(args->sa, "Incorrect usage of protocol", 28);
      returnThreadIndex(args->tidx);
      close(args->sa);
      pthread_exit(NULL);
   }
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
      write(1, "Accepting connections...\n", 26);
      sa = accept(s, 0, 0); /* block for connection request */
      if (sa < 0)
      {
         printf("accept failed\n");
         exit(-1);
      }


      int idx = getLastThreadIndex();

      write(1, "Connection accepted\n", 21);
      my_get_args *arguments = (my_get_args *)malloc(sizeof(my_get_args));
      if (arguments != NULL)
      {
         // Initialize the structure
         arguments->sa = sa;
         arguments->tidx = idx;
      }
      pthread_create(&file_thread[idx], NULL, myGet, (void *)arguments);
   }
}

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "common.h"

typedef struct
{
   int fd;
   char name[MAX_NAME_LEN];
   int has_name;
} client_t;

typedef struct
{
   int listen_fd;
   client_t clients[MAX_CLIENTS];
   fd_set readfds;
   int max_sd;
} server_t;

void server_init (server_t *s);
void server_handle_new_connection (server_t *s);
void server_broadcast (server_t *s, const char *msg);
void server_handle_client_data (server_t *s, int index);
void server_update (server_t *s);

void
server_init (server_t *s)
{
   struct sockaddr_in address;
   int opt = 1;

   for (int i = 0; i < MAX_CLIENTS; i++)
      {
         s->clients[i].fd       = 0;
         s->clients[i].has_name = 0;
         memset (s->clients[i].name, 0, MAX_NAME_LEN);
      }

   s->listen_fd = socket (AF_INET, SOCK_STREAM, 0);
   if (s->listen_fd < 0)
      {
         fprintf (stderr, "[ERROR] SOCKET FAILED");
         exit (1);
      }

   setsockopt (s->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

   address.sin_family      = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port        = htons (PORT);

   if (bind (s->listen_fd, (struct sockaddr *)&address, sizeof (address)) < 0)
      {
         fprintf (stderr, "[ERROR] BIND FAILED");
         exit (1);
      }

   listen (s->listen_fd, 3);
   printf ("[INFO] SERVER START\n");
}

void
server_handle_new_connection (server_t *s)
{
   struct sockaddr_in address;
   int addrlen    = sizeof (address);
   int new_socket = accept (s->listen_fd,
                            (struct sockaddr *)&address,
                            (socklen_t *)&addrlen);

   if (new_socket < 0)
      {
         return;
      }

   for (int i = 0; i < MAX_CLIENTS; i++)
      {
         if (s->clients[i].fd == 0)
            {
               s->clients[i].fd       = new_socket;
               s->clients[i].has_name = 0;
               printf ("[INFO] NEW CONN FD %d\n", new_socket);
               break;
            }
      }
}

void
server_broadcast (server_t *s, const char *msg)
{
   for (int j = 0; j < MAX_CLIENTS; j++)
      {
         if (s->clients[j].fd != 0)
            {
               send (s->clients[j].fd, msg, strlen (msg), 0);
            }
      }
}

void
server_handle_client_data (server_t *s, int index)
{
   char buffer[BUFFER_SIZE] = { 0 };
   int sd                   = s->clients[index].fd;
   int valread              = read (sd, buffer, BUFFER_SIZE - 1);

   if (valread <= 0)
      {
         printf ("[INFO] DISCONNECT FD %d (%s)\n", sd, s->clients[index].name);
         close (sd);
         s->clients[index].fd       = 0;
         s->clients[index].has_name = 0;
      }
   else
      {
         buffer[valread] = '\0';

         if (!s->clients[index].has_name)
            {
               int is_name_taken = 0;
               for (int i = 0; i < MAX_CLIENTS; i++)
                  {
                     if (s->clients[i].has_name
                         && strcmp (s->clients[i].name, buffer) == 0)
                        {
                           is_name_taken = 1;
                           break;
                        }
                  }

               if (is_name_taken)
                  {
                     send (sd, "NAME_TAKEN", 14, 0);
                     printf ("[INFO] REJECT DUPLICATE NAME -- %s\n", buffer);
                  }
               else
                  {

                     int name_len = (valread < MAX_NAME_LEN - 1)
                                        ? valread
                                        : MAX_NAME_LEN - 1;
                     memcpy (s->clients[index].name, buffer, name_len);
                     s->clients[index].name[name_len] = '\0';
                     s->clients[index].has_name       = 1;

                     send (sd, "NAME_OK", 7, 0);
                     printf ("[INFO] FD %d SET NAME -- %s\n",
                             sd,
                             s->clients[index].name);

                     char join_msg[MAX_NAME_LEN + 24];

                     snprintf (join_msg,
                               sizeof (join_msg),
                               "[SYSTEM] %s JOINED THE CHAT",
                               s->clients[index].name);
                     server_broadcast(s, join_msg);
                  }
            }
         else
            {
               char broadcast_msg[BUFFER_SIZE + MAX_NAME_LEN + 8];
               snprintf (broadcast_msg,
                         sizeof (broadcast_msg),
                         "[%s] %s",
                         s->clients[index].name,
                         buffer);
               server_broadcast (s, broadcast_msg);
            }
      }
}

void
server_update (server_t *s)
{
   FD_ZERO (&s->readfds);
   FD_SET (s->listen_fd, &s->readfds);
   s->max_sd = s->listen_fd;

   for (int i = 0; i < MAX_CLIENTS; i++)
      {
         int sd = s->clients[i].fd;
         if (sd > 0)
            {
               FD_SET (sd, &s->readfds);
            }
         if (sd > s->max_sd)
            {
               s->max_sd = sd;
            }
      }

   select (s->max_sd + 1, &s->readfds, NULL, NULL, NULL);

   if (FD_ISSET (s->listen_fd, &s->readfds))
      {
         server_handle_new_connection (s);
      }

   for (int i = 0; i < MAX_CLIENTS; i++)
      {
         if (s->clients[i].fd > 0 && FD_ISSET (s->clients[i].fd, &s->readfds))
            {
               server_handle_client_data (s, i);
            }
      }
}

int
main (void)
{
   server_t server;
   server_init (&server);

   while (1)
      {
         server_update (&server);
      }

   return 0;
}

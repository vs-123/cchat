#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "raylib.h"

typedef enum
{
   SCREEN_LOGIN,
   SCREEN_CHAT,
} screen_t;

typedef struct
{
   int socket;
   struct sockaddr_in server_address;
   screen_t screen;

   char chat_log[MAX_MSG_HISTORY][BUFFER_SIZE];
   char input_text[BUFFER_SIZE];

   short log_count;
   short input_len;
} chat_t;

void chat_init (chat_t *ch);
void chat_screen_login (chat_t *ch);
void chat_screen_chat (chat_t *ch);
void chat_screen (chat_t *ch);
void chat_die (chat_t *ch);

void
chat_init (chat_t *ch)
{
   ch->socket = socket (AF_INET, SOCK_STREAM, 0);
   if (ch->socket < 0)
      {
         fprintf (stderr, "[ERROR] SOCKET CREATION FAILED\n");
         exit (1);
      }

   ch->server_address.sin_family = AF_INET;
   ch->server_address.sin_port   = htons (PORT);
   inet_pton (AF_INET, "127.0.0.1", &ch->server_address.sin_addr);

   if (connect (ch->socket,
                (struct sockaddr *)&ch->server_address,
                sizeof (ch->server_address))
       < 0)
      {
         fprintf (stderr, "[ERROR] CONNECTION FAILED\n");
         exit (1);
      }

   int flags = fcntl (ch->socket, F_GETFL, 0);
   fcntl (ch->socket, F_SETFL, flags | O_NONBLOCK);

   InitWindow (800, 450, "CCHAT-CLIENT");
   SetTargetFPS (30);

   ch->screen    = SCREEN_LOGIN;
   ch->log_count = 0;
   ch->input_len = 0;
   memset (ch->input_text, 0, BUFFER_SIZE);
   memset (ch->chat_log, 0, sizeof (ch->chat_log));
}

void
chat_screen_login (chat_t *ch)
{
   assert (ch->screen == SCREEN_LOGIN);

   /* INPUT */

   int key = GetCharPressed ();
   while (key > 0)
      {
         if ((key >= ' ') && (key <= 'z') && (ch->input_len < 31))
            {
               ch->input_text[ch->input_len++] = (char)key;
               ch->input_text[ch->input_len]   = '\0';
            }
         key = GetCharPressed ();
      }

   if ((IsKeyPressed (KEY_BACKSPACE) || IsKeyPressedRepeat (KEY_BACKSPACE))
       && ch->input_len > 0)
      {
         ch->input_text[--ch->input_len] = '\0';
      }

   char auth_buff[BUFFER_SIZE] = { 0 };
   int bytes = recv (ch->socket, auth_buff, BUFFER_SIZE - 1, 0);
   if (bytes > 0)
      {
         printf ("[INFO] RECEIVED BYTES -- %s\n", auth_buff);
         if (strncmp (auth_buff, "NAME_OK", 7) == 0)
            {
               ch->screen    = SCREEN_CHAT;
               ch->input_len = 0;
               memset (ch->input_text, 0, BUFFER_SIZE);
               printf ("[DEBUG] NAME OK\n");
            }
         else if (strncmp (auth_buff, "NAME_TAKEN", 10) == 0)
            {
               ch->input_len = 0;
               memset (ch->input_text, 0, BUFFER_SIZE);
               printf ("[DEBUG] NAME TAKEN\n");
            }
      }

   if (IsKeyPressed (KEY_ENTER) && ch->input_len > 0)
      {
         send (ch->socket, ch->input_text, ch->input_len, 0);
         printf ("[DEBUG] SENT NAME TO CHECK\n");
      }

   BeginDrawing ();
   ClearBackground (RAYWHITE);
   DrawText ("ENTER NICKNAME", 250, 150, 20, DARKGRAY);
   DrawRectangle (250, 180, 300, 40, LIGHTGRAY);
   DrawText (ch->input_text, 260, 190, 20, BLACK);

   EndDrawing ();
}

void
chat_screen_chat (chat_t *ch)
{
   assert (ch->screen == SCREEN_CHAT);

   /* NETWORK */

   char recv_buffer[BUFFER_SIZE] = { 0 };
   int bytes_received = recv (ch->socket, recv_buffer, BUFFER_SIZE - 1, 0);

   if (bytes_received > 0)
      {
         if (ch->log_count >= MAX_MSG_HISTORY)
            {
               for (int i = 1; i < MAX_MSG_HISTORY; i++)
                  {
                     strcpy (ch->chat_log[i - 1], ch->chat_log[i]);
                  }
               ch->log_count = MAX_MSG_HISTORY - 1;
            }
         strncpy (ch->chat_log[ch->log_count++], recv_buffer, BUFFER_SIZE - 1);
      }

   /* INPUT */

   int key = GetCharPressed ();
   while (key > 0)
      {
         if ((key >= ' ') && (key <= 'z') && (ch->input_len < 64))
            {
               ch->input_text[ch->input_len++] = (char)key;
               ch->input_text[ch->input_len]   = '\0';
            }
         key = GetCharPressed ();
      }

   if ((IsKeyPressed (KEY_BACKSPACE) || IsKeyPressedRepeat (KEY_BACKSPACE))
       && ch->input_len > 0)
      {
         ch->input_text[--ch->input_len] = '\0';
      }

   if (IsKeyPressed (KEY_ENTER) && ch->input_len > 0)
      {
         send (ch->socket, ch->input_text, ch->input_len, 0);
         ch->input_len     = 0;
         ch->input_text[0] = '\0';
      }

   /* GUI */

   BeginDrawing ();
   ClearBackground (RAYWHITE);
   DrawText ("[CHAT]", 20, 15, 20, MAROON);
   for (int i = 0; i < ch->log_count; i++)
      {
         DrawText (ch->chat_log[i], 20, 50 + (i * 22), 20, DARKGRAY);
      }
   DrawRectangle (20, 400, 760, 35, LIGHTGRAY);
   DrawRectangleLines (20, 400, 760, 35, GRAY);
   DrawText (ch->input_text, 30, 408, 20, BLACK);

   /* CURSOR */

   if ((int)(GetTime () * 2) % 2 == 0)
      {
         int tw = MeasureText (ch->input_text, 20);
         DrawRectangle (32 + tw, 410, 2, 18, BLACK);
      }
   EndDrawing ();
}

void
chat_screen (chat_t *ch)
{
   switch (ch->screen)
      {
      case SCREEN_LOGIN:
         chat_screen_login (ch);
         break;
      case SCREEN_CHAT:
         chat_screen_chat (ch);
         break;
      default:
         assert (0 && "UNHANDLED SCREEN");
         break;
      }
}

void
chat_die (chat_t *ch)
{
   close (ch->socket);
   CloseWindow ();
}

int
main (void)
{
   chat_t chat = { 0 };
   chat_init (&chat);
   while (!WindowShouldClose ())
      {
         chat_screen (&chat);
      }
   chat_die (&chat);
   return 0;
}

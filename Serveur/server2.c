#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "server2.h"
#include "client2.h"
#include "struct_group.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      strcpy(buffer,"\r\n");
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }
         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);
         Client c = { csock };
         if (connect_client(&c, buffer))
         {
            clients[actual] = c;
            actual++;
         }
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer);
               }
               else 
               {
                  char bufferCopy[strlen(buffer)];
                  char bufferCopy2[strlen(buffer)];
                  char* check_if_at = strtok(strcpy(bufferCopy,buffer), "@");
                  char* check_command = strtok(strcpy(bufferCopy2,buffer), " ");
                  if (strlen(check_if_at)==strlen(buffer)-1) 
                  //if there is a '@' found, check_if_at variable is the same as the buffer for some reason
                  {
                     char* receiver_name = strtok(check_if_at, " ");                        
                     char* message_to_send = strtok(NULL,"\n");
                     send_message_with_at(clients, actual, client, receiver_name, message_to_send);
                  }
                  else if (strcmp(check_command, "#C")==0)
                  {
                     create_group(buffer, clients, client, actual);
                  }
                  else if (strcmp(check_command, "#L")==0)
                  {  
                     print_clients_list(clients, client, actual);
                  }
                  else if (strcmp(check_command, "#G")==0)
                  {  
                     print_groups_list(client);
                  }
                  else if (strcmp(check_command, "#O")==0)
                  {
                     print_options_list(client);
                  }
                  else
                  {
                     write_client(client.sock, "Please enter a valid input\n");
                  }
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static int connect_client(Client* client, char* buffer) //Client pointer to modify it
/*Try to login (if the nickname/password matches an existing one in the users_informations file), or else
refuse connection if the nickname exists but the password doesn't match, or else sign in the client if
the nickname is new*/
{
   char* nickname = strtok(buffer, "/");
   char* password = strtok(NULL, "\n");
   strcpy(client->name, nickname);
   strcpy(client->password, password);
   FILE* users_file_ptr = fopen("../Infos/users_informations.txt", "a+");
   char users_file_buffer[BUF_SIZE];
   while (!feof(users_file_ptr))
   {
      fgets(users_file_buffer, BUF_SIZE, users_file_ptr);
      if (strcmp(strtok(users_file_buffer, "/"), client->name) == 0)
      {
         if (strcmp(strtok(NULL,"\n"), client->password)==0)
         {
            write_client(client->sock, "Connection OK\n");
            fclose(users_file_ptr);
            print_connection_history(*client); //Only needed if the client re-connects
            print_options_list(*client);
            return 1;
         }
         else
         {
            write_client(client->sock, "Incorrect password\n");
            closesocket(client->sock);
            fclose(users_file_ptr);
            return 0;
         }
      }
   }
   fprintf(users_file_ptr, "%s/%s\n", client->name, client->password);
   write_client(client->sock, "Inscription OK\n");
   fclose(users_file_ptr);
   print_options_list(*client);
   return 1;
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void print_clients_list(Client* connected_clients, Client caller, int nb_connected_clients)
/*Print all the connected clients names to the caller*/
{
   char print_message[BUF_SIZE] = "";
   strcpy(print_message,"List of connected clients :\n");
   for (int i=0; i<nb_connected_clients-1; i++)
   {
      strcat(print_message,connected_clients[i].name);
      strcat(print_message, ", ");
   }
   strcat(print_message, connected_clients[nb_connected_clients-1].name);
   write_client(caller.sock, strcat(print_message,"\n"));
}

static void print_groups_list(Client caller)
/*Print all the groups the caller belongs to, by searching his/her nickname in the groups_informations file*/
{
   char groups_list[BUF_SIZE]="";
   FILE* groups_file_ptr = fopen("../Infos/groups_informations.txt", "a+");
   char groups_file_buffer[BUF_SIZE];
   char caller_name_comma[BUF_SIZE]; //To avoid matching a part of the group name
   strcpy(caller_name_comma, caller.name);
   strcat(caller_name_comma, ",");
   while (!feof(groups_file_ptr))
   {
      fgets(groups_file_buffer, BUF_SIZE, groups_file_ptr);
      if (strstr(groups_file_buffer, caller_name_comma)!=NULL)
      {
         strcat(groups_list,strtok(groups_file_buffer, "="));
         strcat(groups_list, " / ");
      }
   }
   groups_list[strlen(groups_list)-3] = '\0';
   char print_message[BUF_SIZE]="";
   strcpy(print_message,"List of groups you are a member of :\n");
   strcat(print_message, groups_list);
   strcat(print_message, "\n");
   write_client(caller.sock, print_message);
}

static void print_options_list(Client caller)
/*Print all the possible options to the caller*/
{  
   char* message = ("'@member|groupname message' : Send a message to a person|group\n");
   char* group_creation = ("'#C groupname other_member1,other_member2,other_memberX message' : Create a new discussion group\n");
   char* group_list = ("'#G' : See the list of groups you are a member of\n");
   char* clients_list = ("'#L' : See the list of connected clients\n");
   char* options_list = ("'#O' : See the list of options\n");
   char print_message[BUF_SIZE]="";
   strcpy(print_message,"\nAvailable options:\n");
   strcat(print_message, message);
   strcat(print_message, group_creation);
   strcat(print_message, group_list);
   strcat(print_message, clients_list);
   strcat(print_message, options_list);
   write_client(caller.sock, print_message);
}

static void print_connection_history (Client client)
/*Print all the history of messages the caller received (in groups or private discussions) while he/she was
logout (in 'Historiques' directory). Remove the history file at the end, to reset it.*/
{
   char history_file[BUF_SIZE] = "../Historiques/";
   strcat(history_file, client.name);
   strcat(history_file, ".txt");
   if (access(history_file, F_OK)==0)
   {
      char print_message[BUF_SIZE]="";
      char history_buffer[BUF_SIZE]="";
      write_client(client.sock, "History while logout :\n");
      FILE* fptr = fopen(history_file, "a+");
      while (!feof(fptr))
      {
         strcpy(history_buffer, "");
         fgets(history_buffer, BUF_SIZE, fptr);
         strcat(print_message, history_buffer);
      } 
      fclose(fptr);
      strcat(print_message, "\n");
      write_client(client.sock, print_message);
      remove(history_file); //Delete the logout history file
   } 
   else
   {
      write_client(client.sock, "No history while logout\n");
   }
}

static void write_message_to_disconnected_client(char* sender_name, char* receiver_name, char* message)
/* Write private message to disconnected client, both in the private discussion file (in 'Discussions'
directory), and in the client history ('Historiques' directory)*/
{
   write_discussion_in_file_private("../Discussions/", receiver_name, sender_name, message);
   char history_file[BUF_SIZE] = "../Historiques/";
   strcat(history_file, receiver_name);
   strcat(history_file, ".txt");
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   FILE* fptr = fopen(history_file, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s to you : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}

static void write_group_message_to_disconnected_client(char* sender_name, char* receiver_name, char* group_name, char* message)
/* Write group message to disconnected client, in the client history ('Historiques' directory)*/
{
   char history_file[BUF_SIZE] = "../Historiques/";
   strcat(history_file, receiver_name);
   strcat(history_file, ".txt");
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   FILE* fptr = fopen(history_file, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s to %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, group_name, message);
   fclose(fptr);
}

static void create_group(char* buffer, Client* connected_clients, Client sender, int nb_connected_clients)
/* Create a new group made of connected clients or disconnected ones, or a combination of both.
We first verify that the group name isn't taken, else we abort its creation. Then, we look for the group
members in the users_informations file, and create a list of connected ones, and a list of disconnected 
ones (as the disconnected clients don't have socket, they are just a char*). If all the members exist, we
create the group, and send messages to the members (either directly or in their history)*/
{
   Client group_members[MAX_CLIENTS];
   char disconnected_group_members[MAX_CLIENTS][BUF_SIZE];
   char temp[BUF_SIZE];
   strcpy(temp, buffer);
   strtok(temp, " ");
   char* rest = strtok(NULL,"\n");
   strcpy(temp, rest);
   strtok(temp, " ");
   char group_name[BUF_SIZE];
   strcpy(group_name, temp);
   rest = strtok(NULL,"\n");
   FILE* groups_file_ptr = fopen("../Infos/groups_informations.txt", "a+"); //Check if the group name already exists
   char groups_file_buffer[BUF_SIZE]="";
   while(!feof(groups_file_ptr))
   {
      strcpy(groups_file_buffer, "");
      fgets(groups_file_buffer, BUF_SIZE, groups_file_ptr); 
      strtok(groups_file_buffer, "=");
      if (strcmp(groups_file_buffer, group_name)==0)
      {
         write_client(sender.sock,"Group name already exists\nGroup creation aborted");
         return;
      }
   }
   fclose(groups_file_ptr);
   strcpy(temp, rest);
   char* current_person = strtok(temp, ",");
   rest = strtok(NULL,"\n");
   int connected_k=0;
   int disconnected_k=0;
   int ch = ' ';

   while(strchr(current_person, ch)==NULL)
   {
      FILE* users_file_ptr = fopen("../Infos/users_informations.txt", "a+");
      char users_file_buffer[BUF_SIZE]="";
      int receiverFound = 0;
      while(!feof(users_file_ptr))
      {
         strcpy(users_file_buffer, "");
         fgets(users_file_buffer, BUF_SIZE, users_file_ptr); 
         strtok(users_file_buffer, "/");
         if (strcmp(current_person,users_file_buffer)==0)
         {
            for (int i=0;i<nb_connected_clients; i++)
            {
               if (strcmp(current_person, connected_clients[i].name)==0)
               {
                  group_members[connected_k] = connected_clients[i];
                  connected_k++;
                  receiverFound = 1;
                  break;
               }
            }
            if (!receiverFound)
            {
               strcpy(disconnected_group_members[disconnected_k],current_person);
               disconnected_k++;
               receiverFound = 1;
            }
            break;
         }
      }
      fclose(users_file_ptr);
      if (!receiverFound)
      {
         char error_message [BUF_SIZE];
         strcpy(error_message, current_person);
         strcat(error_message, " cannot be found\nGroup creation aborted");
         write_client(sender.sock, error_message);
         return;
      }
      strcpy(temp, rest);
      current_person = strtok(temp, ",");
      rest = strtok(NULL,"\n");
   }
   current_person = strtok(temp, " ");
   rest = strtok(NULL,"\n");
   FILE* users_file_ptr = fopen("../Infos/users_informations.txt", "a+");
   char users_file_buffer[BUF_SIZE]="";
   int receiverFound = 0;
   while(!feof(users_file_ptr))
   {
      strcpy(users_file_buffer, "");
      fgets(users_file_buffer, BUF_SIZE, users_file_ptr);
      strtok(users_file_buffer, "/");
      if (strcmp(current_person, users_file_buffer)==0)
      {
         for (int i=0;i<nb_connected_clients; i++)
         {
            if (strcmp(current_person, connected_clients[i].name)==0)
            {
               group_members[connected_k] = connected_clients[i];
               connected_k++;
               receiverFound = 1;
               break;
            }
         }
         if (!receiverFound)
         {
            strcpy(disconnected_group_members[disconnected_k],current_person);
            disconnected_k++;
            receiverFound = 1;
         }
         break;
      }
   }
   fclose(users_file_ptr);
   if (!receiverFound)
   {
      char error_message [BUF_SIZE];
      strcpy(error_message, current_person);
      strcat(error_message, " cannot be found\nGroup creation aborted");
      write_client(sender.sock, error_message);
      return;
   }
   group_members[connected_k] = sender; //Ajout du client créateur du groupe à ce groupe (il est intégré au groupe qu'il crée)
   connected_k++;

   Group group_to_send = {connected_k, group_members, group_name};
   FILE* group_inscription_ptr = fopen("../Infos/groups_informations.txt", "a+");
   fprintf(group_inscription_ptr, "%s=%d:", group_name, connected_k+disconnected_k);
   for (int i=0;i<connected_k;i++)
   {
      fprintf(group_inscription_ptr, "%s,", group_members[i].name);
   }
   for (int j=0;j<disconnected_k;j++)
   {
      fprintf(group_inscription_ptr, "%s,", disconnected_group_members[j]);
   }
   fprintf(group_inscription_ptr, "\n");
   fclose(group_inscription_ptr);
   write_client(sender.sock, "Group created successfully");
   char message_to_send[BUF_SIZE];
   strcpy(message_to_send, rest);
   for (int i=0; i<disconnected_k-1; i++)
   {
      write_group_message_to_disconnected_client(sender.name, disconnected_group_members[i], group_name, message_to_send);
   }
   send_message_to_a_group(group_to_send, sender, message_to_send, 1);

}

static void write_discussion_in_file_private(char* path, char* receiver_name, char* sender_name, char* message)
/*Write the message in the private conversation file (placed in 'Discussions' directory)*/
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   char* first_name;
   char* second_name;
   if (strcmp(receiver_name, sender_name)<0)
   {
      first_name = receiver_name;
      second_name = sender_name;
   } else
   {
      first_name = sender_name;
      second_name = receiver_name;
   }
   char concatSend[BUF_SIZE]="";
   strcpy(concatSend, path);
   strcat(concatSend, first_name);
   strcat(concatSend, ";");
   strcat(concatSend, second_name);
   strcat(concatSend, ".txt");
   FILE* fptr= fopen(concatSend, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}


static void write_discussion_in_file_group(char* path, char* group_name, char* sender_name, char* message)
/*Write the message in the group file (placed in 'Groupes' directory)*/
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   char concatSend[BUF_SIZE]="";
   strcpy(concatSend, path);
   strcat(concatSend, group_name);
   strcat(concatSend, ".txt");
   FILE* fptr=fopen(concatSend, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}

static void send_message_with_at(Client* connected_clients, int nb_connected_clients, Client sender, char* receiver_name, char* message)
/*We check if the '@' is intended for a user or a group, and we send the message directly to the connected user 
or connected group members. We also write the message in the discussion/group file ('in 'Discussions' directory for 
private conversation or 'Groupes' for group conversation). For those who aren't connected, we directly write into the
history file, which will be consulted at the re-connection.
If neither the receiver nor the group can be found, we abort sending an error message.*/
{
   FILE* users_file_ptr = fopen("../Infos/users_informations.txt", "a+");
   char users_file_buffer[BUF_SIZE];
   while (!feof(users_file_ptr))
   {
      fgets(users_file_buffer, BUF_SIZE, users_file_ptr);
      if (strstr(users_file_buffer, receiver_name)!=NULL) //If the receiver is an existing user
      {
         //We search the connected user associated to receiver_name
         for (int i=0;i<nb_connected_clients; i++)
         {
            if (strcmp(connected_clients[i].name, receiver_name)==0) //Receiver is connected
            {
               fclose(users_file_ptr);
               send_message_to_one_client(connected_clients[i], sender, message);
               return;
            }
         }
         //Receiver exists but isn't connected
         fclose(users_file_ptr);
         write_message_to_disconnected_client(sender.name, receiver_name, message);
         return;
      }
   }
   //Receiver isn't a user
   fclose(users_file_ptr);
   FILE* groups_file_ptr = fopen("../Infos/groups_informations.txt", "a+");
   char groups_file_buffer[BUF_SIZE];
   while (!feof(groups_file_ptr))
   {
      fgets(groups_file_buffer, BUF_SIZE, groups_file_ptr);
      if (strstr(groups_file_buffer, receiver_name)!=NULL) //If the receiver is an existing group
      {
         char verifier[BUF_SIZE];
         strcpy(verifier, sender.name);
         strcat(verifier,",");
         if (strstr(groups_file_buffer, verifier)==NULL)
         {
            write_client(sender.sock, "You are not a member of this group\nMessage not sent\n");
            return;
         }
         strtok(groups_file_buffer, "=");
         int nb_receivers = atoi(strtok(NULL,": "));
         Client group_members[MAX_CLIENTS];
         int found_count=0;
         for (int j=0;j<nb_receivers; j++)
         {
            int found = 0;
            char* current_member = strtok(NULL, ",");
            for (int i=0;i<nb_connected_clients; i++)
            {
               if (strcmp(connected_clients[i].name, current_member)==0) //Receiver is connected
               {
                  group_members[found_count] = connected_clients[i];
                  found_count++;
                  found = 1;
                  break;
               }
            }
            if (!found)
            {
               write_group_message_to_disconnected_client(sender.name, current_member, receiver_name, message);
            }
         }
         //Receiver hasn't been found 
         fclose(groups_file_ptr);
         if (found_count>1)
         {
            Group receivers_group = {found_count, group_members, receiver_name};
            send_message_to_a_group(receivers_group, sender, message, 0);
         }
         else
         {
            write_discussion_in_file_group("../Groupes/", receiver_name, sender.name, message);
         }
         return;
      }
   }
   fclose(groups_file_ptr);
   char error_message [BUF_SIZE];
   strcpy(error_message, receiver_name);
   strcat(error_message, " cannot be found\nMessage not sent\n");
   write_client(sender.sock, error_message);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer)
/* Send a message directly to all connected clients, only used when another client or the server disconnects */
{
   for(int i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         write_client(clients[i].sock, buffer);
      }
   }
}

static void send_message_to_a_group(Group group, Client sender, const char* buffer, int group_creation)
/* Send message directly to group members and also write it in the group file (in 'Groupes' directory)*/
{
   char full_message[BUF_SIZE]="";
   char message_body[BUF_SIZE]="";
   char* path_of_msg="../Groupes/";
   if (group_creation)
   {
      strcpy(full_message, "You have been added to a new group '");
      strcat(full_message, group.group_name);
      strcat(full_message, "' composed of ");
      for (int i=0;i<group.number_members-1;i++)
      {
         strcat(full_message, group.group_members[i].name);
         strcat(full_message, ", ");
      }
      strcat(full_message, group.group_members[group.number_members-1].name);
      strcat(full_message, "\n");
   }
   strcat(full_message, sender.name);
   strcat(full_message, " to ");
   strcat(full_message, group.group_name);
   strcat(full_message, " : ");
   strcpy(message_body, buffer);
   strcat(full_message, message_body);
   for (int j=0; j<group.number_members; j++)
   {
      if (strcmp(group.group_members[j].name, sender.name)!=0)
      {
         write_client(group.group_members[j].sock, full_message);
      }
   }
   write_discussion_in_file_group(path_of_msg, group.group_name, sender.name, message_body);
}

static void send_message_to_one_client(Client receiver, Client sender, const char *buffer)
/* Send message directly to the correspondent and also write it in the private conversation file (in 'Discussions' directory)*/
{
   char message[BUF_SIZE];
   char heading[BUF_SIZE];
   char* path_of_msg="../Discussions/";
   message[0] = 0;
   heading[0] = 0;
   /* we don't send message to the sender */
   if(sender.sock != receiver.sock)
   {
      strncpy(heading, sender.name, BUF_SIZE - 1);
      strncat(heading, " to ", sizeof heading - strlen(heading) - 1);
      strncat(heading, receiver.name, sizeof heading - strlen(heading) - 1);
      strncat(heading, " : ", sizeof heading - strlen(heading) - 1);
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_client(receiver.sock, heading);
      write_client(receiver.sock, message);
      write_discussion_in_file_private(path_of_msg, receiver.name, sender.name, message);
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disconnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}

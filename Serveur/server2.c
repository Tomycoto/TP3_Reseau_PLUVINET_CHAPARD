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
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;
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
               if(c == 0 || strcmp(buffer, "#Q")==0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else 
               {
                  char bufferCopy[strlen(buffer)];
                  char* check_if_at = strtok(strcpy(bufferCopy,buffer), "@");
                  char* check_command = strtok(strcpy(bufferCopy,buffer), " ");
                  if (strlen(check_if_at)==strlen(buffer)-1)
                  {
                     Client receiver;
                     char* receiverName = strtok(check_if_at, " ");
                     int receiverFound = 0;
                     for (int j=0; j<actual; j++)
                     {
                        if (strcmp(receiverName, clients[j].name)==0)
                        {
                           receiver = clients[j];
                           receiverFound = 1;
                           break;
                        }
                     }
                     if (receiverFound){
                        char* message_to_send = strtok(NULL,"\n");
                        send_message_to_one_client(receiver, client, message_to_send, 0);
                     } else {
                        write_client(client.sock, "Utilisateur introuvable");
                     }
                  }
                  else if (strcmp(check_command, "#C")==0)
                  {
                     create_group(buffer, clients, client, actual);
                  }
                  else if (strcmp(check_command, "#L")==0)
                  {  
                     print_clients_list(clients, client, actual);
                  }
                  else if (strcmp(check_command, "#O")==0)
                  {
                     print_options_list(client);
                  }
                  else
                  {
                     send_message_to_all_clients(clients, client, actual, buffer, 0);
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
{
   write_client(caller.sock, "Liste des clients connectés :\n");
   for (int i=0; i<nb_connected_clients-1; i++)
   {
      write_client(caller.sock, connected_clients[i].name);
      write_client(caller.sock, ", ");
   }
   write_client(caller.sock, connected_clients[nb_connected_clients-1].name);
}

static void print_options_list(Client caller)
{  write_client(caller.sock, "\nAvailable options:\n");
   char* simple_message = ("'message' : Send a message to the current discussion\n");
   char* new_message = ("'@member|groupname message' : Send a message to another person|group\n");
   char* group_creation = ("'#C groupname member1,member2,memberX message' : Create a new discussion group\n");
   char* clients_list = ("'#L' : See the list of connected clients\n");
   char* options_list = ("'#O' : See the list of options\n");
   write_client(caller.sock, simple_message);
   write_client(caller.sock, new_message);
   write_client(caller.sock, group_creation);
   write_client(caller.sock, clients_list);
   write_client(caller.sock, options_list);
}

static void create_group(char* buffer, Client* connected_clients, Client sender, int nb_connected_clients)
{
   Client group_members[MAX_CLIENTS];
   char* temp = strtok(buffer, " ");
   char* group_name = strtok(NULL, " ");
   char* current_person = strtok(NULL, ",");
   char* previous_person="";
   int k=0;
   int ch = ' ';

   while(strchr(current_person, ch)==NULL)
   {
      int receiverFound = 0;
      for (int j=0; j<nb_connected_clients; j++)
      {
         if (strcmp(current_person, connected_clients[j].name)==0)
         {
            group_members[k] = connected_clients[j];
            receiverFound = 1;
            break;
         }
      }
      if (!receiverFound)
      {
         write_client(sender.sock, "Un utilisateur est introuvable");
      }
      current_person= strtok(NULL, ",");
      previous_person = current_person;
      k++;
   }

   current_person = strtok(current_person, " ");
   int receiverFound = 0;
   for (int j=0; j<nb_connected_clients; j++)
   {
      if (strcmp(current_person, connected_clients[j].name)==0)
      {
         group_members[k] = connected_clients[j];
         receiverFound = 1;
         break;
      }
   }
   if (!receiverFound)
   {
      write_client(sender.sock, "Un utilisateur est introuvable");
   }
   k++;

   group_members[k] = sender; //Ajout du client créateur du groupe à ce groupe (il est intégré au groupe qu'il crée)
   k++;

   Group group_to_send = {k, group_members, group_name}; 
   char * message_to_send = strtok(NULL, "\n");
   send_message_to_a_group(group_to_send, sender, message_to_send);

}

static void write_discution_in_file_private(char* path, char* receiver_name, char* sender_name, char* message)
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
   char *concatSend = (char*)malloc(strlen(path)+strlen(first_name)+1+strlen(second_name)+5);
   memcpy( concatSend, path, strlen(path) );
   memcpy( concatSend+strlen(path),first_name, strlen(first_name) );
   memcpy( concatSend+strlen(path)+strlen(first_name), ";" ,1 );
   memcpy( concatSend+strlen(path)+strlen(first_name)+1, second_name ,strlen(second_name) );
   memcpy( concatSend+strlen(path)+strlen(first_name)+1+strlen(second_name), ".txt" ,4 );
   concatSend[strlen(path)+strlen(first_name)+1+strlen(second_name)+4] = '\0';
   
   FILE* fptr= fopen(concatSend, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}

static void write_discution_in_file_several(char* path, Client* receivers_names, int number_of_receivers, Client sender, char* message)
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   char receivers_concatenate[BUF_SIZE];
   strcpy(receivers_concatenate, ""); //réinitialisation de receivers_concatenate à chaque message pour éviter l'accumulation de pseudos

   int sender_already_here=0;
   for (int i=0; i<number_of_receivers;i++) //recherche si on a déjà l'envoyeur dans les destinataires
   {
      if (strcmp(sender.name, receivers_names[i].name)==0)
      {
         sender_already_here=1;
         break;
      }
   }
   if(sender_already_here)
   {
      for (int i=0; i<number_of_receivers;i++)
      {
         for (int j=0;j<number_of_receivers;j++)
         {
            if (i!=j && strcmp(receivers_names[i].name, receivers_names[j].name)<0)
            {
               Client temp = receivers_names[i];
               receivers_names[i] = receivers_names[j];
               receivers_names[j] = temp;
            }
         }
      }
      for (int i=0; i<number_of_receivers-1;i++)
      {
         strncat(receivers_concatenate, receivers_names[i].name, BUF_SIZE);
         strncat(receivers_concatenate, ";", strlen(receivers_concatenate)+1);
      }
      strncat(receivers_concatenate, receivers_names[number_of_receivers-1].name, BUF_SIZE);  
   }
   else
   {
      Client receivers_and_sender[number_of_receivers+1];
   
      for (int i=0; i<number_of_receivers;i++)
      {
         receivers_and_sender[i] = receivers_names[i];
      }
      receivers_and_sender[number_of_receivers] = sender;
      
      for (int i=0; i<number_of_receivers+1;i++)
      {
         for (int j=0;j<number_of_receivers+1;j++)
         {
            if (i!=j && strcmp(receivers_and_sender[i].name, receivers_and_sender[j].name)<0)
            {
               Client temp2 = receivers_and_sender[i];
               receivers_and_sender[i] = receivers_and_sender[j];
               receivers_and_sender[j] = temp2;
            }
         }
      }

      for (int i=0; i<number_of_receivers;i++)
      {
         strncat(receivers_concatenate, receivers_and_sender[i].name, BUF_SIZE);
         strncat(receivers_concatenate, ";", strlen(receivers_concatenate)+1);
      }
      strncat(receivers_concatenate, receivers_and_sender[number_of_receivers].name, BUF_SIZE); 
   }

   char *concatSend = (char*)malloc(strlen(path)+strlen(receivers_concatenate)+5);
   memcpy( concatSend, path, strlen(path) );
   memcpy( concatSend+strlen(path), receivers_concatenate ,strlen(receivers_concatenate) );
   memcpy( concatSend+strlen(path)+strlen(receivers_concatenate), ".txt" ,4 );
   concatSend[strlen(path)+strlen(receivers_concatenate)+4] = '\0';
   FILE* fptr;
   fptr = fopen(concatSend, "a+");
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender.name, message);
   fclose(fptr);
}

static void write_discution_in_file_group(char* path, Group group, Client sender, char* message)
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   char *concatSend = (char*)malloc(strlen(path)+strlen(group.group_name)+5);
   memcpy( concatSend, path, strlen(path) );
   memcpy( concatSend+strlen(path), group.group_name ,strlen(group.group_name) );
   memcpy( concatSend+strlen(path)+strlen(group.group_name), ".txt" ,4 );
   concatSend[strlen(path)+strlen(group.group_name)+4] = '\0';
   FILE* fptr;
   if (access(concatSend, F_OK) == 0) {
      fptr = fopen(concatSend, "a+");
   } else {
      fptr = fopen(concatSend, "a+");
      fprintf(fptr, "#members ");
      for (int i=0;i<group.number_members;i++)
      {
         fprintf(fptr, "%s,", group.group_members[i].name);
      }
      fprintf(fptr, "\n");
   }
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender.name, message);
   fclose(fptr);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   char* path_of_msg="./Messages/";
   message[0] = 0;
   char heading[BUF_SIZE];
   strcpy(heading,sender.name);
   int all=0;
   strncat(heading," to ",sizeof heading - strlen(heading) - 1);
   for (int j=0; j<actual-1; j++)
   {
      if (strcmp(clients[j].name,sender.name)==0)
      {
         strcpy(heading,sender.name);
         strncat(heading," to all",sizeof heading - strlen(heading) - 1);
         all=1;
         break;
      }
      strncat(heading,clients[j].name,sizeof heading - strlen(heading) - 1);
      strncat(heading,", ",sizeof heading - strlen(heading) - 1);
   }
   if (strcmp(clients[actual-1].name,sender.name)==0 && !all)
   {
      strcpy(heading,sender.name);
      strncat(heading," to all",sizeof heading - strlen(heading) - 1);
      all=1;
   }
   if (!all)
   {
      strncat(heading,clients[actual-1].name,sizeof heading - strlen(heading) - 1);
   }
   if(from_server == 0)
      {
         strncat(heading, " : ", sizeof heading - strlen(heading) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         write_client(clients[i].sock, heading);
         write_client(clients[i].sock, message);
      }
   }
   write_discution_in_file_several(path_of_msg, clients, actual, sender, message);
}

static void send_message_to_a_group(Group group, Client sender, const char* buffer)
{
   int i = 0;
   char message[BUF_SIZE];
   char* path_of_msg="../Groupes/";
   message[0] = 0;
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   for (int j=0; j<group.number_members; j++)
   {
      if (strcmp(group.group_members[j].name, sender.name)!=0)
      {
         write_client(group.group_members[j].sock, message);
      }
   }
   write_discution_in_file_group(path_of_msg, group, sender, message);
}

static void send_message_to_one_client(Client receiver, Client sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   char heading[BUF_SIZE];
   char* path_of_msg="../Discussions/";
   message[0] = 0;
   heading[0] = 0;
   /* we don't send message to the sender */
   if(sender.sock != receiver.sock)
   {
      if(from_server == 0)
      {
         strncpy(heading, sender.name, BUF_SIZE - 1);
         strncat(heading, " to ", sizeof heading - strlen(heading) - 1);
         strncat(heading, receiver.name, sizeof heading - strlen(heading) - 1);
         strncat(heading, " : ", sizeof heading - strlen(heading) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_client(receiver.sock, heading);
      write_client(receiver.sock, message);
      write_discution_in_file_private(path_of_msg, receiver.name, sender.name, message);
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

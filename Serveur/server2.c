#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "server2.h"
#include "client2.h"

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
   int nbGrp = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   
   /* an array for all groupes */
   Groupe groupes[MAX_CLIENTS];

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
         //Liste des groupes c est membre
         char ismember[MAX_CLIENTS];
         for (int g = 0; i < nbGrp; i++)
         {
            for (int h = 0; i < groupes[g].size; i++)
            {
               if (strcmp(c.name,groupes[g].membreGroupe[h].name)==0)
               {
                  strcat(ismember, groupes[g].name);
                  strcat(ismember, "- ");
               }
            }
            
         }
         write_client(sock, "Vous appartenez aux groupes suivants");
         write_client(sock, ismember);
         write_client(sock, "Pour parler à une personne, entrez: @Destinataire [Message]\r\n Pour créer un groupe, entrez: #NomDuGroupe:Destinataire1+Destinataire2+...+DestinataireN [Message]\r\n Pour envoyer un message à un groupe existant #NomDuGroupe [Message]\r\n");
      
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
               if(c == 0 || buffer=="q")
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
                  printf("%s\r\n", buffer);
               }
               else 
               {
                  char bufferCopy[strlen(buffer)];
                  Client receiver;
                  char* check_if_at = strtok(strcpy(bufferCopy,buffer), "@");
                  char* check_if_hashtag = strtok(strcpy(bufferCopy,buffer), "#");

                  /*char* check_if_at = strtok(strcpy(bufferCopy,buffer), "#");
                  char* check_if_hashtag = strtok(strcpy(bufferCopy,buffer), "@");
                  char* next_at = strtok(NULL, "@");
                  if (next_at!=NULL) //sinon on a un seul destinataire
                  {
                     printf("check : %s\n", check_if_hashtag);
                     memcpy( check_if_hashtag+strlen(check_if_hashtag), "@", 1);
                     memcpy( check_if_hashtag+strlen(check_if_hashtag)+1,next_at, strlen(next_at) );
                     check_if_hashtag[strlen(check_if_hashtag)+1+strlen(next_at)] = '\0';
   
                     //strncat(check_if_hashtag, "@", strlen(check_if_hashtag)+strlen(next_at)+1);
                     //strncat(check_if_hashtag, next_at, strlen(check_if_hashtag)+strlen(next_at)+1);
                  }
                  printf("%s --- %s\n", check_if_hashtag, next_at);*/
                  if (strlen(check_if_at)==strlen(buffer)-1)
                  {
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
                        printf("From %s to %s: %s\r\n", client.name, receiver.name, message_to_send);
                     } else {
                        write_client(client.sock, "Utilisateur hors-ligne");
                        printf("From %s: destinataire hors-ligne\r\n", client.name);
                     }
                  }
                  else if (strlen(check_if_hashtag)==strlen(buffer)-1)
                  {
                     Groupe groupe;

                     //Verifie si le groupe existe déjà 
                     strcpy(groupe.name, strtok(check_if_hashtag, ":"));
                     int existingGroup=0;

                     for (int g = 0; i < nbGrp; i++)
                     {
                        if (strcmp(groupe.name, groupes[g].name)==0)
                        {
                          strcpy(groupe.membreGroupe, groupes[g].membreGroupe);
                          existingGroup = 1;
                          break;
                        }  
                     }
                     if(existingGroup){
                        char * message_to_send = strtok(NULL, "\n");
                        send_message_to_all_clients(groupe.membreGroupe,client,groupe.size,message_to_send,0);
                     }
                     
                     //sinon crée le groupe
                     else{
                        nbGrp++;
                        groupe.size=0;
                        char* current_person= strtok(NULL, "+");
                        char* previous_person="";
                        int ch = ' ';

                        while(strchr(current_person,ch)==NULL)
                        {
                           int receiverFound = 0;
                           for (int j=0; j<actual; j++)
                           {
                              if (strcmp(current_person, clients[j].name)==0)
                              {
                                 groupe.membreGroupe[groupe.size] = clients[j];
                                 receiverFound = 1;
                                 break;
                              }
                           }
                           if (!receiverFound)
                           {
                              write_client(client.sock, "L'utilisateur est hors-ligne");
                              printf("L'utilisateur %s est hors-ligne.\r\n", current_person);
                           }
                           previous_person= current_person;
                           current_person = strtok(NULL, "+");
                           groupe.size++;
                        }
                        current_person = strtok(current_person, " ");
                        int receiverFound = 0;
                        for (int j=0; j<actual; j++)
                        {
                           if (strcmp(current_person, clients[j].name)==0)
                           {
                              groupe.membreGroupe[groupe.size] = clients[j];
                              receiverFound = 1;
                              break;
                           }
                        }
                        if (!receiverFound)
                        {
                           write_client(client.sock, "Utilisateur introuvable");
                           printf("L'utilisateur %s est introuvable.\r\n", current_person);
                        }

                        groupe.size++;

                        groupe.membreGroupe[groupe.size]=client;
                        groupe.size++

                        char * message_to_send = strtok(NULL, "\n");
                        send_message_to_all_clients(groupe.membreGroupe, client, groupe.size, message_to_send, 0);                       
                        printf("From %s to the Group %s: %s\r\n", client.name, groupe.name, message_to_send);
                     }
                  }   
                  else
                  {
                     write_client(client.sock, "Veuillez spécifier un destinataire");
                     //send_message_to_all_clients(clients, client, actual, buffer, 0);
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

static void write_discution_in_file_private(char* path, char* receiver_name, char* sender_name, char* message)
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);

   char *concatSend = (char*)malloc(strlen(path)+strlen(sender_name)+1+strlen(receiver_name)+5);
   memcpy( concatSend, path, strlen(path) );
   memcpy( concatSend+strlen(path),sender_name, strlen(sender_name) );
   memcpy( concatSend+strlen(path)+strlen(sender_name), ";" ,1 );
   memcpy( concatSend+strlen(path)+strlen(sender_name)+1, receiver_name ,strlen(receiver_name) );
   memcpy( concatSend+strlen(path)+strlen(sender_name)+1+strlen(receiver_name), ".txt" ,4 );
   concatSend[strlen(path)+strlen(sender_name)+1+strlen(receiver_name)+4] = '\0';
   char *concatReceive = (char*)malloc(strlen(path)+strlen(sender_name)+1+strlen(receiver_name)+5);
   memcpy( concatReceive, path, strlen(path) );
   memcpy( concatReceive+strlen(path),receiver_name, strlen(receiver_name) );
   memcpy( concatReceive+strlen(path)+strlen(receiver_name), ";" ,1 );
   memcpy( concatReceive+strlen(path)+strlen(receiver_name)+1, sender_name ,strlen(sender_name) );
   memcpy( concatReceive+strlen(path)+strlen(receiver_name)+1+strlen(sender_name), ".txt" ,4 );
   concatReceive[strlen(path)+strlen(sender_name)+1+strlen(receiver_name)+4] = '\0';
   FILE* fptr;
   if (access(concatSend, F_OK) == 0) {
      fptr = fopen(concatSend, "a+");
   } else {
      fptr = fopen(concatReceive, "a+");
   }
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}

static void write_discution_in_file_group(char* path, Client* receivers_names, char* sender_name, char* message)
{
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   
   char receivers_concatenate[BUF_SIZE];
   strcpy(receivers_concatenate, ""); //réinitialisation de receivers_concatenate à chaque message pour éviter l'accumulation de pseudos
   Client* receivers_iterator = receivers_names;
   while(strcmp(receivers_iterator->name, "")!=0)
   {
      if (strcmp(receivers_iterator->name, sender_name)!=0)
      {
         strncat(receivers_concatenate, receivers_iterator->name, BUF_SIZE);
         strncat(receivers_concatenate, ";", BUF_SIZE);
      }
      receivers_iterator++;
   }

   char *concatSend = (char*)malloc(strlen(path)+strlen(sender_name)+1+strlen(receivers_concatenate)+5);
   memcpy( concatSend, path, strlen(path) );
   memcpy( concatSend+strlen(path),sender_name, strlen(sender_name) );
   memcpy( concatSend+strlen(path)+strlen(sender_name), receivers_concatenate ,strlen(receivers_concatenate) );
   memcpy( concatSend+strlen(path)+strlen(sender_name)+strlen(receivers_concatenate), ".txt" ,4 );
   concatSend[strlen(path)+strlen(sender_name)+strlen(receivers_concatenate)+4] = '\0';
   char *concatReceive = (char*)malloc(strlen(path)+strlen(sender_name)+1+strlen(receivers_concatenate)+5);
   memcpy( concatReceive, path, strlen(path) );
   memcpy( concatReceive+strlen(path),receivers_concatenate, strlen(receivers_concatenate) );
   memcpy( concatReceive+strlen(path)+strlen(receivers_concatenate), sender_name ,strlen(sender_name) );
   memcpy( concatReceive+strlen(path)+strlen(receivers_concatenate)+strlen(sender_name), ".txt" ,4 );
   concatReceive[strlen(path)+strlen(sender_name)+strlen(receivers_concatenate)+4] = '\0';
   FILE* fptr;
   if (access(concatSend, F_OK) == 0) {
      fptr = fopen(concatSend, "a+");
   } else {
      fptr = fopen(concatReceive, "a+");
   }
   fprintf(fptr, "%d-%02d-%02d %02d:%02d:%02d - %s : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, sender_name, message);
   fclose(fptr);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   char* path_of_msg="/home/bpluvinet/Documents";
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
   write_discution_in_file_group(path_of_msg, clients, sender.name, message);
}

static void send_message_to_one_client(Client receiver, Client sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   char heading[BUF_SIZE];
   char* path_of_msg="/home/bpluvinet/Documents";
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

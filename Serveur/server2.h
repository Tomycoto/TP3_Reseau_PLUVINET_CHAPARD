#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100

#define BUF_SIZE    1024

#include "client2.h"
#include "struct_group.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void print_clients_list(Client* connected_clients, Client caller, int nb_connected_clients);
static void print_options_list(Client caller);
static void create_group(char* buffer, Client* connected_clients, Client sender, int nb_connected_clients);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void send_message_to_one_client(Client receiver, Client sender, const char* buffer, char from_server);
static void send_message_to_a_group(Group group, Client sender, const char* buffer);
static void write_discution_in_file_private(char* path, char* receiver_name, char* sender_name, char* message);
static void write_discution_in_file_several(char* path, Client* receivers_names, int number_of_receivers, Client sender_name, char* message);
static void write_discution_in_file_group(char* path, Group group, Client sender,char* message);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static int connect_client(Client* client, char* buffer);

#endif /* guard */

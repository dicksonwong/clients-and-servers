/* server.c
 * Author: Dickson Wong
 * Last Updated: December 21, 2017
 * 
 * 
 * A simple server using socket that establishes connections with up to 
 * four clients and simply prints them all out.  The HOST_NAME of this server
 * will be localhost.
 * 
 * Usage: ./server.exe PORT_NO SERVER_NAME
 * 
 * */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_LEN 256
#define MESSAGE_LEN (BUFFER_LEN - 1)
#define MAX_CLIENTS 4

static int num_clients = 0;
static struct client_node *head;
static struct client_node *tail;
static int current_id = 0;

pthread_mutex_t client_table_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_serve_lock = PTHREAD_MUTEX_INITIALIZER;

struct client_node {
	int id;
	int sock_fd;
	char name[5];
	struct client_node *next;
};

/* Add a client to the list; returns -1 on failure and 0 on success */
int add_client(struct client_node *new_client) 
{
	if (num_clients >= MAX_CLIENTS) {
		return -1;
	}
	
	/* Add a new client to the end of the tail and increment num_clients */
	if (tail == NULL) {
		head = new_client;
	} else {
		tail->next = new_client;
	}
	tail = new_client;
	
	num_clients++;
	
	return 0;
}

/* Clears the buffer by replacing all characters by zeros */
void clear_buffer(char *buffer) 
{
	bzero((char *) buffer, BUFFER_LEN);
}

/* Interface with the client as specified in args; prints all messages
 * received from client; return 0 upon disconnection; on any instance of
 * error occuring, return -1 */
void *handle_client(void *args) {
	char buffer[BUFFER_LEN];
	clear_buffer(buffer);
	struct client_node cli_node = *(struct client_node *)args;
	int n;
	int client_connected = 1;
	
	/* While the client is connected, read messages and print on server. 
	 * if the message is ".DISCONNECT", then return 0 */
	while (client_connected) {
		if ((n = read(cli_node.sock_fd, buffer, MESSAGE_LEN)) != 0)
		{
			/* lock serve_client lock */
			pthread_mutex_lock(&client_serve_lock);
			
			/* Client has disconnected from server suddenly */
			if (n < 0)
			{
				printf("%s: suddenly disconnected or unknown error\n", 
					   cli_node.name);
				client_connected = 0;
				close(cli_node.sock_fd);
			} 
			
			/* Client sent a disconnect message */
			else if (strcmp(buffer, ".DISCONNECT") == 0) 
			{
				printf("%s: disonnected\n", cli_node.name);
				client_connected = 0;
				close(cli_node.sock_fd);
				clear_buffer(buffer);
			}
			
			/* Print message from client */
			else 
			{
				printf("%s says: %s\n", cli_node.name, buffer);
				clear_buffer(buffer);
			}
			
			/* unlock serve client lock */
			pthread_mutex_unlock(&client_serve_lock);
		}
    }
	
	return 0;
}

/* Attempts to create a new connection and adds it to the list of clients
 * being served.  If a new connection cannot be made, return -1; otherwise,
 * return 0. */
int handle_new_connection(int sockfd) 
{
	int cli_sockfd, cli_len;
	struct client_node cli_node;
    struct sockaddr_in cli_addr;
    int handle_failed = -1;
    pthread_t cli_thread;
    
    /* Attempt to accept a new connection */
    cli_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (cli_sockfd < 0) {
		printf("handle_new_connection: error on accept\n");
		return -1;
	}
	
	if (num_clients >= MAX_CLIENTS) {
		return -1;
	}
	
	/* Lock the table of clients */
	pthread_mutex_lock(&client_table_lock);
	
	/* Add new client information */
	current_id++;
	cli_node.id = current_id;
	cli_node.sock_fd = cli_sockfd;
	sprintf(cli_node.name, "%d", current_id);
	cli_node.next = NULL;
	
	/* Attempt to add a new client to the table */
	if (add_client((struct client_node *)&cli_node) >= 0) {
		handle_failed = 0;
		
		/* Spawn another thread to handle the client */
		pthread_create(&cli_thread, NULL, (void *)handle_client, (void *)&cli_node);
		//detach
	}
		
	/* Unlock the table lock */
	pthread_mutex_unlock(&client_table_lock);
	
	return handle_failed;	
}
	
int main(int argc, char *argv[])
{
	int sockfd, port_number;
    struct sockaddr_in serv_addr;
    
	/* Check that both a name and a port number are provided */
	if (argc < 3) {
		printf("main: server requires both name and port number.\n");
		exit(1);
	}
	
	/* Create a main socket that communicates with the other sockets */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("main: socket failed\n");
		exit(1);
	}
	
	/* Set all values in buffer serv_addr to zero */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	/* Get the port number from the argument provided */
	port_number = atoi(argv[1]);
	
	/* Initialize serv_addr values; set in_adrr to accept connections to all
	 * IPs via INADDR_ANY */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_number); 
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	/* Attempt to bind address to socket */
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("main: bind socket to %d failed\n", port_number);
		exit(1);
	}
	
	/* Listen for a client connecting to socket */
	while ((listen(sockfd, 5) != -1) && (num_clients < MAX_CLIENTS)) 
	{
		handle_new_connection(sockfd);
    }
    
	return 0;
}

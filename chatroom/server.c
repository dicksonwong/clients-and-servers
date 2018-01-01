/* server.c
 * Author: Dickson Wong
 * Last Updated: December 31, 2017
 * 
 * A simple server using socket that establishes connections with up to 
 * four clients and receives messages.  Server will write the messages to the 
 * other clients connected to the server.  The HOST_NAME of this server
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

/* maximum length of client name */
#define CLI_NAME_LEN 30

/* number of characters in .DISCONNECT */
#define EXIT_MESSAGE_LEN 11

static int num_clients = 0;
static struct client_node *head;
static int current_id = 0;

/* mutex controlling access to table of clients (and number of clients) */
pthread_mutex_t client_table_lock = PTHREAD_MUTEX_INITIALIZER;

/* mutex controlling read/write to a client at any time */
pthread_mutex_t client_serve_lock = PTHREAD_MUTEX_INITIALIZER;

struct client_node {
	int id;
	int sock_fd;
	char name[CLI_NAME_LEN];
	struct client_node *next;
};

/* Add a client to the list; returns 0 on failure and 1 on success */
int add_client(struct client_node *new_client) 
{
	if (num_clients >= MAX_CLIENTS) {
		return 0;
	}
	
	/* If no node at head of list, then make new client node the head */
	if (head == NULL) {
		head = new_client;
		num_clients++;
		return 1;
	}
	
	struct client_node *current = head;
	
	/* Locate the end of the list and add the new client */
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = new_client;
	
	num_clients++;
	
	return 1;
}

/* Remove a client from the list given the id; returns 0 on failure and 1 success */
int remove_client(int id)
{
	int rc = 0;
	
	/* If head is NULL for any reason, then id could not be found */
	if (head == NULL) {
		return rc;
	}
	
	struct client_node *prev;
	struct client_node *current = head;
	
	/* Find the node whose id matches the given id */
	while ((current != NULL) && (current->id  != id)) {
		prev = current;
		current = current->next;
	}
		
	/* If current is NULL, then node with given id doesn't exist */
	if (current == NULL) {
		return rc;
	}
	
	/* If prev was NULL, then node to be removed is head; otherwise, 
	 * join prev and current.next */
	if (prev == NULL) {
		head = current->next;
	} else {
		prev->next = current->next;
	}
	rc = 1;
	return rc;
}
	
/* Clears the buffer by replacing all characters by zeros */
void clear_buffer(char *buffer) 
{
	bzero((char *) buffer, BUFFER_LEN);
}

/* Write message to all clients, given message from specified client */
void write_to_clients(char *name, char *msg) {
	struct client_node *current = head;
	int i = 0;
	
	while (current != NULL) {
		write(current->sock_fd, name, strlen(name));
		write(current->sock_fd, " says: ", 7);
		write(current->sock_fd, msg, strlen(msg));
		write(current->sock_fd, "\n", 2);
		current = current->next;
		
	}
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
	
	/* Wait for the client to identify their name; if no name is received or
	 * client disconnects, then disconnect the client */
	if ((n = read(cli_node.sock_fd, cli_node.name, CLI_NAME_LEN)) <= 0) {
		printf("Client did not identify themselves; disconnecting client...\n");
		client_connected = 0;
	}
	
	/* While the client is connected, read messages and print on server. */
	while (client_connected) {
		n = read(cli_node.sock_fd, buffer, MESSAGE_LEN);
		printf("%d bytes were read\n", n);
		
		/* User must have disconnected */
		if ((n <= 0) || (strncmp(buffer, ".DISCONNECT", EXIT_MESSAGE_LEN) == 0)) {
			client_connected = 0;
		} else {
			
			/* lock serve_client lock */
			pthread_mutex_lock(&client_serve_lock);

			/* Print message from client */
			printf("%s says: %s\n", cli_node.name, buffer);
			
			/* Write to all clients */
			write_to_clients(cli_node.name, buffer);
			
			/* unlock serve client lock */
			pthread_mutex_unlock(&client_serve_lock);
			
			clear_buffer(buffer);

		}
    }
    
    // REMOVE CLIENT
    
    /* Lock the table of clients */
    printf("Removing client with id %d; locking table\n", cli_node.id);
    pthread_mutex_lock(&client_table_lock);
    
    /* Remove client node from the list */
    printf("ending connection with: %d\n", cli_node.id);
    printf("result : %d\n", remove_client(cli_node.id));
    
   	/* Unlock the table of clients */
	printf("succesfully removed; unlocking table\n");
	pthread_mutex_unlock(&client_table_lock);
}

/* Attempts to create a new connection and adds it to the list of clients
 * being served.  If a new connection cannot be made, return 0; otherwise,
 * upon client's disconnection, return 1 */
int handle_new_connection(int sockfd) 
{
	int cli_sockfd, cli_len;
	int rc = 0;
	struct client_node cli_node;
    struct sockaddr_in cli_addr;
    pthread_t cli_thread;
    
    /* Attempt to accept a new connection */
    cli_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (cli_sockfd < 0) {
		printf("handle_new_connection: error on accept\n");
		return rc;
	}
	
	if (num_clients >= MAX_CLIENTS) {
		return rc;
	}
	
	printf("Locking table and adding new client\n");
	/* Lock the table of clients */
	pthread_mutex_lock(&client_table_lock);
	
	/* Add new client information */
	current_id++;
	cli_node.id = current_id;
	cli_node.sock_fd = cli_sockfd;
	cli_node.next = NULL;
	
	/* Attempt to add a new client to the table */
	if (add_client((struct client_node *)&cli_node) > 0) {
		
		/* Spawn another thread to handle the client */
		pthread_create(&cli_thread, NULL, (void *)handle_client, (void *)&cli_node);
		//detach
		rc = 1;
	}
			
	printf("handle_new_connection; rc value: %d\n", rc);
	/* Unlock the table lock */
	pthread_mutex_unlock(&client_table_lock);
		
	return rc;	
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

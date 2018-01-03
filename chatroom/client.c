/* client.c
 * Author: Dickson Wong
 * Date: Jan 1, 2018
 * 
 * A simple client that simply connects to a server and continues to write
 * messages to it until disconnection.
 * 
 * Usage: ./client.exe PORT_NO HOST_NAME
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <pthread.h>

#define BUFFER_LEN 256
#define MESSAGE_LEN (BUFFER_LEN - 1)

#define CLI_NAME_BUFFER_LEN 30
#define CLI_NAME_LEN (CLI_NAME_BUFFER_LEN - 1)

/* Clears the buffer */
void clear_buffer(char *buffer) 
{
	bzero((char *) buffer, BUFFER_LEN);
}

/* Reads and removes non-EOF characters (including newline) in stdin.
int clear_input() {
	char buffer[BUFFER_LEN]; 
	
	while (fgets(buffer, BUFFER_LEN, stdin) != NULL) {
		fgets(buffer, BUFFER_LEN, stdin);
	}
}
*/

/* Gets a name from the user and stores it in cli_name */
void get_username(char *cli_name) {
	
	int n;
	
	if (cli_name) {
		/* Prompt client to enter a name */
		printf("Please enter a name: ");
		
		/* Get a name from the user */
		bzero((char *) cli_name, CLI_NAME_BUFFER_LEN);
		fgets(cli_name, CLI_NAME_BUFFER_LEN, stdin);
		
		/* Remove the newline character in name */
		if ((n = strcspn(cli_name, "\n")) < CLI_NAME_BUFFER_LEN) {
			cli_name[n] = '\0';
		}
		
		/* Removes any extra characters from stdin */
		clear_input();
	}
}
	
void *handle_server(void *args) {
	char buffer[BUFFER_LEN];
	clear_buffer(buffer);
	
	int sockfd = *((int *)args);
	int n;
	int connected = 1;
	
	/* Read messages coming in from the server while we are still connected */
	while (connected) {
		n = read(sockfd, buffer, MESSAGE_LEN);
		
		/* Return when the server has disconnected */
		if (n <= 0) {
			connected = 0;
		} else {
			printf(buffer);
			clear_buffer(buffer);
		}
	}
			
}

int main(int argc, char *argv[])
{	
    int sockfd, port_number, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFER_LEN];
    char cli_name[CLI_NAME_BUFFER_LEN];
    char *msg;
    int connected = 1;
    pthread_t server_thread;
    
    /* Check that both hostname and port are provided */
    if (argc < 3) {
		printf("USAGE: client PORT_NO HOSTNAME\n");
		exit(1);
	}
 
	/* Get the port number of the server from arguments */
    port_number = atoi(argv[1]);
    
    /* Attempt to open a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
		printf("main: error opening a socket\n");
		exit(1);
	}

	/* Attempt to get host information from name provided */
    server = gethostbyname(argv[2]);
    
    if (server == NULL) {
        printf("main: host going by name: %s does not exist", argv[2]);
        exit(1);
    }
    
    /* Fill serv_addr buffer with zeroes */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    /* Fill in serv_addr information */
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port_number);
    
    /* Attempt to create a connection to server via sockfd */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("main: connect to host failed\n");
		exit(1);
	}
    
    /* Get a usename from the user */
    get_username(cli_name);
	
	/* Pass on the username to the server */
	n = write(sockfd, cli_name, CLI_NAME_LEN);
	
	/* Spawn another thread to read messages coming in from server */
	pthread_create(&server_thread, NULL, (void *)handle_server, (void *)&sockfd);
	
	printf("Enter a message: ");
			
	/* Continue to read and write messages while connected */
    while(connected)    
    {

		/* Get message from user and write to server */
		bzero(buffer, BUFFER_LEN);
		msg = fgets(buffer, MESSAGE_LEN, stdin);
		
		/* Remove the newline character entered by the user */
		msg[strcspn(msg, "\n")] = '\0';
		
		/* Print client's message back to client */
		printf("%s says: %s\n",cli_name, msg);
		
		/* Write non-NULL msg to sockfd */
		if (msg != NULL) {

			n = write(sockfd, buffer, strlen(buffer));
			bzero(buffer, BUFFER_LEN);
			
			if (n < 0) {
				printf("main: cannot write to server\n");
				exit(1);
			}
		}
	}
    
    return 0;
}

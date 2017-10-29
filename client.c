/* client.c
 * Modified by: Dickson Wong
 * Last Updated: October 29, 2017
 * 
 * Original code from:
 * http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
 * 
 * A simple client using socket that connects to a server and writes a
 * message to it.  The client then reads a message from the server and proceeds
 * to exit.
 * 
 * Usage: ./client.exe HOST_NAME PORT_NO
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

#define BUFFER_LEN 256
#define MESSAGE_LEN (BUFFER_LEN - 1)

int main(int argc, char *argv[])
{	
    int sockfd, port_number, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFER_LEN];
    
    /* Check that both hostname and port are provided */
    if (argc < 3) {
		printf("main: client needs both hostname and port\n");
		exit(1);
	}
 
	/* Get the port number of the server from arguments */
    port_number = atoi(argv[2]);
    
    /* Attempt to open a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
		printf("main: error opening a socket\n");
		exit(1);
	}

	/* Attempt to get host information from name provided */
    server = gethostbyname(argv[1]);
    
    if (server == NULL) {
        printf("main: host going by name: %s does not exist", argv[1]);
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
        
    /* Get message from user and write to server */
    printf("Enter a message: ");
    bzero(buffer, BUFFER_LEN);
    fgets(buffer, MESSAGE_LEN, stdin);
    n = write(sockfd, buffer, strlen(buffer));
	
    if (n < 0) {
		printf("main: cannot write to server\n");
		exit(1);
	}

	/* Clear the buffer and read a message from server */
    bzero(buffer, BUFFER_LEN);
    n = read(sockfd, buffer, MESSAGE_LEN);
    
    if (n < 0) {
		printf("main: cannot read from server\n");
		exit(1);
	}
	
    printf("%s\n",buffer);
    
    return 0;
}

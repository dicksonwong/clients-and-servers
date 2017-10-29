/* server.c
 * Modified by: Dickson Wong
 * Last Updated: October 29, 2017
 * 
 * Original code from:
 * http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
 * 
 * A simple server using socket that waits for a client and reads a message
 * from the client; the server then writes a message to the client as 
 * confirmation and proceeds to exit.
 * 
 * Usage: ./server.exe PORT_NO SERVER_NAME
 * 
 * */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_LEN 256
#define MESSAGE_LEN (BUFFER_LEN - 1)

int main(int argc, char *argv[])
{
	int sockfd, cli_sockfd, port_number, cli_len;
    char buffer[BUFFER_LEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    
	/* Check that both a name and a port number are provided */
	if (argc < 3) {
		printf("main: server requires both name and port number.\n");
		exit(1);
	}
	
	/* Create a socket; exit on failure */
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
		printf("main: bind failed\n");
		exit(1);
	}
	
	/* Listen for a client connecting to socket */
	listen(sockfd, 5);
	cli_len = sizeof(cli_addr);
    cli_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
     
    if (cli_sockfd < 0) 
	{
		printf("main: error on accept\n");
		exit(1);
	}
	
	/* Fill buffer with 0's */
    bzero(buffer,BUFFER_LEN);
    
    /* Read a message written to cli_sockfd into buffer */
    n = read(cli_sockfd, buffer, MESSAGE_LEN);
     
	if (n < 0) {
		printf("main: error on reading from socket\n");
		exit(1);
	}
	
	printf("Message from client: %s\n", buffer);
	
	/* Attempt to write a message to client and exit */
	n = write(cli_sockfd, "Message received", 16);
	
	if (n < 0) {
		printf("main: error writing to client socket");
		exit(1);
	}
	
	return 0;
}

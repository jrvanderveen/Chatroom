/* client.c - code for example client program that uses TCP */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: client
*
* Purpose: allocate a socket, connect to a server, and print all output
*
* Syntax: client [ host [port] ]
*
* host - name of a computer on which server is executing
* port - protocol port number server is using
*
* Note: Both arguments are optional. If no host name is specified,
* the client uses "localhost"; if no protocol port is
* specified, the client uses the default given by PROTOPORT.
*
*------------------------------------------------------------------------
*/
int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
	char serverResponse;
	char username[11];
	char userMessage[1000];
	uint8_t sendLength;
	uint16_t messageLen;

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./participant local_address server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	/*
	Handle interaction with server
	sends guess to server and recieves server response
	*/

	//recieve a Y or an N
	recv(sd, &serverResponse, sizeof(serverResponse), 0);
	if (serverResponse == 'N'){
		close(sd);
		fprintf(stderr, "Chatroom full please try again latter!\n");
		exit(1);
	}

	bool uniqueName = false;
	while(!uniqueName){
		fprintf(stderr, "Enter username:");
		fgets(username, 10, stdin);
		sendLength = strlen(username) - 1;
		username[sendLength] = 0;
		if(sendLength > 10){
			fprintf(stderr, "try again len %d (must be len < 10)\n", sendLength);
		}
		else{
			n = send(sd, &sendLength, sizeof(sendLength), 0);//uin8_t
			if(n == -1){
		    close(sd);
		    exit(EXIT_FAILURE);
		  }
			n = send(sd, username, sendLength, 0);//char []  make sure no \0
			if(n == -1){
		    close(sd);
		    exit(EXIT_FAILURE);
		  }
			if(recv(sd, &serverResponse, sizeof(serverResponse),MSG_PEEK | MSG_DONTWAIT) == 0){
		    close(sd);
		    exit(EXIT_FAILURE);
		  }
			recv(sd, &serverResponse, sizeof(serverResponse),0);
			if(serverResponse == 'Y'){
				uniqueName = true;
			}
			else if(serverResponse == 'T'){	fprintf(stderr, "try again username taken\n");/*reset timer?*/}
			else if(serverResponse == 'I'){	fprintf(stderr, "try again invalid username\n");/*dont reset timer?*/}
		}
	}
//ENTER CHATROOM
	while (1) {
		fprintf(stderr, "Enter Message: ");
		fgets(userMessage, 1000, stdin);
		// fprintf(stderr, "%s\n", userMessage);
		messageLen = strlen(userMessage) - 1;
		userMessage[messageLen] = 0;
		n = send(sd, &sendLength, sizeof(messageLen), 0);//uin8_t
		if(n == -1){
	    close(sd);
	    exit(EXIT_FAILURE);
	  }
		n = send(sd, userMessage, messageLen, 0);//char []  make sure no \0
		if(n == -1){
	    close(sd);
	    exit(EXIT_FAILURE);
	  }
	}
}








///

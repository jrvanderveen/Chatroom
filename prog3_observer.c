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
	uint32_t clientGuess = 0; /* buffer for data from the server */
	char serverResponse;
	char username[11];
	uint8_t usernamelen;
	uint16_t serverResponseLen;
	char serverMessage[1013];

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./observer local_address server_port\n");
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
	if(recv(sd, &serverResponse, sizeof(serverResponse),MSG_PEEK | MSG_DONTWAIT) == 0){
		close(sd);
		exit(EXIT_FAILURE);
	}
	recv(sd, &serverResponse, sizeof(serverResponse),0);
	// fprintf(stderr, "%c", serverResponse);
	if (serverResponse == 'N'){
		close(sd);
			fprintf(stderr, "Chatroom full please try again!\n");
		exit(1);
	}

	bool nameFound = false;
	while(!nameFound){
		fprintf(stderr, "Enter username:");
		scanf("%s", username);
		usernamelen = strlen(username);
		if(usernamelen > 10){
			fprintf(stderr, "try again (must be len < 10)\n");
		}
		else{
			// fprintf(stderr, "%s\n", username);
			n = send(sd, &usernamelen, sizeof(usernamelen), 0);
			if(n == -1){
				close(sd);
				exit(EXIT_FAILURE);
			}
			n = send(sd, username, usernamelen, 0);//char []  make sure no \0
			if(n == -1){
				close(sd);
				exit(EXIT_FAILURE);
			}
		//	fprintf(stderr, "got before recieve\n");
			if(recv(sd, &serverResponse, sizeof(serverResponse),MSG_PEEK | MSG_DONTWAIT) == 0){
				close(sd);
				exit(EXIT_FAILURE);
			}
			recv(sd, &serverResponse, sizeof(serverResponse),0);
		//	fprintf(stderr, "got after recieve\n");
			if(serverResponse == 'T' || serverResponse == 'N'){
				close(sd);
				exit(0);
			}
			else{
				nameFound = true;
			}
		}
	}

//ENTER CHATROOM
	while (1) {
		if(	recv(sd, &serverResponseLen, sizeof(serverResponseLen),MSG_PEEK | MSG_DONTWAIT) == 0){
			fprintf(stderr, "%s\n", "exit");
			close(sd);
			exit(EXIT_FAILURE);
		}
		recv(sd, &serverResponseLen, sizeof(serverResponseLen),0);
		recv(sd, serverMessage, serverResponseLen,0);
		serverMessage[serverResponseLen] = '\0';
		// fprintf(stderr, "buf len: %d messege: %s\n", serverResponseLen, serverMessage);
		fprintf(stderr, "%s\n", serverMessage);
	}
}















//

/* server.c - code for example server program that uses TCP */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>

#include "trie.h"
#include "trie.c"

#define QLEN 6 /* size of request queue */

/*------------------------------------------------------------------------
* Program: server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) n = send a short message to the client
* (3) close the connection
* (4) go back to step (1)
*
* Syntax: server [ port ]
*
* port - protocol port number to use
*
* Note: The port argument is optional. If no port is specified,
* the server uses the default given by PROTOPORT.
*
*------------------------------------------------------------------------
*/


bool validUsername(char username[]){
	for(int i = 0; i < strlen(username); i++){
		if(!isdigit(username[i]) && !isalpha(username[i]) && username[i] != '_'){
			return false;
		}
	}
	return true;
}

struct participants {
	int Port[256];
	char Usernames[256][10];
	//long/timestruct participantNegotiationTime[256];
};
struct observers {
	int Port[256];
	char Usernames[256][10];
	struct timeval Time[256];
};

int setSocket(struct sockaddr_in sad, char argss[]){
	int sd;
	int port = atoi(argss); /* convert argument to binary */
	struct protoent *ptrp;
	int optval = 1; /* boolean value when we set socket option */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argss);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	return sd;
}

void largestInitialFd(int sdp, int sdo, int *nfds){
	if(sdp > sdo){
		*nfds = sdp;
	}
	if(sdo > sdp){
		*nfds = sdo;
	}
}
fd_set resetParObsSD(int observerConnections[], int participantConnections[], int *nfds, int sdp, int sdo){
	int iterator = 0;
	fd_set tempFd;
	FD_ZERO(&tempFd);
	FD_SET(sdp, &tempFd);
	FD_SET(sdo, &tempFd);
	while(iterator < 255){
		//check if set and only then add to read, sd = ..
		 if(participantConnections[iterator] > 0){
			 FD_SET(participantConnections[iterator], &tempFd);
	//=		fprintf(stderr, "participant found at position %d, port: %d\n", iterator, participantConnections[iterator]);
		 }
		 if(observerConnections[iterator] > 0){
			 FD_SET(observerConnections[iterator], &tempFd);
		 }
		 if(participantConnections[iterator] > *nfds){
			 *nfds = participantConnections[iterator];
		 }
		 if(observerConnections[iterator] > *nfds){
			 *nfds = observerConnections[iterator];
		 }
		 iterator++;
	 }
	 return tempFd;
}

 // int findObserver(char username[], char *observers[]){
 //  int iterator = 0;
 // //  while(observers[iterator] != port && iterator < 255){
 //  while(strcmp(username, observers[iterator]) == 0){
 // 	 iterator++;
 //  }
 //  fprintf(stderr, "observer: %s\n", observers[iterator]);
 //  return iterator;
 // }




int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad,sad2; /* structure to hold server's address */
	struct sockaddr_in cad;
	struct timeval tv;
	int sdp,sdo; /* socket descriptors */
	fd_set readfds;
	int numbParticipant = 0;
	int numbObserver = 0;
	int nfds = 0;
	int n;
	int alen;/* length of address */
	uint8_t PStringSize = 0; /* buffer for data from the server */
	uint16_t serverBuffSize = 0;
	char username[10];
	char serverResponse;
	char serverBuff[1000];
 	char prompt[13];
	char serverMessage[1013];
	bool usernameFound;

	Trie *observerTree = trie_new();
	Trie *participantTree = trie_new();

	struct participants participantStruct;
	struct observers observerStruct;
	for(int i = 0; i < 255; i++){
		participantStruct.Port[i] = 0;
		observerStruct.Port[i] = 0;
	}
	tv.tv_sec = 0;

	signal(SIGCHLD,SIG_IGN);

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./prog3_server server_participant_port server_observer_port\n");
		exit(EXIT_FAILURE);
	}
////////////////////////////observer socket
	sdo = setSocket(sad, argv[2]);
///////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------participant socket
	sdp = setSocket(sad2, argv[1]);
//-----------------------------------------------------------------------------------------

	alen = sizeof(cad);

	 int iterator = 0;
	 bool usernameNegociated = false;
	 while(1){
		 nfds = 0;
		 largestInitialFd(sdp,sdo,&nfds);
	 	 FD_ZERO(&readfds);
		 readfds = resetParObsSD(observerStruct.Port, participantStruct.Port, &nfds, sdp, sdo);

			//change null later
//=			fprintf(stderr, "%s\n", "waiting for select");
				if(select(nfds + 1, &readfds, NULL , NULL, NULL)== -1) {
            perror("select");
            exit(4);
        }
				for(int i = 0; i <= nfds; i++){
					   if (FD_ISSET(i, &readfds)) {// (FD_ISSET(observerSocket, &readfds))(FD_ISSET(participantSOcket, &readfds))
//=							fprintf(stderr, "something happening on port: %d\n", i);
							 if(i == sdp){
								 //if participant
								 iterator = 0;
								 while(participantStruct.Port[iterator] != 0){
									 iterator++;
								 }
								 participantStruct.Port[iterator] = accept(sdp, (struct sockaddr *)&cad, &alen);
								 numbParticipant++;
	//=							fprintf(stderr, "inserting new participant at place %d, port: %d\n", iterator, participantStruct.Port[iterator]);
								 if(numbParticipant < 256){
									 //n = send the participant a Y
									 serverResponse = 'Y';
									 n = send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
									 if(n == -1){
						 				close(participantStruct.Port[iterator]);
										participantStruct.Port[iterator] = 0;
										numbParticipant--;
						 			 }
									 else{
									 usernameNegociated = false;
										 while(!usernameNegociated){
											 n = recv(participantStruct.Port[iterator], &PStringSize, sizeof(PStringSize),0);
											 if(n == 0){
												 close(participantStruct.Port[iterator]);
												 participantStruct.Port[iterator] = 0;
												 usernameNegociated = true;
												 numbParticipant--;
											 }
											 else{
												 recv(participantStruct.Port[iterator], username, PStringSize,0);
												 username[PStringSize] = '\0';
												 //check participanttrie tree
										//		fprintf(stderr, "username is :%s - %d\n", username, PStringSize);
												 if(!validUsername(username)){
	//=												 fprintf(stderr, "%s\n", "invalid user name");
													 serverResponse = 'I';
													 n = send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
													 if(n == -1){
														close(participantStruct.Port[iterator]);
														participantStruct.Port[iterator] = 0;
														usernameNegociated = true;
														numbParticipant--;
													 }
													 //dont reset timer
												 }
												 else{
													 if(trie_lookup(participantTree, username) == TRIE_NULL){
														 serverResponse = 'Y';
														 n = send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
														 if(n == -1){
															close(participantStruct.Port[iterator]);
 															participantStruct.Port[iterator] = 0;
															usernameNegociated = true;
															numbParticipant--;
														 }
														 else{
															 trie_insert(participantTree, username, &username);
															 strcpy(participantStruct.Usernames[iterator],username);
															 usernameNegociated = true;
				//=											 fprintf(stderr, "%s\n", username);
															 sprintf(serverBuff, "User %s has joined", username);
															 serverBuffSize = strlen(serverBuff);
															 for(int x = 0; x < 255; x++){
																 if(observerStruct.Port[x] != 0){
										//							fprintf(stderr, "Ports: %d\n", observerStruct.Port[x]);
										//							fprintf(stderr, "serverBuffSize: %d,  serverBuff: %s\n", serverBuffSize, serverBuff);
																	 n = send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
																	 if(n == -1){
																		close(participantStruct.Port[iterator]);
																		participantStruct.Port[iterator] = 0;
																		usernameNegociated = true;
																		numbParticipant--;
																	 }
																	 else{
				  												 	n = send(observerStruct.Port[x], serverBuff, serverBuffSize, 0);
																 	 }
																 }
															 }
														 }
													 }
													 else{
														 serverResponse = 'T';
														 n = send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
														 if(n == -1){
															close(participantStruct.Port[iterator]);
 															participantStruct.Port[iterator] = 0;
															usernameNegociated = true;
															numbParticipant--;
														 }
													 }
											 		}
												}
									 	 	}
										}
								 	}
								 else{ //too many participants
									 serverResponse = 'N';
									 n = send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
									 close(participantStruct.Port[iterator]);
									 participantStruct.Port[iterator] = 0;
									 numbParticipant--;
								 }
							 }//new participant found
							 else if(i == sdo){
								 iterator = 0;
								 while(observerStruct.Port[iterator] != 0){
									 iterator++;
								 }
								 observerStruct.Port[iterator] = accept(sdo, (struct sockaddr *)&cad, &alen);
								 numbObserver++;
								 if(numbObserver < 256){
									 //accept new observer
									 //n = send a Y
 			//=						 fprintf(stderr, "%s\n", "found new observer");
									 usernameNegociated = false;
									 serverResponse = 'Y';
									 n = send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
									 if(n == -1){
										close(observerStruct.Port[iterator]);
										observerStruct.Port[iterator] = 0;
										numbObserver--;
									 }
									 else{
										 while(!usernameNegociated){
											 n = recv(observerStruct.Port[iterator], &PStringSize, sizeof(PStringSize),0);
											 if(n == 0){
												 close(observerStruct.Port[iterator]);
												 observerStruct.Port[iterator] = 0;
												 usernameNegociated = true;
												 numbObserver--;
											 }
											 else{
												 recv(observerStruct.Port[iterator], username, PStringSize,0);
												 //check observertrie tree then participant
												 username[PStringSize] = '\0';
			//=									 fprintf(stderr, "username is :%s\n", username);
												 if(trie_lookup(observerTree, username) != TRIE_NULL){
													 serverResponse = 'T';
				//=									 fprintf(stderr, "%s\n", "username in observerTree");
													 n = send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
													 close(observerStruct.Port[iterator]);
													 observerStruct.Port[iterator] = 0;
													 usernameNegociated = true;
													 numbObserver--;
												 }
												 else if(trie_lookup(participantTree, username) == TRIE_NULL){
				//=									 fprintf(stderr, "%s\n", "username not in participantTree");
													 serverResponse = 'N';
													 n = send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
													 close(observerStruct.Port[iterator]);
													 observerStruct.Port[iterator] = 0;
													 usernameNegociated = true;
													 numbObserver--;
												 }
												 else{
													 serverResponse = 'Y';
													 n = send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), MSG_NOSIGNAL);
													 if(n == 0){
														observerStruct.Port[iterator] = 0;
														close(observerStruct.Port[iterator]);
														usernameNegociated = true;
														numbObserver--;
													 }
													 else{
														 trie_insert(observerTree, username, &username);
														 strcpy(observerStruct.Usernames[iterator],username);
														 sprintf(serverBuff, "A new observer has joined");
														 serverBuffSize = strlen(serverBuff);
		//=												fprintf(stderr, "buf len: %d messege: %s\n", serverBuffSize, serverBuff);
														 for(int x = 0; x < 255; x++){
															 if(observerStruct.Port[x] != 0){
													//			fprintf(stderr, "sec Ports: %d\n", observerStruct.Port[x]);
															//	fprintf(stderr, "sec serverBuffSize: %d,  serverBuff: %s\n", serverBuffSize, serverBuff);
																 n = send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
																 if(n == 0){
																	observerStruct.Port[iterator] = 0;
																	close(observerStruct.Port[iterator]);
																	numbObserver--;
																 }
																 else{
																 	n = send(observerStruct.Port[x], serverBuff, serverBuffSize, 0);
															 	 }
															 }
														 }
														 usernameNegociated = true;
													 }
												 }
											 }
									 	}
								  }
								}
								 else{//too many observer
									 serverResponse = 'N';
									 n = send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
								 }
							 }
					 else{
//=						 fprintf(stderr, "%s\n", "got into messege");
						 memset(prompt,0,13);
						 memset(serverBuff,0,1000);
						 memset(serverMessage,0,1013);
						 int observerLocation;
						 iterator = 0;
						 bool foundParticipant = true;
//=						  fprintf(stderr, "i is %d\n", i);
						 while(participantStruct.Port[iterator] != i && iterator < 256){
//=							fprintf(stderr, "%d\n",participantStruct.Port[iterator] );
							 iterator++;
							 if(iterator == 256){
								 foundParticipant = false;
							 }
						 }
						 if(foundParticipant){
	//=						 fprintf(stderr, "%s%s\n","message from ", participantStruct.Usernames[iterator] );
							 int senderL = iterator;
							 n = recv(participantStruct.Port[senderL], &serverBuffSize, sizeof(serverBuffSize),0);
							 if(n == 0){
		//=						 fprintf(stderr, "%s\n", "participant disconnected");
								//  observerLocation = findObserver(participantStruct.Usernames[senderL], observerStruct.Usernames);
								  int iterator1 = 0;
									while(strcmp(participantStruct.Usernames[senderL], observerStruct.Usernames[iterator1]) != 0){
											fprintf(stderr, "%s\n", observerStruct.Usernames[iterator1]);
											iterator1++;
								 }
								 observerLocation = iterator1;
								 sprintf(serverBuff, "User %s has left", username);
								 fprintf(stderr, "User %s has left\nobserver username %s", participantStruct.Usernames[senderL], observerStruct.Usernames[observerLocation]);
								 serverBuffSize = strlen(serverBuff);
								 for(int x = 0; x < 255; x++){
									 if(observerStruct.Port[x] != 0){
										 n = send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
										 if(n == 0){
											 memset(observerStruct.Usernames[x], 0, 10);
											 trie_remove(observerTree,observerStruct.Usernames[x]);
											 close(observerStruct.Port[x]);
											 observerStruct.Port[x] = 0;
											 numbObserver--;
										 }
										 else{
										 	n = send(observerStruct.Port[x], serverBuff, serverBuffSize, 0);
									 	 }
									 }
								 }
								 trie_remove(participantTree,participantStruct.Usernames[senderL]);
								 trie_remove(observerTree,participantStruct.Usernames[senderL]);
//=								 fprintf(stderr, "%s%s\n","should have removed", participantStruct.Usernames[senderL] );
								 close(participantStruct.Port[senderL]);
								 close(observerStruct.Port[observerLocation]);
								 numbObserver--;
								 numbParticipant--;
								 memset(participantStruct.Usernames[senderL],0,10);
								 memset(observerStruct.Usernames[observerLocation],0,10);
								 participantStruct.Port[senderL] = 0;
								 observerStruct.Port[observerLocation] = 0;
							 }
							 else{
								 recv(participantStruct.Port[senderL], serverBuff, serverBuffSize,0);
//=								 fprintf(stderr, "Server Buffer:%s\n", serverBuff);
								 prompt[0] = '>';
								 int promptlen = 10 - strlen(participantStruct.Usernames[senderL]);
								 int in;
								 for(in = 1; in < promptlen; in++){
									 prompt[in] = ' ';
								 }
								 strcat(prompt, participantStruct.Usernames[senderL]);
								 strcat(prompt, ": ");
//=								fprintf(stderr, "prompt: %s\n", prompt);
//=								fprintf(stderr, "message: %s\n", serverBuff);

								 strcat(serverMessage, prompt);
								 strcat(serverMessage, serverBuff);
								 serverBuffSize = strlen(serverMessage);
//=								 fprintf(stderr, "server message: %s\n", serverMessage);
//=							   fprintf(stderr, "message: %s\n", serverBuff);
//=							   fprintf(stderr, "message len: %d\n", serverBuffSize);
								if(serverBuff[0] == '@'){
									serverMessage[0] = '*';
									iterator = 1;
									memset(username,0,10);
									while(serverBuff[iterator] != ' ' && iterator < 10){
										username[iterator-1] = serverBuff[iterator];
										iterator++;
									}
									iterator = 0;
									usernameFound = false;
									while(!usernameFound && iterator < 255){
										if(strcmp(observerStruct.Usernames[iterator], username) == 0){
											usernameFound = true;
										}
										else{
											iterator++;
										}
									}
//=									fprintf(stderr, "n = sending private message to %s : %s\n ", username, serverMessage);
									if(usernameFound){
										fprintf(stderr, "%s\n", "user name found");
										n = send(observerStruct.Port[iterator], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
										if(n == 0){
											trie_remove(observerTree,observerStruct.Usernames[iterator]);
											memset(observerStruct.Usernames[iterator], 0, 10);
											close(observerStruct.Port[iterator]);
											observerStruct.Port[iterator] = 0;
											numbObserver--;
										}
										else{
											n = send(observerStruct.Port[iterator], serverMessage, serverBuffSize, 0);
										}
									}
									else{
//=										fprintf(stderr, "%s%s\n", "user name not found: ", username);
										iterator = 0;
										usernameFound = false;
										while(!usernameFound && iterator < 255){
											if(strcmp(observerStruct.Usernames[iterator],participantStruct.Usernames[senderL]) == 0){
												usernameFound = true;
											}
											iterator++;
										}
//=										fprintf(stderr, "%s %d\n", "n = senders observer found", usernameFound);
										if(usernameFound == true){
										 	memset(serverBuff,0,1000);
											sprintf(serverBuff, "Warning: user %s doesn't exist...", username);
											n = send(observerStruct.Port[iterator], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
											if(n == 0){
												trie_remove(observerTree,observerStruct.Usernames[iterator]);
												memset(observerStruct.Usernames[iterator], 0, 10);
						 						close(observerStruct.Port[iterator]);
						 						observerStruct.Port[iterator] = 0;
												numbObserver--;
											}
											else{
												n = send(observerStruct.Port[iterator], serverMessage, serverBuffSize, 0);
											}
										}
									}
								}
								else{
									for(int x = 0; x < 255; x++){
	 								 if(observerStruct.Port[x] != 0){
	 									 n = send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), MSG_NOSIGNAL);
	 									 if(n == 0){
											  trie_remove(observerTree,observerStruct.Usernames[x]);
		 									  memset(observerStruct.Usernames[x], 0, 10);
						 						close(observerStruct.Port[x]);
						 						observerStruct.Port[x] = 0;
												numbObserver--;
										 }
	 									 else{
	 									 	n = send(observerStruct.Port[x], serverMessage, serverBuffSize, 0);
	 								 	 }
	 								 }
	 							 }
							 }//private message sent from participant
					 	}
					}
					else{
//=						fprintf(stderr, "%s\n", "got into observer disconnected");
						iterator = 0;
						while(observerStruct.Port[iterator] != i && iterator < 255){
							iterator++;
						}
						trie_remove(observerTree,observerStruct.Usernames[iterator]);
						memset(observerStruct.Usernames[iterator], 0, 10);
						close(observerStruct.Port[iterator]);
						observerStruct.Port[iterator] = 0;
						numbObserver--;
					}
				 }//message found.  Messageing logic
			 }//FD_SET
		 }//end of for select loop
	} //end of the while.  Chatroom loop
} //end of the program


//Attempt at timeout


//
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <stdbool.h>
// #include <ctype.h>
//
// #include "trie.h"
// #include "trie.c"
//
// #define QLEN 6 /* size of request queue */
//
// /*------------------------------------------------------------------------
// * Program: server
// *
// * Purpose: allocate a socket and then repeatedly execute the following:
// * (1) wait for the next connection from a client
// * (2) send a short message to the client
// * (3) close the connection
// * (4) go back to step (1)
// *
// * Syntax: server [ port ]
// *
// * port - protocol port number to use
// *
// * Note: The port argument is optional. If no port is specified,
// * the server uses the default given by PROTOPORT.
// *
// *------------------------------------------------------------------------
// */
//
//
// bool validUsername(char username[]){
// 	for(int i = 0; i < strlen(username); i++){
// 		if(!isdigit(username[i]) && !isalpha(username[i]) && username[i] != '_'){
// 			return false;
// 		}
// 	}
// 	return true;
// }
//
// struct participants {
// 	int Port[256];
// 	char Usernames[256][10];
// 	long int Time[256];
// };
// struct observers {
// 	int Port[256];
// 	char Usernames[256][10];
// 	long int Time[256];
// };
//
// int setSocket(struct sockaddr_in sad, char argss[]){
// 	int sd;
// 	int port = atoi(argss); /* convert argument to binary */
// 	struct protoent *ptrp;
// 	int optval = 1; /* boolean value when we set socket option */
// 	if (port > 0) { /* test for illegal value */
// 		sad.sin_port = htons((u_short)port);
// 	} else { /* print error message and exit */
// 		fprintf(stderr,"Error: Bad port number %s\n",argss);
// 		exit(EXIT_FAILURE);
// 	}
//
// 	/* Map TCP transport protocol name to protocol number */
// 	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
// 		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
// 		exit(EXIT_FAILURE);
// 	}
//
// 	/* Create a socket */
// 	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
// 	if (sd < 0) {
// 		fprintf(stderr, "Error: Socket creation failed\n");
// 		exit(EXIT_FAILURE);
// 	}
//
// 	/* Allow reuse of port - avoid "Bind failed" issues */
// 	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
// 		fprintf(stderr, "Error Setting socket option failed\n");
// 		exit(EXIT_FAILURE);
// 	}
//
// 	/* Bind a local address to the socket */
// 	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
// 		fprintf(stderr,"Error: Bind failed\n");
// 		exit(EXIT_FAILURE);
// 	}
//
// 	/* Specify size of request queue */
// 	if (listen(sd, QLEN) < 0) {
// 		fprintf(stderr,"Error: Listen failed\n");
// 		exit(EXIT_FAILURE);
// 	}
// 	return sd;
// }
//
// void largestInitialFd(int sdp, int sdo, int *nfds){
// 	if(sdp > sdo){
// 		*nfds = sdp;
// 	}
// 	if(sdo > sdp){
// 		*nfds = sdo;
// 	}
// }
// fd_set resetParObsSD(int observerConnections[], int participantConnections[], int *nfds, int sdp, int sdo){
// 	int iterator = 0;
// 	fd_set tempFd;
// 	FD_ZERO(&tempFd);
// 	FD_SET(sdp, &tempFd);
// 	FD_SET(sdo, &tempFd);
// 	while(iterator < 255){
// 		//check if set and only then add to read, sd = ..
// 	 //  fprintf(stderr, "%d-%d got here\n", iterator, participantConnections[iterator]);
// 		 if(participantConnections[iterator] > 0){
// 			 FD_SET(participantConnections[iterator], &tempFd);
// 			 fprintf(stderr, "participant found at position %d, port: %d\n", iterator, participantConnections[iterator]);
// 		 }
// 		 if(observerConnections[iterator] > 0){
// 			 FD_SET(observerConnections[iterator], &tempFd);
// 		 }
// 		 if(participantConnections[iterator] > *nfds){
// 			 *nfds = participantConnections[iterator];
// 		 }
// 		 if(observerConnections[iterator] > *nfds){
// 			 *nfds = observerConnections[iterator];
// 		 }
// 		 iterator++;
// 	 }
// 	 return tempFd;
// }
//
// int findMinTime(int timesLeft[]){
// 	int minTimeLeft = 61;
// 	for(int i = 0; i < 255; i++){
// 		if(minTimeLeft > timesLeft[i]){
// 			minTimeLeft = timesLeft[i];
// 		}
// 	}
// 	return minTimeLeft;
// }
//
// int main(int argc, char **argv) {
// 	struct protoent *ptrp; /* pointer to a protocol table entry */
// 	struct sockaddr_in sad,sad2; /* structure to hold server's address */
// 	struct sockaddr_in cad;
// 	struct timeval tv;
// 	int sdp,sdo; /* socket descriptors */
// 	fd_set readfds;
// 	int numbParticipant = 0;
// 	int numbObserver = 0;
// 	int nfds = 0;
// 	int n;
// 	int alen;/* length of address */
// 	uint8_t PStringSize = 0; /* buffer for data from the server */
// 	uint16_t serverBuffSize = 0;
// 	char username[10];
// 	char serverResponse;
// 	char serverBuff[1000];
//  	char prompt[13];
// 	char serverMessage[1013];
// 	bool usernameFound;
// 	long int lastTimeFound = 0;
//
// 	Trie *observerTree = trie_new();
// 	Trie *participantTree = trie_new();
//
// 	struct participants participantStruct;
// 	struct observers observerStruct;
// 	for(int i = 0; i < 255; i++){
// 		participantStruct.Port[i] = 0;
// 		participantStruct.Usernames[i][0] = '$';
// 		participantStruct.Time[i] = -1;
// 		observerStruct.Port[i] = 0;
// 		observerStruct.Usernames[i][0] = '$';
// 		observerStruct.Time[i] = -1;
//
// 	}
// 	tv.tv_sec = 0;
//
// 	signal(SIGCHLD,SIG_IGN);
//
// 	if( argc != 3 ) {
// 		fprintf(stderr,"Error: Wrong number of arguments\n");
// 		fprintf(stderr,"usage:\n");
// 		fprintf(stderr,"./prog3_server server_participant_port server_observer_port\n");
// 		exit(EXIT_FAILURE);
// 	}
// ////////////////////////////observer socket
// 	sdo = setSocket(sad, argv[2]);
// ///////////////////////////////////////////////////////////////////////////
// //----------------------------------------------------------------participant socket
// 	sdp = setSocket(sad2, argv[1]);
// //-----------------------------------------------------------------------------------------
//
// 	alen = sizeof(cad);
//
// 	 int iterator = 0;
// 	 bool usernameNegociated = false;
// 	 while(1){
// 		 nfds = 0;
// 		 largestInitialFd(sdp,sdo,&nfds);
// 	 	 FD_ZERO(&readfds);
// 		 readfds = resetParObsSD(observerStruct.Port, participantStruct.Port, &nfds, sdp, sdo);
//
// 			//change null later
// 				if(select(nfds + 1, &readfds, NULL , NULL, 0)== -1) {
//             perror("select");
//             exit(4);
//         }
// 				for(int i = 0; i <= nfds; i++){
// 					   if (FD_ISSET(i, &readfds)) {// (FD_ISSET(observerSocket, &readfds))(FD_ISSET(participantSOcket, &readfds))
// 							 fprintf(stderr, "something happening on port: %d\n", i);
// 							 if(i == sdp){
// 								 //if participant
// 								 if(numbParticipant < 255){
// 									 iterator = 0;
// 									 while(participantStruct.Port[iterator] != 0){
// 										 	iterator++;
// 									 }
// 									 participantStruct.Port[iterator] = accept(sdp, (struct sockaddr *)&cad, &alen);
// 									 fprintf(stderr, "inserting new participant at place %d, port: %d\n", iterator, participantStruct.Port[iterator]);
// 									 //send the participant a Y
// 									 serverResponse = 'Y';
// 									 send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 									 usernameNegociated = false;
// 									 while(!usernameNegociated){///////////////////////////////////////////////////////////////
// 										 recv(participantStruct.Port[iterator], &PStringSize, sizeof(PStringSize),0);
// 										 recv(participantStruct.Port[iterator], username, PStringSize,0);
// 										 username[PStringSize] = '\0';
// 										 //check participanttrie tree
// 										//  fprintf(stderr, "username is :%s - %d\n", username, PStringSize);
// 										 if(!validUsername(username)){
// 											 serverResponse = 'T';
// 											 send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 											 //dont reset timer
// 										 }
// 										 else{
// 											 if(trie_lookup(participantTree, username) == TRIE_NULL){
// 												 serverResponse = 'Y';
// 												 send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 												 trie_insert(participantTree, username, &username);
// 												 strcpy(participantStruct.Usernames[iterator],username);
// 												 numbParticipant++;
// 												 usernameNegociated = true;
// 												 sprintf(serverBuff, "User %s has joined", username);
// 												 serverBuffSize = strlen(serverBuff);
// 												 for(int x = 0; x < 255; x++){
// 													 if(observerStruct.Port[x] != 0){
// 														//  fprintf(stderr, "Ports: %d\n", observerStruct.Port[x]);
// 														//  fprintf(stderr, "serverBuffSize: %d,  serverBuff: %s\n", serverBuffSize, serverBuff);
// 														 send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), 0);
// 	  												 send(observerStruct.Port[x], serverBuff, serverBuffSize, 0);
// 													 }
// 												 }
// 											 }
// 											 else{
// 												 serverResponse = 'T';
// 												 send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 												 //reset timer
// 											 }
// 									 		}
// 								 	 	}
// 								 	}
// 								 else{ //too many participants
// 									 serverResponse = 'N';
// 									 send(participantStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 								 }
// 							 }//new participant found
// 							 else if(i == sdo){
// 								 if(numbObserver < 255){
// 									 //accept new observer
// 									 iterator = 0;
// 									 while(observerStruct.Port[iterator] != 0){
// 										 	iterator++;
// 									 }
// 									 observerStruct.Port[iterator] = accept(sdo, (struct sockaddr *)&cad, &alen);
// 									 //send a Y
//  									// 	 fprintf(stderr, "%s\n", "found new observer");
// 									 usernameNegociated = false;
// 									 serverResponse = 'Y';
// 									 send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 									 while(!usernameNegociated){/////////////////////////////////////////////////////////////////////////////
// 										 recv(observerStruct.Port[iterator], &PStringSize, sizeof(PStringSize),0);
// 										 recv(observerStruct.Port[iterator], username, PStringSize,0);
// 										 //check observertrie tree then participant
// 										 username[PStringSize] = '\0';
// 										 fprintf(stderr, "username is :%s\n", username);
// 										 if(trie_lookup(observerTree, username) != TRIE_NULL){
// 											 serverResponse = 'T';
// 											 fprintf(stderr, "%s\n", "username not in observerTree");
// 											 send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 											 close(observerStruct.Port[iterator]);
// 											 observerStruct.Port[iterator] = 0;
// 										 }
// 										 else if(trie_lookup(participantTree, username) == TRIE_NULL){
// 											 fprintf(stderr, "%s\n", "username not in participantTree");
// 											 serverResponse = 'N';
// 											 send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 											 close(observerStruct.Port[iterator]);
// 											 observerStruct.Port[iterator] = 0;
// 										 }
// 										 else{
// 											 serverResponse = 'Y';
// 											 send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 											 trie_insert(observerTree, username, &username);
// 											 strcpy(observerStruct.Usernames[iterator],username);
// 											 numbObserver++;
// 											 sprintf(serverBuff, "A new observer has joined");
// 											 serverBuffSize = strlen(serverBuff);
// 											//  fprintf(stderr, "buf len: %d messege: %s\n", serverBuffSize, serverBuff);
// 											 for(int x = 0; x < 255; x++){
// 												 if(observerStruct.Port[x] != 0){
// 													//  fprintf(stderr, "sec Ports: %d\n", observerStruct.Port[x]);
// 													//  fprintf(stderr, "sec serverBuffSize: %d,  serverBuff: %s\n", serverBuffSize, serverBuff);
// 													 send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), 0);
// 													 send(observerStruct.Port[x], serverBuff, serverBuffSize, 0);
// 												 }
// 											 }
// 											 usernameNegociated = true;
// 										 }
// 								 	}
// 								}
// 								 else{//too many observer
// 									 serverResponse = 'N';
// 									 send(observerStruct.Port[iterator], &serverResponse, sizeof(serverResponse), 0);
// 								 }
// 							 }
// 					 else{
// 						 memset(prompt,0,13);
// 						 memset(serverBuff,0,1000);
// 						 memset(serverMessage,0,1013);
// 						 fprintf(stderr, "%s%s\n","message from ", participantStruct.Usernames[iterator] );
// 						 int senderL = iterator;
// 						 recv(participantStruct.Port[senderL], &serverBuffSize, sizeof(serverBuffSize),0);
// 						 recv(participantStruct.Port[senderL], serverBuff, serverBuffSize,0);
// 						 fprintf(stderr, "Server Buffer:%s\n", serverBuff);
// 						 prompt[0] = '>';
// 						 int promptlen = 10 - strlen(participantStruct.Usernames[senderL]);
// 						 int in;
// 						 for(in = 1; in < promptlen; in++){
// 							 prompt[in] = ' ';
// 						 }
// 						 strcat(prompt, participantStruct.Usernames[senderL]);
// 						 strcat(prompt, ": ");
// 						//  fprintf(stderr, "prompt: %s\n", prompt);
// 						//  fprintf(stderr, "message: %s\n", serverBuff);
//
// 						 strcat(serverMessage, prompt);
// 						 strcat(serverMessage, serverBuff);
// 						 serverBuffSize = strlen(serverMessage);
// 						//  fprintf(stderr, "server message: %s\n", serverMessage);
// 					//	 fprintf(stderr, "message: %s\n", serverBuff);
// 					//	 fprintf(stderr, "message len: %d\n", serverBuffSize);
// 						if(serverBuff[0] == '@'){
// 							serverMessage[0] = '*';
// 							iterator = 1;
// 							memset(username,0,10);
// 							while(serverBuff[iterator] != ' ' && iterator < 10){
// 								username[iterator-1] = serverBuff[iterator];
// 								iterator++;
// 							}
// 							iterator = 0;
// 							usernameFound = false;
// 							while(!usernameFound && iterator < 255){
// 								if(strcmp(observerStruct.Usernames[iterator], username) == 0){
// 									usernameFound = true;
// 								}
// 								else{
// 									iterator++;
// 								}
// 							}
// 							fprintf(stderr, "sending private message to %s : %s\n ", username, serverMessage);
// 							if(usernameFound){
// 								fprintf(stderr, "%s\n", "user name found");
// 								send(observerStruct.Port[iterator], &serverBuffSize, sizeof(serverBuffSize), 0);
// 								send(observerStruct.Port[iterator], serverMessage, serverBuffSize, 0);
// 							}
// 							else{
// 								fprintf(stderr, "%s%s\n", "user name not found: ", username);
// 								iterator = 0;
// 								usernameFound = false;
// 								while(!usernameFound && iterator < 255){
// 									if(strcmp(observerStruct.Usernames[iterator],participantStruct.Usernames[senderL]) == 0){
// 										usernameFound = true;
// 									}
// 									iterator++;
// 								}
// 								fprintf(stderr, "%s %d\n", "senders observer found", usernameFound);
// 								if(usernameFound == true){
// 								 	memset(serverBuff,0,1000);
// 									sprintf(serverBuff, "Warning: user %s doesn't exist...", username);
// 									send(observerStruct.Port[iterator], &serverBuffSize, sizeof(serverBuffSize), 0);
// 									send(observerStruct.Port[iterator], serverMessage, serverBuffSize, 0);
// 								}
// 							}
// 						}
// 						else{
// 						 for(int x = 0; x < 255; x++){
// 							 if(observerStruct.Port[x] != 0){
// 								 send(observerStruct.Port[x], &serverBuffSize, sizeof(serverBuffSize), 0);
// 								 send(observerStruct.Port[x], serverMessage, serverBuffSize, 0);
// 								 fprintf(stderr, "sending to: %s\n", observerStruct.Usernames[x]);
// 					 		}
// 				 		}
// 			 		}//private message sent from participant
// 		 		}//message found.  Messageing logic
// 			 }//FD_SET
// 		 }//end of for select loop
// 	} //end of the while.  Chatroom loop
// } //end of the program
//
//
// //
// // if(recv(participantStruct.Port[senderL], &serverBuffSize, sizeof(serverBuffSize), MSG_PEEK | MSG_DONTWAIT) == 0){
// // 	close(participantStruct.Port[senderL]);
// // 	participantStruct.Port[senderL] = 0;
// // 	trie_remove(participantTree,participantStruct.Usernames[senderL]);
// // 	iterator = 0;
// // 	usernameFound = false;
// // 	while(!usernameFound && iterator < 255){
// // 		if(strcmp(observerStruct.Usernames[iterator], participantTree,participantStruct.Usernames[senderL]) == 0){
// // 			usernameFound = true;
// // 		}
// // 		else{
// // 			iterator++;
// // 		}
// // 	}
// // 	memset(participantStruct.Usernames[senderL],0,10);
// // }






//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"


int main() {
  	// ... ADD SOME VARIABLES HERE ... //
  	//Creates the variables needed for creating the stop client and the buffer for sending data back and forth
	int clientSocket, addrSize, bytesReceived;
	struct sockaddr_in serverAddr;
 	float buffer[10]; 
  
  	// Register with the server
  	// ... WRITE SOME CODE HERE ... //
  	//Creates the socket for the client, if it fails, it exits the program
	clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (clientSocket < 0) {
 		printf("*** CLIENT ERROR: Could open socket.\n");
 		exit(-1);
 	}
	memset(&serverAddr, 0, sizeof(serverAddr));
 	serverAddr.sin_family = AF_INET;
 	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
 	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

	addrSize = sizeof(serverAddr);
  
  	// Send command string to server
  	// ... WRITE SOME CODE HERE ... //
  	
	//Prints when the STOP command is being sent
	printf("STOP COMMAND CLIENT: SENDING STOP COMMAND TO SERVER\n\n");

	//The STOP command is stored in the buffer
	buffer[0] = STOP;
	
	//The buffer is sent to the server
	sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &serverAddr, addrSize);

	//The stop client socket is closed and the program ends
	close(clientSocket); 

}


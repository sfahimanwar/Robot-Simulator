#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"




// This is the main function that simulates the "life" of the robot
// The code will exit whenever the robot fails to communicate with the server
int main() {
  	// ... ADD SOME VARIABLE HERE ... //
  	//Set thes necessary variables for the client
	int clientSocket, addrSize, bytesReceived;
	struct sockaddr_in serverAddr;
	//Stores the responses from the server
	char array[30];
	//Used for sending data to the server
	float buffer[10];
	
	//Locally stores the ID of the robot, x,y and direction values
	int robotID;
	float x, y;
	int direction;

	float robotData[4];

	int collisionCount = 0;  //when the loop is iterating and robot is trying to move, the variable stores how many times it has collided with a boundary or robot
	int randomDirection = 0; //when the loop is iterating and robot is trying to move, either 1 or 0 is chosen randomly which decides whether robot turns CW or CCW

  	// Set up the random seed
  	srand(time(NULL));
  
  	// Register with the server
	clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (clientSocket < 0) {
 		printf("CLIENT ERROR: Could open socket.\n");
 		exit(-1);
 	}
	memset(&serverAddr, 0, sizeof(serverAddr));
 	serverAddr.sin_family = AF_INET;
 	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
 	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

	addrSize = sizeof(serverAddr);

  	// Send register command to server.  Get back response data
  	// and store it.   If denied registration, then quit.

	buffer[0] = REGISTER;

	//Sends the REGISTER command to the server
	sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &serverAddr, addrSize);
	
	//Receives the response from the server and stores it in the char array
	bytesReceived = recvfrom(clientSocket, array, sizeof(array), 0, (struct sockaddr *) &serverAddr, &addrSize);
	array[bytesReceived] = 0;

	//If it received a NOT_OK response, the client socket is closed and the main function quits
	if(strcmp(array, NOT_OK) == 0){
		printf("CLIENT: CANNOT REGISTER ANY MORE ROBOTS, QUITTING.\n");
		close(clientSocket);
		return 0;
	}else if(strcmp(array, OK) == 0){
		//If response was OK, it receives and stores the robot data sent from the server
		recvfrom(clientSocket, &robotData, sizeof(robotData), 0, (struct sockaddr *) &serverAddr, &addrSize);

		robotID = (int)robotData[0];
		x = robotData[1];
		y = robotData[2];
		direction = (int)robotData[3];

		printf("ROBOT ID: %d\n", robotID);
		printf("X : %.2f\n", x);
		printf("Y : %.2f\n", y);
		printf("Direction: %d\n\n", direction);
		
	}

  	// Go into an infinite loop exhibiting the robot behavior
  	while (1) {
    	// Check if it can move forward
    	
    	//Sends a CHECK_COLLISION request to the server along with the robots ID and its data
    	//so the server can use it to calculate if it'll hit a boundary
		buffer[0] = CHECK_COLLISION;
		buffer[1] = robotID;
		buffer[2] = x;
		buffer[3] = y;
		buffer[4] = direction;

		sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &serverAddr, addrSize);

    	// Get response from server
		bytesReceived = recvfrom(clientSocket, array, sizeof(array), 0, (struct sockaddr *) &serverAddr, &addrSize);
		
		array[bytesReceived] = 0;

		//If the response was a LOST_CONTACT, the robot breaks from the while loop, also prints out which robot is shutting down
		if(strcmp(array, LOST_CONTACT) == 0){
			printf("CLIENT %d: SHUTTING DOWN\n", robotID);
			break;
		}

		// If the response received was OK, move forward
		if(strcmp(array, OK) == 0){
			//Calculates the new X and Y value using the formula given, 
			//the degrees is converted to radians for use in the sin and cos function
			x = x + ROBOT_SPEED * cos((direction) * (PI/180));
			y = y + ROBOT_SPEED * sin((direction) * (PI/180));
			//Resets the collision count
			collisionCount = 0;
		}
    	// Otherwise, we could not move forward, so make a turn.
		else{
			//Increments the collisionCount counter
			collisionCount++;
			//If it's collided more than once, it will continue to turn in the same direction
			if(collisionCount >= 2){
				//If the direction was CW
				if(randomDirection == 1){
					//Makes sure angle stays within +/- 180 
					if(direction + ROBOT_TURN_ANGLE > 180){
						float temp = (direction + ROBOT_TURN_ANGLE) - 180;
						direction = -(temp);
					}else{
						direction += ROBOT_TURN_ANGLE;
					}
				}else{ //If the direction was CCW
					//Makes sure angle stays within +/- 180 
					if(direction - ROBOT_TURN_ANGLE < -180){
						float temp = (direction - ROBOT_TURN_ANGLE) + 180;
						direction = -(temp);
					}else{
						direction -= ROBOT_TURN_ANGLE;
					}
				}
			
			}else{
				//This block executes when it has collided only once
				//Picks 0 or 1 randomly, 0 for CCW and 1 for CW
				//Ensures angle stays within +/- 180 degrees
				randomDirection = (int)(rand()/(double)RAND_MAX*(2));
				if(randomDirection == 1){
					if(direction + ROBOT_TURN_ANGLE > 180){
						float temp = (direction + ROBOT_TURN_ANGLE) - 180;
						direction = -(temp);
					}else{
						direction += ROBOT_TURN_ANGLE;
					}
				}else{
					if(direction - ROBOT_TURN_ANGLE < -180){
						float temp = (direction - ROBOT_TURN_ANGLE) + 180;
						direction = -(temp);
					}else{
						direction -= ROBOT_TURN_ANGLE;
					}
				}
			}
		}   
    	// Send STATUS_UPDATE to server with the new X, Y and direction values
		buffer[0] = STATUS_UPDATE;
		buffer[1] = robotID;
		buffer[2] = x;
		buffer[3] = y;
		buffer[4] = direction;

		sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &serverAddr, addrSize);
    
    	// Uncomment line below to slow things down a bit 
    	usleep(20000);
  	}
	close(clientSocket);
}



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include "simulator.h"

//A function that returns a random float between two specified min and max values inclusive, used for generating random x,y values
float float_rand( float min, float max ){
  float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
  return min + scale * ( max - min );      /* [min, max] */
}



Environment environment;  // The environment that contains all the robots
Environment *env = &environment; //Environment pointer for the threads

//Variable used to store how many robots were sent the LOST_CONTACT message so that the server can know when to shut down
int robotsNotified = 0;



// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should repeatedly grab an incoming messages and process them. 
// The requests that may be handled are STOP, REGISTER, CHECK_COLLISION and STATUS_UPDATE.   
// Upon receiving a STOP request, the server should get ready to shut down but must
// first wait until all robot clients have been informed of the shutdown.   Then it 
// should exit gracefully.  
void *handleIncomingRequests(void *e) {
	char   online = 1;

	//Stores the void pointer passed in as an Environment pointer
	Environment * env = e;
	//Sets the initial number of robots to 0
	env->numRobots = 0;
  	// ... ADD SOME VARIABLE HERE... //
  	//Creates the variables for initializing the server
	int serverSocket;
	struct sockaddr_in serverAddr, clientAddr;
	int status, addrSize, bytesReceived;
	fd_set readfds, writefds;
	float buffer[10];
	
  	// Initializes the server
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket < 0) {
 		printf("SERVER ERROR: Could not open socket.\n");
 		exit(-1);
 	}	
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
 	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
 	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);	
 
	//Tries to bind the socket, exits program if it fails
	status = bind(serverSocket,(struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (status < 0) {
 		printf("SERVER ERROR: Could not bind socket.\n");
 		exit(-1);
 	}

  	// Wait for clients now, goes into a loop
	while (online) {
		// ... WRITE YOUR CODE HERE ... //
		//Sets some variables and values for clients to be received
		FD_ZERO(&readfds);
 		FD_SET(serverSocket, &readfds);
 		FD_ZERO(&writefds);
 		FD_SET(serverSocket, &writefds);

		status = select(FD_SETSIZE, &readfds, &writefds, NULL, NULL);
 		if (status == 0) {
 		}
 		else if (status < 0) {
 			printf("SERVER ERROR: Could not select socket.\n");
 			exit(-1);
 		}
 		//If successful in selecting clients, starts receiving the data
		else{
			addrSize = sizeof(clientAddr);
			//receives the buffer of data containing the request
			recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &addrSize);

			if (bytesReceived > 0) {
 				printf("SERVER: Received client request: %d\n", (int)buffer[0]);
 			}
			
			//If the request was a STOP command, the shutDown value of the environment is set to 1 so at the end of the program, 
			//it can break out of the loop
			if((int)buffer[0] == STOP){
				env->shutDown = 1;
				printf("SERVER: INFORMING ALL CLIENTS OF SHUT DOWN.\n\n");
			}

			//The handler if the request was a REGISTER command from a client
			if((int)buffer[0] == REGISTER){
				//If adding a robot makes the number of robots in the environment more than 20, sends a NOT_OK response to the client
				if(env->numRobots + 1 > 20){
					printf("SERVER: NOT ALLOWED TO REGISTER ANY MORE ROBOTS.\n\n");
					sendto(serverSocket, NOT_OK, strlen(NOT_OK), 0, (struct sockaddr *) &clientAddr, addrSize);
				}else{
					//If the number of robots is less than 20, it sends an OK response to the client, and registers a new robot
					sendto(serverSocket, OK, strlen(OK), 0, (struct sockaddr *) &clientAddr, addrSize);

					//Defines and initializes a new robot
					Robot robot;
					//Use the float_rand function to assign a random X and Y value
					robot.x = float_rand(ROBOT_RADIUS, (ENV_SIZE) - ROBOT_RADIUS);
					robot.y = float_rand(ROBOT_RADIUS, (ENV_SIZE) - ROBOT_RADIUS);
					
					//Assigns a random direction using rand for values between 180 and -180
					robot.direction = rand()%(180-(-180) + 1) + (-180);
					
					//Whether the randomly chosen X,Y values are unique and dont overlap with other robots in the environment
					int unique = 1;
					
					//Goes into a while until it finds a ranom location for the robot
					while(1){
						//Iterates through all the robot array in the environment to check if it's overlapping
						for(int i =0; i<env->numRobots; i++){
							//If it overlaps unique is set to 0 and it breaks from the for loop
							if(robot.x == env->robots[i].x && robot.y == env->robots[i].y){
								unique = 0;
								break;
							}
						}
						//If the location values generated was unique, it breaks from the while loop
						if(unique == 1){
							break;
						}
						//Otherwise new random values are generated
						robot.x = float_rand(ROBOT_RADIUS, (ENV_SIZE) - ROBOT_RADIUS);
						robot.y = float_rand(ROBOT_RADIUS, (ENV_SIZE) - ROBOT_RADIUS);
					}
					
					//Adds the robot to the robots array of the environment and increments the numRobots by 1
					env->robots[env->numRobots] = robot;
					env->numRobots++ ;
				
					//Creates a float array to store the robot data to send to the client to store
					float robotData[4];
					robotData[0] = env->numRobots - 1;
					robotData[1] = robot.x;
					robotData[2] = robot.y;
					robotData[3] = robot.direction;

					sendto(serverSocket, robotData , sizeof(robotData), 0, (struct sockaddr *) &clientAddr, addrSize);
				}
			}
			//Checks if the request send was a CHECK_COLLISION command
			if((int)buffer[0] == CHECK_COLLISION){
				//Stores the robotID sent along with the CHECK_COLLISION command and the robot data along with it for collision check
				int robotID = (int) buffer[1];
				float x = buffer[2];
				float y = buffer[3];
				int direction = (int)buffer[4];
				

				//If the shutDown value was set to 1 after receiving a STOP request earlier in the program,
				//A LOST_CONTACT command is sent to the robot clients when they check for collisions and the robotsNotified counter is incremented,
				//which is used later in the program to see if all the robot clients have been notified of the shutdown
				if(env->shutDown == 1){
					sendto(serverSocket, LOST_CONTACT, strlen(LOST_CONTACT), 0, (struct sockaddr *) &clientAddr, addrSize);
					robotsNotified++;
				}else{
					
					int newX, newY;

					//The new X and Y values when the robot moves is calculated 
					newX = x + ROBOT_SPEED * cos((direction) * (PI/180));
					newY = y + ROBOT_SPEED * sin((direction) * (PI/180));

					//Keeps track of whether the robot has collided with another robot or a boundary
					int collidedWithRobots = 0;

					//Iterates through the robots array to see if the robots is colliding with another robot (except itself)
					for(int i=0; i<env->numRobots; i++){
						if(robotID != i){
							if((pow(env->robots[i].x - newX, 2) + pow(env->robots[i].y - newY, 2)) <= pow(ROBOT_RADIUS * 2, 2)){
								//Sets the variable to 1 if collision is detected and then breaks
								collidedWithRobots = 1;
								break;
							}
						}
					}

					//If it collided with a robot, a NOT_OK_COLLIDE signal is sent to the robot client
					if(collidedWithRobots == 1){
						sendto(serverSocket, NOT_OK_COLLIDE, strlen(NOT_OK_COLLIDE), 0, (struct sockaddr *) &clientAddr, addrSize);
					}
					//If it collided with a boundary, a NOT_OK_BOUNDARY signal is sent to the robot client
					else if(newX + ROBOT_RADIUS >= ENV_SIZE || newY + ROBOT_RADIUS >= ENV_SIZE || newX - ROBOT_RADIUS <= 0 || newY - ROBOT_RADIUS <= 0 ){
						sendto(serverSocket, NOT_OK_BOUNDARY, strlen(NOT_OK_BOUNDARY), 0, (struct sockaddr *) &clientAddr, addrSize);
					}else{
						//Otherwise an OK signal is sent to the robot client
						sendto(serverSocket, OK , strlen(OK), 0, (struct sockaddr *) &clientAddr, addrSize);
					}

				}
			}
			//If the command receeived was a STATUS_UPDATE, it uses the extra data sent in the buffer to update the values in the array
			if((int)buffer[0] == STATUS_UPDATE){
				int robotID = (int) buffer[1];
				env->robots[robotID].x = buffer[2];
				env->robots[robotID].y = buffer[3];
				env->robots[robotID].direction = (int) buffer[4];
			}
			
		}
		//If the shutDown value is 1 and all the robot clients have been sent a LOST_CONTACT signal, it breaks from the while loop
		if(env->shutDown == 1 && robotsNotified == env->numRobots){
			printf("SERVER: SHUTTING DOWN.\n\n");
			break;
		}	
  	}
  	// ... WRITE ANY CLEANUP CODE HERE ... //
  	//Exits the thread
  	pthread_exit(NULL);
}




int main() {
	// So far, the environment is NOT shut down
	environment.shutDown = 0;
  
	// Set up the random seed
	srand(time(NULL));

	//The two threads created to handle the drawing of the display and the server for handling the clients
	pthread_t clientThread;
	pthread_t redrawThread;

	// Spawn an infinite loop to handle incoming requests and update the display

	pthread_create(&clientThread, NULL, handleIncomingRequests, env);
	pthread_create(&redrawThread, NULL, redraw, env);

	// Wait for the update and draw threads to complete
	pthread_join(clientThread, NULL);
}


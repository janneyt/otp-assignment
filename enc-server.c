#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wait.h>
# include "one-time/one-time.h"
# include "constants.h"


// Error function used for reporting issues
void
error (const char *msg)
{
  perror (msg);
  exit (EXIT_FAILURE);
}

// Setup the address struct for the server socket 

int setupAddressStruct (struct sockaddr_in *address, int portNumber){

  // Clear out hte address struct
  memset ((char *) address, '\0', sizeof (*address));

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number 
  address->sin_port = htons (portNumber);
  // Allow a client at any address to connect to this server 
  address->sin_addr.s_addr = INADDR_ANY;
  return EXIT_SUCCESS;
}

int main (int argc, char *argv[])
{
	char hello[MAX_MSG_LEN + 1] = "HELLO";
	char result[MAX_MSG_LEN + 1] = "";
	char key[MAX_MSG_LEN + 1] = "XMCKL";
	if(encrypt_one_time_pad(hello, key, result) == EXIT_FAILURE){
		error("Could not encrypt from standard input");
	};
  	int connectionSocket, charsRead;
  	char buffer[256];
  	char veribuffer[256];
  	struct sockaddr_in clientAddress;
  	socklen_t sizeOfClientInfo = sizeof (clientAddress);


  	int status;
       	pid_t pid, wpid;
  //int key_int;
  //FILE* key_file;
  //char key[4096];
	if(argc < 2){
		error("You forgot the port number");
	}
  	int supplied_port = atoi(argv[1]);
  	if(supplied_port > 65535 || supplied_port <= 1000){
		error("Valid port range is 1001 to 65535 inclusive");
	}
	if(argc > 2){
		error("The enc server only takes the port number");
  	}

  	// Create the socket that will listen for connections 
  	int listenSocket = socket (AF_INET, SOCK_STREAM, 0);
  	if (listenSocket < 0){
		error ("ERROR opening socket");
  	}

  	for(int index = 0; index < NUM_THREADS; index++){
    		// Blank out serverAddress
    		struct sockaddr_in serverAddress;

    		// Create the socket that will listen for connections 
    		int listenSocket = socket (AF_INET, SOCK_STREAM, 0);
    		if (listenSocket < 0){
	    		error ("ERROR opening socket");
		}
    		// Set up the address struct for the server socket    
		if(setupAddressStruct (&serverAddress, supplied_port + index) != EXIT_SUCCESS){
      			error("Could not setup address");
    		};
   
	    	if (bind (listenSocket, (struct sockaddr *)&serverAddress, sizeof (serverAddress)) != 0){
      			error ("Could not bind to socket");
		}
	

    		// Start listening for connections. Allow up to 5 connections to queue up
    		if (listen (listenSocket, 5) < 0){
	      		perror ("Can't listen on those sockets");
    		};
    
		// Accept the connection request which creates a connection socket 
    		connectionSocket = accept (listenSocket, (struct sockaddr *) &clientAddress,&sizeOfClientInfo);
    		if (connectionSocket < 0){
      			error ("ERROR on accept");
    		} else {
      
      			pid = fork();

      			if(pid < 0){
        			error("There was a problem forking");
      			}

			// Child process just encrypts
  
      			else if(pid == 0){

        			printf("SERVER: Connected to client running at host %d port %d\n", ntohs (clientAddress.sin_addr.s_addr),
				ntohs (clientAddress.sin_port));

	      			// Get the message from the client and display it
	      			memset (buffer, '\0', 256);
       				memset(veribuffer, '\0', 256);
	      
        			// Verification it's the client
        			charsRead = send(connectionSocket, "Who are you? I am enc-server.", 39, 0);
        			if(charsRead < 0){
          				error("Could not send message to client");
        			}

        			// Get the client's verification code
        			charsRead = recv(connectionSocket, veribuffer, 255, 0);
        			if(charsRead < 0){
          				error("ERROR reading from socket");
        			}

	
        			if(strcmp(veribuffer, "I am enc-client.") != 0){
          				error("Could not trust client");
        			}

        			charsRead = send(connectionSocket, "Verification received. Send key.", 39, 0);
        			if(charsRead < 0){
          				error("Did not request key from client.");
        			}

        			charsRead = recv(connectionSocket, key, strlen(key), 0);
        			if(charsRead < 0){
          				error("Did not receive key from client.");
        			}


        			if(strlen(key) < strlen(buffer)){
          				printf("Key length: %lu\nBuffer length: %lu", strlen(key), strlen(buffer));
          				charsRead = send(connectionSocket, "Key is too short.", 100, 0);
          				if(charsRead < 0){
            					error("Client disconnected");
          				}

          				printf("Key was cryptographically too short");
					fflush(stdout);
				
        			} else {
		        		charsRead = send(connectionSocket, "Key suffices.", 100, 0);
          				if(charsRead < 0){
            					error("Client disconnected");
          				}
        		

        
        				// Get the message from the client and display it
	      				memset (buffer, '\0', 256);
        				memset(veribuffer, '\0', 256);

        				// Read the client's message from the socket 
	      				charsRead = recv (connectionSocket, buffer, strlen(buffer) -1, 0);
	      				if (charsRead < 0){
		      				error ("ERROR reading from socket");
        				}
	      				printf ("SERVER: I received this from the client: \"%s\"\n", buffer);

	      				for (size_t index = 0; index < strlen (buffer); index++){
          					buffer[index] = (buffer[index] + key[index]) % 26 + 65;

					};

        				printf ("SERVER: I have encrypted your message: \"%s\"\n", buffer);

	      				// Send a Success message back to the client 
	      				charsRead = send (connectionSocket, buffer, 39, 0);

	      				if (charsRead < 0){
		      				perror ("ERROR writing to socket");
		
	      		
						// Close the connection socket for this  client 
						close (connectionSocket);

        					// Exit child, return to parent
						exit(EXIT_FAILURE);
					}

				}
				exit(EXIT_SUCCESS);
			}
      			// parent process
    			else {

               
      				// Process child (but don't hang)
      				do{
        				wpid = waitpid(pid, &status, WNOHANG);
      				} while(!WIFEXITED(status)&!WIFSIGNALED(status) && wpid==0);
				exit(EXIT_SUCCESS);
			}
			exit(EXIT_SUCCESS);

		}
		exit(EXIT_SUCCESS);

	}

	exit(EXIT_SUCCESS);
}


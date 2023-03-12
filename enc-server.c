# include 	<stdio.h>
# include 	<stdlib.h>
# include 	<string.h>
# include 	<unistd.h>
# include 	<sys/types.h>
# include 	<sys/socket.h>
# include 	<netinet/in.h>
# include 	<wait.h>
# ifndef	MAX_MSG_LEN
# include 	"one-time/one-time.h"
# include 	"constants.h"
# include	"reading_funcs.h"
# endif
# include 	<assert.h>
# include 	<time.h>

size_t key_size = MAX_MSG_LEN + 1;

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
	char message[MAX_MSG_LEN + 1] = "HELLO";
	char result[MAX_MSG_LEN + 1] = "";
	char temp_key[MAX_MSG_LEN + 1] = "XMCKL";
	char result2[MAX_MSG_LEN + 1] = "";
	assert(encrypt_one_time_pad(message, temp_key, result) == EXIT_SUCCESS);
	assert(decrypt_one_time_pad(result, temp_key, result2) == EXIT_SUCCESS);
	fprintf(stderr, "Result: %s\nResult2: %s\n", result, result2);
	assert(strcmp(message, result2) == 0);
	char* key = {""};
  	int connectionSocket, charsRead;
  	char buffer[256];
  	char veribuffer[256];
  	struct sockaddr_in clientAddress;
	int num_threads = 0;
  	socklen_t sizeOfClientInfo = sizeof (clientAddress);


  	int status;
       	pid_t pid, wpid;

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

	
	if(num_threads > NUM_THREADS || num_threads < NUM_THREADS - 5){
		fprintf(stderr, "Waiting for threads to be in valid range\n");
		fflush(stderr);
	}
	printf("Opening socket\n");
	fflush(stdout);
 	// Blank out serverAddress
    	struct sockaddr_in serverAddress;

    	// Create the socket that will listen for connections 
    	int listenSocket = socket (AF_INET, SOCK_STREAM, 0);
    	if (listenSocket < 0){
	  	error ("ERROR opening socket\n");
	}
	// Set up the address struct for the server socket    
	if(setupAddressStruct (&serverAddress, supplied_port + num_threads) != EXIT_SUCCESS){
      		error("Could not setup address");		
    	}
	
	while (bind (listenSocket, (struct sockaddr *)&serverAddress, sizeof (serverAddress)) != 0){
      		perror("");
		sleep(1);
		fprintf(stderr, "Waiting for binding\n");
		fflush(stderr);
		continue;
	}
    	// Start listening for connections. Allow up to 5 connections to queue up
    	while(listen (listenSocket, 5) < 0){
	    	fprintf(stderr, "Waiting for socket");
		fflush(stderr);
	};
 
	while(num_threads < NUM_THREADS){
		fprintf(stderr, "Num threads: %d\n", num_threads);
		fflush(stderr);
    		if(num_threads < 0){
			fprintf(stderr, "Threads cannot be negative\n");
			fflush(stderr);
			num_threads = 0;
		}
		// Accept the connection request which creates a connection socket 
    		connectionSocket = accept (listenSocket, (struct sockaddr *) &clientAddress,&sizeOfClientInfo);
    		if (connectionSocket < 0){
			close(connectionSocket);
      			error ("ERROR on accept");
    		} else {
      
      			pid = fork();
			num_threads++;
      			if(pid < 0){
        			close(connectionSocket);
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
					close(connectionSocket);
          				error("Could not send message to client");
        			}

        			// Get the client's verification code
        			charsRead = recv(connectionSocket, veribuffer, 255, 0);
        			if(charsRead < 0){
          				
					fprintf(stderr, "\nERROR reading from socket\nWaiting for client's verification code...\n");
					while(charsRead < 0){
						fprintf(stderr, "\b");
						charsRead = recv(connectionSocket, veribuffer, 255, 0);
						fprintf(stderr, ".");
						fflush(stderr);
						fflush(stdout);
					}
					fprintf(stderr, "Found verification code!\n");
					fflush(stderr);
        			}

	
        			if(strcmp(veribuffer, "I am enc-client.") != 0){
          				close(connectionSocket);
					error("Could not trust client");
        			}

        			charsRead = send(connectionSocket, "Verification received. Send key.", 39, 0);
        			if(charsRead < 0){
					close(connectionSocket);
          				error("Did not request key from client.");
        			}

				key = read_key(key, connectionSocket);
        			
				fprintf(stderr, "Key: %s\nKey length: %jd\nBuffer length: %jd\n", key, strlen(key), strlen(buffer));
				fflush(stderr);

        			if(strlen(key) < strlen(buffer)){
          				printf("Key length: %lu\nBuffer length: %lu", strlen(key), strlen(buffer));
          				charsRead = send(connectionSocket, "Key is too short.", MAX_MSG_LEN, 0);
          				if(charsRead < 0){
						close(connectionSocket);
            					error("Client disconnected");
          				}
					fprintf(stderr, "Closing child in server");
					fflush(stderr);
					close(connectionSocket);
					exit(EXIT_FAILURE);
          				
				
        			} else {
		        		charsRead = send(connectionSocket, KEY_SUFFICES, MAX_MSG_LEN, 0);
          				if(charsRead < 0){
            					close(connectionSocket);
						error("Client disconnected");
          				}

        				// Get the message from the client and display it
	      				memset (buffer, '\0', MAX_MSG_LEN);
        				memset(veribuffer, '\0', MAX_MSG_LEN);
					strcpy(buffer, "");
					strcpy(veribuffer, "");
        				// Read the client's message from the socket 
	      				charsRead = recv (connectionSocket, buffer, MAX_MSG_LEN, 0);
	      				if (charsRead < 0){
						close(connectionSocket);
		      				error ("ERROR reading from socket");
        				}
	      				printf ("SERVER: I received this from the client: \"%s\"\n", buffer);
					strcpy(message, buffer);
					strcpy(result, "");
	      				if(encrypt_one_time_pad(message, key, result) == EXIT_FAILURE){
						fprintf(stderr, "Could not encrypt the message\n");
						close(connectionSocket);
						exit(EXIT_FAILURE);
					}

        				printf ("SERVER: I have encrypted your message: \"%s\"\n", message);

	      				// Send a Success message back to the client 
	      				charsRead = send (connectionSocket, message, MAX_MSG_LEN, 0);

	      				if (charsRead < 0){
						close(connectionSocket);
		      				perror ("ERROR writing to socket\n");
					}
					close(connectionSocket);

				}
				close(connectionSocket);
				exit(EXIT_SUCCESS);
			}
      			// parent process
    			else {

               
      				// Process child (but don't hang)
      				do{
        				wpid = waitpid(0, &status, WNOHANG);
      				} while(!WIFEXITED(status) & !WIFSIGNALED(status));
			
				if(wpid > 0){
					fprintf(stderr, "Successfully exited: %ul", wpid);
					fflush(stderr);
					close(connectionSocket);
					num_threads--;
				} else {
					fprintf(stderr, "Looping\n");
					fflush(stderr);

				}
			}
			

		}
		

	}


}


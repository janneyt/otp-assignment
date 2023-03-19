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

  // Clear out the address struct
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
	//fprintf(stderr, "Result: %s\nResult2: %s\n", result, result2);
	assert(strcmp(message, result2) == 0);
  	int connectionSocket, charsRead;
  	char buffer[MAX_MSG_LEN + 1] = {0};
  	buffer[MAX_MSG_LEN] = '\0';
  	struct sockaddr_in clientAddress;
	int num_threads = 0;
  	socklen_t sizeOfClientInfo = sizeof (clientAddress);


  	int status;
       	pid_t pid, wpid;

	if(argc < 2){
		error("You forgot the port number");
	}
  	int supplied_port = atoi(argv[1]);
  	if(supplied_port > 65535 || supplied_port <= 1025){
		error("Valid port range is 1026 to 65535 inclusive");
	}
	if(argc > 2){
		error("The enc server only takes the port number");
  	}

	
	if(num_threads > NUM_THREADS || num_threads < NUM_THREADS - 5){
		fprintf(stderr, "Waiting for threads to be in valid range\n");
		fflush(stderr);
	}
 	// Blank out serverAddress
    	struct sockaddr_in serverAddress;

    	// Create the socket that will listen for connections 
    	int listenSocket = socket (AF_INET, SOCK_STREAM, 0);
    	if (listenSocket < 0){
		close(listenSocket);
	  	error ("ERROR opening socket\n");
	}
	// Set up the address struct for the server socket    
	if(setupAddressStruct (&serverAddress, supplied_port + num_threads) != EXIT_SUCCESS){
      		close(listenSocket);
		error("Could not setup address");		
    	}
	
	if (bind (listenSocket, (struct sockaddr *)&serverAddress, sizeof (serverAddress)) != 0){
		close(listenSocket);
		error( "Binding is failing");

	}
    	// Start listening for connections. Allow up to 5 connections to queue up
    	if(listen (listenSocket, 5) < 0){
	    	close(listenSocket);
		error("Waiting for socket");
	};
 	for(;;){
		if(num_threads < NUM_THREADS){

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
      				if(pid < 0){
        				close(connectionSocket);
					error("There was a problem forking");
      				}

				// Child process just encrypts
  
      				else if(pid == 0){
					num_threads++;
        			
	      				// Get the message from the client and display it
	      				memset (buffer, '\0', MAX_MSG_LEN);
	      
        				// Verification it's the client
        				charsRead = send(connectionSocket, SERVERENCVERICODE, MAX_MSG_LEN, 0);
        				if(charsRead < 0){
						fprintf(stderr, "Sending verification request failed\n");
						fflush(stderr);
						close(connectionSocket);
          					error("Could not send message to client");
        				}

        				// Get the client's verification code
			
        				charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
        				if(charsRead < 0){
          				
						fprintf(stderr, "\nERROR reading from socket\nWaiting for client's verification code...\n");
						fflush(stderr);
						close(connectionSocket);
						exit(EXIT_FAILURE);
					}

					if(strcmp(buffer, CLIENTENCVERICODE) != 0){
						if(strcmp(buffer, CLIENTDECVERICODE) == 0){
							fprintf(stderr, "Verification failed because dec client cannot connect to enc server");
							fflush(stderr);
							close(connectionSocket);
							exit(EXIT_FAILURE);
						}
          					fprintf(stderr, "Dec client cannot connect to enc server\n");
						fflush(stderr);
						close(connectionSocket);
						exit(EXIT_FAILURE);
        				}

        				charsRead = send(connectionSocket, VERIFICATION_RECEIVED, 39, 0);
        				if(charsRead < 0){
						fprintf(stderr, "Sending verification received failed\n");
						fflush(stderr);
						close(connectionSocket);
          					error("Did not request key from client.");
        				}
			
					// Get key
					char* key = calloc(MAX_MSG_LEN + 1, sizeof(char));
					key[MAX_MSG_LEN] = '\0';
					while(strcmp((key = read_key(key, connectionSocket)), RESTART) == 0){
						//exit(EXIT_FAILURE);
						key = calloc(MAX_MSG_LEN + 1, sizeof(char));
						key[MAX_MSG_LEN] = '\0';
					};

					// Get plaintext
					char* plaintext = calloc(MAX_MSG_LEN + 1, sizeof(char));
					plaintext[MAX_MSG_LEN] = '\0';
					while(strcmp((plaintext = read_key(plaintext, connectionSocket)), RESTART) == 0){
						plaintext = calloc(MAX_MSG_LEN + 1, sizeof(char));
						plaintext[MAX_MSG_LEN] = '\0';
					}
							
					char encrypted[strlen(plaintext) + 1];
					encrypted[strlen(plaintext)] = '\0';
					memset(encrypted, '\0', strlen(plaintext));
					char temp_plaintext[MAX_MSG_LEN + 1];
			
					temp_plaintext[MAX_MSG_LEN] = '\0';
					char temp_key[MAX_MSG_LEN + 1];
					temp_key[MAX_MSG_LEN] = '\0';
					strncpy(temp_plaintext, plaintext, MAX_MSG_LEN);
					strncpy(temp_key, key, MAX_MSG_LEN);
					char result[MAX_MSG_LEN + 1] = {0};
					result[MAX_MSG_LEN] = '\0';
					char* reset_key = key;
					char* reset_plaint = plaintext;
					size_t plain_len = strlen(plaintext);
					// Perform encryption in 1024 bit chunks
					for(size_t advance = 0; advance <= plain_len; advance += MAX_MSG_LEN){
						if(encrypt_one_time_pad(temp_plaintext, temp_key, result) == EXIT_FAILURE){
							send(connectionSocket, KILL, 100, 0);
							free(plaintext);
							free(key);
							//close(connectionSocket);
							exit(EXIT_FAILURE);
						};
						// Add the result to the encrypted string
						strncat(encrypted, result, strlen(result));
						// Advance the key and plaintext pointers to continue the cycle
						key += MAX_MSG_LEN;
						plaintext += MAX_MSG_LEN;
						memset(result, '\0', MAX_MSG_LEN);

						// Put the next MAX_MSG_LEN chunk into the temp variables.
						strncpy(temp_plaintext, plaintext, MAX_MSG_LEN);
						strncpy(temp_key, key, MAX_MSG_LEN);
					}
					key = reset_key;
					plaintext = reset_plaint;
					// Return encrypted plaintext to client
					snprintf(buffer, sizeof buffer, "%zu", strlen(encrypted));
					while(strcmp(send_key(encrypted, connectionSocket, strlen(encrypted)), RESTART) == 0){
						fprintf(stderr, "Restarting plaintext\n");
						fflush(stderr);

					}
					close(connectionSocket);
					exit(EXIT_SUCCESS);
				}
			}
		}
      		// parent process
    		else {

               	
      			// Process child (but don't hang)
			do{
        			wpid = waitpid(0, &status, WNOHANG);
      			} while(!WIFEXITED(status) & !WIFSIGNALED(status));
			if(wpid > 0){
				close(connectionSocket);
				num_threads--;
			}
		}

	}


}


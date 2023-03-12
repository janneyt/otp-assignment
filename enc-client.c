# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# ifndef  key_size
# include "constants.h"
# include "reading_funcs.h"
# endif

size_t key_size = MAX_MSG_LEN + 1;

// Error function for reporting issues
void error(const char *msg) {
	perror(msg);
  	exit(EXIT_FAILURE);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
	// Clear out the address struct 
	memset((char*) address, '\0', sizeof(*address));

  	// The address should be network capable
  	address->sin_family = AF_INET;

  	// Store the port number
  	address->sin_port = htons(portNumber);

  	// Get the DNS entry for this host name
  	struct hostent* hostInfo = gethostbyname(hostname);
  	if(hostInfo == NULL){
    		error("CLIENT: ERROR, no such host\n");
  	}
  	
	// Capy the first IP address from the DNS entry to sin_addr.s_addr
  	memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	
}

int main(int argc, char *argv[]){
  	int socketFD, portNumber, charsWritten, charsRead = 0;
  	struct sockaddr_in serverAddress;
  	char buffer[MAX_MSG_LEN + 1] = "";
  	FILE* key_file;
	size_t key_size = MAX_MSG_LEN * 2;
	char* key = malloc((sizeof(char *) * (key_size + 1)));
	key[key_size+1] = '\0';
	// Check usage & args
  	if(argc < 4){
    		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
		free(key);
		exit(EXIT_FAILURE);
  	}

	// Check for a weird error where argv[3] has been reset to 0x0
	if(argv[3] == 0x0){
		free(key);
		error("Port number is 0x0");
  	}
	strcpy(key, argv[2]);
  	portNumber = atoi(argv[3]);
	if(portNumber < 1000 || portNumber > 65545){
		free(key);
		error("Invalid port number");
	}
  
  	// Create a socket 
  	socketFD = socket(AF_INET, SOCK_STREAM, 0);
  	if(socketFD < 0){
		free(key);
    		error("CLIENT: ERROR opening socket");
  	}

	// Acquire key from keyfile
  	key_file = fopen(key, "r+");
  	if(key_file == NULL){
		free(key);
    		error("Could not open key file");
  	};

	// I saved the file name in key, but now I'm using key to actually hold the key. This resets the key.
	key = calloc(key_size + 1, sizeof(char *));
	key[key_size + 1] = '\0';
	if(key == NULL){
		error("Could not reset key");
	}
	key = key_read(key, key_file);
	fprintf(stderr, "Key: %s", key);
	fflush(stderr);
	/*while(fgets(buffer, MAX_MSG_LEN, key_file) != NULL){
		buffer[MAX_MSG_LEN + 1] = '\0';
		if((strlen(key) + MAX_MSG_LEN) > (key_size / 2) ){
			fprintf(stderr, "Before realloc: %lu\n", strlen(key));
			fflush(stderr);
			// Resize array based on exponential increase
			key_size *= 2;
			printf("Key size: %lu\n", key_size);
			fflush(stdout);
			if((key = realloc(key, key_size + 1)) == NULL){
				error("Could not reallocate to fit entire key");
			};
			key[key_size + 1] = '\0';
			strcat(key, buffer);
			fprintf(stderr, "After realloc: %lu\n", strlen(key));
			fflush(stderr);
		} else {
			strcat(key, buffer);
			fprintf(stderr, "Length of key: %lu\n", strlen(key));
			fflush(stderr);
		}
	}*/
	printf("Length of key: %ld\n", strlen(key));
  	if(fclose(key_file) == EOF){
		fprintf(stdout, "Something must be wrong because closing the file failed");
		free(key);
		exit(EXIT_FAILURE);
	};
	fprintf(stdout, "Key file successfully read into client\n");  
  	// Set up the server address struct 
  	setupAddressStruct(&serverAddress, portNumber, "localhost");
  
  	// Connect to server 
  	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    		error("CLIENT: ERROR connection");
  	}
	fprintf(stdout, "Connecting to server\n");

  	// Wait for verification code from server
	strcpy(buffer, "");
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not receive verification from server");
  	}
	fprintf(stdout, "Verifying...");
  	if(strcmp(buffer, "Who are you? I am enc-server.") != 0){
    		error("Wrong server.");
  	};
	
  	charsWritten = send(socketFD, "I am enc-client.", MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not send verification back to server.");
  	};
	strcpy(buffer, "");
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Server disconnected");
  	};

	charsWritten = send(socketFD, key, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not send key back to server.");
  	};
	fprintf(stdout, "\b");
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not get confirmation.");
  	};

	if(strcmp(buffer, "Key is too short.") == 0){
		fprintf(stderr, "The key is too short. Closing client. Please try again.");
		fflush(stderr);
		free(key);
		exit(EXIT_FAILURE);
	}
  	if(strcmp(buffer, KEY_SUFFICES) != 0){
		close(socketFD);
		fprintf(stderr, "Something went wrong. Please try again.\n%s\n", buffer);
		free(key);
		exit(EXIT_FAILURE);
    
  	};
	fprintf(stdout, "Key approved, preparing encryption\n");
	fflush(stdout);
	fflush(stderr);

  	// Get input message from user
  	printf("\nClient: enter text to send to the server, and then hit enter: ");
  	// Clear out the buffer array 
  	memset(buffer, '\0', sizeof(buffer));
  	// Get input from the user, trunc to buffer - 1 chars, levaing \0
  	if(fgets(buffer, sizeof(buffer) -1, stdin) == NULL){
    		error("User input is flawed");
  	};

  	// Remove the trailing \n that fgets adds 
  	buffer[strcspn(buffer, "\n")] = '\0';
  	if(strlen(buffer) > strlen(key)){
		error("Key must be equal to or greater than the length of the text to be encrypted");
	}
	// send message to server 
  	// Write to the server 
  	charsWritten = send(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		printf("CLIENT: WARNING: Not all data written to socket!\n");
  	};

  	// Get return message from server 
  	// Clear out the buffer again for reuse
  	memset(buffer, '\0', sizeof(buffer));

  	// Read data from the socke, leaving \0 at end
  	charsRead = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsRead < 0){
    		error("CLIENT: ERROR reading from socket");
  	}
  	printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
  
  	// Close the socket
  	close(socketFD);
	fprintf(stdout, "Closing as all tasks are complete");
  	fflush(stdout);
	fflush(stderr);
  	free(key);
	exit(EXIT_SUCCESS);
}

# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# ifndef  MAX_MSG_LEN
# include "constants.h"
# endif

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
  	int socketFD, portNumber, charsWritten, charsRead;
  	struct sockaddr_in serverAddress;
  	char buffer[MAX_MSG_LEN + 1] = "";
	//  int key_int;
  	FILE* key_file;
  	char key[MAX_MSG_LEN + 1] = "";
  
	// Check usage & args
  	if(argc < 4){
    		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
		exit(EXIT_FAILURE);
  	}

	// Check for a weird error where argv[3] has been reset to 0x0
	if(argv[3] == 0x0){
		error("Port number is 0x0");
  	}
	strcpy(key, argv[2]);
  	portNumber = atoi(argv[3]);
	if(portNumber < 1000 || portNumber > 65545){
		error("Invalid port number");
	}
  
  	// Create a socket 
  	socketFD = socket(AF_INET, SOCK_STREAM, 0);
  	if(socketFD < 0){
    		error("CLIENT: ERROR opening socket");
  	}

	// Acquire key from keyfile
  	key_file = fopen(key, "r+");
  	if(key_file == NULL){
    		error("Could not open key file");
  	};
	strcpy(key, "");
	int counter = 0;
	while(fgets(key, MAX_MSG_LEN, key_file) != NULL){
		counter++;
	}
	printf("Length of key: %ld\n", strlen(key));
  	if(fclose(key_file) == EOF){
		fprintf(stdout, "Something must be wrong because closing the file failed");
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
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not receive verification from server");
  	}
	fprintf(stdout, "Verifying...");
  	if(strcmp(buffer, "Who are you? I am enc-server.") != 0){
    		error("Wrong server.");
  	};
	fprintf(stdout, "\b");
	fflush(stdout);
	sleep(1);
  	charsWritten = send(socketFD, "I am enc-client.", MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not send verification back to server.");
  	};
	fprintf(stdout, ".");
	fflush(stdout);
	sleep(1);
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Server disconnected");
  	};
	fprintf(stdout, "\b");
	fflush(stdout);

	charsWritten = send(socketFD, key, MAX_MSG_LEN, 0);
	fprintf(stdout, ".");
	fflush(stdout);
	sleep(1);
  	if(charsWritten < 0){
    		error("Could not send key back to server.");
  	};
	fprintf(stdout, "\b");
  	charsWritten = recv(socketFD, buffer, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
    		error("Could not get confirmation.");
  	};
	fprintf(stdout, ".\n");
	fflush(stdout);
	sleep(1);
	if(strcmp(buffer, "Key is too short.") == 0){
		fprintf(stderr, "The key is too short. Closing client. Please try again.");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
  	if(strcmp(buffer, "Key suffices.") != 0){
		close(socketFD);
		fprintf(stderr, "Something went wrong. Please try again.\n%s\n", buffer);
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
  	exit(EXIT_SUCCESS);
}

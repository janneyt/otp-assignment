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
    		perror("CLIENT: ERROR, no such host\n");
  	}
  	
	// Capy the first IP address from the DNS entry to sin_addr.s_addr
  	memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	
}

int main(int argc, char *argv[]){
  	int connectionSocket, portNumber, charsWritten;
  	struct sockaddr_in serverAddress;
  	char buffer[MAX_MSG_LEN + 1] = "";
  	FILE* key_file;
	FILE* plaintext_file;
	size_t key_size = MAX_MSG_LEN * 2;
	size_t plaintext_size = MAX_MSG_LEN * 2;
	char* key = malloc((sizeof(char *) * (key_size + 1)));
	char* plaintext = malloc(sizeof(char) * (plaintext_size + 1));
	*plaintext = '\0';
	key[key_size+1] = '\0';
	// Check usage & args
  	if(argc < 4){
    		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
		free(key);
		free(plaintext);
		exit(EXIT_FAILURE);
  	}

	// Check for a weird error where argv[3] has been reset to 0x0
	if(argv[3] == 0x0){
		free(key);
		free(plaintext);
		error("Port number is 0x0");
  	}
	strcpy(key, argv[2]);
  	strcpy(plaintext, argv[1]);
	portNumber = atoi(argv[3]);
	if(portNumber < 1000 || portNumber > 65545){
		free(key);
		free(plaintext);
		error("Invalid port number");
	}
  
  	// Create a socket 
  	connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
  	if(connectionSocket < 0){
		free(key);
		free(plaintext);
    		error("CLIENT: ERROR opening socket");
  	}

	// Acquire key from keyfile
  	key_file = fopen(key, "r+");
  	if(key_file == NULL){
		free(key);
		free(plaintext);
    		error("Could not open key file");
  	};

	// Acquire plaintext from plaintext file
	plaintext_file = fopen(plaintext, "r+");
	if(plaintext_file == NULL){
		free(key);
		free(plaintext);
		error("Could not open plaintext");
	}

	// I saved the file name in key, but now I'm using key to actually hold the key. This resets the key.
	key = calloc(key_size + 1, sizeof(char ));
	plaintext = calloc(plaintext_size + 1, sizeof(char));
	plaintext[plaintext_size + 1] = '\0';
	key[key_size + 1] = '\0';
	if(key == NULL){
		free(key);
		free(plaintext);
		error("Could not reset key");
	}
	if(plaintext == NULL){
		free(key);
		free(plaintext);
		error("Could not reset plaintext");
	}
	key = key_read(key, key_file);
	plaintext = key_read(plaintext, plaintext_file);
	if(strlen(key) < strlen(plaintext)){
		fprintf(stderr, "Key is shorter than plaintext, which is cryptographically insecure");
		free(key);
		free(plaintext);
		exit(EXIT_FAILURE);
	}
	printf("Length of key: %ld\n", strlen(key));
	printf("Length of plaintext: %ld\n", strlen(plaintext));
	fflush(stdout);
	plaintext_size = strlen(plaintext);
	key_size = strlen(key);
  	if(fclose(key_file) == EOF || fclose(plaintext_file) == EOF){
		fprintf(stdout, "Something must be wrong because closing the files failed");
		free(key);
		free(plaintext);
		exit(EXIT_FAILURE);
	};

	fprintf(stdout, "Key file successfully read into client\nPlaintexxt file successfully read into client\n");
	fflush(stderr);

  	// Set up the server address struct 
  	setupAddressStruct(&serverAddress, portNumber, "localhost");
  
  	// Connect to server 
  	if (connect(connectionSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    		free(key);
		free(plaintext);
		error("CLIENT: ERROR connection");
  	}
	fprintf(stdout, "Connecting to server\n");
	fflush(stdout);
  	// Wait for verification code from server
	memset(buffer, '\0', MAX_MSG_LEN);
  	charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	printf("Verification code from server: %s", buffer);
  	if(charsWritten < 0){
		free(key);
		free(plaintext);
		close(connectionSocket);
    		error("Could not receive verification from server");
  	}
	fprintf(stdout, "Verifying...\n");
	fflush(stdout);
  	if(strcmp(buffer, "Who are you? I am enc-server.") != 0){
		free(key);
		free(plaintext);
		close(connectionSocket);
    		error("Wrong server.");
  	};
	
  	charsWritten = send(connectionSocket, "I am enc-client.", MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
		free(key);
		free(plaintext);
    		error("Could not send verification back to server.");
  	};
	strcpy(buffer, "");
	while(strcmp(VERIFICATION_RECEIVED, buffer) != 0){
		charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
		if(charsWritten < 0){
			free(key);
			free(plaintext);
			error("Server disconnected");
		}

	}
	snprintf(buffer, sizeof buffer, "%zu", strlen(key));
	fprintf(stderr, "Key size atm: %jd", key_size);
	while(strcmp(send_key(key, connectionSocket, key_size), RESTART) == 0){
		fprintf(stderr, "Restarting key\n");
		fflush(stderr);
	}
	fprintf(stderr, "Sent size: %ld\n", strlen(key));
	fflush(stderr);
	fflush(stdout);
	char temp_buffer[MAX_MSG_LEN + 1];
	
	// Wait for the confirmation (the length)
	while(strcmp(temp_buffer, RECEIVED_LENGTH) != 0){
		charsWritten = recv(connectionSocket, temp_buffer, MAX_MSG_LEN, 0);
		fprintf(stderr, "Temp buffer: %s\n", temp_buffer);
		fflush(stderr);
	}

	if(strcmp(temp_buffer, buffer) != 0){
		fprintf(stderr, "The lengths don't match");
		close(connectionSocket);
		free(key);
		free(plaintext);
		exit(EXIT_SUCCESS);
	}

	while(strcmp(send_key(plaintext, connectionSocket, plaintext_size), RESTART) == 0){
		fprintf(stderr, "Restarting plaintext\n");
		fflush(stderr);
	}
	fprintf(stderr, "Sent size: %ld\n", strlen(plaintext));
	fflush(stderr);
	fflush(stdout);

	// And wait for confirmation
	while(strcmp(temp_buffer, RECEIVED_LENGTH) != 0){
		charsWritten = recv(connectionSocket, temp_buffer, strlen(RECEIVED_LENGTH), 0);
		fprintf(stderr, "Temp buffer: %s\n", temp_buffer);
		fflush(stderr);
	}
	


	// Wait for the encrypted return text
	

  	// Close the socket
  	close(connectionSocket);
	fprintf(stdout, "Closing as all tasks are complete");
  	fflush(stdout);
	fflush(stderr);
  	free(key);free(plaintext);
	exit(EXIT_SUCCESS);
}

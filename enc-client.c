# define _POSIX_C_SOURCE 200809L
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>

# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <string.h>
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
  	/**
	 * The main loop calls sets up and calls the more specialized function for sending and receiving data and encryption.
	 *
	 * argv[1] is the plaintext, argv[2] is the key, argv[3] is the port.
	 *
	 */

	int connectionSocket, portNumber, charsWritten = -1;
  	struct sockaddr_in serverAddress;
  	char buffer[MAX_MSG_LEN + 1] = "";
  	FILE* key_file = stdin;
	FILE* plaintext_file = stdin;
	size_t key_size = MAX_MSG_LEN * 2;
	size_t plaintext_size = MAX_MSG_LEN * 2;
	char* key = malloc((sizeof(char *) * (key_size + 1)));
	char keyname[MAX_MSG_LEN] = {0};
	char* plaintext = malloc(sizeof(char) * (plaintext_size + 1));
	char plaintext_name[MAX_MSG_LEN] = {0};
	*plaintext = '\0';
	key[key_size+1] = '\0';
	// Check usage & args
  	if(argc != 4){
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

	// See function stub for argv index values
	strcpy(keyname, argv[2]);
  	strcpy(plaintext_name, argv[1]);

	// Make sure the port number is valid and not a reserved number
	if((portNumber = atoi(argv[3])) == 0){
		error("Port number is not a number. Please try again.\n");
	};
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
  	key_file = fopen(keyname, "r+");
  	if(key_file == NULL){
		free(key);
		free(plaintext);
    		error("Could not open key file");
  	};

	// Acquire plaintext from plaintext file
	plaintext_file = fopen(plaintext_name, "r+");
	if(plaintext_file == NULL){
		free(key);
		free(plaintext);
		error("Could not open plaintext");
	}

	// I saved the file name in key, but now I'm using key to actually hold the key. This resets the key.
	plaintext[plaintext_size] = '\0';
	key[key_size] = '\0';

	/**
	 * Plaintext should mirror key, as they use the same function for sending and receiving. However, the key_size variable
	 * should only be set by the strlen(key) function. Key_
	 * */
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
	plaintext = key_read(plaintext, plaintext_file);
	key = key_read(key, key_file);

	if(strlen(key) < strlen(plaintext)){
		fprintf(stderr, "Key is shorter than plaintext, which is cryptographically insecure");
		free(key);
		free(plaintext);
		exit(EXIT_FAILURE);
	}

	plaintext_size = strlen(plaintext);
	key_size = strlen(key);
  	if(fclose(key_file) == EOF || fclose(plaintext_file) == EOF){
		fprintf(stdout, "Something must be wrong because closing the files failed");
		free(key);
		free(plaintext);
		exit(EXIT_FAILURE);
	};

	/** Key file successfully read into client and plaintexxt file successfully read into client
	 *  
	 *  Time to send off to server.
	*/

  	// Set up the server address struct 
  	setupAddressStruct(&serverAddress, portNumber, "localhost");
  
  	// Connect to server 
  	if (connect(connectionSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    		free(key);
		free(plaintext);
		error("CLIENT: ERROR connection");
  	}

	/**
	 * This is an elaborate identification routine that uses constants in constants.h to identify client and server and make sure we're 
	 * correctly communicating with the correct server.
	 * */
  	// Wait for verification code from server
	memset(buffer, '\0', MAX_MSG_LEN);
	charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	if(charsWritten < 0){
		free(key);
		free(plaintext);
		close(connectionSocket);
    		error("Could not receive verification from server");
  	}
	if(strcmp(buffer, SERVERVERICODE) != 0){
		free(key);
		free(plaintext);
		close(connectionSocket);
    		error("Wrong server.");
  	};
	
	// We've received the SERVERICODE, send CLIENTVERICODE
	charsWritten = send(connectionSocket, CLIENTVERICODE, MAX_MSG_LEN, 0);
  	if(charsWritten < 0){
		free(key);
		free(plaintext);
    		error("Could not send verification back to server.");
  	};
	memset(buffer, '\0', MAX_MSG_LEN);
	
	// Receive the verification received code (syn-ack)
	charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);

	if(strcmp(VERIFICATION_RECEIVED, buffer) != 0 || charsWritten < 0){
		free(key);
		free(plaintext);
		error("Server disconnected");
		

	}

	/*
	 * By this point, we know that we've connected to a valid server, so it's time to actully encrypt the plaintext with the key
	 *
	 * The functions used are the exact same for keys and plaintext, one for sending, one for receiving
	 * */
	snprintf(buffer, sizeof buffer, "%zu", strlen(key));
	char* result = RESTART;
	char* old_key = strndup(key, strlen(key));
	while(strcmp((result = send_key(key, connectionSocket, key_size)), RESTART) == 0){
		// Check that key wasn't corrupted somehow
		if(strcmp(old_key, key) != 0){
			free(key);
			free(plaintext);
			
			error("Key has been irrevocably corrupted. Exiting and try again.");
		}

		// Since we want an endless loop until RESTART is *not* returned, just continue
		continue;	
		
	}
	if(strcmp(result, SUCCESS) != 0){
		fprintf(stderr, "Didn't return SUCCESS");
		fflush(stderr);
	}

	snprintf(buffer, sizeof buffer, "%zu", strlen(plaintext));
	char* old_plaint = strndup(key, strlen(key));
	while(strcmp(send_key(plaintext, connectionSocket, strlen(plaintext)), RESTART) == 0){
		if(strcmp(plaintext, old_plaint) != 0){
			free(key);
			free(plaintext);
			error("Plaintext has been irrevocably corrupted. Exiting and try again.");
		}
		continue;
	}
	if(strcmp(result, SUCCESS) != 0){
		fprintf(stderr, "Didn't return SUCCESS with plaintext");
		fflush(stderr);
	}

	char* encrypted = calloc(MAX_MSG_LEN, sizeof(char));
	while(strcmp(read_key(encrypted, connectionSocket), RESTART) == 0){
		
		// Unlike the earlier plaintext/key functions, we don't have to store a value to send, we're receiving a value.
		// If we're restarting, just wipe the garbled gunk from the last attempt.
		encrypted = calloc(MAX_MSG_LEN, sizeof(char));
	}

	// Ironically, this is the line we're graded on. The newline at the end is for text file generation.
	fprintf(stdout, "%s\n", encrypted);
	fflush(stdout);

  	// Close the socket
  	close(connectionSocket);
 	free(key);
	free(plaintext);
	free(encrypted);
	exit(EXIT_SUCCESS);
}

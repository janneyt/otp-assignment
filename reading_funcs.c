# include <stdio.h>
# include <stdlib.h>
# ifndef  MAX_MSG_LEN
# include "constants.h"
# endif
# include <string.h> 
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>


char* key_read(char* key, FILE* stream){
	char buffer[MAX_MSG_LEN + 1];
	char* received;
	while((received = fgets(buffer, MAX_MSG_LEN, stream)) != NULL){
		buffer[strlen(received) + 1] = '\0';
		if((strlen(key) + MAX_MSG_LEN) > (key_size / 2)){
			key_size *= 2;
			if((key = realloc(key, key_size + 1)) == NULL){
				perror("Could not realloc to fit next key size");
				return NULL;
			}
			key[key_size + 1] = '\0';
			strcat(key, buffer);
		} else {
			strcat(key, buffer);
		}
	}
	return key;
}

char* read_key(char* key, int connectionSocket){
	// Reads the key over the network, not from a file
	char buffer[MAX_MSG_LEN + 1];
	int charsRead = 0;
	size_t send_size = -1;
	char* length_not_sent = LENGTH_NOT_SENT;
	charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);

	// Look for size of key (the key_size, while global, is illegal to use as server/client setup is required)
	while(charsRead < 0){
		send(connectionSocket, length_not_sent, MAX_MSG_LEN, 0);
		memset(buffer, '\0', MAX_MSG_LEN + 1);
		charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	}

	// Test to make sure we've actually grabbed a number/integer
	while(atoi(buffer) == 0){
		memset(buffer, '\0', MAX_MSG_LEN + 1);
		charsRead = send(connectionSocket, length_not_sent, MAX_MSG_LEN, 0);
		memset(buffer, '\0', MAX_MSG_LEN + 1);
		charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	}
	send_size = charsRead;
	charsRead = 0;
	memset(buffer, '\0', MAX_MSG_LEN + 1);

	// Actually receive the key
	while((charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0)) > 0){
		buffer[charsRead + 1] = '\0';
		if(strlen(key) > send_size){
			perror("Key cannot be longer than key size");
			return NULL;
		}
		strcat(key, buffer);
		memset(buffer, '\0', MAX_MSG_LEN);
	};
	return key;
}

char* read_plaintext(char* message, int connectionSocket){
	char buffer[key_size + 1];
	buffer[key_size + 1] = '\0';
	int charsRead = 0;
	char* length_not_sent = LENGTH_NOT_SENT;
	// Get length of incoming message
	charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	while(charsRead < 0){
		send(connectionSocket, length_not_sent, MAX_MSG_LEN, 0);
		charsRead = recv(connectionSocket, LENGTH_NOT_SENT, MAX_MSG_LEN, 0);
	}
	while(atoi(buffer) == 0){
		charsRead = send(connectionSocket, LENGTH_NOT_SENT, MAX_MSG_LEN, 0);
	}

	while(recv(connectionSocket, buffer, MAX_MSG_LEN, 0) > 0){
		if((strlen(message) ) > key_size){
			
			// A message of size greater than the key_size is invalid
			return NULL;
		} else {
			strcat(message, buffer);
		}
	}
	return message;
}

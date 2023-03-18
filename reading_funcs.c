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
# include <errno.h>


char* send_key(char* key, int connectionSocket, size_t send_size){
	// Sends key across the network
	char buffer[MAX_MSG_LEN + 1] = {0};
	buffer[MAX_MSG_LEN] = '\0';
	int charsWritten = -1;
	char str_size[100];
	snprintf(str_size, 100, "%zu", send_size);

	strcpy(buffer, str_size);

	charsWritten = send(connectionSocket, buffer, MAX_MSG_LEN, 0);
	if(charsWritten < 0){
		fprintf(stderr, "Restarting because no length sent due to error.\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	memset(buffer, '\0', MAX_MSG_LEN);
	charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	if(charsWritten < 0 || strcmp(buffer, RESTART) == 0){
		fprintf(stderr, "Received RESTART or error while waiting for length confirmation. charsWritten: %d. Buffer: %s\n", charsWritten, buffer);
		fflush(stderr);
		return RESTART;
	}
	if(strcmp(buffer, CONFIRM) != 0){
		fprintf(stderr, "Did not receive confirm\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	for(size_t counter = 0; counter < send_size; counter += MAX_MSG_LEN){
		
		charsWritten = send(connectionSocket, key, MAX_MSG_LEN, 0);
		if((int)strlen(key) < MAX_MSG_LEN ){

			return "SUCCESS";
		}
		if(charsWritten < MAX_MSG_LEN && (int)strlen(key) > MAX_MSG_LEN){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		memset(buffer, '\0', MAX_MSG_LEN);
		charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
		if(strcmp(buffer, RESTART) == 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		if(charsWritten < 0 || strcmp(buffer, RESTART) == 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		if(strcmp(buffer, CONFIRM) != 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		memset(buffer, '\0', MAX_MSG_LEN);
		key += MAX_MSG_LEN;
	}
	return SUCCESS;
}


char* key_read(char* key, FILE* stream){
	/**
	 * Reads any text in from a file and dynamically reallocates the size of the text
	 *
	 * Param: key, a pre-allocated string of size MAX_MSG_LEN on the heap.
	 * Param: stream, a File* stream such as stdin or an open file
	 *
	 * Returns: The key/text, with a potentially resized and null-terminated string
	 *
	 * Extern: key_size is a global variable that indicates roughly how large the input is expected to be.
	 *
	 * I'm assuming that key is malloc'd, as key MUST be a pointer returned from malloc, calloc or realloc.
	 * */
	char buffer[MAX_MSG_LEN + 1] = {0};
	buffer[MAX_MSG_LEN] = '\0';
	
	// This makes strlen(key) work on uninitialized malloc'd key
	*key = '\0';
	size_t key_length = 0;

	// Note on key_length and key_size. Key_size controls the amount of memory to be allocated and works on the same principle
	// as certain types of hashing algorithms - when you're at fifty percent capacity, double.
	// Key_length, on the other hand, is to avoid excessive strlen(key) calls. It's just the accumulated length of the key *string* plus the new string and can be up to 49% the size of key_size.
	while((fgets(buffer, MAX_MSG_LEN, stream)) != NULL){
		buffer[strlen(buffer)] = '\0';
		if((key_length + MAX_MSG_LEN) > (key_size / 2)){
			key_size *= 2;
			int old_errno = errno;
			char* temp_key = malloc(2);
			if((temp_key = realloc(key, key_size + 1)) == NULL){
				free(temp_key);
				perror("Could not realloc to fit next key size");
				return NULL;
			}
			key = temp_key;
			if(old_errno != errno){
				perror("Realloc failed");
				return NULL;
			}
			key[key_length] = '\0';
		} 
		strcat(key, buffer);
		key_length += strlen(buffer);
	}
	return key;
}

char* read_key(char* key, int connectionSocket){
	/**
	 * Receives (reads) key or any text over a connected socket.
	 *
	 * A few assumptions. The socket must be already connected as no verification is managed. The key must be malloc'd.
	 * There's some verification (have you received my length? Does it match?) but the server/client relationship is not verified.
	 *
	 * Params: key is a malloc'd string.
	 * Params: connectionSocket is the file descriptor returned by the socket/listen/accept socket family.
	 *
	 * Returns: RESTART is an unrecoverable error has happened, SUCCESS if key read, potentially corrupted materials as there is only
	 * limited validation of input vis-a-vis what was actually sent.
	 *
	 * Note that freeing, like mallocing, is handled by the calling function. It's extremely important to understand that free must be called 
	 * on key somewhere else.
	 * */
	char buffer[MAX_MSG_LEN + 1] = {0};
	buffer[MAX_MSG_LEN] = '\0';
	int charsRead = -1;
	size_t send_size = -1;
	
	// Have I received the length?
	charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);

	// Did I receive anything?
	while(charsRead < 0){
		fprintf(stderr, "No, I received nothing, restart everything because the socket errored\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}

	// Test to make sure we've actually grabbed a number/integer
	if((send_size = atoi(buffer)) == 0){
		
		fprintf(stderr, "No, I received something that isn't an integer, the network is not synchronized. Buffer: %s\n", buffer);
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}

	// Did we receive the restart signal?
	if(strcmp(buffer, RESTART) == 0){
		fprintf(stderr, "Restart signal received\n");
		fflush(stderr);
		return RESTART;
	}

	// Send length as a confirmation
	charsRead = send(connectionSocket, CONFIRM, MAX_MSG_LEN, 0);
	if(charsRead < 0){
		fprintf(stderr, "Error while sending\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	
	int old_errno = errno;

	// We now know the size of the total message, so key can be reallocated to it's maximum length
	// The only reliable way to see if realloc has failed is to check errno
	key = realloc(key, send_size + 1);
	if(old_errno != errno){
		fprintf(stderr, "Realloc problem");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}

	// Reset everything before heading into the actual key processing
	charsRead = 0;
	memset(buffer, '\0', MAX_MSG_LEN + 1);

	// Actually receive the key
	size_t counter = 0;
	
	while(counter < send_size ){
		// We need a positive number of bytes, as a zero byte message makes no sense in this context
		if((charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0)) < 0){
			fprintf(stderr, "Restarting before the first MAX_MSG_LEN segment is received. charsRead: %d\n", charsRead);
			fflush(stderr);
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		};

		// Did I receive restart? Then blank out key and start over because send function failed.		
		if(strcmp(buffer, RESTART) == 0){
			return RESTART;
		}

		if(strlen(buffer) < MAX_MSG_LEN){
			if(counter + strlen(buffer) >= send_size){
				strcat(key, buffer);
				return key;
			}
			fprintf(stderr, "Decided to RESTART\n");
			fflush(stderr);
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
	
		buffer[charsRead] = '\0';

		// If we've overflowed the send_size, something's wrong. Restart everything.
		if(strlen(key) + strlen(buffer) > send_size){

			fprintf(stderr, "I'm leery of this one");
			fflush(stderr);
			charsRead = send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		} 

		// If we're not receiving a full message but we're also not at the end of the message as measured by send_size, restart
		if(charsRead < MAX_MSG_LEN && strlen(key) < send_size - MAX_MSG_LEN){
			fprintf(stderr, "This should be fine\n");
			fflush(stderr);
			charsRead = send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}

	
		// charsRead has to be an int, counter a size_t, so cast them to the same type for the addition.
		counter += strlen(buffer);

		// This is a running null terminator since we can't guaratee the network sends it.
		key[counter] = '\0';
		strcat(key, buffer);
		//fprintf(stderr, "Characters read: %d\nStrlen of buffer: %lu\nStrlen of key: %lu\n", charsRead, strlen(buffer), strlen(key));
		//fflush(stderr);
		charsRead = send(connectionSocket, CONFIRM, MAX_MSG_LEN, 0);
		memset(buffer, '\0', MAX_MSG_LEN);
	};
	return key;
}



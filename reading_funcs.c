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
	char buffer[MAX_MSG_LEN + 1];
	buffer[MAX_MSG_LEN + 1] = '\0';
	int charsWritten = -1;
	char str_size[100];
	snprintf(str_size, 100, "%zu", send_size);

	strcpy(buffer, str_size);
	//printf("send_size: %lu\n", send_size);
	//fflush(stdout);
	// Send length
	//fprintf(stderr, "Sending length: %s\n", buffer);
	fprintf(stderr, "Sending length: %s\n", buffer);
	fflush(stderr);
	charsWritten = send(connectionSocket, buffer, MAX_MSG_LEN, 0);
	if(charsWritten < 0){
		fprintf(stderr, "Restarting because no length sent due to error.\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	fprintf(stderr, "Waiting for Confirmation\n");
	fflush(stderr);
	memset(buffer, '\0', MAX_MSG_LEN);
	charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	if(charsWritten < 0 || strcmp(buffer, RESTART) == 0){
		fprintf(stderr, "Received RESTART or error while waiting for length confirmation. charsWritten: %d. Buffer: %s\n", charsWritten, buffer);
		fflush(stderr);
		return RESTART;
	}
	fprintf(stderr, "Confirm: %s\n", buffer);
	fflush(stderr);
	if(strcmp(buffer, CONFIRM) != 0){
		fprintf(stderr, "Did not receive confirm\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	fprintf(stderr, "Received confirmation, sending over key\n");
	fflush(stderr);
	for(size_t counter = 0; counter < send_size; counter += MAX_MSG_LEN){
		
		charsWritten = send(connectionSocket, key, MAX_MSG_LEN, 0);
		fprintf(stderr, "3 - charsWritten: %d strlen(key): %lu\n", charsWritten, strlen(key));
		fflush(stderr);
		if((int)strlen(key) < MAX_MSG_LEN ){
			fprintf(stderr, "Not sure what to do, this is the problem, yeah? key: %s\n", key);
			fflush(stderr);
			return "SUCCESS";
		}
		if(charsWritten < MAX_MSG_LEN && (int)strlen(key) > MAX_MSG_LEN){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		fprintf(stderr, "4\n");
		fflush(stderr);
		memset(buffer, '\0', MAX_MSG_LEN);
		charsWritten = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
		if(strcmp(buffer, RESTART) == 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		fprintf(stderr, "5\n");
		fflush(stderr);
		if(charsWritten < 0 || strcmp(buffer, RESTART) == 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		if(strcmp(buffer, CONFIRM) != 0){
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
		fprintf(stderr, "Before advance");
		fflush(stderr);
		memset(buffer, '\0', MAX_MSG_LEN);
		key += MAX_MSG_LEN;
	}
	return "SUCCESS";
}


char* key_read(char* key, FILE* stream){
	// Reads key in from a file
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
//	fprintf(stderr, "Inside key_read key_size: %jd", key_size);
//	fflush(stderr);
	return key;
}

char* read_key(char* key, int connectionSocket){
	// Reads the key over the network, not from a file
	// Returns "-1" for failure
	char buffer[MAX_MSG_LEN + 1];
	buffer[MAX_MSG_LEN + 1] = '\0';
	int charsRead = -1;
	size_t send_size = -1;
	fprintf(stderr, "Have I received the length?\n");
	fflush(stderr);
	//charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);
	// Have I received the length?
	charsRead = recv(connectionSocket, buffer, MAX_MSG_LEN, 0);

	// Did I receive anything?
	while(charsRead < 0){
		fprintf(stderr, "No, I received nothing, restart everything because the socket errored\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}

	fprintf(stderr, "Atoi buffer before: %s. charsRead: %d\n", buffer, charsRead);
	fflush(stderr);
	// Test to make sure we've actually grabbed a number/integer
	if((send_size = atoi(buffer)) == 0){
		
		fprintf(stderr, "No, I received something that isn't an integer, the network is not synchronized. Buffer: %s\n", buffer);
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	fprintf(stderr, "Atoi after\n");
	fflush(stderr);

	// Did we receive the restart signal?
	if(strcmp(buffer, RESTART) == 0){
		fprintf(stderr, "Restart signal received\n");
		fflush(stderr);
		return RESTART;
	}
	fprintf(stderr, "Sending CONFIRM");
	fflush(stderr);
	// Send length as a confirmation
	charsRead = send(connectionSocket, CONFIRM, MAX_MSG_LEN, 0);
	if(charsRead < 0){
		fprintf(stderr, "Error while sending\n");
		fflush(stderr);
		send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
		return RESTART;
	}
	fprintf(stderr, "Looking for key of size: %ld\n", send_size);
	fflush(stderr);
	int old_errno = errno;

	// We now know the size of the total message, so key can be reallocated to it's maximum length
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
	fprintf(stderr, "Heading into key loop\n");
	fflush(stderr);
	// Actually receive the key
	size_t counter = 0;
	
	while(counter < send_size ){
		fprintf(stderr, "Counter: %lu\n", counter);
		fflush(stderr);
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
			fprintf(stderr, "Buffer is too short. Buffer: %s\nLength of buffer + counter: %lu", buffer, strlen(buffer) + counter);
			fflush(stderr);
			if(counter + strlen(buffer) >= send_size){
				strcat(key, buffer);
				return key;
			}
			
			send(connectionSocket, RESTART, MAX_MSG_LEN, 0);
			return RESTART;
		}
	
		fprintf(stderr, "Inside loop\n");
		fflush(stderr);
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

		fprintf(stderr, "Advancing counter\n");
		fflush(stderr);
		
		// charsRead has to be an int, counter a size_t, so cast them to the same type for the addition.
		counter += strlen(buffer);
		printf("Counter: %ld\n", counter);
		fflush(stdout);

		// This is a running null terminator since we can't guaratee the network sends it.
		key[counter + 1] = '\0';
		strcat(key, buffer);
		fprintf(stderr, "Characters read: %d\nStrlen of buffer: %lu\nStrlen of key: %lu\n", charsRead, strlen(buffer), strlen(key));
		fflush(stderr);
		charsRead = send(connectionSocket, CONFIRM, MAX_MSG_LEN, 0);
		fprintf(stderr, "It's the strcat innit?\n");
		fflush(stderr);
		memset(buffer, '\0', MAX_MSG_LEN);
	};
	printf("Key length as we leave read_key: %lu\n", strlen(key));
	fflush(stdout);
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

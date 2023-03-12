#include <stdio.h>
#include <stdlib.h>
#include <string.h>
# include <assert.h>
# include <unistd.h>

# ifndef  key_size
# include "../constants.h"
# endif

int encrypt_one_time_pad(char* message, char* key, char* result)
{
    char cipher[MAX_MSG_LEN + 1] = { 0 }; 
    printf("Inside encryption routine: %s\n", message);
    fflush(stdout);
    /* Remove newline character from the end of the message */
    if (message[strlen(message) - 1] == '\n') {
        message[strlen(message) - 1] = '\0';
    }


    /* Remove newline character from the end of the key */
    if (key[strlen(key) - 1] == '\n') {
        key[strlen(key) - 1] = '\0';
    }

    /* Check that the key is at least as long as the message */
    if (strlen(key) < strlen(message)) {
        fprintf(stderr, "Error: Key is too short for the message.\n");
        return EXIT_FAILURE;
    }

    /* Encrypt the message using one-time pad */
    for (size_t i = 0; i < strlen(message); i++) {
        cipher[i] = (message[i] + key[i]) % 26 + 'A';
    }

    /* Print the encrypted message to standard output */
    printf("Cipher: %s\n", cipher);
    fflush(stdout);
    strcpy(result, cipher);
    return EXIT_SUCCESS;
}

int decrypt_one_time_pad(char message[MAX_MSG_LEN + 1], char key[MAX_MSG_LEN + 1], char result[MAX_MSG_LEN + 1])
{
    char cipher[MAX_MSG_LEN + 1] = { 0 }; 

    /* Remove newline character from the end of the message */
    if (message[strlen(message) - 1] == '\n') {
        message[strlen(message) - 1] = '\0';
    }


    /* Remove newline character from the end of the key */
    if (key[strlen(key) - 1] == '\n') {
        key[strlen(key) - 1] = '\0';
    }

    /* Check that the key is at least as long as the message */
    if (strlen(key) < strlen(message)) {
        fprintf(stderr, "Error: Key is too short for the message.\n");
        return EXIT_FAILURE;
    }

    /* Encrypt the message using one-time pad */
    for (size_t i = 0; i < strlen(message); i++) {
        cipher[i] = message[i] - key[i];
	if(cipher[i] < 0){
		cipher[i] = cipher[i] + 26;
	}
	cipher[i] = cipher[i] % 26 + 'A';
    }

    /* Print the encrypted message to standard output */
    printf("Message: %s\n", cipher);
    fflush(stdout);
    strcpy(result, cipher);
    return EXIT_SUCCESS;
}


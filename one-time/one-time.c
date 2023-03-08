#include <stdio.h>
#include <stdlib.h>
#include <string.h>
# include <assert.h>
# include <unistd.h>

#define MAX_MSG_LEN 1024

int encrypt_one_time_pad(char message[MAX_MSG_LEN + 1], char key[MAX_MSG_LEN + 1])
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
        cipher[i] = ((message[i] + key[i]) % 26)+65;
    }

    /* Print the encrypted message to standard output */
    printf("%s\n", cipher);
    fflush(stdout);
    return EXIT_SUCCESS;
}


int main(void)
{
    /* Test 1: Encrypt a short message with a short key */
    char msg1[MAX_MSG_LEN + 1] = "HELLO";
    char key1[MAX_MSG_LEN + 1] = "XMCKL";
    char* expected1 = "EQNVZ";
    assert(encrypt_one_time_pad(msg1, key1) == EXIT_SUCCESS);
    char result1[MAX_MSG_LEN + 1] = { 0 };
    assert(strcmp(result1, expected1) == 0);

    /* Test 2: Encrypt a long message with a long key */
    /*char msg2[MAX_MSG_LEN + 1] = "The quick brown fox jumps over the lazy dog.";
    char key2[MAX_MSG_LEN + 1] = "zOw7Vq3KtBpShTf0Gd1rjIxiLyX9ubE8lmJc65nMFaD2sCvN4geUAHYPQ";
    char* expected2 = "QRa8]lWfozggLy4i`U}hTE`8A@W<)v77twEg8JTs>X+U*iw6%_F";
    assert(encrypt_one_time_pad(msg2, key2) == EXIT_SUCCESS);
    char result2[MAX_MSG_LEN + 1] = { 0 };
    assert(strcmp(result2, expected2) == 0);

    printf("All tests passed.\n");*/

    return EXIT_SUCCESS;
}

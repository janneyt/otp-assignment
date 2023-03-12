# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# ifndef  key_size
# include "../constants.h"
# endif

int encrypt_one_time_pad(char message[MAX_MSG_LEN], char key[MAX_MSG_LEN], char result[MAX_MSG_LEN]);
int decrypt_one_time_pad(char message[MAX_MSG_LEN], char key[MAX_MSG_LEN], char result[MAX_MSG_LEN]);

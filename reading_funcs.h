# include <stdio.h>
# include <stdlib.h>
# ifndef  MAX_MSG_LEN
# include "constants.h"
# endif

char* send_key(char* key, int connectionSocket, size_t send_size);
char* key_read(char* message, FILE* stream);
char* read_key(char* key, int connectionSocket);

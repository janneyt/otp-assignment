# include <stdio.h>
# include <stdlib.h>
# ifndef  MAX_MSG_LEN
# include "constants.h"
# endif

char* key_read(char* message, FILE* stream);
char* read_key(char* message, int connectionSocket);

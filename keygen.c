# include <stdlib.h>
# include <stdio.h>
# include <time.h>
# include <string.h>

char *allowed_characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

int main(int argc, char *argv[]){
  time_t time1;

  if(argc == 2){
    // Initialize random number generator
    srand((unsigned) time(&time1));

    for(int index = 0; index < atoi(*(argv+1)); index++){

      	printf("%c", allowed_characters[rand() / (RAND_MAX / 27 +1)]);
    };
    printf("\n");
    
  } else {
    printf("Too few arguments");
    exit(EXIT_FAILURE);
  };
}

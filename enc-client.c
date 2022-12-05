# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>

// Error function for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(0);
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
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(0);
  }
  // COpy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

};

int main(int argc, char *argv[]){
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[256];
  int key_int;
  FILE* key_file;
  char key[2048];
  // Check usage & args
  if(argc< 3){
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
    exit(0);
  }


  
  // Create a socket 
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if(socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

  
  // Set up the server address struct 
  setupAddressStruct(&serverAddress, atoi(argv[2]), argv[1]);
  
  // Connect to server 
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connection");
  }

  // Wait for verification code from server
  charsWritten = recv(socketFD, buffer, 39, 0);
  if(charsWritten < 0){
    error("Could not receive verification from server");
  }

  if(strcmp(buffer, "Who are you? I am enc-server.") != 0){
    error("Wrong server.");
  };

  charsWritten = send(socketFD, "I am enc-client.", 39, 0);
  if(charsWritten < 0){
    error("Could not send verification back to server.");
  };

  charsWritten = recv(socketFD, buffer, 100, 0);
  if(charsWritten < 0){
    error("Server disconnected");
  };

  // Acquire key from keyfile
  key_file = fopen("keyfile", "r");
  if(key_file == NULL || fgets(key, strlen(key), key_file) == NULL){
    error("Could not open key file");
  };
  fclose(key_file);
  charsWritten = send(socketFD, key, strlen(key), 0);

  if(charsWritten < 0){
    error("Could not sendkey back to server.");
  };

  charsWritten = recv(socketFD, buffer, 255, 0);
  if(charsWritten < 0){
    error("Could not get confirmation.");
  };

  if(strcmp(buffer, "Key suffices.") != 0){
    printf("Something went wrong. Please try again.\n");
    
  };

  // Get input message from user
  printf("Client: enter text to send to the server, and then hit enter: ");
  // Clear out the buffer array 
  memset(buffer, '\0', sizeof(buffer));
  // Get input from the user, trunc to buffer - 1 chars, levaing \0
  if(fgets(buffer, sizeof(buffer) -1, stdin) == NULL){
    error("User input is flawed");
  };
  // Remove the trailing \n that fgets adds 
  buffer[strcspn(buffer, "\n")] = '\0';
  // send message to server 
  // Write to the server 
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);
  if(charsWritten < 0){
    printf("CLIENT: WARNING: Not all data written to socket!\n");
  };

  // Get return message from server 
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));

  // Read data from the socke, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) -1, 0);
  if(charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
  
  // Close the socket
  close(socketFD);
  
  return 0;
}

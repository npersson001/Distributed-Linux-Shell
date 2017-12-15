

// client program 
/*
	this client program takes input from the command line and sends it to 
	a server program that it has connected to using sockets.  the output of
	the executed program is returned to the client from the server.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "Socket.h"
#include "ServerClient.h"

int main(int argc, char **argv){
	//some initialized variables
	char *host;
	char *port;
	int sizeInput = 0;
	int i, charInput, charReturn;
	Socket socket;
	char line[MAX_LINE];
	int count = 0;

	// check correct arguments were passed in
	if(argc < 3) {
		fprintf(stderr, "no host and port specified\n");
		return(-1);
	}

	// get arguments
	host = argv[1];
	port = argv[2];  // note still array of char

	// connect to welcoming socket of server
	socket = Socket_new(host, atoi(port));
	if(socket < 0){
		fprintf(stderr, "failed to connect to server\n");
		return(-1);
	}

	//print shell prompt for output
	printf("%% ");

	// start loop for input
	while ((fgets(line, sizeof(line), stdin) != NULL)){
		// get size of input line
		sizeInput = strlen(line) + 1;  // note +1 includes \0

		//determine if fgets truncated user input before inserting into line
		if(line[sizeInput - 2] == '\n'){
			// send input line though socket to server
			for (i = 0; i < sizeInput; i++){
				// get char and send through socket, char is cast as int
	    	  	charInput = line[i];
	          	charReturn = Socket_putc(charInput, socket);

	          	// check returned value, if EOF exit
	          	if (charReturn == EOF){
		      		fprintf(stderr, "Socket_putc EOF or error\n");             
	              	Socket_close(socket);
	              	exit (-1);  // fatal socket problem
	            }
	        }

	        // get response from server 
	        while(count < 2){
	       		for (i = 0; i < MAX_LINE; i++){
		        	// get char from server
		        	charInput = Socket_getc(socket);

		        	// check returned value, if EOF exit
		           	if (charInput == EOF){
		           		fprintf(stderr, "Socket_putc EOF or error\n");             
		              	Socket_close(socket);
		              	exit (-1);  // fatal socket problem
		           	} 
		           	else {
		           		line[i] = charInput;
		               	if (line[i] == '\0'){
		               		count++;
		               		break;
		               	}
		               	else if(line[i] == '\n'){
		               		line[i+1] = '\0';
		               		break;
		               	}
		           	}
		        }

		        if(line[0] == '*' && line[1] == '*' && line[2] == '*'
		        		&& line[3] == 'p' && line[4] == 'i' && line[5] == 'd'){
		        	// dont print out
		        }
		        else{
		        	// print to stdout
		       		printf("%s", line);
		        }
	        }

	        // reset count
	        count = 0;
		}
		// if truncation occured, print message and skip rest of line
		else{
			char c;
			while((c = getchar()) != '\n' && c != EOF){};
			fprintf(stderr, "user input exceeded buffer size of %i (including \\n and \\0)\n", MAX_LINE);
		}

       	// print shell prompt for output
		printf("%% ");
	}

	// no newline after EOF so print one out
	printf("\n");

	// close the socket
	Socket_close(socket);
  	exit(0);
}

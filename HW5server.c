

// server program 
/*
	this server program provides a welcome listending socket and a connection socket
	the server takes a line from the client, forks a child, and the child
	executes the progaram specified by the client.  the results are then sent
	to the client.  the server exits when the client sends an EOF.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "Socket.h"
#include "ServerClient.h"

#define MAX_TMP  100

// global socket descriptors
ServerSocket welcome_socket;
Socket connect_socket;
unsigned char tmp_name[MAX_TMP];
int randomNumber;

// exit server on EOF read error
void exitEOF(){
	printf("Socket_putc EOF or error\n");             
  	Socket_close(connect_socket);
  	Socket_close(welcome_socket);
  	remove(tmp_name);
  	exit (-1);  // fatal socket problem
}

// send an error line to the client over connect_socket
void sendError(char* line){
	int charReturn, size, i;

	// send status line to client
	size = strlen(line);  // dont null terminate cause we dont want the client to get multiple \0
	for(i = 0; i < size; i++){
		charReturn = Socket_putc(line[i], connect_socket);

		// check returned value, if EOF exit
      	if (charReturn == EOF){
      		exitEOF();
        }
	}

	// send sentinel 
	charReturn = Socket_putc(EOF, connect_socket);

	// check returned value, if EOF exit
  	if (charReturn == EOF){
  		exitEOF();
    }
}

// send a line to the client over the connect_socket
void sendLine(char* line){
	int charReturn, size, i;

    // send sentinel 
	charReturn = Socket_putc('\0', connect_socket);

	// check returned value, if EOF exit
  	if (charReturn == EOF){
  		exitEOF();
    }

	// send status line to client
	size = strlen(line);  // dont null terminate cause we dont want the client to get multiple \0
	for(i = 0; i < size; i++){
		charReturn = Socket_putc(line[i], connect_socket);

		// check returned value, if EOF exit
      	if (charReturn == EOF){
      		exitEOF();
        }
	}

    // send sentinel 
	charReturn = Socket_putc('\0', connect_socket);

	// check returned value, if EOF exit
  	if (charReturn == EOF){
  		exitEOF();
    }
}

//function to parse the input line into an array of arrays
void parse(char *input, char **output){
	//loop until the null terminator is reached
	while(*input != '\0'){
		//loop while white space is encountered and replace it with null terminator
		while(*input == '\n' || *input == '\t' || *input == ' '){
			*input++ = '\0';	
		}
		//if the read char is not null terminator then put input line into array
		if(*input != '\0'){
			*output++ = input;
			//increment until the next whitespace or terminator is reached
			while(*input != '\0' && *input != '\n' && *input != '\t' && *input != ' '){
				input++;
			}	
		}
	}
	//terminate the array of arrays
	*output = NULL;
}

// function to service the client by executing the passed command
void service(char* line){
	// some initialized variables
	char *parsedLine[MAX_LINE];
	int exec_status;

   	// parse input line to get command
   	parse(line, parsedLine);

   	//execute the given program from the child process
	exec_status = execvp(parsedLine[0], parsedLine);
	if(exec_status < 0){
		printf("%d\n", randomNumber);
		printf("error occured while calling execvp()\n");
		exit(1);
	}
}

void takeInput(char* line){
	// initialize some variables
	int i; 
	char charInput;

	// get the line from the client
	for (i = 0; i < MAX_LINE; i++){
    	// get char from server
    	charInput = Socket_getc(connect_socket);

    	// check returned value, if EOF exit
       	if (charInput == EOF){
       		exitEOF();
       	} 
       	else {
       		line[i] = charInput;
           	if (line[i] == '\0'){
           		break;
           	}
       	}
    }

   	// terminate line for certain
   	if (i == MAX_LINE){
   		line[i-1] = '\0';
   	}
}

int main(int argc, char **argv){
	// some initialized variables
	int child_status, exec_status, i, count, id, size, charReturn;
	pid_t cpid, term_pid;
	char *port;
	char line[MAX_LINE];
	char new_line[MAX_LINE];
	FILE *fp;
	FILE *tmpFP;
	unsigned char id_str[MAX_TMP];
	unsigned char tmp_name[MAX_TMP];

	// check correct arguments were passed in
	if (argc < 2){
      	fprintf(stderr, "no port specified\n");
      	return (-1);
    }

    // get arguments
    port = argv[1];

    // create welcoming socket
    welcome_socket = ServerSocket_new(atoi(port));
  	if (welcome_socket < 0){
      	fprintf(stderr, "failed to create welcoming socket\n");
      	return (-1);
    }

    // accept incomming connection
	connect_socket = ServerSocket_accept(welcome_socket);
	if (connect_socket < 0){
        fprintf(stderr, "failed accept on server socket\n");
        Socket_close(welcome_socket);
        exit (-1);
    }

    // file name for output
    id = (int) getpid();
    sprintf(id_str, "%d", id);
    strcpy(tmp_name, "tmp");
    strcat(tmp_name, id_str);

    // create random number for child error checking
    randomNumber = rand();

    // loop to take input
    while(1){

    	// take the input
    	takeInput(line);

	    // fork parent process so child can do the work
	    cpid = fork();
	    if (cpid == -1){
	        sprintf(line, "failed forking child process\n");
	        sendError(line);
	        exit (-1);
	    }
	    if (cpid == 0){ 
	    	Socket_close(welcome_socket);
	    	fp = freopen(tmp_name, "w", stdout);
	      	service(line);
	        Socket_close(connect_socket);
	        fclose(fp);
	        exit (0);
	    }
	    else{
	    	// wait for child to terminate 
	        term_pid = waitpid(-1, &child_status, 0);

	        // check child pid and print pid and status
	        if (term_pid == -1){
	        	sprintf(line, "failed with return pid for waitpid\n"); 
	        }
	        else{
	        	if (WIFEXITED(child_status)) {
	        		sprintf(line, "***pid %d exited, status = %d\n", cpid, WEXITSTATUS(child_status));
	        	}
          		else{
	     			sprintf(line, "failed pid %d did not exit normally\n", cpid);
	     			// should i still print out the contents of tmp??? ****************************************
          		}
	        }

	        // open the temp file with output
	        if ((tmpFP = fopen (tmp_name, "r")) == NULL){
	        	sprintf (line, "failed opening tmp file\n");
	        	sendError(line);
	        	Socket_close(connect_socket);
		        Socket_close(welcome_socket);
		        remove(tmp_name);
	  			exit (-1);
	        }
		
			// get lines from file and send them to the client
			while(1){
				if (fgets (new_line, sizeof(new_line), tmpFP) == NULL){
            		break;
				}

				// dont send tmp file contents if error occured
				if (randomNumber == atoi(new_line)){
					sprintf(line, "failed to execvp\n");
					break;
				}

				size = strlen(new_line);  
				for(i = 0; i < size; i++){
					charReturn = Socket_putc(new_line[i], connect_socket);

					// check returned value, if EOF exit
		          	if (charReturn == EOF){
			      		exitEOF();
		            }
				}
			}

			// send status line to client
			sendLine(line);

			// remove the temp file
			remove(tmp_name);
	    }
    }

    // close all sockets remove file - note should never be reached
    Socket_close(connect_socket);
    Socket_close(welcome_socket);
    remove(tmp_name);
}

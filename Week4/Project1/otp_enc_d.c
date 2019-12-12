/******************************************************************************
* Author:		Edmund Dea (deae@oregonstate.edu)
* Student ID:	933280343
* Date:			8/10/2019
* Title:		CS344 Program 4: OTP - OTP Encoder Daemon
* Description:	This program will run in the background as a daemon. Upon
*				execution, otp_enc_d must output an error if it cannot be run
*				due to a network error, such as the ports being unavailable.
*				Its function is to perform the actual encoding, as described
*				above in the Wikipedia quote. This program will listen on a
*				particular port/socket, assigned when it is first ran. When a
*				connection is made, otp_enc_d must call accept() to generate
*				the socket used for actual communication, and then use a
*				separate process to handle the rest of the transaction, which
*				will occur on the newly accepted socket.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define DEBUG						0

/* Global macros */
#define MAX_BUFFER_SIZE				4096
#define MAX_SOCKET_CONNECTIONS		5
#define INCOMPLETE					0
#define COMPLETE					1
#define NUM_CIPHERTEXT_CHAR			27

/* Exit codes */
#define ERROR_NONE					0
#define ERROR_GENERIC				1
#define ERROR_SHORT_KEY				2

/******************************************************************************
* Name:			error
* Arguments:	string
* Return:		void
* Description:	Error function used for reporting issues
******************************************************************************/
void error(const char *msg)
{
	perror(msg);
	exit(ERROR_GENERIC);
}

/******************************************************************************
* Name:			strlen2
* Arguments:	input string
* Return:		String length
* Description:	Returns number of characters in the input string
******************************************************************************/
int strlen2(char* input)
{
	int i, len = 0;

	for (i = 0; input[i] != '\0'; i++)
		len++;

	return len;
}

/******************************************************************************
* Name:			getCiphertextIndex
* Arguments:	char
* Return:		index to ciphertext array
* Description:	Takes a plaintext/key char and returns the index corresponding
*				with that char in the ciphertext array.
******************************************************************************/
int getCiphertextIndex(char *cipherTable, char letter)
{
	int i;
	
	/* Find the index corresponding to the letter in cipherTable */
	for (i = 0; i < NUM_CIPHERTEXT_CHAR; i++)
	{
		if ((int)letter == cipherTable[i])
			return i;
	}
	
	return -1;
}

/******************************************************************************
* Name:			sendData
* Arguments:	socket file descriptor, data
* Return:		Exit Code
* Description:	Entry point for otp_enc
******************************************************************************/
int sendData(int socketFD, char *data)
{
	int charsWritten;
	int i, status, maxRetries = 10;
	char buffer[MAX_BUFFER_SIZE];

	/* Clear buffer */
	memset(buffer, '\0', sizeof(buffer));

	/* Keep trying to send data until success up to maxRetries times */
	for (i = 0; i < maxRetries; i++)
	{
		status = INCOMPLETE;

		/* Send message to server. Write to the server. */
		charsWritten = send(socketFD, data, strlen2(data), 0);
		
		if (charsWritten < 0)
		{
			error("SERVER: ERROR writing to socket");
			status = INCOMPLETE;
		}
		else if (charsWritten < strlen2(buffer))
		{
			printf("SERVER: WARNING: Not all data written to socket!\n");
			status = INCOMPLETE;
		}
		else
		{
#if DEBUG
			printf("[SERVER] Sent \"%s\"\n", data);
#endif
			status = COMPLETE;
			break;
		}
	}
	
	return status;
}

/******************************************************************************
* Name:			sendReceive
* Arguments:	[in] socket file descriptor
*				[out] buffer
* Return:		charsRead
* Description:	Receives data from the server
******************************************************************************/
int receiveData(int socketFD, char *buffer, int bufferSize)
{
	int charsRead, i, status, maxRetries = 10;
	
	for (i = 0; i < maxRetries; i++)
	{
		status = INCOMPLETE;
		
		/* Clear buffer */
		memset(buffer, '\0', bufferSize);
		
		/* Read data from the socket, leaving \0 at end */
		charsRead = recv(socketFD, buffer, bufferSize - 1, 0);
		
		if (charsRead < 0)
		{
			status = INCOMPLETE;
			error("SERVER: ERROR reading from socket");
		}
		else
		{
#if DEBUG
			printf("[SERVER] Received \"%s\"\n", buffer);
#endif
			status = COMPLETE;
			break;
		}
	}
	
	return status;
}

/******************************************************************************
* Name:			encryptPlaintext
* Arguments:	plaintext, key
* Return:		ciphertext
* Description:	Encrypts plaintext using key through modular addition and
*				returns ciphertext.
*				http://en.wikipedia.org/wiki/One-time_pad
******************************************************************************/
char* encryptPlaintext(char *cipherTable, char *plaintext, char *key)
{
	int i, iCiphertext;
	int len_key = strlen2(key);
	int len_plaintext = strlen2(plaintext);
	char *ciphertext = malloc(sizeof(char) * len_plaintext);
	int letter_plntxt, letter_key;

#if DEBUG
	printf("[SERVER] plaintext = \"%s\"\n"
		"[SERVER] key = \"%s\"\n"
		"len_key=%d\n"
		"len_plntxt=%d\n", 
		plaintext, key, len_key, len_plaintext);
#endif

	/* Clean ciphertext */
	memset(ciphertext, '\0', sizeof(char) * len_plaintext);

	/* If key is too short */
	if (len_key < len_plaintext)
	{
		fprintf(stderr, "Error: key is too short\n");
		return NULL;
	}

	/* Iterate through each char in plaintext */
	for (i = 0; i < len_plaintext; i++)
	{
		/* Find index corresponding to plaintext char in cipherTable */
		letter_plntxt = getCiphertextIndex(cipherTable, plaintext[i]);
		
		/* Find index corresponding to key char in cipherTable */
		letter_key = getCiphertextIndex(cipherTable, key[i]);
		
#if DEBUG
		printf("letter_plntxt = %d\n", letter_plntxt);
		printf("letter_key = %d\n", letter_key);
#endif
		
		/* Validate letter */
		if (letter_plntxt < 0)
		{
			fprintf(stderr, "Error: invalid character in plaintext\n");
			return NULL;
		}
		
		if (letter_key < 0)
		{
			fprintf(stderr, "Error: invalid character in key\n");
			return NULL;
		}
		
		/* Encrypt plaintext using modular addition */
		iCiphertext = (letter_plntxt + letter_key) % NUM_CIPHERTEXT_CHAR;

		ciphertext[i] = cipherTable[iCiphertext];
		
#if DEBUG
		printf("(%d + %d) mod %d = %d (%c)\n", letter_plntxt, letter_key, NUM_CIPHERTEXT_CHAR, (int)ciphertext[i], (char)ciphertext[i]);
#endif
	}

#if DEBUG
	printf("ciphertext = %s\n", ciphertext);
#endif

	return ciphertext;
}

/******************************************************************************
* Name:			main
* Arguments:	argc, argv
* Return:		Exit Code
* Description:	Entry point for otp_enc_d
******************************************************************************/
int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, i;
	pid_t pid;
	socklen_t sizeOfClientInfo;
	char *ciphertext = NULL;
	char buffer[MAX_BUFFER_SIZE];
	char plaintext[MAX_BUFFER_SIZE];
	char key[MAX_BUFFER_SIZE];
	struct sockaddr_in serverAddress, clientAddress;
	struct sigaction SIGCHLD_action = {{0}};
	char cipherTable[NUM_CIPHERTEXT_CHAR];

	/* Check usage & args */
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}
	
	/* Setup ciphertext ascii table values. This table consists of A-Z 
	 * capital letters and space ' '.
	 */
	for (i = 0; i < (NUM_CIPHERTEXT_CHAR - 1); i++)
	{
		cipherTable[i] = 65 + i;
	}
	
	/* Last ascii code represents space ' ' */
	cipherTable[NUM_CIPHERTEXT_CHAR - 1] = 32;

	/* Set up the address struct for this process (the server) */
	/* Clear out the address struct */
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));
	
	/* Get the port number, convert to an integer from a string */
	portNumber = atoi(argv[1]);
	
	/* Create a network-capable socket */
	serverAddress.sin_family = AF_INET;
	
	/* Store the port number */
	serverAddress.sin_port = htons(portNumber);
	
	/* Any address is allowed for connection to this process */
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	/* Create the socket */
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	if (listenSocketFD < 0)
		error("ERROR opening socket");

	/* Enable the socket to begin listening. Connect socket to port */
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		error("ERROR on binding");
	
	/* Flip the socket on - it can now receive up to 5 connections */
	listen(listenSocketFD, 5);

	/* Loop to continue listening for new connections */
	while (1)
	{
		/* Accept a connection, blocking if one is not available until one connects */
		/* Get the size of the address for the client that will connect */
		sizeOfClientInfo = sizeof(clientAddress);
		
		 /* Accept connection */
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		
		if (establishedConnectionFD < 0)
			error("ERROR on accept");
		
#if 0
		printf("[SERVER] Connected client at port %d\n", ntohs(clientAddress.sin_port));
#endif

		/* otp_enc_d must support up to five concurrent socket connections
		 * running at the same time. Fork a new process that handles 
		 * encryption.
		 */
		pid = fork();
		
		switch (pid)
		{
			case 0:
				/* This is a child process */
				
				/* Clear variables */
				memset(plaintext, '\0', sizeof(plaintext));
				memset(key, '\0', sizeof(key));
				
				/* 1st packet - Identify client process */
				receiveData(establishedConnectionFD, buffer, MAX_BUFFER_SIZE - 1);
				
				/* If client process is otp_enc */
				if (strcmp(buffer, "otp_enc: connection request") == 0)
				{
					/* Signal client that server is ready to receive plaintext */
					sendData(establishedConnectionFD, "otp_enc_d: connection success");
					
					/* Receive plaintext */
					receiveData(establishedConnectionFD, plaintext, MAX_BUFFER_SIZE - 1);
					
					/* Signal client that server is ready to receive key */
					sendData(establishedConnectionFD, "otp_enc_d: ready to receive key");
					
					/* Receive key */
					receiveData(establishedConnectionFD, key, MAX_BUFFER_SIZE - 1);

					/* Encrypt plaintext using key */
					ciphertext = encryptPlaintext(cipherTable, plaintext, key);
					
					/* Validate ciphertext */
					if (ciphertext == NULL)
						exit(ERROR_GENERIC);
					
					/* Send ciphertext to client */
					sendData(establishedConnectionFD, ciphertext);
					
					/* Cleanup */
					free(ciphertext);
				}
				else
				{					
					/* Signal client that server is ready to receive plaintext */
					sendData(establishedConnectionFD, "otp_enc_d: connection refused");
				}
				
				/* Close the existing socket which is connected to the client */
				close(establishedConnectionFD);
				
				/* Exit child process */
				exit(ERROR_NONE);
		
				break;
				
			case -1:
				/* Error occurred while trying to fork a process */
				error("ERROR on fork");
				break;
				
			default:
				/* This is a parent process */

				/* Setup parent process to ignore SIGCHLD signal.
				 * SIGCHLD is ignored by the system and the child process
				 * is deleted from the process table, so no zombie
				 * process is created after the child process exits.
				 */
				SIGCHLD_action.sa_handler = SIG_IGN;
				sigfillset(&SIGCHLD_action.sa_mask);
				SIGCHLD_action.sa_flags = 0;
				sigaction(SIGCHLD, &SIGCHLD_action, NULL);
				break;
		};
	}

	/* Close the listening socket */
	close(listenSocketFD);

	return ERROR_NONE;
}

/******************************************************************************
* Author:	Edmund Dea (deae@oregonstate.edu)
* Student ID:	933280343
* Date:		10/28/2019
* Title:	CS372 Project 1: chatclient
* Description:	This program acts a chat client that sends and receives 
*               messages with a chat server using socket programming.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define DEBUG					0

/* Global macros */
#define MAX_BUFFER_SIZE				4096
#define MAX_CHAR_PER_LINE			100
#define NUM_CONNECTIONS				5
#define NUM_ASCII_CHAR				27
#define INCOMPLETE				0
#define COMPLETE				1
#define DEFAULT_HOSTNAME			"localhost"
#define MAX_USERNAME_LEN			10

/* Exit codes */
#define ERROR_NONE				0
#define ERROR_GENERIC				1
#define ERROR_CONNECTION_REFUSED		2
#define ERROR_ARGS				3
#define ERROR_FILEIO				4
#define ERROR_OOM				5
#define ERROR_SEND				6

/* Debug print macros */
#if DEBUG
#define DBG_PRINT(format, arg) printf(format, arg);
#define DBG_PRINT2(format, arg1, agr2) printf(format, arg1, arg2);
#define DBG_PRINT3(format, arg1, agr2, arg3) printf(format, arg1, arg2, arg3);
#else
#define DBG_PRINT(format, arg)
#define DBG_PRINT2(format, arg1, arg2)
#define DBG_PRINT3(format, arg1, arg2, arg3)
#endif

/******************************************************************************
* Name:		error
* Arguments:	string
* Return:	void
* Description:	Error function used for reporting issues
******************************************************************************/
void error(const char *msg)
{
	perror(msg);
	exit(ERROR_GENERIC);
}

/******************************************************************************
* Name:		sendData
* Arguments:	socket file descriptor, data
* Return:	Exit Code
* Description:	Entry point for otp_enc
******************************************************************************/
int sendData(int socketFD, char data[MAX_BUFFER_SIZE])
{
	int charsWritten;
	int i, status, maxRetries = 10;

	/* Keep trying to send data until success up to maxRetries times */
	for (i = 0; i < maxRetries; i++)
	{
		status = INCOMPLETE;

		/* Send message to server. Write to the server. */
		charsWritten = send(socketFD, data, strlen(data), 0);
		
		if (charsWritten < 0)
		{
			error("chatclient: error writing to socket");
			status = INCOMPLETE;
		}
		else
		{
			DBG_PRINT("chatclient> Sent \"%s\"\n", data);
			status = COMPLETE;
			break;
		}
	}
	
	return status;
}

/******************************************************************************
* Name:		receiveData
* Arguments:	[in] socket file descriptor
*		[out] buffer
* Return:	charsRead
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
			error("chatclient: error reading from socket");
		}
		else
		{
			DBG_PRINT("chatclient: received \"%s\"\n", buffer);
			status = COMPLETE;
			break;
		}
	}
	
	return status;
}

/******************************************************************************
* Name:		main
* Arguments:	argc, argv
* Return:	Exit Code
* Description:	Entry point for chatclient
******************************************************************************/
int main(int argc, char *argv[])
{
	int socketFD = 0;
	int portNumber = 0;
	int status = ERROR_NONE;
	int chat_status = 1;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[MAX_BUFFER_SIZE];
	char *conn_buff;
	char *hostName = NULL;
	char *tmp;
	char username[MAX_BUFFER_SIZE];
	char svr_username[MAX_BUFFER_SIZE];

	/* Check usage & args */
	if (argc != 3)
	{
		fprintf(stderr,"usage: %s <hostname> <portNumber>\n", argv[0]);
		exit(ERROR_ARGS);
	}

	/* Set up the server address struct */
	/* Clear variables before use */
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));

	/* Get host name and port number from command line */
	if (!argv[1] || !argv[2])
		exit(ERROR_ARGS);
	else
	{
		hostName = argv[1];
		portNumber = atoi(argv[2]);
	}

	/* Prompt user to enter username */
	memset(username, 0, MAX_BUFFER_SIZE);
	while (1)
	{
		printf("Enter username: ");
		fgets(username, MAX_BUFFER_SIZE, stdin);

		/* Remove newline from input */
		username[strcspn(username, "\n")] = 0;

		/* Validate username */
		if (strlen(username) == 0 || strlen(username) > MAX_USERNAME_LEN)
			printf("error: username must be between 1 and 10 characters.\n");
		else
			break;
	}

	DBG_PRINT3("hostName = %s, portNumber = %d, username = \"%s\"\n", hostName, portNumber, username);

	/* Create a network-capable socket */
	serverAddress.sin_family = AF_INET;
	
	/* Store the port number */
	serverAddress.sin_port = htons(portNumber);
	
	/* Convert the machine name into a special form of address */
	serverHostInfo = gethostbyname(hostName);
	
	if (serverHostInfo == NULL)
	{
		fprintf(stderr, "chatclient: error: no such host\n");
		exit(0);
	}
	
	/* Copy in the address */
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	/* Create the socket */
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socketFD < 0)
		error("chatclient: error: opening socket");
	
	/* Connect to server. Connect socket to address. */
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("chatclient: error: connecting");

	/* Send connection request to server */
	strcpy(buffer, "chatclient: connection request.");
	strcat(buffer, username);
	sendData(socketFD, buffer);

	/* Wait for server to acknowledge connection */
	receiveData(socketFD, buffer, sizeof(buffer));

	/* Parse received data */
	conn_buff = strtok(buffer, ".");
	tmp = strtok(NULL, ".");
	strcpy(svr_username, tmp);

	/* If connection was successful */
	if (strcmp(conn_buff, "chatserve: connection success") == 0)
	{
		while (chat_status)
		{
			/* Get input message from user */
			strcpy(buffer, username);
			strcat(buffer, "> ");
			printf("%s", buffer);
	
			/* Get input from the user, truncate to buffer - 1 chars, leaving \0 */
			fgets(buffer, MAX_BUFFER_SIZE, stdin);

			/* Remove newline from input */
			buffer[strcspn(buffer, "\n")] = 0;

			/* Send message to server. */
			sendData(socketFD, buffer);

			/* If server entered \quit, then exit */
			if (strcmp(buffer, "\\quit") == 0)
			{
				chat_status = 0;
				break;
			}
			else
			{
				/* Wait to receive data from server. */
				receiveData(socketFD, buffer, sizeof(buffer));

				/* Display msg received */
				printf("%s> %s\n", svr_username, buffer);

				/* If server entered \quit, then exit */
				if (strcmp(buffer, "\\quit") == 0)
					chat_status = 0;
			}
		}
	}
	else
	{
		/* Connection refused */
		fprintf(stderr, "chatclient: error: could not contact %s on port %d\n", hostName, portNumber);
		
		/* Set exit code */
		status = ERROR_CONNECTION_REFUSED;
	}

	/* Close the socket */
	close(socketFD);

	return status;
}

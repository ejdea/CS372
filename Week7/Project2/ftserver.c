/******************************************************************************
* Author:			Edmund Dea (deae@oregonstate.edu)
* Student ID:		933280343
* Last Modified:	12/1/2019
* Course:			CS372
* Title:			Project 2: FTP Server
* Description:		This program acts as an FTP server that sends and receives 
*					files using socket programming.
* References:		Many sections of this code are from CS344 projects
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG					1
#define DEBUG2					0

/* Global macros */
#define MAX_BUFFER_SIZE			4096
#define MAX_CHAR_PER_LINE		100
#define NUM_CONNECTIONS			5
#define NUM_ASCII_CHAR			27
#define INCOMPLETE				0
#define COMPLETE				1
#define DEFAULT_HOSTNAME		"flip3.engr.oregonstate.edu"
#define MAX_USERNAME_LEN		10
#define MAX_CMDLINE_ARGUMENTS	512
#define FD_STDIN				0
#define FD_STDOUT				1
#define FD_STDERR				2

/* Exit codes */
#define ERROR_NONE					0
#define ERROR_GENERIC				1
#define ERROR_CONNECTION_REFUSED	2
#define ERROR_ARGS					3
#define ERROR_FILEIO				4
#define ERROR_OOM					5
#define ERROR_SEND					6
#define ERROR_FORK					7
#define ERROR_EXEC					8
#define ERR_GETHOSTBYNAME			9

/* Debug print macros */
#if DEBUG
#define DBG_PRINT(format) printf(format);
#define DBG_PRINT1(format, arg) printf(format, arg);
#define DBG_PRINT2(format, arg1, arg2) printf(format, arg1, arg2);
#define DBG_PRINT3(format, arg1, arg2, arg3) printf(format, arg1, arg2, arg3);
#else
#define DBG_PRINT(format, arg)
#define DBG_PRINT2(format, arg1, arg2)
#define DBG_PRINT3(format, arg1, arg2, arg3)
#endif

typedef struct sigaction sig_action;

static int ctrlSocketFD, ctrlConnFD, dataSocketFD;
static int exitStatus = 0;

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
* Arguments:	N/A
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
* Name:			catchSIGINT
* Arguments:	N/A
* Return:		void
* Description:	Catches and handles the SIGINT signal.
******************************************************************************/
void catchSIGINT(int signo)
{
#if DEBUG
	/* Output status to user */
	char msg[] = "\n[ftserver] Exiting...\n";
	write(STDOUT_FILENO, msg, strlen2(msg));
	fflush(stdout);
#endif

	exitStatus = 1;

	/* Close sockets */
	if (ctrlSocketFD)
		close(ctrlSocketFD);

	if (ctrlConnFD)
		close(ctrlConnFD);

	if (dataSocketFD)
		close(dataSocketFD);
}

/******************************************************************************
* Name:			sendData
* Arguments:	socket file descriptor, data
* Return:		Exit Code
* Description:	Transmits data over the socket to the target host
* References:	This function was previously developed in CS344
******************************************************************************/
int sendData(int socketFD, char data[MAX_BUFFER_SIZE])
{
	int charsWritten;
	int i, status, maxRetries = 10;

#if DEBUG2
	DBG_PRINT("[ftserver] ---> sendData ENTER\n")
#endif

	/* Keep trying to send data until success up to maxRetries times */
	for (i = 0; i < maxRetries; i++)
	{
		status = INCOMPLETE;

		/* Send message to server. Write to the server. */
		charsWritten = send(socketFD, data, strlen(data), 0);
		
		if (charsWritten < 0)
		{
			error("ftserver: error writing to socket");
			status = INCOMPLETE;
		}
		else
		{
			DBG_PRINT1("[ftserver] Sent \"%s\"\n", data);
			status = COMPLETE;
			break;
		}
	}
	
#if DEBUG2
	DBG_PRINT("[ftserver] <--- sendData EXIT\n")
#endif

	return status;
}

/******************************************************************************
* Name:			receiveData
* Arguments:	[in] socket file descriptor
*				[out] buffer
* Return:		charsRead
* Description:	Receives data from the target host
* References:	This function was previously developed in CS344
******************************************************************************/
int receiveData(int socketFD, char *buffer, int bufferSize)
{
	int charsRead, i, status, maxRetries = 10;
	
#if DEBUG2
	DBG_PRINT("[ftserver] ---> receiveData ENTER\n")
#endif

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
			error("ftserver: error reading from socket");
		}
		else
		{
			DBG_PRINT1("[ftserver] received \"%s\"\n", buffer);
			status = COMPLETE;
			break;
		}
	}
	
#if DEBUG2
	DBG_PRINT("[ftserver] <--- receiveData EXIT\n")
#endif

	return status;
}

/******************************************************************************
* Name:			startup
* Arguments:	N/A
* Return:		Exit Code
* Description:	Starts the FTP server and handles connections with ftclient
* Reference:	Some sections of this code are from Edmund's CS344 projects
******************************************************************************/
int startup(char *hostName, int ctrlPort)
{
	int status = ERROR_NONE, fileStatus = ERROR_NONE;
	int dataPort = 0;
	int enable = 1;
	struct sockaddr_in ctrlSocketAddr, dataSocketAddr;
	struct hostent *ctrlServerHostInfo, *dataServerHostInfo;
	char buffer[MAX_BUFFER_SIZE];
	char buffer2[MAX_BUFFER_SIZE];
	char filename[MAX_BUFFER_SIZE];
	char *args[MAX_CMDLINE_ARGUMENTS];
	pid_t pid;
	socklen_t sizeOfClientInfo;
	int pipe_fd[2];
	sig_action SIGINT_action_parent = { {0} };
	struct stat st = { 0 };
	FILE *fp;
	long byteSize;
	int length = 0;

	/* Init pipe to redirect cmd output to string 
	 * Reference - https://stackoverflow.com/questions/50281787/putting-output-of-execvp-into-string
	 */
	pipe(pipe_fd);

	/* Setup parent process to ignore SIGINT signal */
	SIGINT_action_parent.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action_parent.sa_mask);
	SIGINT_action_parent.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action_parent, NULL);
	exitStatus = 0;

	/* Set up the server address struct */
	memset((char*)&ctrlSocketAddr, '\0', sizeof(ctrlSocketAddr));
	
	/* Create a network-capable socket */
	ctrlSocketAddr.sin_family = AF_INET;
	
	/* Store the port number */
	ctrlSocketAddr.sin_port = htons(ctrlPort);
	
	/* Convert the machine name into a special form of address */
	ctrlServerHostInfo = gethostbyname(hostName);
	
	if (ctrlServerHostInfo == NULL)
	{
		fprintf(stderr, "ftserver: error: no such host\n");
		exit(ERR_GETHOSTBYNAME);
	}
	
	/* Copy in the address */
	memcpy((char*)&ctrlSocketAddr.sin_addr.s_addr, (char*)ctrlServerHostInfo->h_addr, ctrlServerHostInfo->h_length);

	/* Create the socket */
	ctrlSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	if (ctrlSocketFD < 0)
		error("ftserver: error: could not open socket");

	/* Setup socket address to be reusable */
	if (setsockopt(ctrlSocketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		error("ftserver: error: failed to set socket options");

	/* Enable the socket to begin listening. Connect socket to port */
	if (bind(ctrlSocketFD, (struct sockaddr *)&ctrlSocketAddr, sizeof(ctrlSocketAddr)) < 0)
		error("ftserver: error: could not bind socket to port");
	
	DBG_PRINT2("[ftserver] Ctrl Connection: hostName = %s, ctrlPort = %d\n", hostName, ctrlPort);

	/* Enable socket - it can now receive up to 1 connection */
	listen(ctrlSocketFD, 1);

	/* Accept connection */
	ctrlConnFD = accept(ctrlSocketFD, (struct sockaddr *)&ctrlSocketAddr, &sizeOfClientInfo);

	if (!exitStatus && ctrlConnFD < 0)
		error("ftserver: error: could not accept connection");

	if (exitStatus)
		exit(ERROR_NONE);

	DBG_PRINT2("[ftserver] Established control connection with %s:%d\n", hostName, ctrlPort);

	/* Process commands from ftclient */
	{
		/* Clear buffer */
		memset(buffer, '\0', MAX_BUFFER_SIZE);

		/* Wait for ftclient to send a command */
		receiveData(ctrlConnFD, buffer, MAX_BUFFER_SIZE);

		/* Validate command */
		if (strcmp(buffer, "-l") != 0 && strcmp(buffer, "-g") != 0)
		{
			/* Signal client that server is ready to receive plaintext */
			sendData(ctrlConnFD, "ftserver: error: invalid command");
		}
		else
		{
			/* Initiate a data socket connection with ftclient */

			/* Send ack */
			sendData(ctrlConnFD, "ftserver: ack");

			DBG_PRINT("[ftserver] Waiting to receive port number\n");

			/* Receive data port number */
			memset(buffer2, '\0', sizeof(buffer2));
			receiveData(ctrlConnFD, buffer2, sizeof(buffer2));
			
			/* Convert data port number to int */
			dataPort = atoi(buffer2);

			/* Send ack to ftclient */
			sendData(ctrlConnFD, "ftserver: ack");

			/* Set up the server address struct */
			memset((char*)&dataSocketAddr, '\0', sizeof(dataSocketAddr));

			/* Create a network-capable socket */
			dataSocketAddr.sin_family = AF_INET;

			/* Store the port number */
			dataSocketAddr.sin_port = htons(dataPort);

			DBG_PRINT2("[ftserver] dataPort = %d, dataSocketAddr.sin_port = %hu\n", dataPort, dataSocketAddr.sin_port);

			/* Convert the machine name into a special form of address */
			dataServerHostInfo = gethostbyname(hostName);

			if (dataServerHostInfo == NULL)
			{
				DBG_PRINT1("ftserver: error: could not get host by name %s\n", hostName);

				/* Signal client that server is ready to receive plaintext */
				sendData(ctrlConnFD, "ftserver: error: could not get host by name");

				exit(ERR_GETHOSTBYNAME);
			}

			/* Copy in the address */
			memcpy((char*)&dataSocketAddr.sin_addr.s_addr, (char*)dataServerHostInfo->h_addr, dataServerHostInfo->h_length);

			/* Create the socket */
			dataSocketFD = socket(AF_INET, SOCK_STREAM, 0);

			if (dataSocketFD < 0)
				error("ftserver: error: opening socket");

			/* Setup socket address to be reusable */
			if (setsockopt(dataSocketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
				error("ftserver: error: failed to set socket options");

			DBG_PRINT2("[ftserver] Data Connection: hostName = %s, dataPort = %d\n", hostName, dataPort);
			DBG_PRINT("[ftserver] Waiting to receive ftclient connection ready status\n");

			/* Wait for ftclient to send connection ready status */
			memset(buffer2, '\0', sizeof(buffer2));
			receiveData(ctrlConnFD, buffer2, sizeof(buffer2));

			/* Send ack to ftclient */
			sendData(ctrlConnFD, "ftserver: ack");

			/* Connect to server. Connect socket to address. */
			if (connect(dataSocketFD, (struct sockaddr*)&dataSocketAddr, sizeof(dataSocketAddr)) < 0)
			{
				/* Signal client that server is ready to receive plaintext */
				sendData(ctrlConnFD, "ftserver: error: could not connect to ftclient");

				error("ftserver: error: could not connect to socket");
			}

			DBG_PRINT2("[ftserver] Established data connection with %s:%d\n", hostName, dataPort);

			if (strcmp(buffer, "-l") == 0)
			{
				DBG_PRINT("[ftserver] cmd = -l\n");

				/* Fork process to run a separate cmd */
				pid = fork();

				switch (pid)
				{
				case 0:
					/* This is the Child process. Run the requested cmd. */

					/* Setup arguments */
					memset(args, 0, sizeof(args));
					args[0] = "ls";
					/*args[1] = "-l";*/

					/* Capture cmd output by redirect output to stdout */
					dup2(pipe_fd[1], FD_STDOUT);
					dup2(pipe_fd[1], FD_STDERR);
					close(pipe_fd[1]);

					/* Run cmd */
					status = execvp(args[0], args);

					/* Validate execvp result */
					if (status == -1)
					{
						/* Output error */
						fprintf(stderr, "%s: no such file or directory\n", args[0]);
						fflush(stderr);

						/* Set error status */
						error("ftserver: error: failed to run forked process");
						return ERROR_EXEC;
					}
					break;

				case -1:
					error("ftserver: error: failed to fork process");
					return ERROR_FORK;

				default:
					/* This is the Parent process. Wait for child process to finish and
					 * cmd output.
					 */

					/* Block parent process until child process ends */
					waitpid(-1, &status, 0);

					/* Close target file descriptor*/
					close(pipe_fd[1]);

					/* Read redirected output from child process */
					memset(buffer2, '\0', sizeof(buffer2));
					read(pipe_fd[0], buffer2, MAX_BUFFER_SIZE);

					/*DBG_PRINT2("size = %d, output = %s\n", size, buffer2);*/

					/* Send output from child process to ftclient */
					sendData(dataSocketFD, buffer2);
					break;
				}
			}
			else if (strcmp(buffer, "-g") == 0)
			{
				DBG_PRINT("[ftserver] Wait for ftclient to send filename\n");

				/* Wait for ftclient to send filename */
				memset(buffer, '\0', sizeof(buffer));
				receiveData(dataSocketFD, buffer, sizeof(buffer));

				/* Save filename */
				memcpy(filename, buffer, sizeof(buffer));

				if (strcmp(filename, "-1/Invalid filename/") == 0)
				{
					DBG_PRINT("ftclient: error: invalid filename\n");

					/* Signal ftclient filename is invalid */
					sendData(dataSocketFD, "ftclient: error: invalid filename");
				}
				else
				{
					/* Get file size */
					fileStatus = stat(filename, &st);

					if (fileStatus == 0)
					{
						/* Convert file size from int to string */
						memset(buffer, '\0', sizeof(buffer));
						sprintf(buffer, "%lu", st.st_size);
						byteSize = st.st_size;

						DBG_PRINT1("[ftserver] File size = %s\n", buffer);

						/* Send file size to ftclient */
						sendData(dataSocketFD, buffer);

						/* Receive ack from ftclient */
						memset(buffer, '\0', sizeof(buffer));
						receiveData(dataSocketFD, buffer, sizeof(buffer));

						if (strcmp(buffer, "ftclient: ack") == 0)
						{
							/* Open requested file with read permissions */
							fp = fopen(filename, "r");
							
							if (!fp)
							{
								fprintf(stderr, "ftserver: error: failed to open file %s\n", filename);
								status = ERROR_FILEIO;
							}
							else
							{
								/* Set filestream pointer to beginning of file */
								fseek(fp, 0L, SEEK_SET);

								DBG_PRINT1("[ftserver] byteSize = %ld\n", byteSize);

								/* Read entire contents of file into memory */
								memset(buffer, '\0', sizeof(buffer));
								fread(buffer, 1, byteSize, fp);

								DBG_PRINT1("[ftserver] buffer = \"%s\"\n", buffer);

								/* Send file to ftclient */
								sendData(dataSocketFD, buffer);
							}

							/* Close file */
							if (fp)
								fclose(fp);
						}
						else
						{
							error("ftserver: error: failed to receive ack from ftserver\n");
						}
					}
					else
					{
						DBG_PRINT("ftserver: error: could not get file size\n");

						/* Signal ftclient filename is invalid */
						sendData(dataSocketFD, "ftclient: error: invalid filename");

						status = ERROR_FILEIO;
					}
				}
			}
		}
	}

	/* Close sockets */
	if (ctrlSocketFD)
	{
		DBG_PRINT("[ftserver] Cleanup ctrlSocketFD\n");
		close(ctrlSocketFD);
	}

	if (ctrlConnFD)
	{
		DBG_PRINT("[ftserver] Cleanup ctrlConnFD\n");
		close(ctrlConnFD);
	}

	if (dataSocketFD)
	{
		DBG_PRINT("[ftserver] Cleanup dataSocketFD\n");
		close(dataSocketFD);
	}
	
	DBG_PRINT1("[ftserver] Return status %d\n", status);

	return status;
}

/******************************************************************************
* Name:			main
* Arguments:	argc, argv
* Return:		Exit Code
* Description:	Entry point for ftserver
******************************************************************************/
int main(int argc, char *argv[])
{
	char *hostName = DEFAULT_HOSTNAME;
	int ctrlPort = 0;
	int status = ERROR_NONE;

	/* Check usage & args */
	if (argc != 2)
	{
		fprintf(stderr,"usage: %s <port_number>\n", argv[0]);
		exit(ERROR_ARGS);
	}

	/* Get host name and port number from command line */
	if (!argv[1])
		exit(ERROR_ARGS);
	else
		ctrlPort = atoi(argv[1]);

	/* Start FTP server */
	status = startup(hostName, ctrlPort);

	return status;
}

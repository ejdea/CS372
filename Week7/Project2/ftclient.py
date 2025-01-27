###############################################################################
# Author:	 	Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	11/30/2019
# Course:		CS372
# Title:		Project 2: FTP Client
# Description:		This program acts an FTP client that sends and receives 
#			files with an FTP server using socket programming.
###############################################################################

import socket
import sys
import signal
import os.path

DEBUG 				= 0

# Macros
MAX_BUFFER_SIZE 		= 4096
MAX_CONNECTION_REQUESTS		= 1
CHAR_ENCODING_FORMAT		= "utf-8"
DEFAULT_HOSTNAME        	= "localhost"
DEFAULT_CTRLPORT        	= 35121
DEFAULT_DATAPORT        	= 35122
MIN_PORT_NUMBER			= 1
MAX_PORT_NUMBER			= 2

# Exit codes
ERR_ARGS			= 1

#------------------------------------------------------------------------------
# Class:       dbg_print
# Description: Prints debug messages to stdout
#------------------------------------------------------------------------------
def dbg_print(msg):
	if DEBUG == 1:
		print("[ftclient] " + msg)

#------------------------------------------------------------------------------
# Class:       exithandler
# Description: Handles SIGINT interrupts
# Reference:   https://docs.python.org/2/library/signal.html
#------------------------------------------------------------------------------
def exitHandler(signum, frame):
	dbg_print("\r\nExiting...\r\n")
	sys.exit(0)

#------------------------------------------------------------------------------
# Class:       send
# Description: Sends a packet to the network destination
#------------------------------------------------------------------------------
def sendData(sock, data):
	dbg_print("sent \"" + data + "\"")
	sock.send(data)

#------------------------------------------------------------------------------
# Class:       receiveData
# Description: Receives data from the network source
# Reference:   This function is based on Edmund's CS344 project
#------------------------------------------------------------------------------
def receiveData(conn, fileSize, useFileSize):
	charsRead = 0
	charsReadTotal = 0
	buffer = ""

	if useFileSize == 0:
		buffer = conn.recv(MAX_BUFFER_SIZE)
		dbg_print("received \"" + buffer + "\"")
	else:
		# Keep receiving file data until full file is transferred
		while charsRead < fileSize:
			data = conn.recv(MAX_BUFFER_SIZE)

			if not data:
				# If received no data, then end loop
				break
			else:
				# Get size of packet transferred
				charsRead = len(data)

				# Update total file size transferred
				charsReadTotal += charsRead

				# Append data received to buffer
				buffer += data

				dbg_print("received " + str(charsReadTotal) + "/" + str(fileSize))

	#dbg_print("[receiveFile] buffer = " + buffer)

	return buffer

#------------------------------------------------------------------------------
# Class:       initiateContact
# Description: Starts a socket connection with ftserver
#------------------------------------------------------------------------------
def initiateContact(hostname, ctrlPort):
	dbg_print("socket = " + hostname + ":" + str(ctrlPort))

	# Register signal handler for SIGINT
	signal.signal(signal.SIGINT, exitHandler)

	# Open TCP socket
	ctrlSockFD = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# Set socket options
	ctrlSockFD.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

	# Connect to socket with control port
	try:
		ctrlSockFD.connect((hostname, ctrlPort))
	except socket.error as err:
		sock = None
		print("ftclient: error: could not connect to " + hostname + ":" + str(ctrlPort))
		sys.exit(ERR_ARGS)

	return ctrlSockFD

#------------------------------------------------------------------------------
# Class:       makeRequest
# Description: Parses command request and sends it to ftserver
#------------------------------------------------------------------------------
def makeRequest(cmd, ctrlSockFD, dataConn, hostname, dataPort, filename):
	if cmd == "-l":
		# Receive connection request
		data = receiveData(dataConn, 0, 0)

		print("Receiving directory structure from " + hostname + ":" + str(dataPort))

		# Output directory structure
		print(data)
	elif cmd == "-g":
		# Send filename
		sendData(dataConn, filename)

		# Receive file payload size
		fileSize = receiveData(dataConn, 0, 0)

		dbg_print("fileSize = " + str(fileSize))

		# Send ack to ftserver
		sendData(dataConn, "ftclient: ack")

		if fileSize == "ftclient: error" or str(fileSize) == "":
			# Receive error msg
			data = receiveData(ctrlSockFD, 0, 0)

			# Print error msg
			print(data)
		else:
			# Wait for ftserver to send file
			dbg_print("[ftclient] Wait for ftserver to send file");
			data = receiveData(dataConn, fileSize, 1)

			#dbg_print("data = " + data)

			# Check if file exists already
			# Reference: https://linuxize.com/post/python-check-if-file-exists/
			if os.path.exists(filename):
				choice = raw_input("File already exists. Do you want to overwrite \"" + filename + "\" (y/n)? ")
				while not choice or (choice != "y" and choice != "n"):
					choice = raw_input("File already exists. Do you want to overwrite \"" + filename + "\" (y/n)? ")

			if choice == "y":
				# Save data to file
				fpFile = open(filename, "w")

				# Write data to file
				# Reference: https://www.pythonforbeginners.com/files/reading-and-writing-files-in-python
				fpFile.write(data)

				# Cleanup
				if (fpFile):
					fpFile.close()
			elif choice == "n":
				print("Skipped file overwrite")

			# Output status
			print("transfer complete")

#------------------------------------------------------------------------------
# Class:       startup
# Description: Initiates the client server
#------------------------------------------------------------------------------
def startup():
	hostname = DEFAULT_HOSTNAME
	ctrlPort = DEFAULT_CTRLPORT
	dataPort = DEFAULT_DATAPORT
	ctrlSockFD = None
	dataSockFD = None
	cmd = None
	choice = None
	filename = "-1/Invalid filename/"

	# Validate and parse input from command line
	if len(sys.argv) < 5 or len(sys.argv) > 6:
		sys.exit("incorrect command line format.\r\n\r\n" +
			 "usage: List directory\r\nftclient <server_hostname> <server_control_port> <server_data_port> -l\r\n\r\n" +
			 "usage: Get file\r\nftclient <server_hostname> <server_control_port> <server_data_port> -g <filename>\r\n")
	elif ctrlPort == dataPort:
		sys.exit("ftclient: error: server control port cannot be the same as server data port.\r\n" +
			 "usage: ftclient <server_hostname> <server_control_port> <server_data_port>")
	elif ctrlPort < 1 or dataPort < 1 or ctrlPort > 65535 or dataPort > 65535:
		sys.exit("ftclient: error: port numbers be in the range 1 to 65535.\r\n" +
			 "usage: ftclient <server_hostname> <server_control_port> <server_data_port>")
	else:
		hostname = sys.argv[1]
		ctrlPort = int(sys.argv[2])
		dataPort = int(sys.argv[3])
		cmd = sys.argv[4]

		if len(sys.argv) == 6:
			filename = sys.argv[5]

	# Initiate contact with ftserver
	ctrlSockFD = initiateContact(hostname, ctrlPort)

	# Debug output
	dbg_print("hostname = " + hostname)
	dbg_print("ctrlPort = " + str(ctrlPort))
	dbg_print("dataPort = " + str(dataPort))
	dbg_print("cmd = " + cmd)
	if len(sys.argv) == 6:
		dbg_print("filename = " + filename)

	try:
		# Send command to ftserver
		sendData(ctrlSockFD, cmd)

		# Wait for ack response from ftserver
		data = receiveData(ctrlSockFD, 0, 0)

		if (data == "ftserver: ack"):
			# Send port number to ftserver
			dbg_print("Send port number to ftserver");
			sendData(ctrlSockFD, str(dataPort))

			# Receive response from ftserver
			dbg_print("Receive response from ftserver");
			data = receiveData(ctrlSockFD, 0, 0)

			# Send port number to ftserver
			dbg_print("Send hostname to ftserver");
			sendData(ctrlSockFD, socket.gethostname())

			# Receive response from ftserver
			dbg_print("Receive response from ftserver");
			data = receiveData(ctrlSockFD, 0, 0)

			dbg_print("Received data = " + data)

			# Parse ftserver response
			if data == "ftserver: error: invalid command":
				print("ftclient: error: invalid command")
			elif data == "ftserver: error: could not connect to ftclient":
				print("ftclient: error: could not connect to ftclient")
			else:
				# Let ftserver initiate a data socket connection

				dbg_print("Data connection: hostname = " + hostname + ", dataPort = " + str(dataPort))

				# Open TCP socket
				dataSockFD = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

				# Set socket options
				dataSockFD.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

				# Get ftclient host name
				clientHostname = socket.gethostname()
				
				# Bind socket
				dataSockFD.bind((clientHostname, dataPort))

				# Listen for network traffic over the socket
				dataSockFD.listen(1)

				# Let ftserver know that ftclient is ready to receive connections
				dbg_print("[ftclient] Let ftserver know that ftclient is ready to receive connections");
				sendData(ctrlSockFD, "ftclient: data connection ready")

				dbg_print("[ftclient] Listen started...")

				# Accept connection requets
				dataConn, dataAddr = dataSockFD.accept()
				dbg_print("[ftclient] Accepted data connection request")

				# Wait for ack response from ftserver
				dbg_print("[ftclient] Wait for ack response from ftserver");
				data = receiveData(ctrlSockFD, 0, 0)

				if data == "ftserver: ack":
					dbg_print("[ftclient] Received connection request")

				# Send cmd to ftserver
				makeRequest(cmd, ctrlSockFD, dataConn, hostname, dataPort, filename)

		elif data == "ftserver: error: could not connect to ftclient":
			print("ftclient: error: could not connect to ftclient")
		else:
			print("ftclient: error: invalid command")
	except Exception as e:
		print("ftclient: error: " + str(e));
	finally:
		dbg_print("Closing sockets")

		# Close sockets
		if ctrlSockFD is not None:
			dbg_print("Closing ctrlSockFD")
			ctrlSockFD.close()

		if dataSockFD is not None:
			dbg_print("Closing dataSockFD")
			dataSockFD.close()

#------------------------------------------------------------------------------
# Function:	Main
# Description:	Entrypoint for the application
# References:	https://docs.python.org/release/2.6.5/howto/sockets.html
#		https://docs.python.org/2/howto/sockets.html
#------------------------------------------------------------------------------
if __name__ == "__main__":
	startup()
###############################################################################
# Author:	 		Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	12/1/2019
# Course:			CS372
# Title:			Project 2: FTP Client
# Description:		This program acts an FTP client that sends and receives 
#					files with an FTP server using socket programming.
###############################################################################

import socket
import sys
import signal

DEBUG 					= 1

ERR_ARGS                = 1

MAX_BUFFER_SIZE 		= 4096
MAX_CONNECTION_REQUESTS	= 1
CHAR_ENCODING_FORMAT	= "utf-8"
DEFAULT_HOSTNAME        = "flip3.engr.oregonstate.edu"
DEFAULT_CTRLPORT        = 35121
DEFAULT_DATAPORT        = 35122
MIN_PORT_NUMBER			= 1
MAX_PORT_NUMBER         = 2

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
def send(sock, data):
	dbg_print("sent \"" + data + "\"")
	sock.send(data)

#------------------------------------------------------------------------------
# Class:       receive
# Description: Receives a packet from the network source
#------------------------------------------------------------------------------
def receive(conn):
	data = conn.recv(MAX_BUFFER_SIZE)
	dbg_print("received \"" + data + "\"")
	return data

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
	filename = None

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

	# Get hostname
	hostname = socket.gethostname()

	# Debug output
	dbg_print("hostname = " + hostname)
	dbg_print("ctrlPort = " + str(ctrlPort))
	dbg_print("dataPort = " + str(dataPort))
	dbg_print("cmd = " + cmd)
	if len(sys.argv) == 6:
		dbg_print("filename = " + filename)

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

	try:
		# Send command to ftserver
		send(ctrlSockFD, cmd)

		# Wait for ack response from ftserver
		data = receive(ctrlSockFD)

		if (data == "ftserver: ack"):
			# Send port number to ftserver
			dbg_print("Send port number to ftserver");
			send(ctrlSockFD, str(dataPort))

			# Receive response from ftserver
			dbg_print("Receive response from ftserver");
			data = receive(ctrlSockFD)

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

				# Bind socket
				dataSockFD.bind((hostname, dataPort))

				# Listen for network traffic over the socket
				dataSockFD.listen(1)

				# Let ftserver know that ftclient is ready to receive connections
				dbg_print("[ftclient] Let ftserver know that ftclient is ready to receive connections");
				send(ctrlSockFD, "ftclient: data connection ready")

				dbg_print("[ftclient] Listen started...")

				# Accept connection requets
				dataConn, dataAddr = dataSockFD.accept()

				# Wait for ack response from ftserver
				dbg_print("[ftclient] Wait for ack response from ftserver");
				data = receive(ctrlSockFD)

				if data == "ftserver: ack":
					dbg_print("Received connection request")

					# Receive connection request
					data = receive(dataConn)

					print(data)
		elif data == "ftserver: error: could not connect to ftclient":
			print("ftclient: error: could not connect to ftclient")
		else:
			print("ftclient: error: invalid command")
	except Exception as e:
		print("ftclient: error: " + str(e));
	finally:
		dbg_print("Closing socket")

		# Cleanup
		if ctrlSockFD is not None:
			ctrlSockFD.close()
		if dataSockFD is not None:
			dataSockFD.close()

#------------------------------------------------------------------------------
# Function:		Main
# Description:	Entrypoint for the application
# References:	https://docs.python.org/release/2.6.5/howto/sockets.html
#				https://docs.python.org/2/howto/sockets.html
#------------------------------------------------------------------------------
if __name__ == "__main__":
	startup()
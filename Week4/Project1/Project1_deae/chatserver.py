###############################################################################
# Author:			Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	10/28/2019
# Course:			CS372
# Title:			Project 1: Chat Server
# Description:		This program acts a chat server that sends and receives 
#					messages with a chat client using socket programming.
###############################################################################

import socket
import sys
import signal
import threading

DEBUG					= 1
MAX_BUFFER_SIZE 		= 4096
DEFAULT_PORT			= 32500
MAX_CONNECTION_REQUESTS	= 1
CHAR_ENCODING_FORMAT	= "utf-8"
SVR_USERNAME			= "chatserve"
connection_status		= 1

#------------------------------------------------------------------------------
# Class:       dbg_print
# Description: Prints debug messages to stdout
#------------------------------------------------------------------------------
def dbg_print(msg):
	if DEBUG == 1:
		print(SVR_USERNAME + "/dbg> " + msg)

#------------------------------------------------------------------------------
# Class:       exithandler
# Description: Handles SIGINT interrupts
# Reference:   https://docs.python.org/2/library/signal.html
#------------------------------------------------------------------------------
def exitHandler(signum, frame):
	print("\r\nChatserve exiting.\r\n")
	sys.exit(0)

#------------------------------------------------------------------------------
# Class:       send
# Description: Sends a packet to the network destination
#------------------------------------------------------------------------------
def send(conn, data):
	while connection_status == 1:
		# Prompt chatserve host to respond with a message
		msg = raw_input(SVR_USERNAME + "> ")
							
		# Append server username to network packet
		data = msg + "[]" + SVR_USERNAME

		# Send data over the socket connection
		conn.send(data)

		# Disconnect connection
		if msg == "\quit":
			connection_status = 0

#------------------------------------------------------------------------------
# Class:       receive
# Description: Receives a packet from the network source
#------------------------------------------------------------------------------
def receive(conn, sock):
	while connection_status = 1:
		# Receive data over the socket connection
		data = conn.recv(MAX_BUFFER_SIZE)

		# Parse msg and username
		msg, client_username = data.split("[]", 1)

		dbg_print("msg = " + msg)
		
		if msg == "\quit":
			dbg_print("Disconnecting")

			# Close connection
			sock.shutdown(1)

			# Stop chatting
			connection_status = 0
		else:
			# Display received data
			print(client_username + "> " + msg)

#------------------------------------------------------------------------------
# Function:	Main
# Description:	Entrypoint for the application
# References:	https://docs.python.org/release/2.6.5/howto/sockets.html
#		https://docs.python.org/2/howto/sockets.html
#------------------------------------------------------------------------------
if __name__ == "__main__":
	host = "localhost"
	port = DEFAULT_PORT
	server_status = 1
	chat_status = 1
	client_username = ""

	# Get input from command line
	if len(sys.argv) != 2:
		sys.exit("incorrect command line format.\r\n" +
			 "usage: chatserve <port>")
	else:
		port = int(sys.argv[1])

	# Get hostname
	host = socket.gethostname()

	dbg_print("host = " + host)
	dbg_print("port = " + str(port))

	# Register signal handler for SIGINT
	signal.signal(signal.SIGINT, exitHandler)

	# Open TCP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# Set socket options
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

	# Bind socket to localhost
	sock.bind((host, port))

	# Listen for network traffic over the socket
	sock.listen(MAX_CONNECTION_REQUESTS)

	try:
		# Run chat server until user enters \quit
		while 1:
			print("Waiting for connection request...")

			# Accept connection requets
			conn, addr = sock.accept()

			dbg_print("Received connection request")

			# Receive connection request
			data = receive(conn)

			# Parse data received
			msg, client_username = data.split('[]', 1)

			print(SVR_USERNAME + "> Received \"" + msg + "\" from " + client_username)

			# Validate connecton request
			if (msg != "chatclient: connection request"):
				print("chatserve> error: connection refused")
			else:
				print("Connection established...")

				# Connection established. Send connection request confirmation.
				send(conn, "chatserve: connection success[]" + SVR_USERNAME)

				# Create send and receive threads
				tReceive = threading.Thread(target=receive, args=())
				tSend = threading.Thread(target=send, args())

				# Start send and receive threads
				tReceive.start()
				tSend.start()

				# Wait for send and receive thread to finish
				tReceive.join()
				tSend.join()
	except Exception as e:
		print("unexpected error: " + str(e));
	finally:
		dbg_print("Closing socket")
		sock.close()

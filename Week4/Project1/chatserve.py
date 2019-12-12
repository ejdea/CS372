###############################################################################
# Author:		Edmund Dea (deae@oregonstate.edu)
# Student ID:	933280343
# Date:			10/28/2019
# Title:		CS372 Project 1: Chat Server
# Description:	This program acts a chat server that sends and receives 
#               messages from a chat client using socket programming.
###############################################################################

import SocketServer

#------------------------------------------------------------------------------
# Class: TCPHandler
# Description: Handles sending and receiving data using a TCP connection
#------------------------------------------------------------------------------
class TCPHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		self.data = self.request.recv(4096).strip()
		print self.client_address[0] + "> " + self.data

#------------------------------------------------------------------------------
# Function: Main
# Description: Entrypoint for the application
#------------------------------------------------------------------------------
if __name__ == "__main__":
	HOST = "localhost"
	PORT = 32500
	
	server = SocketServer.TCPServer((HOST, PORT), TCPHandler)
	
	server.serve_forever()
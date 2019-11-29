###############################################################################
# Author:	 		Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	12/1/2019
# Course:			CS372
# Title:			README
# Description:		Instructions on how to compile and run ftserver and 
#					ftclient.
###############################################################################

Build Instructions:

1) Extract the following files into the same directory:

	ftserver.c
	ftclient.py
	makefile
	README.txt

2) Run the command "make". This will generate 1 binary called "ftserver".

3) Start the FTP server with the following cmd. Then, the FTP server will 
wait for the FTP client to initiate a connection request via the PORT.

	$ python ftclient.py [PORT]

4) Start the chat client with the following cmd. Then, enter a username and 
you will be prompted to send a message to the chat server.

	$ ./ftclient [target_hostname] [target_port]
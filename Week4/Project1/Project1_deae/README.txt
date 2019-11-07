###############################################################################
# Author:	 Edmund Dea (deae@oregonstate.edu)
# Student ID:	 933280343
# Last Modified: 10/28/2019
# Course:	 CS372
# Title:	 README
# Description:	 Instructions on how to compile and run chatserver and chatclient
###############################################################################

Build Instructions:

1) Extract the following files into the same directory:

	charclient.c
	chatserver.py
	makefile
	README.txt

2) Run the command "make". This will generate 1 binary called "chatclient".

3) Start the chat server with the following cmd. Then, the char server will 
wait for the chatclient to initiate a connection request.

	$ python chatserver.py [PORT]

4) Start the chat client with the following cmd. Then, enter a username and 
you will be prompted to send a message to the chat server.

	$ ./chatclient [target_hostname] [target_port]
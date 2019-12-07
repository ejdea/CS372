###############################################################################
# Author:	 	Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	12/1/2019
# Course:		CS372
# Title:		README
# Description:		Instructions on how to compile and run ftserver and 
#			ftclient.
###############################################################################

Build Instructions:

1) Extract the following files into the same directory:

	ftserver.c
	ftclient.py
	makefile
	README.txt

2) Run the command "make". This will generate 1 binary called "ftserver".

3) Start the FTP server with the following cmd. The FTP server will 
listen on the requested port number and wait for the FTP client to initiate a 
connection request.

    $ ./ftserver <port_number>
    
    Example: 
    $ ./ftserver 35121

4) Start the FTP client. To list directory, use the following cmd line format:

    $ python ftclient <server_hostname> <server_control_port> <server_data_port> -l
    
    Example:
    $ python python ftclient.py flip3.engr.oregonstate.edu 35121 35122 -l

To get a file, use the following cmd line format:

    $ python ftclient <server_hostname> <server_control_port> <server_data_port> -g <filename>
    
    Example:
    $ python ftclient.py flip3.engr.oregonstate.edu 35121 35122 -g README.txt

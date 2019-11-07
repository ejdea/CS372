###############################################################################
# Author:	 		Edmund Dea (deae@oregonstate.edu)
# Student ID:		933280343
# Last Modified:	10/28/2019
# Course:			CS372
# Title:			makefile
# Description:		Builds chatclient
###############################################################################

# Compiler
CC = gcc

# Source Files
SRC1 = chatclient.c

# Target name
TARGET1 = chatclient

# Compiler Flags
CFLAGS = -lpthread -Wall -pedantic -g

# Default compile command
default: clean all

all:
	@echo "CC	$(TARGET1)"
	@$(CC) $(SRC1) -o $(TARGET1) $(CFLAGS)

# Clean unnecessary files
clean:
	@echo "RM	$(TARGET1)"
	@rm -f $(TARGET1)

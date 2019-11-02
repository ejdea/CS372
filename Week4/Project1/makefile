###############################################################################
# Author:      Edmund Dea (deae@oregonstate.edu)
# Student ID:  933280343
# Date:        10/27/2019
# Title:       CS372 Project 1 makefile
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

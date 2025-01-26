##
# IMCShell
#
# @file
# @version 0.1

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output binary
TARGET = imcsh

# Source files
SRC = main.c

# Default target: build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Debug: Build and run the program with debugging
debug: $(TARGET)
	./$(TARGET)
	clear

# Clean: Remove the executable
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all debug clean

# end

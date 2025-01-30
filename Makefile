##
# IMCShell
#
# @file
# @version 0.1

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output binary
BLD-PATH = build/
BIN = imcsh
TARGET = $(BLD-PATH)$(BIN)

# Source files
SRC-PATH = src/
SOURCE-FILES = main.c
SRC = $(SRC-PATH)$(SOURCE-FILES)

# Default target: build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC) | $(BLD-PATH)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Ensure build directory exists
$(BLD-PATH):
	mkdir -p $(BLD-PATH)

# Debug: Build and run the program with debugging
debug: $(TARGET)
	./$(TARGET)

# Clean: Remove the executable and build directory
clean:
	rm -rf $(BLD-PATH)

# Phony targets
.PHONY: all debug clean

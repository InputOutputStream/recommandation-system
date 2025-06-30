# Directories
CLIENT_DIR = client
SERVER_DIR = server
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Source files

SERVER_SRCS = $(wildcard $(SERVER_DIR)/*.c)
CLIENT_SRCS = $(wildcard $(CLIENT_DIR)/*.c)

# Object files
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.c,$(OBJ_DIR)/server_%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.c,$(OBJ_DIR)/client_%.o,$(CLIENT_SRCS))

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -I$(INCLUDE_DIR) -lm -lndarray -lpthread

# Default target
all: directories client server models data

# Create directories
directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Compile object files
$(OBJ_DIR)/server_%.o: $(SERVER_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR)/client_%.o: $(CLIENT_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)


# Link binaries
server: $(SERVER_OBJS)
	$(CC) -o $(BIN_DIR)/server $^ $(CFLAGS)

client: $(CLIENT_OBJS)
	$(CC) -o $(BIN_DIR)/client $^ $(CFLAGS)

# Clean build
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all directories clean client server

CC = gcc
CFLAGS = -pthread

SERVER_SRC = src/server/*.c
CLIENT_SRC = src/client/*.c

BIN_DIR = bin

all:
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(BIN_DIR)/server
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(BIN_DIR)/client

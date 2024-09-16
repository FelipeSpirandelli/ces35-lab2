# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g
CLIBS = -lpthread -lrt

# Output executable names
CLIENT = client
SERVER = server

# Default target to build both client and server
all: $(CLIENT) $(SERVER)

# Rule to compile the client
$(CLIENT): client.o
	$(CC) $(CFLAGS) -o $(CLIENT) client.o $(CLIBS)

# Rule to compile the server
$(SERVER): server.o
	$(CC) $(CFLAGS) -o $(SERVER) server.o $(CLIBS)

# Rule to compile client.o
client.o: client.cpp
	$(CC) $(CFLAGS) -c client.cpp $(CLIBS)

# Rule to compile server.o
server.o: server.cpp
	$(CC) $(CFLAGS) -c server.cpp $(CLIBS)

# Target to run the client
run-client: $(CLIENT)
	./$(CLIENT)

# Target to run the server
run-server: $(SERVER)
	./$(SERVER)

# Clean up object files and executables
clean:
	rm -f *.o $(CLIENT) $(SERVER)

# PHONY targets are not actual files
.PHONY: all run-client run-server clean

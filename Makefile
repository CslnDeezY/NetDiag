CC = gcc
CFLAGS = -Wall -g

CLIENT_SRC = Client_NetDiag.c
SERVER_SRC = Server_NetDiag.c traceroute.c command.c 

EXEC_CLIENT = client
EXEC_SERVER = server

all: $(EXEC_CLIENT) $(EXEC_SERVER)

$(EXEC_CLIENT):
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(EXEC_CLIENT)

$(EXEC_SERVER):
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(EXEC_SERVER)

rebuild: clean all

clean:
	rm -f $(EXEC_CLIENT) $(EXEC_SERVER)
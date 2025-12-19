CC = gcc
CFLAGS = -Wall -pthread

# [수정] minigame.c 추가됨
SERVER_SRCS = server_main.c server_thread.c server_room.c server_log.c minigame.c
CLIENT_SRCS = client_main.c client_ui.c client_file.c client_cmd.c 

all: server client

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o server $(SERVER_SRCS)

client: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o client $(CLIENT_SRCS)

clean:
	rm -f server client server_log.txt
CC = gcc

all: client server proxy2 listener talker proxy3

client: client.c
	$(CC) -o client -Wall client.c

server: server.c
	$(CC) -o server -Wall server.c

proxy2: proxy2.c
	$(CC) -o proxy2 -Wall proxy2.c

listener: listener.c
	$(CC) -o listener -Wall listener.c

talker: talker.c
	$(CC) -o talker -Wall talker.c

proxy3: proxy3.c
	$(CC) -o proxy3 -Wall proxy3.c

clean:
	$(RM) client server proxy2 listener talker proxy3 -r
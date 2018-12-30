CC = clang

.PHONY: all
all: capsd client

capsd: capsd.c
	$(CC) -o capsd -I ../ capsd.c ../mig/capsdServer.c -framework CoreFoundation -l bsm

client: client.c
	$(CC) -o client -I ../ client.c

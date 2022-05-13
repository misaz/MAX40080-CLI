all:
	mkdir -p bin
	gcc -Wall -pedantic -O2 -g -o bin/max40080 \
		src/MAX40080_PlatformSpecific.c \
		src/MAX40080.c \
		src/CommandLineArguments.c \
		src/main.c \
		-li2c

install: all
	sudo cp bin/max40080 /usr/bin

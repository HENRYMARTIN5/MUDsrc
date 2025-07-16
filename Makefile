CC = gcc
CFLAGS = -I/usr/local/include -g -Wno-stringop-overflow
LDFLAGS = -ldaqhats -lpthread -lwiringPi -lm

all: MUD.c
	$(CC) -o mud MUD.c $(CFLAGS) $(LDFLAGS)
	$(CC) -o test.out MUD_BlueOrigin_Code_test.c $(CFLAGS) $(LDFLAGS)
	$(CC) -o mud_blue.2025 MUD_BlueOrigin_Code.c $(CFLAGS) $(LDFLAGS) -O3

clean:
	rm -f mud



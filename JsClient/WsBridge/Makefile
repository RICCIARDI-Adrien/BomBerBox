CC = gcc
CFLAGS = -std=c99 -D_POSIX_SOURCE -Wall -O2
LDFLAGS = -lpthread

INCLUDES = -Iinclude
BINARY = ws-bridge
SOURCES = $(BINARY).c cWebSockets.c sha1.c base64.c

ifneq ($(DEBUG),)
	CFLAGS += -DDEBUG
endif

all: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -o $(BINARY) $(LDFLAGS)

clean:
	rm -f $(BINARY)

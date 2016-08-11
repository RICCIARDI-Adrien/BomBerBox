CC = gcc
CCFLAGS = -W -Wall -Werror -O2
LDFLAGS = -lrt

INCLUDES_PATH = Includes
SOURCES_PATH = Sources

BINARY = bomberbox-server
INCLUDES = -I$(INCLUDES_PATH)
SOURCES = $(SOURCES_PATH)/Game.c $(SOURCES_PATH)/Main.c $(SOURCES_PATH)/Map.c $(SOURCES_PATH)/Network.c

all:
	$(CC) $(CCFLAGS) $(INCLUDES) $(SOURCES) -o $(BINARY) $(LDFLAGS)

clean:
	rm -f $(BINARY)

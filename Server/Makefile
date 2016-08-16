# Allow a cross-compiler to be used without any modification
CC ?= gcc
CCFLAGS += -W -Wall -Werror -O2

INCLUDES_PATH = Includes
SOURCES_PATH = Sources

BINARY = bomberbox-server
INCLUDES = -I$(INCLUDES_PATH)
LIBRARIES = -lrt
SOURCES = $(SOURCES_PATH)/Game.c $(SOURCES_PATH)/Main.c $(SOURCES_PATH)/Map.c $(SOURCES_PATH)/Network.c

all:
	$(CC) $(CCFLAGS) $(INCLUDES) $(SOURCES) $(LIBRARIES) -o $(BINARY)

clean:
	rm -f $(BINARY)

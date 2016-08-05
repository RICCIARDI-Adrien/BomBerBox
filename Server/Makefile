CC = gcc
CCFLAGS = -W -Wall -O2
# Enable at the development end
# -Werror

INCLUDES_PATH = Includes
SOURCES_PATH = Sources

BINARY = bomberbox-server
INCLUDES = -I$(INCLUDES_PATH)
SOURCES = $(SOURCES_PATH)/Main.c $(SOURCES_PATH)/Map.c $(SOURCES_PATH)/Network.c

all:
	$(CC) $(CCFLAGS) $(INCLUDES) $(SOURCES) -o $(BINARY)

clean:
	rm -f $(BINARY)
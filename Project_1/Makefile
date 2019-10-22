CC = gcc
DEBUG = -g
CFLAGS = -g -pthread -Wall -pedantic -std=gnu99 
TARGETS = station
# files with those names
# default target
.DEFAULT: all

.PHONY: all clean debug

all: $(TARGETS)

debug: CFLAGS += $(DEBUG)
debug: clean $(TARGETS)
    
# creates clubhub executable

station: station.c 
	$(CC) $(CFLAGS) -o station station.c

# creates the server executable

clean:
	rm -f $(TARGETS) *.o



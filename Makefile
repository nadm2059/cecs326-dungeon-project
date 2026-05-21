CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lrt -pthread

TARGETS = game barbarian wizard rogue

all: $(TARGETS)

# Add the Xlinker flag right here to bypass duplicate globals
game: game.o dungeon.o
	$(CC) game.o dungeon.o -o game $(LIBS) -Xlinker --allow-multiple-definition

game.o: game.c dungeon_info.h dungeon_settings.h
	$(CC) $(CFLAGS) -c game.c -o game.o

barbarian: barbarian.c dungeon_info.h
	$(CC) $(CFLAGS) barbarian.c -o barbarian $(LIBS)

wizard: wizard.c dungeon_info.h
	$(CC) $(CFLAGS) wizard.c -o wizard $(LIBS)

rogue: rogue.c dungeon_info.h
	$(CC) $(CFLAGS) rogue.c -o rogue $(LIBS)

clean:
	rm -f $(TARGETS) game.o

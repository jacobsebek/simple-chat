EXEC=./server
VPATH=src

SDL_CONFIG?=/usr/local/bin/sdl2-config

CFLAGS=-Wall -Wextra -std=c99 -pedantic -Iinclude -I../protlib/include `${SDL_CONFIG} --cflags`
LDFLAGS=-L../protlib
LDLIBS=-lprotlib -lSDL2_net `$(SDL_CONFIG) --libs` 

OBJECTS=$(patsubst %.c, %.o, $(notdir $(wildcard $(VPATH)/*.c)))

$(EXEC) : $(OBJECTS)
	${CC} -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

%.o : include/*.h

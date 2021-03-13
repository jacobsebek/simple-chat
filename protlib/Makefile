OUT=./libprotlib.a
VPATH=src

SDL_CONFIG?=/usr/local/bin/sdl2-config

CFLAGS=-Wall -Wextra -std=c99 -pedantic -Iinclude `${SDL_CONFIG} --cflags`
LDLIBS=-lSDL2_net `$(SDL_CONFIG) --libs` 

OBJECTS=$(patsubst %.c, %.o, $(notdir $(wildcard $(VPATH)/*.c)))

$(OUT) : $(OBJECTS)
	$(AR) rvs $@ $^

%.o : include/*.h

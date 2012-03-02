# Makefile for Project 3
# <andrus@uchicago.edu>
CC = gcc
CFLAGS=-Wall\
			 -O2\
			 -g
LFLAGS=

# Maclab stuff
GLFW_DIR_MACLAB=/opt/gfx
IS_MACLAB=$(wildcard $(GLFW_DIR_MACLAB))

# Output binary name, source files, headers, and libraries
BIN=proj3
SRC=$(shell ls *.c)
HEADERS=$(shell ls *.h)
OBJS=$(SRC:.c=.o)
LIBS=libglfw

# Add the appropriate flags for each of our libraries using pkg-config
ifeq ($(strip $(IS_MACLAB)), $(GLFW_DIR_MACLAB))
CFLAGS+=-I$(GLFW_DIR_MACLAB)/include `pkg-config --cflags $(LIBS)`
LFLAGS+=-L$(GLFW_DIR_MACLAB)/lib `pkg-config --libs $(LIBS)` ../lib/libAntTweakBar.dylib\
				../lib/libpng15.a
else
LIBS+=libpng
CFLAGS+=`pkg-config --cflags $(LIBS)` 
LFLAGS+=`pkg-config --libs $(LIBS)` /usr/local/lib/libAntTweakBar.dylib
endif

# Compilation rules
# %.o:$(SRC) $(HEADERS)
%.o:%.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Linking rules
$(BIN):$(OBJS)
	$(CC) $^ $(LFLAGS) -o $@

# Phonies
.PHONY: all clean run
all:$(BIN)
clean:
	rm -f $(BIN).dSYM $(BIN) $(OBJS)
run:$(BIN)
	./$(BIN)

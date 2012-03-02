
##
## demo Makefile for Project 3
##

# Compiler settings
CC     ?= cc

GLFW_DIR_MACLAB = /opt/gfx
IS_MACLAB = $(wildcard $(GLFW_DIR_MACLAB))

ifeq ($(strip $(IS_MACLAB)), $(GLFW_DIR_MACLAB)) 
CFLAGS ?= -Wall -O2 -g -I/opt/gfx/include -I../include
LFLAGS = -L/opt/gfx/lib -lglfw -framework Cocoa -framework OpenGL -lz
else 
CFLAGS ?= -Wall -O2 -g -I../include
LFLAGS = -L/usr/local/lib -lglfw -framework Cocoa -framework OpenGL -lz
endif 

OBJS = proj3.o spotGeomShapes.o spotGeomMethods.o spotImage.o spotUtils.o callbacks.o matrixFunctions.o

all: proj3 $(OBJS)

%.o: %.c spot.h spotMacros.h callbacks.h matrixFunctions.h types.h
	$(CC) $(CFLAGS) -c -o $@ $< 

proj3: $(OBJS) spot.h spotMacros.h callbacks.h matrixFunctions.h types.h
	$(CC) $(OBJS) $(LFLAGS) ../lib/libAntTweakBar.dylib ../lib/libpng15.a -o proj3

clean:
	rm -rf proj3 proj3.dSYM $(OBJS) 

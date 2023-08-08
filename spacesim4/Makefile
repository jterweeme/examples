# Makefile by Panteleakis Ioannis
# mail: pioann@csd.auth.gr

MAINLIB = -lglut

SRC = tutorial4.cpp

OBJ = $(SRC:.c=.o)

OPT = -O3
CC = gcc $(OPT) -Wall

# You might have to change GLU to MesaGLU depending on your Mesa version
GLLIBS = $(MAINLIB) -lGL -lGLU
LIBS = $(GLLIBS) -lm

all: tutorial4

.c.o:
	$(CC) -c $<

tutorial4:
	g++ -o tutorial4 tutorial4.cpp $(LIBS)

clean:
	rm -f tutorial4 *.o

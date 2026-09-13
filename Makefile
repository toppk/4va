# Makefile for 4va.
# Please send questions or bugs to mdw1@crux2.cit.cornell.edu

CC = cc
CLIBS = -lm -lX11
CFLAGS = -O 


all: 4va ctorus cutctorus 4vdmake

4va: 4vamain.o 4vaproj.o 4vartn.o 4vad_x.o
	$(CC) -o 4va 4vamain.o 4vaproj.o 4vartn.o 4vad_x.o $(CLIBS)

ctorus: ctorus.o
	$(CC) -o ctorus ctorus.o $(CLIBS)

cutctorus: cutctorus.o
	$(CC) -o cutctorus cutctorus.o $(CLIBS)

4vdmake: 4vdmake.o
	$(CC) -o 4vdmake 4vdmake.o $(CLIBS)

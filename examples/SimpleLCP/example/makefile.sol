
PATHDIR = ../../..
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/solaris/static

CC    = cc
CFLAGS = -I. -I.. -I$(PATHINC) -xO5 -dalign 

all: 
	$(CC) $(CFLAGS) -o main main.c ../SimpleLCP_Path.o -L$(PATHLIB) -lpath46 -lm


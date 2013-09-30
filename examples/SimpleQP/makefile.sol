
PATHDIR = ../..
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/solaris

CC    = cc
CFLAGS = -I. -I$(PATHINC) -xO5 -dalign 

all: 
	$(CC) $(CFLAGS) -c SimpleQP_Path.c


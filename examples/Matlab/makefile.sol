
PATHDIR = ../..
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/solaris/static

CC    = mex
CFLAGS = -I. -I$(PATHINC) -O

all: 
	$(CC) $(CFLAGS) lcppath.c Matlab_Error.c Matlab_Memory.c -L$(PATHLIB) -lpath46
	$(CC) $(CFLAGS) mcppath.c Matlab_Error.c Matlab_Memory.c -L$(PATHLIB) -lpath46


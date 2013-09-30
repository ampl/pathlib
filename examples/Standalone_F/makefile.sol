
PATHDIR = ../..
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/solaris

CC    = cc
CFLAGS = -I. -I$(PATHINC) -DFNAME_LCASE_DECOR -xO5 -dalign 

F77   = f77
FFLAGE = -xO5 -dalign

all: 
	$(CC) $(CFLAGS) -c Standalone_Path.c StandaloneF_Output.c
	$(F77) $(FFLAGS) -c fout.f


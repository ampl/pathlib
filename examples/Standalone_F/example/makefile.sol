
PATHDIR = ../../..
PATHINC = $(PATHDIR)/include
PATHLIB = $(PATHDIR)/lib/solaris/dynamic

F77    = f77
CFLAGS = -xO5 -dalign

all: 
	$(F77) $(FFLAGS) -o linmcp linmcp.f ../Standalone_Path.o ../StandaloneF_Output.o ../fout.o -L$(PATHLIB) -lpath46 -lm


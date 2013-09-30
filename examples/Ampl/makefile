
COMMONDIR = ../..

include $(COMMONDIR)/makefile.rules

ALG	=
INC     =
LIB	=
EXE	=

NEOS_FLAGS  = -DNEOS -DLOGFILE="\"$(LNAME).log\"" 
NEOS_FLAGS +=        -DSTATUSFILE="\"$(LNAME).stat\""

CFLAGS += -I$(ROOTDIR)/algorithms/$(INC)/include -I$(AMPL_DIR)/solvers $(AMPL_CFLAGS)

LIBS    = $(LSTATIC) $(AMPL_DIR)/solvers/amplsolver.a -l$(LIB) $(LLIBS)

EXENAME = path.ampl

SRCS =  Solver_Path.c
OBJS =  Solver_Path.o

all: $(EXENAME)

version.c: version.c0
	sed "s/Machine_details/`uname -sr`/" version.c0 >version.c

$(EXENAME): $(OBJS) $(LCHKS)
	$(LD) $(LFLAGS) -o $(EXENAME) $(OBJS) $(LIBS)

clean:
	/bin/rm -f *.o *.a *.so *~ version.c *.ampl


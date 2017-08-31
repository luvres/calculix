CFLAGS = -Wall -O3  -I /usr/include/spooles -DARCH="Linux" -DSPOOLES -DARPACK -DMATRIXSTORAGE
FFLAGS = -Wall -O3 -fopenmp

CC=cc
FC=gfortran

.c.o :
	$(CC) $(CFLAGS) -c $<
.f.o :
	$(FC) $(FFLAGS) -c $<

include Makefile.inc

SCCXMAIN = ccx_2.12.c

OCCXF = $(SCCXF:.f=.o)
OCCXC = $(SCCXC:.c=.o)
OCCXMAIN = $(SCCXMAIN:.c=.o)

LIBS = -lspooles -larpack -lm -lpthread -lblas -lc -lpthread 

ccx_2.12: $(OCCXMAIN) ccx_2.12.a  $(LIBS)
	./date.pl; $(CC) $(CFLAGS) -c ccx_2.12.c; $(FC) -fopenmp -Wall -O3 -o $@ $(OCCXMAIN) ccx_2.12.a $(LIBS)

ccx_2.12.a: $(OCCXF) $(OCCXC)
	ar vr $@ $?
                                                                               

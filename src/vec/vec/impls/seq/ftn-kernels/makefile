
# This directory contains some computational kernels written in
# Fortran77.  These are mainly intended to provide high performance for
# COMPLEX arithmetic (since most C++ compilers do not do a good job
# optimizing complex number calculations), but can also be used to
# compare against C versions of the same kernels.

ALL: lib
#requirespackage   'PETSC_USE_FORTRAN_KERNELS'

CFLAGS   =
FFLAGS   =
SOURCEC  = 
SOURCEF  = fwaxpy.F faypx.F fnorm.F fxtimesy.F fmdot.F fmaxpy.F
SOURCEH  = 
OBJSC    = 
OBJSF    = fwaxpy.o faypx.o fnorm.o fxtimesy.o fmdot.o fmaxpy.o
LIBBASE  = libpetscvec
MANSEC   = Sys
LOCDIR   = src/vec/vec/impls/seq/ftn-kernels/

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules
include ${PETSC_DIR}/conf/test




#requirespackage   'PETSC_HAVE_FFTW'
#requiresprecision double

ALL: lib

# Set CFLAGS="-DPETSC_DEBUG_USFFT" to turn on debugging output
CFLAGS   = 
FFLAGS   =
SOURCEC  = matusfft.c
OBJSC    = matusfft.o
LIBBASE  = libpetscdm
MANSEC   = DA
DIRS     =
LOCDIR   = src/dm/da/src/usfft/

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules
include ${PETSC_DIR}/conf/test

#requirespackage   'PETSC_USING_F90'
#
#   Makes Fortran module functions from *.h90 files in include/finclude
#
ALL: buildmod
speciallib: buildmod
specialfastlib: buildmodfast
SPECIALLIB     = yes
SPECIALFASTLIB = yes


CFLAGS	   = 
FFLAGS	   = 
SOURCEC	   = 
SOURCEF	   = petscsysmod.F
SOURCEH	   = 
OBJSC	   = 
OBJSF	   = petscsysmod.o
LIBBASE	   = libpetscsys
MANSEC	   = Sys
LOCDIR	   = src/sys/f90-mod/
CLEANFILES = *.mod

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules
include ${PETSC_DIR}/conf/test

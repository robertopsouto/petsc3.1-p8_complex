#include "private/fortranimpl.h"
#include "petscts.h"

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define tscreate_                            TSCREATE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define tscreate_                            tscreate
#endif

EXTERN_C_BEGIN
void PETSC_STDCALL tscreate_(MPI_Comm *comm,TS *outts,PetscErrorCode *ierr)
{
  *ierr = TSCreate(MPI_Comm_f2c(*(MPI_Fint *)&*comm),outts);
}

EXTERN_C_END

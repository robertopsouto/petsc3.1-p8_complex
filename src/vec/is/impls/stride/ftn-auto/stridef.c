#include "petscsys.h"
#include "petscfix.h"
/* stride.c */
/* Fortran interface file */

/*
* This file was generated automatically by bfort from the C source
* file.  
 */

#ifdef PETSC_USE_POINTER_CONVERSION
#if defined(__cplusplus)
extern "C" { 
#endif 
extern void *PetscToPointer(void*);
extern int PetscFromPointer(void *);
extern void PetscRmPointer(void*);
#if defined(__cplusplus)
} 
#endif 

#else

#define PetscToPointer(a) (*(long *)(a))
#define PetscFromPointer(a) (long)(a)
#define PetscRmPointer(a)
#endif

#include "petscis.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define isstridegetinfo_ ISSTRIDEGETINFO
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define isstridegetinfo_ isstridegetinfo
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define isstride_ ISSTRIDE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define isstride_ isstride
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define iscreatestride_ ISCREATESTRIDE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define iscreatestride_ iscreatestride
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   isstridegetinfo_(IS is,PetscInt *first,PetscInt *step, int *__ierr ){
*__ierr = ISStrideGetInfo(
	(IS)PetscToPointer((is) ),first,step);
}
void PETSC_STDCALL   isstride_(IS is,PetscTruth *flag, int *__ierr ){
*__ierr = ISStride(
	(IS)PetscToPointer((is) ),flag);
}
void PETSC_STDCALL   iscreatestride_(MPI_Fint * comm,PetscInt *n,PetscInt *first,PetscInt *step,IS *is, int *__ierr ){
*__ierr = ISCreateStride(
	MPI_Comm_f2c( *(comm) ),*n,*first,*step,is);
}
#if defined(__cplusplus)
}
#endif

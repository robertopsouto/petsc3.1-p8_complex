#include "petscsys.h"
#include "petscfix.h"
/* gmpre.c */
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

#include "petscksp.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define kspgmressetpreallocatevectors_ KSPGMRESSETPREALLOCATEVECTORS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define kspgmressetpreallocatevectors_ kspgmressetpreallocatevectors
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   kspgmressetpreallocatevectors_(KSP ksp, int *__ierr ){
*__ierr = KSPGMRESSetPreAllocateVectors(
	(KSP)PetscToPointer((ksp) ));
}
#if defined(__cplusplus)
}
#endif

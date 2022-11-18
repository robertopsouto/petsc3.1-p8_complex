#include "petscsys.h"
#include "petscfix.h"
/* mumps.c */
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

#include "petscmat.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmumpsseticntl_ MATMUMPSSETICNTL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmumpsseticntl_ matmumpsseticntl
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
#undef __FUNCT__ 
#define __FUNCT__ ""
void PETSC_STDCALL  matmumpsseticntl_(Mat F,PetscInt *idx,PetscInt *icntl, int *__ierr ){
*__ierr = MatMumpsSetIcntl(
	(Mat)PetscToPointer((F) ),*idx,*icntl);
}
#if defined(__cplusplus)
}
#endif

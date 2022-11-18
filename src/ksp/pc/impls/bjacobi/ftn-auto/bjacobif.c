#include "petscsys.h"
#include "petscfix.h"
/* bjacobi.c */
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

#include "petscpc.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcbjacobisetusetruelocal_ PCBJACOBISETUSETRUELOCAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcbjacobisetusetruelocal_ pcbjacobisetusetruelocal
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcbjacobisettotalblocks_ PCBJACOBISETTOTALBLOCKS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcbjacobisettotalblocks_ pcbjacobisettotalblocks
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcbjacobisetlocalblocks_ PCBJACOBISETLOCALBLOCKS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcbjacobisetlocalblocks_ pcbjacobisetlocalblocks
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   pcbjacobisetusetruelocal_(PC pc, int *__ierr ){
*__ierr = PCBJacobiSetUseTrueLocal(
	(PC)PetscToPointer((pc) ));
}
void PETSC_STDCALL   pcbjacobisettotalblocks_(PC pc,PetscInt *blocks, PetscInt lens[], int *__ierr ){
*__ierr = PCBJacobiSetTotalBlocks(
	(PC)PetscToPointer((pc) ),*blocks,lens);
}
void PETSC_STDCALL   pcbjacobisetlocalblocks_(PC pc,PetscInt *blocks, PetscInt lens[], int *__ierr ){
*__ierr = PCBJacobiSetLocalBlocks(
	(PC)PetscToPointer((pc) ),*blocks,lens);
}
#if defined(__cplusplus)
}
#endif

#include "petscsys.h"
#include "petscfix.h"
/* drawv.c */
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

#include "petscdraw.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerdrawclear_ PETSCVIEWERDRAWCLEAR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerdrawclear_ petscviewerdrawclear
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   petscviewerdrawclear_(PetscViewer viewer, int *__ierr ){
*__ierr = PetscViewerDrawClear(
	(PetscViewer)PetscToPointer((viewer) ));
}
#if defined(__cplusplus)
}
#endif

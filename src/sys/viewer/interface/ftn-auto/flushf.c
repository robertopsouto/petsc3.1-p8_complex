#include "petscsys.h"
#include "petscfix.h"
/* flush.c */
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

#include "petscviewer.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerflush_ PETSCVIEWERFLUSH
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerflush_ petscviewerflush
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   petscviewerflush_(PetscViewer viewer, int *__ierr ){
*__ierr = PetscViewerFlush(
	(PetscViewer)PetscToPointer((viewer) ));
}
#if defined(__cplusplus)
}
#endif

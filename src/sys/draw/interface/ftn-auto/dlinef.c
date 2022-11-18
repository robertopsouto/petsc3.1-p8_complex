#include "petscsys.h"
#include "petscfix.h"
/* dline.c */
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
#define petscdrawline_ PETSCDRAWLINE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscdrawline_ petscdrawline
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscdrawisnull_ PETSCDRAWISNULL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscdrawisnull_ petscdrawisnull
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   petscdrawline_(PetscDraw draw,PetscReal *xl,PetscReal *yl,PetscReal *xr,PetscReal *yr,int *cl, int *__ierr ){
*__ierr = PetscDrawLine(
	(PetscDraw)PetscToPointer((draw) ),*xl,*yl,*xr,*yr,*cl);
}
void PETSC_STDCALL   petscdrawisnull_(PetscDraw draw,PetscTruth *yes, int *__ierr ){
*__ierr = PetscDrawIsNull(
	(PetscDraw)PetscToPointer((draw) ),yes);
}
#if defined(__cplusplus)
}
#endif

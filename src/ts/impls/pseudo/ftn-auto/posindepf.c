#include "petscsys.h"
#include "petscfix.h"
/* posindep.c */
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

#include "petscts.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tspseudocomputetimestep_ TSPSEUDOCOMPUTETIMESTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tspseudocomputetimestep_ tspseudocomputetimestep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tspseudoverifytimestep_ TSPSEUDOVERIFYTIMESTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tspseudoverifytimestep_ tspseudoverifytimestep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tspseudosettimestepincrement_ TSPSEUDOSETTIMESTEPINCREMENT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tspseudosettimestepincrement_ tspseudosettimestepincrement
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tspseudoincrementdtfrominitialdt_ TSPSEUDOINCREMENTDTFROMINITIALDT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tspseudoincrementdtfrominitialdt_ tspseudoincrementdtfrominitialdt
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   tspseudocomputetimestep_(TS ts,PetscReal *dt, int *__ierr ){
*__ierr = TSPseudoComputeTimeStep(
	(TS)PetscToPointer((ts) ),dt);
}
void PETSC_STDCALL   tspseudoverifytimestep_(TS ts,Vec update,PetscReal *dt,PetscTruth *flag, int *__ierr ){
*__ierr = TSPseudoVerifyTimeStep(
	(TS)PetscToPointer((ts) ),
	(Vec)PetscToPointer((update) ),dt,flag);
}
void PETSC_STDCALL   tspseudosettimestepincrement_(TS ts,PetscReal *inc, int *__ierr ){
*__ierr = TSPseudoSetTimeStepIncrement(
	(TS)PetscToPointer((ts) ),*inc);
}
void PETSC_STDCALL   tspseudoincrementdtfrominitialdt_(TS ts, int *__ierr ){
*__ierr = TSPseudoIncrementDtFromInitialDt(
	(TS)PetscToPointer((ts) ));
}
#if defined(__cplusplus)
}
#endif

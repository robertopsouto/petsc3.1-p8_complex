#include "petscsys.h"
#include "petscfix.h"
/* fieldsplit.c */
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
#define pcfieldsplitsetfields_ PCFIELDSPLITSETFIELDS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcfieldsplitsetfields_ pcfieldsplitsetfields
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcfieldsplitsetis_ PCFIELDSPLITSETIS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcfieldsplitsetis_ pcfieldsplitsetis
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcfieldsplitsetblocksize_ PCFIELDSPLITSETBLOCKSIZE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcfieldsplitsetblocksize_ pcfieldsplitsetblocksize
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcfieldsplitschurprecondition_ PCFIELDSPLITSCHURPRECONDITION
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcfieldsplitschurprecondition_ pcfieldsplitschurprecondition
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define pcfieldsplitsettype_ PCFIELDSPLITSETTYPE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define pcfieldsplitsettype_ pcfieldsplitsettype
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   pcfieldsplitsetfields_(PC pc,PetscInt *n,PetscInt *fields, int *__ierr ){
*__ierr = PCFieldSplitSetFields(
	(PC)PetscToPointer((pc) ),*n,fields);
}
void PETSC_STDCALL   pcfieldsplitsetis_(PC pc,IS is, int *__ierr ){
*__ierr = PCFieldSplitSetIS(
	(PC)PetscToPointer((pc) ),
	(IS)PetscToPointer((is) ));
}
void PETSC_STDCALL   pcfieldsplitsetblocksize_(PC pc,PetscInt *bs, int *__ierr ){
*__ierr = PCFieldSplitSetBlockSize(
	(PC)PetscToPointer((pc) ),*bs);
}
void PETSC_STDCALL   pcfieldsplitschurprecondition_(PC pc,PCFieldSplitSchurPreType *ptype,Mat pre, int *__ierr ){
*__ierr = PCFieldSplitSchurPrecondition(
	(PC)PetscToPointer((pc) ),*ptype,
	(Mat)PetscToPointer((pre) ));
}
void PETSC_STDCALL   pcfieldsplitsettype_(PC pc,PCCompositeType *type, int *__ierr ){
*__ierr = PCFieldSplitSetType(
	(PC)PetscToPointer((pc) ),*type);
}
#if defined(__cplusplus)
}
#endif

#include "petscsys.h"
#include "petscfix.h"
/* multequal.c */
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
#define matmultequal_ MATMULTEQUAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmultequal_ matmultequal
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmultaddequal_ MATMULTADDEQUAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmultaddequal_ matmultaddequal
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmulttransposeequal_ MATMULTTRANSPOSEEQUAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmulttransposeequal_ matmulttransposeequal
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmulttransposeaddequal_ MATMULTTRANSPOSEADDEQUAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmulttransposeaddequal_ matmulttransposeaddequal
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   matmultequal_(Mat A,Mat B,PetscInt *n,PetscTruth *flg, int *__ierr ){
*__ierr = MatMultEqual(
	(Mat)PetscToPointer((A) ),
	(Mat)PetscToPointer((B) ),*n,flg);
}
void PETSC_STDCALL   matmultaddequal_(Mat A,Mat B,PetscInt *n,PetscTruth *flg, int *__ierr ){
*__ierr = MatMultAddEqual(
	(Mat)PetscToPointer((A) ),
	(Mat)PetscToPointer((B) ),*n,flg);
}
void PETSC_STDCALL   matmulttransposeequal_(Mat A,Mat B,PetscInt *n,PetscTruth *flg, int *__ierr ){
*__ierr = MatMultTransposeEqual(
	(Mat)PetscToPointer((A) ),
	(Mat)PetscToPointer((B) ),*n,flg);
}
void PETSC_STDCALL   matmulttransposeaddequal_(Mat A,Mat B,PetscInt *n,PetscTruth *flg, int *__ierr ){
*__ierr = MatMultTransposeAddEqual(
	(Mat)PetscToPointer((A) ),
	(Mat)PetscToPointer((B) ),*n,flg);
}
#if defined(__cplusplus)
}
#endif

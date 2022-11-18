#include "petscsys.h"
#include "petscfix.h"
/* pbvec.c */
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

#include "petscvec.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define vecghostgetlocalform_ VECGHOSTGETLOCALFORM
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define vecghostgetlocalform_ vecghostgetlocalform
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define vecghostrestorelocalform_ VECGHOSTRESTORELOCALFORM
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define vecghostrestorelocalform_ vecghostrestorelocalform
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define vecghostupdatebegin_ VECGHOSTUPDATEBEGIN
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define vecghostupdatebegin_ vecghostupdatebegin
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define vecghostupdateend_ VECGHOSTUPDATEEND
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define vecghostupdateend_ vecghostupdateend
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define veccreateghost_ VECCREATEGHOST
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define veccreateghost_ veccreateghost
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define veccreateghostblock_ VECCREATEGHOSTBLOCK
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define veccreateghostblock_ veccreateghostblock
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   vecghostgetlocalform_(Vec g,Vec *l, int *__ierr ){
*__ierr = VecGhostGetLocalForm(
	(Vec)PetscToPointer((g) ),l);
}
void PETSC_STDCALL   vecghostrestorelocalform_(Vec g,Vec *l, int *__ierr ){
*__ierr = VecGhostRestoreLocalForm(
	(Vec)PetscToPointer((g) ),l);
}
void PETSC_STDCALL   vecghostupdatebegin_(Vec g,InsertMode *insertmode,ScatterMode *scattermode, int *__ierr ){
*__ierr = VecGhostUpdateBegin(
	(Vec)PetscToPointer((g) ),*insertmode,*scattermode);
}
void PETSC_STDCALL   vecghostupdateend_(Vec g,InsertMode *insertmode,ScatterMode *scattermode, int *__ierr ){
*__ierr = VecGhostUpdateEnd(
	(Vec)PetscToPointer((g) ),*insertmode,*scattermode);
}
void PETSC_STDCALL   veccreateghost_(MPI_Fint * comm,PetscInt *n,PetscInt *N,PetscInt *nghost, PetscInt ghosts[],Vec *vv, int *__ierr ){
*__ierr = VecCreateGhost(
	MPI_Comm_f2c( *(comm) ),*n,*N,*nghost,ghosts,vv);
}
void PETSC_STDCALL   veccreateghostblock_(MPI_Fint * comm,PetscInt *bs,PetscInt *n,PetscInt *N,PetscInt *nghost, PetscInt ghosts[],Vec *vv, int *__ierr ){
*__ierr = VecCreateGhostBlock(
	MPI_Comm_f2c( *(comm) ),*bs,*n,*N,*nghost,ghosts,vv);
}
#if defined(__cplusplus)
}
#endif

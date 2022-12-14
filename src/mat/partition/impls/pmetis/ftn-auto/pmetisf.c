#include "petscsys.h"
#include "petscfix.h"
/* pmetis.c */
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
#define matpartitioningparmetissetcoarsesequential_ MATPARTITIONINGPARMETISSETCOARSESEQUENTIAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matpartitioningparmetissetcoarsesequential_ matpartitioningparmetissetcoarsesequential
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matpartitioningparmetisgetedgecut_ MATPARTITIONINGPARMETISGETEDGECUT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matpartitioningparmetisgetedgecut_ matpartitioningparmetisgetedgecut
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmeshtovertexgraph_ MATMESHTOVERTEXGRAPH
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmeshtovertexgraph_ matmeshtovertexgraph
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define matmeshtocellgraph_ MATMESHTOCELLGRAPH
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define matmeshtocellgraph_ matmeshtocellgraph
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
void PETSC_STDCALL   matpartitioningparmetissetcoarsesequential_(MatPartitioning part, int *__ierr ){
*__ierr = MatPartitioningParmetisSetCoarseSequential(
	(MatPartitioning)PetscToPointer((part) ));
}
void PETSC_STDCALL   matpartitioningparmetisgetedgecut_(MatPartitioning part,PetscInt *cut, int *__ierr ){
*__ierr = MatPartitioningParmetisGetEdgeCut(
	(MatPartitioning)PetscToPointer((part) ),cut);
}
void PETSC_STDCALL  matmeshtovertexgraph_(Mat mesh,PetscInt *ncommonnodes,Mat *dual, int *__ierr ){
*__ierr = MatMeshToVertexGraph(
	(Mat)PetscToPointer((mesh) ),*ncommonnodes,dual);
}
void PETSC_STDCALL  matmeshtocellgraph_(Mat mesh,PetscInt *ncommonnodes,Mat *dual, int *__ierr ){
*__ierr = MatMeshToCellGraph(
	(Mat)PetscToPointer((mesh) ),*ncommonnodes,dual);
}
#if defined(__cplusplus)
}
#endif

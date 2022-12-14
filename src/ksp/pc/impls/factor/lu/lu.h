/* 
   Private data structure for LU preconditioner.
*/
#if !defined(__LU_H)
#define __LU_H

#include "../src/ksp/pc/impls/factor/factor.h"

typedef struct {
  PC_Factor   hdr;
  PetscReal   actualfill;       /* actual fill in factor */
  PetscTruth  inplace;          /* flag indicating in-place factorization */
  IS          row,col;          /* index sets used for reordering */
  PetscTruth  reuseordering;    /* reuses previous reordering computed */
  PetscTruth  reusefill;        /* reuse fill from previous LU */
  PetscTruth  nonzerosalongdiagonal;
  PetscReal   nonzerosalongdiagonaltol;
} PC_LU;

#endif

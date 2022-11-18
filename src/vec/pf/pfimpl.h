
#ifndef _PFIMPL
#define _PFIMPL

#include "petscpf.h"

typedef struct _PFOps *PFOps;
struct _PFOps {
  PetscErrorCode (*apply)(void*,PetscInt,PetscScalar*,PetscScalar*);
  PetscErrorCode (*applyvec)(void*,Vec,Vec);
  PetscErrorCode (*destroy)(void*);
  PetscErrorCode (*view)(void*,PetscViewer);
  PetscErrorCode (*setfromoptions)(PF);
};

struct _p_PF {
  PETSCHEADER(struct _PFOps);
  PetscInt dimin,dimout;             /* dimension of input and output spaces */
  void     *data;
};

#endif

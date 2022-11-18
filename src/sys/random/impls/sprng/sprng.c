#define PETSC_DLL

#include "../src/sys/random/randomimpl.h"
#if defined (PETSC_HAVE_STDLIB_H)
#include <stdlib.h>
#endif

#define USE_MPI
#define SIMPLE_SPRNG
EXTERN_C_BEGIN
#include "sprng.h"
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PetscRandomSeed_Sprng"
PetscErrorCode PETSC_DLLEXPORT PetscRandomSeed_Sprng(PetscRandom r)
{
  PetscFunctionBegin;
  init_sprng(r->seed,SPRNG_DEFAULT);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscRandomGetValue_Sprng"
PetscErrorCode PETSC_DLLEXPORT PetscRandomGetValue_Sprng(PetscRandom r,PetscScalar *val)
{
  PetscFunctionBegin;
#if defined(PETSC_USE_COMPLEX)  
  if (r->iset) {
    *val = PetscRealPart(r->width)*sprng() + PetscRealPart(r->low) +
      (PetscImaginaryPart(r->width)*sprng() + PetscImaginaryPart(r->low)) * PETSC_i;
  } else {
    *val = sprng() + sprng()*PETSC_i;
  } 
#else
  if (r->iset) *val = r->width * sprng() + r->low;
  else         *val = sprng();
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscRandomGetValue_Sprng"
PetscErrorCode PETSC_DLLEXPORT PetscRandomGetValueReal_Sprng(PetscRandom r,PetscScalar *val)
{
  PetscFunctionBegin;
#if defined(PETSC_USE_COMPLEX)
  if (r->iset) *val = PetscRealPart(r->width)*sprng() + PetscRealPart(r->low);
  else         *val = sprng();
#else
  if (r->iset) *val = r->width * sprng() + r->low;
  else         *val = sprng();
#endif
  PetscFunctionReturn(0);
}

static struct _PetscRandomOps PetscRandomOps_Values = {
  /* 0 */
  PetscRandomSeed_Sprng,
  PetscRandomGetValue_Sprng,
  PetscRandomGetValueReal_Sprng,
  0,
  /* 5 */
  0
};

/*MC
   PETSCSPRNG- access to the publically available random number generator sprng

   Options Database Keys:
. -random_type <rand,rand48,sprng> 

  Level: beginner

   PETSc must have been config/configure.py with the option --download-sprng to use
   this random number generator.

   This is NOT currently using a parallel random number generator. Sprng does have
   an MPI version we should investigate.

.seealso: RandomCreate(), RandomSetType(), PETSCRAND, PETSCRAND48
M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscRandomCreate_Sprng" 
PetscErrorCode PETSC_DLLEXPORT PetscRandomCreate_Sprng(PetscRandom r)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMemcpy(r->ops,&PetscRandomOps_Values,sizeof(PetscRandomOps_Values));CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)r,PETSCSPRNG);CHKERRQ(ierr);
  ierr = PetscPublishAll(r);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

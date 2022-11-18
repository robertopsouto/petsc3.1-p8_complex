#define PETSC_DLL

#include "petscsys.h"        /*I    "petscsys.h"   I*/

#undef __FUNCT__  
#define __FUNCT__ "PetscObjectGetName"
/*@C
   PetscObjectGetName - Gets a string name associated with a PETSc object.

   Not Collective

   Input Parameters:
+  obj - the Petsc variable
         Thus must be cast with a (PetscObject), for example, 
         PetscObjectGetName((PetscObject)mat,&name);
-  name - the name associated with obj

   Level: intermediate

   Concepts: object name

.seealso: PetscObjectSetName()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscObjectGetName(PetscObject obj,const char *name[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidPointer(name,2);
  if (!obj->name) { ierr = PetscObjectName(obj);CHKERRQ(ierr); }
  *name = obj->name;
  PetscFunctionReturn(0);
}


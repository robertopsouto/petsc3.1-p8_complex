#define PETSCTS_DLL

#include "private/tsimpl.h"

static PetscTruth TSPackageInitialized = PETSC_FALSE;
#undef __FUNCT__  
#define __FUNCT__ "TSFinalizePackage"
/*@C
  TSFinalizePackage - This function destroys everything in the Petsc interface to Mathematica. It is
  called from PetscFinalize().

  Level: developer

.keywords: Petsc, destroy, package, mathematica
.seealso: PetscFinalize()
@*/
PetscErrorCode PETSC_DLLEXPORT TSFinalizePackage(void) 
{
  PetscFunctionBegin;
  TSPackageInitialized = PETSC_FALSE;
  TSList               = PETSC_NULL;
  TSRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "TSInitializePackage"
/*@C
  TSInitializePackage - This function initializes everything in the TS package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to TSCreate()
  when using static libraries.

  Input Parameter:
  path - The dynamic library path, or PETSC_NULL

  Level: developer

.keywords: TS, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PETSCTS_DLLEXPORT TSInitializePackage(const char path[]) 
{
  char              logList[256];
  char              *className;
  PetscTruth        opt;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  if (TSPackageInitialized) PetscFunctionReturn(0);
  TSPackageInitialized = PETSC_TRUE;
  /* Inialize subpackages */
  ierr = TSGLInitializePackage(path);CHKERRQ(ierr);
  ierr = TSGLAdaptInitializePackage(path);CHKERRQ(ierr);
  /* Register Classes */
  ierr = PetscCookieRegister("TS",&TS_COOKIE);CHKERRQ(ierr);
  /* Register Constructors */
  ierr = TSRegisterAll(path);CHKERRQ(ierr);
  /* Register Events */
  ierr = PetscLogEventRegister("TSStep",           TS_COOKIE,&TS_Step);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("TSPseudoCmptTStp", TS_COOKIE,&TS_PseudoComputeTimeStep);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("TSFunctionEval",   TS_COOKIE,&TS_FunctionEval);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("TSJacobianEval",   TS_COOKIE,&TS_JacobianEval);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(PETSC_NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "ts", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(TS_COOKIE);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(PETSC_NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "ts", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(TS_COOKIE);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(TSFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#ifdef PETSC_USE_DYNAMIC_LIBRARIES
EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscDLLibraryRegister_petscts"
/*
  PetscDLLibraryRegister - This function is called when the dynamic library it is in is opened.

  This one registers all the TS methods that are in the basic PETSc libpetscts library.

  Input Parameter:
  path - library path
 */
PetscErrorCode PETSCTS_DLLEXPORT PetscDLLibraryRegister_petscts(const char path[])
{
  PetscErrorCode ierr;

  ierr = PetscInitializeNoArguments(); if (ierr) return 1;
  PetscFunctionBegin;
  /*
      If we got here then PETSc was properly loaded
  */
  ierr = TSInitializePackage(path);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END


#endif /* PETSC_USE_DYNAMIC_LIBRARIES */

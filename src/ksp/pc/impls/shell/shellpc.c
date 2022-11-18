#define PETSCKSP_DLL


/*
   This provides a simple shell for Fortran (and C programmers) to 
  create their own preconditioner without writing much interface code.
*/

#include "private/pcimpl.h"        /*I "petscpc.h" I*/
#include "private/vecimpl.h"  

EXTERN_C_BEGIN 
typedef struct {
  void           *ctx;                     /* user provided contexts for preconditioner */
  PetscErrorCode (*destroy)(PC);
  PetscErrorCode (*setup)(PC);
  PetscErrorCode (*apply)(PC,Vec,Vec);
  PetscErrorCode (*applyBA)(PC,PCSide,Vec,Vec,Vec);
  PetscErrorCode (*presolve)(PC,KSP,Vec,Vec);
  PetscErrorCode (*postsolve)(PC,KSP,Vec,Vec);
  PetscErrorCode (*view)(PC,PetscViewer);
  PetscErrorCode (*applytranspose)(PC,Vec,Vec);
  PetscErrorCode (*applyrich)(PC,Vec,Vec,Vec,PetscReal,PetscReal,PetscReal,PetscInt,PetscTruth,PetscInt*,PCRichardsonConvergedReason*);
  char           *name;
} PC_Shell;
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PCShellGetContext"
/*@C
    PCShellGetContext - Returns the user-provided context associated with a shell PC

    Not Collective

    Input Parameter:
.   pc - should have been created with PCCreateShell()

    Output Parameter:
.   ctx - the user provided context

    Level: advanced

    Notes:
    This routine is intended for use within various shell routines
    
.keywords: PC, shell, get, context

.seealso: PCCreateShell(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellGetContext(PC pc,void **ctx)
{
  PetscErrorCode ierr;
  PetscTruth     flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  PetscValidPointer(ctx,2); 
  ierr = PetscTypeCompare((PetscObject)pc,PCSHELL,&flg);CHKERRQ(ierr);
  if (!flg) *ctx = 0; 
  else      *ctx = ((PC_Shell*)(pc->data))->ctx; 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetContext"
/*@
    PCShellSetContext - sets the context for a shell PC

   Collective on PC

    Input Parameters:
+   pc - the shell PC
-   ctx - the context

   Level: advanced

   Fortran Notes: The context can only be an integer or a PetscObject
      unfortunately it cannot be a Fortran array or derived type.


.seealso: PCCreateShell(), PCShellGetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetContext(PC pc,void *ctx)
{
  PC_Shell      *shell;
  PetscErrorCode ierr;
  PetscTruth     flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  shell = (PC_Shell*)pc->data;
  ierr = PetscTypeCompare((PetscObject)pc,PCSHELL,&flg);CHKERRQ(ierr);
  if (flg) {
    shell->ctx = ctx;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetUp_Shell"
static PetscErrorCode PCSetUp_Shell(PC pc)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->setup) SETERRQ(PETSC_ERR_USER,"No setup() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function setup()");
  CHKMEMQ;
  ierr  = (*shell->setup)(pc);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApply_Shell"
static PetscErrorCode PCApply_Shell(PC pc,Vec x,Vec y)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->apply) SETERRQ(PETSC_ERR_USER,"No apply() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function apply()");
  CHKMEMQ;
  ierr  = (*shell->apply)(pc,x,y);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplyBA_Shell"
static PetscErrorCode PCApplyBA_Shell(PC pc,PCSide side,Vec x,Vec y,Vec w)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->applyBA) SETERRQ(PETSC_ERR_USER,"No applyBA() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function applyBA()");
  CHKMEMQ;
  ierr  = (*shell->applyBA)(pc,side,x,y,w);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCPreSolve_Shell"
static PetscErrorCode PCPreSolve_Shell(PC pc,KSP ksp,Vec b,Vec x)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->presolve) SETERRQ(PETSC_ERR_USER,"No presolve() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function presolve()");
  CHKMEMQ;
  ierr  = (*shell->presolve)(pc,ksp,b,x);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCPostSolve_Shell"
static PetscErrorCode PCPostSolve_Shell(PC pc,KSP ksp,Vec b,Vec x)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->postsolve) SETERRQ(PETSC_ERR_USER,"No postsolve() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function postsolve()");
  CHKMEMQ;
  ierr  = (*shell->postsolve)(pc,ksp,b,x);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplyTranspose_Shell"
static PetscErrorCode PCApplyTranspose_Shell(PC pc,Vec x,Vec y)
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->applytranspose) SETERRQ(PETSC_ERR_USER,"No applytranspose() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function applytranspose()");
  CHKMEMQ;
  ierr  = (*shell->applytranspose)(pc,x,y);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplyRichardson_Shell"
static PetscErrorCode PCApplyRichardson_Shell(PC pc,Vec x,Vec y,Vec w,PetscReal rtol,PetscReal abstol, PetscReal dtol,PetscInt it,PetscTruth guesszero,PetscInt *outits,PCRichardsonConvergedReason *reason)
{
  PetscErrorCode ierr;
  PC_Shell       *shell;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  if (!shell->applyrich) SETERRQ(PETSC_ERR_USER,"No applyrichardson() routine provided to Shell PC");
  PetscStackPush("PCSHELL user function applyrichardson()");
  CHKMEMQ;
  ierr  = (*shell->applyrich)(pc,x,y,w,rtol,abstol,dtol,it,guesszero,outits,reason);CHKERRQ(ierr);
  CHKMEMQ;
  PetscStackPop;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCDestroy_Shell"
static PetscErrorCode PCDestroy_Shell(PC pc)
{
  PC_Shell       *shell = (PC_Shell*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscStrfree(shell->name);CHKERRQ(ierr);
  if (shell->destroy) {
    ierr  = (*shell->destroy)(pc);CHKERRQ(ierr);
  }
  ierr = PetscFree(shell);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCView_Shell"
static PetscErrorCode PCView_Shell(PC pc,PetscViewer viewer)
{
  PC_Shell       *shell = (PC_Shell*)pc->data;
  PetscErrorCode ierr;
  PetscTruth     iascii;

  PetscFunctionBegin;
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (shell->name) {ierr = PetscViewerASCIIPrintf(viewer,"  Shell: %s\n",shell->name);CHKERRQ(ierr);}
    else             {ierr = PetscViewerASCIIPrintf(viewer,"  Shell: no name\n");CHKERRQ(ierr);}
  }
  if (shell->view) {
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
    ierr  = (*shell->view)(pc,viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------------------*/
EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetDestroy_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetDestroy_Shell(PC pc, PetscErrorCode (*destroy)(PC))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell          = (PC_Shell*)pc->data;
  shell->destroy = destroy;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetSetUp_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetSetUp_Shell(PC pc, PetscErrorCode (*setup)(PC))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell        = (PC_Shell*)pc->data;
  shell->setup = setup;
  if (setup) pc->ops->setup = PCSetUp_Shell;
  else       pc->ops->setup = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApply_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApply_Shell(PC pc,PetscErrorCode (*apply)(PC,Vec,Vec))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell        = (PC_Shell*)pc->data;
  shell->apply = apply;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyBA_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyBA_Shell(PC pc,PetscErrorCode (*applyBA)(PC,PCSide,Vec,Vec,Vec))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell          = (PC_Shell*)pc->data;
  shell->applyBA = applyBA;
  if (applyBA) pc->ops->applyBA  = PCApplyBA_Shell;
  else         pc->ops->applyBA  = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetPreSolve_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetPreSolve_Shell(PC pc,PetscErrorCode (*presolve)(PC,KSP,Vec,Vec))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell           = (PC_Shell*)pc->data;
  shell->presolve = presolve;
  if (presolve) pc->ops->presolve = PCPreSolve_Shell;
  else          pc->ops->presolve = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetPostSolve_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetPostSolve_Shell(PC pc,PetscErrorCode (*postsolve)(PC,KSP,Vec,Vec))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell            = (PC_Shell*)pc->data;
  shell->postsolve = postsolve;
  if (postsolve) pc->ops->postsolve = PCPostSolve_Shell;
  else           pc->ops->postsolve = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetView_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetView_Shell(PC pc,PetscErrorCode (*view)(PC,PetscViewer))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell        = (PC_Shell*)pc->data;
  shell->view = view;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyTranspose_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyTranspose_Shell(PC pc,PetscErrorCode (*applytranspose)(PC,Vec,Vec))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell                 = (PC_Shell*)pc->data;
  shell->applytranspose = applytranspose;
  if (applytranspose) pc->ops->applytranspose = PCApplyTranspose_Shell;
  else                pc->ops->applytranspose = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyRichardson_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyRichardson_Shell(PC pc,PetscErrorCode (*applyrich)(PC,Vec,Vec,Vec,PetscReal,PetscReal,PetscReal,PetscInt,PetscTruth,PetscInt*,PCRichardsonConvergedReason*))
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell            = (PC_Shell*)pc->data;
  shell->applyrich = applyrich;
  if (applyrich) pc->ops->applyrichardson  = PCApplyRichardson_Shell;
  else           pc->ops->applyrichardson  = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellSetName_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetName_Shell(PC pc,const char name[])
{
  PC_Shell       *shell;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  shell = (PC_Shell*)pc->data;
  ierr = PetscStrfree(shell->name);CHKERRQ(ierr);    
  ierr = PetscStrallocpy(name,&shell->name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCShellGetName_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCShellGetName_Shell(PC pc,char *name[])
{
  PC_Shell *shell;

  PetscFunctionBegin;
  shell  = (PC_Shell*)pc->data;
  *name  = shell->name;
  PetscFunctionReturn(0);
}
EXTERN_C_END

/* -------------------------------------------------------------------------------*/

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetDestroy"
/*@C
   PCShellSetDestroy - Sets routine to use to destroy the user-provided 
   application context.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
.  destroy - the application-provided destroy routine

   Calling sequence of destroy:
.vb
   PetscErrorCode destroy (PC)
.ve

.  ptr - the application context

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, destroy, user-provided

.seealso: PCShellSetApply(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetDestroy(PC pc,PetscErrorCode (*destroy)(PC))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetDestroy_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,destroy);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PCShellSetSetUp"
/*@C
   PCShellSetSetUp - Sets routine to use to "setup" the preconditioner whenever the 
   matrix operator is changed.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
.  setup - the application-provided setup routine

   Calling sequence of setup:
.vb
   PetscErrorCode setup (PC pc)
.ve

.  pc - the preconditioner, get the application context with PCShellGetContext()

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, setup, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetApply(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetSetUp(PC pc,PetscErrorCode (*setup)(PC))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetSetUp_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,setup);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PCShellSetView"
/*@C
   PCShellSetView - Sets routine to use as viewer of shell preconditioner

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  view - the application-provided view routine

   Calling sequence of apply:
.vb
   PetscErrorCode view(PC pc,PetscViewer v)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
-  v   - viewer

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApplyTranspose()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetView(PC pc,PetscErrorCode (*view)(PC,PetscViewer))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,PetscViewer));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetView_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,view);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApply"
/*@C
   PCShellSetApply - Sets routine to use as preconditioner.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  apply - the application-provided preconditioning routine

   Calling sequence of apply:
.vb
   PetscErrorCode apply (PC pc,Vec xin,Vec xout)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  xin - input vector
-  xout - output vector

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApplyTranspose(), PCShellSetContext(), PCShellSetApplyBA()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApply(PC pc,PetscErrorCode (*apply)(PC,Vec,Vec))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,Vec,Vec));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetApply_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,apply);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyBA"
/*@C
   PCShellSetApplyBA - Sets routine to use as preconditioner times operator.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  applyBA - the application-provided BA routine

   Calling sequence of apply:
.vb
   PetscErrorCode applyBA (PC pc,Vec xin,Vec xout)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  xin - input vector
-  xout - output vector

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApplyTranspose(), PCShellSetContext(), PCShellSetApply()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyBA(PC pc,PetscErrorCode (*applyBA)(PC,PCSide,Vec,Vec,Vec))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,PCSide,Vec,Vec,Vec));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetApplyBA_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,applyBA);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyTranspose"
/*@C
   PCShellSetApplyTranspose - Sets routine to use as preconditioner transpose.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  apply - the application-provided preconditioning transpose routine

   Calling sequence of apply:
.vb
   PetscErrorCode applytranspose (PC pc,Vec xin,Vec xout)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  xin - input vector
-  xout - output vector

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

   Notes: 
   Uses the same context variable as PCShellSetApply().

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApply(), PCSetContext(), PCShellSetApplyBA()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyTranspose(PC pc,PetscErrorCode (*applytranspose)(PC,Vec,Vec))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,Vec,Vec));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetApplyTranspose_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,applytranspose);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetPreSolve"
/*@C
   PCShellSetPreSolve - Sets routine to apply to the operators/vectors before a KSPSolve() is
      applied. This usually does something like scale the linear system in some application 
      specific way.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  presolve - the application-provided presolve routine

   Calling sequence of presolve:
.vb
   PetscErrorCode presolve (PC,KSP ksp,Vec b,Vec x)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  xin - input vector
-  xout - output vector

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApplyTranspose(), PCShellSetPostSolve(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetPreSolve(PC pc,PetscErrorCode (*presolve)(PC,KSP,Vec,Vec))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,KSP,Vec,Vec));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetPreSolve_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,presolve);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetPostSolve"
/*@C
   PCShellSetPostSolve - Sets routine to apply to the operators/vectors before a KSPSolve() is
      applied. This usually does something like scale the linear system in some application 
      specific way.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  postsolve - the application-provided presolve routine

   Calling sequence of postsolve:
.vb
   PetscErrorCode postsolve(PC,KSP ksp,Vec b,Vec x)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  xin - input vector
-  xout - output vector

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, user-provided

.seealso: PCShellSetApplyRichardson(), PCShellSetSetUp(), PCShellSetApplyTranspose(), PCShellSetPreSolve(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetPostSolve(PC pc,PetscErrorCode (*postsolve)(PC,KSP,Vec,Vec))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,KSP,Vec,Vec));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetPostSolve_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,postsolve);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetName"
/*@C
   PCShellSetName - Sets an optional name to associate with a shell
   preconditioner.

   Not Collective

   Input Parameters:
+  pc - the preconditioner context
-  name - character string describing shell preconditioner

   Level: developer

.keywords: PC, shell, set, name, user-provided

.seealso: PCShellGetName()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetName(PC pc,const char name[])
{
  PetscErrorCode ierr,(*f)(PC,const char []);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetName_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,name);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellGetName"
/*@C
   PCShellGetName - Gets an optional name that the user has set for a shell
   preconditioner.

   Not Collective

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  name - character string describing shell preconditioner (you should not free this)

   Level: developer

.keywords: PC, shell, get, name, user-provided

.seealso: PCShellSetName()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellGetName(PC pc,char *name[])
{
  PetscErrorCode ierr,(*f)(PC,char *[]);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  PetscValidPointer(name,2);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellGetName_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,name);CHKERRQ(ierr);
  } else {
    SETERRQ(PETSC_ERR_ARG_WRONG,"Not shell preconditioner, cannot get name");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCShellSetApplyRichardson"
/*@C
   PCShellSetApplyRichardson - Sets routine to use as preconditioner
   in Richardson iteration.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  apply - the application-provided preconditioning routine

   Calling sequence of apply:
.vb
   PetscErrorCode apply (PC pc,Vec b,Vec x,Vec r,PetscReal rtol,PetscReal abstol,PetscReal dtol,PetscInt maxits)
.ve

+  pc - the preconditioner, get the application context with PCShellGetContext()
.  b - right-hand-side
.  x - current iterate
.  r - work space
.  rtol - relative tolerance of residual norm to stop at
.  abstol - absolute tolerance of residual norm to stop at
.  dtol - if residual norm increases by this factor than return
-  maxits - number of iterations to run

   Notes: the function MUST return an error code of 0 on success and nonzero on failure.

   Level: developer

.keywords: PC, shell, set, apply, Richardson, user-provided

.seealso: PCShellSetApply(), PCShellSetContext()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCShellSetApplyRichardson(PC pc,PetscErrorCode (*apply)(PC,Vec,Vec,Vec,PetscReal,PetscReal,PetscReal,PetscInt,PetscTruth,PetscInt*,PCRichardsonConvergedReason*))
{
  PetscErrorCode ierr,(*f)(PC,PetscErrorCode (*)(PC,Vec,Vec,Vec,PetscReal,PetscReal,PetscReal,PetscInt,PetscTruth,PetscInt*,PCRichardsonConvergedReason*));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCShellSetApplyRichardson_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,apply);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*MC
   PCSHELL - Creates a new preconditioner class for use with your 
              own private data storage format.

   Level: advanced
>
   Concepts: providing your own preconditioner

  Usage:
$             extern PetscErrorCode apply(PC,Vec,Vec);
$             extern PetscErrorCode applyba(PC,PCSide,Vec,Vec,Vec);
$             extern PetscErrorCode applytranspose(PC,Vec,Vec);
$             extern PetscErrorCode setup(PC);
$             extern PetscErrorCode destroy(PC);
$
$             PCCreate(comm,&pc);
$             PCSetType(pc,PCSHELL);
$             PCShellSetContext(pc,ctx)
$             PCShellSetApply(pc,apply);
$             PCShellSetApplyBA(pc,applyba);               (optional)
$             PCShellSetApplyTranspose(pc,applytranspose); (optional)
$             PCShellSetSetUp(pc,setup);                   (optional)
$             PCShellSetDestroy(pc,destroy);               (optional)

.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC,
           MATSHELL, PCShellSetSetUp(), PCShellSetApply(), PCShellSetView(), 
           PCShellSetApplyTranspose(), PCShellSetName(), PCShellSetApplyRichardson(), 
           PCShellGetName(), PCShellSetContext(), PCShellGetContext(), PCShellSetApplyBA()
M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCCreate_Shell"
PetscErrorCode PETSCKSP_DLLEXPORT PCCreate_Shell(PC pc)
{
  PetscErrorCode ierr;
  PC_Shell       *shell;

  PetscFunctionBegin;
  ierr = PetscNewLog(pc,PC_Shell,&shell);CHKERRQ(ierr);
  pc->data  = (void*)shell;

  pc->ops->destroy         = PCDestroy_Shell;
  pc->ops->view            = PCView_Shell;
  pc->ops->apply           = PCApply_Shell;
  pc->ops->applytranspose  = 0;
  pc->ops->applyrichardson = 0;
  pc->ops->setup           = 0;
  pc->ops->presolve        = 0;
  pc->ops->postsolve       = 0;

  shell->apply          = 0;
  shell->applytranspose = 0;
  shell->name           = 0;
  shell->applyrich      = 0;
  shell->presolve       = 0;
  shell->postsolve      = 0;
  shell->ctx            = 0;
  shell->setup          = 0;
  shell->view           = 0;
  shell->destroy        = 0;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetDestroy_C","PCShellSetDestroy_Shell",
                    PCShellSetDestroy_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetSetUp_C","PCShellSetSetUp_Shell",
                    PCShellSetSetUp_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetApply_C","PCShellSetApply_Shell",
                    PCShellSetApply_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetApplyBA_C","PCShellSetApplyBA_Shell",
                    PCShellSetApplyBA_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetPreSolve_C","PCShellSetPreSolve_Shell",
                    PCShellSetPreSolve_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetPostSolve_C","PCShellSetPostSolve_Shell",
                    PCShellSetPostSolve_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetView_C","PCShellSetView_Shell",
                    PCShellSetView_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetApplyTranspose_C","PCShellSetApplyTranspose_Shell",
                    PCShellSetApplyTranspose_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetName_C","PCShellSetName_Shell",
                    PCShellSetName_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellGetName_C","PCShellGetName_Shell",
                    PCShellGetName_Shell);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCShellSetApplyRichardson_C","PCShellSetApplyRichardson_Shell",
                    PCShellSetApplyRichardson_Shell);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END







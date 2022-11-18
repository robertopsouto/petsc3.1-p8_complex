#define PETSCKSP_DLL

#include "private/pcimpl.h"   /*I "petscpc.h" I*/
#include "petscksp.h"            /*I "petscksp.h" I*/

typedef struct {
  PetscTruth use_true_matrix;       /* use mat rather than pmat in inner linear solve */
  KSP        ksp; 
  PetscInt   its;                   /* total number of iterations KSP uses */
} PC_KSP;


#undef __FUNCT__  
#define __FUNCT__ "PCKSPCreateKSP_KSP"
static PetscErrorCode PETSCKSP_DLLEXPORT PCKSPCreateKSP_KSP(PC pc)
{
  PetscErrorCode ierr;
  const char     *prefix;
  PC_KSP         *jac = (PC_KSP *)pc->data;

  PetscFunctionBegin;
 ierr  = KSPCreate(((PetscObject)pc)->comm,&jac->ksp);CHKERRQ(ierr);
  ierr = PetscObjectIncrementTabLevel((PetscObject)jac->ksp,(PetscObject)pc,1);CHKERRQ(ierr);
  ierr = PCGetOptionsPrefix(pc,&prefix);CHKERRQ(ierr);
  ierr = KSPSetOptionsPrefix(jac->ksp,prefix);CHKERRQ(ierr);
  ierr = KSPAppendOptionsPrefix(jac->ksp,"ksp_");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApply_KSP"
static PetscErrorCode PCApply_KSP(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PetscInt       its;
  PC_KSP         *jac = (PC_KSP*)pc->data;

  PetscFunctionBegin;
  ierr      = KSPSetInitialGuessNonzero(jac->ksp,pc->nonzero_guess);CHKERRQ(ierr);
  ierr      = KSPSolve(jac->ksp,x,y);CHKERRQ(ierr);
  ierr      = KSPGetIterationNumber(jac->ksp,&its);CHKERRQ(ierr);
  jac->its += its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplyTranspose_KSP"
static PetscErrorCode PCApplyTranspose_KSP(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PetscInt       its;
  PC_KSP         *jac = (PC_KSP*)pc->data;

  PetscFunctionBegin;
  ierr      = KSPSolveTranspose(jac->ksp,x,y);CHKERRQ(ierr);
  ierr      = KSPGetIterationNumber(jac->ksp,&its);CHKERRQ(ierr);
  jac->its += its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetUp_KSP"
static PetscErrorCode PCSetUp_KSP(PC pc)
{
  PetscErrorCode ierr;
  PC_KSP         *jac = (PC_KSP*)pc->data;
  Mat            mat;
  PetscTruth     A;

  PetscFunctionBegin;
  if (!jac->ksp) {ierr = PCKSPCreateKSP_KSP(pc);CHKERRQ(ierr);}
  ierr = KSPSetFromOptions(jac->ksp);CHKERRQ(ierr);
  if (jac->use_true_matrix) mat = pc->mat;
  else                      mat = pc->pmat;

  ierr = KSPGetOperatorsSet(jac->ksp,&A,PETSC_NULL);CHKERRQ(ierr);
  if (!A) {
    ierr = KSPSetOperators(jac->ksp,mat,pc->pmat,pc->flag);CHKERRQ(ierr);
  }
  ierr = KSPSetUp(jac->ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* Default destroy, if it has never been setup */
#undef __FUNCT__  
#define __FUNCT__ "PCDestroy_KSP"
static PetscErrorCode PCDestroy_KSP(PC pc)
{
  PC_KSP         *jac = (PC_KSP*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (jac->ksp) {ierr = KSPDestroy(jac->ksp);CHKERRQ(ierr);}
  ierr = PetscFree(jac);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCView_KSP"
static PetscErrorCode PCView_KSP(PC pc,PetscViewer viewer)
{
  PC_KSP         *jac = (PC_KSP*)pc->data;
  PetscErrorCode ierr;
  PetscTruth     iascii;

  PetscFunctionBegin;
  if (!jac->ksp) {ierr = PCKSPCreateKSP_KSP(pc);CHKERRQ(ierr);}
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (jac->use_true_matrix) {
      ierr = PetscViewerASCIIPrintf(viewer,"Using true matrix (not preconditioner matrix) on inner solve\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"KSP and PC on KSP preconditioner follow\n");CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"---------------------------------\n");CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_SUP,"Viewer type %s not supported for this object",((PetscObject)viewer)->type_name);
  }
  ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
  ierr = KSPView(jac->ksp,viewer);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"---------------------------------\n");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetFromOptions_KSP"
static PetscErrorCode PCSetFromOptions_KSP(PC pc)
{
  PetscErrorCode ierr;
  PetscTruth     flg = PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscOptionsHead("KSP preconditioner options");CHKERRQ(ierr);
  ierr = PetscOptionsTruth("-pc_ksp_true","Use true matrix to define inner linear system, not preconditioner matrix","PCKSPSetUseTrue",flg,&flg,PETSC_NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = PCKSPSetUseTrue(pc);CHKERRQ(ierr);
    }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------------------*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCKSPSetUseTrue_KSP"
PetscErrorCode PETSCKSP_DLLEXPORT PCKSPSetUseTrue_KSP(PC pc)
{
  PC_KSP   *jac;

  PetscFunctionBegin;
  jac                  = (PC_KSP*)pc->data;
  jac->use_true_matrix = PETSC_TRUE;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCKSPGetKSP_KSP"
PetscErrorCode PETSCKSP_DLLEXPORT PCKSPGetKSP_KSP(PC pc,KSP *ksp)
{
  PC_KSP         *jac = (PC_KSP*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!jac->ksp) {ierr = PCKSPCreateKSP_KSP(pc);CHKERRQ(ierr);}
  *ksp        = jac->ksp;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PCKSPSetUseTrue"
/*@
   PCKSPSetUseTrue - Sets a flag to indicate that the true matrix (rather than
   the matrix used to define the preconditioner) is used to compute the
   residual inside the inner solve.

   Collective on PC

   Input Parameters:
.  pc - the preconditioner context

   Options Database Key:
.  -pc_ksp_true - Activates PCKSPSetUseTrue()

   Note:
   For the common case in which the preconditioning and linear 
   system matrices are identical, this routine is unnecessary.

   Level: advanced

.keywords:  PC, KSP, set, true, local, flag

.seealso: PCSetOperators(), PCBJacobiSetUseTrueLocal()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCKSPSetUseTrue(PC pc)
{
  PetscErrorCode ierr,(*f)(PC);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCKSPSetUseTrue_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCKSPGetKSP"
/*@
   PCKSPGetKSP - Gets the KSP context for a KSP PC.

   Not Collective but KSP returned is parallel if PC was parallel

   Input Parameter:
.  pc - the preconditioner context

   Output Parameters:
.  ksp - the PC solver

   Notes:
   You must call KSPSetUp() before calling PCKSPGetKSP().

   Level: advanced

.keywords:  PC, KSP, get, context
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCKSPGetKSP(PC pc,KSP *ksp)
{
  PetscErrorCode ierr,(*f)(PC,KSP*);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  PetscValidPointer(ksp,2);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCKSPGetKSP_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,ksp);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------------------*/

/*MC
     PCKSP -    Defines a preconditioner that can consist of any KSP solver.
                 This allows, for example, embedding a Krylov method inside a preconditioner.

   Options Database Key:
.     -pc_ksp_true - use the matrix that defines the linear system as the matrix for the
                    inner solver, otherwise by default it uses the matrix used to construct
                    the preconditioner (see PCSetOperators())

   Level: intermediate

   Concepts: inner iteration

   Notes: Using a Krylov method inside another Krylov method can be dangerous (you get divergence or
          the incorrect answer) unless you use KSPFGMRES as the other Krylov method


.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC,
           PCSHELL, PCCOMPOSITE, PCKSPUseTrue(), PCKSPGetKSP()

M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCCreate_KSP"
PetscErrorCode PETSCKSP_DLLEXPORT PCCreate_KSP(PC pc)
{
  PetscErrorCode ierr;
  PC_KSP         *jac;

  PetscFunctionBegin;
  ierr = PetscNewLog(pc,PC_KSP,&jac);CHKERRQ(ierr);
  pc->ops->apply              = PCApply_KSP;
  pc->ops->applytranspose     = PCApplyTranspose_KSP;
  pc->ops->setup              = PCSetUp_KSP;
  pc->ops->destroy            = PCDestroy_KSP;
  pc->ops->setfromoptions     = PCSetFromOptions_KSP;
  pc->ops->view               = PCView_KSP;
  pc->ops->applyrichardson    = 0;

  pc->data               = (void*)jac;
 

  jac->use_true_matrix = PETSC_FALSE;
  jac->its             = 0;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCKSPSetUseTrue_C","PCKSPSetUseTrue_KSP",
                    PCKSPSetUseTrue_KSP);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCKSPGetKSP_C","PCKSPGetKSP_KSP",
                    PCKSPGetKSP_KSP);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
EXTERN_C_END


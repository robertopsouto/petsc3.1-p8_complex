#define PETSCKSP_DLL

/*
   Defines a  Eisenstat trick SSOR  preconditioner. This uses about 
 %50 of the usual amount of floating point ops used for SSOR + Krylov 
 method. But it requires actually solving the preconditioned problem 
 with both left and right preconditioning. 
*/
#include "private/pcimpl.h"           /*I "petscpc.h" I*/

typedef struct {
  Mat        shell,A;
  Vec        b,diag;     /* temporary storage for true right hand side */
  PetscReal  omega;
  PetscTruth usediag;    /* indicates preconditioner should include diagonal scaling*/
} PC_Eisenstat;


#undef __FUNCT__  
#define __FUNCT__ "PCMult_Eisenstat"
static PetscErrorCode PCMult_Eisenstat(Mat mat,Vec b,Vec x)
{
  PetscErrorCode ierr;
  PC             pc;
  PC_Eisenstat   *eis;

  PetscFunctionBegin;
  ierr = MatShellGetContext(mat,(void **)&pc);CHKERRQ(ierr);
  eis = (PC_Eisenstat*)pc->data;
  ierr = MatSOR(eis->A,b,eis->omega,SOR_EISENSTAT,0.0,1,1,x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApply_Eisenstat"
static PetscErrorCode PCApply_Eisenstat(PC pc,Vec x,Vec y)
{
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data;
  PetscErrorCode ierr;
  PetscTruth     hasop;

  PetscFunctionBegin;
  if (eis->usediag)  {
    ierr = MatHasOperation(pc->pmat,MATOP_MULT_DIAGONAL_BLOCK,&hasop);CHKERRQ(ierr);
    if (hasop) {
      ierr = MatMultDiagonalBlock(pc->pmat,x,y);CHKERRQ(ierr);
    } else {
      ierr = VecPointwiseMult(y,x,eis->diag);CHKERRQ(ierr);
    }
  } else {ierr = VecCopy(x,y);CHKERRQ(ierr);}
  PetscFunctionReturn(0); 
}

#undef __FUNCT__  
#define __FUNCT__ "PCPreSolve_Eisenstat"
static PetscErrorCode PCPreSolve_Eisenstat(PC pc,KSP ksp,Vec b,Vec x)
{
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data;
  PetscTruth     nonzero;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (pc->mat != pc->pmat) SETERRQ(PETSC_ERR_SUP,"Cannot have different mat and pmat"); 
 
  /* swap shell matrix and true matrix */
  eis->A    = pc->mat;
  pc->mat   = eis->shell;

  if (!eis->b) {
    ierr = VecDuplicate(b,&eis->b);CHKERRQ(ierr);
    ierr = PetscLogObjectParent(pc,eis->b);CHKERRQ(ierr);
  }
  
  /* save true b, other option is to swap pointers */
  ierr = VecCopy(b,eis->b);CHKERRQ(ierr);

  /* if nonzero initial guess, modify x */
  ierr = KSPGetInitialGuessNonzero(ksp,&nonzero);CHKERRQ(ierr);
  if (nonzero) {
    ierr = MatSOR(eis->A,x,eis->omega,SOR_APPLY_UPPER,0.0,1,1,x);CHKERRQ(ierr);
  }

  /* modify b by (L + D/omega)^{-1} */
  ierr =   MatSOR(eis->A,b,eis->omega,(MatSORType)(SOR_ZERO_INITIAL_GUESS | SOR_LOCAL_FORWARD_SWEEP),0.0,1,1,b);CHKERRQ(ierr);  
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCPostSolve_Eisenstat"
static PetscErrorCode PCPostSolve_Eisenstat(PC pc,KSP ksp,Vec b,Vec x)
{
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* modify x by (U + D/omega)^{-1} */
  ierr =   MatSOR(eis->A,x,eis->omega,(MatSORType)(SOR_ZERO_INITIAL_GUESS | SOR_LOCAL_BACKWARD_SWEEP),0.0,1,1,x);CHKERRQ(ierr);
  pc->mat = eis->A;
  /* get back true b */
  ierr = VecCopy(eis->b,b);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCDestroy_Eisenstat"
static PetscErrorCode PCDestroy_Eisenstat(PC pc)
{
  PC_Eisenstat   *eis = (PC_Eisenstat *)pc->data; 
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (eis->b)     {ierr = VecDestroy(eis->b);CHKERRQ(ierr);}
  if (eis->shell) {ierr = MatDestroy(eis->shell);CHKERRQ(ierr);}
  if (eis->diag)  {ierr = VecDestroy(eis->diag);CHKERRQ(ierr);}
  ierr = PetscFree(eis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetFromOptions_Eisenstat"
static PetscErrorCode PCSetFromOptions_Eisenstat(PC pc)
{
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data; 
  PetscErrorCode ierr;
  PetscTruth     flg = PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscOptionsHead("Eisenstat SSOR options");CHKERRQ(ierr);
    ierr = PetscOptionsReal("-pc_eisenstat_omega","Relaxation factor 0 < omega < 2","PCEisenstatSetOmega",eis->omega,&eis->omega,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsTruth("-pc_eisenstat_no_diagonal_scaling","Do not use standard diagonal scaling","PCEisenstatNoDiagonalScaling",flg,&flg,PETSC_NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = PCEisenstatNoDiagonalScaling(pc);CHKERRQ(ierr);
    }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCView_Eisenstat"
static PetscErrorCode PCView_Eisenstat(PC pc,PetscViewer viewer)
{
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data; 
  PetscErrorCode ierr;
  PetscTruth     iascii;

  PetscFunctionBegin;
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"Eisenstat: omega = %G\n",eis->omega);CHKERRQ(ierr);
    if (eis->usediag) {
      ierr = PetscViewerASCIIPrintf(viewer,"Eisenstat: Using diagonal scaling (default)\n");CHKERRQ(ierr);
    } else {
      ierr = PetscViewerASCIIPrintf(viewer,"Eisenstat: Not using diagonal scaling\n");CHKERRQ(ierr);
    }
  } else {
    SETERRQ1(PETSC_ERR_SUP,"Viewer type %s not supported for Eisenstat PC",((PetscObject)viewer)->type_name);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetUp_Eisenstat"
static PetscErrorCode PCSetUp_Eisenstat(PC pc)
{
  PetscErrorCode ierr;
  PetscInt       M,N,m,n;
  PC_Eisenstat   *eis = (PC_Eisenstat*)pc->data;

  PetscFunctionBegin;
  if (!pc->setupcalled) {
    ierr = MatGetSize(pc->mat,&M,&N);CHKERRQ(ierr);
    ierr = MatGetLocalSize(pc->mat,&m,&n);CHKERRQ(ierr);
    ierr = MatCreate(((PetscObject)pc)->comm,&eis->shell);CHKERRQ(ierr);
    ierr = MatSetSizes(eis->shell,m,n,M,N);CHKERRQ(ierr);
    ierr = MatSetType(eis->shell,MATSHELL);CHKERRQ(ierr);
    ierr = MatShellSetContext(eis->shell,(void*)pc);CHKERRQ(ierr);
    ierr = PetscLogObjectParent(pc,eis->shell);CHKERRQ(ierr);
    ierr = MatShellSetOperation(eis->shell,MATOP_MULT,(void(*)(void))PCMult_Eisenstat);CHKERRQ(ierr);
  }
  if (!eis->usediag) PetscFunctionReturn(0);
  if (!pc->setupcalled) {
    ierr = MatGetVecs(pc->pmat,&eis->diag,0);CHKERRQ(ierr);
    ierr = PetscLogObjectParent(pc,eis->diag);CHKERRQ(ierr);
  }
  ierr = MatGetDiagonal(pc->pmat,eis->diag);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCEisenstatSetOmega_Eisenstat"
PetscErrorCode PETSCKSP_DLLEXPORT PCEisenstatSetOmega_Eisenstat(PC pc,PetscReal omega)
{
  PC_Eisenstat  *eis;

  PetscFunctionBegin;
  if (omega >= 2.0 || omega <= 0.0) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"Relaxation out of range");
  eis = (PC_Eisenstat*)pc->data;
  eis->omega = omega;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCEisenstatNoDiagonalScaling_Eisenstat"
PetscErrorCode PETSCKSP_DLLEXPORT PCEisenstatNoDiagonalScaling_Eisenstat(PC pc)
{
  PC_Eisenstat *eis;

  PetscFunctionBegin;
  eis = (PC_Eisenstat*)pc->data;
  eis->usediag = PETSC_FALSE;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PCEisenstatSetOmega"
/*@ 
   PCEisenstatSetOmega - Sets the SSOR relaxation coefficient, omega,
   to use with Eisenstat's trick (where omega = 1.0 by default).

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  omega - relaxation coefficient (0 < omega < 2)

   Options Database Key:
.  -pc_eisenstat_omega <omega> - Sets omega

   Notes: 
   The Eisenstat trick implementation of SSOR requires about 50% of the
   usual amount of floating point operations used for SSOR + Krylov method;
   however, the preconditioned problem must be solved with both left 
   and right preconditioning.

   To use SSOR without the Eisenstat trick, employ the PCSOR preconditioner, 
   which can be chosen with the database options
$    -pc_type  sor  -pc_sor_symmetric

   Level: intermediate

.keywords: PC, Eisenstat, set, SOR, SSOR, relaxation, omega

.seealso: PCSORSetOmega()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCEisenstatSetOmega(PC pc,PetscReal omega)
{
  PetscErrorCode ierr,(*f)(PC,PetscReal);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCEisenstatSetOmega_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,omega);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCEisenstatNoDiagonalScaling"
/*@
   PCEisenstatNoDiagonalScaling - Causes the Eisenstat preconditioner
   not to do additional diagonal preconditioning. For matrices with a constant 
   along the diagonal, this may save a small amount of work.

   Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Options Database Key:
.  -pc_eisenstat_no_diagonal_scaling - Activates PCEisenstatNoDiagonalScaling()

   Level: intermediate

   Note:
     If you use the KPSSetDiagonalScaling() or -ksp_diagonal_scale option then you will
   likley want to use this routine since it will save you some unneeded flops.

.keywords: PC, Eisenstat, use, diagonal, scaling, SSOR

.seealso: PCEisenstatSetOmega()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT PCEisenstatNoDiagonalScaling(PC pc)
{
  PetscErrorCode ierr,(*f)(PC);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCEisenstatNoDiagonalScaling_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------*/

/*MC
     PCEISENSTAT - An implementation of SSOR (symmetric successive over relaxation, symmetric Gauss-Seidel)
           preconditioning that incorporates Eisenstat's trick to reduce the amount of computation needed.

   Options Database Keys:
+  -pc_eisenstat_omega <omega> - Sets omega
-  -pc_eisenstat_no_diagonal_scaling - Activates PCEisenstatNoDiagonalScaling()

   Level: beginner

  Concepts: SOR, preconditioners, Gauss-Seidel, Eisenstat's trick

   Notes: Only implemented for the SeqAIJ matrix format.
          Not a true parallel SOR, in parallel this implementation corresponds to block
          Jacobi with SOR on each block.

.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC,
           PCEisenstatNoDiagonalScaling(), PCEisenstatSetOmega(), PCSOR
M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCCreate_Eisenstat"
PetscErrorCode PETSCKSP_DLLEXPORT PCCreate_Eisenstat(PC pc)
{
  PetscErrorCode ierr;
  PC_Eisenstat   *eis;

  PetscFunctionBegin;
  ierr = PetscNewLog(pc,PC_Eisenstat,&eis);CHKERRQ(ierr);

  pc->ops->apply           = PCApply_Eisenstat;
  pc->ops->presolve        = PCPreSolve_Eisenstat;
  pc->ops->postsolve       = PCPostSolve_Eisenstat;
  pc->ops->applyrichardson = 0;
  pc->ops->setfromoptions  = PCSetFromOptions_Eisenstat;
  pc->ops->destroy         = PCDestroy_Eisenstat;
  pc->ops->view            = PCView_Eisenstat;
  pc->ops->setup           = PCSetUp_Eisenstat;

  pc->data           = (void*)eis;
  eis->omega         = 1.0;
  eis->b             = 0;
  eis->diag          = 0;
  eis->usediag       = PETSC_TRUE;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCEisenstatSetOmega_C","PCEisenstatSetOmega_Eisenstat",
                    PCEisenstatSetOmega_Eisenstat);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCEisenstatNoDiagonalScaling_C",
                    "PCEisenstatNoDiagonalScaling_Eisenstat",
                    PCEisenstatNoDiagonalScaling_Eisenstat);CHKERRQ(ierr);
 PetscFunctionReturn(0);
}
EXTERN_C_END

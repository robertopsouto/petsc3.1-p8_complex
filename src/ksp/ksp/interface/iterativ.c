#define PETSCKSP_DLL

/*
   This file contains some simple default routines.  
   These routines should be SHORT, since they will be included in every
   executable image that uses the iterative routines (note that, through
   the registry system, we provide a way to load only the truely necessary
   files) 
 */
#include "private/kspimpl.h"   /*I "petscksp.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultFreeWork"
/*
  KSPDefaultFreeWork - Free work vectors

  Input Parameters:
. ksp  - iterative context
 */
PetscErrorCode KSPDefaultFreeWork(KSP ksp)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (ksp->work)  {
    ierr = VecDestroyVecs(ksp->work,ksp->nwork);CHKERRQ(ierr);
    ksp->work = PETSC_NULL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetResidualNorm"
/*@
   KSPGetResidualNorm - Gets the last (approximate preconditioned)
   residual norm that has been computed.
 
   Not Collective

   Input Parameters:
.  ksp - the iterative context

   Output Parameters:
.  rnorm - residual norm

   Level: intermediate

.keywords: KSP, get, residual norm

.seealso: KSPBuildResidual()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPGetResidualNorm(KSP ksp,PetscReal *rnorm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidDoublePointer(rnorm,2);
  *rnorm = ksp->rnorm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetIterationNumber"
/*@
   KSPGetIterationNumber - Gets the current iteration number; if the 
         KSPSolve() is complete, returns the number of iterations
         used.
 
   Not Collective

   Input Parameters:
.  ksp - the iterative context

   Output Parameters:
.  its - number of iterations

   Level: intermediate

   Notes:
      During the ith iteration this returns i-1
.keywords: KSP, get, residual norm

.seealso: KSPBuildResidual(), KSPGetResidualNorm()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPGetIterationNumber(KSP ksp,PetscInt *its)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidIntPointer(its,2);
  *its = ksp->its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorSingularValue"
/*@C
    KSPMonitorSingularValue - Prints the two norm of the true residual and
    estimation of the extreme singular values of the preconditioned problem
    at each iteration.
 
    Collective on KSP

    Input Parameters:
+   ksp - the iterative context
.   n  - the iteration
-   rnorm - the two norm of the residual

    Options Database Key:
.   -ksp_monitor_singular_value - Activates KSPMonitorSingularValue()

    Notes:
    The CG solver uses the Lanczos technique for eigenvalue computation, 
    while GMRES uses the Arnoldi technique; other iterative methods do
    not currently compute singular values.

    Level: intermediate

.keywords: KSP, CG, default, monitor, extreme, singular values, Lanczos, Arnoldi

.seealso: KSPComputeExtremeSingularValues()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorSingularValue(KSP ksp,PetscInt n,PetscReal rnorm,void *dummy)
{
  PetscReal               emin,emax,c;
  PetscErrorCode          ierr;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)ksp)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
  if (!ksp->calc_sings) {
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm %14.12e \n",n,rnorm);CHKERRQ(ierr);
  } else {
    ierr = KSPComputeExtremeSingularValues(ksp,&emax,&emin);CHKERRQ(ierr);
    c = emax/emin;
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm %14.12e %% max %G min %G max/min %G\n",n,rnorm,emax,emin,c);CHKERRQ(ierr);
  }
  if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorSolution"
/*@C
   KSPMonitorSolution - Monitors progress of the KSP solvers by calling 
   VecView() for the approximate solution at each iteration.

   Collective on KSP

   Input Parameters:
+  ksp - the KSP context
.  its - iteration number
.  fgnorm - 2-norm of residual (or gradient)
-  dummy - either a viewer or PETSC_NULL

   Level: intermediate

   Notes:
    For some Krylov methods such as GMRES constructing the solution at
  each iteration is expensive, hence using this will slow the code.

.keywords: KSP, nonlinear, vector, monitor, view

.seealso: KSPMonitorSet(), KSPMonitorDefault(), VecView()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorSolution(KSP ksp,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec            x;
  PetscViewer    viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  ierr = KSPBuildSolution(ksp,PETSC_NULL,&x);CHKERRQ(ierr);
  if (!viewer) {
    MPI_Comm comm;
    ierr   = PetscObjectGetComm((PetscObject)ksp,&comm);CHKERRQ(ierr);
    viewer = PETSC_VIEWER_DRAW_(comm);
  }
  ierr = VecView(x,viewer);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorDefault"
/*@C
   KSPMonitorDefault - Print the residual norm at each iteration of an
   iterative solver.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).  
-  dummy - unused monitor context 

   Level: intermediate

.keywords: KSP, default, monitor, residual

.seealso: KSPMonitorSet(), KSPMonitorTrueResidualNorm(), KSPMonitorLGCreate()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorDefault(KSP ksp,PetscInt n,PetscReal rnorm,void *dummy)
{
  PetscErrorCode          ierr;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;

  PetscFunctionBegin;
  if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)ksp)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
  if (n == 0 && ((PetscObject)ksp)->prefix) {
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"  Residual norms for %s solve.\n",((PetscObject)ksp)->prefix);CHKERRQ(ierr);
  }
  ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm %14.12e \n",n,rnorm);CHKERRQ(ierr);
  if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorTrueResidualNorm"
/*@C
   KSPMonitorTrueResidualNorm - Prints the true residual norm as well as the preconditioned
   residual norm at each iteration of an iterative solver.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).  
-  dummy - unused monitor context 

   Options Database Key:
.  -ksp_monitor_true_residual - Activates KSPMonitorTrueResidualNorm()

   Notes:
   When using right preconditioning, these values are equivalent.

   Level: intermediate

.keywords: KSP, default, monitor, residual

.seealso: KSPMonitorSet(), KSPMonitorDefault(), KSPMonitorLGCreate()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorTrueResidualNorm(KSP ksp,PetscInt n,PetscReal rnorm,void *dummy)
{
  PetscErrorCode          ierr;
  Vec                     resid,work;
  PetscReal               scnorm,bnorm;
  PC                      pc;
  Mat                     A,B;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;
  
  PetscFunctionBegin;
  if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)ksp)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
  ierr = VecDuplicate(ksp->vec_rhs,&work);CHKERRQ(ierr);
  ierr = KSPBuildResidual(ksp,0,work,&resid);CHKERRQ(ierr);

  /*
     Unscale the residual but only if both matrices are the same matrix, since only then would 
    they be scaled.
  */
  ierr = VecCopy(resid,work);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCGetOperators(pc,&A,&B,PETSC_NULL);CHKERRQ(ierr);
  if (A == B) {
    ierr = MatUnScaleSystem(A,work,PETSC_NULL);CHKERRQ(ierr);
  }
  ierr = VecNorm(work,NORM_2,&scnorm);CHKERRQ(ierr);
  ierr = VecDestroy(work);CHKERRQ(ierr);
  ierr = VecNorm(ksp->vec_rhs,NORM_2,&bnorm);CHKERRQ(ierr);
  ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP preconditioned resid norm %14.12e true resid norm %14.12e ||Ae||/||Ax|| %14.12e\n",n,rnorm,scnorm,scnorm/bnorm);CHKERRQ(ierr);
  if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorRange_Private"
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorRange_Private(KSP ksp,PetscInt it,PetscReal *per)
{
  PetscErrorCode          ierr;
  Vec                     resid,work;
  PetscReal               rmax,pwork;
  PC                      pc;
  Mat                     A,B;
  PetscInt                i,n,N;
  PetscScalar             *r;

  PetscFunctionBegin;
  ierr = VecDuplicate(ksp->vec_rhs,&work);CHKERRQ(ierr);
  ierr = KSPBuildResidual(ksp,0,work,&resid);CHKERRQ(ierr);

  /*
     Unscale the residual if the matrix is, but only if both matrices are the same matrix, since only then would 
    they be scaled.
  */
  ierr = VecCopy(resid,work);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCGetOperators(pc,&A,&B,PETSC_NULL);CHKERRQ(ierr);
  if (A == B) {
    ierr = MatUnScaleSystem(A,work,PETSC_NULL);CHKERRQ(ierr);
  }
  ierr = VecNorm(work,NORM_INFINITY,&rmax);CHKERRQ(ierr);
  ierr = VecGetLocalSize(work,&n);CHKERRQ(ierr);
  ierr = VecGetSize(work,&N);CHKERRQ(ierr);
  ierr = VecGetArray(work,&r);CHKERRQ(ierr);
  pwork = 0.0;
  for (i=0; i<n; i++) {
    pwork += (PetscAbsScalar(r[i]) > .20*rmax); 
  }
  ierr = MPI_Allreduce(&pwork,per,1,MPIU_REAL,MPIU_SUM,((PetscObject)ksp)->comm);CHKERRQ(ierr);
  ierr = VecRestoreArray(work,&r);CHKERRQ(ierr);
  ierr = VecDestroy(work);CHKERRQ(ierr);
  *per  = *per/N;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorRange"
/*@C
   KSPMonitorRange - Prints the percentage of residual elements that are more then 10 percent of the maximum value.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  it    - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).  
-  dummy - unused monitor context 

   Options Database Key:
.  -ksp_monitor_range - Activates KSPMonitorRange()

   Level: intermediate

.keywords: KSP, default, monitor, residual

.seealso: KSPMonitorSet(), KSPMonitorDefault(), KSPMonitorLGCreate()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorRange(KSP ksp,PetscInt it,PetscReal rnorm,void *dummy)
{
  PetscErrorCode          ierr;
  PetscReal               perc,rel;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;
  /* should be in a MonitorRangeContext */
  static PetscReal        prev;

  PetscFunctionBegin;
  if (!it) prev = rnorm;
  if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)ksp)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}
  ierr = KSPMonitorRange_Private(ksp,it,&perc);CHKERRQ(ierr);

  rel  = (prev - rnorm)/prev;
  prev = rnorm;
  ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP preconditioned resid norm %14.12e Percent values above 20 percent of maximum %5.2f relative decrease %5.2e ratio %5.2e \n",it,rnorm,100.0*perc,rel,rel/perc);CHKERRQ(ierr);
  if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPMonitorDefaultShort"
/*
  Default (short) KSP Monitor, same as KSPMonitorDefault() except
  it prints fewer digits of the residual as the residual gets smaller.
  This is because the later digits are meaningless and are often 
  different on different machines; by using this routine different 
  machines will usually generate the same output.
*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPMonitorDefaultShort(KSP ksp,PetscInt its,PetscReal fnorm,void *dummy)
{
  PetscErrorCode          ierr;
  PetscViewerASCIIMonitor viewer = (PetscViewerASCIIMonitor) dummy;

  PetscFunctionBegin;
  if (!dummy) {ierr = PetscViewerASCIIMonitorCreate(((PetscObject)ksp)->comm,"stdout",0,&viewer);CHKERRQ(ierr);}

  if (fnorm > 1.e-9) {
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm %G \n",its,fnorm);CHKERRQ(ierr);
  } else if (fnorm > 1.e-11){
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm %5.3e \n",its,fnorm);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIMonitorPrintf(viewer,"%3D KSP Residual norm < 1.e-11\n",its);CHKERRQ(ierr);
  }
  if (!dummy) {ierr = PetscViewerASCIIMonitorDestroy(viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPSkipConverged"
/*@C
   KSPSkipConverged - Convergence test that do not return as converged
   until the maximum number of iterations is reached.

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm residual value (may be estimated)
-  dummy - unused convergence context 

   Returns:
.  reason - KSP_CONVERGED_ITERATING, KSP_CONVERGED_ITS

   Notes: 
   This should be used as the convergence test with the option
   KSPSetNormType(ksp,KSP_NORM_NO), since norms of the residual are
   not computed. Convergence is then declared after the maximum number
   of iterations have been reached. Useful when one is using CG or
   BiCGStab as a smoother.
                    
   Level: advanced

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSetNormType()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPSkipConverged(KSP ksp,PetscInt n,PetscReal rnorm,KSPConvergedReason *reason,void *dummy)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidPointer(reason,4);
  *reason = KSP_CONVERGED_ITERATING;
  if (n >= ksp->max_it) *reason = KSP_CONVERGED_ITS;
  PetscFunctionReturn(0);
}

typedef struct {
  PetscTruth initialrtol;    /* default relative residual decrease is computing from initial residual, not rhs */
  PetscTruth mininitialrtol; /* default relative residual decrease is computing from min of initial residual and rhs */
  Vec        work;
} KSPDefaultConvergedCtx;

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConvergedCreate"
/*@C
   KSPDefaultConvergedCreate - Creates and initializes the space used by the KSPDefaultConverged() function context

   Collective on KSP

   Output Parameter:
.  ctx - convergence context 

   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPDefaultConverged(), KSPDefaultConvergedDestroy(), KSPSetConvergenceTest(), KSPSetTolerances(),
          KSPSkipConverged(), KSPConvergedReason, KSPGetConvergedReason(), KSPDefaultConvergedSetUIRNorm(), KSPDefaultConvergedSetUMIRNorm()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPDefaultConvergedCreate(void **ctx)
{
  PetscErrorCode         ierr;
  KSPDefaultConvergedCtx *cctx;

  PetscFunctionBegin;
  ierr = PetscNew(KSPDefaultConvergedCtx,&cctx);CHKERRQ(ierr);
  *ctx = cctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConvergedSetUIRNorm"
/*@
   KSPDefaultConvergedSetUIRNorm - makes the default convergence test use || B*(b - A*(initial guess))||
      instead of || B*b ||. In the case of right preconditioner or if KSPSetNormType(ksp,KSP_NORM_UNPRECONDIITONED)
      is used there is no B in the above formula. UIRNorm is short for Use Initial Residual Norm.

   Collective on KSP

   Input Parameters:
.  ksp   - iterative context

   Options Database:
.   -ksp_converged_use_initial_residual_norm

   Use KSPSetTolerances() to alter the defaults for rtol, abstol, dtol.

   The precise values of reason are macros such as KSP_CONVERGED_RTOL, which
   are defined in petscksp.h.

   If the convergence test is not KSPDefaultConverged() then this is ignored.
   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSkipConverged(), KSPConvergedReason, KSPGetConvergedReason(), KSPDefaultConvergedSetUMIRNorm()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPDefaultConvergedSetUIRNorm(KSP ksp)
{
  KSPDefaultConvergedCtx *ctx = (KSPDefaultConvergedCtx*) ksp->cnvP;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (ksp->converged != KSPDefaultConverged) PetscFunctionReturn(0);
  if (ctx->mininitialrtol) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Cannot use KSPDefaultConvergedSetUIRNorm() and KSPDefaultConvergedSetUMIRNorm() together");
  ctx->initialrtol = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConvergedSetUMIRNorm"
/*@
   KSPDefaultConvergedSetUMIRNorm - makes the default convergence test use min(|| B*(b - A*(initial guess))||,|| B*b ||)
      In the case of right preconditioner or if KSPSetNormType(ksp,KSP_NORM_UNPRECONDIITONED)
      is used there is no B in the above formula. UMIRNorm is short for Use Minimum Initial Residual Norm.

   Collective on KSP

   Input Parameters:
.  ksp   - iterative context

   Options Database:
.   -ksp_converged_use_min_initial_residual_norm

   Use KSPSetTolerances() to alter the defaults for rtol, abstol, dtol.

   The precise values of reason are macros such as KSP_CONVERGED_RTOL, which
   are defined in petscksp.h.

   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSkipConverged(), KSPConvergedReason, KSPGetConvergedReason(), KSPDefaultConvergedSetUIRNorm()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPDefaultConvergedSetUMIRNorm(KSP ksp)
{
  KSPDefaultConvergedCtx *ctx = (KSPDefaultConvergedCtx*) ksp->cnvP;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  if (ksp->converged != KSPDefaultConverged) PetscFunctionReturn(0);
  if (ctx->initialrtol) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Cannot use KSPDefaultConvergedSetUIRNorm() and KSPDefaultConvergedSetUMIRNorm() together");
  ctx->mininitialrtol = PETSC_TRUE;
   PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConverged"
/*@C
   KSPDefaultConverged - Determines convergence of
   the iterative solvers (default code).

   Collective on KSP

   Input Parameters:
+  ksp   - iterative context
.  n     - iteration number
.  rnorm - 2-norm residual value (may be estimated)
-  ctx - convergence context which must be created by KSPDefaultConvergedCreate()

   reason is set to:
+   positive - if the iteration has converged;
.   negative - if residual norm exceeds divergence threshold;
-   0 - otherwise.

   Notes:
   KSPDefaultConverged() reaches convergence when
$      rnorm < MAX (rtol * rnorm_0, abstol);
   Divergence is detected if
$      rnorm > dtol * rnorm_0,

   where 
+     rtol = relative tolerance,
.     abstol = absolute tolerance.
.     dtol = divergence tolerance,
-     rnorm_0 is the two norm of the right hand side. When initial guess is non-zero you 
          can call KSPDefaultConvergedSetUIRNorm() to use the norm of (b - A*(initial guess))
          as the starting point for relative norm convergence testing.

   Use KSPSetTolerances() to alter the defaults for rtol, abstol, dtol.

   The precise values of reason are macros such as KSP_CONVERGED_RTOL, which
   are defined in petscksp.h.

   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPSetConvergenceTest(), KSPSetTolerances(), KSPSkipConverged(), KSPConvergedReason, KSPGetConvergedReason(),
          KSPDefaultConvergedSetUIRNorm(), KSPDefaultConvergedSetUMIRNorm(), KSPDefaultConvergedCreate(), KSPDefaultConvergedDestroy()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPDefaultConverged(KSP ksp,PetscInt n,PetscReal rnorm,KSPConvergedReason *reason,void *ctx)
{
  PetscErrorCode         ierr;
  KSPDefaultConvergedCtx *cctx = (KSPDefaultConvergedCtx*) ctx;
  KSPNormType            normtype;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidPointer(reason,4);
  *reason = KSP_CONVERGED_ITERATING;
  
  ierr = KSPGetNormType(ksp,&normtype);CHKERRQ(ierr);
  if (normtype == KSP_NORM_NO) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Use KSPSkipConverged() with KSPNormType of KSP_NORM_NO");

  if (!cctx) SETERRQ(PETSC_ERR_ARG_NULL,"Convergence context must have been created with KSPDefaultConvergedCreate()");
  if (!n) {
    /* if user gives initial guess need to compute norm of b */
    if (!ksp->guess_zero && !cctx->initialrtol) {
      PetscReal      snorm;
      if (ksp->normtype == KSP_NORM_UNPRECONDITIONED || ksp->pc_side == PC_RIGHT) {
        ierr = PetscInfo(ksp,"user has provided nonzero initial guess, computing 2-norm of RHS\n");CHKERRQ(ierr);
        ierr = VecNorm(ksp->vec_rhs,NORM_2,&snorm);CHKERRQ(ierr);        /*     <- b'*b */
      } else {
        Vec z;
        if (!cctx->work) {
          ierr = VecDuplicate(ksp->vec_rhs,&cctx->work);CHKERRQ(ierr);
        }
        z = cctx->work;
        ierr = KSP_PCApply(ksp,ksp->vec_rhs,z);CHKERRQ(ierr);
        if (ksp->normtype == KSP_NORM_PRECONDITIONED) {
          ierr = PetscInfo(ksp,"user has provided nonzero initial guess, computing 2-norm of preconditioned RHS\n");CHKERRQ(ierr);
          ierr = VecNorm(z,NORM_2,&snorm);CHKERRQ(ierr);                 /*    dp <- b'*B'*B*b */
        } else if (ksp->normtype == KSP_NORM_NATURAL) {
          PetscScalar norm;
           ierr = PetscInfo(ksp,"user has provided nonzero initial guess, computing natural norm of RHS\n");CHKERRQ(ierr);
          ierr  = VecDot(ksp->vec_rhs,z,&norm);
          snorm = sqrt(PetscAbsScalar(norm));                            /*    dp <- b'*B*b */
        }
      }
      /* handle special case of zero RHS and nonzero guess */
      if (!snorm) {
        ierr = PetscInfo(ksp,"Special case, user has provided nonzero initial guess and zero RHS\n");CHKERRQ(ierr);
        snorm = rnorm;
      }
      if (cctx->mininitialrtol) {
        ksp->rnorm0 = PetscMin(snorm,rnorm);
      } else {
        ksp->rnorm0 = snorm;
      }
    } else {
      ksp->rnorm0 = rnorm;
    }
    ksp->ttol   = PetscMax(ksp->rtol*ksp->rnorm0,ksp->abstol);
  }

  if (n <= ksp->chknorm) PetscFunctionReturn(0);

  if (PetscIsInfOrNanScalar(rnorm)) {
    ierr = PetscInfo(ksp,"Linear solver has created a not a number (NaN) as the residual norm, declaring divergence \n");CHKERRQ(ierr);
    *reason = KSP_DIVERGED_NAN;
  } else if (rnorm <= ksp->ttol) {
    if (rnorm < ksp->abstol) {
      ierr = PetscInfo3(ksp,"Linear solver has converged. Residual norm %G is less than absolute tolerance %G at iteration %D\n",rnorm,ksp->abstol,n);CHKERRQ(ierr);
      *reason = KSP_CONVERGED_ATOL;
    } else {
      if (cctx->initialrtol) {
        ierr = PetscInfo4(ksp,"Linear solver has converged. Residual norm %G is less than relative tolerance %G times initial residual norm %G at iteration %D\n",rnorm,ksp->rtol,ksp->rnorm0,n);CHKERRQ(ierr);
      } else {
        ierr = PetscInfo4(ksp,"Linear solver has converged. Residual norm %G is less than relative tolerance %G times initial right hand side norm %G at iteration %D\n",rnorm,ksp->rtol,ksp->rnorm0,n);CHKERRQ(ierr);
      }
      *reason = KSP_CONVERGED_RTOL;
    }
  } else if (rnorm >= ksp->divtol*ksp->rnorm0) {
    ierr = PetscInfo3(ksp,"Linear solver is diverging. Initial right hand size norm %G, current residual norm %G at iteration %D\n",ksp->rnorm0,rnorm,n);CHKERRQ(ierr);
    *reason = KSP_DIVERGED_DTOL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultConvergedDestroy"
/*@C
   KSPDefaultConvergedDestroy - Frees the space used by the KSPDefaultConverged() function context

   Collective on KSP

   Input Parameters:
.  ctx - convergence context 

   Level: intermediate

.keywords: KSP, default, convergence, residual

.seealso: KSPDefaultConverged(), KSPDefaultConvergedCreate(), KSPSetConvergenceTest(), KSPSetTolerances(), KSPSkipConverged(),
          KSPConvergedReason, KSPGetConvergedReason(), KSPDefaultConvergedSetUIRNorm(), KSPDefaultConvergedSetUMIRNorm()
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPDefaultConvergedDestroy(void *ctx)
{
  PetscErrorCode         ierr;
  KSPDefaultConvergedCtx *cctx = (KSPDefaultConvergedCtx*) ctx;

  PetscFunctionBegin;
  if (cctx->work) {ierr = VecDestroy(cctx->work);CHKERRQ(ierr);}
  ierr = PetscFree(cctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultBuildSolution"
/*
   KSPDefaultBuildSolution - Default code to create/move the solution.

   Input Parameters:
+  ksp - iterative context
-  v   - pointer to the user's vector  

   Output Parameter:
.  V - pointer to a vector containing the solution

   Level: advanced

.keywords:  KSP, build, solution, default

.seealso: KSPGetSolution(), KSPDefaultBuildResidual()
*/
PetscErrorCode KSPDefaultBuildSolution(KSP ksp,Vec v,Vec *V)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (ksp->pc_side == PC_RIGHT) {
    if (ksp->pc) {
      if (v) {ierr = KSP_PCApply(ksp,ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else {SETERRQ(PETSC_ERR_SUP,"Not working with right preconditioner");}
    } else {
      if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else { *V = ksp->vec_sol;}
    }
  } else if (ksp->pc_side == PC_SYMMETRIC) {
    if (ksp->pc) {
      if (ksp->transpose_solve) SETERRQ(PETSC_ERR_SUP,"Not working with symmetric preconditioner and transpose solve");
      if (v) {ierr = PCApplySymmetricRight(ksp->pc,ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else {SETERRQ(PETSC_ERR_SUP,"Not working with symmetric preconditioner");}
    } else  {
      if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
      else { *V = ksp->vec_sol;}
    }
  } else {
    if (v) {ierr = VecCopy(ksp->vec_sol,v);CHKERRQ(ierr); *V = v;}
    else { *V = ksp->vec_sol; }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultBuildResidual"
/*
   KSPDefaultBuildResidual - Default code to compute the residual.

   Input Parameters:
.  ksp - iterative context
.  t   - pointer to temporary vector
.  v   - pointer to user vector  

   Output Parameter:
.  V - pointer to a vector containing the residual

   Level: advanced

.keywords:  KSP, build, residual, default

.seealso: KSPDefaultBuildSolution()
*/
PetscErrorCode KSPDefaultBuildResidual(KSP ksp,Vec t,Vec v,Vec *V)
{
  PetscErrorCode ierr;
  MatStructure   pflag;
  Mat            Amat,Pmat;

  PetscFunctionBegin;
  if (!ksp->pc) {ierr = KSPGetPC(ksp,&ksp->pc);CHKERRQ(ierr);}
  ierr = PCGetOperators(ksp->pc,&Amat,&Pmat,&pflag);CHKERRQ(ierr);
  ierr = KSPBuildSolution(ksp,t,PETSC_NULL);CHKERRQ(ierr);
  ierr = KSP_MatMult(ksp,Amat,t,v);CHKERRQ(ierr);
  ierr = VecAYPX(v,-1.0,ksp->vec_rhs);CHKERRQ(ierr);
  *V = v;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetVecs"
/*@C
  KSPGetVecs - Gets a number of work vectors.

  Input Parameters:
+ ksp  - iterative context
. rightn  - number of right work vectors
- leftn   - number of left work vectors to allocate

  Output Parameter:
+  right - the array of vectors created
-  left - the array of left vectors

   Note: The right vector has as many elements as the matrix has columns. The left
     vector has as many elements as the matrix has rows.

   Level: advanced

.seealso:   MatGetVecs()

@*/
PetscErrorCode KSPGetVecs(KSP ksp,PetscInt rightn, Vec **right,PetscInt leftn,Vec **left)
{
  PetscErrorCode ierr;
  Vec            vecr,vecl;

  PetscFunctionBegin;
  if (rightn) {
    if (!right) SETERRQ(PETSC_ERR_ARG_INCOMP,"You asked for right vectors but did not pass a pointer to hold them");
    if (ksp->vec_sol) vecr = ksp->vec_sol;
    else {
      Mat pmat;
      if (!ksp->pc) {ierr = KSPGetPC(ksp,&ksp->pc);CHKERRQ(ierr);}
      ierr = PCGetOperators(ksp->pc,PETSC_NULL,&pmat,PETSC_NULL);CHKERRQ(ierr);
      ierr = MatGetVecs(pmat,&vecr,PETSC_NULL);CHKERRQ(ierr);
    }
    ierr = VecDuplicateVecs(vecr,rightn,right);CHKERRQ(ierr);
    if (!ksp->vec_sol) {
      ierr = VecDestroy(vecr);CHKERRQ(ierr);
    }
  }
  if (leftn) {
    if (!left) SETERRQ(PETSC_ERR_ARG_INCOMP,"You asked for left vectors but did not pass a pointer to hold them");
    if (ksp->vec_rhs) vecl = ksp->vec_rhs;
    else {
      Mat pmat;
      if (!ksp->pc) {ierr = KSPGetPC(ksp,&ksp->pc);CHKERRQ(ierr);}
      ierr = PCGetOperators(ksp->pc,PETSC_NULL,&pmat,PETSC_NULL);CHKERRQ(ierr);
      ierr = MatGetVecs(pmat,PETSC_NULL,&vecl);CHKERRQ(ierr);
    }
    ierr = VecDuplicateVecs(vecl,leftn,left);CHKERRQ(ierr);
    if (!ksp->vec_rhs) {
      ierr = VecDestroy(vecl);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultGetWork"
/*
  KSPDefaultGetWork - Gets a number of work vectors.

  Input Parameters:
. ksp  - iterative context
. nw   - number of work vectors to allocate

  Notes:
  Call this only if no work vectors have been allocated 
 */
PetscErrorCode KSPDefaultGetWork(KSP ksp,PetscInt nw)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ksp->work) {ierr = KSPDefaultFreeWork(ksp);CHKERRQ(ierr);}
  ksp->nwork = nw;
  ierr = KSPGetVecs(ksp,nw,&ksp->work,0,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(ksp,nw,ksp->work);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPDefaultDestroy"
/*
  KSPDefaultDestroy - Destroys a iterative context variable for methods with
  no separate context.  Preferred calling sequence KSPDestroy().

  Input Parameter: 
. ksp - the iterative context
*/
PetscErrorCode KSPDefaultDestroy(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  /* free work vectors */
  ierr = KSPDefaultFreeWork(ksp);CHKERRQ(ierr);
  /* free private data structure */
  ierr = PetscFree(ksp->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPGetConvergedReason"
/*@
   KSPGetConvergedReason - Gets the reason the KSP iteration was stopped.

   Not Collective

   Input Parameter:
.  ksp - the KSP context

   Output Parameter:
.  reason - negative value indicates diverged, positive value converged, see KSPConvergedReason

   Possible values for reason:
+  KSP_CONVERGED_RTOL (residual 2-norm decreased by a factor of rtol, from 2-norm of right hand side)
.  KSP_CONVERGED_ATOL (residual 2-norm less than abstol)
.  KSP_CONVERGED_ITS (used by the preonly preconditioner that always uses ONE iteration, or when the KSPSkipConverged() convergence 
           test routine is set.
.  KSP_CONVERGED_CG_NEG_CURVE
.  KSP_CONVERGED_CG_CONSTRAINED
.  KSP_CONVERGED_STEP_LENGTH
.  KSP_DIVERGED_ITS  (required more than its to reach convergence)
.  KSP_DIVERGED_DTOL (residual norm increased by a factor of divtol)
.  KSP_DIVERGED_NAN (residual norm became Not-a-number likely due to 0/0)
.  KSP_DIVERGED_BREAKDOWN (generic breakdown in method)
-  KSP_DIVERGED_BREAKDOWN_BICG (Initial residual is orthogonal to preconditioned initial
                                residual. Try a different preconditioner, or a different initial Level.)
 
   See also manual page for each reason.

   guess: beginner

   Notes: Can only be called after the call the KSPSolve() is complete.

   Level: intermediate
 
.keywords: KSP, nonlinear, set, convergence, test

.seealso: KSPSetConvergenceTest(), KSPDefaultConverged(), KSPSetTolerances(), KSPConvergedReason
@*/
PetscErrorCode PETSCKSP_DLLEXPORT KSPGetConvergedReason(KSP ksp,KSPConvergedReason *reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidPointer(reason,2);
  *reason = ksp->reason;
  PetscFunctionReturn(0);
}

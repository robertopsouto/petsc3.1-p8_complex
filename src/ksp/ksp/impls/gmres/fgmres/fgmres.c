#define PETSCKSP_DLL

/*
    This file implements FGMRES (a Generalized Minimal Residual) method.  
    Reference:  Saad, 1993.

    Preconditioning:  If the preconditioner is constant then this fgmres
    code is equivalent to RIGHT-PRECONDITIONED GMRES.
    FGMRES is a modification of gmres that allows the preconditioner to change
    at each iteration.

    Restarts:  Restarts are basically solves with x0 not equal to zero.
 
       Contributed by Allison Baker

*/

#include "../src/ksp/ksp/impls/gmres/fgmres/fgmresimpl.h"       /*I  "petscksp.h"  I*/
#define FGMRES_DELTA_DIRECTIONS 10
#define FGMRES_DEFAULT_MAXK     30
static PetscErrorCode FGMRESGetNewVectors(KSP,PetscInt);
static PetscErrorCode FGMRESUpdateHessenberg(KSP,PetscInt,PetscTruth,PetscReal *);
static PetscErrorCode BuildFgmresSoln(PetscScalar*,Vec,Vec,KSP,PetscInt);

EXTERN PetscErrorCode KSPView_GMRES(KSP,PetscViewer);
EXTERN PetscErrorCode KSPSetUp_GMRES(KSP);
/*

    KSPSetUp_FGMRES - Sets up the workspace needed by fgmres.

    This is called once, usually automatically by KSPSolve() or KSPSetUp(),
    but can be called directly by KSPSetUp().

*/
#undef __FUNCT__  
#define __FUNCT__ "KSPSetUp_FGMRES"
PetscErrorCode    KSPSetUp_FGMRES(KSP ksp)
{
  PetscErrorCode ierr;
  PetscInt       max_k,k;
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)ksp->data;

  PetscFunctionBegin;
  if (ksp->pc_side == PC_SYMMETRIC) {
    SETERRQ(PETSC_ERR_SUP,"no symmetric preconditioning for KSPFGMRES");
  } else if (ksp->pc_side == PC_LEFT) {
    SETERRQ(PETSC_ERR_SUP,"no left preconditioning for KSPFGMRES");
  }
  max_k         = fgmres->max_k;

  ierr = KSPSetUp_GMRES(ksp);CHKERRQ(ierr);

  ierr = PetscMalloc((VEC_OFFSET+2+max_k)*sizeof(void*),&fgmres->prevecs);CHKERRQ(ierr);
  ierr = PetscMalloc((VEC_OFFSET+2+max_k)*sizeof(void*),&fgmres->prevecs_user_work);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(ksp,(VEC_OFFSET+2+max_k)*(2*sizeof(void*)));CHKERRQ(ierr);

  ierr = KSPGetVecs(ksp,fgmres->vv_allocated,&fgmres->prevecs_user_work[0],0,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(ksp,fgmres->vv_allocated,fgmres->prevecs_user_work[0]);CHKERRQ(ierr);
  for (k=0; k < fgmres->vv_allocated; k++) {
    fgmres->prevecs[k] = fgmres->prevecs_user_work[0][k];
  } 

  PetscFunctionReturn(0);
}

/* 
    FGMRESResidual - This routine computes the initial residual (NOT PRECONDITIONED) 
*/
#undef __FUNCT__  
#define __FUNCT__ "FGMRESResidual"
static PetscErrorCode FGMRESResidual(KSP ksp)
{
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)(ksp->data);
  Mat            Amat,Pmat;
  MatStructure   pflag;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PCGetOperators(ksp->pc,&Amat,&Pmat,&pflag);CHKERRQ(ierr);

  /* put A*x into VEC_TEMP */
  ierr = MatMult(Amat,ksp->vec_sol,VEC_TEMP);CHKERRQ(ierr);
  /* now put residual (-A*x + f) into vec_vv(0) */
  ierr = VecWAXPY(VEC_VV(0),-1.0,VEC_TEMP,ksp->vec_rhs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*

    FGMRESCycle - Run fgmres, possibly with restart.  Return residual 
                  history if requested.

    input parameters:
.	 fgmres  - structure containing parameters and work areas

    output parameters:
.        itcount - number of iterations used.  If null, ignored.
.        converged - 0 if not converged

		  
    Notes:
    On entry, the value in vector VEC_VV(0) should be 
    the initial residual.


 */
#undef __FUNCT__  
#define __FUNCT__ "FGMREScycle"
PetscErrorCode FGMREScycle(PetscInt *itcount,KSP ksp)
{

  KSP_FGMRES     *fgmres = (KSP_FGMRES *)(ksp->data);
  PetscReal      res_norm;             
  PetscReal      hapbnd,tt;
  PetscTruth     hapend = PETSC_FALSE;  /* indicates happy breakdown ending */
  PetscErrorCode ierr;
  PetscInt       loc_it;                /* local count of # of dir. in Krylov space */ 
  PetscInt       max_k = fgmres->max_k; /* max # of directions Krylov space */
  Mat            Amat,Pmat;
  MatStructure   pflag;

  PetscFunctionBegin;

  /* Number of pseudo iterations since last restart is the number 
     of prestart directions */
  loc_it = 0;

  /* note: (fgmres->it) is always set one less than (loc_it) It is used in 
     KSPBUILDSolution_FGMRES, where it is passed to BuildFGmresSoln.  
     Note that when BuildFGmresSoln is called from this function, 
     (loc_it -1) is passed, so the two are equivalent */
  fgmres->it = (loc_it - 1);

  /* initial residual is in VEC_VV(0)  - compute its norm*/ 
  ierr   = VecNorm(VEC_VV(0),NORM_2,&res_norm);CHKERRQ(ierr);

  /* first entry in right-hand-side of hessenberg system is just 
     the initial residual norm */
  *RS(0) = res_norm;

  ksp->rnorm = res_norm;
  KSPLogResidualHistory(ksp,res_norm);

  /* check for the convergence - maybe the current guess is good enough */
  ierr = (*ksp->converged)(ksp,ksp->its,res_norm,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
  if (ksp->reason) {
    if (itcount) *itcount = 0;
    PetscFunctionReturn(0);
  }

  /* scale VEC_VV (the initial residual) */
  ierr = VecScale(VEC_VV(0),1.0/res_norm);CHKERRQ(ierr);
   
  /* MAIN ITERATION LOOP BEGINNING*/
  /* keep iterating until we have converged OR generated the max number
     of directions OR reached the max number of iterations for the method */ 
  while (!ksp->reason && loc_it < max_k && ksp->its < ksp->max_it) {
    if (loc_it) KSPLogResidualHistory(ksp,res_norm);
    fgmres->it = (loc_it - 1);
    KSPMonitor(ksp,ksp->its,res_norm); 

    /* see if more space is needed for work vectors */
    if (fgmres->vv_allocated <= loc_it + VEC_OFFSET + 1) {
      ierr = FGMRESGetNewVectors(ksp,loc_it+1);CHKERRQ(ierr);
      /* (loc_it+1) is passed in as number of the first vector that should
         be allocated */
    }

    /* CHANGE THE PRECONDITIONER? */ 
    /* ModifyPC is the callback function that can be used to
       change the PC or its attributes before its applied */
    (*fgmres->modifypc)(ksp,ksp->its,loc_it,res_norm,fgmres->modifyctx);
   
  
    /* apply PRECONDITIONER to direction vector and store with 
       preconditioned vectors in prevec */
    ierr = KSP_PCApply(ksp,VEC_VV(loc_it),PREVEC(loc_it));CHKERRQ(ierr);
     
    ierr = PCGetOperators(ksp->pc,&Amat,&Pmat,&pflag);CHKERRQ(ierr);
    /* Multiply preconditioned vector by operator - put in VEC_VV(loc_it+1) */
    ierr = MatMult(Amat,PREVEC(loc_it),VEC_VV(1+loc_it));CHKERRQ(ierr);

 
    /* update hessenberg matrix and do Gram-Schmidt - new direction is in
       VEC_VV(1+loc_it)*/
    ierr = (*fgmres->orthog)(ksp,loc_it);CHKERRQ(ierr);

    /* new entry in hessenburg is the 2-norm of our new direction */
    ierr = VecNorm(VEC_VV(loc_it+1),NORM_2,&tt);CHKERRQ(ierr);
    *HH(loc_it+1,loc_it)   = tt;
    *HES(loc_it+1,loc_it)  = tt;

    /* Happy Breakdown Check */
    hapbnd  = PetscAbsScalar((tt) / *RS(loc_it));
    /* RS(loc_it) contains the res_norm from the last iteration  */
    hapbnd = PetscMin(fgmres->haptol,hapbnd);
    if (tt > hapbnd) {
        /* scale new direction by its norm */
        ierr = VecScale(VEC_VV(loc_it+1),1.0/tt);CHKERRQ(ierr);
    } else {
        /* This happens when the solution is exactly reached. */
        /* So there is no new direction... */
          ierr   = VecSet(VEC_TEMP,0.0);CHKERRQ(ierr); /* set VEC_TEMP to 0 */
          hapend = PETSC_TRUE;
    }
    /* note that for FGMRES we could get HES(loc_it+1, loc_it)  = 0 and the
       current solution would not be exact if HES was singular.  Note that 
       HH non-singular implies that HES is no singular, and HES is guaranteed
       to be nonsingular when PREVECS are linearly independent and A is 
       nonsingular (in GMRES, the nonsingularity of A implies the nonsingularity 
       of HES). So we should really add a check to verify that HES is nonsingular.*/

 
    /* Now apply rotations to new col of hessenberg (and right side of system), 
       calculate new rotation, and get new residual norm at the same time*/
    ierr = FGMRESUpdateHessenberg(ksp,loc_it,hapend,&res_norm);CHKERRQ(ierr);
    if (ksp->reason) break;

    loc_it++;
    fgmres->it  = (loc_it-1);  /* Add this here in case it has converged */
 
    ierr = PetscObjectTakeAccess(ksp);CHKERRQ(ierr);
    ksp->its++;
    ksp->rnorm = res_norm;
    ierr = PetscObjectGrantAccess(ksp);CHKERRQ(ierr);

    ierr = (*ksp->converged)(ksp,ksp->its,res_norm,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);

    /* Catch error in happy breakdown and signal convergence and break from loop */
    if (hapend) {
      if (!ksp->reason) {
        SETERRQ(0,"You reached the happy break down,but convergence was not indicated.");
      }
      break;
    }
  }
  /* END OF ITERATION LOOP */

  KSPLogResidualHistory(ksp,res_norm);

  /*
     Monitor if we know that we will not return for a restart */
  if (ksp->reason || ksp->its >= ksp->max_it) {
    KSPMonitor(ksp,ksp->its,res_norm);
  }

  if (itcount) *itcount    = loc_it;

  /*
    Down here we have to solve for the "best" coefficients of the Krylov
    columns, add the solution values together, and possibly unwind the
    preconditioning from the solution
   */
 
  /* Form the solution (or the solution so far) */
  /* Note: must pass in (loc_it-1) for iteration count so that BuildFgmresSoln
     properly navigates */

  ierr = BuildFgmresSoln(RS(0),ksp->vec_sol,ksp->vec_sol,ksp,loc_it-1);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

/*  
    KSPSolve_FGMRES - This routine applies the FGMRES method.


   Input Parameter:
.     ksp - the Krylov space object that was set to use fgmres

   Output Parameter:
.     outits - number of iterations used

*/
#undef __FUNCT__  
#define __FUNCT__ "KSPSolve_FGMRES"

PetscErrorCode KSPSolve_FGMRES(KSP ksp)
{
  PetscErrorCode ierr;
  PetscInt       cycle_its = 0; /* iterations done in a call to FGMREScycle */
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)ksp->data;
  PetscTruth     diagonalscale;

  PetscFunctionBegin;
  ierr    = PCDiagonalScale(ksp->pc,&diagonalscale);CHKERRQ(ierr);
  if (diagonalscale) SETERRQ1(PETSC_ERR_SUP,"Krylov method %s does not support diagonal scaling",((PetscObject)ksp)->type_name);
  if (ksp->normtype != KSP_NORM_UNPRECONDITIONED) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Can only use FGMRES with unpreconditioned residual (it is coded with right preconditioning)");

  ierr = PetscObjectTakeAccess(ksp);CHKERRQ(ierr);
  ksp->its = 0;
  ierr = PetscObjectGrantAccess(ksp);CHKERRQ(ierr);

  /* Compute the initial (NOT preconditioned) residual */
  if (!ksp->guess_zero) {
    ierr = FGMRESResidual(ksp);CHKERRQ(ierr);
  } else { /* guess is 0 so residual is F (which is in ksp->vec_rhs) */
    ierr = VecCopy(ksp->vec_rhs,VEC_VV(0));CHKERRQ(ierr);
  }
  /* now the residual is in VEC_VV(0) - which is what 
     FGMREScycle expects... */
  
  ierr    = FGMREScycle(&cycle_its,ksp);CHKERRQ(ierr);
  while (!ksp->reason) {
    ierr     = FGMRESResidual(ksp);CHKERRQ(ierr);
    if (ksp->its >= ksp->max_it) break;
    ierr     = FGMREScycle(&cycle_its,ksp);CHKERRQ(ierr);
  }
  /* mark lack of convergence */
  if (ksp->its >= ksp->max_it && !ksp->reason) ksp->reason = KSP_DIVERGED_ITS;

  PetscFunctionReturn(0);
}

extern PetscErrorCode KSPDestroy_GMRES(KSP);
/*

   KSPDestroy_FGMRES - Frees all memory space used by the Krylov method.

*/
#undef __FUNCT__  
#define __FUNCT__ "KSPDestroy_FGMRES" 
PetscErrorCode KSPDestroy_FGMRES(KSP ksp)
{
  KSP_FGMRES     *fgmres = (KSP_FGMRES*)ksp->data;
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  ierr = PetscFree (fgmres->prevecs);CHKERRQ(ierr);
  for (i=0; i<fgmres->nwork_alloc; i++) {
    ierr = VecDestroyVecs(fgmres->prevecs_user_work[i],fgmres->mwork_alloc[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(fgmres->prevecs_user_work);CHKERRQ(ierr);
  if (fgmres->modifydestroy) {
    ierr = (*fgmres->modifydestroy)(fgmres->modifyctx);CHKERRQ(ierr);
  }

  /* clear composed functions */
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPFGMRESSetModifyPC_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = KSPDestroy_GMRES(ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
    BuildFgmresSoln - create the solution from the starting vector and the
                      current iterates.

    Input parameters:
        nrs - work area of size it + 1.
	vguess  - index of initial guess
	vdest - index of result.  Note that vguess may == vdest (replace
	        guess with the solution).
        it - HH upper triangular part is a block of size (it+1) x (it+1)  

     This is an internal routine that knows about the FGMRES internals.
 */
#undef __FUNCT__  
#define __FUNCT__ "BuildFgmresSoln"
static PetscErrorCode BuildFgmresSoln(PetscScalar* nrs,Vec vguess,Vec vdest,KSP ksp,PetscInt it)
{
  PetscScalar    tt;
  PetscErrorCode ierr;
  PetscInt       ii,k,j;
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)(ksp->data);

  PetscFunctionBegin;
  /* Solve for solution vector that minimizes the residual */

  /* If it is < 0, no fgmres steps have been performed */
  if (it < 0) {
    ierr = VecCopy(vguess,vdest);CHKERRQ(ierr); /* VecCopy() is smart, exists immediately if vguess == vdest */
    PetscFunctionReturn(0);
  }

  /* so fgmres steps HAVE been performed */

  /* solve the upper triangular system - RS is the right side and HH is 
     the upper triangular matrix  - put soln in nrs */
  if (*HH(it,it) != 0.0) {
    nrs[it] = *RS(it) / *HH(it,it);
  } else {
    nrs[it] = 0.0;
  }
  for (ii=1; ii<=it; ii++) {
    k   = it - ii;
    tt  = *RS(k);
    for (j=k+1; j<=it; j++) tt  = tt - *HH(k,j) * nrs[j];
    nrs[k]   = tt / *HH(k,k);
  }

  /* Accumulate the correction to the soln of the preconditioned prob. in 
     VEC_TEMP - note that we use the preconditioned vectors  */
  ierr = VecSet(VEC_TEMP,0.0);CHKERRQ(ierr); /* set VEC_TEMP components to 0 */
  ierr = VecMAXPY(VEC_TEMP,it+1,nrs,&PREVEC(0));CHKERRQ(ierr); 

  /* put updated solution into vdest.*/
  if (vdest != vguess) {
    ierr = VecCopy(VEC_TEMP,vdest);CHKERRQ(ierr);
    ierr = VecAXPY(vdest,1.0,vguess);CHKERRQ(ierr);
  } else  {/* replace guess with solution */
    ierr = VecAXPY(vdest,1.0,VEC_TEMP);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*

    FGMRESUpdateHessenberg - Do the scalar work for the orthogonalization.  
                            Return new residual.

    input parameters:

.        ksp -    Krylov space object
.	 it  -    plane rotations are applied to the (it+1)th column of the 
                  modified hessenberg (i.e. HH(:,it))
.        hapend - PETSC_FALSE not happy breakdown ending.

    output parameters:
.        res - the new residual
	
 */
#undef __FUNCT__  
#define __FUNCT__ "FGMRESUpdateHessenberg"
static PetscErrorCode FGMRESUpdateHessenberg(KSP ksp,PetscInt it,PetscTruth hapend,PetscReal *res)
{
  PetscScalar   *hh,*cc,*ss,tt;
  PetscInt      j;
  KSP_FGMRES    *fgmres = (KSP_FGMRES *)(ksp->data);

  PetscFunctionBegin;
  hh  = HH(0,it);  /* pointer to beginning of column to update - so 
                      incrementing hh "steps down" the (it+1)th col of HH*/ 
  cc  = CC(0);     /* beginning of cosine rotations */ 
  ss  = SS(0);     /* beginning of sine rotations */

  /* Apply all the previously computed plane rotations to the new column
     of the Hessenberg matrix */
  /* Note: this uses the rotation [conj(c)  s ; -s   c], c= cos(theta), s= sin(theta),
     and some refs have [c   s ; -conj(s)  c] (don't be confused!) */

  for (j=1; j<=it; j++) {
    tt  = *hh;
#if defined(PETSC_USE_COMPLEX)
    *hh = PetscConj(*cc) * tt + *ss * *(hh+1);
#else
    *hh = *cc * tt + *ss * *(hh+1);
#endif
    hh++;
    *hh = *cc++ * *hh - (*ss++ * tt);
    /* hh, cc, and ss have all been incremented one by end of loop */
  }

  /*
    compute the new plane rotation, and apply it to:
     1) the right-hand-side of the Hessenberg system (RS)
        note: it affects RS(it) and RS(it+1)
     2) the new column of the Hessenberg matrix
        note: it affects HH(it,it) which is currently pointed to 
        by hh and HH(it+1, it) (*(hh+1))  
    thus obtaining the updated value of the residual...
  */

  /* compute new plane rotation */

  if (!hapend) {
#if defined(PETSC_USE_COMPLEX)
    tt        = PetscSqrtScalar(PetscConj(*hh) * *hh + PetscConj(*(hh+1)) * *(hh+1));
#else
    tt        = PetscSqrtScalar(*hh * *hh + *(hh+1) * *(hh+1));
#endif
    if (tt == 0.0) {
      ksp->reason = KSP_DIVERGED_NULL;
      PetscFunctionReturn(0);
    }

    *cc       = *hh / tt;   /* new cosine value */
    *ss       = *(hh+1) / tt;  /* new sine value */

    /* apply to 1) and 2) */
    *RS(it+1) = - (*ss * *RS(it));
#if defined(PETSC_USE_COMPLEX)
    *RS(it)   = PetscConj(*cc) * *RS(it);
    *hh       = PetscConj(*cc) * *hh + *ss * *(hh+1);
#else
    *RS(it)   = *cc * *RS(it);
    *hh       = *cc * *hh + *ss * *(hh+1);
#endif

    /* residual is the last element (it+1) of right-hand side! */
    *res      = PetscAbsScalar(*RS(it+1));

  } else { /* happy breakdown: HH(it+1, it) = 0, therfore we don't need to apply 
            another rotation matrix (so RH doesn't change).  The new residual is 
            always the new sine term times the residual from last time (RS(it)), 
            but now the new sine rotation would be zero...so the residual should
            be zero...so we will multiply "zero" by the last residual.  This might
            not be exactly what we want to do here -could just return "zero". */
 
    *res = 0.0;
  }
  PetscFunctionReturn(0);
}

/*

   FGMRESGetNewVectors - This routine allocates more work vectors, starting from 
                         VEC_VV(it), and more preconditioned work vectors, starting 
                         from PREVEC(i).

*/
#undef __FUNCT__  
#define __FUNCT__ "FGMRESGetNewVectors" 
static PetscErrorCode FGMRESGetNewVectors(KSP ksp,PetscInt it)
{
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)ksp->data;
  PetscInt       nwork = fgmres->nwork_alloc; /* number of work vector chunks allocated */
  PetscInt       nalloc;                      /* number to allocate */
  PetscErrorCode ierr;
  PetscInt       k;
 
  PetscFunctionBegin;
  nalloc = fgmres->delta_allocate; /* number of vectors to allocate 
                                      in a single chunk */

  /* Adjust the number to allocate to make sure that we don't exceed the
     number of available slots (fgmres->vecs_allocated)*/
  if (it + VEC_OFFSET + nalloc >= fgmres->vecs_allocated){
    nalloc = fgmres->vecs_allocated - it - VEC_OFFSET;
  }
  if (!nalloc) PetscFunctionReturn(0);

  fgmres->vv_allocated += nalloc; /* vv_allocated is the number of vectors allocated */

  /* work vectors */
  ierr = KSPGetVecs(ksp,nalloc,&fgmres->user_work[nwork],0,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(ksp,nalloc,fgmres->user_work[nwork]);CHKERRQ(ierr);
  for (k=0; k < nalloc; k++) {
    fgmres->vecs[it+VEC_OFFSET+k] = fgmres->user_work[nwork][k];
  }
  /* specify size of chunk allocated */
  fgmres->mwork_alloc[nwork] = nalloc;

  /* preconditioned vectors */
  ierr = KSPGetVecs(ksp,nalloc,&fgmres->prevecs_user_work[nwork],0,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(ksp,nalloc,fgmres->prevecs_user_work[nwork]);CHKERRQ(ierr);
  for (k=0; k < nalloc; k++) {
    fgmres->prevecs[it+VEC_OFFSET+k] = fgmres->prevecs_user_work[nwork][k];
  } 

  /* increment the number of work vector chunks */
  fgmres->nwork_alloc++;
  PetscFunctionReturn(0);
}

/* 

   KSPBuildSolution_FGMRES

     Input Parameter:
.     ksp - the Krylov space object
.     ptr-

   Output Parameter:
.     result - the solution

   Note: this calls BuildFgmresSoln - the same function that FGMREScycle
   calls directly.  

*/
#undef __FUNCT__  
#define __FUNCT__ "KSPBuildSolution_FGMRES"
PetscErrorCode KSPBuildSolution_FGMRES(KSP ksp,Vec ptr,Vec *result)
{
  KSP_FGMRES     *fgmres = (KSP_FGMRES *)ksp->data; 
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!ptr) {
    if (!fgmres->sol_temp) {
      ierr = VecDuplicate(ksp->vec_sol,&fgmres->sol_temp);CHKERRQ(ierr);
      ierr = PetscLogObjectParent(ksp,fgmres->sol_temp);CHKERRQ(ierr);
    }
    ptr = fgmres->sol_temp;
  }
  if (!fgmres->nrs) {
    /* allocate the work area */
    ierr = PetscMalloc(fgmres->max_k*sizeof(PetscScalar),&fgmres->nrs);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory(ksp,fgmres->max_k*sizeof(PetscScalar));CHKERRQ(ierr);
  }
 
  ierr = BuildFgmresSoln(fgmres->nrs,ksp->vec_sol,ptr,ksp,fgmres->it);CHKERRQ(ierr);
  if (result) *result = ptr; 
  
  PetscFunctionReturn(0);
}

extern PetscErrorCode KSPSetFromOptions_GMRES(KSP);

#undef __FUNCT__  
#define __FUNCT__ "KSPSetFromOptions_FGMRES"
PetscErrorCode KSPSetFromOptions_FGMRES(KSP ksp)
{
  PetscErrorCode ierr;
  PetscTruth     flg;

  PetscFunctionBegin;
  ierr = KSPSetFromOptions_GMRES(ksp);CHKERRQ(ierr);
  ierr = PetscOptionsHead("KSP flexible GMRES Options");CHKERRQ(ierr);
    ierr = PetscOptionsTruthGroupBegin("-ksp_fgmres_modifypcnochange","do not vary the preconditioner","KSPFGMRESSetModifyPC",&flg);CHKERRQ(ierr);
    if (flg) {ierr = KSPFGMRESSetModifyPC(ksp,KSPFGMRESModifyPCNoChange,0,0);CHKERRQ(ierr);} 
    ierr = PetscOptionsTruthGroupEnd("-ksp_fgmres_modifypcksp","vary the KSP based preconditioner","KSPFGMRESSetModifyPC",&flg);CHKERRQ(ierr);
    if (flg) {ierr = KSPFGMRESSetModifyPC(ksp,KSPFGMRESModifyPCKSP,0,0);CHKERRQ(ierr);} 
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN PetscErrorCode KSPComputeExtremeSingularValues_GMRES(KSP,PetscReal *,PetscReal *);
EXTERN PetscErrorCode KSPComputeEigenvalues_GMRES(KSP,PetscInt,PetscReal *,PetscReal *,PetscInt *);

typedef PetscErrorCode (*FCN1)(KSP,PetscInt,PetscInt,PetscReal,void*); /* force argument to next function to not be extern C*/
typedef PetscErrorCode (*FCN2)(void*);
EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "KSPFGMRESSetModifyPC_FGMRES" 
PetscErrorCode PETSCKSP_DLLEXPORT KSPFGMRESSetModifyPC_FGMRES(KSP ksp,FCN1 fcn,void *ctx,FCN2 d)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  ((KSP_FGMRES *)ksp->data)->modifypc      = fcn;
  ((KSP_FGMRES *)ksp->data)->modifydestroy = d;
  ((KSP_FGMRES *)ksp->data)->modifyctx     = ctx;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
EXTERN PetscErrorCode PETSCKSP_DLLEXPORT KSPGMRESSetPreAllocateVectors_GMRES(KSP);
EXTERN PetscErrorCode PETSCKSP_DLLEXPORT KSPGMRESSetRestart_GMRES(KSP,PetscInt);
EXTERN PetscErrorCode PETSCKSP_DLLEXPORT KSPGMRESSetOrthogonalization_GMRES(KSP,PetscErrorCode (*)(KSP,PetscInt));
EXTERN_C_END

EXTERN PetscErrorCode KSPDestroy_GMRES_Internal(KSP);

#undef __FUNCT__  
#define __FUNCT__ "KSPDestroy_FGMRES_Internal" 
PetscErrorCode KSPDestroy_FGMRES_Internal(KSP ksp)
{
  KSP_FGMRES     *gmres = (KSP_FGMRES*)ksp->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = KSPDestroy_GMRES_Internal(ksp);CHKERRQ(ierr);
  ierr = PetscFree (gmres->prevecs);CHKERRQ(ierr);
  ierr = PetscFree(gmres->prevecs_user_work);CHKERRQ(ierr);
  if (gmres->modifydestroy) {
    ierr = (*gmres->modifydestroy)(gmres->modifyctx);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "KSPGMRESSetRestart_FGMRES" 
PetscErrorCode PETSCKSP_DLLEXPORT KSPGMRESSetRestart_FGMRES(KSP ksp,PetscInt max_k)
{
  KSP_FGMRES     *gmres = (KSP_FGMRES *)ksp->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (max_k < 1) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"Restart must be positive");
  if (!ksp->setupcalled) {
    gmres->max_k = max_k;
  } else if (gmres->max_k != max_k) {
     gmres->max_k = max_k;
     ksp->setupcalled = 0;
     /* free the data structures, then create them again */
     ierr = KSPDestroy_FGMRES_Internal(ksp);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
EXTERN PetscErrorCode PETSCKSP_DLLEXPORT KSPGMRESSetCGSRefinementType_GMRES(KSP,KSPGMRESCGSRefinementType);
EXTERN_C_END

/*MC
     KSPFGMRES - Implements the Flexible Generalized Minimal Residual method.  
                developed by Saad with restart


   Options Database Keys:
+   -ksp_gmres_restart <restart> - the number of Krylov directions to orthogonalize against
.   -ksp_gmres_haptol <tol> - sets the tolerance for "happy ending" (exact convergence)
.   -ksp_gmres_preallocate - preallocate all the Krylov search directions initially (otherwise groups of 
                             vectors are allocated as needed)
.   -ksp_gmres_classicalgramschmidt - use classical (unmodified) Gram-Schmidt to orthogonalize against the Krylov space (fast) (the default)
.   -ksp_gmres_modifiedgramschmidt - use modified Gram-Schmidt in the orthogonalization (more stable, but slower)
.   -ksp_gmres_cgs_refinement_type <never,ifneeded,always> - determine if iterative refinement is used to increase the 
                                   stability of the classical Gram-Schmidt  orthogonalization.
.   -ksp_gmres_krylov_monitor - plot the Krylov space generated
.   -ksp_fgmres_modifypcnochange - do not change the preconditioner between iterations
-   -ksp_fgmres_modifypcksp - modify the preconditioner using KSPFGMRESModifyPCKSP()

   Level: beginner

    Notes: See KSPFGMRESSetModifyPC() for how to vary the preconditioner between iterations
           Only right preconditioning is supported.

    Developer Notes: This object is subclassed off of KSPGMRES

.seealso:  KSPCreate(), KSPSetType(), KSPType (for list of available types), KSP, KSPGMRES, KSPLGMRES,
           KSPGMRESSetRestart(), KSPGMRESSetHapTol(), KSPGMRESSetPreAllocateVectors(), KSPGMRESSetOrthogonalization()
           KSPGMRESClassicalGramSchmidtOrthogonalization(), KSPGMRESModifiedGramSchmidtOrthogonalization(),
           KSPGMRESCGSRefinementType, KSPGMRESSetCGSRefinementType(), KSPGMRESMonitorKrylov(), KSPFGMRESSetModifyPC(),
           KSPFGMRESModifyPCKSP()

M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "KSPCreate_FGMRES"
PetscErrorCode PETSCKSP_DLLEXPORT KSPCreate_FGMRES(KSP ksp)
{
  KSP_FGMRES     *fgmres;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNewLog(ksp,KSP_FGMRES,&fgmres);CHKERRQ(ierr);
  ksp->data                              = (void*)fgmres;
  ksp->ops->buildsolution                = KSPBuildSolution_FGMRES;
  ksp->ops->setup                        = KSPSetUp_FGMRES;
  ksp->ops->solve                        = KSPSolve_FGMRES;
  ksp->ops->destroy                      = KSPDestroy_FGMRES;
  ksp->ops->view                         = KSPView_GMRES;
  ksp->ops->setfromoptions               = KSPSetFromOptions_FGMRES;
  ksp->ops->computeextremesingularvalues = KSPComputeExtremeSingularValues_GMRES;
  ksp->ops->computeeigenvalues           = KSPComputeEigenvalues_GMRES;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPGMRESSetPreAllocateVectors_C",
                                    "KSPGMRESSetPreAllocateVectors_GMRES",
                                     KSPGMRESSetPreAllocateVectors_GMRES);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPGMRESSetOrthogonalization_C",
                                    "KSPGMRESSetOrthogonalization_GMRES",
                                     KSPGMRESSetOrthogonalization_GMRES);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPGMRESSetRestart_C",
                                    "KSPGMRESSetRestart_FGMRES",
                                     KSPGMRESSetRestart_FGMRES);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPFGMRESSetModifyPC_C",
                                    "KSPFGMRESSetModifyPC_FGMRES",
                                     KSPFGMRESSetModifyPC_FGMRES);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)ksp,"KSPGMRESSetCGSRefinementType_C",
                                    "KSPGMRESSetCGSRefinementType_GMRES",
                                     KSPGMRESSetCGSRefinementType_GMRES);CHKERRQ(ierr);


  fgmres->haptol              = 1.0e-30;
  fgmres->q_preallocate       = 0;
  fgmres->delta_allocate      = FGMRES_DELTA_DIRECTIONS;
  fgmres->orthog              = KSPGMRESClassicalGramSchmidtOrthogonalization;
  fgmres->nrs                 = 0;
  fgmres->sol_temp            = 0;
  fgmres->max_k               = FGMRES_DEFAULT_MAXK;
  fgmres->Rsvd                = 0;
  fgmres->orthogwork          = 0;
  fgmres->modifypc            = KSPFGMRESModifyPCNoChange;
  fgmres->modifyctx           = PETSC_NULL;
  fgmres->modifydestroy       = PETSC_NULL;
  fgmres->cgstype             = KSP_GMRES_CGS_REFINE_NEVER;
  /*
        This is not great since it changes this without explicit request from the user
     but there is no left preconditioning in the FGMRES
  */
  if (ksp->pc_side != PC_RIGHT) {
     ierr = PetscInfo(ksp,"WARNING! Setting PC_SIDE for FGMRES to right!\n");CHKERRQ(ierr);
  }
  ksp->pc_side  = PC_RIGHT;
  ksp->normtype = KSP_NORM_UNPRECONDITIONED;
  PetscFunctionReturn(0);
}
EXTERN_C_END

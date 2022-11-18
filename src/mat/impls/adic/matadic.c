#define PETSCMAT_DLL

/*
    ADIC matrix-free matrix implementation
*/

#include "private/matimpl.h"
#include "petscda.h"          /*I   "petscda.h"    I*/
#include "petscsnes.h"        /*I   "petscsnes.h"  I*/
EXTERN_C_BEGIN
#include "adic/ad_utils.h"
EXTERN_C_END

typedef struct {
  DA         da;
  Vec        localu;         /* point at which Jacobian is evaluated */
  void       *ctx;
  SNES       snes;
  Vec        diagonal;       /* current matrix diagonal */
  PetscTruth diagonalvalid;  /* indicates if diagonal matches current base vector */
} Mat_DAAD;

#undef __FUNCT__  
#define __FUNCT__ "MatAssemblyEnd_DAAD"
PetscErrorCode MatAssemblyEnd_DAAD(Mat A,MatAssemblyType atype)
{
  Mat_DAAD *a = (Mat_DAAD*)A->data;
  PetscErrorCode ierr;
  Vec      u;

  PetscFunctionBegin;
  a->diagonalvalid = PETSC_FALSE;
  if (a->snes) {
    ierr = SNESGetSolution(a->snes,&u);CHKERRQ(ierr);
    ierr = DAGlobalToLocalBegin(a->da,u,INSERT_VALUES,a->localu);CHKERRQ(ierr);
    ierr = DAGlobalToLocalEnd(a->da,u,INSERT_VALUES,a->localu);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatMult_DAAD"
PetscErrorCode MatMult_DAAD(Mat A,Vec xx,Vec yy)
{
  Mat_DAAD *a = (Mat_DAAD*)A->data;
  Vec      localxx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DAGetLocalVector(a->da,&localxx);CHKERRQ(ierr);
  ierr = DAGlobalToLocalBegin(a->da,xx,INSERT_VALUES,localxx);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(a->da,xx,INSERT_VALUES,localxx);CHKERRQ(ierr);
  ierr = DAMultiplyByJacobian1WithAD(a->da,a->localu,localxx,yy,a->ctx);CHKERRQ(ierr);
  ierr = DARestoreLocalVector(a->da,&localxx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include "../src/dm/da/daimpl.h"

#undef __FUNCT__  
#define __FUNCT__ "MatGetDiagonal_DAAD"
PetscErrorCode MatGetDiagonal_DAAD(Mat A,Vec dd)
{
  Mat_DAAD      *a = (Mat_DAAD*)A->data;
  PetscErrorCode ierr;
  int j,nI,gI,gtdof;
  PetscScalar   *avu,*ad_vustart,ad_f[2],*d;
  DALocalInfo   info;
  MatStencil    stencil;
  void*         *ad_vu;

  PetscFunctionBegin;

  /* get space for derivative object.  */
  ierr = DAGetAdicMFArray(a->da,PETSC_TRUE,(void **)&ad_vu,(void**)&ad_vustart,&gtdof);CHKERRQ(ierr);

  /* copy input vector into derivative object */
  ierr = VecGetArray(a->localu,&avu);CHKERRQ(ierr);
  for (j=0; j<gtdof; j++) {
    ad_vustart[2*j]   = avu[j];
    ad_vustart[2*j+1] = 0.0;
  }
  ierr = VecRestoreArray(a->localu,&avu);CHKERRQ(ierr);

  PetscADResetIndep();
  ierr = PetscADIncrementTotalGradSize(1);CHKERRQ(ierr);
  PetscADSetIndepDone();

  ierr = VecGetArray(dd,&d);CHKERRQ(ierr);

  ierr = DAGetLocalInfo(a->da,&info);CHKERRQ(ierr);
  nI = 0;
  for (stencil.k = info.zs; stencil.k<info.zs+info.zm; stencil.k++) {
    for (stencil.j = info.ys; stencil.j<info.ys+info.ym; stencil.j++) {
      for (stencil.i = info.xs; stencil.i<info.xs+info.xm; stencil.i++) {
	for (stencil.c = 0; stencil.c<info.dof; stencil.c++) {
	  gI   = stencil.c + (stencil.i - info.gxs)*info.dof + (stencil.j - info.gys)*info.dof*info.gxm + (stencil.k - info.gzs)*info.dof*info.gxm*info.gym;
          ad_vustart[1+2*gI] = 1.0;
	  ierr = (*a->da->adicmf_lfi)(&info,&stencil,ad_vu,ad_f,a->ctx);CHKERRQ(ierr);
	  d[nI] = ad_f[1];
          ad_vustart[1+2*gI] = 0.0;
	  nI++;
	}
      }
    }
  }

  ierr = VecRestoreArray(dd,&d);CHKERRQ(ierr);
  ierr = DARestoreAdicMFArray(a->da,PETSC_TRUE,(void **)&ad_vu,(void**)&ad_vustart,&gtdof);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "MatSOR_DAAD"
PetscErrorCode MatSOR_DAAD(Mat A,Vec bb,PetscReal omega,MatSORType flag,PetscReal fshift,int its,int lits,Vec xx)
{
  Mat_DAAD      *a = (Mat_DAAD*)A->data;
  PetscErrorCode ierr;
  int j,gtdof,nI,gI;
  PetscScalar   *avu,*av,*ad_vustart,ad_f[2],*d,*b;
  Vec           localxx,dd;
  DALocalInfo   info;
  MatStencil    stencil;
  void*         *ad_vu;

  PetscFunctionBegin;
  if (omega != 1.0) SETERRQ(PETSC_ERR_ARG_WRONG,"Currently only support omega of 1.0");
  if (fshift)       SETERRQ(PETSC_ERR_ARG_WRONG,"Currently do not support fshift");
  if (its <= 0 || lits <= 0) SETERRQ2(PETSC_ERR_ARG_WRONG,"Relaxation requires global its %D and local its %D both positive",its,lits);

  if (!a->diagonal) {
    ierr = DACreateGlobalVector(a->da,&a->diagonal);CHKERRQ(ierr);
  }
  if (!a->diagonalvalid) {
    ierr             = MatGetDiagonal(A,a->diagonal);CHKERRQ(ierr);
    a->diagonalvalid = PETSC_TRUE;
  }
  dd   = a->diagonal;


  ierr = DAGetLocalVector(a->da,&localxx);CHKERRQ(ierr);
  if (flag & SOR_ZERO_INITIAL_GUESS) {
    ierr = VecSet(localxx,0.0);CHKERRQ(ierr);
  } else {
    ierr = DAGlobalToLocalBegin(a->da,xx,INSERT_VALUES,localxx);CHKERRQ(ierr);
    ierr = DAGlobalToLocalEnd(a->da,xx,INSERT_VALUES,localxx);CHKERRQ(ierr);
  }

  /* get space for derivative object.  */
  ierr = DAGetAdicMFArray(a->da,PETSC_TRUE,(void **)&ad_vu,(void**)&ad_vustart,&gtdof);CHKERRQ(ierr);

  /* copy input vector into derivative object */
  ierr = VecGetArray(a->localu,&avu);CHKERRQ(ierr);
  ierr = VecGetArray(localxx,&av);CHKERRQ(ierr);
  for (j=0; j<gtdof; j++) {
    ad_vustart[2*j]   = avu[j];
    ad_vustart[2*j+1] = av[j];
  }
  ierr = VecRestoreArray(a->localu,&avu);CHKERRQ(ierr);
  ierr = VecRestoreArray(localxx,&av);CHKERRQ(ierr);

  PetscADResetIndep();
  ierr = PetscADIncrementTotalGradSize(1);CHKERRQ(ierr);
  PetscADSetIndepDone();

  ierr = VecGetArray(dd,&d);CHKERRQ(ierr);
  ierr = VecGetArray(bb,&b);CHKERRQ(ierr);

  ierr = DAGetLocalInfo(a->da,&info);CHKERRQ(ierr);
  while (its--) {
    if (flag & SOR_FORWARD_SWEEP || flag & SOR_LOCAL_FORWARD_SWEEP){
      nI = 0;
      for (stencil.k = info.zs; stencil.k<info.zs+info.zm; stencil.k++) {
        for (stencil.j = info.ys; stencil.j<info.ys+info.ym; stencil.j++) {
          for (stencil.i = info.xs; stencil.i<info.xs+info.xm; stencil.i++) {
            for (stencil.c = 0; stencil.c<info.dof; stencil.c++) {
              ierr = (*a->da->adicmf_lfi)(&info,&stencil,ad_vu,ad_f,a->ctx);CHKERRQ(ierr);
              gI   = stencil.c + (stencil.i - info.gxs)*info.dof + (stencil.j - info.gys)*info.dof*info.gxm + (stencil.k - info.gzs)*info.dof*info.gxm*info.gym;
              ad_vustart[1+2*gI] += (b[nI] - ad_f[1])/d[nI];
              nI++;
            }
          }
        }
      }
    }
    if (flag & SOR_BACKWARD_SWEEP || flag & SOR_LOCAL_BACKWARD_SWEEP){
      nI = info.dof*info.xm*info.ym*info.zm - 1;
      for (stencil.k = info.zs+info.zm-1; stencil.k>=info.zs; stencil.k--) {
        for (stencil.j = info.ys+info.ym-1; stencil.j>=info.ys; stencil.j--) {
          for (stencil.i = info.xs+info.xm-1; stencil.i>=info.xs; stencil.i--) {
            for (stencil.c = info.dof-1; stencil.c>=0; stencil.c--) {
              ierr = (*a->da->adicmf_lfi)(&info,&stencil,ad_vu,ad_f,a->ctx);CHKERRQ(ierr);
              gI   = stencil.c + (stencil.i - info.gxs)*info.dof + (stencil.j - info.gys)*info.dof*info.gxm + (stencil.k - info.gzs)*info.dof*info.gxm*info.gym;
              ad_vustart[1+2*gI] += (b[nI] - ad_f[1])/d[nI];
              nI--;
            }
          }
        }
      }
    }
  }

  ierr = VecRestoreArray(dd,&d);CHKERRQ(ierr);
  ierr = VecRestoreArray(bb,&b);CHKERRQ(ierr);

  ierr = VecGetArray(localxx,&av);CHKERRQ(ierr);
  for (j=0; j<gtdof; j++) {
    av[j] = ad_vustart[2*j+1];
  }
  ierr = VecRestoreArray(localxx,&av);CHKERRQ(ierr);
  ierr = DALocalToGlobal(a->da,localxx,INSERT_VALUES,xx);CHKERRQ(ierr);

  ierr = DARestoreLocalVector(a->da,&localxx);CHKERRQ(ierr);
  ierr = DARestoreAdicMFArray(a->da,PETSC_TRUE,(void **)&ad_vu,(void**)&ad_vustart,&gtdof);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_DAAD"
PetscErrorCode MatDestroy_DAAD(Mat A)
{
  Mat_DAAD *a = (Mat_DAAD*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DADestroy(a->da);CHKERRQ(ierr);
  ierr = VecDestroy(a->localu);CHKERRQ(ierr);
  if (a->diagonal) {ierr = VecDestroy(a->diagonal);CHKERRQ(ierr);}
  ierr = PetscFree(a);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)A,0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMFFDSetBase_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatDAADSetDA_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatDAADSetSNES_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatDAADSetCtx_C","",PETSC_NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = {0,
       0,
       0,
       MatMult_DAAD,
/* 4*/ 0,
       0,
       0,
       0,
       0,
       0,
/*10*/ 0,
       0,
       0,
       MatSOR_DAAD,
       0,
/*15*/ 0,
       0,
       MatGetDiagonal_DAAD,
       0,
       0,
/*20*/ 0,
       MatAssemblyEnd_DAAD,
       0,
       0,
/*24*/ 0,
       0,
       0,
       0,
       0,
/*29*/ 0,
       0,
       0,
       0,
       0,
/*34*/ 0,
       0,
       0,
       0,
       0,
/*39*/ 0,
       0,
       0,
       0,
       0,
/*44*/ 0,
       0,
       0,
       0,
       0,
/*49*/ 0,
       0,
       0,
       0,
       0,
/*54*/ 0,
       0,
       0,
       0,
       0,
/*59*/ 0,
       MatDestroy_DAAD,
       0,
       0,
       0,
/*64*/ 0,
       0,
       0,
       0,
       0,
/*69*/ 0,
       0,
       0,
       0,
       0,
/*74*/ 0,
       0,
       0,
       0,
       0,
/*79*/ 0,
       0,
       0,
       0,
       0,
/*84*/ 0,
       0,
       0,
       0,
       0,
/*89*/ 0,
       0,
       0,
       0,
       0,
/*94*/ 0,
       0,
       0,
       0};

/* --------------------------------------------------------------------------------*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "MatMFFDSetBase_AD"
PetscErrorCode PETSCMAT_DLLEXPORT MatMFFDSetBase_AD(Mat J,Vec U,Vec F)
{
  PetscErrorCode ierr;
  Mat_DAAD       *a = (Mat_DAAD*)J->data;

  PetscFunctionBegin;
  a->diagonalvalid = PETSC_FALSE;
  ierr = DAGlobalToLocalBegin(a->da,U,INSERT_VALUES,a->localu);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(a->da,U,INSERT_VALUES,a->localu);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetDA_AD"
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetDA_AD(Mat A,DA da)
{
  Mat_DAAD       *a = (Mat_DAAD*)A->data;
  PetscErrorCode ierr;
  PetscInt       nc,nx,ny,nz,Nx,Ny,Nz;

  PetscFunctionBegin;
  ierr  = PetscObjectReference((PetscObject)da);CHKERRQ(ierr);
  if (a->da) { ierr = DADestroy(a->da);CHKERRQ(ierr); }
  a->da = da;
  ierr  = DAGetInfo(da,0,&Nx,&Ny,&Nz,0,0,0,&nc,0,0,0);CHKERRQ(ierr);
  ierr  = DAGetCorners(da,0,0,0,&nx,&ny,&nz);CHKERRQ(ierr);
  A->rmap->n  = A->cmap->n = nc*nx*ny*nz;
  A->rmap->N  = A->cmap->N = nc*Nx*Ny*Nz;
  ierr  = DACreateLocalVector(da,&a->localu);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetSNES_AD"
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetSNES_AD(Mat A,SNES snes)
{
  Mat_DAAD *a = (Mat_DAAD*)A->data;

  PetscFunctionBegin;
  a->snes = snes;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetCtx_AD"
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetCtx_AD(Mat A,void *ctx)
{
  Mat_DAAD *a = (Mat_DAAD*)A->data;

  PetscFunctionBegin;
  a->ctx = ctx;
  PetscFunctionReturn(0);
}
EXTERN_C_END

/*MC
  MATDAAD - MATDAAD = "daad" - A matrix type that can do matrix-vector products using a local function that
  is differentiated with ADIFOR or ADIC. 

  Level: intermediate

.seealso: MatCreateDAAD
M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "MatCreate_DAAD"
PetscErrorCode PETSCMAT_DLLEXPORT MatCreate_DAAD(Mat B)
{
  Mat_DAAD       *b;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr    = PetscNewLog(B,Mat_DAAD,&b);CHKERRQ(ierr);
  B->data = (void*)b;
  ierr = PetscMemcpy(B->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);
  
  ierr = PetscLayoutSetBlockSize(B->rmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(B->cmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)B,MATDAAD);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatMFFDSetBase_C","MatMFFDSetBase_AD",MatMFFDSetBase_AD);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatDAADSetDA_C","MatDAADSetDA_AD",MatDAADSetDA_AD);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatDAADSetSNES_C","MatDAADSetSNES_AD",MatDAADSetSNES_AD);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatDAADSetCtx_C","MatDAADSetCtx_AD",MatDAADSetCtx_AD);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATDAAD);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END


#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetDA"
/*@C
   MatDAADSetDA - Tells the matrix what DA it is using for layout and Jacobian.

   Collective on Mat and DA

   Input Parameters:
+  mat - the matrix
-  da - the DA

   Level: intermediate

.seealso: MatCreate(), DASetLocalAdicMFFunction(), MatCreateDAAD()

@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetDA(Mat A,DA da)
{
  PetscErrorCode ierr,(*f)(Mat,void*);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  PetscValidHeaderSpecific(da,DM_COOKIE,2);
  ierr = PetscObjectQueryFunction((PetscObject)A,"MatDAADSetDA_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(A,da);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetSNES"
/*@C
   MatDAADSetSNES - Tells the matrix what SNES it is using for the base U.

   Collective on Mat and SNES

   Input Parameters:
+  mat - the matrix
-  snes - the SNES

   Level: intermediate

   Notes: this is currently turned off for Fortran usage

.seealso: MatCreate(), DASetLocalAdicMFFunction(), MatCreateDAAD(), MatDAADSetDA()

@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetSNES(Mat A,SNES snes)
{
  PetscErrorCode ierr,(*f)(Mat,void*);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  PetscValidHeaderSpecific(snes,SNES_COOKIE,2);
  ierr = PetscObjectQueryFunction((PetscObject)A,"MatDAADSetSNES_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(A,snes);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatDAADSetCtx"
/*@C
   MatDAADSetCtx - Sets the user context for a DAAD (ADIC matrix-free) matrix.

   Collective on Mat

   Input Parameters:
+  mat - the matrix
-  ctx - the context

   Level: intermediate

.seealso: MatCreate(), DASetLocalAdicMFFunction(), MatCreateDAAD(), MatDAADSetDA()

@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatDAADSetCtx(Mat A,void *ctx)
{
  PetscErrorCode ierr,(*f)(Mat,void*);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)A,"MatDAADSetCtx_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(A,ctx);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatCreateDAAD"
/*@C
   MatCreateDAAD - Creates a matrix that can do matrix-vector products using a local 
   function that is differentiated with ADIFOR or ADIC.

   Collective on DA

   Input Parameters:
.  da - the DA that defines the distribution of the vectors

   Output Parameter:
.  A - the matrix 

   Level: intermediate

   Notes: this is currently turned off for Fortran

.seealso: MatCreate(), DASetLocalAdicMFFunction()

@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatCreateDAAD(DA da,Mat *A)
{
  PetscErrorCode ierr;
  MPI_Comm comm;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATDAAD);CHKERRQ(ierr);
  ierr = MatDAADSetDA(*A,da);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "MatRegisterDAAD"
/*@
   MatRegisterDAAD - Registers DAAD matrix type

   Level: advanced

.seealso: MatCreateDAAD(), DASetLocalAdicMFFunction()

@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatRegisterDAAD(void)
{ 
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = MatRegisterDynamic(MATDAAD,PETSC_NULL,"MatCreate_DAAD",MatCreate_DAAD);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

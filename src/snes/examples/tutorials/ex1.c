
static char help[] = "Newton's method for a two-variable system, sequential.\n\n";

/*T
   Concepts: SNES^basic example
T*/

/* 
   Include "petscsnes.h" so that we can use SNES solvers.  Note that this
   file automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers
*/
#include "petscsnes.h"

typedef struct {
  Vec         xloc,rloc;    /* local solution, residual vectors */
  VecScatter  scatter;
} AppCtx;

/* 
   User-defined routines
*/
extern PetscErrorCode FormJacobian1(SNES,Vec,Mat*,Mat*,MatStructure*,void*);
extern PetscErrorCode FormFunction1(SNES,Vec,Vec,void*);
extern PetscErrorCode FormJacobian2(SNES,Vec,Mat*,Mat*,MatStructure*,void*);
extern PetscErrorCode FormFunction2(SNES,Vec,Vec,void*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  SNES           snes;         /* nonlinear solver context */
  KSP            ksp;          /* linear solver context */
  PC             pc;           /* preconditioner context */
  Vec            x,r;          /* solution, residual vectors */
  Mat            J;            /* Jacobian matrix */
  PetscErrorCode ierr;
  PetscInt       its;
  PetscMPIInt    size,rank;
  PetscScalar    pfive = .5,*xx;
  PetscTruth     flg;
  AppCtx         user;         /* user-defined work context */
  IS             isglobal,islocal;

  PetscInitialize(&argc,&argv,(char *)0,help);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create nonlinear solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESCreate(PETSC_COMM_WORLD,&snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create matrix and vector data structures; set corresponding routines
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Create vectors for solution and nonlinear function
  */
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,2);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&r);CHKERRQ(ierr);

  if (size > 1){
    ierr = VecCreateSeq(PETSC_COMM_SELF,2,&user.xloc);CHKERRQ(ierr);
    ierr = VecDuplicate(user.xloc,&user.rloc);CHKERRQ(ierr);

    /* Create the scatter between the global x and local xloc */
    ierr = ISCreateStride(MPI_COMM_SELF,2,0,1,&islocal);CHKERRQ(ierr);
    ierr = ISCreateStride(MPI_COMM_SELF,2,0,1,&isglobal);CHKERRQ(ierr);
    ierr = VecScatterCreate(x,isglobal,user.xloc,islocal,&user.scatter);CHKERRQ(ierr);
    ierr = ISDestroy(isglobal);CHKERRQ(ierr);
    ierr = ISDestroy(islocal);CHKERRQ(ierr);
  }

  /*
     Create Jacobian matrix data structure
  */
  ierr = MatCreate(PETSC_COMM_WORLD,&J);CHKERRQ(ierr);
  ierr = MatSetSizes(J,PETSC_DECIDE,PETSC_DECIDE,2,2);CHKERRQ(ierr);
  ierr = MatSetFromOptions(J);CHKERRQ(ierr);

  ierr = PetscOptionsHasName(PETSC_NULL,"-hard",&flg);CHKERRQ(ierr);
  if (!flg) {
    /* 
     Set function evaluation routine and vector.
    */
    ierr = SNESSetFunction(snes,r,FormFunction1,&user);CHKERRQ(ierr);

    /* 
     Set Jacobian matrix data structure and Jacobian evaluation routine
    */
    ierr = SNESSetJacobian(snes,J,J,FormJacobian1,PETSC_NULL);CHKERRQ(ierr);
  } else {
    if (size != 1) SETERRQ(1,"This case is a uniprocessor example only!");
    ierr = SNESSetFunction(snes,r,FormFunction2,PETSC_NULL);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,J,J,FormJacobian2,PETSC_NULL);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Customize nonlinear solver; set runtime options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /* 
     Set linear solver defaults for this problem. By extracting the
     KSP, KSP, and PC contexts from the SNES context, we can then
     directly call any KSP, KSP, and PC routines to set various options.
  */
  ierr = SNESGetKSP(snes,&ksp);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCNONE);CHKERRQ(ierr);
  ierr = KSPSetTolerances(ksp,1.e-4,PETSC_DEFAULT,PETSC_DEFAULT,20);CHKERRQ(ierr);

  /* 
     Set SNES/KSP/KSP/PC runtime options, e.g.,
         -snes_view -snes_monitor -ksp_type <ksp> -pc_type <pc>
     These options will override those specified above as long as
     SNESSetFromOptions() is called _after_ any other customization
     routines.
  */
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Evaluate initial guess; then solve nonlinear system
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  if (!flg) {
    ierr = VecSet(x,pfive);CHKERRQ(ierr);
  } else {
    ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
    xx[0] = 2.0; xx[1] = 3.0;
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
  }
  /*
     Note: The user should initialize the vector, x, with the initial guess
     for the nonlinear solver prior to calling SNESSolve().  In particular,
     to employ an initial guess of zero, the user should explicitly set
     this vector to zero by calling VecSet().
  */

  ierr = SNESSolve(snes,PETSC_NULL,x);CHKERRQ(ierr);
  ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
  if (flg) {
    Vec f;
    ierr = VecView(x,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = SNESGetFunction(snes,&f,0,0);CHKERRQ(ierr);
    ierr = VecView(r,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  ierr = PetscPrintf(PETSC_COMM_WORLD,"number of Newton iterations = %D\n\n",its);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = VecDestroy(x);CHKERRQ(ierr); ierr = VecDestroy(r);CHKERRQ(ierr);
  ierr = MatDestroy(J);CHKERRQ(ierr); ierr = SNESDestroy(snes);CHKERRQ(ierr);
  if (size > 1){
    ierr = VecDestroy(user.xloc);CHKERRQ(ierr); 
    ierr = VecDestroy(user.rloc);CHKERRQ(ierr);
    ierr = VecScatterDestroy(user.scatter);CHKERRQ(ierr);
  }
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormFunction1"
/* 
   FormFunction1 - Evaluates nonlinear function, F(x).

   Input Parameters:
.  snes - the SNES context
.  x    - input vector
.  ctx  - optional user-defined context

   Output Parameter:
.  f - function vector
 */
PetscErrorCode FormFunction1(SNES snes,Vec x,Vec f,void *ctx)
{
  PetscErrorCode ierr;
  PetscScalar    *xx,*ff;
  AppCtx         *user = (AppCtx*)ctx;
  Vec            xloc=user->xloc,floc=user->rloc;
  VecScatter     scatter=user->scatter;
  MPI_Comm       comm;
  PetscMPIInt    size,rank;
  PetscInt       rstart,rend;

  ierr = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  if (size > 1){
    /* 
       This is a ridiculous case for testing intermidiate steps from sequential
           code development to parallel implementation.
       (1) scatter x into a sequetial vector;
       (2) each process evaluates all values of floc; 
       (3) scatter floc back to the parallel f.
     */
    ierr = VecScatterBegin(scatter,x,xloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter,x,xloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

    ierr = VecGetOwnershipRange(f,&rstart,&rend);CHKERRQ(ierr);
    ierr = VecGetArray(xloc,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(floc,&ff);CHKERRQ(ierr);
    ff[0] = xx[0]*xx[0] + xx[0]*xx[1] - 3.0;
    ff[1] = xx[0]*xx[1] + xx[1]*xx[1] - 6.0;
    ierr = VecRestoreArray(floc,&ff);CHKERRQ(ierr);
    ierr = VecRestoreArray(xloc,&xx);CHKERRQ(ierr);

    ierr = VecScatterBegin(scatter,floc,f,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter,floc,f,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  } else {
    /*
     Get pointers to vector data.
       - For default PETSc vectors, VecGetArray() returns a pointer to
         the data array.  Otherwise, the routine is implementation dependent.
       - You MUST call VecRestoreArray() when you no longer need access to
         the array.
    */
    ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(f,&ff);CHKERRQ(ierr);

    /* Compute function */
    ff[0] = xx[0]*xx[0] + xx[0]*xx[1] - 3.0;
    ff[1] = xx[0]*xx[1] + xx[1]*xx[1] - 6.0;

    /* Restore vectors */
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(f,&ff);CHKERRQ(ierr); 
  }
  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormJacobian1"
/*
   FormJacobian1 - Evaluates Jacobian matrix.

   Input Parameters:
.  snes - the SNES context
.  x - input vector
.  dummy - optional user-defined context (not used here)

   Output Parameters:
.  jac - Jacobian matrix
.  B - optionally different preconditioning matrix
.  flag - flag indicating matrix structure
*/
PetscErrorCode FormJacobian1(SNES snes,Vec x,Mat *jac,Mat *B,MatStructure *flag,void *dummy)
{
  PetscScalar    *xx,A[4];
  PetscErrorCode ierr;
  PetscInt       idx[2] = {0,1};

  /*
     Get pointer to vector data
  */
  ierr = VecGetArray(x,&xx);CHKERRQ(ierr);

  /*
     Compute Jacobian entries and insert into matrix.
      - Since this is such a small problem, we set all entries for
        the matrix at once.
  */
  A[0] = 2.0*xx[0] + xx[1]; A[1] = xx[0];
  A[2] = xx[1]; A[3] = xx[0] + 2.0*xx[1];
  ierr = MatSetValues(*B,2,idx,2,idx,A,INSERT_VALUES);CHKERRQ(ierr);
  *flag = SAME_NONZERO_PATTERN;

  /*
     Restore vector
  */
  ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);

  /* 
     Assemble matrix
  */
  ierr = MatAssemblyBegin(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (*jac != *B){
    ierr = MatAssemblyBegin(*jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(*jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  return 0;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormFunction2"
PetscErrorCode FormFunction2(SNES snes,Vec x,Vec f,void *dummy)
{
  PetscErrorCode ierr;
  PetscScalar    *xx,*ff;

  /*
     Get pointers to vector data.
       - For default PETSc vectors, VecGetArray() returns a pointer to
         the data array.  Otherwise, the routine is implementation dependent.
       - You MUST call VecRestoreArray() when you no longer need access to
         the array.
  */
  ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
  ierr = VecGetArray(f,&ff);CHKERRQ(ierr);

  /*
     Compute function
  */
  ff[0] = PetscSinScalar(3.0*xx[0]) + xx[0];
  ff[1] = xx[1];

  /*
     Restore vectors
  */
  ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
  ierr = VecRestoreArray(f,&ff);CHKERRQ(ierr); 
  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormJacobian2"
PetscErrorCode FormJacobian2(SNES snes,Vec x,Mat *jac,Mat *B,MatStructure *flag,void *dummy)
{
  PetscScalar    *xx,A[4];
  PetscErrorCode ierr;
  PetscInt       idx[2] = {0,1};

  /*
     Get pointer to vector data
  */
  ierr = VecGetArray(x,&xx);CHKERRQ(ierr);

  /*
     Compute Jacobian entries and insert into matrix.
      - Since this is such a small problem, we set all entries for
        the matrix at once.
  */
  A[0] = 3.0*PetscCosScalar(3.0*xx[0]) + 1.0; A[1] = 0.0;
  A[2] = 0.0;                     A[3] = 1.0;
  ierr = MatSetValues(*B,2,idx,2,idx,A,INSERT_VALUES);CHKERRQ(ierr);
  *flag = SAME_NONZERO_PATTERN;

  /*
     Restore vector
  */
  ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);

  /*
     Assemble matrix
  */
  ierr = MatAssemblyBegin(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (*jac != *B){
    ierr = MatAssemblyBegin(*jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(*jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  return 0;
}



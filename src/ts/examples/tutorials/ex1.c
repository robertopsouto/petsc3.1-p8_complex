
static char help[] ="Solves the time dependent Bratu problem using pseudo-timestepping.";

/*
   Concepts: TS^pseudo-timestepping
   Concepts: pseudo-timestepping
   Concepts: nonlinear problems
   Processors: 1

*/

/* ------------------------------------------------------------------------

    This code demonstrates how one may solve a nonlinear problem 
    with pseudo-timestepping. In this simple example, the pseudo-timestep
    is the same for all grid points, i.e., this is equivalent to using
    the backward Euler method with a variable timestep.

    Note: This example does not require pseudo-timestepping since it
    is an easy nonlinear problem, but it is included to demonstrate how
    the pseudo-timestepping may be done.

    See snes/examples/tutorials/ex4.c[ex4f.F] and 
    snes/examples/tutorials/ex5.c[ex5f.F] where the problem is described
    and solved using Newton's method alone.

  ----------------------------------------------------------------------------- */
/*
    Include "petscts.h" to use the PETSc timestepping routines. Note that
    this file automatically includes "petscsys.h" and other lower-level
    PETSc include files.
*/
#include "petscts.h"

/*
  Create an application context to contain data needed by the 
  application-provided call-back routines, FormJacobian() and
  FormFunction().
*/
typedef struct {
  PetscReal   param;        /* test problem parameter */
  PetscInt    mx;           /* Discretization in x-direction */
  PetscInt    my;           /* Discretization in y-direction */
} AppCtx;

/* 
   User-defined routines
*/
extern PetscErrorCode  FormJacobian(TS,PetscReal,Vec,Mat*,Mat*,MatStructure*,void*),
     FormFunction(TS,PetscReal,Vec,Vec,void*),
     FormInitialGuess(Vec,AppCtx*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  TS             ts;                 /* timestepping context */
  Vec            x,r;               /* solution, residual vectors */
  Mat            J;                  /* Jacobian matrix */
  AppCtx         user;               /* user-defined work context */
  PetscInt       its,N;                /* iterations for convergence */
  PetscErrorCode ierr; 
  PetscReal      param_max = 6.81,param_min = 0.,dt;
  PetscReal      ftime;

  PetscInitialize(&argc,&argv,PETSC_NULL,help);
  user.mx        = 4;
  user.my        = 4;
  user.param     = 6.0;
  
  /*
     Allow user to set the grid dimensions and nonlinearity parameter at run-time
  */
  PetscOptionsGetInt(PETSC_NULL,"-mx",&user.mx,PETSC_NULL);
  PetscOptionsGetInt(PETSC_NULL,"-my",&user.my,PETSC_NULL);
  PetscOptionsGetReal(PETSC_NULL,"-param",&user.param,PETSC_NULL);
  if (user.param >= param_max || user.param <= param_min) {
    SETERRQ(1,"Parameter is out of range");
  }
  dt = .5/PetscMax(user.mx,user.my);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-dt",&dt,PETSC_NULL);CHKERRQ(ierr);
  N          = user.mx*user.my;
  
  /* 
      Create vectors to hold the solution and function value
  */
  ierr = VecCreateSeq(PETSC_COMM_SELF,N,&x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&r);CHKERRQ(ierr);

  /*
    Create matrix to hold Jacobian. Preallocate 5 nonzeros per row
    in the sparse matrix. Note that this is not the optimal strategy; see
    the Performance chapter of the users manual for information on 
    preallocating memory in sparse matrices.
  */
  ierr = MatCreateSeqAIJ(PETSC_COMM_SELF,N,N,5,0,&J);CHKERRQ(ierr);

  /* 
     Create timestepper context 
  */
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR);CHKERRQ(ierr);

  /*
     Tell the timestepper context where to compute solutions
  */
  ierr = TSSetSolution(ts,x);CHKERRQ(ierr);

  /*
     Provide the call-back for the nonlinear function we are 
     evaluating. Thus whenever the timestepping routines need the
     function they will call this routine. Note the final argument
     is the application context used by the call-back functions.
  */
  ierr = TSSetRHSFunction(ts,FormFunction,&user);CHKERRQ(ierr);

  /*
     Set the Jacobian matrix and the function used to compute 
     Jacobians.
  */
  ierr = TSSetRHSJacobian(ts,J,J,FormJacobian,&user);CHKERRQ(ierr);

  /*
       For the initial guess for the problem
  */
  ierr = FormInitialGuess(x,&user);

  /*
       This indicates that we are using pseudo timestepping to 
     find a steady state solution to the nonlinear problem.
  */
  ierr = TSSetType(ts,TSPSEUDO);CHKERRQ(ierr);

  /*
       Set the initial time to start at (this is arbitrary for 
     steady state problems; and the initial timestep given above
  */
  ierr = TSSetInitialTimeStep(ts,0.0,dt);CHKERRQ(ierr);

  /*
      Set a large number of timesteps and final duration time
     to insure convergence to steady state.
  */
  ierr = TSSetDuration(ts,1000,1.e12);

  /*
      Use the default strategy for increasing the timestep
  */
  ierr = TSPseudoSetTimeStep(ts,TSPseudoDefaultTimeStep,0);CHKERRQ(ierr);

  /*
      Set any additional options from the options database. This
     includes all options for the nonlinear and linear solvers used
     internally the the timestepping routines.
  */
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);

  ierr = TSSetUp(ts);CHKERRQ(ierr);

  /*
      Perform the solve. This is where the timestepping takes place.
  */
  ierr = TSStep(ts,&its,&ftime);CHKERRQ(ierr);
  
  printf("Number of pseudo timesteps = %d final time %4.2e\n",(int)its,ftime);

  /* 
     Free the data structures constructed above
  */
  ierr = VecDestroy(x);CHKERRQ(ierr);
  ierr = VecDestroy(r);CHKERRQ(ierr);
  ierr = MatDestroy(J);CHKERRQ(ierr);
  ierr = TSDestroy(ts);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);

  return 0;
}
/* ------------------------------------------------------------------ */
/*           Bratu (Solid Fuel Ignition) Test Problem                 */
/* ------------------------------------------------------------------ */

/* --------------------  Form initial approximation ----------------- */

#undef __FUNCT__
#define __FUNCT__ "FormInitialGuess"
PetscErrorCode FormInitialGuess(Vec X,AppCtx *user)
{
  PetscInt       i,j,row,mx,my;
  PetscErrorCode ierr;
  PetscReal      one = 1.0,lambda;
  PetscReal      temp1,temp,hx,hy;
  PetscScalar    *x;

  mx	 = user->mx; 
  my	 = user->my;
  lambda = user->param;

  hx    = one / (PetscReal)(mx-1);
  hy    = one / (PetscReal)(my-1);

  ierr = VecGetArray(X,&x);CHKERRQ(ierr);
  temp1 = lambda/(lambda + one);
  for (j=0; j<my; j++) {
    temp = (PetscReal)(PetscMin(j,my-j-1))*hy;
    for (i=0; i<mx; i++) {
      row = i + j*mx;  
      if (i == 0 || j == 0 || i == mx-1 || j == my-1) {
        x[row] = 0.0; 
        continue;
      }
      x[row] = temp1*sqrt(PetscMin((PetscReal)(PetscMin(i,mx-i-1))*hx,temp)); 
    }
  }
  ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);
  return 0;
}
/* --------------------  Evaluate Function F(x) --------------------- */

#undef __FUNCT__
#define __FUNCT__ "FormFunction"
PetscErrorCode FormFunction(TS ts,PetscReal t,Vec X,Vec F,void *ptr)
{
  AppCtx         *user = (AppCtx*)ptr;
  PetscErrorCode ierr;
  PetscInt       i,j,row,mx,my;
  PetscReal      two = 2.0,one = 1.0,lambda;
  PetscReal      hx,hy,hxdhy,hydhx;
  PetscScalar    ut,ub,ul,ur,u,uxx,uyy,sc,*x,*f;

  mx	 = user->mx; 
  my	 = user->my;
  lambda = user->param;

  hx    = one / (PetscReal)(mx-1);
  hy    = one / (PetscReal)(my-1);
  sc    = hx*hy;
  hxdhy = hx/hy;
  hydhx = hy/hx;

  ierr = VecGetArray(X,&x);CHKERRQ(ierr);
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  for (j=0; j<my; j++) {
    for (i=0; i<mx; i++) {
      row = i + j*mx;
      if (i == 0 || j == 0 || i == mx-1 || j == my-1) {
        f[row] = x[row];
        continue;
      }
      u = x[row];
      ub = x[row - mx];
      ul = x[row - 1];
      ut = x[row + mx];
      ur = x[row + 1];
      uxx = (-ur + two*u - ul)*hydhx;
      uyy = (-ut + two*u - ub)*hxdhy;
      f[row] = -uxx + -uyy + sc*lambda*PetscExpScalar(u);
    }
  }
  ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  return 0; 
}
/* --------------------  Evaluate Jacobian F'(x) -------------------- */

#undef __FUNCT__
#define __FUNCT__ "FormJacobian"
/*
   Calculate the Jacobian matrix J(X,t).

   Note: We put the Jacobian in the preconditioner storage B instead of J. This
   way we can give the -snes_mf_operator flag to check our work. This replaces
   J with a finite difference approximation, using our analytic Jacobian B for
   the preconditioner.
*/
PetscErrorCode FormJacobian(TS ts,PetscReal t,Vec X,Mat *J,Mat *B,MatStructure *flag,void *ptr)
{
  AppCtx         *user = (AppCtx*)ptr;
  Mat            jac = *B;
  PetscInt       i,j,row,mx,my,col[5];
  PetscErrorCode ierr;
  PetscScalar    two = 2.0,one = 1.0,lambda,v[5],sc,*x;
  PetscReal      hx,hy,hxdhy,hydhx;


  mx	 = user->mx; 
  my	 = user->my;
  lambda = user->param;

  hx    = 1.0 / (PetscReal)(mx-1);
  hy    = 1.0 / (PetscReal)(my-1);
  sc    = hx*hy;
  hxdhy = hx/hy;
  hydhx = hy/hx;

  ierr = VecGetArray(X,&x);CHKERRQ(ierr);
  for (j=0; j<my; j++) {
    for (i=0; i<mx; i++) {
      row = i + j*mx;
      if (i == 0 || j == 0 || i == mx-1 || j == my-1) {
        ierr = MatSetValues(jac,1,&row,1,&row,&one,INSERT_VALUES);CHKERRQ(ierr);
        continue;
      }
      v[0] = hxdhy; col[0] = row - mx;
      v[1] = hydhx; col[1] = row - 1;
      v[2] = -two*(hydhx + hxdhy) + sc*lambda*PetscExpScalar(x[row]); col[2] = row;
      v[3] = hydhx; col[3] = row + 1;
      v[4] = hxdhy; col[4] = row + mx;
      ierr = MatSetValues(jac,1,&row,5,col,v,INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  *flag = SAME_NONZERO_PATTERN;
  return 0;
}





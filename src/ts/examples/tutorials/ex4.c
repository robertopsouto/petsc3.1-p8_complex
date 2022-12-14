
/* Program usage:  mpiexec -n <procs> ex4 [-help] [all PETSc options] */

static char help[] ="Solves a simple time-dependent linear PDE (the heat equation).\n\
Input parameters include:\n\
  -m <points>, where <points> = number of grid points\n\
  -time_dependent_rhs : Treat the problem as having a time-dependent right-hand side\n\
  -debug              : Activate debugging printouts\n\
  -nox                : Deactivate x-window graphics\n\n";

/*
   Concepts: TS^time-dependent linear problems
   Concepts: TS^heat equation
   Concepts: TS^diffusion equation
   Processors: n
*/

/* ------------------------------------------------------------------------

   This program solves the one-dimensional heat equation (also called the
   diffusion equation),
       u_t = u_xx, 
   on the domain 0 <= x <= 1, with the boundary conditions
       u(t,0) = 0, u(t,1) = 0,
   and the initial condition
       u(0,x) = sin(6*pi*x) + 3*sin(2*pi*x).
   This is a linear, second-order, parabolic equation.

   We discretize the right-hand side using finite differences with
   uniform grid spacing h:
       u_xx = (u_{i+1} - 2u_{i} + u_{i-1})/(h^2)
   We then demonstrate time evolution using the various TS methods by
   running the program via
       mpiexec -n <procs> ex3 -ts_type <timestepping solver>

   We compare the approximate solution with the exact solution, given by
       u_exact(x,t) = exp(-36*pi*pi*t) * sin(6*pi*x) +
                      3*exp(-4*pi*pi*t) * sin(2*pi*x)

   Notes:
   This code demonstrates the TS solver interface to two variants of 
   linear problems, u_t = f(u,t), namely
     - time-dependent f:   f(u,t) is a function of t
     - time-independent f: f(u,t) is simply f(u)

    The uniprocessor version of this code is ts/examples/tutorials/ex3.c

  ------------------------------------------------------------------------- */

/* 
   Include "petscda.h" so that we can use distributed arrays (DAs) to manage
   the parallel grid.  Include "petscts.h" so that we can use TS solvers.  
   Note that this file automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h  - vectors
     petscmat.h  - matrices
     petscis.h     - index sets            petscksp.h  - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h   - preconditioners
     petscksp.h   - linear solvers        petscsnes.h - nonlinear solvers
*/

#include "petscda.h"
#include "petscts.h"

/* 
   User-defined application context - contains data needed by the 
   application-provided call-back routines.
*/
typedef struct {
  MPI_Comm    comm;              /* communicator */
  DA          da;                /* distributed array data structure */
  Vec         localwork;         /* local ghosted work vector */
  Vec         u_local;           /* local ghosted approximate solution vector */
  Vec         solution;          /* global exact solution vector */
  PetscInt    m;                 /* total number of grid points */
  PetscReal   h;                 /* mesh width h = 1/(m-1) */
  PetscTruth  debug;             /* flag (1 indicates activation of debugging printouts) */
  PetscViewer viewer1,viewer2;  /* viewers for the solution and error */
  PetscReal   norm_2,norm_max;  /* error norms */
} AppCtx;

/* 
   User-defined routines
*/
extern PetscErrorCode InitialConditions(Vec,AppCtx*);
extern PetscErrorCode RHSMatrixHeat(TS,PetscReal,Mat*,Mat*,MatStructure*,void*);
extern PetscErrorCode RHSFunctionHeat(TS,PetscReal,Vec,Vec,void*);
extern PetscErrorCode Monitor(TS,PetscInt,PetscReal,Vec,void*);
extern PetscErrorCode ExactSolution(PetscReal,Vec,AppCtx*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  AppCtx         appctx;                 /* user-defined application context */
  TS             ts;                     /* timestepping context */
  Mat            A;                      /* matrix data structure */
  Vec            u;                      /* approximate solution vector */
  PetscReal      time_total_max = 100.0; /* default max total time */
  PetscInt       time_steps_max = 100;   /* default max timesteps */
  PetscDraw      draw;                   /* drawing context */
  PetscErrorCode ierr;
  PetscInt       steps,m;
  PetscMPIInt    size;
  PetscReal      dt,ftime;
  PetscTruth     flg;
  TSProblemType  tsproblem = TS_LINEAR;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program and set problem parameters
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 
  appctx.comm = PETSC_COMM_WORLD;

  m    = 60;
  ierr = PetscOptionsGetInt(PETSC_NULL,"-m",&m,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(PETSC_NULL,"-debug",&appctx.debug);CHKERRQ(ierr);    
  appctx.m        = m;
  appctx.h        = 1.0/(m-1.0);
  appctx.norm_2   = 0.0;
  appctx.norm_max = 0.0;
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Solving a linear TS problem, number of processors = %d\n",size);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create vector data structures
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Create distributed array (DA) to manage parallel grid and vectors
     and to set up the ghost point communication pattern.  There are M 
     total grid values spread equally among all the processors.
  */ 

  ierr = DACreate1d(PETSC_COMM_WORLD,DA_NONPERIODIC,m,1,1,PETSC_NULL,&appctx.da);CHKERRQ(ierr);

  /*
     Extract global and local vectors from DA; we use these to store the
     approximate solution.  Then duplicate these for remaining vectors that
     have the same types.
  */ 
  ierr = DACreateGlobalVector(appctx.da,&u);CHKERRQ(ierr);
  ierr = DACreateLocalVector(appctx.da,&appctx.u_local);CHKERRQ(ierr);

  /* 
     Create local work vector for use in evaluating right-hand-side function;
     create global work vector for storing exact solution.
  */
  ierr = VecDuplicate(appctx.u_local,&appctx.localwork);CHKERRQ(ierr);
  ierr = VecDuplicate(u,&appctx.solution);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set up displays to show graphs of the solution and error 
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",80,380,400,160,&appctx.viewer1);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDraw(appctx.viewer1,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);   
  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",80,0,400,160,&appctx.viewer2);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDraw(appctx.viewer2,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);   

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);

  ierr = PetscOptionsHasName(PETSC_NULL,"-nonlinear",&flg);CHKERRQ(ierr);
  if (flg) tsproblem = TS_NONLINEAR;
#if defined(PETSC_HAVE_SUNDIALS)
  tsproblem = TS_NONLINEAR;
#endif
  ierr = TSSetProblemType(ts,tsproblem);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set optional user-defined monitoring routine
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSMonitorSet(ts,Monitor,&appctx,PETSC_NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

     Create matrix data structure; set matrix evaluation routine.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,m,m);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);

  ierr = PetscOptionsHasName(PETSC_NULL,"-time_dependent_rhs",&flg);CHKERRQ(ierr);
  if (flg) {
    /*
       For linear problems with a time-dependent f(u,t) in the equation 
       u_t = f(u,t), the user provides the discretized right-hand-side
       as a time-dependent matrix.
    */
    ierr = TSSetMatrices(ts,A,RHSMatrixHeat,PETSC_NULL,PETSC_NULL,DIFFERENT_NONZERO_PATTERN,&appctx);CHKERRQ(ierr);
  } else {
    /*
       For linear problems with a time-independent f(u) in the equation 
       u_t = f(u), the user provides the discretized right-hand-side
       as a matrix only once, and then sets a null matrix evaluation
       routine.
    */
    MatStructure A_structure;
    ierr = RHSMatrixHeat(ts,0.0,&A,&A,&A_structure,&appctx);CHKERRQ(ierr);
    ierr = TSSetMatrices(ts,A,PETSC_NULL,PETSC_NULL,PETSC_NULL,DIFFERENT_NONZERO_PATTERN,&appctx);CHKERRQ(ierr);
  }
 
  if ( tsproblem == TS_NONLINEAR){
    ierr = TSSetRHSFunction(ts,RHSFunctionHeat,&appctx);CHKERRQ(ierr);
    ierr = TSSetRHSJacobian(ts,A,A,TSDefaultComputeJacobian,&appctx);CHKERRQ(ierr); 
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set solution vector and initial timestep
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  dt = appctx.h*appctx.h/2.0;
  ierr = TSSetInitialTimeStep(ts,0.0,dt);CHKERRQ(ierr);
  ierr = TSSetSolution(ts,u);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Customize timestepping solver:  
       - Set the solution method to be the Backward Euler method.
       - Set timestepping duration info 
     Then set runtime options, which can override these defaults.
     For example,
          -ts_max_steps <maxsteps> -ts_max_time <maxtime>
     to override the defaults set by TSSetDuration().
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSSetDuration(ts,time_steps_max,time_total_max);CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve the problem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Evaluate initial conditions
  */
  ierr = InitialConditions(u,&appctx);CHKERRQ(ierr);

  /*
     Run the timestepping solver
  */
  ierr = TSStep(ts,&steps,&ftime);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     View timestepping solver info
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = PetscPrintf(PETSC_COMM_WORLD,"avg. error (2 norm) = %G, avg. error (max norm) = %G\n",
              appctx.norm_2/steps,appctx.norm_max/steps);CHKERRQ(ierr);
  ierr = TSView(ts,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSDestroy(ts);CHKERRQ(ierr);
  ierr = MatDestroy(A);CHKERRQ(ierr);
  ierr = VecDestroy(u);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(appctx.viewer1);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(appctx.viewer2);CHKERRQ(ierr);
  ierr = VecDestroy(appctx.localwork);CHKERRQ(ierr);
  ierr = VecDestroy(appctx.solution);CHKERRQ(ierr);
  ierr = VecDestroy(appctx.u_local);CHKERRQ(ierr);
  ierr = DADestroy(appctx.da);CHKERRQ(ierr);

  /*
     Always call PetscFinalize() before exiting a program.  This routine
       - finalizes the PETSc libraries as well as MPI
       - provides summary and diagnostic information if certain runtime
         options are chosen (e.g., -log_summary). 
  */
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "InitialConditions"
/*
   InitialConditions - Computes the solution at the initial time. 

   Input Parameter:
   u - uninitialized solution vector (global)
   appctx - user-defined application context

   Output Parameter:
   u - vector with solution at initial time (global)
*/ 
PetscErrorCode InitialConditions(Vec u,AppCtx *appctx)
{
  PetscScalar    *u_localptr,h = appctx->h;
  PetscInt       i,mybase,myend;
  PetscErrorCode ierr;

  /* 
     Determine starting point of each processor's range of
     grid values.
  */
  ierr = VecGetOwnershipRange(u,&mybase,&myend);CHKERRQ(ierr);

  /* 
    Get a pointer to vector data.
    - For default PETSc vectors, VecGetArray() returns a pointer to
      the data array.  Otherwise, the routine is implementation dependent.
    - You MUST call VecRestoreArray() when you no longer need access to
      the array.
    - Note that the Fortran interface to VecGetArray() differs from the
      C version.  See the users manual for details.
  */
  ierr = VecGetArray(u,&u_localptr);CHKERRQ(ierr);

  /* 
     We initialize the solution array by simply writing the solution
     directly into the array locations.  Alternatively, we could use
     VecSetValues() or VecSetValuesLocal().
  */
  for (i=mybase; i<myend; i++) {
    u_localptr[i-mybase] = PetscSinScalar(PETSC_PI*i*6.*h) + 3.*PetscSinScalar(PETSC_PI*i*2.*h);
  }

  /* 
     Restore vector
  */
  ierr = VecRestoreArray(u,&u_localptr);CHKERRQ(ierr);

  /* 
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"initial guess vector\n");CHKERRQ(ierr);
     ierr = VecView(u,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "ExactSolution"
/*
   ExactSolution - Computes the exact solution at a given time.

   Input Parameters:
   t - current time
   solution - vector in which exact solution will be computed
   appctx - user-defined application context

   Output Parameter:
   solution - vector with the newly computed exact solution
*/
PetscErrorCode ExactSolution(PetscReal t,Vec solution,AppCtx *appctx)
{
  PetscScalar    *s_localptr,h = appctx->h,ex1,ex2,sc1,sc2;
  PetscInt       i,mybase,myend;
  PetscErrorCode ierr;

  /* 
     Determine starting and ending points of each processor's 
     range of grid values
  */
  ierr = VecGetOwnershipRange(solution,&mybase,&myend);CHKERRQ(ierr);

  /*
     Get a pointer to vector data.
  */
  ierr = VecGetArray(solution,&s_localptr);CHKERRQ(ierr);

  /* 
     Simply write the solution directly into the array locations.
     Alternatively, we culd use VecSetValues() or VecSetValuesLocal().
  */
  ex1 = exp(-36.*PETSC_PI*PETSC_PI*t); ex2 = exp(-4.*PETSC_PI*PETSC_PI*t);
  sc1 = PETSC_PI*6.*h;                 sc2 = PETSC_PI*2.*h;
  for (i=mybase; i<myend; i++) {
    s_localptr[i-mybase] = PetscSinScalar(sc1*(PetscReal)i)*ex1 + 3.*PetscSinScalar(sc2*(PetscReal)i)*ex2;
  }

  /* 
     Restore vector
  */
  ierr = VecRestoreArray(solution,&s_localptr);CHKERRQ(ierr);
  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "Monitor"
/*
   Monitor - User-provided routine to monitor the solution computed at 
   each timestep.  This example plots the solution and computes the
   error in two different norms.

   Input Parameters:
   ts     - the timestep context
   step   - the count of the current step (with 0 meaning the
             initial condition)
   time   - the current time
   u      - the solution at this timestep
   ctx    - the user-provided context for this monitoring routine.
            In this case we use the application context which contains 
            information about the problem size, workspace and the exact 
            solution.
*/
PetscErrorCode Monitor(TS ts,PetscInt step,PetscReal time,Vec u,void *ctx)
{
  AppCtx         *appctx = (AppCtx*) ctx;   /* user-defined application context */
  PetscErrorCode ierr;
  PetscReal      norm_2,norm_max;

  /* 
     View a graph of the current iterate
  */
  ierr = VecView(u,appctx->viewer2);CHKERRQ(ierr);

  /* 
     Compute the exact solution
  */
  ierr = ExactSolution(time,appctx->solution,appctx);CHKERRQ(ierr);

  /*
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"Computed solution vector\n");CHKERRQ(ierr);
     ierr = VecView(u,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
     ierr = PetscPrintf(appctx->comm,"Exact solution vector\n");CHKERRQ(ierr);
     ierr = VecView(appctx->solution,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  /*
     Compute the 2-norm and max-norm of the error
  */
  ierr = VecAXPY(appctx->solution,-1.0,u);CHKERRQ(ierr);
  ierr = VecNorm(appctx->solution,NORM_2,&norm_2);CHKERRQ(ierr);
  norm_2 = sqrt(appctx->h)*norm_2;
  ierr = VecNorm(appctx->solution,NORM_MAX,&norm_max);CHKERRQ(ierr);

  /*
     PetscPrintf() causes only the first processor in this 
     communicator to print the timestep information.
  */
  ierr = PetscPrintf(appctx->comm,"Timestep %D: time = %G, 2-norm error = %G, max norm error = %G\n",
              step,time,norm_2,norm_max);CHKERRQ(ierr);
  appctx->norm_2   += norm_2;
  appctx->norm_max += norm_max;

  /* 
     View a graph of the error
  */
  ierr = VecView(appctx->solution,appctx->viewer1);CHKERRQ(ierr);

  /*
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"Error vector\n");CHKERRQ(ierr);
     ierr = VecView(appctx->solution,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  return 0;
}

/* --------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "RHSMatrixHeat"
/*
   RHSMatrixHeat - User-provided routine to compute the right-hand-side
   matrix for the heat equation.

   Input Parameters:
   ts - the TS context
   t - current time
   global_in - global input vector
   dummy - optional user-defined context, as set by TSetRHSJacobian()

   Output Parameters:
   AA - Jacobian matrix
   BB - optionally different preconditioning matrix
   str - flag indicating matrix structure

  Notes:
  RHSMatrixHeat computes entries for the locally owned part of the system.
   - Currently, all PETSc parallel matrix formats are partitioned by
     contiguous chunks of rows across the processors. 
   - Each processor needs to insert only elements that it owns
     locally (but any non-local elements will be sent to the
     appropriate processor during matrix assembly). 
   - Always specify global row and columns of matrix entries when
     using MatSetValues(); we could alternatively use MatSetValuesLocal().
   - Here, we set all entries for a particular row at once.
   - Note that MatSetValues() uses 0-based row and column numbers
     in Fortran as well as in C.
*/
PetscErrorCode RHSMatrixHeat(TS ts,PetscReal t,Mat *AA,Mat *BB,MatStructure *str,void *ctx)
{
  Mat            A = *AA;                      /* Jacobian matrix */
  AppCtx         *appctx = (AppCtx*)ctx;     /* user-defined application context */
  PetscErrorCode ierr;
  PetscInt       i,mstart,mend,idx[3];
  PetscScalar    v[3],stwo = -2./(appctx->h*appctx->h),sone = -.5*stwo;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute entries for the locally owned part of the matrix
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatGetOwnershipRange(A,&mstart,&mend);CHKERRQ(ierr);

  /* 
     Set matrix rows corresponding to boundary data
  */

  if (mstart == 0) {  /* first processor only */
    v[0] = 1.0;
    ierr = MatSetValues(A,1,&mstart,1,&mstart,v,INSERT_VALUES);CHKERRQ(ierr);
    mstart++;
  }

  if (mend == appctx->m) { /* last processor only */
    mend--;
    v[0] = 1.0;
    ierr = MatSetValues(A,1,&mend,1,&mend,v,INSERT_VALUES);CHKERRQ(ierr);
  }

  /*
     Set matrix rows corresponding to interior data.  We construct the 
     matrix one row at a time.
  */
  v[0] = sone; v[1] = stwo; v[2] = sone;  
  for (i=mstart; i<mend; i++) {
    idx[0] = i-1; idx[1] = i; idx[2] = i+1;
    ierr = MatSetValues(A,1,&i,3,idx,v,INSERT_VALUES);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Complete the matrix assembly process and set some options
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Assemble matrix, using the 2-step process:
       MatAssemblyBegin(), MatAssemblyEnd()
     Computations can be done while messages are in transition
     by placing code between these two statements.
  */
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /*
     Set flag to indicate that the Jacobian matrix retains an identical
     nonzero structure throughout all timestepping iterations (although the
     values of the entries change). Thus, we can save some work in setting
     up the preconditioner (e.g., no need to redo symbolic factorization for
     ILU/ICC preconditioners).
      - If the nonzero structure of the matrix is different during
        successive linear solves, then the flag DIFFERENT_NONZERO_PATTERN
        must be used instead.  If you are unsure whether the matrix
        structure has changed or not, use the flag DIFFERENT_NONZERO_PATTERN.
      - Caution:  If you specify SAME_NONZERO_PATTERN, PETSc
        believes your assertion and does not check the structure
        of the matrix.  If you erroneously claim that the structure
        is the same when it actually is not, the new preconditioner
        will not function correctly.  Thus, use this optimization
        feature with caution!
  */
  *str = SAME_NONZERO_PATTERN;

  /*
     Set and option to indicate that we will never add a new nonzero location 
     to the matrix. If we do, it will generate an error.
  */
  ierr = MatSetOption(A,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);

  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "RHSFunctionHeat"
PetscErrorCode RHSFunctionHeat(TS ts,PetscReal t,Vec globalin,Vec globalout,void *ctx)
{
  PetscErrorCode ierr;
  Mat            A;
  MatStructure   A_structure;

  PetscFunctionBegin;
  ierr = TSGetMatrices(ts,&A,PETSC_NULL,&ctx);CHKERRQ(ierr);
  ierr = RHSMatrixHeat(ts,t,&A,PETSC_NULL,&A_structure,ctx);CHKERRQ(ierr);
  /* ierr = MatView(A,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr); */
  ierr = MatMult(A,globalin,globalout);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}






static char help[] = "Nonlinear driven cavity with multigrid and pseudo timestepping 2d.\n\
  \n\
The 2D driven cavity problem is solved in a velocity-vorticity formulation.\n\
The flow can be driven with the lid or with bouyancy or both:\n\
  -lidvelocity <lid>, where <lid> = dimensionless velocity of lid\n\
  -grashof <gr>, where <gr> = dimensionless temperature gradent\n\
  -prandtl <pr>, where <pr> = dimensionless thermal/momentum diffusity ratio\n\
  -contours : draw contour plots of solution\n\n";

/*T
   Concepts: SNES^solving a system of nonlinear equations (parallel multicomponent example);
   Concepts: DA^using distributed arrays;
   Concepts: multicomponent
   Processors: n
T*/

/* ------------------------------------------------------------------------

    We thank David E. Keyes for contributing the driven cavity discretization
    within this example code.

    This problem is modeled by the partial differential equation system
  
	dU/dt          - Lap(U)     - Grad_y(Omega) = 0
	dV/dt          - Lap(V)     + Grad_x(Omega) = 0
	d(omega)/dt    - Lap(Omega) + Div([U*Omega,V*Omega]) - GR*Grad_x(T) = 0
	dT/dt          - Lap(T)     + PR*Div([U*T,V*T]) = 0

    in the unit square, which is uniformly discretized in each of x and
    y in this simple encoding.

    No-slip, rigid-wall Dirichlet conditions are used for [U,V].
    Dirichlet conditions are used for Omega, based on the definition of
    vorticity: Omega = - Grad_y(U) + Grad_x(V), where along each
    constant coordinate boundary, the tangential derivative is zero.
    Dirichlet conditions are used for T on the left and right walls,
    and insulation homogeneous Neumann conditions are used for T on
    the top and bottom walls. 

    A finite difference approximation with the usual 5-point stencil 
    is used to discretize the boundary value problem to obtain a 
    nonlinear system of equations.  Upwinding is used for the divergence
    (convective) terms and central for the gradient (source) terms.
    
    The Jacobian can be either
      * formed via finite differencing using coloring (the default), or
      * applied matrix-free via the option -snes_mf 
        (for larger grid problems this variant may not converge 
        without a preconditioner due to ill-conditioning).


    This example is used in the paper

        Todd S. Coffey and C. T. Kelley and David E. Keyes,
        Pseudotransient Continuation and Differential-Algebraic Equations, 2003.

    however Figure 3.1 was generated with a slightly different algorithm (see
    targets runex27 and runex27_p) than described in the paper.  In particular,
    the described algorithm is linearly implicit, advancing to the next step
    after one Newton step, so that the steady state residual is always used, but
    the figure was generated by converging each step to a relative tolerance of
    1.e-3.  On the example problem, setting -snes_max_it 1 has only minor impact
    on number of steps, but significantly reduces the required number of linear
    solves.

  ------------------------------------------------------------------------- */

/* 
   Include "petscda.h" so that we can use distributed arrays (DAs).
   Include "petscsnes.h" so that we can use SNES solvers.  Note that this
   file automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers 
*/
#include "petscsnes.h"
#include "petscda.h"
#include "petscdmmg.h"

/* 
   User-defined routines and data structures
*/

typedef struct {
  PassiveScalar  fnorm_ini,dt_ini,cfl_ini;
  PassiveScalar  fnorm,dt,cfl;
  PassiveScalar  ptime;
  PassiveScalar  cfl_max,max_time;
  PassiveScalar  fnorm_ratio;
  PetscInt       ires,iramp,itstep;
  PetscInt       max_steps,print_freq;
  PetscInt       LocalTimeStepping;                         
  PetscTruth     use_parabolic;
} TstepCtx;

/*
    The next two structures are essentially the same. The difference is that
  the first does not contain derivative information (as used by ADIC) while the
  second does. The first is used only to contain the solution at the previous time-step
  which is a constant in the computation of the current time step and hence passive 
  (meaning not active in terms of derivatives).
*/
typedef struct {
  PassiveScalar u,v,omega,temp;
} PassiveField;

typedef struct {
  PetscScalar u,v,omega,temp;
} Field;


typedef struct {
  PetscInt     mglevels;
  PetscInt     cycles;                       /* numbers of time steps for integration */ 
  PassiveReal  lidvelocity,prandtl,grashof;  /* physical parameters */
  PetscTruth   draw_contours;                /* indicates drawing contours of solution */
  PetscTruth   PreLoading;
} Parameter;

typedef struct {
  Vec          Xold,func;
  TstepCtx     *tsCtx;
  Parameter    *param;
} AppCtx;

extern PetscErrorCode FormInitialGuess(DMMG,Vec);
extern PetscErrorCode FormFunctionLocal(DALocalInfo*,Field**,Field**,void*);
extern PetscErrorCode FormFunctionLocali(DALocalInfo*,MatStencil*,Field**,PetscScalar*,void*);
extern PetscErrorCode Update(DMMG *);
extern PetscErrorCode Initialize(DMMG *);
extern PetscErrorCode ComputeTimeStep(SNES,void*);
extern PetscErrorCode AddTSTermLocal(DALocalInfo*,Field**,Field**,void*);


#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  DMMG           *dmmg;               /* multilevel grid structure */
  AppCtx         *user;                /* user-defined work context */
  TstepCtx       tsCtx;
  Parameter      param;
  PetscInt       mx,my,i;
  PetscErrorCode ierr;
  MPI_Comm       comm;
  DA             da;

  PetscInitialize(&argc,&argv,(char *)0,help);
  comm = PETSC_COMM_WORLD;


  PreLoadBegin(PETSC_TRUE,"SetUp");

    param.PreLoading = PreLoading;
    ierr = DMMGCreate(comm,2,&user,&dmmg);CHKERRQ(ierr);
    param.mglevels = DMMGGetLevels(dmmg);


    /*
      Create distributed array multigrid object (DMMG) to manage parallel grid and vectors
      for principal unknowns (x) and governing residuals (f)
    */
    ierr = DACreate2d(comm,DA_NONPERIODIC,DA_STENCIL_STAR,-4,-4,PETSC_DECIDE,PETSC_DECIDE,4,1,0,0,&da);CHKERRQ(ierr);
    ierr = DMMGSetDM(dmmg,(DM)da);CHKERRQ(ierr);
    ierr = DADestroy(da);CHKERRQ(ierr);

    ierr = DAGetInfo(DMMGGetDA(dmmg),0,&mx,&my,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,
                     PETSC_IGNORE,PETSC_IGNORE);CHKERRQ(ierr);
    /* 
     Problem parameters (velocity of lid, prandtl, and grashof numbers)
    */
    param.lidvelocity = 1.0/(mx*my);
    param.prandtl     = 1.0;
    param.grashof     = 1.0;
    ierr = PetscOptionsGetReal(PETSC_NULL,"-lidvelocity",&param.lidvelocity,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsGetReal(PETSC_NULL,"-prandtl",&param.prandtl,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsGetReal(PETSC_NULL,"-grashof",&param.grashof,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsHasName(PETSC_NULL,"-contours",&param.draw_contours);CHKERRQ(ierr);

    ierr = DASetFieldName(DMMGGetDA(dmmg),0,"x-velocity");CHKERRQ(ierr);
    ierr = DASetFieldName(DMMGGetDA(dmmg),1,"y-velocity");CHKERRQ(ierr);
    ierr = DASetFieldName(DMMGGetDA(dmmg),2,"Omega");CHKERRQ(ierr);
    ierr = DASetFieldName(DMMGGetDA(dmmg),3,"temperature");CHKERRQ(ierr);

    /*======================================================================*/
    /* Initilize stuff related to time stepping */
    /*======================================================================*/
    tsCtx.fnorm_ini = 0.0;  tsCtx.cfl_ini     = 50.0;    tsCtx.cfl_max = 1.0e+06;
    tsCtx.max_steps = 50;   tsCtx.max_time    = 1.0e+12; tsCtx.iramp   = -50;
    tsCtx.dt        = 0.01; tsCtx.fnorm_ratio = 1.0e+10;
    tsCtx.LocalTimeStepping = 0;
    tsCtx.use_parabolic     = PETSC_FALSE;
    ierr = PetscOptionsGetTruth(PETSC_NULL,"-use_parabolic",&tsCtx.use_parabolic,PETSC_IGNORE);CHKERRQ(ierr);
    ierr = PetscOptionsGetInt(PETSC_NULL,"-max_st",&tsCtx.max_steps,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsGetReal(PETSC_NULL,"-ts_rtol",&tsCtx.fnorm_ratio,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsGetReal(PETSC_NULL,"-cfl_ini",&tsCtx.cfl_ini,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsGetReal(PETSC_NULL,"-cfl_max",&tsCtx.cfl_max,PETSC_NULL);CHKERRQ(ierr);
    tsCtx.print_freq = tsCtx.max_steps; 
    ierr = PetscOptionsGetInt(PETSC_NULL,"-print_freq",&tsCtx.print_freq,PETSC_NULL);CHKERRQ(ierr);
    
    ierr = PetscMalloc(param.mglevels*sizeof(AppCtx),&user);CHKERRQ(ierr);
    for (i=0; i<param.mglevels; i++) {
      ierr = VecDuplicate(dmmg[i]->x, &(user[i].Xold));CHKERRQ(ierr);
      ierr = VecDuplicate(dmmg[i]->x, &(user[i].func));CHKERRQ(ierr);
      user[i].tsCtx = &tsCtx;
      user[i].param = &param;
      dmmg[i]->user = &user[i];
    }
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Create user context, set problem data, create vector data structures.
       Also, compute the initial guess.
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Create nonlinear solver context
       
       Process adiC(20):  AddTSTermLocal FormFunctionLocal FormFunctionLocali
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    ierr = DMMGSetSNESLocal(dmmg,FormFunctionLocal,0,ad_FormFunctionLocal,admf_FormFunctionLocal);CHKERRQ(ierr);
    ierr = DMMGSetFromOptions(dmmg);CHKERRQ(ierr);
    ierr = DMMGSetSNESLocali(dmmg,FormFunctionLocali,ad_FormFunctionLocali,admf_FormFunctionLocali);CHKERRQ(ierr);
    
    ierr = PetscPrintf(comm,"lid velocity = %G, prandtl # = %G, grashof # = %G\n",
		       param.lidvelocity,param.prandtl,param.grashof);CHKERRQ(ierr);
    
    
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Solve the nonlinear system
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    ierr = DMMGSetInitialGuess(dmmg,FormInitialGuess);CHKERRQ(ierr);
    
    PreLoadStage("Solve");
    ierr = Initialize(dmmg);CHKERRQ(ierr);
    ierr = Update(dmmg);CHKERRQ(ierr);
    /*
      Visualize solution
    */
    
    if (param.draw_contours) {
      ierr = VecView(DMMGGetx(dmmg),PETSC_VIEWER_DRAW_WORLD);CHKERRQ(ierr);
    }
    /*ierr = VecView(DMMGGetx(dmmg),PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);*/   
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Free work space.  All PETSc objects should be destroyed when they
       are no longer needed.
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    
    for (i=0; i<param.mglevels; i++) {
      ierr = VecDestroy(user[i].Xold);CHKERRQ(ierr);
      ierr = VecDestroy(user[i].func);CHKERRQ(ierr);
    }
    ierr = PetscFree(user);CHKERRQ(ierr);
    ierr = DMMGDestroy(dmmg);CHKERRQ(ierr);
    PreLoadEnd();
    
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "Initialize"
/* ------------------------------------------------------------------- */
PetscErrorCode Initialize(DMMG *dmmg)
{
  AppCtx         *user = (AppCtx*)dmmg[0]->user;
  DA             da;
  Parameter      *param = user->param;
  PetscInt       i,j,mx,xs,ys,xm,ym,mglevel;
  PetscErrorCode ierr;
  PetscReal      grashof,dx;
  Field          **x;

  da = (DA)(dmmg[param->mglevels-1]->dm);
  grashof = user->param->grashof;

  ierr = DAGetInfo(da,0,&mx,0,0,0,0,0,0,0,0,0);CHKERRQ(ierr);
  dx  = 1.0/(mx-1);

  /*
     Get local grid boundaries (for 2-dimensional DA):
       xs, ys   - starting grid indices (no ghost points)
       xm, ym   - widths of local grid (no ghost points)
  */
  ierr = DAGetCorners(da,&xs,&ys,PETSC_NULL,&xm,&ym,PETSC_NULL);CHKERRQ(ierr);

  /*
     Get a pointer to vector data.
       - For default PETSc vectors, VecGetArray() returns a pointer to
         the data array.  Otherwise, the routine is implementation dependent.
       - You MUST call VecRestoreArray() when you no longer need access to
         the array.
  */
  ierr = DAVecGetArray(da,((AppCtx*)dmmg[param->mglevels-1]->user)->Xold,&x);CHKERRQ(ierr);

  /*
     Compute initial guess over the locally owned part of the grid
     Initial condition is motionless fluid and equilibrium temperature
  */
  for (j=ys; j<ys+ym; j++) {
    for (i=xs; i<xs+xm; i++) {
      x[j][i].u     = 0.0;
      x[j][i].v     = 0.0;
      x[j][i].omega = 0.0;
      x[j][i].temp  = (grashof>0)*i*dx;
    }
  }

  /*
     Restore vector
  */
  ierr = DAVecRestoreArray(da,((AppCtx*)dmmg[param->mglevels-1]->user)->Xold,&x);CHKERRQ(ierr);
  /* Restrict Xold to coarser levels */
  for (mglevel=param->mglevels-1; mglevel>0; mglevel--) {
    ierr = MatRestrict(dmmg[mglevel]->R, ((AppCtx*)dmmg[mglevel]->user)->Xold, ((AppCtx*)dmmg[mglevel-1]->user)->Xold);CHKERRQ(ierr);
    ierr = VecPointwiseMult(((AppCtx*)dmmg[mglevel-1]->user)->Xold,((AppCtx*)dmmg[mglevel-1]->user)->Xold,dmmg[mglevel]->Rscale);CHKERRQ(ierr);
  }
  
  /* Store X in the qold for time stepping */
  /*ierr = VecDuplicate(X,&tsCtx->qold);CHKERRQ(ierr);
  ierr = VecDuplicate(X,&tsCtx->func);CHKERRQ(ierr);
  ierr = VecCopy(X,tsCtx->Xold);CHKERRQ(ierr);
  tsCtx->ires = 0;
  ierr = SNESComputeFunction(snes,tsCtx->Xold,tsCtx->func);
  ierr = VecNorm(tsCtx->func,NORM_2,&tsCtx->fnorm_ini);CHKERRQ(ierr);
  tsCtx->ires = 1;
  PetscPrintf(PETSC_COMM_WORLD,"Initial function norm is %G\n",tsCtx->fnorm_ini);*/
  return 0;
} 
/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormInitialGuess"
/* 
   FormInitialGuess - Forms initial approximation.

   Input Parameters:
   user - user-defined application context
   X - vector

   Output Parameter:
   X - vector
 */
PetscErrorCode FormInitialGuess(DMMG dmmg,Vec X)
{
  AppCtx         *user = (AppCtx*)dmmg->user;
  TstepCtx       *tsCtx = user->tsCtx;
  PetscErrorCode ierr;

  ierr = VecCopy(user->Xold, X);CHKERRQ(ierr);
  /* calculate the residual on fine mesh */
  if (user->tsCtx->fnorm_ini == 0.0) {
    tsCtx->ires = 0;
    ierr = SNESComputeFunction(dmmg->snes,user->Xold,user->func);
    ierr = VecNorm(user->func,NORM_2,&tsCtx->fnorm_ini);CHKERRQ(ierr);
    tsCtx->ires = 1;
    tsCtx->cfl = tsCtx->cfl_ini;
  }
  return 0;
} 

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "AddTSTermLocal"
PetscErrorCode AddTSTermLocal(DALocalInfo* info,Field **x,Field **f,void *ptr)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user = (AppCtx*)ptr;
  TstepCtx       *tsCtx = user->tsCtx;
  DA             da = info->da;
  PetscErrorCode ierr;
  PetscInt       i,j, xints,xinte,yints,yinte;
  PetscReal      hx,hy,dhx,dhy,hxhy;
  PassiveScalar  dtinv;
  PassiveField   **xold;
  PetscTruth     use_parab = tsCtx->use_parabolic;

  PetscFunctionBegin; 
  xints = info->xs; xinte = info->xs+info->xm; yints = info->ys; yinte = info->ys+info->ym;
  dhx = (PetscReal)(info->mx-1);  dhy = (PetscReal)(info->my-1);
  hx = 1.0/dhx;                   hy = 1.0/dhy;
  hxhy = hx*hy;
  ierr = DAVecGetArray(da,user->Xold,&xold);CHKERRQ(ierr);
  dtinv = hxhy/(tsCtx->cfl*tsCtx->dt);
  /* 
     use_parab = PETSC_TRUE for parabolic equations; all the four equations have temporal term.
               = PETSC_FALSE for differential algebraic equtions (DAE); 
                 velocity equations do not have temporal term.
  */
  for (j=yints; j<yinte; j++) {
    for (i=xints; i<xinte; i++) {
      if (use_parab) {
	f[j][i].u     += dtinv*(x[j][i].u-xold[j][i].u);
	f[j][i].v     += dtinv*(x[j][i].v-xold[j][i].v);
      }
      f[j][i].omega += dtinv*(x[j][i].omega-xold[j][i].omega);
      f[j][i].temp  += dtinv*(x[j][i].temp-xold[j][i].temp);
    }
  }
  ierr = DAVecRestoreArray(da,user->Xold,&xold);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocal"
PetscErrorCode FormFunctionLocal(DALocalInfo *info,Field **x,Field **f,void *ptr)
 {
  AppCtx         *user = (AppCtx*)ptr;
  TstepCtx       *tsCtx = user->tsCtx;
  PetscErrorCode ierr;
  PetscInt       xints,xinte,yints,yinte,i,j;
  PetscReal      hx,hy,dhx,dhy,hxdhy,hydhx;
  PetscReal      grashof,prandtl,lid;
  PetscScalar    u,uxx,uyy,vx,vy,avx,avy,vxp,vxm,vyp,vym;

  PetscFunctionBegin;
  grashof = user->param->grashof;  
  prandtl = user->param->prandtl;
  lid     = user->param->lidvelocity;

  /* 
     Define mesh intervals ratios for uniform grid.
     [Note: FD formulae below are normalized by multiplying through by
     local volume element to obtain coefficients O(1) in two dimensions.]
  */
  dhx = (PetscReal)(info->mx-1);  dhy = (PetscReal)(info->my-1);
  hx = 1.0/dhx;                   hy = 1.0/dhy;
  hxdhy = hx*dhy;                 hydhx = hy*dhx;

  xints = info->xs; xinte = info->xs+info->xm; yints = info->ys; yinte = info->ys+info->ym;

  /* Test whether we are on the bottom edge of the global array */
  if (yints == 0) {
    j = 0;
    yints = yints + 1;
    /* bottom edge */
    for (i=info->xs; i<info->xs+info->xm; i++) {
        f[j][i].u     = x[j][i].u;
        f[j][i].v     = x[j][i].v;
        f[j][i].omega = x[j][i].omega + (x[j+1][i].u - x[j][i].u)*dhy; 
	f[j][i].temp  = x[j][i].temp-x[j+1][i].temp;
    }
  }

  /* Test whether we are on the top edge of the global array */
  if (yinte == info->my) {
    j = info->my - 1;
    yinte = yinte - 1;
    /* top edge */
    for (i=info->xs; i<info->xs+info->xm; i++) {
        f[j][i].u     = x[j][i].u - lid;
        f[j][i].v     = x[j][i].v;
        f[j][i].omega = x[j][i].omega + (x[j][i].u - x[j-1][i].u)*dhy; 
	f[j][i].temp  = x[j][i].temp-x[j-1][i].temp;
    }
  }

  /* Test whether we are on the left edge of the global array */
  if (xints == 0) {
    i = 0;
    xints = xints + 1;
    /* left edge */
    for (j=info->ys; j<info->ys+info->ym; j++) {
      f[j][i].u     = x[j][i].u;
      f[j][i].v     = x[j][i].v;
      f[j][i].omega = x[j][i].omega - (x[j][i+1].v - x[j][i].v)*dhx; 
      f[j][i].temp  = x[j][i].temp;
    }
  }

  /* Test whether we are on the right edge of the global array */
  if (xinte == info->mx) {
    i = info->mx - 1;
    xinte = xinte - 1;
    /* right edge */ 
    for (j=info->ys; j<info->ys+info->ym; j++) {
      f[j][i].u     = x[j][i].u;
      f[j][i].v     = x[j][i].v;
      f[j][i].omega = x[j][i].omega - (x[j][i].v - x[j][i-1].v)*dhx; 
      f[j][i].temp  = x[j][i].temp - (PetscReal)(grashof>0);
    }
  }

  /* Compute over the interior points */
  for (j=yints; j<yinte; j++) {
    for (i=xints; i<xinte; i++) {

	/*
	  convective coefficients for upwinding
        */
	vx = x[j][i].u; avx = PetscAbsScalar(vx);
        vxp = .5*(vx+avx); vxm = .5*(vx-avx);
	vy = x[j][i].v; avy = PetscAbsScalar(vy);
        vyp = .5*(vy+avy); vym = .5*(vy-avy);

	/* U velocity */
        u          = x[j][i].u;
        uxx        = (2.0*u - x[j][i-1].u - x[j][i+1].u)*hydhx;
        uyy        = (2.0*u - x[j-1][i].u - x[j+1][i].u)*hxdhy;
        f[j][i].u  = uxx + uyy - .5*(x[j+1][i].omega-x[j-1][i].omega)*hx;

	/* V velocity */
        u          = x[j][i].v;
        uxx        = (2.0*u - x[j][i-1].v - x[j][i+1].v)*hydhx;
        uyy        = (2.0*u - x[j-1][i].v - x[j+1][i].v)*hxdhy;
        f[j][i].v  = uxx + uyy + .5*(x[j][i+1].omega-x[j][i-1].omega)*hy;

	/* Omega */
        u          = x[j][i].omega;
        uxx        = (2.0*u - x[j][i-1].omega - x[j][i+1].omega)*hydhx;
        uyy        = (2.0*u - x[j-1][i].omega - x[j+1][i].omega)*hxdhy;
	f[j][i].omega = uxx + uyy + 
			(vxp*(u - x[j][i-1].omega) +
			  vxm*(x[j][i+1].omega - u)) * hy +
			(vyp*(u - x[j-1][i].omega) +
			  vym*(x[j+1][i].omega - u)) * hx -
			.5 * grashof * (x[j][i+1].temp - x[j][i-1].temp) * hy;

        /* Temperature */
        u             = x[j][i].temp;
        uxx           = (2.0*u - x[j][i-1].temp - x[j][i+1].temp)*hydhx;
        uyy           = (2.0*u - x[j-1][i].temp - x[j+1][i].temp)*hxdhy;
	f[j][i].temp =  uxx + uyy  + prandtl * (
			(vxp*(u - x[j][i-1].temp) +
			  vxm*(x[j][i+1].temp - u)) * hy +
		        (vyp*(u - x[j-1][i].temp) +
		       	  vym*(x[j+1][i].temp - u)) * hx);
    }
  }

  /* Add time step contribution */
  if (tsCtx->ires) {
    ierr = AddTSTermLocal(info,x,f,ptr);CHKERRQ(ierr);
  }
  /*
     Flop count (multiply-adds are counted as 2 operations)
  */
  ierr = PetscLogFlops(84.0*info->ym*info->xm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
} 

#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocali"
PetscErrorCode FormFunctionLocali(DALocalInfo *info,MatStencil *st,Field **x,PetscScalar *f,void *ptr)
 {
  AppCtx      *user = (AppCtx*)ptr;
  PetscInt    i,j,c;
  PassiveReal hx,hy,dhx,dhy,hxdhy,hydhx;
  PassiveReal grashof,prandtl,lid;
  PetscScalar u,uxx,uyy,vx,vy,avx,avy,vxp,vxm,vyp,vym;

  PetscFunctionBegin;
  grashof = user->param->grashof;  
  prandtl = user->param->prandtl;
  lid     = user->param->lidvelocity;

  /* 
     Define mesh intervals ratios for uniform grid.
     [Note: FD formulae below are normalized by multiplying through by
     local volume element to obtain coefficients O(1) in two dimensions.]
  */
  dhx = (PetscReal)(info->mx-1);     dhy = (PetscReal)(info->my-1);
  hx = 1.0/dhx;                   hy = 1.0/dhy;
  hxdhy = hx*dhy;                 hydhx = hy*dhx;

  i = st->i; j = st->j; c = st->c;

  /* Test whether we are on the right edge of the global array */
  if (i == info->mx-1) {
    if (c == 0) *f     = x[j][i].u;
    else if (c == 1) *f     = x[j][i].v;
    else if (c == 2) *f = x[j][i].omega - (x[j][i].v - x[j][i-1].v)*dhx; 
    else *f  = x[j][i].temp - (PetscReal)(grashof>0);

  /* Test whether we are on the left edge of the global array */
  } else if (i == 0) {
    if (c == 0) *f     = x[j][i].u;
    else if (c == 1) *f     = x[j][i].v;
    else if (c == 2) *f = x[j][i].omega - (x[j][i+1].v - x[j][i].v)*dhx; 
    else *f  = x[j][i].temp;

  /* Test whether we are on the top edge of the global array */
  } else if (j == info->my-1) {
    if (c == 0) *f     = x[j][i].u - lid;
    else if (c == 1) *f     = x[j][i].v;
    else if (c == 2) *f = x[j][i].omega + (x[j][i].u - x[j-1][i].u)*dhy; 
    else *f  = x[j][i].temp-x[j-1][i].temp;

  /* Test whether we are on the bottom edge of the global array */
  } else if (j == 0) {
    if (c == 0) *f     = x[j][i].u;
    else if (c == 1) *f     = x[j][i].v;
    else if (c == 2) *f = x[j][i].omega + (x[j+1][i].u - x[j][i].u)*dhy; 
    else *f  = x[j][i].temp-x[j+1][i].temp;

  /* Compute over the interior points */
  } else {
    /*
      convective coefficients for upwinding
    */
    vx = x[j][i].u; avx = PetscAbsScalar(vx);
    vxp = .5*(vx+avx); vxm = .5*(vx-avx);
    vy = x[j][i].v; avy = PetscAbsScalar(vy);
    vyp = .5*(vy+avy); vym = .5*(vy-avy);

    /* U velocity */
    if (c == 0) {
      u          = x[j][i].u;
      uxx        = (2.0*u - x[j][i-1].u - x[j][i+1].u)*hydhx;
      uyy        = (2.0*u - x[j-1][i].u - x[j+1][i].u)*hxdhy;
      *f         = uxx + uyy - .5*(x[j+1][i].omega-x[j-1][i].omega)*hx;

    /* V velocity */
    } else if (c == 1) {
      u          = x[j][i].v;
      uxx        = (2.0*u - x[j][i-1].v - x[j][i+1].v)*hydhx;
      uyy        = (2.0*u - x[j-1][i].v - x[j+1][i].v)*hxdhy;
      *f         = uxx + uyy + .5*(x[j][i+1].omega-x[j][i-1].omega)*hy;
    
    /* Omega */
    } else if (c == 2) {
      u          = x[j][i].omega;
      uxx        = (2.0*u - x[j][i-1].omega - x[j][i+1].omega)*hydhx;
      uyy        = (2.0*u - x[j-1][i].omega - x[j+1][i].omega)*hxdhy;
      *f         = uxx + uyy + 
	(vxp*(u - x[j][i-1].omega) +
	 vxm*(x[j][i+1].omega - u)) * hy +
	(vyp*(u - x[j-1][i].omega) +
	 vym*(x[j+1][i].omega - u)) * hx -
	.5 * grashof * (x[j][i+1].temp - x[j][i-1].temp) * hy;
    
    /* Temperature */
    } else {
      u           = x[j][i].temp;
      uxx         = (2.0*u - x[j][i-1].temp - x[j][i+1].temp)*hydhx;
      uyy         = (2.0*u - x[j-1][i].temp - x[j+1][i].temp)*hxdhy;
      *f          =  uxx + uyy  + prandtl * (
					     (vxp*(u - x[j][i-1].temp) +
					      vxm*(x[j][i+1].temp - u)) * hy +
					     (vyp*(u - x[j-1][i].temp) +
					      vym*(x[j+1][i].temp - u)) * hx);
    }
  }

  PetscFunctionReturn(0);
} 


/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "Update"
PetscErrorCode Update(DMMG *dmmg)
/*---------------------------------------------------------------------*/
{
 AppCtx         *user = (AppCtx *) ((dmmg[0])->user);
 TstepCtx 	*tsCtx = user->tsCtx;
 Parameter      *param = user->param;
 SNES           snes;
 PetscErrorCode ierr;
 PetscScalar 	fratio;
 PetscScalar 	time1,time2,cpuloc;
 PetscInt       max_steps,its;
 PetscTruth     print_flag = PETSC_FALSE;
 PetscInt       nfailsCum = 0,nfails = 0;

  PetscFunctionBegin;
  /* Note: print_flag displays diagnostic info, not convergence behavior. Use '-snes_monitor' for converges info. */
  ierr = PetscOptionsHasName(PETSC_NULL,"-print",&print_flag);CHKERRQ(ierr);
  if (user->param->PreLoading) 
   max_steps = 1;
  else
   max_steps = tsCtx->max_steps;
  fratio = 1.0;
  
  ierr = PetscGetTime(&time1);CHKERRQ(ierr);
  for (tsCtx->itstep = 0; (tsCtx->itstep < max_steps) && 
	 (fratio <= tsCtx->fnorm_ratio); tsCtx->itstep++) {
    ierr = DMMGSolve(dmmg);CHKERRQ(ierr); 
    snes = DMMGGetSNES(dmmg);CHKERRQ(ierr);
    ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Number of Newton iterations = %D\n", its);CHKERRQ(ierr);
    ierr = SNESGetNonlinearStepFailures(snes,&nfails);CHKERRQ(ierr);
    nfailsCum += nfails; nfails = 0;
    if (nfailsCum >= 2) SETERRQ(1,"Unable to find a Newton Step");
    /*tsCtx->qcur = DMMGGetx(dmmg);
      ierr = VecCopy(tsCtx->qcur,tsCtx->qold);CHKERRQ(ierr);*/
   
    ierr = VecCopy(dmmg[param->mglevels-1]->x, ((AppCtx*)dmmg[param->mglevels-1]->user)->Xold);CHKERRQ(ierr);
    for (its=param->mglevels-1; its>0 ;its--) {
      ierr = MatRestrict(dmmg[its]->R, ((AppCtx*)dmmg[its]->user)->Xold, ((AppCtx*)dmmg[its-1]->user)->Xold);CHKERRQ(ierr);
      ierr = VecPointwiseMult(((AppCtx*)dmmg[its-1]->user)->Xold,((AppCtx*)dmmg[its-1]->user)->Xold,dmmg[its]->Rscale);CHKERRQ(ierr);
    }

    
    ierr = ComputeTimeStep(snes,((AppCtx*)dmmg[param->mglevels-1]->user));CHKERRQ(ierr);
    if (print_flag) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"At Time Step %D cfl = %G and fnorm = %G\n",
			 tsCtx->itstep,tsCtx->cfl,tsCtx->fnorm);CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"Wall clock time needed %G seconds for %D time steps\n",
			 cpuloc,tsCtx->itstep);CHKERRQ(ierr);    
    }
    fratio = tsCtx->fnorm_ini/tsCtx->fnorm;
    ierr = PetscGetTime(&time2);CHKERRQ(ierr);
    cpuloc = time2-time1;            
    ierr = MPI_Barrier(PETSC_COMM_WORLD);CHKERRQ(ierr);
  } /* End of time step loop */

  if (print_flag) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Total wall clock time needed %G seconds for %D time steps\n",
		     cpuloc,tsCtx->itstep);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"cfl = %G fnorm = %G\n",tsCtx->cfl,tsCtx->fnorm);CHKERRQ(ierr);
  }
  if (user->param->PreLoading) {
    tsCtx->fnorm_ini = 0.0;
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Preloading done ...\n");CHKERRQ(ierr);
  }
  /*
  {
    Vec xx,yy;
    PetscScalar fnorm,fnorm1;
    ierr = SNESGetFunctionNorm(snes,&fnorm);CHKERRQ(ierr);
    xx = DMMGGetx(dmmg);
    ierr = VecDuplicate(xx,&yy);CHKERRQ(ierr);
    ierr = SNESComputeFunction(snes,xx,yy);
    ierr = VecNorm(yy,NORM_2,&fnorm1);CHKERRQ(ierr);
    PetscPrintf(PETSC_COMM_WORLD,"fnorm = %G, fnorm1 = %G\n",fnorm,fnorm1);
    
  }
  */

  PetscFunctionReturn(0);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ComputeTimeStep"
PetscErrorCode ComputeTimeStep(SNES snes,void *ptr)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user = (AppCtx*)ptr;
  TstepCtx       *tsCtx = user->tsCtx;
  Vec	         func = user->func;
  PetscScalar    inc = 1.1,  newcfl;
  PetscErrorCode ierr;
  /*int	       iramp = tsCtx->iramp;*/
 
  PetscFunctionBegin; 
  tsCtx->dt = 0.01;
  ierr = PetscOptionsGetScalar(PETSC_NULL,"-deltat",&tsCtx->dt,PETSC_NULL);CHKERRQ(ierr);
  tsCtx->ires = 0;
  ierr = SNESComputeFunction(snes,user->Xold,user->func);
  tsCtx->ires = 1;
  ierr = VecNorm(func,NORM_2,&tsCtx->fnorm);CHKERRQ(ierr);
  newcfl     = inc*tsCtx->cfl_ini*tsCtx->fnorm_ini/tsCtx->fnorm;
  tsCtx->cfl = PetscMin(newcfl,tsCtx->cfl_max);
  /* first time through so compute initial function norm */
  /*if (tsCtx->fnorm_ini == 0.0) {
    tsCtx->fnorm_ini = tsCtx->fnorm;
    tsCtx->cfl       = tsCtx->cfl_ini;
  } else {
    newcfl     = inc*tsCtx->cfl_ini*tsCtx->fnorm_ini/tsCtx->fnorm;
    tsCtx->cfl = PetscMin(newcfl,tsCtx->cfl_max);
    }*/
  PetscFunctionReturn(0);
}


static char help[] = 
"ex30: Steady-state 2D subduction flow, pressure and temperature solver.\n\
       The flow is driven by the subducting slab.\n\
---------------------------------ex30 help---------------------------------\n\
  -OPTION <DEFAULT> = (UNITS) DESCRIPTION.\n\n\
  -width <320> = (km) width of domain.\n\
  -depth <300> = (km) depth of domain.\n\
  -slab_dip <45> = (degrees) dip angle of the slab (determines the grid aspect ratio).\n\
  -lid_depth <35> = (km) depth of the static conductive lid.\n\
  -fault_depth <35> = (km) depth of slab-wedge mechanical coupling\n\
     ( fault dept >= lid depth ).\n\
\n\
  -ni <82> = grid cells in x-direction. (nj adjusts to accommodate\n\
      the slab dip & depth). DO NOT USE -da_grid_x option!!!\n\
  -ivisc <3> = rheology option.\n\
      0 --- constant viscosity.\n\
      1 --- olivine diffusion creep rheology (T&P-dependent, newtonian).\n\
      2 --- olivine dislocation creep rheology (T&P-dependent, non-newtonian).\n\
      3 --- Full mantle rheology, combination of 1 & 2.\n\
\n\
  -slab_velocity <5> = (cm/year) convergence rate of slab into subduction zone.\n\
  -slab_age <50> = (million yrs) age of slab for thermal profile boundary condition.\n\
  -lid_age <50> = (million yrs) age of lid for thermal profile boundary condition.\n\
\n\
  FOR OTHER PARAMETER OPTIONS AND THEIR DEFAULT VALUES, see SetParams() in ex30.c.\n\
---------------------------------ex30 help---------------------------------\n";


/* ------------------------------------------------------------------------
   
    This PETSc 2.2.0 example by Richard F. Katz
    http://www.ldeo.columbia.edu/~katz/

    The problem is modeled by the partial differential equation system
  
       (1)   -Grad(P) + Div[Eta (Grad(v) + Grad(v)^T)] = 0
       (2)                                    Div(U,W) = 0
       (3)             dT/dt + Div(vT) - 1/Pe Del^2(T) = 0
       (4)   Eta(T,Eps_dot) = constant                             if ivisc==0
                            = diffusion creep (T,P-dependent)      if ivisc==1
        	            = dislocation creep (T,P,v-dependent)  if ivisc==2
      		            = mantle viscosity (difn & disl)       if ivisc==3

    which is uniformly discretized on a staggered mesh:
                      -------w_ij------
                      |               |
                  u_i-1j    P,T_ij   u_ij
                      |               |
 		      ------w_ij-1-----

  ------------------------------------------------------------------------- */

#include "petscsnes.h"
#include "petscda.h"
#include "petscdmmg.h"

#define VISC_CONST   0
#define VISC_DIFN    1
#define VISC_DISL    2
#define VISC_FULL    3
#define CELL_CENTER  0
#define CELL_CORNER  1
#define BC_ANALYTIC  0
#define BC_NOSTRESS  1
#define BC_EXPERMNT  2
#define ADVECT_FV    0
#define ADVECT_FROMM 1
#define PLATE_SLAB   0
#define PLATE_LID    1
#define EPS_ZERO     0.00000001

typedef struct { /* holds the variables to be solved for */
  PetscScalar u,w,p,T;
} Field;

typedef struct { /* parameters needed to compute viscosity */
  PetscReal    A,n,Estar,Vstar;
} ViscParam;

typedef struct { /* physical and miscelaneous parameters */
  PetscReal    width, depth, scaled_width, scaled_depth, peclet, potentialT;
  PetscReal    slab_dip, slab_age, slab_velocity, kappa, z_scale; 
  PetscReal    c, d, sb, cb, skt, visc_cutoff, lid_age, eta0, continuation;
  PetscReal    L, V, lid_depth, fault_depth;
  ViscParam    diffusion, dislocation;
  PetscInt     ivisc, adv_scheme, ibound, output_ivisc;
  PetscTruth   quiet, param_test, output_to_file, pv_analytic;
  PetscTruth   interrupted, stop_solve, toggle_kspmon, kspmon;
  char         filename[PETSC_MAX_PATH_LEN];
} Parameter;

typedef struct { /* grid parameters */
  DAPeriodicType periodic;
  DAStencilType  stencil;
  PetscInt       corner,ni,nj,jlid,jfault,inose;
  PetscInt       dof,stencil_width,mglevels;
  PassiveScalar  dx,dz;
} GridInfo;

typedef struct { /* application context */
  Vec          Xguess;
  Parameter    *param;
  GridInfo     *grid;
} AppCtx;

/* Callback functions (static interface) */
extern PetscErrorCode FormInitialGuess(DMMG,Vec);
extern PetscErrorCode FormFunctionLocal(DALocalInfo*,Field**,Field**,void*);

/* Main routines */
extern PetscErrorCode SetParams(Parameter *param, GridInfo *grid);
extern PetscErrorCode ReportParams(Parameter *param, GridInfo *grid);
extern PetscErrorCode Initialize(DMMG*);
extern PetscErrorCode UpdateSolution(DMMG *dmmg, AppCtx *user, PetscInt *nits);
extern PetscErrorCode DoOutput(DMMG *dmmg, PetscInt its);

/* Physics subroutines */
extern PetscScalar Viscosity(PetscScalar T, PetscScalar eps, PassiveScalar z, Parameter *param);
extern PetscScalar CalcSecInv(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user);
extern PetscScalar XMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user);
extern PetscScalar ZMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user);
extern PetscScalar ContinuityResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user);
extern PetscScalar EnergyResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user);
extern PetscScalar XNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user);
extern PetscScalar ZNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user);
extern PetscScalar ShearStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user);

/* Utilities for interpolation, ICs and BCs */
extern PassiveScalar HorizVelocity(PetscInt i, PetscInt j, AppCtx *user);
extern PassiveScalar VertVelocity(PetscInt i, PetscInt j, AppCtx *user);
extern PassiveScalar Pressure(PetscInt i, PetscInt j, AppCtx *user);
extern PassiveScalar PlateModel(PetscInt j, PetscInt plate, AppCtx *user);
extern PassiveScalar SlabVel(char c, PetscInt i, PetscInt j, AppCtx *user);
extern PetscScalar   UInterp(Field **x, PetscInt i, PetscInt j);
extern PetscScalar   WInterp(Field **x, PetscInt i, PetscInt j);
extern PetscScalar   PInterp(Field **x, PetscInt i, PetscInt j);
extern PetscScalar   TInterp(Field **x, PetscInt i, PetscInt j);

/* Post-processing & misc */
extern PetscErrorCode ViscosityField(DMMG,Vec,Vec);
extern PetscErrorCode StressField(DMMG *dmmg);
extern PetscErrorCode SNESConverged_Interactive(SNES, PetscInt, PetscReal, PetscReal, PetscReal, SNESConvergedReason *, void *);
extern PetscErrorCode InteractiveHandler(int, void *);
extern PetscTruth OptionsHasName(const char pre[],const char name[]);

/*-----------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
/*-----------------------------------------------------------------------*/
{
  DMMG           *dmmg;               /* multilevel grid structure */
  AppCtx         *user;               /* user-defined work context */
  Parameter      param;
  GridInfo       grid;
  PetscInt       nits;
  PetscErrorCode ierr;
  MPI_Comm       comm;
  DA             da;

  PetscInitialize(&argc,&argv,(char *)0,help);
  PetscOptionsSetValue("-file","ex30_output");
  PetscOptionsSetValue("-snes_monitor",PETSC_NULL);
  PetscOptionsSetValue("-snes_max_it","20");
  PetscOptionsSetValue("-ksp_max_it","1500");
  PetscOptionsSetValue("-ksp_gmres_restart","300");
  PetscOptionsInsert(&argc,&argv,PETSC_NULL);

  comm = PETSC_COMM_WORLD;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set up the problem parameters.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */ 
  ierr = SetParams(&param,&grid);CHKERRQ(ierr);
  ierr = ReportParams(&param,&grid);CHKERRQ(ierr);

#if 0
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create user context, set problem data, create vector data structures.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */   
  ierr = PetscMalloc(sizeof(AppCtx),&user);CHKERRQ(ierr);
  user->param = &param;
  user->grid  = &grid;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create distributed array multigrid object (DMMG) to manage parallel grid and vectors
     for principal unknowns (x) and governing residuals (f)
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */ 
  ierr = DMMGCreate(comm,grid.mglevels,user,&dmmg);CHKERRQ(ierr); 
  ierr = DACreate2d(comm,grid.periodic,grid.stencil,grid.ni,grid.nj,PETSC_DECIDE,PETSC_DECIDE,grid.dof,grid.stencil_width,0,0,&da);CHKERRQ(ierr);
  ierr = DMMGSetDM(dmmg,(DM)da);CHKERRQ(ierr);
  ierr = DADestroy(da);CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),0,"x-velocity");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),1,"y-velocity");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),2,"pressure");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),3,"temperature");CHKERRQ(ierr);
  ierr = VecDuplicate(dmmg[0]->x, &(user->Xguess));CHKERRQ(ierr);
#else
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create distributed array multigrid object (DMMG) to manage parallel grid and vectors
     for principal unknowns (x) and governing residuals (f)
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */ 
  ierr = DMMGCreate(comm,grid.mglevels,&user,&dmmg);CHKERRQ(ierr); 
  ierr = DACreate2d(comm,grid.periodic,grid.stencil,grid.ni,grid.nj,PETSC_DECIDE,PETSC_DECIDE,grid.dof,grid.stencil_width,0,0,&da);CHKERRQ(ierr);
  ierr = DMMGSetDM(dmmg,(DM)da);CHKERRQ(ierr);
  ierr = DADestroy(da);CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),0,"x-velocity");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),1,"y-velocity");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),2,"pressure");CHKERRQ(ierr);
  ierr = DASetFieldName(DMMGGetDA(dmmg),3,"temperature");CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create user context, set problem data, create vector data structures.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */   
  ierr = PetscMalloc(sizeof(AppCtx),&user);CHKERRQ(ierr);
  user->param   = &param;
  user->grid    = &grid;
  dmmg[0]->user = user; 
  ierr = VecDuplicate(dmmg[0]->x, &(user->Xguess));CHKERRQ(ierr);
#endif

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set up the SNES solver with callback functions.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DMMGSetSNESLocal(dmmg,FormFunctionLocal,0,0,0);CHKERRQ(ierr);
  ierr = DMMGSetFromOptions(dmmg);CHKERRQ(ierr);
  ierr = DMMGSetInitialGuess(dmmg,FormInitialGuess);CHKERRQ(ierr);
  ierr = SNESSetConvergenceTest(DMMGGetSNES(dmmg),SNESConverged_Interactive,(void*)user,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscPushSignalHandler(InteractiveHandler,(void*)user);CHKERRQ(ierr);    
   
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize and solve the nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = Initialize(dmmg);CHKERRQ(ierr);
  ierr = UpdateSolution(dmmg,user,&nits);CHKERRQ(ierr); 
  
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Output variables.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DoOutput(dmmg,nits);CHKERRQ(ierr);
  
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space. 
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecDestroy(user->Xguess);CHKERRQ(ierr);
  ierr = PetscFree(user);CHKERRQ(ierr);
  ierr = DMMGDestroy(dmmg);CHKERRQ(ierr);
  
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}

/*=====================================================================
  PETSc INTERACTION FUNCTIONS (initialize & call SNESSolve)
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "UpdateSolution"
/*  manages solve: adaptive continuation method  */
PetscErrorCode UpdateSolution(DMMG *dmmg, AppCtx *user, PetscInt *nits)
{
  SNES                snes;
  KSP                 ksp;
  PC                  pc;
  SNESConvergedReason reason;
  Parameter           *param = user->param;
  PassiveScalar       cont_incr=0.3;
  PetscInt            its;
  PetscErrorCode      ierr;
  PetscTruth          q = PETSC_FALSE;

  PetscFunctionBegin;
  snes = DMMGGetSNES(dmmg);
  ierr = SNESGetKSP(snes,&ksp);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp, &pc);CHKERRQ(ierr);
  ierr = KSPSetComputeSingularValues(ksp, PETSC_TRUE);CHKERRQ(ierr);

  *nits=0;

  /* Isoviscous solve */
  if (param->ivisc == VISC_CONST && !param->stop_solve) {
    param->ivisc = VISC_CONST;
    ierr = DMMGSolve(dmmg);CHKERRQ(ierr); 
    ierr = VecCopy(DMMGGetx(dmmg),user->Xguess);CHKERRQ(ierr);
    ierr = SNESGetIterationNumber(snes, &its);CHKERRQ(ierr);
    *nits +=its;
    if (param->stop_solve) goto done;
  }

  /* Olivine diffusion creep */
  if (param->ivisc >= VISC_DIFN && !param->stop_solve) {
    if (!q) PetscPrintf(PETSC_COMM_WORLD,"Computing Variable Viscosity Solution\n");

    /* continuation method on viscosity cutoff */
    for (param->continuation=0.0; ; param->continuation+=cont_incr) { 
      if (!q) PetscPrintf(PETSC_COMM_WORLD," Continuation parameter = %G\n", param->continuation);

      /* solve the non-linear system */
      ierr = DMMGSolve(dmmg);CHKERRQ(ierr); 
      ierr = SNESGetConvergedReason(snes,&reason);CHKERRQ(ierr);
      ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
      *nits += its;
      if (!q) PetscPrintf(PETSC_COMM_WORLD," Newton iterations: %D, Cumulative: %D\n", its, *nits);
      if (param->stop_solve) goto done;

      if (reason<0) {
	/* NOT converged */
	cont_incr = -fabs(cont_incr)/2.0;
	if (fabs(cont_incr)<0.01) goto done;

      } else {
	/* converged */
	ierr = VecCopy(DMMGGetx(dmmg),user->Xguess);CHKERRQ(ierr);
	if (param->continuation >= 1.0) goto done;
	if (its<=3) {
	  cont_incr = 0.30001;
	} else if (its<=8) {
	  cont_incr = 0.15001;
	} else {
	  cont_incr = 0.10001;
	}
	if (param->continuation+cont_incr > 1.0) {
	  cont_incr = 1.0 - param->continuation;
	}
      } /* endif reason<0 */
    }
  }
  done:
  if (param->stop_solve && !q) PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: stopping solve.\n");
  if (reason<0 && !q) PetscPrintf(PETSC_COMM_WORLD,"FAILED TO CONVERGE: stopping solve.\n");
  PetscFunctionReturn(0);
}   

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormInitialGuess"
/*  used by SNESSolve to get an initial guess for the solution X */
PetscErrorCode FormInitialGuess(DMMG dmmg,Vec X)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user = (AppCtx*)dmmg->user;
  PetscErrorCode ierr;

  ierr = VecCopy(user->Xguess, X);CHKERRQ(ierr);
  return 0;
} 

/*=====================================================================
  PHYSICS FUNCTIONS (compute the discrete residual)
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocal"
/*  main call-back function that computes the processor-local piece 
    of the residual */
PetscErrorCode FormFunctionLocal(DALocalInfo *info,Field **x,Field **f,void *ptr)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user = (AppCtx*)ptr;
  Parameter      *param = user->param;
  GridInfo       *grid  = user->grid;
  PetscScalar    mag_w, mag_u;
  PetscInt       i,j,mx,mz,ilim,jlim;
  PetscInt       is,ie,js,je,ivisc,ibound;

  PetscFunctionBegin;

  /* Define global and local grid parameters */
  mx   = info->mx;     mz   = info->my;
  ilim = mx-1;         jlim = mz-1;
  is   = info->xs;     ie   = info->xs+info->xm; 
  js   = info->ys;     je   = info->ys+info->ym;

  /* Define geometric and numeric parameters */
  ivisc = param->ivisc;       ibound = param->ibound;

  for (j=js; j<je; j++) { 
    for (i=is; i<ie; i++) {

      /************* X-MOMENTUM/VELOCITY *************/
      if (i<j) {
	  f[j][i].u = x[j][i].u - SlabVel('U',i,j,user);

      } else if (j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
	/* in the lithospheric lid */
	f[j][i].u = x[j][i].u - 0.0;

      } else if (i==ilim) {
	/* on the right side boundary */
	if (ibound==BC_ANALYTIC) {
	  f[j][i].u = x[j][i].u - HorizVelocity(i,j,user);
	} else {
	  f[j][i].u = XNormalStress(x,i,j,CELL_CENTER,user) - EPS_ZERO; 
	}

      } else if (j==jlim) {
	/* on the bottom boundary */
	if (ibound==BC_ANALYTIC) {
	  f[j][i].u = x[j][i].u - HorizVelocity(i,j,user);
	} else if (ibound==BC_NOSTRESS) {
	  f[j][i].u = XMomentumResidual(x,i,j,user);
	} else {
	  /* experimental boundary condition */
	}

      } else { 
	/* in the mantle wedge */
	f[j][i].u = XMomentumResidual(x,i,j,user);
      }
      
      /************* Z-MOMENTUM/VELOCITY *************/
      if (i<=j) {
	f[j][i].w = x[j][i].w - SlabVel('W',i,j,user);

      } else if (j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
	/* in the lithospheric lid */
	f[j][i].w = x[j][i].w - 0.0;

      } else if (j==jlim) {
	/* on the bottom boundary */
	if (ibound==BC_ANALYTIC) {
	  f[j][i].w = x[j][i].w - VertVelocity(i,j,user);
	} else {
	  f[j][i].w = ZNormalStress(x,i,j,CELL_CENTER,user) - EPS_ZERO; 
	}

      } else if (i==ilim) {
	/* on the right side boundary */
	if (ibound==BC_ANALYTIC) {
	  f[j][i].w = x[j][i].w - VertVelocity(i,j,user);
	} else if (ibound==BC_NOSTRESS) {
	  f[j][i].w = ZMomentumResidual(x,i,j,user);
	} else {
	  /* experimental boundary condition */
	}

      } else {
	/* in the mantle wedge */
	f[j][i].w =  ZMomentumResidual(x,i,j,user);
      }

      /************* CONTINUITY/PRESSURE *************/
      if (i<j || j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
	/* in the lid or slab */
	f[j][i].p = x[j][i].p;
	
      } else if ((i==ilim || j==jlim) && ibound==BC_ANALYTIC) {
	/* on an analytic boundary */
	f[j][i].p = x[j][i].p - Pressure(i,j,user);

      } else {
	/* in the mantle wedge */
	f[j][i].p = ContinuityResidual(x,i,j,user);
      }

      /************* TEMPERATURE *************/
      if (j==0) {
	/* on the surface */
	f[j][i].T = x[j][i].T + x[j+1][i].T + PetscMax(x[j][i].T,0.0);

      } else if (i==0) {
	/* slab inflow boundary */
	f[j][i].T = x[j][i].T - PlateModel(j,PLATE_SLAB,user);

      } else if (i==ilim) {
	/* right side boundary */
	mag_u = 1.0 - pow( (1.0-PetscMax(PetscMin(x[j][i-1].u/param->cb,1.0),0.0)), 5.0 );
	f[j][i].T = x[j][i].T - mag_u*x[j-1][i-1].T - (1.0-mag_u)*PlateModel(j,PLATE_LID,user);

      } else if (j==jlim) {
	/* bottom boundary */
	mag_w = 1.0 - pow( (1.0-PetscMax(PetscMin(x[j-1][i].w/param->sb,1.0),0.0)), 5.0 );
	f[j][i].T = x[j][i].T - mag_w*x[j-1][i-1].T - (1.0-mag_w);

      } else {
	/* in the mantle wedge */
	f[j][i].T = EnergyResidual(x,i,j,user);      
      }
    }
  }
  PetscFunctionReturn(0);
} 

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "XMomentumResidual"
/*  computes the residual of the x-component of eqn (1) above */
PetscScalar XMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter      *param=user->param;
  GridInfo       *grid =user->grid;
  PetscScalar    dx = grid->dx, dz=grid->dz;
  PetscScalar    etaN,etaS,etaE,etaW,epsN=0.0,epsS=0.0,epsE=0.0,epsW=0.0;
  PetscScalar    TE=0.0,TN=0.0,TS=0.0,TW=0.0, dPdx, residual, z_scale;
  PetscScalar    dudxW,dudxE,dudzN,dudzS,dwdxN,dwdxS;
  PetscInt            jlim = grid->nj-1;

  z_scale = param->z_scale; 

  if ( param->ivisc==VISC_DIFN || param->ivisc>=VISC_DISL ) { /* viscosity is T-dependent */
    TS = param->potentialT * TInterp(x,i,j-1) * exp( (j-1.0)*dz*z_scale );
    if (j==jlim) TN = TS;
    else         TN = param->potentialT * TInterp(x,i,j)   * exp(  j     *dz*z_scale );
    TW = param->potentialT * x[j][i].T        * exp( (j-0.5)*dz*z_scale );
    TE = param->potentialT * x[j][i+1].T      * exp( (j-0.5)*dz*z_scale );
    if (param->ivisc>=VISC_DISL) { /* olivine dislocation creep */
      epsN = CalcSecInv(x,i,j,  CELL_CORNER,user);
      epsS = CalcSecInv(x,i,j-1,CELL_CORNER,user);
      epsE = CalcSecInv(x,i+1,j,CELL_CENTER,user);
      epsW = CalcSecInv(x,i,j,  CELL_CENTER,user);
    }
  }
  etaN = Viscosity(TN,epsN,dz*(j+0.5),param);   
  etaS = Viscosity(TS,epsS,dz*(j-0.5),param);
  etaW = Viscosity(TW,epsW,dz*j,param);   
  etaE = Viscosity(TE,epsE,dz*j,param);

  dPdx = ( x[j][i+1].p - x[j][i].p )/dx;
  if (j==jlim) dudzN = etaN * ( x[j][i].w   - x[j][i+1].w )/dx;
  else         dudzN = etaN * ( x[j+1][i].u - x[j][i].u   )/dz;
  dudzS = etaS * ( x[j][i].u    - x[j-1][i].u )/dz; 
  dudxE = etaE * ( x[j][i+1].u  - x[j][i].u   )/dx;
  dudxW = etaW * ( x[j][i].u    - x[j][i-1].u )/dx;

  residual  = -dPdx                         /* X-MOMENTUM EQUATION*/
	      +( dudxE - dudxW )/dx 
              +( dudzN - dudzS )/dz; 

  if ( param->ivisc!=VISC_CONST ) {
    dwdxN = etaN * ( x[j][i+1].w   - x[j][i].w   )/dx;
    dwdxS = etaS * ( x[j-1][i+1].w - x[j-1][i].w )/dx;

    residual += ( dudxE - dudxW )/dx + ( dwdxN - dwdxS )/dz;
  }

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ZMomentumResidual"
/*  computes the residual of the z-component of eqn (1) above */
PetscScalar ZMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter      *param=user->param;
  GridInfo       *grid =user->grid;
  PetscScalar    dx = grid->dx, dz=grid->dz;
  PetscScalar    etaN=0.0,etaS=0.0,etaE=0.0,etaW=0.0,epsN=0.0,epsS=0.0,epsE=0.0,epsW=0.0;
  PetscScalar    TE=0.0,TN=0.0,TS=0.0,TW=0.0, dPdz, residual,z_scale;
  PetscScalar    dudzE,dudzW,dwdxW,dwdxE,dwdzN,dwdzS;
  PetscInt            ilim = grid->ni-1;

  /* geometric and other parameters */
  z_scale = param->z_scale;
	
  /* viscosity */
  if ( param->ivisc==VISC_DIFN || param->ivisc>=VISC_DISL ) { /* viscosity is T-dependent */
    TN = param->potentialT * x[j+1][i].T      * exp( (j+0.5)*dz*z_scale );
    TS = param->potentialT * x[j][i].T        * exp( (j-0.5)*dz*z_scale );
    TW = param->potentialT * TInterp(x,i-1,j) * exp(  j     *dz*z_scale );
    if (i==ilim) TE = TW;
    else         TE = param->potentialT * TInterp(x,i,j)   * exp(  j*dz*z_scale );
    if (param->ivisc>=VISC_DISL) { /* olivine dislocation creep */
      epsN = CalcSecInv(x,i,j+1,CELL_CENTER,user);
      epsS = CalcSecInv(x,i,j,  CELL_CENTER,user);
      epsE = CalcSecInv(x,i,j,  CELL_CORNER,user);
      epsW = CalcSecInv(x,i-1,j,CELL_CORNER,user);
    }
  }
  etaN = Viscosity(TN,epsN,dz*(j+1),param);   
  etaS = Viscosity(TS,epsS,dz*j,param);
  etaW = Viscosity(TW,epsW,dz*(j+0.5),param);   
  etaE = Viscosity(TE,epsE,dz*(j+0.5),param);

  dPdz = ( x[j+1][i].p - x[j][i].p )/dz;  
  dwdzN = etaN * ( x[j+1][i].w - x[j][i].w )/dz;
  dwdzS = etaS * ( x[j][i].w - x[j-1][i].w )/dz;
  if (i==ilim) dwdxE = etaE * ( x[j][i].u   - x[j+1][i].u )/dz;
  else         dwdxE = etaE * ( x[j][i+1].w - x[j][i].w   )/dx;
  dwdxW = 2.0*etaW * ( x[j][i].w - x[j][i-1].w )/dx;
  
  /* Z-MOMENTUM */
  residual  = -dPdz                /* constant viscosity terms */                  
	      +( dwdzN - dwdzS )/dz 
              +( dwdxE - dwdxW )/dx; 

  if ( param->ivisc!=VISC_CONST ) {
    dudzE = etaE * ( x[j+1][i].u - x[j][i].u )/dz;
    dudzW = etaW * ( x[j+1][i-1].u - x[j][i-1].u )/dz;

    residual += ( dwdzN - dwdzS )/dz + ( dudzE - dudzW )/dx;
  }

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ContinuityResidual"
/*  computes the residual of eqn (2) above */
PetscScalar ContinuityResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  GridInfo       *grid =user->grid;
  PetscScalar    uE,uW,wN,wS,dudx,dwdz;

  uW = x[j][i-1].u; uE = x[j][i].u; dudx = ( uE - uW )/grid->dx;
  wS = x[j-1][i].w; wN = x[j][i].w; dwdz = ( wN - wS )/grid->dz;

  return dudx + dwdz;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "EnergyResidual"
/*  computes the residual of eqn (3) above */
PetscScalar EnergyResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter      *param=user->param;
  GridInfo       *grid =user->grid;
  PetscScalar    dx = grid->dx, dz=grid->dz;
  PetscInt            ilim=grid->ni-1, jlim=grid->nj-1, jlid=grid->jlid;
  PetscScalar    TE, TN, TS, TW, residual;
  PetscScalar    uE,uW,wN,wS;
  PetscScalar    fN,fS,fE,fW,dTdxW,dTdxE,dTdzN,dTdzS;

  dTdzN = ( x[j+1][i].T - x[j][i].T   )/dz;
  dTdzS = ( x[j][i].T   - x[j-1][i].T )/dz;
  dTdxE = ( x[j][i+1].T - x[j][i].T   )/dx;
  dTdxW = ( x[j][i].T   - x[j][i-1].T )/dx;
	
  residual = ( ( dTdzN - dTdzS )/dz + /* diffusion term */
	       ( dTdxE - dTdxW )/dx  )*dx*dz/param->peclet;

  if (j<=jlid && i>=j) {
    /* don't advect in the lid */
    return residual;

  } else if (i<j) {
    /* beneath the slab sfc */
    uW = uE = param->cb;
    wS = wN = param->sb;

  } else {
    /* advect in the slab and wedge */
    uW = x[j][i-1].u; uE = x[j][i].u;
    wS = x[j-1][i].w; wN = x[j][i].w; 
  }

  if ( param->adv_scheme==ADVECT_FV || i==ilim-1 || j==jlim-1 || i==1 || j==1 ) { 
    /* finite volume advection */
    TS  = ( x[j][i].T + x[j-1][i].T )/2.0;
    TN  = ( x[j][i].T + x[j+1][i].T )/2.0;  
    TE  = ( x[j][i].T + x[j][i+1].T )/2.0;  
    TW  = ( x[j][i].T + x[j][i-1].T )/2.0;
    fN = wN*TN*dx; fS = wS*TS*dx;           
    fE = uE*TE*dz; fW = uW*TW*dz;
    
  } else {        
    /* Fromm advection scheme */
    fE =     ( uE *(-x[j][i+2].T + 5.0*(x[j][i+1].T+x[j][i].T)-x[j][i-1].T)/8.0 
	       - fabs(uE)*(-x[j][i+2].T + 3.0*(x[j][i+1].T-x[j][i].T)+x[j][i-1].T)/8.0 )*dz;
    fW =     ( uW *(-x[j][i+1].T + 5.0*(x[j][i].T+x[j][i-1].T)-x[j][i-2].T)/8.0 
	       - fabs(uW)*(-x[j][i+1].T + 3.0*(x[j][i].T-x[j][i-1].T)+x[j][i-2].T)/8.0 )*dz;
    fN =     ( wN *(-x[j+2][i].T + 5.0*(x[j+1][i].T+x[j][i].T)-x[j-1][i].T)/8.0 
	       - fabs(wN)*(-x[j+2][i].T + 3.0*(x[j+1][i].T-x[j][i].T)+x[j-1][i].T)/8.0 )*dx;
    fS =     ( wS *(-x[j+1][i].T + 5.0*(x[j][i].T+x[j-1][i].T)-x[j-2][i].T)/8.0 
	       - fabs(wS)*(-x[j+1][i].T + 3.0*(x[j][i].T-x[j-1][i].T)+x[j-2][i].T)/8.0 )*dx;
  }
  
  residual -= ( fE - fW + fN - fS );  

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ShearStress"
/*  computes the shear stress---used on the boundaries */
PetscScalar ShearStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter    *param=user->param;
  GridInfo     *grid=user->grid;
  PetscInt          ilim=grid->ni-1, jlim=grid->nj-1;
  PetscScalar  uN, uS, wE, wW;

  if ( j<=grid->jlid || i<j || i==ilim || j==jlim ) return EPS_ZERO;

  if (ipos==CELL_CENTER) { /* on cell center */

    wE = WInterp(x,i,j-1); 
    if (i==j) { wW = param->sb; uN = param->cb;} 
    else      { wW = WInterp(x,i-1,j-1); uN = UInterp(x,i-1,j); }
    if (j==grid->jlid+1)  uS = 0.0;
    else                  uS = UInterp(x,i-1,j-1);

  } else { /* on cell corner */

    uN = x[j+1][i].u;         uS = x[j][i].u;
    wW = x[j][i].w;           wE = x[j][i+1].w;   

  }

  return (uN-uS)/grid->dz + (wE-wW)/grid->dx;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "XNormalStress"
/*  computes the normal stress---used on the boundaries */
PetscScalar XNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter      *param=user->param;
  GridInfo       *grid =user->grid;
  PetscScalar    dx = grid->dx, dz=grid->dz;
  PetscInt            ilim=grid->ni-1, jlim=grid->nj-1, ivisc;
  PetscScalar    epsC=0.0, etaC, TC, uE, uW, pC, z_scale;
  if (i<j || j<=grid->jlid) return EPS_ZERO;

  ivisc=param->ivisc;  z_scale = param->z_scale; 

  if (ipos==CELL_CENTER) { /* on cell center */

    TC = param->potentialT * x[j][i].T * exp( (j-0.5)*dz*z_scale );
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CENTER,user);
    etaC = Viscosity(TC,epsC,dz*j,param);

    uW = x[j][i-1].u;   uE = x[j][i].u; 
    pC = x[j][i].p;

  } else { /* on cell corner */
    if ( i==ilim || j==jlim ) return EPS_ZERO;

    TC = param->potentialT * TInterp(x,i,j) * exp( j*dz*z_scale );
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CORNER,user);
    etaC = Viscosity(TC,epsC,dz*(j+0.5),param);

    if (i==j) uW = param->sb;
    else      uW = UInterp(x,i-1,j);
    uE = UInterp(x,i,j); pC = PInterp(x,i,j);
  }
    
  return 2.0*etaC*(uE-uW)/dx - pC;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ZNormalStress"
/*  computes the normal stress---used on the boundaries */
PetscScalar ZNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter      *param=user->param;
  GridInfo       *grid =user->grid;
  PetscScalar    dz=grid->dz;
  PetscInt            ilim=grid->ni-1, jlim=grid->nj-1, ivisc;
  PetscScalar    epsC=0.0, etaC, TC;
  PetscScalar    pC, wN, wS, z_scale;
  if (i<j || j<=grid->jlid) return EPS_ZERO;

  ivisc=param->ivisc;  z_scale = param->z_scale; 

  if (ipos==CELL_CENTER) { /* on cell center */

    TC = param->potentialT * x[j][i].T * exp( (j-0.5)*dz*z_scale );
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CENTER,user);
    etaC = Viscosity(TC,epsC,dz*j,param);
    wN = x[j][i].w; wS = x[j-1][i].w; pC = x[j][i].p;

  } else { /* on cell corner */
    if ( (i==ilim) || (j==jlim) ) return EPS_ZERO;

    TC = param->potentialT * TInterp(x,i,j) * exp( j*dz*z_scale );
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CORNER,user);
    etaC = Viscosity(TC,epsC,dz*(j+0.5),param);
    if (i==j) wN = param->sb;
    else      wN = WInterp(x,i,j); 
    wS = WInterp(x,i,j-1); pC = PInterp(x,i,j);
  }

  return  2.0*etaC*(wN-wS)/dz - pC;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "CalcSecInv"
/*  computes the second invariant of the strain rate tensor */
PetscScalar CalcSecInv(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter     *param = user->param;
  GridInfo      *grid  = user->grid;
  PetscInt           ilim=grid->ni-1, jlim=grid->nj-1;
  PetscScalar   uN,uS,uE,uW,wN,wS,wE,wW;
  PetscScalar   eps11, eps12, eps22;

  if (i<j) return EPS_ZERO;
  if (i==ilim) i--; if (j==jlim) j--;

  if (ipos==CELL_CENTER) { /* on cell center */
    if (j<=grid->jlid) return EPS_ZERO;

    uE = x[j][i].u; uW = x[j][i-1].u;
    wN = x[j][i].w; wS = x[j-1][i].w; 
    wE = WInterp(x,i,j-1); 
    if (i==j) { uN = param->cb; wW = param->sb; }
    else      { uN = UInterp(x,i-1,j); wW = WInterp(x,i-1,j-1); }

    if (j==grid->jlid+1) uS = 0.0;
    else                 uS = UInterp(x,i-1,j-1);  

  } else {       /* on CELL_CORNER */
    if (j<grid->jlid) return EPS_ZERO;

    uN = x[j+1][i].u;  uS = x[j][i].u;  
    wE = x[j][i+1].w;  wW = x[j][i].w; 
    if (i==j) { wN = param->sb; uW = param->cb; }
    else      { wN = WInterp(x,i,j); uW = UInterp(x,i-1,j); }           

    if (j==grid->jlid) {
      uE = 0.0;  uW = 0.0;
      uS = -uN;
      wS = -wN;
    } else { 
      uE = UInterp(x,i,j);  
      wS = WInterp(x,i,j-1);
    }
  } 

  eps11 = (uE-uW)/grid->dx;  eps22 = (wN-wS)/grid->dz;
  eps12 = 0.5*((uN-uS)/grid->dz + (wE-wW)/grid->dx);

  return sqrt( 0.5*( eps11*eps11 + 2.0*eps12*eps12 + eps22*eps22 ) );
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "Viscosity"
/*  computes the shear viscosity */
PetscScalar Viscosity(PetscScalar T, PetscScalar eps, PassiveScalar z, 
		       Parameter *param)
/*---------------------------------------------------------------------*/
{
  PetscScalar  result=0.0;
  ViscParam    difn=param->diffusion, disl=param->dislocation;
  PetscInt          iVisc=param->ivisc;
  double       eps_scale=param->V/(param->L*1000.0);
  double       strain_power, v1, v2, P;
  double       rho_g = 32340.0, R=8.3144;

  P = rho_g*(z*param->L*1000.0); /* Pa */

  if        (iVisc==VISC_CONST) {  
    /* constant viscosity */
    return 1.0;

  } else if (iVisc==VISC_DIFN) {   
    /* diffusion creep rheology */
    result = difn.A*PetscExpScalar((difn.Estar + P*difn.Vstar)/R/(T+273.0))/param->eta0;

  } else if (iVisc==VISC_DISL) { 
    /* dislocation creep rheology */
    strain_power = pow( eps*eps_scale, (1.0-disl.n)/disl.n );
    result = disl.A*PetscExpScalar((disl.Estar + P*disl.Vstar)/disl.n/R/(T+273.0))*strain_power/param->eta0;

  } else if (iVisc==VISC_FULL) { 
    /* dislocation/diffusion creep rheology */
    strain_power = pow( eps*eps_scale, (1.0-disl.n)/disl.n );
    v1 = difn.A*PetscExpScalar((difn.Estar + P*difn.Vstar)/R/(T+273.0))/param->eta0;
    v2 = disl.A*PetscExpScalar((disl.Estar + P*disl.Vstar)/disl.n/R/(T+273.0))*strain_power/param->eta0;
    result = 1.0/(1.0/v1 + 1.0/v2);
  }

  /* max viscosity is param->eta0 */
  result = PetscMin( result, 1.0 );
  /* min viscosity is param->visc_cutoff */
  result = PetscMax( result, param->visc_cutoff );
  /* continuation method */
  result = pow(result,param->continuation);
  return result;
}

/*=====================================================================
  INITIALIZATION, POST-PROCESSING AND OUTPUT FUNCTIONS 
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "SetParams"
/* initializes the problem parameters and checks for 
   command line changes */
PetscErrorCode SetParams(Parameter *param, GridInfo *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode ierr, ierr_out=0;
  PetscReal      SEC_PER_YR = 3600.00*24.00*365.2500;
  PetscReal      PI = 3.14159265358979323846;
  PetscReal      alpha_g_on_cp_units_inverse_km=4.0e-5*9.8;  
  
  /* domain geometry */
  param->slab_dip      = 45.0;
  param->width         = 320.0;                                            /* km */
  param->depth         = 300.0;                                            /* km */
  param->lid_depth     = 35.0;                                             /* km */
  param->fault_depth   = 35.0;                                             /* km */
  ierr = PetscOptionsGetReal(PETSC_NULL,"-slab_dip",&(param->slab_dip),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-width",&(param->width),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-depth",&(param->depth),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-lid_depth",&(param->lid_depth),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-fault_depth",&(param->fault_depth),PETSC_NULL);CHKERRQ(ierr);
  param->slab_dip      = param->slab_dip*PI/180.0;                    /* radians */

  /* grid information */
  ierr = PetscOptionsGetInt(PETSC_NULL, "-jfault",&(grid->jfault),PETSC_NULL);CHKERRQ(ierr);
  grid->ni             = 82;
  ierr = PetscOptionsGetInt(PETSC_NULL, "-ni",&(grid->ni),PETSC_NULL);CHKERRQ(ierr);
  grid->dx             = param->width/((double)(grid->ni-2));              /* km */
  grid->dz             = grid->dx*tan(param->slab_dip);                    /* km */
  grid->nj             = (PetscInt)(param->depth/grid->dz + 3.0);        /* gridpoints*/
  param->depth         = grid->dz*(grid->nj-2);                            /* km */
  grid->inose          = 0;                                         /* gridpoints*/
  ierr = PetscOptionsGetInt(PETSC_NULL,"-inose",&(grid->inose),PETSC_NULL);CHKERRQ(ierr);
  grid->periodic       = DA_NONPERIODIC;
  grid->stencil        = DA_STENCIL_BOX;
  grid->dof            = 4;
  grid->stencil_width  = 2;
  grid->mglevels       = 1;

  /* boundary conditions */
  param->pv_analytic        = PETSC_FALSE;
  param->ibound             = BC_NOSTRESS;    
  ierr = PetscOptionsGetInt(PETSC_NULL,"-ibound",&(param->ibound),PETSC_NULL);CHKERRQ(ierr);

  /* physical constants */
  param->slab_velocity = 5.0;               /* cm/yr */
  param->slab_age      = 50.0;              /* Ma */
  param->lid_age       = 50.0;              /* Ma */
  param->kappa         = 0.7272e-6;         /* m^2/sec */
  param->potentialT    = 1300.0;            /* degrees C */
  ierr = PetscOptionsGetReal(PETSC_NULL,"-slab_velocity",&(param->slab_velocity),PETSC_NULL);CHKERRQ(ierr); 
  ierr = PetscOptionsGetReal(PETSC_NULL,"-slab_age",&(param->slab_age),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-lid_age",&(param->lid_age),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-kappa",&(param->kappa),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-potentialT",&(param->potentialT),PETSC_NULL);CHKERRQ(ierr);

  /* viscosity */
  param->ivisc         = 3;                 /* 0=isovisc, 1=difn creep, 2=disl creep, 3=full */
  param->eta0          = 1e24;              /* Pa-s */
  param->visc_cutoff   = 0.0;               /* factor of eta_0 */
  param->continuation  = 1.0;
  /* constants for diffusion creep */
  param->diffusion.A       = 1.8e7;           /* Pa-s */
  param->diffusion.n       = 1.0;             /* dim'less */
  param->diffusion.Estar   = 375e3;           /* J/mol */
  param->diffusion.Vstar   = 5e-6;            /* m^3/mol */
  /* constants for param->dislocationocation creep */
  param->dislocation.A     = 2.8969e4;        /* Pa-s */
  param->dislocation.n     = 3.5;             /* dim'less */
  param->dislocation.Estar = 530e3;           /* J/mol */
  param->dislocation.Vstar = 14e-6;           /* m^3/mol */
  ierr = PetscOptionsGetInt(PETSC_NULL, "-ivisc",&(param->ivisc),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-visc_cutoff",&(param->visc_cutoff),PETSC_NULL);CHKERRQ(ierr);
  param->output_ivisc  = param->ivisc;
  ierr = PetscOptionsGetInt(PETSC_NULL,"-output_ivisc",&(param->output_ivisc),PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-vstar",&(param->dislocation.Vstar),PETSC_NULL);CHKERRQ(ierr);

  /* output options */
  param->quiet            = PETSC_FALSE;
  param->param_test       = PETSC_FALSE;
  ierr = PetscOptionsHasName(PETSC_NULL,"-quiet",&(param->quiet));CHKERRQ(ierr);
  ierr = PetscOptionsHasName(PETSC_NULL,"-test",&(param->param_test));CHKERRQ(ierr);
  ierr = PetscOptionsGetString(PETSC_NULL,"-file",param->filename,PETSC_MAX_PATH_LEN,&(param->output_to_file));  

  /* advection */
  param->adv_scheme       = ADVECT_FROMM;       /* advection scheme: 0=finite vol, 1=Fromm */
  ierr = PetscOptionsGetInt(PETSC_NULL,"-adv_scheme",&(param->adv_scheme),PETSC_NULL);CHKERRQ(ierr);

  /* misc. flags */
  param->stop_solve          = PETSC_FALSE;
  param->interrupted         = PETSC_FALSE;
  param->kspmon              = PETSC_FALSE;
  param->toggle_kspmon       = PETSC_FALSE;

  /* derived parameters for slab angle */
  param->sb  = sin(param->slab_dip); 
  param->cb  = cos(param->slab_dip);
  param->c   =  param->slab_dip*param->sb/(param->slab_dip*param->slab_dip-param->sb*param->sb);
  param->d   = (param->slab_dip*param->cb-param->sb)/(param->slab_dip*param->slab_dip-param->sb*param->sb); 

  /* length, velocity and time scale for non-dimensionalization */
  param->L = PetscMin(param->width,param->depth);               /* km */
  param->V = param->slab_velocity/100.0/SEC_PER_YR;             /* m/sec */

  /* other unit conversions and derived parameters */
  param->scaled_width  = param->width/param->L;                 /* dim'less */
  param->scaled_depth  = param->depth/param->L;                 /* dim'less */
  param->lid_depth     = param->lid_depth/param->L;             /* dim'less */
  param->fault_depth   = param->fault_depth/param->L;           /* dim'less */
  grid->dx             = grid->dx/param->L;                     /* dim'less */
  grid->dz             = grid->dz/param->L;                     /* dim'less */
  grid->jlid           = (PetscInt)(param->lid_depth/grid->dz);      /* gridcells */
  grid->jfault         = (PetscInt)(param->fault_depth/grid->dz);    /* gridcells */
  param->lid_depth     = grid->jlid*grid->dz;                   /* dim'less */
  param->fault_depth   = grid->jfault*grid->dz;                 /* dim'less */
  grid->corner         = grid->jlid+1;                          /* gridcells */
  param->peclet        = param->V                               /* m/sec */
                       * param->L*1000.0                        /* m */
                       / param->kappa;                          /* m^2/sec */
  param->z_scale       = param->L * alpha_g_on_cp_units_inverse_km;
  param->skt           = sqrt(param->kappa*param->slab_age*SEC_PER_YR);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-peclet",&(param->peclet),PETSC_NULL);CHKERRQ(ierr);
  
  return ierr_out;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ReportParams"
/*  prints a report of the problem parameters to stdout */
PetscErrorCode ReportParams(Parameter *param, GridInfo *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode ierr, ierr_out=0;
  char           date[30];
  PetscReal      PI = 3.14159265358979323846;

  ierr = PetscGetDate(date,30);CHKERRQ(ierr);

  if ( !(param->quiet) ) {
    PetscPrintf(PETSC_COMM_WORLD,"---------------------BEGIN ex30 PARAM REPORT-------------------\n");
    PetscPrintf(PETSC_COMM_WORLD,"                   %s\n",&(date[0]));

    PetscPrintf(PETSC_COMM_WORLD,"Domain: \n");
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Width = %G km,         Depth = %G km\n",param->width,param->depth);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Slab dip = %G degrees,  Slab velocity = %G cm/yr\n",param->slab_dip*180.0/PI,param->slab_velocity);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Lid depth = %5.2f km,   Fault depth = %5.2f km\n",param->lid_depth*param->L,param->fault_depth*param->L);CHKERRQ(ierr);

    PetscPrintf(PETSC_COMM_WORLD,"\nGrid: \n");
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  [ni,nj] = %D, %D       [dx,dz] = %G, %G km\n",grid->ni,grid->nj,grid->dx*param->L,grid->dz*param->L);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  jlid = %3D              jfault = %3D \n",grid->jlid,grid->jfault);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Pe = %G\n",param->peclet);CHKERRQ(ierr);

    PetscPrintf(PETSC_COMM_WORLD,"\nRheology:");
    if (param->ivisc==VISC_CONST) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Isoviscous \n");CHKERRQ(ierr);
      if (param->pv_analytic)
	ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Pressure and Velocity prescribed! \n");CHKERRQ(ierr);
    } else if (param->ivisc==VISC_DIFN) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Diffusion Creep (T-Dependent Newtonian) \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %G--%G Pa-sec \n",param->eta0,param->visc_cutoff*param->eta0);CHKERRQ(ierr);
    } else if (param->ivisc==VISC_DISL ) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Dislocation Creep (T-Dependent Non-Newtonian) \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %G--%G Pa-sec \n",param->eta0,param->visc_cutoff*param->eta0);CHKERRQ(ierr);
    } else if (param->ivisc==VISC_FULL ) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Full Rheology \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %G--%G Pa-sec \n",param->eta0,param->visc_cutoff*param->eta0);CHKERRQ(ierr);
    } else {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Invalid! \n");CHKERRQ(ierr);
      ierr_out=1;
    }

    PetscPrintf(PETSC_COMM_WORLD,"Boundary condition:");
    if ( param->ibound==BC_ANALYTIC ) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Isoviscous Analytic Dirichlet \n");CHKERRQ(ierr);
    } else if ( param->ibound==BC_NOSTRESS ) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Stress-Free (normal & shear stress)\n");CHKERRQ(ierr);
    } else if ( param->ibound==BC_EXPERMNT ) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Experimental boundary condition \n");CHKERRQ(ierr);
    } else { 
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Invalid! \n");CHKERRQ(ierr);
      ierr_out=1;
    }    

    if (param->output_to_file)
#if defined(PETSC_HAVE_MATLAB_ENGINE)
      PetscPrintf(PETSC_COMM_WORLD,"Output Destination:       Mat file \"%s\"\n",param->filename);
#else
      PetscPrintf(PETSC_COMM_WORLD,"Output Destination:       PETSc binary file \"%s\"\n",param->filename);
#endif
    if ( param->output_ivisc != param->ivisc ) 
      PetscPrintf(PETSC_COMM_WORLD,"                          Output viscosity: -ivisc %D\n",param->output_ivisc);

    ierr = PetscPrintf(PETSC_COMM_WORLD,"---------------------END ex30 PARAM REPORT---------------------\n");CHKERRQ(ierr);
  }
  if ( param->param_test ) PetscEnd();
  return ierr_out;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "Initialize"
/*  generates an inital guess using the analytic solution for isoviscous
    corner flow */
PetscErrorCode Initialize(DMMG *dmmg)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user = (AppCtx*)dmmg[0]->user;
  Parameter      *param = user->param;
  GridInfo       *grid  = user->grid;
  DA             da;
  PetscInt       i,j,is,js,im,jm;
  PetscErrorCode ierr;
  Field          **x;

  /* Get the fine grid */
  da = (DA)(dmmg[0]->dm); 
  ierr = DAGetCorners(da,&is,&js,PETSC_NULL,&im,&jm,PETSC_NULL);CHKERRQ(ierr);
  ierr = DAVecGetArray(da,((AppCtx*)dmmg[0]->user)->Xguess,(void**)&x);CHKERRQ(ierr);

  /* Compute initial guess */
  for (j=js; j<js+jm; j++) {
    for (i=is; i<is+im; i++) {
      if (i<j) {
	x[j][i].u  = param->cb;
      } else if (j<=grid->jlid) {
	x[j][i].u  = 0.0;
      } else {
	x[j][i].u  = HorizVelocity(i,j,user);
      }
      if (i<=j) {
	x[j][i].w = param->sb;
      } else if (j<=grid->jlid) {
	x[j][i].w = 0.0;
      } else {
	x[j][i].w = VertVelocity(i,j,user);
      }
      if (i<j || j<=grid->jlid) {
	x[j][i].p = 0.0;
      } else {
	x[j][i].p = Pressure(i,j,user);
      }
      x[j][i].T   = PetscMin(grid->dz*(j-0.5),1.0);
    }
  }

  /* Restore x to Xguess */
  ierr = DAVecRestoreArray(da,((AppCtx*)dmmg[0]->user)->Xguess,(void**)&x);CHKERRQ(ierr);

  return 0;
} 

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "DoOutput"
/*  controls output to a file */
PetscErrorCode DoOutput(DMMG *dmmg, PetscInt its)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user = (AppCtx*)dmmg[0]->user;
  Parameter      *param = user->param;
  GridInfo       *grid  = user->grid;
  PetscErrorCode ierr;
  PetscMPIInt    rank;
  PetscInt       ivt=param->ivisc;
  PetscViewer    viewer;
  Vec            res, pars;
  MPI_Comm       comm;

  param->ivisc = param->output_ivisc;

  /* compute final residual and final viscosity/strain rate fields */
  ierr = SNESGetFunction(DMMGGetSNES(dmmg), &res, PETSC_NULL, PETSC_NULL);CHKERRQ(ierr);
  ierr = ViscosityField(DMMGGetFine(dmmg), DMMGGetx(dmmg), ((AppCtx *)dmmg[0]->user)->Xguess);CHKERRQ(ierr); 

  /* get the communicator and the rank of the processor */
  ierr = PetscObjectGetComm((PetscObject)DMMGGetSNES(dmmg), &comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);

  if (param->output_to_file) { /* send output to binary file */
    ierr = VecCreate(comm, &pars);CHKERRQ(ierr);
    if (rank == 0) { /* on processor 0 */
      ierr = VecSetSizes(pars, 20, PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = VecSetFromOptions(pars);CHKERRQ(ierr);
      ierr = VecSetValue(pars,0, (PetscScalar)(grid->ni),INSERT_VALUES);
      ierr = VecSetValue(pars,1, (PetscScalar)(grid->nj),INSERT_VALUES);
      ierr = VecSetValue(pars,2, (PetscScalar)(grid->dx),INSERT_VALUES);
      ierr = VecSetValue(pars,3, (PetscScalar)(grid->dz),INSERT_VALUES);
      ierr = VecSetValue(pars,4, (PetscScalar)(param->L),INSERT_VALUES);
      ierr = VecSetValue(pars,5, (PetscScalar)(param->V),INSERT_VALUES);
      /* skipped 6 intentionally */
      ierr = VecSetValue(pars,7, (PetscScalar)(param->slab_dip),INSERT_VALUES);
      ierr = VecSetValue(pars,8, (PetscScalar)(grid->jlid),INSERT_VALUES);
      ierr = VecSetValue(pars,9, (PetscScalar)(param->lid_depth),INSERT_VALUES);
      ierr = VecSetValue(pars,10,(PetscScalar)(grid->jfault),INSERT_VALUES);
      ierr = VecSetValue(pars,11,(PetscScalar)(param->fault_depth),INSERT_VALUES);
      ierr = VecSetValue(pars,12,(PetscScalar)(param->potentialT),INSERT_VALUES);
      ierr = VecSetValue(pars,13,(PetscScalar)(param->ivisc),INSERT_VALUES);
      ierr = VecSetValue(pars,14,(PetscScalar)(param->visc_cutoff),INSERT_VALUES);
      ierr = VecSetValue(pars,15,(PetscScalar)(param->ibound),INSERT_VALUES);
      ierr = VecSetValue(pars,16,(PetscScalar)(its),INSERT_VALUES);
    } else { /* on some other processor */
      ierr = VecSetSizes(pars, 0, PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = VecSetFromOptions(pars);CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(pars);CHKERRQ(ierr); ierr = VecAssemblyEnd(pars);CHKERRQ(ierr); 

    /* create viewer */
#if defined(PETSC_HAVE_MATLAB_ENGINE)
    ierr = PetscViewerMatlabOpen(PETSC_COMM_WORLD,param->filename,FILE_MODE_WRITE,&viewer);CHKERRQ(ierr);
#else
    ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,param->filename,FILE_MODE_WRITE,&viewer);CHKERRQ(ierr);
#endif

    /* send vectors to viewer */
    ierr = PetscObjectSetName((PetscObject)res,"res");
    ierr = VecView(res,viewer);CHKERRQ(ierr); 
    ierr = PetscObjectSetName((PetscObject)DMMGGetx(dmmg),"out");
    ierr = VecView(DMMGGetx(dmmg), viewer);CHKERRQ(ierr);  
    ierr = PetscObjectSetName((PetscObject)(user->Xguess),"aux");
    ierr = VecView(user->Xguess, viewer);CHKERRQ(ierr); 
    ierr = StressField(dmmg);CHKERRQ(ierr); /* compute stress fields */
    ierr = PetscObjectSetName((PetscObject)(user->Xguess),"str");
    ierr = VecView(user->Xguess, viewer);CHKERRQ(ierr); 
    ierr = PetscObjectSetName((PetscObject)pars,"par");
    ierr = VecView(pars, viewer);CHKERRQ(ierr);
    
    /* destroy viewer and vector */
    ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
    ierr = VecDestroy(pars);CHKERRQ(ierr);
  } 
  
  param->ivisc = ivt;
  return 0;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "ViscosityField"
/* Compute both the second invariant of the strain rate tensor and the viscosity, at both cell centers and cell corners */
PetscErrorCode ViscosityField(DMMG dmmg, Vec X, Vec V)
/* ------------------------------------------------------------------- */
{
  DA             da    = (DA) dmmg->dm;
  AppCtx         *user  = (AppCtx *) dmmg->user;
  Parameter      *param = user->param;
  GridInfo       *grid  = user->grid;
  Vec            localX;
  Field          **v, **x;
  PassiveReal    eps, dx, dz, T, epsC, TC;
  PetscInt       i,j,is,js,im,jm,ilim,jlim,ivt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ivt          = param->ivisc;
  param->ivisc = param->output_ivisc;

  ierr = DACreateLocalVector(da, &localX);CHKERRQ(ierr);
  ierr = DAGlobalToLocalBegin(da, X, INSERT_VALUES, localX);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da, X, INSERT_VALUES, localX);CHKERRQ(ierr);
  ierr = DAVecGetArray(da,localX,(void**)&x);CHKERRQ(ierr);
  ierr = DAVecGetArray(da,V,(void**)&v);CHKERRQ(ierr);

  /* Parameters */
  dx   = grid->dx;   dz   = grid->dz;
  ilim = grid->ni-1; jlim = grid->nj-1;

  /* Compute real temperature, strain rate and viscosity */
  ierr = DAGetCorners(da,&is,&js,PETSC_NULL,&im,&jm,PETSC_NULL);CHKERRQ(ierr);
  for (j=js; j<js+jm; j++) {
    for (i=is; i<is+im; i++) {
      T  = param->potentialT * x[j][i].T * exp( (j-0.5)*dz*param->z_scale );
      if (i<ilim && j<jlim) {
	TC = param->potentialT * TInterp(x,i,j) * exp( j*dz*param->z_scale );
      } else {
	TC = T;
      }
      eps  = CalcSecInv(x,i,j,CELL_CENTER,user);
      epsC = CalcSecInv(x,i,j,CELL_CORNER,user);
      v[j][i].u = eps;
      v[j][i].w = epsC;
      v[j][i].p = Viscosity(T,eps,dz*(j-0.5),param);
      v[j][i].T = Viscosity(TC,epsC,dz*j,param);
    }
  }
  ierr = DAVecRestoreArray(da,V,(void**)&v);CHKERRQ(ierr);
  ierr = DAVecRestoreArray(da,localX,(void**)&x);CHKERRQ(ierr);
  param->ivisc = ivt;
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "StressField"
/* post-processing: compute stress everywhere */
PetscErrorCode StressField(DMMG *dmmg)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user = (AppCtx*)dmmg[0]->user;
  PetscInt       i,j,is,js,im,jm;
  PetscErrorCode ierr;
  DA             da;
  Vec            locVec;
  Field          **x, **y;

  /* Get the fine grid of Xguess and X */
  da = (DA)(dmmg[0]->dm); 
  ierr = DAGetCorners(da,&is,&js,PETSC_NULL,&im,&jm,PETSC_NULL);CHKERRQ(ierr);
  ierr = DAVecGetArray(da,((AppCtx*)dmmg[0]->user)->Xguess,(void**)&x);CHKERRQ(ierr);

  ierr = DACreateLocalVector(da, &locVec);CHKERRQ(ierr);
  ierr = DAGlobalToLocalBegin(da, DMMGGetx(dmmg), INSERT_VALUES, locVec);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da, DMMGGetx(dmmg), INSERT_VALUES, locVec);CHKERRQ(ierr);
  ierr = DAVecGetArray(da,locVec,(void**)&y);CHKERRQ(ierr);

  /* Compute stress on the corner points */
  for (j=js; j<js+jm; j++) {
     for (i=is; i<is+im; i++) {
      
	x[j][i].u = ShearStress(y,i,j,CELL_CENTER,user);
	x[j][i].w = ShearStress(y,i,j,CELL_CORNER,user);
	x[j][i].p = XNormalStress(y,i,j,CELL_CENTER,user);
	x[j][i].T = ZNormalStress(y,i,j,CELL_CENTER,user);
    }
  }

  /* Restore the fine grid of Xguess and X */
  ierr = DAVecRestoreArray(da,((AppCtx*)dmmg[0]->user)->Xguess,(void**)&x);CHKERRQ(ierr);
  ierr = DAVecRestoreArray(da,locVec,(void**)&y);CHKERRQ(ierr);

  return 0;
}

/*=====================================================================
  UTILITY FUNCTIONS 
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "SlabVel"
/* returns the velocity of the subducting slab and handles fault nodes 
   for BC */
PassiveScalar SlabVel(char c, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter     *param = user->param;
  GridInfo      *grid  = user->grid;

  if (c=='U' || c=='u') {
    if (i<j-1) {
      return param->cb;
    } else if (j<=grid->jfault) {
      return 0.0;
    } else 
      return param->cb;

  } else {           
    if (i<j) {
      return param->sb;
    } else if (j<=grid->jfault) {
      return 0.0;
    } else 
      return param->sb;
  }
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PlateModel"
/*  solution to diffusive half-space cooling model for BC */
PassiveScalar PlateModel(PetscInt j, PetscInt plate, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter     *param = user->param;
  PassiveScalar z;
  if (plate==PLATE_LID) 
    z = (j-0.5)*user->grid->dz;
  else /* PLATE_SLAB */
    z = (j-0.5)*user->grid->dz*param->cb; 
#if defined (PETSC_HAVE_ERF)
  return erf(z*param->L/2.0/param->skt);
#else
  SETERRQ(1,"erf() not available on this machine");
#endif
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "UInterp"
PetscScalar UInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].u+x[j+1][i].u+x[j][i+1].u+x[j+1][i+1].u);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "WInterp"
PetscScalar WInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].w+x[j+1][i].w+x[j][i+1].w+x[j+1][i+1].w);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PInterp"
PetscScalar PInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].p+x[j+1][i].p+x[j][i+1].p+x[j+1][i+1].p);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TInterp"
PetscScalar TInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].T+x[j+1][i].T+x[j][i+1].T+x[j+1][i+1].T);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "HorizVelocity"
/*  isoviscous analytic solution for IC */
PassiveScalar HorizVelocity(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar x, z, r, st, ct, th, c=param->c, d=param->d;
  
  x = (i - grid->jlid)*grid->dx;  z = (j - grid->jlid - 0.5)*grid->dz;
  r = sqrt(x*x+z*z); st = z/r;  ct = x/r;  th = atan(z/x); 
  return ct*(c*th*st+d*(st+th*ct)) + st*(c*(st-th*ct)+d*th*st);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "VertVelocity"
/*  isoviscous analytic solution for IC */
PetscScalar VertVelocity(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar x, z, r, st, ct, th, c=param->c, d=param->d;
  
  x = (i - grid->jlid - 0.5)*grid->dx;  z = (j - grid->jlid)*grid->dz;
  r = sqrt(x*x+z*z); st = z/r;  ct = x/r;  th = atan(z/x); 
  return st*(c*th*st+d*(st+th*ct)) - ct*(c*(st-th*ct)+d*th*st);  
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "Pressure"
/*  isoviscous analytic solution for IC */
PetscScalar Pressure(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar x, z, r, st, ct, c=param->c, d=param->d;

  x = (i - grid->jlid - 0.5)*grid->dx;  z = (j - grid->jlid - 0.5)*grid->dz;
  r = sqrt(x*x+z*z);  st = z/r;  ct = x/r;  
  return (-2.0*(c*ct-d*st)/r);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "OptionsHasName"
/*  utility function */
PetscTruth OptionsHasName(const char pre[],const char name[])
/* ------------------------------------------------------------------- */
{
  PetscTruth     retval; 
  PetscErrorCode ierr;
  ierr = PetscOptionsHasName(pre,name,&retval);
  return retval;
}

/*=====================================================================
  INTERACTIVE SIGNAL HANDLING 
  =====================================================================*/

/* ------------------------------------------------------------------- */
#undef __FUNCT__  
#define __FUNCT__ "SNESConverged_Interactive"
PetscErrorCode SNESConverged_Interactive(SNES snes, PetscInt it,PetscReal xnorm, PetscReal pnorm, PetscReal fnorm, SNESConvergedReason *reason, void *ctx)
/* ------------------------------------------------------------------- */
{
  AppCtx        *user = (AppCtx *) ctx;
  Parameter     *param = user->param;
  KSP            ksp;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (param->interrupted) {
    param->interrupted = PETSC_FALSE;
    PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: exiting SNES solve. \n");
    *reason = SNES_CONVERGED_FNORM_ABS;
    PetscFunctionReturn(0);
  } else if (param->toggle_kspmon) {
    param->toggle_kspmon = PETSC_FALSE;
    ierr = SNESGetKSP(snes, &ksp);CHKERRQ(ierr);
    if (param->kspmon) {
      ierr = KSPMonitorCancel(ksp);CHKERRQ(ierr);
      param->kspmon = PETSC_FALSE;
      PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: deactivating ksp singular value monitor. \n");
    } else {
      ierr = KSPMonitorSet(ksp,KSPMonitorSingularValue,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
      param->kspmon = PETSC_TRUE;
      PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: activating ksp singular value monitor. \n");
    }
  }
  PetscFunctionReturn(SNESDefaultConverged(snes,it,xnorm,pnorm,fnorm,reason,ctx));
}

/* ------------------------------------------------------------------- */
#include <signal.h>
#undef __FUNCT__  
#define __FUNCT__ "InteractiveHandler"
PetscErrorCode InteractiveHandler(int signum, void *ctx)
/* ------------------------------------------------------------------- */
{
  AppCtx    *user = (AppCtx *) ctx;
  Parameter *param = user->param;

  if (signum == SIGILL) {
    param->toggle_kspmon = PETSC_TRUE;
  } else if (signum == SIGCONT) {
    param->interrupted = PETSC_TRUE;
  } else if (signum == SIGURG) {
    param->stop_solve = PETSC_TRUE;
  } 
  return 0;
}


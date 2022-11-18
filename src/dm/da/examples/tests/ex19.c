
static char help[] = "Tests DA with variable multiple degrees of freedom per node.\n\n";

/*
   This code only compiles with gcc, since it is not ANSI C
*/

#include "petscda.h"

PetscErrorCode doit(DA da,Vec global)
{
  PetscErrorCode ierr;
  PetscInt       i,j,k,M,N,dof;

  ierr = DAGetInfo(da,0,&M,&N,0,0,0,0,&dof,0,0,0);CHKERRQ(ierr);
  {
    struct {PetscScalar inside[dof];} **mystruct;
    ierr = DAVecGetArray(da,global,(void*) &mystruct);
    for ( i=0; i<N; i++) {
      for ( j=0; j<M; j++) {
	for ( k=0; k<dof; k++) {
	  ierr = PetscPrintf(PETSC_COMM_WORLD,"%d %d %G\n",i,j,mystruct[i][j].inside[0]);CHKERRQ(ierr);
	  mystruct[i][j].inside[1] = 2.1;
	}
      }
    }
    ierr = DAVecRestoreArray(da,global,(void*) &mystruct);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscInt       dof = 2,M = 3,N = 3,m = PETSC_DECIDE,n = PETSC_DECIDE;
  PetscErrorCode ierr;
  DA             da;
  Vec            global,local;
 
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 

  ierr = PetscOptionsGetInt(0,"-dof",&dof,0);CHKERRQ(ierr);
  /* Create distributed array and get vectors */
  ierr = DACreate2d(PETSC_COMM_WORLD,DA_NONPERIODIC,DA_STENCIL_BOX,
                    M,N,m,n,dof,1,PETSC_NULL,PETSC_NULL,&da);CHKERRQ(ierr);
  ierr = DACreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = DACreateLocalVector(da,&local);CHKERRQ(ierr);

  ierr = doit(da,global);CHKERRQ(ierr);


  ierr = VecView(global,0);CHKERRQ(ierr);

  /* Free memory */
  ierr = VecDestroy(local);CHKERRQ(ierr);
  ierr = VecDestroy(global);CHKERRQ(ierr);
  ierr = DADestroy(da);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 

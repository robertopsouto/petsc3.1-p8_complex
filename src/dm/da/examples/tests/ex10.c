
static char help[] = "Tests various 1-dimensional DA routines.\n\n";

#include "petscda.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscInt       M = 13,dof=1,s=1,wrap=0,i,n,j;
  PetscErrorCode ierr;
  DA             da;
  PetscViewer    viewer;
  Vec            local,locala,global,coors;
  PetscScalar    *x,*alocal;
  PetscDraw      draw;
  char           fname[16];

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 

  /* Create viewers */
  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",PETSC_DECIDE,PETSC_DECIDE,600,200,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);

  /* Read options */
  ierr = PetscOptionsGetInt(PETSC_NULL,"-M",&M,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-dof",&dof,PETSC_NULL);CHKERRQ(ierr); 
  ierr = PetscOptionsGetInt(PETSC_NULL,"-s",&s,PETSC_NULL);CHKERRQ(ierr); 
  ierr = PetscOptionsGetInt(PETSC_NULL,"-periodic",&wrap,PETSC_NULL);CHKERRQ(ierr); 

  /* Create distributed array and get vectors */
  ierr = DACreate1d(PETSC_COMM_WORLD,(DAPeriodicType)wrap,M,dof,s,PETSC_NULL,&da);CHKERRQ(ierr);
  ierr = DASetUniformCoordinates(da,0.0,1.0,0.0,0.0,0.0,0.0);CHKERRQ(ierr);
  for (i=0; i<dof; i++) {
    sprintf(fname,"Field %d",(int)i);
    ierr = DASetFieldName(da,i,fname);
  }

  ierr = DAView(da,viewer);CHKERRQ(ierr);
  ierr = DACreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = DACreateLocalVector(da,&local);CHKERRQ(ierr);
  ierr = DACreateLocalVector(da,&locala);CHKERRQ(ierr);
  ierr = DAGetCoordinates(da,&coors);CHKERRQ(ierr);
  ierr = VecGetArray(coors,&x);CHKERRQ(ierr);

  /* Set values into global vectors */
  ierr = VecGetArray(global,&alocal);CHKERRQ(ierr);
  ierr = VecGetLocalSize(global,&n);CHKERRQ(ierr);
  n    = n/dof;
  for (j=0; j<dof; j++) {
    for (i=0; i<n; i++) {
      alocal[j+dof*i] = PetscSinScalar(2*PETSC_PI*(j+1)*x[i]); 
    }
  }
  ierr = VecRestoreArray(global,&alocal);CHKERRQ(ierr);
  ierr = VecRestoreArray(coors,&x);CHKERRQ(ierr);
  ierr = VecDestroy(coords);CHKERRQ(ierr);

  ierr = VecView(global,viewer);CHKERRQ(ierr); 

  /* Send ghost points to local vectors */
  ierr = DAGlobalToLocalBegin(da,global,INSERT_VALUES,locala);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da,global,INSERT_VALUES,locala);CHKERRQ(ierr);

  /* Free memory */
  ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
  ierr = VecDestroy(global);CHKERRQ(ierr);
  ierr = VecDestroy(local);CHKERRQ(ierr);
  ierr = VecDestroy(locala);CHKERRQ(ierr);
  ierr = DADestroy(da);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 










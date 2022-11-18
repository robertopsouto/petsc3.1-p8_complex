/*
      Tests the signal handler.
*/
#include "petscsys.h"

#undef __FUNCT__
#define __FUNCT__ "CreateError"
int CreateError(int n)
{
  PetscErrorCode ierr;
  PetscReal      *x = 0;
  if (!n) x[0] = 100.; 
  ierr = CreateError(n-1);CHKERRQ(ierr);
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  PetscInitialize(&argc,&argv,(char *)0,0);
  ierr = PetscFPrintf(PETSC_COMM_WORLD,stdout,"Demonstrates how PETSc can trap error interrupts\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(PETSC_COMM_WORLD,stdout,"The error below is contrived to test the code!\n");CHKERRQ(ierr);
  ierr = PetscSynchronizedFlush(PETSC_COMM_WORLD);CHKERRQ(ierr);
  ierr = CreateError(5);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 

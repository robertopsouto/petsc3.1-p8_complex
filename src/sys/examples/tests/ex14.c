
/* 
   Tests PetscOptionsGetScalar() for complex numbers
 */

#include "petscsys.h"


#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  int         ierr;
  PetscScalar a;

  PetscInitialize(&argc,&argv,(char *)0,0);
  ierr = PetscOptionsGetScalar(PETSC_NULL,"-a",&a,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_SELF,"Scalar a = %G + %Gi\n",PetscRealPart(a),PetscImaginaryPart(a));CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 

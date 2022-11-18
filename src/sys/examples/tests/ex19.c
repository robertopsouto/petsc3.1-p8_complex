
static char help[] = "Tests string options with spaces";

#include "petscsys.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  char           option2[20],option3[30];
  PetscTruth     flg;
  PetscInt       option1;

  PetscInitialize(&argc,&argv,"ex19options",help);
  ierr = PetscOptionsGetInt(0,"-option1",&option1,&flg);CHKERRQ(ierr);
  ierr = PetscOptionsGetString(0,"-option2",option2,20,&flg);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"%s\n",option2);CHKERRQ(ierr);
  ierr = PetscOptionsGetString(0,"-option3",option3,30,&flg);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"%s\n",option3);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}

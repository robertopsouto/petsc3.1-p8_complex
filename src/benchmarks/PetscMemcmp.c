
#include "petscsys.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscLogDouble x,y,z;
  PetscScalar    A[10000],B[10000];
  int            i,ierr;
  PetscTruth     flg;

  PetscInitialize(&argc,&argv,0,0);

  for (i=0; i<10000; i++) {
    A[i] = i%61897;
    B[i] = i%61897;
  }
  /* To take care of paging effects */
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  ierr = PetscGetTime(&x);CHKERRQ(ierr);

  ierr = PetscGetTime(&x);CHKERRQ(ierr);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*10000,&flg);
  ierr = PetscGetTime(&y);CHKERRQ(ierr);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  PetscMemcmp(A,B,sizeof(PetscScalar)*0,&flg);
  ierr = PetscGetTime(&z);CHKERRQ(ierr);

  fprintf(stdout,"%s : \n","PetscMemcmp");
  fprintf(stdout,"    %-15s : %e sec\n","Latency",(z-y)/10.0);
  fprintf(stdout,"    %-15s : %e sec\n","Per PetscScalar",(2*y-x-z)/100000);

  ierr = PetscFinalize();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

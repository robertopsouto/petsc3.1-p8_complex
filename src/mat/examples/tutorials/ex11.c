
static char help[] = "Tests MatMeshToDual()\n\n";

/*T
   Concepts: Mat^mesh partitioning
   Processors: n
T*/

/* 
  Include "petscmat.h" so that we can use matrices.
  automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h    - vectors
     petscmat.h    - matrices
     petscis.h     - index sets            petscviewer.h - viewers               
*/
#include "petscmat.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat                   mesh,dual;
  PetscErrorCode        ierr;
  PetscInt              Nvertices = 6; /* total number of vertices */
  PetscInt              ncells = 2;    /* number cells on this process */
  PetscInt              *ii,*jj;
  PetscMPIInt           size,rank;
  MatPartitioning       part;
  IS                    is;

  PetscInitialize(&argc,&args,(char *)0,help);
  ierr = MPI_Comm_size(MPI_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size != 2) SETERRQ(PETSC_ERR_SUP,"This example is for exactly two processes");
  ierr = MPI_Comm_rank(MPI_COMM_WORLD,&rank);CHKERRQ(ierr);

  ierr = PetscMalloc(3*sizeof(PetscInt),&ii);CHKERRQ(ierr);
  ierr = PetscMalloc(6*sizeof(PetscInt),&jj);CHKERRQ(ierr);
  ii[0] = 0; ii[1] = 3; ii[2] = 6;
  if (!rank) {
    jj[0] = 0; jj[1] = 1; jj[2] = 2; jj[3] = 1; jj[4] = 3; jj[5] = 2;
  } else {
    jj[0] = 1; jj[1] = 4; jj[2] = 5; jj[3] = 1; jj[4] = 5; jj[5] = 3;
  }
  ierr = MatCreateMPIAdj(MPI_COMM_WORLD,ncells,Nvertices,ii,jj,PETSC_NULL,&mesh);CHKERRQ(ierr);
  ierr = MatMeshToCellGraph(mesh,2,&dual);CHKERRQ(ierr);
  ierr = MatView(dual,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  ierr = MatPartitioningCreate(MPI_COMM_WORLD,&part);CHKERRQ(ierr);
  ierr = MatPartitioningSetAdjacency(part,dual);CHKERRQ(ierr);
  ierr = MatPartitioningSetFromOptions(part);CHKERRQ(ierr);
  ierr = MatPartitioningApply(part,&is);CHKERRQ(ierr);
  ierr = ISView(is,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = ISDestroy(is);CHKERRQ(ierr);
  ierr = MatPartitioningDestroy(part);CHKERRQ(ierr);

  ierr = MatDestroy(mesh);CHKERRQ(ierr);
  ierr = MatDestroy(dual);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}




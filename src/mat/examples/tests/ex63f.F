!
!
!   This program tests storage of PETSc Dense matrix. 
!   It Creates a Dense matrix, and stores it into a file,
!   and then reads it back in as a SeqDense and MPIDense
!   matrix, and prints out the contents.
!
      program main
      implicit none
#include "finclude/petscsys.h"
#include "finclude/petscmat.h"
#include "finclude/petscvec.h"
#include "finclude/petscviewer.h"
      PetscErrorCode ierr
      PetscInt row,col,ten
      PetscMPIInt rank
      PetscScalar  v
      Mat     A
      PetscViewer  view

      call PetscInitialize(PETSC_NULL_CHARACTER,ierr)
      
      call MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr)
!
!     Proc-0 Create a seq-dense matrix and write it to a file
!
      if (rank .eq. 0) then
         ten = 10
         call MatCreateSeqDense(PETSC_COMM_SELF,ten,ten,                  &
     &        PETSC_NULL_SCALAR,A,ierr)
         v = 1.0d0
         do row=0,9 
            do col=0,9 
               call MatSetValue(A,row,col,v,INSERT_VALUES,ierr)
               v = v + 1.0d0
            enddo
         enddo
         
         call MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY,ierr)
         call MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY,ierr)
         write (6,100)
 100     format('The Contents of the Original Matrix')
         call MatView(A,PETSC_VIEWER_STDOUT_SELF,ierr)
!         
!        Now Write this matrix to a binary file
!
         call PetscViewerBinaryOpen(PETSC_COMM_SELF,'dense.mat',           &
     &        FILE_MODE_WRITE,view,ierr)
         call PetscViewerSetFormat(view,PETSC_VIEWER_NATIVE,ierr)
         call MatView(A,view,ierr)
         call PetscViewerDestroy(view,ierr)
         call MatDestroy(A,ierr)
!
!        Read this matrix into a SeqDense matrix

         call PetscViewerBinaryOpen(PETSC_COMM_SELF,'dense.mat',            &
     &        FILE_MODE_READ,view,ierr)
         call MatLoad(view,MATSEQDENSE,A,ierr)
         write (6,200)
 200     format('The Contents of SeqDense Matrix read in from file')
         call MatView(A,PETSC_VIEWER_STDOUT_SELF,ierr)
         call MatDestroy(A,ierr)
         call PetscViewerDestroy(view,ierr)
      endif

!
!     Use a barrier, so that the procs do not try opening the file before
!     it is created.
!
      call MPI_Barrier(PETSC_COMM_WORLD,ierr)

      call PetscViewerBinaryOpen(PETSC_COMM_WORLD,'dense.mat',               &
     &     FILE_MODE_READ,view,ierr)
      call MatLoad(view,MATMPIDENSE,A,ierr)
      if (rank .eq. 0) then
         write (6,300)
 300     format('The Contents of MPIDense Matrix read in from file')
      endif
      call MatView(A,PETSC_VIEWER_STDOUT_WORLD,ierr)
      call MatDestroy(A,ierr)
      call PetscViewerDestroy(view,ierr)
      call PetscFinalize(ierr)
      end

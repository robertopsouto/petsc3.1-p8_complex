!
!  Description: Creates an index set based on a set of integers. Views that index set
!  and then destroys it.
!
!/*T
!    Concepts: index sets^manipulating a general index set;
!    Concepts: Fortran90^accessing indices of index set;
!T*/
!    
!  The following include statements are required for Fortran programs
!  that use PETSc index sets:
!     petscsys.h  - base PETSc routines
!     petscis.h     - index sets (IS objects)
!     petscis.h90   - to allow access to Fortran90 features of index sets
!
      program main
      implicit none

#include "finclude/petscsys.h"
#include "finclude/petscis.h"
#include "finclude/petscis.h90"

      PetscErrorCode ierr
      PetscInt indices(5),n
      PetscInt five
      PetscMPIInt rank
      PetscInt, pointer :: idx(:)
      IS      is

      five = 5
      call PetscInitialize(PETSC_NULL_CHARACTER,ierr)
      call MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr)

!  Create an index set with 5 entries. Each processor creates
!  its own index set with its own list of integers.
   
      indices(1) = rank + 1
      indices(2) = rank + 2 
      indices(3) = rank + 3 
      indices(4) = rank + 4 
      indices(5) = rank + 5 
      call ISCreateGeneral(PETSC_COMM_SELF,five,indices,is,ierr)

!  Print the index set to stdout

      call ISView(is,PETSC_VIEWER_STDOUT_SELF,ierr)

!  Get the number of indices in the set 

      call ISGetLocalSize(is,n,ierr)

!   Get the indices in the index set

      call ISGetIndicesF90(is,idx,ierr)

      if (associated(idx)) then 
         write (*,*) 'Association check passed'
      else
         write (*,*) 'Association check failed'
      endif

!   Now any code that needs access to the list of integers
!   has access to it here

      write(6,50) idx
 50   format(5I3)

      write(6,100) rank,idx(1),idx(5)
 100  format('[',i5,'] First index = ',i5,' fifth index = ',i5)
      
!   Once we no longer need access to the indices they should 
!   returned to the system 

      call ISRestoreIndicesF90(is,idx,ierr)
  
!   All PETSc objects should be destroyed once they are 
!   no longer needed

      call ISDestroy(is,ierr)
      call PetscFinalize(ierr)
      end

 




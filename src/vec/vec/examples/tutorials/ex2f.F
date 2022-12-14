!
!
!  Description: Builds a parallel vector with 1 component on the first
!               processor, 2 on the second, etc.  Then each processor adds
!               one to all elements except the last rank.
!
!/*T
!   Concepts: vectors^assembling
!   Processors: n
!T*/
! -----------------------------------------------------------------------

      program main
      implicit none

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!                    Include files
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!  The following include statements are required for Fortran programs
!  that use PETSc vectors:
!     petscsys.h       - base PETSc routines
!     petscvec.h    - vectors
!  Additional include statements may be needed if using additional
!  PETSc routines in a Fortran program, e.g.,
!     petscviewer.h - viewers
!     petscis.h     - index sets
!
#include "finclude/petscsys.h"
#include "finclude/petscvec.h"

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!                 Beginning of program
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      Vec     x
      PetscInt N,i,ione
      PetscErrorCode ierr
      PetscMPIInt rank
      PetscScalar  one

      call PetscInitialize(PETSC_NULL_CHARACTER,ierr)
      one   = 1.0
      call MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr)

!  Create a parallel vector.
!   - In this case, we specify the size of the local portion on 
!     each processor, and PETSc computes the global size.  Alternatively,
!     if we pass the global size and use PETSC_DECIDE for the 
!     local size PETSc will choose a reasonable partition trying 
!     to put nearly an equal number of elements on each processor.

      N = rank + 1
      call VecCreateMPI(PETSC_COMM_WORLD,N,PETSC_DECIDE,x,ierr)
      call VecGetSize(x,N,ierr)
      call VecSet(x,one,ierr)

!  Set the vector elements.
!   - Note that VecSetValues() uses 0-based row and column numbers
!     in Fortran as well as in C.
!   - Always specify global locations of vector entries.
!   - Each processor can contribute any vector entries,
!     regardless of which processor "owns" them; any nonlocal
!     contributions will be transferred to the appropriate processor
!     during the assembly process.
!   - In this example, the flag ADD_VALUES indicates that all
!     contributions will be added together.

      ione = 1
      do 100 i=0,N-rank-1
         call VecSetValues(x,ione,i,one,ADD_VALUES,ierr)
 100  continue

!  Assemble vector, using the 2-step process:
!    VecAssemblyBegin(), VecAssemblyEnd()
!  Computations can be done while messages are in transition
!  by placing code between these two statements.

      call VecAssemblyBegin(x,ierr)
      call VecAssemblyEnd(x,ierr)

!  View the vector; then destroy it.

      call VecView(x,PETSC_VIEWER_STDOUT_WORLD,ierr)
      call VecDestroy(x,ierr)

      call PetscFinalize(ierr)
      end
 

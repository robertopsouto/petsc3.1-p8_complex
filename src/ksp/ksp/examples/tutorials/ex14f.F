!
!
!  Solves a nonlinear system in parallel with a user-defined
!  Newton method that uses KSP to solve the linearized Newton sytems.  This solver
!  is a very simplistic inexact Newton method.  The intent of this code is to
!  demonstrate the repeated solution of linear sytems with the same nonzero pattern.
!
!  This is NOT the recommended approach for solving nonlinear problems with PETSc!
!  We urge users to employ the SNES component for solving nonlinear problems whenever
!  possible, as it offers many advantages over coding nonlinear solvers independently.
!
!  We solve the  Bratu (SFI - solid fuel ignition) problem in a 2D rectangular
!  domain, using distributed arrays (DAs) to partition the parallel grid.
!
!  The command line options include:
!  -par <parameter>, where <parameter> indicates the problem's nonlinearity
!     problem SFI:  <parameter> = Bratu parameter (0 <= par <= 6.81)
!  -mx <xg>, where <xg> = number of grid points in the x-direction
!  -my <yg>, where <yg> = number of grid points in the y-direction
!  -Nx <npx>, where <npx> = number of processors in the x-direction
!  -Ny <npy>, where <npy> = number of processors in the y-direction
!  -mf use matrix free for matrix vector product
!
!/*T
!   Concepts: KSP^writing a user-defined nonlinear solver
!   Concepts: DA^using distributed arrays
!   Processors: n
!T*/
!  ------------------------------------------------------------------------
!
!    Solid Fuel Ignition (SFI) problem.  This problem is modeled by
!    the partial differential equation
!  
!            -Laplacian u - lambda*exp(u) = 0,  0 < x,y < 1,
!  
!    with boundary conditions
!   
!             u = 0  for  x = 0, x = 1, y = 0, y = 1.
!  
!    A finite difference approximation with the usual 5-point stencil
!    is used to discretize the boundary value problem to obtain a nonlinear 
!    system of equations.
!
!    The SNES version of this problem is:  snes/examples/tutorials/ex5f.F
!
!  ------------------------------------------------------------------------- 

      program main
      implicit none

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!                    Include files
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!     petscsys.h       - base PETSc routines   petscvec.h - vectors
!     petscmat.h - matrices
!     petscis.h     - index sets            petscksp.h - Krylov subspace methods
!     petscviewer.h - viewers               petscpc.h  - preconditioners

#include "finclude/petscsys.h"
#include "finclude/petscis.h"
#include "finclude/petscvec.h"
#include "finclude/petscmat.h"
#include "finclude/petscpc.h"
#include "finclude/petscksp.h"
#include "finclude/petscda.h"

      MPI_Comm comm
      Vec      X,Y,F,localX,localF
      Mat      J,B
      DA       da
      KSP      ksp

      PetscInt  Nx,Ny,N,mx,my,ifive,ithree
      PetscTruth flg,nooutput,usemf
      common   /mycommon/ mx,my,B,localX,localF,da
!
!
!      This is the routine to use for matrix-free approach
!
      external mymult

!     --------------- Data to define nonlinear solver -------------- 
      double precision   rtol,ttol
      double precision   fnorm,ynorm,xnorm
      PetscInt            max_nonlin_its,one
      PetscInt            lin_its
      PetscInt           i,m
      PetscScalar        mone
      PetscErrorCode ierr

      mone           = -1.d0
      rtol           = 1.d-8
      max_nonlin_its = 10
      one            = 1
      ifive          = 5
      ithree         = 3

      call PetscInitialize(PETSC_NULL_CHARACTER,ierr)
      comm = PETSC_COMM_WORLD

!  Initialize problem parameters

!
      mx = 4
      my = 4
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-mx',mx,flg,ierr)
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-my',my,flg,ierr)
      N = mx*my

      nooutput = .false.
      call PetscOptionsHasName(PETSC_NULL_CHARACTER,'-no_output',       &
     &     nooutput,ierr)

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Create linear solver context
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call KSPCreate(comm,ksp,ierr)

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Create vector data structures
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

!  
!  Create distributed array (DA) to manage parallel grid and vectors
!  
      Nx = PETSC_DECIDE
      Ny = PETSC_DECIDE
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-Nx',Nx,flg,ierr)
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-Ny',Ny,flg,ierr)
      call DACreate2d(comm,DA_NONPERIODIC,DA_STENCIL_STAR,mx,           &
     &     my,Nx,Ny,one,one,PETSC_NULL_INTEGER,PETSC_NULL_INTEGER,       &
     &     da,ierr)

!  
!  Extract global and local vectors from DA then duplicate for remaining
!  vectors that are the same types
!  
       call DACreateGlobalVector(da,X,ierr)
       call DACreateLocalVector(da,localX,ierr)
       call VecDuplicate(X,F,ierr)
       call VecDuplicate(X,Y,ierr)
       call VecDuplicate(localX,localF,ierr)


!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Create matrix data structure for Jacobian
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!  
!     Note:  For the parallel case, vectors and matrices MUST be partitioned
!     accordingly.  When using distributed arrays (DAs) to create vectors,
!     the DAs determine the problem partitioning.  We must explicitly
!     specify the local matrix dimensions upon its creation for compatibility
!     with the vector distribution.
!
!     Note: Here we only approximately preallocate storage space for the
!     Jacobian.  See the users manual for a discussion of better techniques
!     for preallocating matrix memory.
!  
      call VecGetLocalSize(X,m,ierr)
      call MatCreateMPIAIJ(comm,m,m,N,N,ifive,PETSC_NULL_INTEGER,ithree,         &
     &     PETSC_NULL_INTEGER,B,ierr)

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     if usemf is on then matrix vector product is done via matrix free
!     approach. Note this is just an example, and not realistic because 
!     we still use the actual formed matrix, but in reality one would
!     provide their own subroutine that would directly do the matrix 
!     vector product and not call MatMult()
!     Note: we put B into a common block so it will be visible to the
!     mymult() routine
      usemf = .false.
      call PetscOptionsHasName(PETSC_NULL_CHARACTER,'-mf',usemf,ierr)
      if (usemf) then
         call MatCreateShell(comm,m,m,N,N,PETSC_NULL_INTEGER,J,ierr)
         call MatShellSetOperation(J,MATOP_MULT,mymult,ierr)
      else 
!        If not doing matrix free then matrix operator, J,  and matrix used 
!        to construct preconditioner, B, are the same
        J = B
      endif

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Customize linear solver set runtime options
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!  
!     Set runtime options (e.g., -ksp_monitor -ksp_rtol <rtol> -ksp_type <type>)
!  
       call KSPSetFromOptions(ksp,ierr)

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Evaluate initial guess
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

       call FormInitialGuess(X,ierr)
       call ComputeFunction(X,F,ierr)
       call VecNorm(F,NORM_2,fnorm,ierr)
       ttol = fnorm*rtol
       if (.not. nooutput) then
         print*, 'Initial function norm ',fnorm
       endif

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Solve nonlinear system with a user-defined method
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

!  This solver is a very simplistic inexact Newton method, with no
!  no damping strategies or bells and whistles. The intent of this code
!  is merely to demonstrate the repeated solution with KSP of linear
!  sytems with the same nonzero structure.
!
!  This is NOT the recommended approach for solving nonlinear problems
!  with PETSc!  We urge users to employ the SNES component for solving
!  nonlinear problems whenever possible with application codes, as it
!  offers many advantages over coding nonlinear solvers independently.

       do 10 i=0,max_nonlin_its

!  Compute the Jacobian matrix.  See the comments in this routine for
!  important information about setting the flag mat_flag.

         call ComputeJacobian(X,B,ierr)

!  Solve J Y = F, where J is the Jacobian matrix.
!    - First, set the KSP linear operators.  Here the matrix that
!      defines the linear system also serves as the preconditioning
!      matrix.
!    - Then solve the Newton system.

         call KSPSetOperators(ksp,J,B,SAME_NONZERO_PATTERN,ierr)
         call KSPSolve(ksp,F,Y,ierr)

!  Compute updated iterate

         call VecNorm(Y,NORM_2,ynorm,ierr)
         call VecAYPX(Y,mone,X,ierr)
         call VecCopy(Y,X,ierr)
         call VecNorm(X,NORM_2,xnorm,ierr)
         call KSPGetIterationNumber(ksp,lin_its,ierr)
         if (.not. nooutput) then
           print*,'linear solve iterations = ',lin_its,' xnorm = ',     &
     &         xnorm,' ynorm = ',ynorm
         endif

!  Evaluate nonlinear function at new location

         call ComputeFunction(X,F,ierr)
         call VecNorm(F,NORM_2,fnorm,ierr)
         if (.not. nooutput) then
           print*, 'Iteration ',i+1,' function norm',fnorm
         endif

!  Test for convergence

       if (fnorm .le. ttol) then
         if (.not. nooutput) then
           print*,'Converged: function norm ',fnorm,' tolerance ',ttol
         endif
         goto 20
       endif
 10   continue
 20   continue

      write(6,100) i+1
 100  format('Number of Newton iterations =',I2)

!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Free work space.  All PETSc objects should be destroyed when they
!     are no longer needed.
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

       call MatDestroy(B,ierr)
       if (usemf) then
         call MatDestroy(J,ierr)
       endif
       call VecDestroy(localX,ierr)
       call VecDestroy(X,ierr)
       call VecDestroy(Y,ierr)
       call VecDestroy(localF,ierr)
       call VecDestroy(F,ierr)
       call KSPDestroy(ksp,ierr)
       call DADestroy(da,ierr)
       call PetscFinalize(ierr)
       end

! ------------------------------------------------------------------- 
! 
!   FormInitialGuess - Forms initial approximation.
!
!   Input Parameters:
!   X - vector
!
!   Output Parameter:
!   X - vector
! 
      subroutine FormInitialGuess(X,ierr)
      implicit none

!     petscsys.h       - base PETSc routines   petscvec.h - vectors
!     petscmat.h - matrices
!     petscis.h     - index sets            petscksp.h - Krylov subspace methods
!     petscviewer.h - viewers               petscpc.h  - preconditioners

#include "finclude/petscsys.h"
#include "finclude/petscis.h"
#include "finclude/petscvec.h"
#include "finclude/petscmat.h"
#include "finclude/petscpc.h"
#include "finclude/petscksp.h"
#include "finclude/petscda.h"
      PetscErrorCode    ierr
      PetscOffset      idx
      Vec       X,localX,localF
      PetscInt  i,j,row,mx
      PetscInt  my, xs,ys,xm
      PetscInt  ym,gxm,gym,gxs,gys
      double precision one,lambda,temp1,temp,hx,hy
      PetscScalar      xx(1)
      DA               da
      Mat              B
      common   /mycommon/ mx,my,B,localX,localF,da
 
      one    = 1.d0
      lambda = 6.d0
      hx     = one/(mx-1)
      hy     = one/(my-1)
      temp1  = lambda/(lambda + one)

!  Get a pointer to vector data.
!    - VecGetArray() returns a pointer to the data array.  
!    - You MUST call VecRestoreArray() when you no longer need access to
!      the array.
       call VecGetArray(localX,xx,idx,ierr)

!  Get local grid boundaries (for 2-dimensional DA):
!    xs, ys   - starting grid indices (no ghost points)
!    xm, ym   - widths of local grid (no ghost points)
!    gxs, gys - starting grid indices (including ghost points)
!    gxm, gym - widths of local grid (including ghost points)

       call DAGetCorners(da,xs,ys,PETSC_NULL_INTEGER,xm,ym,             &
     &      PETSC_NULL_INTEGER,ierr)
       call DAGetGhostCorners(da,gxs,gys,PETSC_NULL_INTEGER,gxm,gym,    &
     &      PETSC_NULL_INTEGER,ierr)

!  Compute initial guess over the locally owned part of the grid

      do 30 j=ys,ys+ym-1
        temp = (min(j,my-j-1))*hy
        do 40 i=xs,xs+xm-1
          row = i - gxs + (j - gys)*gxm + 1
          if (i .eq. 0 .or. j .eq. 0 .or. i .eq. mx-1 .or.              &
     &        j .eq. my-1) then
            xx(idx+row) = 0.d0 
            continue
          endif
          xx(idx+row) = temp1*sqrt(min((min(i,mx-i-1))*hx,temp))
 40     continue
 30   continue

!     Restore vector

       call VecRestoreArray(localX,xx,idx,ierr)

!     Insert values into global vector

       call DALocalToGlobal(da,localX,INSERT_VALUES,X,ierr)
       return 
       end

! ------------------------------------------------------------------- 
! 
!   ComputeFunction - Evaluates nonlinear function, F(x).
!
!   Input Parameters:
!.  X - input vector
!
!   Output Parameter:
!.  F - function vector
! 
      subroutine  ComputeFunction(X,F,ierr)
      implicit none

!     petscsys.h       - base PETSc routines   petscvec.h - vectors
!     petscmat.h - matrices
!     petscis.h     - index sets            petscksp.h - Krylov subspace methods
!     petscviewer.h - viewers               petscpc.h  - preconditioners

#include "finclude/petscsys.h"
#include "finclude/petscis.h"
#include "finclude/petscvec.h"
#include "finclude/petscmat.h"
#include "finclude/petscpc.h"
#include "finclude/petscksp.h"
#include "finclude/petscda.h"

      Vec              X,F,localX,localF
      PetscInt         gys,gxm,gym
      PetscOffset      idx,idf
      PetscErrorCode ierr
      PetscInt i,j,row,mx,my,xs,ys,xm,ym,gxs 
      double precision two,one,lambda,hx
      double precision hy,hxdhy,hydhx,sc
      PetscScalar      u,uxx,uyy,xx(1),ff(1)
      DA               da
      Mat              B
      common   /mycommon/ mx,my,B,localX,localF,da

      two    = 2.d0
      one    = 1.d0
      lambda = 6.d0

      hx     = one/(mx-1)
      hy     = one/(my-1)
      sc     = hx*hy*lambda 
      hxdhy  = hx/hy  
      hydhx  = hy/hx

!  Scatter ghost points to local vector, using the 2-step process
!     DAGlobalToLocalBegin(), DAGlobalToLocalEnd().
!  By placing code between these two statements, computations can be
!  done while messages are in transition.
!  
      call DAGlobalToLocalBegin(da,X,INSERT_VALUES,localX,ierr)
      call DAGlobalToLocalEnd(da,X,INSERT_VALUES,localX,ierr)

!  Get pointers to vector data

      call VecGetArray(localX,xx,idx,ierr)
      call VecGetArray(localF,ff,idf,ierr)

!  Get local grid boundaries

      call DAGetCorners(da,xs,ys,PETSC_NULL_INTEGER,xm,ym,              &
     &     PETSC_NULL_INTEGER,ierr)
      call DAGetGhostCorners(da,gxs,gys,PETSC_NULL_INTEGER,gxm,gym,     &
     &     PETSC_NULL_INTEGER,ierr)

!  Compute function over the locally owned part of the grid

      do 50 j=ys,ys+ym-1

        row = (j - gys)*gxm + xs - gxs
        do 60 i=xs,xs+xm-1
          row = row + 1

          if (i .eq. 0 .or. j .eq. 0 .or. i .eq. mx-1 .or.              &
     &        j .eq. my-1) then
            ff(idf+row) = xx(idx+row)
            goto 60
          endif
          u   = xx(idx+row)
          uxx = (two*u - xx(idx+row-1) - xx(idx+row+1))*hydhx
          uyy = (two*u - xx(idx+row-gxm) - xx(idx+row+gxm))*hxdhy
          ff(idf+row) = uxx + uyy - sc*exp(u)
 60     continue
 50   continue

!  Restore vectors

       call VecRestoreArray(localX,xx,idx,ierr)
       call VecRestoreArray(localF,ff,idf,ierr)

!  Insert values into global vector

       call DALocalToGlobal(da,localF,INSERT_VALUES,F,ierr)
       return 
       end

! ------------------------------------------------------------------- 
!
!   ComputeJacobian - Evaluates Jacobian matrix.
!
!   Input Parameters:
!   x - input vector
!
!   Output Parameters:
!   jac - Jacobian matrix
!   flag - flag indicating matrix structure
!
!   Notes:
!   Due to grid point reordering with DAs, we must always work
!   with the local grid points, and then transform them to the new
!   global numbering with the 'ltog' mapping (via DAGetGlobalIndices()).
!   We cannot work directly with the global numbers for the original
!   uniprocessor grid!
!
      subroutine ComputeJacobian(X,jac,ierr)
      implicit none

!     petscsys.h  - base PETSc routines   petscvec.h - vectors
!     petscmat.h - matrices
!     petscis.h     - index sets            petscksp.h - Krylov subspace methods
!     petscviewer.h - viewers               petscpc.h  - preconditioners

#include "finclude/petscsys.h"
#include "finclude/petscis.h"
#include "finclude/petscvec.h"
#include "finclude/petscmat.h"
#include "finclude/petscpc.h"
#include "finclude/petscksp.h"
#include "finclude/petscda.h"

      Vec         X
      Mat         jac
      Vec         localX,localF
      DA          da
      PetscInt     ltog(1)
      PetscOffset idltog,idx
      PetscErrorCode ierr
      PetscInt nloc,xs,ys,xm,ym
      PetscInt gxs,gys,gxm,gym
      PetscInt grow(1),i,j
      PetscInt row,mx,my,ione
      PetscInt col(5),ifive
      PetscScalar two,one,lambda
      PetscScalar v(5),hx,hy,hxdhy
      PetscScalar hydhx,sc,xx(1)
      Mat         B
      common   /mycommon/ mx,my,B,localX,localF,da

      ione   = 1
      ifive  = 5
      one    = 1.d0
      two    = 2.d0
      hx     = one/(mx-1)
      hy     = one/(my-1)
      sc     = hx*hy  
      hxdhy  = hx/hy        
      hydhx  = hy/hx
      lambda = 6.d0

!  Scatter ghost points to local vector, using the 2-step process
!     DAGlobalToLocalBegin(), DAGlobalToLocalEnd().
!  By placing code between these two statements, computations can be
!  done while messages are in transition.

      call DAGlobalToLocalBegin(da,X,INSERT_VALUES,localX,ierr)
      call DAGlobalToLocalEnd(da,X,INSERT_VALUES,localX,ierr)

!  Get pointer to vector data

      call VecGetArray(localX,xx,idx,ierr)

!  Get local grid boundaries

      call DAGetCorners(da,xs,ys,PETSC_NULL_INTEGER,xm,ym,              &
     &     PETSC_NULL_INTEGER,ierr)
      call DAGetGhostCorners(da,gxs,gys,PETSC_NULL_INTEGER,gxm,gym,     &
     &                        PETSC_NULL_INTEGER,ierr)

!  Get the global node numbers for all local nodes, including ghost points

      call DAGetGlobalIndices(da,nloc,ltog,idltog,ierr)

!  Compute entries for the locally owned part of the Jacobian.
!   - Currently, all PETSc parallel matrix formats are partitioned by
!     contiguous chunks of rows across the processors. The 'grow'
!     parameter computed below specifies the global row number 
!     corresponding to each local grid point.
!   - Each processor needs to insert only elements that it owns
!     locally (but any non-local elements will be sent to the
!     appropriate processor during matrix assembly). 
!   - Always specify global row and columns of matrix entries.
!   - Here, we set all entries for a particular row at once.

      do 10 j=ys,ys+ym-1
        row = (j - gys)*gxm + xs - gxs 
        do 20 i=xs,xs+xm-1
          row = row + 1
          grow(1) = ltog(idltog+row)
          if (i .eq. 0 .or. j .eq. 0 .or. i .eq. (mx-1) .or.            &
     &        j .eq. (my-1)) then
             call MatSetValues(jac,ione,grow,ione,grow,one,             &
     &                         INSERT_VALUES,ierr)
             go to 20
          endif
          v(1)   = -hxdhy
          col(1) = ltog(idltog+row - gxm)
          v(2)   = -hydhx 
          col(2) = ltog(idltog+row - 1)
          v(3)   = two*(hydhx + hxdhy) - sc*lambda*exp(xx(idx+row))
          col(3) = grow(1)
          v(4)   = -hydhx 
          col(4) = ltog(idltog+row + 1)
          v(5)   = -hxdhy 
          col(5) = ltog(idltog+row + gxm)
          call MatSetValues(jac,ione,grow,ifive,col,v,INSERT_VALUES,       &
     &                      ierr)
 20     continue
 10   continue

!  Assemble matrix, using the 2-step process:
!    MatAssemblyBegin(), MatAssemblyEnd().
!  By placing code between these two statements, computations can be
!  done while messages are in transition.

      call MatAssemblyBegin(jac,MAT_FINAL_ASSEMBLY,ierr)
      call VecRestoreArray(localX,xx,idx,ierr)
      call MatAssemblyEnd(jac,MAT_FINAL_ASSEMBLY,ierr)
      return 
      end


! ------------------------------------------------------------------- 
! 
!   MyMult - user provided matrix multiply 
!
!   Input Parameters:
!.  X - input vector
!
!   Output Parameter:
!.  F - function vector
! 
      subroutine  MyMult(J,X,F,ierr)
      implicit none
      Mat     J,B
      Vec     X,F
      PetscErrorCode ierr
      PetscInt mx,my
      DA      da
      Vec     localX,localF

      common   /mycommon/ mx,my,B,localX,localF,da
!
!       Here we use the actual formed matrix B; users would 
!     instead write their own matrix vector product routine
!
      call MatMult(B,X,F,ierr)
      return
      end






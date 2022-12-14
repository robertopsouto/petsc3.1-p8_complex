#define PETSC_DLL
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include "../src/sys/draw/drawimpl.h"  /*I "petscdraw.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "PetscDrawClear" 
/*@
   PetscDrawClear - Clears graphical output.

   Not collective (Use PetscDrawSynchronizedClear() for collective)

   Input Parameter:
.  draw - the drawing context

   Level: beginner

   Concepts: clear^window

.seealso: PetscDrawBOP(), PetscDrawEOP(), PetscDrawSynchronizedClear()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDrawClear(PetscDraw draw)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_COOKIE,1);
  if (draw->ops->clear) {
    ierr = (*draw->ops->clear)(draw);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscDrawBOP" 
/*@
   PetscDrawBOP - Begins a new page or frame on the selected graphical device.

   Collective on PetscDraw

   Input Parameter:
.  draw - the drawing context

   Level: advanced

.seealso: PetscDrawEOP(), PetscDrawClear()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDrawBOP(PetscDraw draw)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_COOKIE,1);
  if (draw->ops->beginpage) {
    ierr = (*draw->ops->beginpage)(draw);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
#undef __FUNCT__  
#define __FUNCT__ "PetscDrawEOP" 
/*@
   PetscDrawEOP - Ends a page or frame on the selected graphical device.

   Collective on PetscDraw

   Input Parameter:
.  draw - the drawing context

   Level: advanced

.seealso: PetscDrawBOP(), PetscDrawClear()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDrawEOP(PetscDraw draw)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_COOKIE,1);
  if (draw->ops->endpage) {
    ierr =  (*draw->ops->endpage)(draw);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


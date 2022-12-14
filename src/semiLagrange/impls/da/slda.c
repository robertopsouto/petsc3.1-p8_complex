#include "slda.h"       /*I  "characteristic.h"  I*/

#undef __FUNCT__  
#define __FUNCT__ "CharacteristicView_DA" 
PetscErrorCode CharacteristicView_DA(Characteristic c, PetscViewer viewer)
{
  Characteristic_DA *da = (Characteristic_DA *) c->data; 
  PetscTruth         iascii, isstring;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  /* Pull out field names from DA */
  ierr = PetscTypeCompare((PetscObject) viewer, PETSC_VIEWER_ASCII, &iascii);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject) viewer, PETSC_VIEWER_STRING, &isstring);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"  DA: dummy=%D\n", da->dummy);CHKERRQ(ierr);
  } else if (isstring) {
    ierr = PetscViewerStringSPrintf(viewer,"dummy %D", da->dummy);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_SUP, "Viewer type %s not supported for Characteristic DA", ((PetscObject) viewer)->type_name);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "CharacteristicDestroy_DA" 
PetscErrorCode CharacteristicDestroy_DA(Characteristic c)
{
  Characteristic_DA *da = (Characteristic_DA*) c->data;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  ierr = PetscFree(da);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CharacteristicSetUp_DA"
PetscErrorCode CharacteristicSetUp_DA(Characteristic c)
{
  PetscMPIInt    blockLen[2];
  MPI_Aint       indices[2];
  MPI_Datatype   oldtypes[2]; 
  PetscInt       dim, numValues;
  PetscErrorCode ierr;

  ierr = DAGetInfo(c->velocityDA, &dim, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);CHKERRQ(ierr);
  if (c->structured) {
    c->numIds = dim;
  } else {
    c->numIds = 3;
  }
  if (c->numFieldComp > MAX_COMPONENTS) {
    SETERRQ2(PETSC_ERR_ARG_OUTOFRANGE, "The maximum number of fields allowed is %d, you have %d. You can recompile after increasing MAX_COMPONENTS.", MAX_COMPONENTS, c->numFieldComp);
  }
  numValues = 4 + MAX_COMPONENTS;

  /* Create new MPI datatype for communication of characteristic point structs */
  blockLen[0] = 1+c->numIds; indices[0] = 0;                              oldtypes[0] = MPIU_INT;
  blockLen[1] = numValues;   indices[1] = (1+c->numIds)*sizeof(PetscInt); oldtypes[1] = MPIU_SCALAR;
  ierr = MPI_Type_struct(2, blockLen, indices, oldtypes, &c->itemType);CHKERRQ(ierr);
  ierr = MPI_Type_commit(&c->itemType);CHKERRQ(ierr);

  /* Initialize the local queue for char foot values */
  ierr = VecGetLocalSize(c->velocity, &c->queueMax);CHKERRQ(ierr);
  ierr = PetscMalloc(c->queueMax * sizeof(CharacteristicPointDA2D), &c->queue);CHKERRQ(ierr);
  c->queueSize = 0;

  /* Allocate communication structures */
  if (c->numNeighbors <= 0) {
    SETERRQ1(PETSC_ERR_ARG_WRONGSTATE, "Invalid number of neighbors %d. Call CharactersiticSetNeighbors() before setup.", c->numNeighbors);
  }
  ierr = PetscMalloc(c->numNeighbors * sizeof(PetscInt), &c->needCount);CHKERRQ(ierr);
  ierr = PetscMalloc(c->numNeighbors * sizeof(PetscInt), &c->localOffsets);CHKERRQ(ierr);
  ierr = PetscMalloc(c->numNeighbors * sizeof(PetscInt), &c->fillCount);CHKERRQ(ierr);
  ierr = PetscMalloc(c->numNeighbors * sizeof(PetscInt), &c->remoteOffsets);CHKERRQ(ierr);
  ierr = PetscMalloc((c->numNeighbors-1) * sizeof(MPI_Request), &c->request);CHKERRQ(ierr);
  ierr = PetscMalloc((c->numNeighbors-1) * sizeof(MPI_Status),  &c->status);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "CharacteristicCreate_DA"
PetscErrorCode CharacteristicCreate_DA(Characteristic c)
{
  Characteristic_DA *da;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  ierr = PetscNew(Characteristic_DA, &da);CHKERRQ(ierr);
  ierr = PetscMemzero(da, sizeof(Characteristic_DA));CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(c, sizeof(Characteristic_DA));CHKERRQ(ierr);
  c->data = (void *) da;

  c->ops->setup   = CharacteristicSetUp_DA;
  c->ops->destroy = CharacteristicDestroy_DA;
  c->ops->view    = CharacteristicView_DA;

  da->dummy = 0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__
#define __FUNCT__ "DAMapCoordsToPeriodicDomain"
/* -----------------------------------------------------------------------------
   Checks for periodicity of a DA and Maps points outside of a domain back onto the domain
   using appropriate periodicity. At the moment assumes only a 2-D DA.
   ----------------------------------------------------------------------------------------*/
PetscErrorCode DAMapCoordsToPeriodicDomain(DA da, PetscScalar *x, PetscScalar *y)
{
  DAPeriodicType periodic_type;
  PetscInt       dim, gx, gy;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DAGetInfo(da, &dim, &gx, &gy, 0, 0, 0, 0, 0, 0, &periodic_type, 0);

  if ( periodic_type == DA_NONPERIODIC ) {
    ierr = 0;
  } else {
    if (periodic_type==DA_XPERIODIC || periodic_type==DA_XYPERIODIC) {
      while (*x >= ( PetscScalar ) gx ) { *x -= ( PetscScalar ) gx; }
      while (*x < 0.0 )                 { *x += ( PetscScalar ) gx; }
    }
    if (periodic_type==DA_YPERIODIC || periodic_type==DA_XYPERIODIC) {
      while (*y >= ( PetscScalar ) gy ) { *y -= ( PetscScalar ) gy; }
      while (*y < 0.0 )                 { *y += ( PetscScalar ) gy; }
    }
  }
    
  PetscFunctionReturn(ierr);
}

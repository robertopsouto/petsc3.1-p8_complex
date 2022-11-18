#define PETSCDM_DLL

/* 
   Plots vectors obtained with DACreate1d()
*/

#include "petscda.h"      /*I  "petscda.h"   I*/

#undef __FUNCT__  
#define __FUNCT__ "DASetUniformCoordinates"
/*@
    DASetUniformCoordinates - Sets a DA coordinates to be a uniform grid

  Collective on DA

  Input Parameters:
+  da - the distributed array object
.  xmin,xmax - extremes in the x direction
.  ymin,ymax - extremes in the y direction (use PETSC_NULL for 1 dimensional problems)
-  zmin,zmax - extremes in the z direction (use PETSC_NULL for 1 or 2 dimensional problems)

  Level: beginner

.seealso: DASetCoordinates(), DAGetCoordinates(), DACreate1d(), DACreate2d(), DACreate3d()

@*/
PetscErrorCode PETSCDM_DLLEXPORT DASetUniformCoordinates(DA da,PetscReal xmin,PetscReal xmax,PetscReal ymin,PetscReal ymax,PetscReal zmin,PetscReal zmax)
{
  MPI_Comm       comm;
  DA             cda;
  DAPeriodicType periodic;
  Vec            xcoor;
  PetscScalar   *coors;
  PetscReal      hx,hy,hz_;
  PetscInt       i,j,k,M,N,P,istart,isize,jstart,jsize,kstart,ksize,dim,cnt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (xmax <= xmin) SETERRQ2(PETSC_ERR_ARG_INCOMP,"xmax must be larger than xmin %G %G",xmin,xmax);

  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
  ierr = DAGetInfo(da,&dim,&M,&N,&P,0,0,0,0,0,&periodic,0);CHKERRQ(ierr);
  ierr = DAGetCorners(da,&istart,&jstart,&kstart,&isize,&jsize,&ksize);CHKERRQ(ierr);
  ierr = DAGetCoordinateDA(da, &cda);CHKERRQ(ierr);
  ierr = DACreateGlobalVector(cda, &xcoor);CHKERRQ(ierr);
  if (dim == 1) {
    if (periodic == DA_NONPERIODIC) hx = (xmax-xmin)/(M-1);
    else                            hx = (xmax-xmin)/M;
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    for (i=0; i<isize; i++) {
      coors[i] = xmin + hx*(i+istart);
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else if (dim == 2) {
    if (ymax <= ymin) SETERRQ2(PETSC_ERR_ARG_INCOMP,"ymax must be larger than ymin %G %G",ymin,ymax);
    if (DAXPeriodic(periodic)) hx = (xmax-xmin)/(M);
    else                       hx = (xmax-xmin)/(M-1);
    if (DAYPeriodic(periodic)) hy = (ymax-ymin)/(N);
    else                       hy = (ymax-ymin)/(N-1);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    cnt  = 0;
    for (j=0; j<jsize; j++) {
      for (i=0; i<isize; i++) {
        coors[cnt++] = xmin + hx*(i+istart);
        coors[cnt++] = ymin + hy*(j+jstart);
      }
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else if (dim == 3) {
    if (ymax <= ymin) SETERRQ2(PETSC_ERR_ARG_INCOMP,"ymax must be larger than ymin %G %G",ymin,ymax);
    if (zmax <= zmin) SETERRQ2(PETSC_ERR_ARG_INCOMP,"zmax must be larger than zmin %G %G",zmin,zmax);
    if (DAXPeriodic(periodic)) hx = (xmax-xmin)/(M);
    else                       hx = (xmax-xmin)/(M-1);
    if (DAYPeriodic(periodic)) hy = (ymax-ymin)/(N);
    else                       hy = (ymax-ymin)/(N-1);
    if (DAZPeriodic(periodic)) hz_ = (zmax-zmin)/(P);
    else                       hz_ = (zmax-zmin)/(P-1);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    cnt  = 0;
    for (k=0; k<ksize; k++) {
      for (j=0; j<jsize; j++) {
        for (i=0; i<isize; i++) {
          coors[cnt++] = xmin + hx*(i+istart);
          coors[cnt++] = ymin + hy*(j+jstart);
          coors[cnt++] = zmin + hz_*(k+kstart);
        }
      }
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_SUP,"Cannot create uniform coordinates for this dimension %D\n",dim);
  }
  ierr = DASetCoordinates(da,xcoor);CHKERRQ(ierr);
  ierr = PetscLogObjectParent(da,xcoor);CHKERRQ(ierr);
  ierr = VecDestroy(xcoor);CHKERRQ(ierr);
  ierr = DADestroy(cda);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "VecView_MPI_Draw_DA1d"
PetscErrorCode VecView_MPI_Draw_DA1d(Vec xin,PetscViewer v)
{
  DA             da;
  PetscErrorCode ierr;
  PetscMPIInt    rank,size,tag1,tag2;
  PetscInt       i,n,N,step,istart,isize,j;
  MPI_Status     status;
  PetscReal      coors[4],ymin,ymax,min,max,xmin,xmax,tmp,xgtmp;
  PetscScalar    *array,*xg;
  PetscDraw      draw;
  PetscTruth     isnull,showpoints = PETSC_FALSE;
  MPI_Comm       comm;
  PetscDrawAxis  axis;
  Vec            xcoor;
  DAPeriodicType periodic;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(v,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);

  ierr = PetscObjectQuery((PetscObject)xin,"DA",(PetscObject*)&da);CHKERRQ(ierr);
  if (!da) SETERRQ(PETSC_ERR_ARG_WRONG,"Vector not generated from a DA");

  ierr = PetscOptionsGetTruth(PETSC_NULL,"-draw_vec_mark_points",&showpoints,PETSC_NULL);CHKERRQ(ierr);

  ierr = DAGetInfo(da,0,&N,0,0,0,0,0,&step,0,&periodic,0);CHKERRQ(ierr);
  ierr = DAGetCorners(da,&istart,0,0,&isize,0,0);CHKERRQ(ierr);
  ierr = VecGetArray(xin,&array);CHKERRQ(ierr);
  ierr = VecGetLocalSize(xin,&n);CHKERRQ(ierr);
  n    = n/step;

  /* get coordinates of nodes */
  ierr = DAGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  if (!xcoor) {
    ierr = DASetUniformCoordinates(da,0.0,1.0,0.0,0.0,0.0,0.0);CHKERRQ(ierr);
    ierr = DAGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  }
  ierr = VecGetArray(xcoor,&xg);CHKERRQ(ierr);

  ierr = PetscObjectGetComm((PetscObject)xin,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr); 
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  /*
      Determine the min and max x coordinate in plot 
  */
  if (!rank) {
    xmin = PetscRealPart(xg[0]);
  } 
  if (rank == size-1) {
    xmax = PetscRealPart(xg[n-1]);
  }
  ierr = MPI_Bcast(&xmin,1,MPIU_REAL,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(&xmax,1,MPIU_REAL,size-1,comm);CHKERRQ(ierr);

  for (j=0; j<step; j++) {
    ierr = PetscViewerDrawGetDraw(v,j,&draw);CHKERRQ(ierr);
    ierr = PetscDrawCheckResizedWindow(draw);CHKERRQ(ierr);

    /*
        Determine the min and max y coordinate in plot 
    */
    min = 1.e20; max = -1.e20;
    for (i=0; i<n; i++) {
#if defined(PETSC_USE_COMPLEX)
      if (PetscRealPart(array[j+i*step]) < min) min = PetscRealPart(array[j+i*step]);
      if (PetscRealPart(array[j+i*step]) > max) max = PetscRealPart(array[j+i*step]);
#else
      if (array[j+i*step] < min) min = array[j+i*step];
      if (array[j+i*step] > max) max = array[j+i*step];
#endif
    }
    if (min + 1.e-10 > max) {
      min -= 1.e-5;
      max += 1.e-5;
    }
    ierr = MPI_Reduce(&min,&ymin,1,MPIU_REAL,MPI_MIN,0,comm);CHKERRQ(ierr);
    ierr = MPI_Reduce(&max,&ymax,1,MPIU_REAL,MPI_MAX,0,comm);CHKERRQ(ierr);

    ierr = PetscDrawSynchronizedClear(draw);CHKERRQ(ierr);
    ierr = PetscViewerDrawGetDrawAxis(v,j,&axis);CHKERRQ(ierr);
    ierr = PetscLogObjectParent(draw,axis);CHKERRQ(ierr);
    if (!rank) {
      char *title;

      ierr = PetscDrawAxisSetLimits(axis,xmin,xmax,ymin,ymax);CHKERRQ(ierr);
      ierr = PetscDrawAxisDraw(axis);CHKERRQ(ierr);
      ierr = PetscDrawGetCoordinates(draw,coors,coors+1,coors+2,coors+3);CHKERRQ(ierr);
      ierr = DAGetFieldName(da,j,&title);CHKERRQ(ierr);
      if (title) {ierr = PetscDrawSetTitle(draw,title);CHKERRQ(ierr);}
    }
    ierr = MPI_Bcast(coors,4,MPIU_REAL,0,comm);CHKERRQ(ierr);
    if (rank) {
      ierr = PetscDrawSetCoordinates(draw,coors[0],coors[1],coors[2],coors[3]);CHKERRQ(ierr);
    }

    /* draw local part of vector */
    PetscObjectGetNewTag((PetscObject)xin,&tag1);CHKERRQ(ierr);
    PetscObjectGetNewTag((PetscObject)xin,&tag2);CHKERRQ(ierr);
    if (rank < size-1) { /*send value to right */
      ierr = MPI_Send(&array[j+(n-1)*step],1,MPIU_REAL,rank+1,tag1,comm);CHKERRQ(ierr);
      ierr = MPI_Send(&xg[n-1],1,MPIU_REAL,rank+1,tag1,comm);CHKERRQ(ierr);
    }
    if (!rank && periodic && size > 1) { /* first processor sends first value to last */
      ierr = MPI_Send(&array[j],1,MPIU_REAL,size-1,tag2,comm);CHKERRQ(ierr);
    }

    for (i=1; i<n; i++) {
#if !defined(PETSC_USE_COMPLEX)
      ierr = PetscDrawLine(draw,xg[i-1],array[j+step*(i-1)],xg[i],array[j+step*i],
                      PETSC_DRAW_RED);CHKERRQ(ierr);
#else
      ierr = PetscDrawLine(draw,PetscRealPart(xg[i-1]),PetscRealPart(array[j+step*(i-1)]),
                      PetscRealPart(xg[i]),PetscRealPart(array[j+step*i]),PETSC_DRAW_RED);CHKERRQ(ierr);
#endif
      if (showpoints) {
        ierr = PetscDrawPoint(draw,PetscRealPart(xg[i-1]),PetscRealPart(array[j+step*(i-1)]),PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    if (rank) { /* receive value from left */
      ierr = MPI_Recv(&tmp,1,MPIU_REAL,rank-1,tag1,comm,&status);CHKERRQ(ierr);
      ierr = MPI_Recv(&xgtmp,1,MPIU_REAL,rank-1,tag1,comm,&status);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
      ierr = PetscDrawLine(draw,xgtmp,tmp,xg[0],array[j],PETSC_DRAW_RED);CHKERRQ(ierr);
#else
      ierr = PetscDrawLine(draw,xgtmp,tmp,PetscRealPart(xg[0]),PetscRealPart(array[j]),
                      PETSC_DRAW_RED);CHKERRQ(ierr);
#endif
      if (showpoints) {
        ierr = PetscDrawPoint(draw,xgtmp,tmp,PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    if (rank == size-1 && periodic && size > 1) {
      ierr = MPI_Recv(&tmp,1,MPIU_REAL,0,tag2,comm,&status);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
      ierr = PetscDrawLine(draw,xg[n-2],array[j+step*(n-1)],xg[n-1],tmp,PETSC_DRAW_RED);CHKERRQ(ierr);
#else
      ierr = PetscDrawLine(draw,PetscRealPart(xg[n-2]),PetscRealPart(array[j+step*(n-1)]),
                      PetscRealPart(xg[n-1]),tmp,PETSC_DRAW_RED);CHKERRQ(ierr);
#endif
      if (showpoints) {
        ierr = PetscDrawPoint(draw,PetscRealPart(xg[n-2]),PetscRealPart(array[j+step*(n-1)]),PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
    ierr = PetscDrawPause(draw);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(xcoor,&xg);CHKERRQ(ierr);
  ierr = VecRestoreArray(xin,&array);CHKERRQ(ierr);
  ierr = VecDestroy(xcoor);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


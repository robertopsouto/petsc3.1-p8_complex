#define PETSCVEC_DLL

/* Routines to be used by MatIncreaseOverlap() for BAIJ and SBAIJ matrices */
#include "petscis.h" 
#include "petscbt.h"
#include "petscctable.h"


#undef __FUNCT__  
#define __FUNCT__ "ISCompressIndicesGeneral"
/*@C
   ISCompressIndicesGeneral - convert the indices into block indices
   Input Parameters:
+  n - the length of the index set
.  bs - the size of block 
.  imax - the number of index sets
-  is_in - the non-blocked array of index sets 

   Output Parameter:
.  is_out - the blocked new index set

   Level: intermediate
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISCompressIndicesGeneral(PetscInt n,PetscInt bs,PetscInt imax,const IS is_in[],IS is_out[])
{
  PetscErrorCode     ierr;
  PetscInt           isz,len,i,j,ival,Nbs;
  const PetscInt     *idx;
#if defined (PETSC_USE_CTABLE)
  PetscTable         gid1_lid1;
  PetscInt           tt, gid1, *nidx;
  PetscTablePosition tpos;
#else
  PetscInt           *nidx;
  PetscBT            table;
#endif

  PetscFunctionBegin;
  Nbs =n/bs;
#if defined (PETSC_USE_CTABLE)
  ierr = PetscTableCreate(Nbs,&gid1_lid1);CHKERRQ(ierr);
#else
  ierr = PetscMalloc(Nbs*sizeof(PetscInt),&nidx);CHKERRQ(ierr); 
  ierr = PetscBTCreate(Nbs,table);CHKERRQ(ierr);
#endif
  for (i=0; i<imax; i++) {
    isz  = 0;
#if defined (PETSC_USE_CTABLE)
    ierr = PetscTableRemoveAll(gid1_lid1);CHKERRQ(ierr);
#else
    ierr = PetscBTMemzero(Nbs,table);CHKERRQ(ierr);
#endif
    ierr = ISGetIndices(is_in[i],&idx);CHKERRQ(ierr);
    ierr = ISGetLocalSize(is_in[i],&len);CHKERRQ(ierr);
    for (j=0; j<len ; j++) {
      ival = idx[j]/bs; /* convert the indices into block indices */
#if defined (PETSC_USE_CTABLE)
      ierr = PetscTableFind(gid1_lid1,ival+1,&tt);CHKERRQ(ierr);
      if (!tt) {
	ierr = PetscTableAdd(gid1_lid1,ival+1,isz+1);CHKERRQ(ierr);
        isz++;
      }
#else
      if (ival>Nbs) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"index greater than mat-dim");
      if(!PetscBTLookupSet(table,ival)) { nidx[isz++] = ival;}
#endif
    }
    ierr = ISRestoreIndices(is_in[i],&idx);CHKERRQ(ierr);
#if defined (PETSC_USE_CTABLE)
    ierr = PetscMalloc(isz*sizeof(PetscInt),&nidx);CHKERRQ(ierr); 
    ierr = PetscTableGetHeadPosition(gid1_lid1,&tpos);CHKERRQ(ierr); 
    j = 0;
    while (tpos) {  
      ierr = PetscTableGetNext(gid1_lid1,&tpos,&gid1,&tt);CHKERRQ(ierr);
      if (tt-- > isz) { SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"index greater than array-dim"); }
      nidx[tt] = gid1 - 1;
      j++;
    }
    if (j != isz) { SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"table error: jj != isz"); }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,isz,nidx,(is_out+i));CHKERRQ(ierr);
    ierr = PetscFree(nidx);CHKERRQ(ierr);
#else
    ierr = ISCreateGeneral(PETSC_COMM_SELF,isz,nidx,(is_out+i));CHKERRQ(ierr);
#endif
  }
#if defined (PETSC_USE_CTABLE)
  ierr = PetscTableDestroy(gid1_lid1);CHKERRQ(ierr);
#else
  ierr = PetscBTDestroy(table);CHKERRQ(ierr);
  ierr = PetscFree(nidx);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISCompressIndicesSorted"
PetscErrorCode PETSCVEC_DLLEXPORT ISCompressIndicesSorted(PetscInt n,PetscInt bs,PetscInt imax,const IS is_in[],IS is_out[])
{
  PetscErrorCode ierr;
  PetscInt       i,j,k,val,len,*nidx,bbs;
  const PetscInt *idx,*idx_local;
  PetscTruth     flg,isblock;
#if defined (PETSC_USE_CTABLE)
  PetscInt       maxsz;
#else
  PetscInt       Nbs=n/bs;
#endif

  PetscFunctionBegin;
  for (i=0; i<imax; i++) {
    ierr = ISSorted(is_in[i],&flg);CHKERRQ(ierr);
    if (!flg) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Indices are not sorted");
  }

#if defined (PETSC_USE_CTABLE)
  /* Now check max size */
  for (i=0,maxsz=0; i<imax; i++) {
    ierr = ISGetLocalSize(is_in[i],&len);CHKERRQ(ierr);
    if (len%bs !=0) SETERRQ(PETSC_ERR_ARG_INCOMP,"Indices are not block ordered");
    len = len/bs; /* The reduced index size */
    if (len > maxsz) maxsz = len;
  }
  ierr = PetscMalloc(maxsz*sizeof(PetscInt),&nidx);CHKERRQ(ierr);   
#else
  ierr = PetscMalloc(Nbs*sizeof(PetscInt),&nidx);CHKERRQ(ierr); 
#endif
  /* Now check if the indices are in block order */
  for (i=0; i<imax; i++) {
    ierr = ISGetLocalSize(is_in[i],&len);CHKERRQ(ierr);

    /* special case where IS is already block IS of the correct size */
    ierr = ISBlock(is_in[i],&isblock);CHKERRQ(ierr);
    if (isblock) {
      ierr = ISBlockGetLocalSize(is_in[i],&bbs);CHKERRQ(ierr);
      if (bs == bbs) {
        len = len/bs;
        ierr = ISBlockGetIndices(is_in[i],&idx);CHKERRQ(ierr);
        for (j=0; j<len; j++) nidx[j] = idx[j]/bs;
        ierr = ISBlockRestoreIndices(is_in[i],&idx);CHKERRQ(ierr);
        ierr = ISCreateGeneral(PETSC_COMM_SELF,len,nidx,(is_out+i));CHKERRQ(ierr);
        continue;
      }
    }
    ierr = ISGetIndices(is_in[i],&idx);CHKERRQ(ierr);
    if (len%bs !=0) SETERRQ(PETSC_ERR_ARG_INCOMP,"Indices are not block ordered");

    len = len/bs; /* The reduced index size */
    idx_local = idx;
    for (j=0; j<len ; j++) {
      val = idx_local[0];
      if (val%bs != 0) SETERRQ(PETSC_ERR_ARG_INCOMP,"Indices are not block ordered");
      for (k=0; k<bs; k++) {
        if (val+k != idx_local[k]) SETERRQ(PETSC_ERR_ARG_INCOMP,"Indices are not block ordered");
      }
      nidx[j] = val/bs;
      idx_local +=bs;
    }
    ierr = ISRestoreIndices(is_in[i],&idx);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PETSC_COMM_SELF,len,nidx,(is_out+i));CHKERRQ(ierr);
  }
  ierr = PetscFree(nidx);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "ISExpandIndicesGeneral"
PetscErrorCode PETSCVEC_DLLEXPORT ISExpandIndicesGeneral(PetscInt n,PetscInt bs,PetscInt imax,const IS is_in[],IS is_out[])
{
  PetscErrorCode ierr;
  PetscInt       len,i,j,k,*nidx;
  const PetscInt *idx;
#if defined (PETSC_USE_CTABLE)
  PetscInt       maxsz;
#else
  PetscInt       Nbs; 
#endif

  PetscFunctionBegin;
#if defined (PETSC_USE_CTABLE)
  /* Now check max size */
  for (i=0,maxsz=0; i<imax; i++) {
    ierr = ISGetIndices(is_in[i],&idx);CHKERRQ(ierr);
    ierr = ISGetLocalSize(is_in[i],&len);CHKERRQ(ierr);
    if (len*bs > maxsz) maxsz = len*bs;
  }
  ierr = PetscMalloc(maxsz*sizeof(PetscInt),&nidx);CHKERRQ(ierr);   
#else
  Nbs = n/bs; 
  ierr = PetscMalloc(Nbs*bs*sizeof(PetscInt),&nidx);CHKERRQ(ierr); 
#endif

  for (i=0; i<imax; i++) {
    ierr = ISGetIndices(is_in[i],&idx);CHKERRQ(ierr);
    ierr = ISGetLocalSize(is_in[i],&len);CHKERRQ(ierr);
    for (j=0; j<len ; ++j){
      for (k=0; k<bs; k++)
        nidx[j*bs+k] = idx[j]*bs+k;
    }
    ierr = ISRestoreIndices(is_in[i],&idx);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PETSC_COMM_SELF,len*bs,nidx,is_out+i);CHKERRQ(ierr);
  }
  ierr = PetscFree(nidx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

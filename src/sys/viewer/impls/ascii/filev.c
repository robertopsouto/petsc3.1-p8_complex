#define PETSC_DLL

#include "../src/sys/viewer/impls/ascii/asciiimpl.h"  /*I     "petscsys.h"   I*/
#include <stdarg.h>

#define QUEUESTRINGSIZE 8192

/* ----------------------------------------------------------------------*/
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerDestroy_ASCII" 
PetscErrorCode PetscViewerDestroy_ASCII(PetscViewer viewer)
{
  PetscMPIInt       rank;
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscViewerLink   *vlink;
  PetscTruth        flg;
  int               err;

  PetscFunctionBegin;
  if (vascii->sviewer) {
    SETERRQ(PETSC_ERR_ORDER,"ASCII PetscViewer destroyed before restoring singleton PetscViewer");
  }
  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  if (!rank && vascii->fd != stderr && vascii->fd != PETSC_STDOUT) {
    if (vascii->fd) {
      err = fclose(vascii->fd);
      if (err) SETERRQ(PETSC_ERR_SYS,"fclose() failed on file");    
    }
    if (vascii->storecompressed) {
      char par[PETSC_MAX_PATH_LEN],buf[PETSC_MAX_PATH_LEN];
      FILE *fp;
      ierr = PetscStrcpy(par,"gzip ");CHKERRQ(ierr);
      ierr = PetscStrcat(par,vascii->filename);CHKERRQ(ierr);
#if defined(PETSC_HAVE_POPEN)
      ierr = PetscPOpen(PETSC_COMM_SELF,PETSC_NULL,par,"r",&fp);CHKERRQ(ierr);
      if (fgets(buf,1024,fp)) {
        SETERRQ2(PETSC_ERR_LIB,"Error from compression command %s\n%s",par,buf);
      }
      ierr = PetscPClose(PETSC_COMM_SELF,fp);CHKERRQ(ierr);
#else
      SETERRQ(PETSC_ERR_SUP_SYS,"Cannot run external programs on this machine");
#endif
    }
  }
  ierr = PetscStrfree(vascii->filename);CHKERRQ(ierr);
  ierr = PetscFree(vascii);CHKERRQ(ierr);

  /* remove the viewer from the list in the MPI Communicator */
  if (Petsc_Viewer_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelViewer,&Petsc_Viewer_keyval,(void*)0);CHKERRQ(ierr);
  }

  ierr = MPI_Attr_get(((PetscObject)viewer)->comm,Petsc_Viewer_keyval,(void**)&vlink,(PetscMPIInt*)&flg);CHKERRQ(ierr);
  if (flg) {
    if (vlink && vlink->viewer == viewer) {
      ierr = MPI_Attr_put(((PetscObject)viewer)->comm,Petsc_Viewer_keyval,vlink->next);CHKERRQ(ierr);
      ierr = PetscFree(vlink);CHKERRQ(ierr);
    } else {
      while (vlink && vlink->next) {
        if (vlink->next->viewer == viewer) {
          PetscViewerLink *nv = vlink->next;
          vlink->next = vlink->next->next;
          ierr = PetscFree(nv);CHKERRQ(ierr);
        }
        vlink = vlink->next;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerDestroy_ASCII_Singleton" 
PetscErrorCode PetscViewerDestroy_ASCII_Singleton(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscErrorCode    ierr;
  PetscFunctionBegin;
  ierr = PetscViewerRestoreSingleton(vascii->bviewer,&viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerDestroy_ASCII_Subcomm" 
PetscErrorCode PetscViewerDestroy_ASCII_Subcomm(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscErrorCode    ierr;
  PetscFunctionBegin;
  ierr = PetscViewerRestoreSubcomm(vascii->bviewer,((PetscObject)viewer)->comm,&viewer);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFlush_ASCII_Singleton_0" 
PetscErrorCode PetscViewerFlush_ASCII_Singleton_0(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  int               err;

  PetscFunctionBegin;
  err = fflush(vascii->fd);
  if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
  PetscFunctionReturn(0);  
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFlush_ASCII" 
PetscErrorCode PetscViewerFlush_ASCII(PetscViewer viewer)
{
  PetscMPIInt       rank;
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  int               err;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  if (!rank) {
    err = fflush(vascii->fd);
    if (err) SETERRQ(PETSC_ERR_SYS,"fflush() call failed");
  }

  /*
     Also flush anything printed with PetscViewerASCIISynchronizedPrintf()
  */
  ierr = PetscSynchronizedFlush(((PetscObject)viewer)->comm);CHKERRQ(ierr);
  PetscFunctionReturn(0);  
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIGetPointer" 
/*@C
    PetscViewerASCIIGetPointer - Extracts the file pointer from an ASCII PetscViewer.

    Not Collective

+   viewer - PetscViewer context, obtained from PetscViewerASCIIOpen()
-   fd - file pointer

    Level: intermediate

    Fortran Note:
    This routine is not supported in Fortran.

  Concepts: PetscViewer^file pointer
  Concepts: file pointer^getting from PetscViewer

.seealso: PetscViewerASCIIOpen(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerCreate(), PetscViewerASCIIPrintf(),
          PetscViewerASCIISynchronizedPrintf(), PetscViewerFlush()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIGetPointer(PetscViewer viewer,FILE **fd)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  *fd = vascii->fd;
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileGetMode_ASCII"
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileGetMode_ASCII(PetscViewer viewer, PetscFileMode *mode)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  *mode = vascii->mode;
  PetscFunctionReturn(0);
}
EXTERN_C_END

/*@C
    PetscViewerFileSetMode - Sets the mode in which to open the file.

    Not Collective

+   viewer - viewer context, obtained from PetscViewerCreate()
-   mode   - The file mode

    Level: intermediate

    Fortran Note:
    This routine is not supported in Fortran.

.keywords: Viewer, file, get, pointer

.seealso: PetscViewerASCIIOpen(), PetscViewerBinaryOpen()
@*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileSetMode_ASCII"
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileSetMode_ASCII(PetscViewer viewer, PetscFileMode mode)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  vascii->mode = mode;
  PetscFunctionReturn(0);
}
EXTERN_C_END

/*
   If petsc_history is on, then all Petsc*Printf() results are saved
   if the appropriate (usually .petschistory) file.
*/
extern FILE *petsc_history;

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIISetTab" 
/*@
    PetscViewerASCIISetTab - Causes PetscViewer to tab in a number of times

    Not Collective, but only first processor in set has any effect

    Input Parameters:
+    viewer - optained with PetscViewerASCIIOpen()
-    tabs - number of tabs

    Level: developer

    Fortran Note:
    This routine is not supported in Fortran.

  Concepts: PetscViewerASCII^formating
  Concepts: tab^setting

.seealso: PetscPrintf(), PetscSynchronizedPrintf(), PetscViewerASCIIPrintf(),
          PetscViewerASCIIPopTab(), PetscViewerASCIISynchronizedPrintf(), PetscViewerASCIIOpen(),
          PetscViewerCreate(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerASCIIGetPointer(), PetscViewerASCIIPushTab()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIISetTab(PetscViewer viewer,PetscInt tabs)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscTruth        iascii;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ascii->tab = tabs;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIPushTab" 
/*@
    PetscViewerASCIIPushTab - Adds one more tab to the amount that PetscViewerASCIIPrintf()
     lines are tabbed.

    Not Collective, but only first processor in set has any effect

    Input Parameters:
.    viewer - optained with PetscViewerASCIIOpen()

    Level: developer

    Fortran Note:
    This routine is not supported in Fortran.

  Concepts: PetscViewerASCII^formating
  Concepts: tab^setting

.seealso: PetscPrintf(), PetscSynchronizedPrintf(), PetscViewerASCIIPrintf(),
          PetscViewerASCIIPopTab(), PetscViewerASCIISynchronizedPrintf(), PetscViewerASCIIOpen(),
          PetscViewerCreate(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerASCIIGetPointer()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIPushTab(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscTruth        iascii;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ascii->tab++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIPopTab" 
/*@
    PetscViewerASCIIPopTab - Removes one tab from the amount that PetscViewerASCIIPrintf()
     lines are tabbed.

    Not Collective, but only first processor in set has any effect

    Input Parameters:
.    viewer - optained with PetscViewerASCIIOpen()

    Level: developer

    Fortran Note:
    This routine is not supported in Fortran.

  Concepts: PetscViewerASCII^formating
  Concepts: tab^setting

.seealso: PetscPrintf(), PetscSynchronizedPrintf(), PetscViewerASCIIPrintf(),
          PetscViewerASCIIPushTab(), PetscViewerASCIISynchronizedPrintf(), PetscViewerASCIIOpen(),
          PetscViewerCreate(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerASCIIGetPointer()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIPopTab(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscErrorCode    ierr;
  PetscTruth        iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (ascii->tab <= 0) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"More tabs popped than pushed");
    ascii->tab--;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIUseTabs" 
/*@
    PetscViewerASCIIUseTabs - Turns on or off the use of tabs with the ASCII PetscViewer

    Not Collective, but only first processor in set has any effect

    Input Parameters:
+    viewer - optained with PetscViewerASCIIOpen()
-    flg - PETSC_YES or PETSC_NO

    Level: developer

    Fortran Note:
    This routine is not supported in Fortran.

  Concepts: PetscViewerASCII^formating
  Concepts: tab^setting

.seealso: PetscPrintf(), PetscSynchronizedPrintf(), PetscViewerASCIIPrintf(),
          PetscViewerASCIIPopTab(), PetscViewerASCIISynchronizedPrintf(), PetscViewerASCIIPushTab(), PetscViewerASCIIOpen(),
          PetscViewerCreate(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerASCIIGetPointer()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIUseTabs(PetscViewer viewer,PetscTruth flg)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscTruth        iascii;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (flg) {
      ascii->tab       = ascii->tab_store;
    } else {
      ascii->tab_store = ascii->tab;
      ascii->tab       = 0;
    }
  }
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------- */

#include "../src/sys/fileio/mprint.h" /* defines the queue datastructures and variables */

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIPrintf" 
/*@C
    PetscViewerASCIIPrintf - Prints to a file, only from the first
    processor in the PetscViewer

    Not Collective, but only first processor in set has any effect

    Input Parameters:
+    viewer - optained with PetscViewerASCIIOpen()
-    format - the usual printf() format string 

    Level: developer

    Fortran Note:
    The call sequence is PetscViewerASCIIPrintf(PetscViewer, character(*), int ierr) from Fortran. 
    That is, you can only pass a single character string from Fortran.

  Concepts: PetscViewerASCII^printing
  Concepts: printing^to file
  Concepts: printf

.seealso: PetscPrintf(), PetscSynchronizedPrintf(), PetscViewerASCIIOpen(),
          PetscViewerASCIIPushTab(), PetscViewerASCIIPopTab(), PetscViewerASCIISynchronizedPrintf(),
          PetscViewerCreate(), PetscViewerDestroy(), PetscViewerSetType(), PetscViewerASCIIGetPointer()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIPrintf(PetscViewer viewer,const char format[],...)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscMPIInt       rank;
  PetscInt          tab;
  PetscErrorCode    ierr;
  FILE              *fd = ascii->fd;
  PetscTruth        iascii;
  int               err;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  PetscValidCharPointer(format,2);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (!iascii) SETERRQ(PETSC_ERR_ARG_WRONG,"Not ASCII PetscViewer");

  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  if (ascii->bviewer) {ierr = MPI_Comm_rank(((PetscObject)ascii->bviewer)->comm,&rank);CHKERRQ(ierr);}
  if (!rank) {
    va_list Argp;
    if (ascii->bviewer) {
      queuefile = fd;
    }

    tab = ascii->tab;
    while (tab--) {ierr = PetscFPrintf(PETSC_COMM_SELF,fd,"  ");CHKERRQ(ierr);}

    va_start(Argp,format);
    ierr = PetscVFPrintf(fd,format,Argp);CHKERRQ(ierr);
    err = fflush(fd);
    if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    if (petsc_history) {
      va_start(Argp,format);
      tab = ascii->tab;
      while (tab--) {ierr = PetscFPrintf(PETSC_COMM_SELF,fd,"  ");CHKERRQ(ierr);}
      ierr = (*PetscVFPrintf)(petsc_history,format,Argp);CHKERRQ(ierr);
      err = fflush(petsc_history);
      if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    }
    va_end(Argp);
  } else if (ascii->bviewer) { /* this is a singleton PetscViewer that is not on process 0 */
    va_list     Argp;
    int         fullLength;
    char        *string;

    PrintfQueue next;
    ierr = PetscNew(struct _PrintfQueue,&next);CHKERRQ(ierr);
    if (queue) {queue->next = next; queue = next;}
    else       {queuebase   = queue = next;}
    queuelength++;
    next->size = QUEUESTRINGSIZE;
    ierr = PetscMalloc(next->size*sizeof(char), &next->string);CHKERRQ(ierr);
    ierr = PetscMemzero(next->string,next->size);CHKERRQ(ierr);
    string = next->string;
    tab = 2*ascii->tab;
    while (tab--) {*string++ = ' ';}
    va_start(Argp,format);
    ierr = PetscVSNPrintf(string,next->size-2*ascii->tab,format,&fullLength,Argp);CHKERRQ(ierr);
    va_end(Argp);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileSetName" 
/*@C
     PetscViewerFileSetName - Sets the name of the file the PetscViewer uses.

    Collective on PetscViewer

  Input Parameters:
+  viewer - the PetscViewer; either ASCII or binary
-  name - the name of the file it should use

    Level: advanced

.seealso: PetscViewerCreate(), PetscViewerSetType(), PetscViewerASCIIOpen(), PetscViewerBinaryOpen(), PetscViewerDestroy(),
          PetscViewerASCIIGetPointer(), PetscViewerASCIIPrintf(), PetscViewerASCIISynchronizedPrintf()

@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileSetName(PetscViewer viewer,const char name[])
{
  PetscErrorCode ierr,(*f)(PetscViewer,const char[]);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  PetscValidCharPointer(name,2);
  ierr = PetscObjectQueryFunction((PetscObject)viewer,"PetscViewerFileSetName_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(viewer,name);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileGetName" 
/*@C
     PetscViewerFileGetName - Gets the name of the file the PetscViewer uses.

    Not Collective

  Input Parameter:
.  viewer - the PetscViewer; either ASCII or binary

  Output Parameter:
.  name - the name of the file it is using

    Level: advanced

.seealso: PetscViewerCreate(), PetscViewerSetType(), PetscViewerASCIIOpen(), PetscViewerBinaryOpen(), PetscViewerFileSetName()

@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileGetName(PetscViewer viewer,char **name)
{
  PetscErrorCode ierr,(*f)(PetscViewer,char **);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)viewer,"PetscViewerFileGetName_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(viewer,name);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileGetName_ASCII" 
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileGetName_ASCII(PetscViewer viewer,char **name)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII*)viewer->data;

  PetscFunctionBegin;
  *name = vascii->filename;
  PetscFunctionReturn(0);
}
EXTERN_C_END


EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerFileSetName_ASCII" 
PetscErrorCode PETSC_DLLEXPORT PetscViewerFileSetName_ASCII(PetscViewer viewer,const char name[])
{
  PetscErrorCode    ierr;
  size_t            len;
  char              fname[PETSC_MAX_PATH_LEN],*gz;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII*)viewer->data;
  PetscTruth        isstderr,isstdout;
  PetscMPIInt       rank;

  PetscFunctionBegin;
  if (!name) PetscFunctionReturn(0);
  ierr = PetscStrfree(vascii->filename);CHKERRQ(ierr);
  ierr = PetscStrallocpy(name,&vascii->filename);CHKERRQ(ierr);

  /* Is this file to be compressed */
  vascii->storecompressed = PETSC_FALSE;
  ierr = PetscStrstr(vascii->filename,".gz",&gz);CHKERRQ(ierr);
  if (gz) {
    ierr = PetscStrlen(gz,&len);CHKERRQ(ierr);
    if (len == 3) {
      *gz = 0;
      vascii->storecompressed = PETSC_TRUE;
    } 
  }
  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscStrcmp(name,"stderr",&isstderr);CHKERRQ(ierr);
    ierr = PetscStrcmp(name,"stdout",&isstdout);CHKERRQ(ierr);
    /* empty filename means stdout */
    if (name[0] == 0)  isstdout = PETSC_TRUE;
    if (isstderr)      vascii->fd = PETSC_STDERR;
    else if (isstdout) vascii->fd = PETSC_STDOUT;
    else {


      ierr = PetscFixFilename(name,fname);CHKERRQ(ierr);
      switch(vascii->mode) {
      case FILE_MODE_READ:
	vascii->fd = fopen(fname,"r");
	break;
      case FILE_MODE_WRITE:
	vascii->fd = fopen(fname,"w");
	break;
      case FILE_MODE_APPEND:
	vascii->fd = fopen(fname,"a");
	break;
      case FILE_MODE_UPDATE:
	vascii->fd = fopen(fname,"r+");
	if (!vascii->fd) {
	  vascii->fd = fopen(fname,"w+");
	}
	break;
      case FILE_MODE_APPEND_UPDATE:
	/* I really want a file which is opened at the end for updating,
	   not a+, which opens at the beginning, but makes writes at the end.
	*/
	vascii->fd = fopen(fname,"r+");
	if (!vascii->fd) {
	  vascii->fd = fopen(fname,"w+");
	} else {
	  ierr     = fseek(vascii->fd, 0, SEEK_END);CHKERRQ(ierr);
	}
	break;
      default:
	SETERRQ1(PETSC_ERR_ARG_WRONG, "Invalid file mode %d", vascii->mode);
      }
      if (!vascii->fd) SETERRQ1(PETSC_ERR_FILE_OPEN,"Cannot open PetscViewer file: %s",fname);
    }
  }
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)viewer,"File: %s",name);
#endif
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerGetSingleton_ASCII" 
PetscErrorCode PetscViewerGetSingleton_ASCII(PetscViewer viewer,PetscViewer *outviewer)
{
  PetscMPIInt       rank;
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data,*ovascii;
  const char        *name;

  PetscFunctionBegin;
  if (vascii->sviewer) {
    SETERRQ(PETSC_ERR_ORDER,"Singleton already obtained from PetscViewer and not restored");
  }
  ierr         = PetscViewerCreate(PETSC_COMM_SELF,outviewer);CHKERRQ(ierr);
  ierr         = PetscViewerSetType(*outviewer,PETSC_VIEWER_ASCII);CHKERRQ(ierr);
  ovascii      = (PetscViewer_ASCII*)(*outviewer)->data;
  ovascii->fd  = vascii->fd;
  ovascii->tab = vascii->tab;

  vascii->sviewer = *outviewer;

  (*outviewer)->format     = viewer->format;
  (*outviewer)->iformat    = viewer->iformat;

  ierr = PetscObjectGetName((PetscObject)viewer,&name);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)(*outviewer),name);CHKERRQ(ierr);

  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  ((PetscViewer_ASCII*)((*outviewer)->data))->bviewer = viewer;
  (*outviewer)->ops->destroy = PetscViewerDestroy_ASCII_Singleton;
  if (rank) {
    (*outviewer)->ops->flush = 0;
  } else {
    (*outviewer)->ops->flush = PetscViewerFlush_ASCII_Singleton_0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerRestoreSingleton_ASCII" 
PetscErrorCode PetscViewerRestoreSingleton_ASCII(PetscViewer viewer,PetscViewer *outviewer)
{
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)(*outviewer)->data;
  PetscViewer_ASCII *ascii  = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  if (!ascii->sviewer) {
    SETERRQ(PETSC_ERR_ORDER,"Singleton never obtained from PetscViewer");
  }
  if (ascii->sviewer != *outviewer) {
    SETERRQ(PETSC_ERR_ARG_WRONG,"This PetscViewer did not generate singleton");
  }

  ascii->sviewer             = 0;
  vascii->fd                 = PETSC_STDOUT;
  (*outviewer)->ops->destroy = PetscViewerDestroy_ASCII;
  ierr                       = PetscViewerDestroy(*outviewer);CHKERRQ(ierr);
  ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerGetSubcomm_ASCII" 
PetscErrorCode PetscViewerGetSubcomm_ASCII(PetscViewer viewer,MPI_Comm subcomm,PetscViewer *outviewer)
{
  PetscMPIInt       rank;
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data,*ovascii;
  const char        *name;

  PetscFunctionBegin;
  if (vascii->sviewer) {
    SETERRQ(PETSC_ERR_ORDER,"Subcomm already obtained from PetscViewer and not restored");
  }
  /* ierr         = PetscViewerCreate(PETSC_COMM_SELF,outviewer);CHKERRQ(ierr); */
  ierr         = PetscViewerCreate(subcomm,outviewer);CHKERRQ(ierr);
  ierr         = PetscViewerSetType(*outviewer,PETSC_VIEWER_ASCII);CHKERRQ(ierr);
  ovascii      = (PetscViewer_ASCII*)(*outviewer)->data;
  ovascii->fd  = vascii->fd;
  ovascii->tab = vascii->tab;

  vascii->sviewer = *outviewer;

  (*outviewer)->format     = viewer->format;
  (*outviewer)->iformat    = viewer->iformat;

  ierr = PetscObjectGetName((PetscObject)viewer,&name);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)(*outviewer),name);CHKERRQ(ierr);

  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr); 
  ((PetscViewer_ASCII*)((*outviewer)->data))->bviewer = viewer;
  (*outviewer)->ops->destroy = PetscViewerDestroy_ASCII_Singleton;
  /* following might not be correct??? */
  if (rank) {
    (*outviewer)->ops->flush = 0;
  } else {
    (*outviewer)->ops->flush = PetscViewerFlush_ASCII_Singleton_0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerRestoreSubcomm_ASCII" 
PetscErrorCode PetscViewerRestoreSubcomm_ASCII(PetscViewer viewer,MPI_Comm subcomm,PetscViewer *outviewer)
{
  PetscErrorCode    ierr;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)(*outviewer)->data;
  PetscViewer_ASCII *ascii  = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  if (!ascii->sviewer) {
    SETERRQ(PETSC_ERR_ORDER,"Subcomm never obtained from PetscViewer");
  }
  if (ascii->sviewer != *outviewer) {
    SETERRQ(PETSC_ERR_ARG_WRONG,"This PetscViewer did not generate subcomm");
  }

  ascii->sviewer             = 0;
  vascii->fd                 = PETSC_STDOUT;
  (*outviewer)->ops->destroy = PetscViewerDestroy_ASCII; 
  ierr = PetscViewerDestroy(*outviewer);CHKERRQ(ierr);
  ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerCreate_ASCII" 
PetscErrorCode PETSC_DLLEXPORT PetscViewerCreate_ASCII(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr         = PetscNewLog(viewer,PetscViewer_ASCII,&vascii);CHKERRQ(ierr);
  viewer->data = (void*)vascii;

  viewer->ops->destroy          = PetscViewerDestroy_ASCII;
  viewer->ops->flush            = PetscViewerFlush_ASCII;
  viewer->ops->getsingleton     = PetscViewerGetSingleton_ASCII;
  viewer->ops->restoresingleton = PetscViewerRestoreSingleton_ASCII;
  viewer->ops->getsubcomm       = PetscViewerGetSubcomm_ASCII;
  viewer->ops->restoresubcomm   = PetscViewerRestoreSubcomm_ASCII;

  /* defaults to stdout unless set with PetscViewerFileSetName() */
  vascii->fd             = PETSC_STDOUT;
  vascii->mode           = FILE_MODE_WRITE;
  vascii->bviewer        = 0;
  vascii->sviewer        = 0;
  viewer->format         = PETSC_VIEWER_DEFAULT;
  viewer->iformat        = 0;
  vascii->tab            = 0;
  vascii->tab_store      = 0;
  vascii->filename       = 0;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)viewer,"PetscViewerFileSetName_C","PetscViewerFileSetName_ASCII",
                                     PetscViewerFileSetName_ASCII);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)viewer,"PetscViewerFileGetName_C","PetscViewerFileGetName_ASCII",
                                     PetscViewerFileGetName_ASCII);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)viewer,"PetscViewerFileGetMode_C","PetscViewerFileGetMode_ASCII",
                                     PetscViewerFileGetMode_ASCII);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)viewer,"PetscViewerFileSetMode_C","PetscViewerFileSetMode_ASCII",
                                     PetscViewerFileSetMode_ASCII);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
EXTERN_C_END


#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIISynchronizedPrintf" 
/*@C
    PetscViewerASCIISynchronizedPrintf - Prints synchronized output to the specified file from
    several processors.  Output of the first processor is followed by that of the 
    second, etc.

    Not Collective, must call collective PetscViewerFlush() to get the results out

    Input Parameters:
+   viewer - the ASCII PetscViewer
-   format - the usual printf() format string 

    Level: intermediate

    Fortran Note:
      Can only print a single character* string

.seealso: PetscSynchronizedPrintf(), PetscSynchronizedFlush(), PetscFPrintf(),
          PetscFOpen(), PetscViewerFlush(), PetscViewerASCIIGetPointer(), PetscViewerDestroy(), PetscViewerASCIIOpen(),
          PetscViewerASCIIPrintf()

@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIISynchronizedPrintf(PetscViewer viewer,const char format[],...)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscErrorCode    ierr;
  PetscMPIInt       rank;
  PetscInt          tab = vascii->tab;
  MPI_Comm          comm;
  FILE              *fp;
  PetscTruth        iascii;
  int               err;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  PetscValidCharPointer(format,2);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (!iascii) SETERRQ(PETSC_ERR_ARG_WRONG,"Not ASCII PetscViewer");

  comm = ((PetscObject)viewer)->comm;
  fp   = vascii->fd;
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  if (vascii->bviewer) {ierr = MPI_Comm_rank(((PetscObject)vascii->bviewer)->comm,&rank);CHKERRQ(ierr);}
  

  /* First processor prints immediately to fp */
  if (!rank) {
    va_list Argp;

    while (tab--) {ierr = PetscFPrintf(PETSC_COMM_SELF,fp,"  ");CHKERRQ(ierr);}

    va_start(Argp,format);
    ierr = (*PetscVFPrintf)(fp,format,Argp);CHKERRQ(ierr);
    err = fflush(fp);
    if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    queuefile = fp;
    if (petsc_history) {
      va_start(Argp,format);
      ierr = (*PetscVFPrintf)(petsc_history,format,Argp);CHKERRQ(ierr);
      err = fflush(petsc_history);
      if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    }
    va_end(Argp);
  } else { /* other processors add to local queue */
    char        *string;
    va_list     Argp;
    int         fullLength;
    PrintfQueue next;

    ierr = PetscNew(struct _PrintfQueue,&next);CHKERRQ(ierr);
    if (queue) {queue->next = next; queue = next;}
    else       {queuebase   = queue = next;}
    queuelength++;
    next->size = QUEUESTRINGSIZE;
    ierr = PetscMalloc(next->size*sizeof(char), &next->string);CHKERRQ(ierr);
    ierr = PetscMemzero(next->string,next->size);CHKERRQ(ierr);
    string = next->string;
    tab *= 2;
    while (tab--) {*string++ = ' ';}
    va_start(Argp,format);
    ierr = PetscVSNPrintf(string,next->size-2*vascii->tab,format,&fullLength,Argp);
    va_end(Argp);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIMonitorCreate" 
/*@C
   PetscViewerASCIIMonitorCreate - Opens an ASCII file as a monitor object, suitable for the default KSP, SNES and TS monitors

   Collective on MPI_Comm

   Input Parameters:
+  comm - the communicator
.  name - the file name
-  tabs - how far in the text should be tabbed

   Output Parameter:
.  lab - the context to be used with KSP/SNES/TSMonitorSet()

   Level: advanced

   Notes:
   This can be destroyed with PetscViewerASCIIMonitorDestroy().

   See PetscViewerASCIIOpen()

.seealso: KSPMonitorSet(), SNESMonitorSet(), TSMonitorSet(), KSPMonitorDefault(), PetscViewerASCIIMonitor, PetscViewerASCIIMonitorDestroy()

@*/
PetscErrorCode PetscViewerASCIIMonitorCreate(MPI_Comm comm,const char *filename,PetscInt tabs,PetscViewerASCIIMonitor* ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr         = PetscNew(struct _p_PetscViewerASCIIMonitor,ctx);CHKERRQ(ierr);
  ierr         = PetscViewerASCIIOpen(comm,filename,&(*ctx)->viewer);CHKERRQ(ierr);
  (*ctx)->tabs = tabs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIMonitorDestroy" 
/*@C
   PetscViewerASCIIMonitorDestroys - removes a monitor context.

   Collective on PetscViewerASCIIMonitor

   Input Parameters:
.   ctx - the monitor context created with PetscViewerASCIIMonitorCreate()

   Level: advanced

   Notes:
     This is rarely called by users, it is usually called when the KSP, SNES or TS object is destroyed

.seealso: KSPMonitorSet(), SNESMonitorSet(), TSMonitorSet(), KSPMonitorDefault(), PetscViewerASCIIMonitor, PetscViewerASCIIMonitorCreate()

@*/
PetscErrorCode PetscViewerASCIIMonitorDestroy(PetscViewerASCIIMonitor ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr         = PetscViewerDestroy(ctx->viewer);CHKERRQ(ierr);
  ierr         = PetscFree(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerASCIIMonitorPrintf" 
/*@C
    PetscViewerASCIIMonitorPrintf - Prints to the viewer associated with this monitor context

    Not Collective, but only first processor in set has any effect

    Input Parameters:
+    ctx - the context obtained with PetscViewerASCIIMonitorCreate()
-    format - the usual printf() format string 

    Level: developer

    Developer Notes: This code is virtually identical to PetscViewerASCIIPrintf(), however the code
      could not simply be called from here due to the var args.

.seealso: KSPMonitorSet(), SNESMonitorSet(), TSMonitorSet(), KSPMonitorDefault(), PetscViewerASCIIMonitor, PetscViewerASCIIMonitorCreate(),
          PetscPrintf(), PetscFPrintf(), PetscViewerASCIIPrintf()


@*/
PetscErrorCode PETSC_DLLEXPORT PetscViewerASCIIMonitorPrintf(PetscViewerASCIIMonitor ctx,const char format[],...)
{
  PetscViewer       viewer = ctx->viewer;
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII*)viewer->data;
  PetscMPIInt       rank;
  PetscInt          tab;
  PetscErrorCode    ierr;
  FILE              *fd = ascii->fd;
  PetscTruth        iascii;
  int               err;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,1);
  PetscValidCharPointer(format,2);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (!iascii) SETERRQ(PETSC_ERR_ARG_WRONG,"Not ASCII PetscViewer");

  ierr = MPI_Comm_rank(((PetscObject)viewer)->comm,&rank);CHKERRQ(ierr);
  if (ascii->bviewer) {ierr = MPI_Comm_rank(((PetscObject)ascii->bviewer)->comm,&rank);CHKERRQ(ierr);}
  if (!rank) {
    va_list Argp;
    if (ascii->bviewer) {
      queuefile = fd;
    }

    tab = ascii->tab + ctx->tabs;
    while (tab--) {ierr = PetscFPrintf(PETSC_COMM_SELF,fd,"  ");CHKERRQ(ierr);}

    va_start(Argp,format);
    ierr = (*PetscVFPrintf)(fd,format,Argp);CHKERRQ(ierr);
    err = fflush(fd);
    if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    if (petsc_history) {
      va_start(Argp,format);
      tab = ascii->tab + ctx->tabs;
      while (tab--) {ierr = PetscFPrintf(PETSC_COMM_SELF,fd,"  ");CHKERRQ(ierr);}
      ierr = (*PetscVFPrintf)(petsc_history,format,Argp);CHKERRQ(ierr);
      err = fflush(petsc_history);
      if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
    }
    va_end(Argp);
  } else if (ascii->bviewer) { /* this is a singleton PetscViewer that is not on process 0 */
    va_list     Argp;
    int         fullLength;
    char        *string;
    PrintfQueue next;

    ierr = PetscNew(struct _PrintfQueue,&next);CHKERRQ(ierr);
    if (queue) {queue->next = next; queue = next;}
    else       {queuebase   = queue = next;}
    queuelength++;
    next->size = QUEUESTRINGSIZE;
    ierr = PetscMalloc(next->size*sizeof(char), &next->string);CHKERRQ(ierr);
    ierr = PetscMemzero(next->string,next->size);CHKERRQ(ierr);
    string = next->string;
    tab = 2*(ascii->tab + ctx->tabs);
    while (tab--) {*string++ = ' ';}
    va_start(Argp,format);
    ierr = PetscVSNPrintf(string,next->size-2*ascii->tab,format,&fullLength,Argp);CHKERRQ(ierr);
    va_end(Argp);
  }
  PetscFunctionReturn(0);
}

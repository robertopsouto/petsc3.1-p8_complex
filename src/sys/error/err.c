#define PETSC_DLL
/*
      Code that allows one to set the error handlers
*/
#include "petscsys.h"           /*I "petscsys.h" I*/
#include <stdarg.h>
#if defined(PETSC_HAVE_STDLIB_H)
#include <stdlib.h>
#endif

typedef struct _EH *EH;
struct _EH {
  int            cookie;
  PetscErrorCode (*handler)(int,const char*,const char*,const char *,PetscErrorCode,int,const char*,void *);
  void           *ctx;
  EH             previous;
};

static EH eh = 0;

#undef __FUNCT__  
#define __FUNCT__ "PetscEmacsClientErrorHandler" 
/*@C
   PetscEmacsClientErrorHandler - Error handler that uses the emacsclient program to 
    load the file where the error occured. Then calls the "previous" error handler.

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where error is detected (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - specific error number
-  ctx - error handler context

   Options Database Key:
.   -on_error_emacs <machinename>

   Level: developer

   Notes:
   You must put (server-start) in your .emacs file for the emacsclient software to work

   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERRQ, which has 
   the calling sequence
$     SETERRQ(number,p,mess)

   Notes for experienced users:
   Use PetscPushErrorHandler() to set the desired error handler.

   Concepts: emacs^going to on error
   Concepts: error handler^going to line in emacs

.seealso:  PetscPushErrorHandler(), PetscAttachDebuggerErrorHandler(), 
          PetscAbortErrorHandler()
 @*/
PetscErrorCode PETSC_DLLEXPORT PetscEmacsClientErrorHandler(int line,const char *fun,const char* file,const char *dir,PetscErrorCode n,int p,const char *mess,void *ctx)
{
  PetscErrorCode ierr;
  char        command[PETSC_MAX_PATH_LEN];
  const char  *pdir;
  FILE        *fp;

  PetscFunctionBegin;
  /* Note: don't check error codes since this an error handler :-) */
  ierr = PetscGetPetscDir(&pdir);CHKERRQ(ierr);
  sprintf(command,"emacsclient +%d %s/%s%s\n",line,pdir,dir,file);
#if defined(PETSC_HAVE_POPEN)
  ierr = PetscPOpen(MPI_COMM_WORLD,(char*)ctx,command,"r",&fp);
  ierr = PetscPClose(MPI_COMM_WORLD,fp);
#else
  SETERRQ(PETSC_ERR_SUP_SYS,"Cannot run external programs on this machine");
#endif
  ierr = PetscPopErrorHandler(); /* remove this handler from the stack of handlers */
  if (!eh)     ierr = PetscTraceBackErrorHandler(line,fun,file,dir,n,p,mess,0);
  else         ierr = (*eh->handler)(line,fun,file,dir,n,p,mess,eh->ctx);
  PetscFunctionReturn(ierr);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscPushErrorHandler" 
/*@C
   PetscPushErrorHandler - Sets a routine to be called on detection of errors.

   Not Collective

   Input Parameters:
+  handler - error handler routine
-  ctx - optional handler context that contains information needed by the handler (for 
         example file pointers for error messages etc.)

   Calling sequence of handler:
$    int handler(int line,char *func,char *file,char *dir,PetscErrorCode n,int p,char *mess,void *ctx);

+  func - the function where the error occured (indicated by __FUNCT__)
.  line - the line number of the error (indicated by __LINE__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  n - the generic error number (see list defined in include/petscerror.h)
.  p - the specific error number
.  mess - an error text string, usually just printed to the screen
-  ctx - the error handler context

   Options Database Keys:
+   -on_error_attach_debugger <noxterm,gdb or dbx>
-   -on_error_abort

   Level: intermediate

   Notes:
   The currently available PETSc error handlers include PetscTraceBackErrorHandler(),
   PetscAttachDebuggerErrorHandler(), PetscAbortErrorHandler(), and PetscMPIAbortErrorHandler(), PetscReturnErrorHandler().

   Fortran Notes: You can only push one error handler from Fortran before poping it.

.seealso: PetscPopErrorHandler(), PetscAttachDebuggerErrorHandler(), PetscAbortErrorHandler(), PetscTraceBackErrorHandler()

@*/
PetscErrorCode PETSC_DLLEXPORT PetscPushErrorHandler(PetscErrorCode (*handler)(int,const char *,const char*,const char*,PetscErrorCode,int,const char*,void*),void *ctx)
{
  EH  neweh;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNew(struct _EH,&neweh);CHKERRQ(ierr);
  if (eh) {neweh->previous = eh;} 
  else    {neweh->previous = 0;}
  neweh->handler = handler;
  neweh->ctx     = ctx;
  eh             = neweh;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscPopErrorHandler" 
/*@
   PetscPopErrorHandler - Removes the latest error handler that was 
   pushed with PetscPushErrorHandler().

   Not Collective

   Level: intermediate

   Concepts: error handler^setting

.seealso: PetscPushErrorHandler()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscPopErrorHandler(void)
{
  EH  tmp;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!eh) PetscFunctionReturn(0);
  tmp  = eh;
  eh   = eh->previous;
  ierr = PetscFree(tmp);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
 
#undef __FUNCT__
#define __FUNCT__ "PetscReturnErrorHandler"
/*@C
  PetscReturnErrorHandler - Error handler that causes a return to the current
  level.

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where error is detected (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - specific error number
-  ctx - error handler context

   Level: developer

   Notes:
   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERRQ, which has 
   the calling sequence
$     SETERRQ(number,p,mess)

   Notes for experienced users:
   This routine is good for catching errors such as zero pivots in preconditioners
   or breakdown of iterative methods. It is not appropriate for memory violations
   and similar errors.

   Use PetscPushErrorHandler() to set the desired error handler.  The
   currently available PETSc error handlers include PetscTraceBackErrorHandler(),
   PetscAttachDebuggerErrorHandler(), PetscAbortErrorHandler(), and PetscAbortErrorHandler()

   Concepts: error handler

.seealso:  PetscPushErrorHandler(), PetscPopErrorHandler().
 @*/

PetscErrorCode PETSC_DLLEXPORT PetscReturnErrorHandler(int line,const char *fun,const char* file,const char *dir,PetscErrorCode n,int p,const char *mess,void *ctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(n);
}

static char PetscErrorBaseMessage[1024];
/*
       The numerical values for these are defined in include/petscerror.h; any changes
   there must also be made here
*/
static const char *PetscErrorStrings[] = {
  /*55 */ "Out of memory",
          "No support for this operation for this object type",
          "No support for this operation on this system",
  /*58 */ "Operation done in wrong order",
  /*59 */ "Signal received",
  /*60 */ "Nonconforming object sizes",
          "Argument aliasing not permitted",
          "Invalid argument",
  /*63 */ "Argument out of range",
          "Corrupt argument: see http://www.mcs.anl.gov/petsc/petsc-as/documentation/troubleshooting.html#Corrupt",
          "Unable to open file",
          "Read from file failed",
          "Write to file failed",
          "Invalid pointer",
  /*69 */ "Arguments must have same type",
          "",
  /*71 */ "Detected zero pivot in LU factorization\nsee http://www.mcs.anl.gov/petsc/petsc-as/documentation/troubleshooting.html#ZeroPivot",
  /*72 */ "Floating point exception",
  /*73 */ "Object is in wrong state",
          "Corrupted Petsc object",
          "Arguments are incompatible",
          "Error in external library",
  /*77 */ "Petsc has generated inconsistent data",
          "Memory corruption",
          "Unexpected data in file",
  /*80 */ "Arguments must have same communicators",
  /*81 */ "Detected zero pivot in Cholesky factorization\nsee http://www.mcs.anl.gov/petsc/petsc-as/documentation/troubleshooting.html#ZeroPivot",
          "  ",
          "  ",
          "  ",
  /*85 */ "Null argument, when expecting valid pointer",
  /*86 */ "Unknown type. Check for miss-spelling or missing external package needed for type\n seehttp://www.mcs.anl.gov/petsc/petsc-as/documentation/installation.html#external",
  /*87 */ "Not used",
  /*88 */ "Error in system call",
  /*89 */ "Object Type not set: see http://www.mcs.anl.gov/petsc/petsc-as/documentation/troubleshooting.html#typenotset"};

#undef __FUNCT__  
#define __FUNCT__ "PetscErrorMessage" 
/*@C
   PetscErrorMessage - returns the text string associated with a PETSc error code.

   Not Collective

   Input Parameter:
.   errnum - the error code

   Output Parameter: 
+  text - the error message (PETSC_NULL if not desired) 
-  specific - the specific error message that was set with SETERRxxx() or PetscError().  (PETSC_NULL if not desired) 

   Level: developer

   Concepts: error handler^messages

.seealso:  PetscPushErrorHandler(), PetscAttachDebuggerErrorHandler(), 
          PetscAbortErrorHandler(), PetscTraceBackErrorHandler()
 @*/
PetscErrorCode PETSC_DLLEXPORT PetscErrorMessage(int errnum,const char *text[],char **specific)
{
  PetscFunctionBegin;
  if (text && errnum > PETSC_ERR_MIN_VALUE && errnum < PETSC_ERR_MAX_VALUE) {
    *text = PetscErrorStrings[errnum-PETSC_ERR_MIN_VALUE-1];
  } else if (text) *text = 0;

  if (specific) {
    *specific = PetscErrorBaseMessage;
  }
  PetscFunctionReturn(0);
}

#if defined(PETSC_USE_ERRORCHECKING)
PetscErrorCode PETSC_DLLEXPORT PetscErrorUncatchable[PETSC_EXCEPTIONS_MAX] = {0};
PetscInt       PETSC_DLLEXPORT PetscErrorUncatchableCount                  = 0;
PetscErrorCode PETSC_DLLEXPORT PetscExceptions[PETSC_EXCEPTIONS_MAX]       = {0};
PetscInt       PETSC_DLLEXPORT PetscExceptionsCount                        = 0;
PetscErrorCode PETSC_DLLEXPORT PetscExceptionTmp                           = 0;
PetscErrorCode PETSC_DLLEXPORT PetscExceptionTmp1                          = 0;

#undef __FUNCT__  
#define __FUNCT__ "PetscErrorIsCatchable" 
/*@C
      PetscErrorIsCatchable - Returns if a PetscErrorCode can be caught with a PetscExceptionTry1() or
           PetscExceptionPush()

  Input Parameters:
.   err - error code 

  Level: advanced

   Notes:
    PETSc must not be configured using the option --with-errorchecking=0 for this to work

.seealso: PetscExceptionTry1(), PetscExceptionCaught(), PetscExceptionPush(), PetscExceptionPop(), PetscErrorSetCatchable()
@*/
PetscTruth PETSC_DLLEXPORT PetscErrorIsCatchable(PetscErrorCode err)
{
  PetscInt i;
  for (i=0; i<PetscErrorUncatchableCount; i++) {
    if (err == PetscErrorUncatchable[i]) return PETSC_FALSE;
  }
  return PETSC_TRUE;
}

#undef __FUNCT__  
#define __FUNCT__ "PetscErrorSetCatchable" 
/*@
      PetscErrorSetCatchable - Sets if a PetscErrorCode can be caught with a PetscExceptionTry1()
    PetscExceptionCaught() pair, or PetscExceptionPush(). By default all errors are catchable.

  Input Parameters:
+   err - error code 
-   flg - PETSC_TRUE means allow to be caught, PETSC_FALSE means do not allow to be caught

  Level: advanced

   Notes:
    PETSc must not be configured using the option --with-errorchecking=0 for this to work

.seealso: PetscExceptionTry1(), PetscExceptionCaught(), PetscExceptionPush(), PetscExceptionPop(), PetscErrorIsCatchable()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscErrorSetCatchable(PetscErrorCode err,PetscTruth flg)
{
  PetscFunctionBegin;
  if (!flg && PetscErrorIsCatchable(err)) {
    /* add to list of uncatchable */
    if (PetscErrorUncatchableCount >= PETSC_EXCEPTIONS_MAX) SETERRQ(PETSC_ERR_PLIB,"Stack for PetscErrorUncatchable is overflowed, recompile \nsrc/sysd/error/err.c with a larger value for PETSC_EXCEPTIONS_MAX");
    PetscErrorUncatchable[PetscErrorUncatchableCount++] = err;
  } else if (flg && !PetscErrorIsCatchable(err)) {
    /* remove from list of uncatchable */
    PetscInt i;
    for (i=0; i<PetscErrorUncatchableCount; i++) {
      if (PetscErrorUncatchable[i] == err) break;
    }
    for (;i<PetscErrorUncatchableCount; i++) {
      PetscErrorUncatchable[i] = PetscErrorUncatchable[i+1];
    }
    PetscErrorUncatchableCount--;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscExceptionPush" 
/*@
      PetscExceptionPush - Adds the exception as one to be caught and passed up. If passed up
        can be checked with PetscExceptionCaught() or PetscExceptionValue()

  Input Parameters:
.   err - the exception to catch

  Level: advanced

   Notes:
    PETSc must not be configured using the option --with-errorchecking=0 for this to work

    Use PetscExceptionPop() to remove this as a value to be caught

    This is not usually needed in C/C++ rather use PetscExceptionTry1()

.seealso: PetscExceptionTry1(), PetscExceptionCaught(), PetscExceptionPush(), PetscExceptionPop()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscExceptionPush(PetscErrorCode err) 
{
  PetscFunctionBegin;
  if (PetscExceptionsCount >= PETSC_EXCEPTIONS_MAX) SETERRQ(PETSC_ERR_PLIB,"Stack for PetscExceptions is overflowed, recompile \nsrc/sysd/error/err.c with a larger value for PETSC_EXCEPTIONS_MAX");
  if (PetscErrorIsCatchable(err)) PetscExceptions[PetscExceptionsCount++] = err;
  PetscFunctionReturn(0);   
}

#undef __FUNCT__  
#define __FUNCT__ "PetscExceptionPop" 
/*@
      PetscExceptionPop - Removes  the most recent exception asked to be caught with PetscExceptionPush()

  Input Parameters:
.   err - the exception that was pushed

  Level: advanced

   Notes:
    PETSc must not be configured using the option --with-errorchecking=0 for this to work

    This is not usually needed in C/C++ rather use PetscExceptionTry1()

.seealso: PetscExceptionTry1(), PetscExceptionCaught(), PetscExceptionPush(), PetscExceptionPop()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscExceptionPop(PetscErrorCode err)
{
  PetscFunctionBegin;
  if (PetscExceptionsCount <= 0)SETERRQ(PETSC_ERR_PLIB,"Stack for PetscExceptions is empty");
  if (PetscErrorIsCatchable(err)) PetscExceptionsCount--;
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__  
#define __FUNCT__ "PetscError" 
/*@C
   PetscError - Routine that is called when an error has been detected, 
   usually called through the macro SETERRQ().

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where the error occured (indicated by __FUNCT__)
.  dir - the directory of file (indicated by __SDIR__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - 1 indicates the error was initially detected, 0 indicates this is a traceback from a 
   previously detected error
-  mess - formatted message string - aka printf

  Level: intermediate

   Notes:
   Most users need not directly use this routine and the error handlers, but
   can instead use the simplified interface SETERRQ, which has the calling 
   sequence
$     SETERRQ(n,mess)

   Experienced users can set the error handler with PetscPushErrorHandler().

   Concepts: error^setting condition

.seealso: PetscTraceBackErrorHandler(), PetscPushErrorHandler(), SETERRQ(), CHKERRQ(), CHKMEMQ, SETERRQ1(), SETERRQ2()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscError(int line,const char *func,const char* file,const char *dir,PetscErrorCode n,int p,const char *mess,...)
{
  va_list        Argp;
  int            fullLength;
  PetscErrorCode ierr;
  char           buf[2048],*lbuf = 0;
  PetscTruth     ismain,isunknown;
#if defined(PETSC_USE_ERRORCHECKING)
  PetscInt       i;
#endif

  if (!func)  func = "User provided function";
  if (!file)  file = "User file";
  if (!dir)   dir = " ";

  PetscFunctionBegin;
  /* Compose the message evaluating the print format */
  if (mess) {
    va_start(Argp,mess);
    PetscVSNPrintf(buf,2048,mess,&fullLength,Argp);
    va_end(Argp);
    lbuf = buf;
    if (p == 1) {
      PetscStrncpy(PetscErrorBaseMessage,lbuf,1023);
    }
  }

#if defined(PETSC_USE_ERRORCHECKING)
  /* check if user is catching this exception */
  for (i=0; i<PetscExceptionsCount; i++) {
    if (n == PetscExceptions[i])  PetscFunctionReturn(n);
  }
#endif

  if (!eh)     ierr = PetscTraceBackErrorHandler(line,func,file,dir,n,p,lbuf,0);
  else         ierr = (*eh->handler)(line,func,file,dir,n,p,lbuf,eh->ctx);

  /* 
      If this is called from the main() routine we call MPI_Abort() instead of 
    return to allow the parallel program to be properly shutdown.

    Since this is in the error handler we don't check the errors below. Of course,
    PetscStrncmp() does its own error checking which is problamatic
  */
  PetscStrncmp(func,"main",4,&ismain);
  PetscStrncmp(func,"unknown",7,&isunknown);
  if (ismain || isunknown) {
    MPI_Abort(PETSC_COMM_WORLD,(int)ierr);
  }
  PetscFunctionReturn(ierr);
}

#if defined(PETSC_CLANGUAGE_CXX) && !defined(PETSC_USE_EXTERN_CXX)
#undef __FUNCT__  
#define __FUNCT__ "PetscErrorCxx" 
/*@C
   PetscErrorCxx - Routine that is called when an error has been detected, 
   usually called through the macro SETERROR().

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where the error occured (indicated by __FUNCT__)
.  dir - the directory of file (indicated by __SDIR__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  n - the generic error number
.  p - 1 indicates the error was initially detected, 0 indicates this is a traceback from a 
   previously detected error

  Level: intermediate

   Notes:
   Most users need not directly use this routine and the error handlers, but
   can instead use the simplified interface SETERRQ, which has the calling 
   sequence
$     SETERRQ(n,mess)

   Experienced users can set the error handler with PetscPushErrorHandler().

   Concepts: error^setting condition

.seealso: PetscTraceBackErrorHandler(), PetscPushErrorHandler(), SETERRQ(), CHKERRQ(), CHKMEMQ, SETERRQ1(), SETERRQ2()
@*/
void PETSC_DLLEXPORT PetscErrorCxx(int line,const char *func,const char* file,const char *dir,PetscErrorCode n,int p)
{
  PetscTruth ismain, isunknown;
#if 0
#if defined(PETSC_USE_ERRORCHECKING)
  PetscInt   i;
#endif
#endif

  if (!func) func = "User provided function";
  if (!file) file = "User file";
  if (!dir)  dir  = " ";

#if 0
#if defined(PETSC_USE_ERRORCHECKING)
  /* check if user is catching this exception */
  for (i=0; i<PetscExceptionsCount; i++) {
    if (n == PetscExceptions[i])  PetscFunctionReturn(n);
  }
#endif
#endif

  std::ostringstream msg;

  PetscTraceBackErrorHandlerCxx(line, func, file, dir, n, p, msg);

  /* 
      If this is called from the main() routine we call MPI_Abort() instead of 
    return to allow the parallel program to be properly shutdown.

    Since this is in the error handler we don't check the errors below. Of course,
    PetscStrncmp() does its own error checking which is problamatic
  */
  PetscStrncmp(func,"main",4,&ismain);
  PetscStrncmp(func,"unknown",7,&isunknown);
  if (ismain || isunknown) {
    MPI_Abort(PETSC_COMM_WORLD, (int) n);
  }
  throw PETSc::Exception(msg.str().c_str());
}
#endif

/* -------------------------------------------------------------------------*/

#undef __FUNCT__  
#define __FUNCT__ "PetscIntView" 
/*@C
    PetscIntView - Prints an array of integers; useful for debugging.

    Collective on PetscViewer

    Input Parameters:
+   N - number of integers in array
.   idx - array of integers
-   viewer - location to print array,  PETSC_VIEWER_STDOUT_WORLD, PETSC_VIEWER_STDOUT_SELF or 0

  Level: intermediate

    Developer Notes: idx cannot be const because may be passed to binary viewer where byte swappping is done

.seealso: PetscRealView() 
@*/
PetscErrorCode PETSC_DLLEXPORT PetscIntView(PetscInt N,const PetscInt idx[],PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscInt       j,i,n = N/20,p = N % 20;
  PetscTruth     iascii,isbinary;
  MPI_Comm       comm;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_SELF;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,3);
  if (N) PetscValidIntPointer(idx,2);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);

  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_BINARY,&isbinary);CHKERRQ(ierr);
  if (iascii) {
    for (i=0; i<n; i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%D:",20*i);CHKERRQ(ierr);
      for (j=0; j<20; j++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," %D",idx[i*20+j]);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    if (p) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%D:",20*n);CHKERRQ(ierr);
      for (i=0; i<p; i++) { ierr = PetscViewerASCIISynchronizedPrintf(viewer," %D",idx[20*n+i]);CHKERRQ(ierr);}
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    PetscMPIInt rank,size,*sizes,Ntotal,*displs, NN = PetscMPIIntCast(N);
    PetscInt    *array;
    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

    if (size > 1) {
      if (rank) {
        ierr = MPI_Gather(&NN,1,MPI_INT,0,0,MPI_INT,0,comm);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPIU_INT,0,0,0,MPIU_INT,0,comm);CHKERRQ(ierr);
      } else {
	ierr      = PetscMalloc(size*sizeof(PetscMPIInt),&sizes);CHKERRQ(ierr);
        ierr      = MPI_Gather(&NN,1,MPI_INT,sizes,1,MPIU_INT,0,comm);CHKERRQ(ierr);
        Ntotal    = sizes[0]; 
	ierr      = PetscMalloc(size*sizeof(PetscMPIInt),&displs);CHKERRQ(ierr);
        displs[0] = 0;
        for (i=1; i<size; i++) {
          Ntotal    += sizes[i];
          displs[i] =  displs[i-1] + sizes[i-1];
        }
	ierr = PetscMalloc(Ntotal*sizeof(PetscInt),&array);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPIU_INT,array,sizes,displs,MPIU_INT,0,comm);CHKERRQ(ierr);
        ierr = PetscViewerBinaryWrite(viewer,array,Ntotal,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
        ierr = PetscFree(sizes);CHKERRQ(ierr);
        ierr = PetscFree(displs);CHKERRQ(ierr);
        ierr = PetscFree(array);CHKERRQ(ierr);
      }
    } else {
      ierr = PetscViewerBinaryWrite(viewer,(void *) idx,N,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);
    }
  } else {
    const char *tname;
    ierr = PetscObjectGetName((PetscObject)viewer,&tname);CHKERRQ(ierr);
    SETERRQ1(PETSC_ERR_SUP,"Cannot handle that PetscViewer of type %s",tname);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscRealView" 
/*@C
    PetscRealView - Prints an array of doubles; useful for debugging.

    Collective on PetscViewer

    Input Parameters:
+   N - number of doubles in array
.   idx - array of doubles
-   viewer - location to print array,  PETSC_VIEWER_STDOUT_WORLD, PETSC_VIEWER_STDOUT_SELF or 0

  Level: intermediate

    Developer Notes: idx cannot be const because may be passed to binary viewer where byte swappping is done

.seealso: PetscIntView() 
@*/
PetscErrorCode PETSC_DLLEXPORT PetscRealView(PetscInt N,const PetscReal idx[],PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscInt       j,i,n = N/5,p = N % 5;
  PetscTruth     iascii,isbinary;
  MPI_Comm       comm;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_SELF;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE,3);
  PetscValidScalarPointer(idx,2);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);

  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_BINARY,&isbinary);CHKERRQ(ierr);
  if (iascii) {
    for (i=0; i<n; i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%2d:",5*i);CHKERRQ(ierr);
      for (j=0; j<5; j++) {
         ierr = PetscViewerASCIISynchronizedPrintf(viewer," %12.4e",idx[i*5+j]);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    if (p) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%2d:",5*n);CHKERRQ(ierr);
      for (i=0; i<p; i++) { PetscViewerASCIISynchronizedPrintf(viewer," %12.4e",idx[5*n+i]);CHKERRQ(ierr);}
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    PetscMPIInt rank,size,*sizes,*displs, Ntotal,NN = PetscMPIIntCast(N);
    PetscReal   *array;

    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

    if (size > 1) {
      if (rank) {
        ierr = MPI_Gather(&NN,1,MPI_INT,0,0,MPI_INT,0,comm);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPI_DOUBLE,0,0,0,MPI_DOUBLE,0,comm);CHKERRQ(ierr);
      } else {
	ierr   = PetscMalloc(size*sizeof(PetscMPIInt),&sizes);CHKERRQ(ierr);
        ierr   = MPI_Gather(&NN,1,MPI_INT,sizes,1,MPI_INT,0,comm);CHKERRQ(ierr);
        Ntotal = sizes[0]; 
	ierr   = PetscMalloc(size*sizeof(PetscMPIInt),&displs);CHKERRQ(ierr);
        displs[0] = 0;
        for (i=1; i<size; i++) {
          Ntotal    += sizes[i];
          displs[i] =  displs[i-1] + sizes[i-1];
        }
	ierr = PetscMalloc(Ntotal*sizeof(PetscReal),&array);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPI_DOUBLE,array,sizes,displs,MPI_DOUBLE,0,comm);CHKERRQ(ierr);
        ierr = PetscViewerBinaryWrite(viewer,array,Ntotal,PETSC_REAL,PETSC_TRUE);CHKERRQ(ierr);
        ierr = PetscFree(sizes);CHKERRQ(ierr);
        ierr = PetscFree(displs);CHKERRQ(ierr);
        ierr = PetscFree(array);CHKERRQ(ierr);
      }
    } else {
      ierr = PetscViewerBinaryWrite(viewer,(void *) idx,N,PETSC_REAL,PETSC_FALSE);CHKERRQ(ierr);
    }
  } else {
    const char *tname;
    ierr = PetscObjectGetName((PetscObject)viewer,&tname);CHKERRQ(ierr);
    SETERRQ1(PETSC_ERR_SUP,"Cannot handle that PetscViewer of type %s",tname);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscScalarView" 
/*@C
    PetscScalarView - Prints an array of scalars; useful for debugging.

    Collective on PetscViewer

    Input Parameters:
+   N - number of scalars in array
.   idx - array of scalars
-   viewer - location to print array,  PETSC_VIEWER_STDOUT_WORLD, PETSC_VIEWER_STDOUT_SELF or 0

  Level: intermediate

    Developer Notes: idx cannot be const because may be passed to binary viewer where byte swappping is done

.seealso: PetscIntView(), PetscRealView()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscScalarView(PetscInt N,const PetscScalar idx[],PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscInt       j,i,n = N/3,p = N % 3;
  PetscTruth     iascii,isbinary;
  MPI_Comm       comm;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_SELF;
  PetscValidHeader(viewer,3);
  PetscValidScalarPointer(idx,2);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);

  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_BINARY,&isbinary);CHKERRQ(ierr);
  if (iascii) {
    for (i=0; i<n; i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%2d:",3*i);CHKERRQ(ierr);
      for (j=0; j<3; j++) {
#if defined (PETSC_USE_COMPLEX)
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," (%12.4e,%12.4e)",
                                 PetscRealPart(idx[i*3+j]),PetscImaginaryPart(idx[i*3+j]));CHKERRQ(ierr);
#else       
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," %12.4e",idx[i*3+j]);CHKERRQ(ierr);
#endif
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    if (p) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%2d:",3*n);CHKERRQ(ierr);
      for (i=0; i<p; i++) { 
#if defined (PETSC_USE_COMPLEX)
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," (%12.4e,%12.4e)",
                                 PetscRealPart(idx[n*3+i]),PetscImaginaryPart(idx[n*3+i]));CHKERRQ(ierr);
#else
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," %12.4e",idx[3*n+i]);CHKERRQ(ierr);
#endif
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    PetscMPIInt size,rank,*sizes,Ntotal,*displs,NN = PetscMPIIntCast(N);
    PetscScalar *array;

    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

    if (size > 1) {
      if (rank) {
        ierr = MPI_Gather(&NN,1,MPI_INT,0,0,MPI_INT,0,comm);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPIU_SCALAR,0,0,0,MPIU_SCALAR,0,comm);CHKERRQ(ierr);
      } else {
	ierr   = PetscMalloc(size*sizeof(PetscMPIInt),&sizes);CHKERRQ(ierr);
        ierr   = MPI_Gather(&NN,1,MPI_INT,sizes,1,MPI_INT,0,comm);CHKERRQ(ierr);
        Ntotal = sizes[0]; 
	ierr   = PetscMalloc(size*sizeof(PetscMPIInt),&displs);CHKERRQ(ierr);
        displs[0] = 0;
        for (i=1; i<size; i++) {
          Ntotal    += sizes[i];
          displs[i] =  displs[i-1] + sizes[i-1];
        }
	ierr = PetscMalloc(Ntotal*sizeof(PetscScalar),&array);CHKERRQ(ierr);
        ierr = MPI_Gatherv((void*)idx,NN,MPIU_SCALAR,array,sizes,displs,MPIU_SCALAR,0,comm);CHKERRQ(ierr);
        ierr = PetscViewerBinaryWrite(viewer,array,Ntotal,PETSC_SCALAR,PETSC_TRUE);CHKERRQ(ierr);
        ierr = PetscFree(sizes);CHKERRQ(ierr);
        ierr = PetscFree(displs);CHKERRQ(ierr);
        ierr = PetscFree(array);CHKERRQ(ierr);
      }
    } else {
      ierr = PetscViewerBinaryWrite(viewer,(void *) idx,N,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
    }
  } else {
    const char *tname;
    ierr = PetscObjectGetName((PetscObject)viewer,&tname);CHKERRQ(ierr);
    SETERRQ1(PETSC_ERR_SUP,"Cannot handle that PetscViewer of type %s",tname);
  }
  PetscFunctionReturn(0);
}





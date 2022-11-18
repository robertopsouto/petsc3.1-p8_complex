#define PETSC_DLL

#include "../src/sys/plog/plog.h"  /*I    "petscsys.h"   I*/

/* Variables for the tracing logger */
extern FILE          *tracefile;
extern int            tracelevel;
extern const char    *traceblanks;
extern char           tracespace[128];
extern PetscLogDouble tracetime;

/*----------------------------------------------- Creation Functions -------------------------------------------------*/
/* Note: these functions do not have prototypes in a public directory, so they are considered "internal" and not exported. */

#undef __FUNCT__  
#define __FUNCT__ "EventRegLogCreate"
/*@C
  EventRegLogCreate - This creates a EventRegLog object.

  Not collective

  Input Parameter:
. eventLog - The EventRegLog

  Level: beginner

.keywords: log, event, create
.seealso: EventRegLogDestroy(), StageLogCreate()
@*/
PetscErrorCode EventRegLogCreate(EventRegLog *eventLog) 
{
  EventRegLog l;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNew(struct _n_EventRegLog, &l);CHKERRQ(ierr);
  l->numEvents   = 0;
  l->maxEvents   = 100;
  ierr = PetscMalloc(l->maxEvents * sizeof(EventRegInfo), &l->eventInfo);CHKERRQ(ierr);
  *eventLog = l;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventRegLogDestroy"
/*@C
  EventRegLogDestroy - This destroys a EventRegLog object.

  Not collective

  Input Paramter:
. eventLog - The EventRegLog

  Level: beginner

.keywords: log, event, destroy
.seealso: EventRegLogCreate()
@*/
PetscErrorCode EventRegLogDestroy(EventRegLog eventLog) 
{
  int e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  for(e = 0; e < eventLog->numEvents; e++) {
    ierr = PetscFree(eventLog->eventInfo[e].name);CHKERRQ(ierr);
  }
  ierr = PetscFree(eventLog->eventInfo);CHKERRQ(ierr);
  ierr = PetscFree(eventLog);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogCreate"
/*@C
  EventPerfLogCreate - This creates a EventPerfLog object.

  Not collective

  Input Parameter:
. eventLog - The EventPerfLog

  Level: beginner

.keywords: log, event, create
.seealso: EventPerfLogDestroy(), StageLogCreate()
@*/
PetscErrorCode EventPerfLogCreate(EventPerfLog *eventLog) 
{
  EventPerfLog l;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNew(struct _n_EventPerfLog, &l);CHKERRQ(ierr);
  l->numEvents   = 0;
  l->maxEvents   = 100;
  ierr = PetscMalloc(l->maxEvents * sizeof(EventPerfInfo), &l->eventInfo);CHKERRQ(ierr);
  *eventLog = l;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogDestroy"
/*@C
  EventPerfLogDestroy - This destroys a EventPerfLog object.

  Not collective

  Input Paramter:
. eventLog - The EventPerfLog

  Level: beginner

.keywords: log, event, destroy
.seealso: EventPerfLogCreate()
@*/
PetscErrorCode EventPerfLogDestroy(EventPerfLog eventLog) 
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(eventLog->eventInfo);CHKERRQ(ierr);
  ierr = PetscFree(eventLog);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------ General Functions -------------------------------------------------*/
#undef __FUNCT__  
#define __FUNCT__ "EventPerfInfoClear"
/*@C
  EventPerfInfoClear - This clears a EventPerfInfo object.

  Not collective

  Input Paramter:
. eventInfo - The EventPerfInfo

  Level: beginner

.keywords: log, event, destroy
.seealso: EventPerfLogCreate()
@*/
PetscErrorCode EventPerfInfoClear(EventPerfInfo *eventInfo) 
{
  PetscFunctionBegin;
  eventInfo->id            = -1;
  eventInfo->active        = PETSC_TRUE;
  eventInfo->visible       = PETSC_TRUE;
  eventInfo->depth         = 0;
  eventInfo->count         = 0;
  eventInfo->flops         = 0.0;
  eventInfo->time          = 0.0;
  eventInfo->numMessages   = 0.0;
  eventInfo->messageLength = 0.0;
  eventInfo->numReductions = 0.0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfInfoCopy"
/*@C
  EventPerfInfoCopy - Copy the activity and visibility data in eventInfo to outInfo

  Not collective

  Input Paramter:
. eventInfo - The input EventPerfInfo

  Output Paramter:
. outInfo   - The output EventPerfInfo

  Level: beginner

.keywords: log, event, copy
.seealso: EventPerfInfoClear()
@*/
PetscErrorCode EventPerfInfoCopy(EventPerfInfo *eventInfo, EventPerfInfo *outInfo) 
{
  PetscFunctionBegin;
  outInfo->id      = eventInfo->id;
  outInfo->active  = eventInfo->active;
  outInfo->visible = eventInfo->visible;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogEnsureSize"
/*@C
  EventPerfLogEnsureSize - This ensures that a EventPerfLog is at least of a certain size.

  Not collective

  Input Paramters:
+ eventLog - The EventPerfLog
- size     - The size

  Level: intermediate

.keywords: log, event, size, ensure
.seealso: EventPerfLogCreate()
@*/
PetscErrorCode EventPerfLogEnsureSize(EventPerfLog eventLog, int size) 
{
  EventPerfInfo *eventInfo;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  while(size > eventLog->maxEvents) {
    ierr = PetscMalloc(eventLog->maxEvents*2 * sizeof(EventPerfInfo), &eventInfo);CHKERRQ(ierr);
    ierr = PetscMemcpy(eventInfo, eventLog->eventInfo, eventLog->maxEvents * sizeof(EventPerfInfo));CHKERRQ(ierr);
    ierr = PetscFree(eventLog->eventInfo);CHKERRQ(ierr);
    eventLog->eventInfo  = eventInfo;
    eventLog->maxEvents *= 2;
  }
  while(eventLog->numEvents < size) {
    ierr = EventPerfInfoClear(&eventLog->eventInfo[eventLog->numEvents++]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*--------------------------------------------- Registration Functions ----------------------------------------------*/
#undef __FUNCT__  
#define __FUNCT__ "EventRegLogRegister"
/*@C
  EventRegLogRegister - Registers an event for logging operations in an application code.

  Not Collective

  Input Parameters:
+ eventLog - The EventLog
. ename    - The name associated with the event
- cookie   - The cookie associated to the class for this event

  Output Parameter:
. event    - The event

  Example of Usage:
.vb
      int USER_EVENT;
      PetscLogDouble user_event_flops;
      PetscLogEventRegister("User event name",0,&USER_EVENT);
      PetscLogEventBegin(USER_EVENT,0,0,0,0);
         [code segment to monitor]
         PetscLogFlops(user_event_flops);
      PetscLogEventEnd(USER_EVENT,0,0,0,0);
.ve

  Notes: 
  PETSc automatically logs library events if the code has been
  compiled with -DPETSC_USE_LOG (which is the default) and -log,
  -log_summary, or -log_all are specified.  PetscLogEventRegister() is
  intended for logging user events to supplement this PETSc
  information. 

  PETSc can gather data for use with the utilities Upshot/Nupshot
  (part of the MPICH distribution).  If PETSc has been compiled
  with flag -DPETSC_HAVE_MPE (MPE is an additional utility within
  MPICH), the user can employ another command line option, -log_mpe,
  to create a logfile, "mpe.log", which can be visualized
  Upshot/Nupshot.

  Level: intermediate

.keywords: log, event, register
.seealso: PetscLogEventBegin(), PetscLogEventEnd(), PetscLogFlops(), PetscLogEventMPEActivate(), PetscLogEventMPEDeactivate(),
          EventLogActivate(), EventLogDeactivate()
@*/
PetscErrorCode EventRegLogRegister(EventRegLog eventLog, const char ename[], PetscCookie cookie, PetscLogEvent *event) 
{
  EventRegInfo *eventInfo;
  char         *str;
  int           e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidCharPointer(ename,2);
  PetscValidIntPointer(event,4);
  /* Should check cookie I think */
  e = eventLog->numEvents++;
  if (eventLog->numEvents > eventLog->maxEvents) {
    ierr = PetscMalloc(eventLog->maxEvents*2 * sizeof(EventRegInfo), &eventInfo);CHKERRQ(ierr);
    ierr = PetscMemcpy(eventInfo, eventLog->eventInfo, eventLog->maxEvents * sizeof(EventRegInfo));CHKERRQ(ierr);
    ierr = PetscFree(eventLog->eventInfo);CHKERRQ(ierr);
    eventLog->eventInfo  = eventInfo;
    eventLog->maxEvents *= 2;
  }
  ierr = PetscStrallocpy(ename, &str);CHKERRQ(ierr);
  eventLog->eventInfo[e].name   = str;
  eventLog->eventInfo[e].cookie = cookie;
#if defined(PETSC_HAVE_MPE)
  if (UseMPE) {
    const char  *color;
    PetscMPIInt rank;
    int         beginID, endID;

    beginID = MPE_Log_get_event_number();
    endID   = MPE_Log_get_event_number();
    eventLog->eventInfo[e].mpe_id_begin = beginID;
    eventLog->eventInfo[e].mpe_id_end   = endID;
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);CHKERRQ(ierr);
    if (!rank) {
      ierr = PetscLogGetRGBColor(&color);CHKERRQ(ierr);
      MPE_Describe_state(beginID, endID, str, (char*)color);
    }
  }
#endif
  *event = e;
  PetscFunctionReturn(0);
}

/*---------------------------------------------- Activation Functions -----------------------------------------------*/
#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogActivate"
/*@C
  EventPerfLogActivate - Indicates that a particular event should be logged.

  Not Collective

  Input Parameters:
+ eventLog - The EventPerfLog
- event    - The event

   Usage:
.vb
      EventPerfLogDeactivate(log, VEC_SetValues);
        [code where you do not want to log VecSetValues()]
      EventPerfLogActivate(log, VEC_SetValues);
        [code where you do want to log VecSetValues()]
.ve 

  Note:
  The event may be either a pre-defined PETSc event (found in 
  include/petsclog.h) or an event number obtained with EventRegLogRegister().

  Level: advanced

.keywords: log, event, activate
.seealso: PetscLogEventMPEDeactivate(), PetscLogEventMPEActivate(), EventPerfLogDeactivate()
@*/
PetscErrorCode EventPerfLogActivate(EventPerfLog eventLog, PetscLogEvent event) 
{
  PetscFunctionBegin;
  eventLog->eventInfo[event].active = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogDeactivate"
/*@C
  EventPerfLogDeactivate - Indicates that a particular event should not be logged.

  Not Collective

  Input Parameters:
+ eventLog - The EventPerfLog
- event    - The event

   Usage:
.vb
      EventPerfLogDeactivate(log, VEC_SetValues);
        [code where you do not want to log VecSetValues()]
      EventPerfLogActivate(log, VEC_SetValues);
        [code where you do want to log VecSetValues()]
.ve 

  Note:
  The event may be either a pre-defined PETSc event (found in 
  include/petsclog.h) or an event number obtained with EventRegLogRegister().

  Level: advanced

.keywords: log, event, activate
.seealso: PetscLogEventMPEDeactivate(), PetscLogEventMPEActivate(), EventPerfLogActivate()
@*/
PetscErrorCode EventPerfLogDeactivate(EventPerfLog eventLog, PetscLogEvent event) 
{
  PetscFunctionBegin;
  eventLog->eventInfo[event].active = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogActivateClass"
/*@C
  EventPerfLogActivateClass - Activates event logging for a PETSc object class.

  Not Collective

  Input Parameters:
+ eventLog    - The EventPerfLog
. eventRegLog - The EventRegLog
- cookie      - The class id, for example MAT_COOKIE, SNES_COOKIE,

  Level: developer

.seealso: EventPerfLogDeactivateClass(), EventPerfLogActivate(), EventPerfLogDeactivate()
@*/
PetscErrorCode EventPerfLogActivateClass(EventPerfLog eventLog, EventRegLog eventRegLog, PetscCookie cookie) 
{ 
  int e;

  PetscFunctionBegin;
  for(e = 0; e < eventLog->numEvents; e++) {
    int c = eventRegLog->eventInfo[e].cookie;

    if (c == cookie) eventLog->eventInfo[e].active = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogDeactivateClass"
/*@C
  EventPerfLogDeactivateClass - Deactivates event logging for a PETSc object class.

  Not Collective

  Input Parameters:
+ eventLog    - The EventPerfLog
. eventRegLog - The EventRegLog
- cookie - The class id, for example MAT_COOKIE, SNES_COOKIE,

  Level: developer

.seealso: EventPerfLogDeactivateClass(), EventPerfLogDeactivate(), EventPerfLogActivate()
@*/
PetscErrorCode EventPerfLogDeactivateClass(EventPerfLog eventLog, EventRegLog eventRegLog, PetscCookie cookie) 
{
  int e;

  PetscFunctionBegin;
  for(e = 0; e < eventLog->numEvents; e++) {
    int c = eventRegLog->eventInfo[e].cookie;

    if (c == cookie) eventLog->eventInfo[e].active = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

/*------------------------------------------------ Query Functions --------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "EventRegLogGetEvent"
/*@
  EventRegLogGetEvent - This function returns the event id given the event name.

  Not Collective

  Input Parameters:
+ eventLog - The EventRegLog
- name     - The stage name

  Output Parameter:
. event    - The event id

  Level: intermediate

.keywords: log, stage
.seealso: EventRegLogRegister()
@*/
PetscErrorCode PETSC_DLLEXPORT EventRegLogGetEvent(EventRegLog eventLog, const char name[], PetscLogEvent *event)
{
  PetscTruth match;
  int        e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidCharPointer(name,2);
  PetscValidIntPointer(event,3);
  *event = -1;
  for(e = 0; e < eventLog->numEvents; e++) {
    ierr = PetscStrcasecmp(eventLog->eventInfo[e].name, name, &match);CHKERRQ(ierr);
    if (match) break;
  }
  if (e == eventLog->numEvents) SETERRQ1(PETSC_ERR_ARG_WRONG, "No event named %s", name);
  *event = e;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogSetVisible"
/*@C
  EventPerfLogSetVisible - This function determines whether an event is printed during PetscLogPrintSummary()

  Not Collective

  Input Parameters:
+ eventLog  - The EventPerfLog
. event     - The event to log
- isVisible - The visibility flag, PETSC_TRUE for printing, otherwise PETSC_FALSE (default is PETSC_TRUE)

  Database Options:
. -log_summary - Activates log summary

  Level: intermediate

.keywords: log, visible, event
.seealso: EventPerfLogGetVisible(), EventRegLogRegister(), StageLogGetEventLog()
@*/
PetscErrorCode EventPerfLogSetVisible(EventPerfLog eventLog, PetscLogEvent event, PetscTruth isVisible) 
{
  PetscFunctionBegin;
  eventLog->eventInfo[event].visible = isVisible;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "EventPerfLogGetVisible"
/*@C
  EventPerfLogGetVisible - This function returns whether an event is printed during PetscLogPrintSummary()

  Not Collective

  Input Parameters:
+ eventLog  - The EventPerfLog
- event     - The event id to log

  Output Parameter:
. isVisible - The visibility flag, PETSC_TRUE for printing, otherwise PETSC_FALSE (default is PETSC_TRUE)

  Database Options:
. -log_summary - Activates log summary

  Level: intermediate

.keywords: log, visible, event
.seealso: EventPerfLogSetVisible(), EventRegLogRegister(), StageLogGetEventLog()
@*/
PetscErrorCode EventPerfLogGetVisible(EventPerfLog eventLog, PetscLogEvent event, PetscTruth *isVisible) 
{
  PetscFunctionBegin;
  PetscValidIntPointer(isVisible,3);
  *isVisible = eventLog->eventInfo[event].visible;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventGetFlops"
PetscErrorCode PetscLogEventGetFlops(PetscLogEvent event, PetscLogDouble *flops)
{
  StageLog       stageLog;
  EventPerfLog   eventLog;
  int            stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventLog);CHKERRQ(ierr);
  *flops = eventLog->eventInfo[event].flops;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventZeroFlops"
PetscErrorCode PetscLogEventZeroFlops(PetscLogEvent event)
{
  StageLog       stageLog;
  EventPerfLog   eventLog;
  int            stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventLog);CHKERRQ(ierr);
  eventLog->eventInfo[event].flops = 0.0;
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_CHUD)
#include <CHUD/CHUD.h>
#endif
#if defined(PETSC_HAVE_PAPI)
#include "papi.h"
extern int PAPIEventSet;
#endif

/*------------------------------------------------ Action Functions -------------------------------------------------*/
#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventBeginDefault"
PetscErrorCode PetscLogEventBeginDefault(PetscLogEvent event, int t, PetscObject o1, PetscObject o2, PetscObject o3, PetscObject o4) 
{
  StageLog       stageLog;
  EventPerfLog   eventLog;
  int            stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventLog);CHKERRQ(ierr);
  /* Check for double counting */
  eventLog->eventInfo[event].depth++;
  if (eventLog->eventInfo[event].depth > 1) PetscFunctionReturn(0);
  /* Log performance info */
  eventLog->eventInfo[event].count++;
  PetscTimeSubtract(eventLog->eventInfo[event].time);
#if defined(PETSC_HAVE_CHUD)
  eventLog->eventInfo[event].flops         -= chudGetPMCEventCount(chudCPU1Dev,PMC_1);
#elif defined(PETSC_HAVE_PAPI)
  { long_long values[2];
    ierr = PAPI_read(PAPIEventSet,values);CHKERRQ(ierr);
    eventLog->eventInfo[event].flops -= values[0];
    /*    printf("fma %g flops %g\n",(double)values[1],(double)values[0]); */
  }
#else
  eventLog->eventInfo[event].flops         -= _TotalFlops;
#endif
  eventLog->eventInfo[event].numMessages   -= irecv_ct  + isend_ct  + recv_ct  + send_ct;
  eventLog->eventInfo[event].messageLength -= irecv_len + isend_len + recv_len + send_len;
  eventLog->eventInfo[event].numReductions -= allreduce_ct;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventEndDefault"
PetscErrorCode PetscLogEventEndDefault(PetscLogEvent event, int t, PetscObject o1, PetscObject o2, PetscObject o3, PetscObject o4)
{
  StageLog     stageLog;
  EventPerfLog eventLog;
  int          stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventLog);CHKERRQ(ierr);
  /* Check for double counting */
  eventLog->eventInfo[event].depth--;
  if (eventLog->eventInfo[event].depth > 0) {
    PetscFunctionReturn(0);
  } else if (eventLog->eventInfo[event].depth < 0) {
    SETERRQ(PETSC_ERR_ARG_WRONGSTATE, "Logging event had unbalanced begin/end pairs");
  }
  /* Log performance info */
  PetscTimeAdd(eventLog->eventInfo[event].time);
#if defined(PETSC_HAVE_CHUD)
  eventLog->eventInfo[event].flops         += chudGetPMCEventCount(chudCPU1Dev,PMC_1);
#elif defined(PETSC_HAVE_PAPI)
  { long_long values[2];
    ierr = PAPI_read(PAPIEventSet,values);CHKERRQ(ierr);
    eventLog->eventInfo[event].flops += values[0];
    /* printf("fma %g flops %g\n",(double)values[1],(double)values[0]); */
  }
#else
  eventLog->eventInfo[event].flops         += _TotalFlops;
#endif
  eventLog->eventInfo[event].numMessages   += irecv_ct  + isend_ct  + recv_ct  + send_ct;
  eventLog->eventInfo[event].messageLength += irecv_len + isend_len + recv_len + send_len;
  eventLog->eventInfo[event].numReductions += allreduce_ct;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventBeginComplete"
PetscErrorCode PetscLogEventBeginComplete(PetscLogEvent event, int t, PetscObject o1, PetscObject o2, PetscObject o3, PetscObject o4) 
{
  StageLog       stageLog;
  EventRegLog    eventRegLog;
  EventPerfLog   eventPerfLog;
  Action        *tmpAction;
  PetscLogDouble start, end;
  PetscLogDouble curTime;
  int            stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Dynamically enlarge logging structures */
  if (numActions >= maxActions) {
    PetscTime(start);
    ierr = PetscMalloc(maxActions*2 * sizeof(Action), &tmpAction);CHKERRQ(ierr);
    ierr = PetscMemcpy(tmpAction, actions, maxActions * sizeof(Action));CHKERRQ(ierr);
    ierr = PetscFree(actions);CHKERRQ(ierr);
    actions     = tmpAction;
    maxActions *= 2;
    PetscTime(end);
    BaseTime += (end - start);
  }
  /* Record the event */
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventRegLog(stageLog, &eventRegLog);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventPerfLog);CHKERRQ(ierr);
  PetscTime(curTime);
  if (logActions) {
    actions[numActions].time   = curTime - BaseTime;
    actions[numActions].action = ACTIONBEGIN;
    actions[numActions].event  = event;
    actions[numActions].cookie = eventRegLog->eventInfo[event].cookie;
    if (o1) actions[numActions].id1 = o1->id; else actions[numActions].id1 = -1;
    if (o2) actions[numActions].id2 = o2->id; else actions[numActions].id2 = -1;
    if (o3) actions[numActions].id3 = o3->id; else actions[numActions].id3 = -1;
    actions[numActions].flops    = _TotalFlops;
    ierr = PetscMallocGetCurrentUsage(&actions[numActions].mem);CHKERRQ(ierr);
    ierr = PetscMallocGetMaximumUsage(&actions[numActions].maxmem);CHKERRQ(ierr);
    numActions++;
  }
  /* Check for double counting */
  eventPerfLog->eventInfo[event].depth++;
  if (eventPerfLog->eventInfo[event].depth > 1) PetscFunctionReturn(0);
  /* Log the performance info */
  eventPerfLog->eventInfo[event].count++;
  eventPerfLog->eventInfo[event].time          -= curTime;
  eventPerfLog->eventInfo[event].flops         -= _TotalFlops;
  eventPerfLog->eventInfo[event].numMessages   -= irecv_ct  + isend_ct  + recv_ct  + send_ct;
  eventPerfLog->eventInfo[event].messageLength -= irecv_len + isend_len + recv_len + send_len;
  eventPerfLog->eventInfo[event].numReductions -= allreduce_ct;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventEndComplete"
PetscErrorCode PetscLogEventEndComplete(PetscLogEvent event, int t, PetscObject o1, PetscObject o2, PetscObject o3, PetscObject o4) 
{
  StageLog       stageLog;
  EventRegLog    eventRegLog;
  EventPerfLog   eventPerfLog;
  Action        *tmpAction;
  PetscLogDouble start, end;
  PetscLogDouble curTime;
  int            stage;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Dynamically enlarge logging structures */
  if (numActions >= maxActions) {
    PetscTime(start);
    ierr = PetscMalloc(maxActions*2 * sizeof(Action), &tmpAction);CHKERRQ(ierr);
    ierr = PetscMemcpy(tmpAction, actions, maxActions * sizeof(Action));CHKERRQ(ierr);
    ierr = PetscFree(actions);CHKERRQ(ierr);
    actions     = tmpAction;
    maxActions *= 2;
    PetscTime(end);
    BaseTime += (end - start);
  }
  /* Record the event */
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventRegLog(stageLog, &eventRegLog);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventPerfLog);CHKERRQ(ierr);
  PetscTime(curTime);
  if (logActions) {
    actions[numActions].time   = curTime - BaseTime;
    actions[numActions].action = ACTIONEND;
    actions[numActions].event  = event;
    actions[numActions].cookie = eventRegLog->eventInfo[event].cookie;
    if (o1) actions[numActions].id1 = o1->id; else actions[numActions].id1 = -1;
    if (o2) actions[numActions].id2 = o2->id; else actions[numActions].id2 = -1;
    if (o3) actions[numActions].id3 = o3->id; else actions[numActions].id3 = -1;
    actions[numActions].flops    = _TotalFlops;
    ierr = PetscMallocGetCurrentUsage(&actions[numActions].mem);CHKERRQ(ierr);
    ierr = PetscMallocGetMaximumUsage(&actions[numActions].maxmem);CHKERRQ(ierr);
    numActions++;
  }
  /* Check for double counting */
  eventPerfLog->eventInfo[event].depth--;
  if (eventPerfLog->eventInfo[event].depth > 0) {
    PetscFunctionReturn(0);
  } else if (eventPerfLog->eventInfo[event].depth < 0) {
    SETERRQ(PETSC_ERR_ARG_WRONGSTATE, "Logging event had unbalanced begin/end pairs");
  }
  /* Log the performance info */
  eventPerfLog->eventInfo[event].count++;
  eventPerfLog->eventInfo[event].time          += curTime;
  eventPerfLog->eventInfo[event].flops         += _TotalFlops;
  eventPerfLog->eventInfo[event].numMessages   += irecv_ct  + isend_ct  + recv_ct  + send_ct;
  eventPerfLog->eventInfo[event].messageLength += irecv_len + isend_len + recv_len + send_len;
  eventPerfLog->eventInfo[event].numReductions += allreduce_ct;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventBeginTrace"
PetscErrorCode PetscLogEventBeginTrace(PetscLogEvent event, int t, PetscObject o1, PetscObject o2, PetscObject o3, PetscObject o4) 
{
  StageLog       stageLog;
  EventRegLog    eventRegLog;
  EventPerfLog   eventPerfLog;
  PetscLogDouble cur_time;
  PetscMPIInt    rank;
  int            stage,err;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!tracetime) {PetscTime(tracetime);}

  ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);CHKERRQ(ierr);
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventRegLog(stageLog, &eventRegLog);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventPerfLog);CHKERRQ(ierr);
  /* Check for double counting */
  eventPerfLog->eventInfo[event].depth++;
  tracelevel++;
  if (eventPerfLog->eventInfo[event].depth > 1) PetscFunctionReturn(0);
  /* Log performance info */
  ierr = PetscStrncpy(tracespace, traceblanks, 2*tracelevel);CHKERRQ(ierr);
  tracespace[2*tracelevel] = 0;
  PetscTime(cur_time);
  ierr = PetscFPrintf(PETSC_COMM_SELF,tracefile, "%s[%d] %g Event begin: %s\n", tracespace, rank, cur_time-tracetime, eventRegLog->eventInfo[event].name);CHKERRQ(ierr);
  err = fflush(tracefile);
  if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscLogEventEndTrace"
PetscErrorCode PetscLogEventEndTrace(PetscLogEvent event,int t,PetscObject o1,PetscObject o2,PetscObject o3,PetscObject o4) 
{
  StageLog       stageLog;
  EventRegLog    eventRegLog;
  EventPerfLog   eventPerfLog;
  PetscLogDouble cur_time;
  int            stage,err;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  tracelevel--;
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);CHKERRQ(ierr);
  ierr = PetscLogGetStageLog(&stageLog);CHKERRQ(ierr);
  ierr = StageLogGetCurrent(stageLog, &stage);CHKERRQ(ierr);
  ierr = StageLogGetEventRegLog(stageLog, &eventRegLog);CHKERRQ(ierr);
  ierr = StageLogGetEventPerfLog(stageLog, stage, &eventPerfLog);CHKERRQ(ierr);
  /* Check for double counting */
  eventPerfLog->eventInfo[event].depth--;
  if (eventPerfLog->eventInfo[event].depth > 0) {
    PetscFunctionReturn(0);
  } else if (eventPerfLog->eventInfo[event].depth < 0 || tracelevel < 0) {
    SETERRQ(PETSC_ERR_ARG_WRONGSTATE, "Logging event had unbalanced begin/end pairs");
  }
  /* Log performance info */
  ierr = PetscStrncpy(tracespace, traceblanks, 2*tracelevel);CHKERRQ(ierr);
  tracespace[2*tracelevel] = 0;
  PetscTime(cur_time);
  ierr = PetscFPrintf(PETSC_COMM_SELF,tracefile, "%s[%d] %g Event end: %s\n", tracespace, rank, cur_time-tracetime, eventRegLog->eventInfo[event].name);CHKERRQ(ierr);
  err = fflush(tracefile);
  if (err) SETERRQ(PETSC_ERR_SYS,"fflush() failed on file");        
  PetscFunctionReturn(0);
}

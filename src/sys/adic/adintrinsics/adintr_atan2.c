/*
  macro expansion:
  function_driver -> adintr_atan2
  exception number -> ADINTR_ATAN2
  exceptional code -> 

  */

#include <stdarg.h>
#include "adintrinsics.h"
#include "knr-compat.h"
#if defined(__cplusplus)
extern "C" {
#endif

/* #include "report-once.h" */
void reportonce_accumulate Proto((int,int,int));


/* The fy must be on a line by itself to be removed for funcs like sin(x). */
void
adintr_atan2 (int deriv_order, int file_number, int line_number,
		 double*fx, double*fy, ...)
{
     /* Hack to make assignments to (*fxx) et alia OK, regardless */
     double scratch;
     double *fxx = &scratch;
     double *fxy = &scratch;
     double *fyy = &scratch;

     const int exception = ADINTR_ATAN2;

     va_list argptr;
     va_start(argptr,fy);

     if (deriv_order == 2)
     {
	  fxx = va_arg(argptr, double *);
	  fxy = va_arg(argptr, double *);
	  fyy = va_arg(argptr, double *);
     }

     /* Here is where exceptional partials should be set. */
     *fx = ADIntr_Partials[ADINTR_ATAN2][ADINTR_FX];
     *fy = ADIntr_Partials[ADINTR_ATAN2][ADINTR_FY];
     *fxx = ADIntr_Partials[ADINTR_ATAN2][ADINTR_FXX];
     *fxy = ADIntr_Partials[ADINTR_ATAN2][ADINTR_FXY];
     *fyy = ADIntr_Partials[ADINTR_ATAN2][ADINTR_FYY];


     /* Here is where we perform the action appropriate to the current mode. */
     if (ADIntr_Mode == ADINTR_REPORTONCE)
     {
	  reportonce_accumulate(file_number, line_number, exception);
     }
     
     va_end(argptr);
}
#if defined(__cplusplus)
}
#endif


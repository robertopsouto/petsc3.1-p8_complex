#include "private/fortranimpl.h"
#include "petscksp.h"

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define pcbjacobigetsubksp_        PCBJACOBIGETSUBKSP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define pcbjacobigetsubksp_        pcbjacobigetsubksp
#endif

EXTERN_C_BEGIN
void PETSC_STDCALL pcbjacobigetsubksp_(PC *pc,PetscInt *n_local,PetscInt *first_local,KSP *ksp,PetscErrorCode *ierr)
{
  KSP *tksp;
  PetscInt  i,nloc;
  CHKFORTRANNULLINTEGER(n_local);
  CHKFORTRANNULLINTEGER(first_local);
  *ierr = PCBJacobiGetSubKSP(*pc,&nloc,first_local,&tksp);
  if (n_local) *n_local = nloc;
  CHKFORTRANNULLOBJECT(ksp);
  if (ksp) {
    for (i=0; i<nloc; i++){
      ksp[i] = tksp[i];
    }
  }
}

EXTERN_C_END

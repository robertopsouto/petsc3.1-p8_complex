#include "f90impl.h"

/*************************************************************************/

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define f90array1dcreatescalar_           F90ARRAY1DCREATESCALAR
#define f90array1daccessscalar_           F90ARRAY1DACCESSSCALAR
#define f90array1ddestroyscalar_          F90ARRAY1DDESTROYSCALAR
#define f90array1dcreatereal_             F90ARRAY1DCREATEREAL
#define f90array1daccessreal_             F90ARRAY1DACCESSREAL
#define f90array1ddestroyreal_            F90ARRAY1DDESTROYREAL
#define f90array1dcreateint_              F90ARRAY1DCREATEINT
#define f90array1daccessint_              F90ARRAY1DACCESSINT
#define f90array1ddestroyint_             F90ARRAY1DDESTROYINT
#define f90array1dcreatefortranaddr_      F90ARRAY1DCREATEFORTRANADDR
#define f90array1daccessfortranaddr_      F90ARRAY1DACCESSFORTRANADDR
#define f90array1ddestroyfortranaddr_     F90ARRAY1DDESTROYFORTRANADDR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define f90array1dcreatescalar_           f90array1dcreatescalar
#define f90array1daccessscalar_           f90array1daccessscalar
#define f90array1ddestroyscalar_          f90array1ddestroyscalar
#define f90array1dcreatereal_             f90array1dcreatereal
#define f90array1daccessreal_             f90array1daccessreal
#define f90array1ddestroyreal_            f90array1ddestroyreal
#define f90array1dcreateint_              f90array1dcreateint
#define f90array1daccessint_              f90array1daccessint
#define f90array1ddestroyint_             f90array1ddestroyint
#define f90array1dcreatefortranaddr_      f90array1dcreatefortranaddr
#define f90array1daccessfortranaddr_      f90array1daccessfortranaddr
#define f90array1ddestroyfortranaddr_     f90array1ddestroyfortranaddr
#endif

EXTERN_C_BEGIN
extern void PETSC_STDCALL f90array1dcreatescalar_(void *,PetscInt *,PetscInt *,F90Array1d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1daccessscalar_(F90Array1d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1ddestroyscalar_(F90Array1d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1dcreatereal_(void *,PetscInt *,PetscInt *,F90Array1d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1daccessreal_(F90Array1d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1ddestroyreal_(F90Array1d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1dcreateint_(void *,PetscInt *,PetscInt *,F90Array1d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1daccessint_(F90Array1d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1ddestroyint_(F90Array1d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1dcreatefortranaddr_(void *,PetscInt *,PetscInt *,F90Array1d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1daccessfortranaddr_(F90Array1d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array1ddestroyfortranaddr_(F90Array1d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
EXTERN_C_END

#undef __FUNCT__
#define __FUNCT__ "F90Array1dCreate"
PetscErrorCode F90Array1dCreate(void *array,PetscDataType type,PetscInt start,PetscInt len,F90Array1d *ptr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    if (!len) array = PETSC_NULL_SCALAR_Fortran;
    f90array1dcreatescalar_(array,&start,&len,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    if (!len) array = PETSC_NULL_REAL_Fortran;
    f90array1dcreatereal_(array,&start,&len,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    if (!len) array = PETSC_NULL_INTEGER_Fortran;
    f90array1dcreateint_(array,&start,&len,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array1dcreatefortranaddr_(array,&start,&len,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "F90Array1dAccess"
PetscErrorCode PETSC_DLLEXPORT F90Array1dAccess(F90Array1d *ptr,PetscDataType type,void **array PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    f90array1daccessscalar_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    f90array1daccessreal_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    f90array1daccessint_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array1daccessfortranaddr_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "F90Array1dDestroy"
PetscErrorCode PETSC_DLLEXPORT F90Array1dDestroy(F90Array1d *ptr,PetscDataType type PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    f90array1ddestroyscalar_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    f90array1ddestroyreal_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    f90array1ddestroyint_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array1ddestroyfortranaddr_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

/*************************************************************************/

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define f90array2dcreatescalar_           F90ARRAY2DCREATESCALAR
#define f90array2daccessscalar_           F90ARRAY2DACCESSSCALAR
#define f90array2ddestroyscalar_          F90ARRAY2DDESTROYSCALAR
#define f90array2dcreatereal_             F90ARRAY2DCREATEREAL
#define f90array2daccessreal_             F90ARRAY2DACCESSREAL
#define f90array2ddestroyreal_            F90ARRAY2DDESTROYREAL
#define f90array2dcreateint_              F90ARRAY2DCREATEINT
#define f90array2daccessint_              F90ARRAY2DACCESSINT
#define f90array2ddestroyint_             F90ARRAY2DDESTROYINT
#define f90array2dcreatefortranaddr_      F90ARRAY2DCREATEFORTRANADDR
#define f90array2daccessfortranaddr_      F90ARRAY2DACCESSFORTRANADDR
#define f90array2ddestroyfortranaddr_     F90ARRAY2DDESTROYFORTRANADDR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define f90array2dcreatescalar_           f90array2dcreatescalar
#define f90array2daccessscalar_           f90array2daccessscalar
#define f90array2ddestroyscalar_          f90array2ddestroyscalar
#define f90array2dcreatereal_             f90array2dcreatereal
#define f90array2daccessreal_             f90array2daccessreal
#define f90array2ddestroyreal_            f90array2ddestroyreal
#define f90array2dcreateint_              f90array2dcreateint
#define f90array2daccessint_              f90array2daccessint
#define f90array2ddestroyint_             f90array2ddestroyint
#define f90array2dcreatefortranaddr_      f90array2dcreatefortranaddr
#define f90array2daccessfortranaddr_      f90array2daccessfortranaddr
#define f90array2ddestroyfortranaddr_     f90array2ddestroyfortranaddr
#endif

EXTERN_C_BEGIN
extern void PETSC_STDCALL f90array2dcreatescalar_(void *,PetscInt *,PetscInt *,PetscInt *,PetscInt *,F90Array2d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2daccessscalar_(F90Array2d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2ddestroyscalar_(F90Array2d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2dcreatereal_(void *,PetscInt *,PetscInt *,PetscInt *,PetscInt *,F90Array2d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2daccessreal_(F90Array2d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2ddestroyreal_(F90Array2d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2dcreateint_(void *,PetscInt *,PetscInt *,PetscInt *,PetscInt *,F90Array2d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2daccessint_(F90Array2d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2ddestroyint_(F90Array2d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2dcreatefortranaddr_(void *,PetscInt *,PetscInt *,PetscInt *,PetscInt *,F90Array2d * PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2daccessfortranaddr_(F90Array2d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
extern void PETSC_STDCALL f90array2ddestroyfortranaddr_(F90Array2d *ptr PETSC_F90_2PTR_PROTO_NOVAR);
EXTERN_C_END

#undef __FUNCT__
#define __FUNCT__ "F90Array2dCreate"
PetscErrorCode F90Array2dCreate(void *array,PetscDataType type,PetscInt start1,PetscInt len1,PetscInt start2,PetscInt len2,F90Array2d *ptr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    f90array2dcreatescalar_(array,&start1,&len1,&start2,&len2,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    f90array2dcreatereal_(array,&start1,&len1,&start2,&len2,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    f90array2dcreateint_(array,&start1,&len1,&start2,&len2,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array2dcreatefortranaddr_(array,&start1,&len1,&start2,&len2,ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "F90Array2dAccess"
PetscErrorCode PETSC_DLLEXPORT F90Array2dAccess(F90Array2d *ptr,PetscDataType type,void **array PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    f90array2daccessscalar_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    f90array2daccessreal_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    f90array2daccessint_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array2daccessfortranaddr_(ptr,array PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "F90Array2dDestroy"
PetscErrorCode PETSC_DLLEXPORT F90Array2dDestroy(F90Array2d *ptr,PetscDataType type PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFunctionBegin;
  if (type == PETSC_SCALAR) {
    f90array2ddestroyscalar_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_REAL) {
    f90array2ddestroyreal_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_INT) {
    f90array2ddestroyint_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else if (type == PETSC_FORTRANADDR) {
    f90array2ddestroyfortranaddr_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  } else {
    SETERRQ1(PETSC_ERR_SUP,"unsupported PetscDataType: %d",(PetscInt)type);
  }
  PetscFunctionReturn(0);
}

/*************************************************************************/
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define f90array1dgetaddrscalar_            F90ARRAY1DGETADDRSCALAR
#define f90array1dgetaddrreal_              F90ARRAY1DGETADDRREAL
#define f90array1dgetaddrint_               F90ARRAY1DGETADDRINT
#define f90array1dgetaddrfortranaddr_       F90ARRAY1DGETADDRFORTRANADDR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define f90array1dgetaddrscalar_            f90array1dgetaddrscalar
#define f90array1dgetaddrreal_              f90array1dgetaddrreal
#define f90array1dgetaddrint_               f90array1dgetaddrint
#define f90array1dgetaddrfortranaddr_       f90array1dgetaddrfortranaddr
#endif

EXTERN_C_BEGIN
void PETSC_STDCALL f90array1dgetaddrscalar_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array1dgetaddrreal_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array1dgetaddrint_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array1dgetaddrfortranaddr_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
EXTERN_C_END

/*************************************************************************/
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define f90array2dgetaddrscalar_            F90ARRAY2DGETADDRSCALAR
#define f90array2dgetaddrreal_              F90ARRAY2DGETADDRREAL
#define f90array2dgetaddrint_               F90ARRAY2DGETADDRINT
#define f90array2dgetaddrfortranaddr_       F90ARRAY2DGETADDRFORTRANADDR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define f90array2dgetaddrscalar_            f90array2dgetaddrscalar
#define f90array2dgetaddrreal_              f90array2dgetaddrreal
#define f90array2dgetaddrint_               f90array2dgetaddrint
#define f90array2dgetaddrfortranaddr_       f90array2dgetaddrfortranaddr
#endif

EXTERN_C_BEGIN
void PETSC_STDCALL f90array2dgetaddrscalar_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array2dgetaddrreal_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array2dgetaddrint_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
void PETSC_STDCALL f90array2dgetaddrfortranaddr_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}
EXTERN_C_END

/*************************************************************************/

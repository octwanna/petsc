#ifdef PETSC_RCS_HEADER
"$Id: petscconf.h,v 1.34 2000/11/28 17:26:31 bsmith Exp $"
"Defines the configuration for this machine"
#endif

#if !defined(INCLUDED_PETSCCONF_H)
#define INCLUDED_PETSCCONF_H
 
#define PARCH_IRIX64 
#define PETSC_ARCH_NAME "IRIX64"

#define HAVE_POPEN
#define HAVE_LIMITS_H
#define HAVE_PWD_H 
#define HAVE_STRING_H 
#define HAVE_STROPTS_H 
#define HAVE_MALLOC_H 
#define HAVE_DRAND48 
#define HAVE_GETDOMAINNAME 
#define HAVE_UNAME 
#define HAVE_UNISTD_H 
#define HAVE_STDLIB_H
#define HAVE_SYS_TIME_H 
#define HAVE_SYS_UTSNAME_H
#define PETSC_USE_SHARED_MEMORY
#define HAVE_GETCWD

#define PETSC_HAVE_FORTRAN_UNDERSCORE 
#define SIZEOF_VOID_P 8
#define SIZEOF_INT 4
#define SIZEOF_DOUBLE 8
#define BITS_PER_BYTE 8

#define WORDS_BIGENDIAN 1

#define HAVE_MEMMOVE

#define PETSC_HAVE_DOUBLE_ALIGN
#define PETSC_HAVE_DOUBLE_ALIGN_MALLOC

#define HAVE_MEMALIGN

#define PETSC_HAVE_FAST_MPI_WTIME

#define PETSC_USE_DBX_DEBUGGER
#define HAVE_SYS_RESOURCE_H

#define PETSC_HAVE_RTLD_GLOBAL 1

#define PETSC_CAN_SLEEP_AFTER_ERROR

#define PETSC_HAVE_4ARG_SIGNAL_HANDLER

#define PETSC_USE_KBYTES_FOR_SIZE
#define PETSC_USE_P_FOR_DEBUGGER

#define PETSC_HAVE_F90_H "f90impl/f90_IRIX.h"
#define PETSC_HAVE_F90_C "src/sys/src/f90/f90_IRIX.c"

#if defined(__cplusplus)
#define PETSC_SIGNAL_CAST (void (*)(int))
#endif
#define PETSC_HAVE_IRIX_STYLE_FPTRAP

#endif

#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: mpiu.c,v 1.85 1998/04/28 15:19:23 bsmith Exp bsmith $";
#endif

#include "petsc.h"        
#include "sys.h"             /*I    "sys.h"   I*/
#include <stdarg.h>
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#include "pinclude/petscfix.h"

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseBegin_Private" 
int PetscSequentialPhaseBegin_Private(MPI_Comm comm,int ng )
{
  int        lidx, np, tag = 0,ierr;
  MPI_Status status;

  PetscFunctionBegin;
  MPI_Comm_size( comm, &np );
  if (np == 1) PetscFunctionReturn(0);
  MPI_Comm_rank( comm, &lidx );
  if (lidx != 0) {
    ierr = MPI_Recv( 0, 0, MPI_INT, lidx-1, tag, comm, &status );CHKERRQ(ierr);
  }
  /* Send to the next process in the group unless we are the last process */ 
  if ((lidx % ng) < ng - 1 && lidx != np - 1) {
    ierr = MPI_Send( 0, 0, MPI_INT, lidx + 1, tag, comm );CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseEnd_Private" 
int PetscSequentialPhaseEnd_Private(MPI_Comm comm,int ng )
{
  int        lidx, np, tag = 0,ierr;
  MPI_Status status;

  PetscFunctionBegin;
  MPI_Comm_rank( comm, &lidx );
  MPI_Comm_size( comm, &np );
  if (np == 1) PetscFunctionReturn(0);

  /* Send to the first process in the next group */
  if ((lidx % ng) == ng - 1 || lidx == np - 1) {
    ierr = MPI_Send( 0, 0, MPI_INT, (lidx + 1) % np, tag, comm );CHKERRQ(ierr);
  }
  if (lidx == 0) {
    ierr = MPI_Recv( 0, 0, MPI_INT, np-1, tag, comm, &status );CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------------------*/
/*
    The variable Petsc_Seq_keyval is used to indicate an MPI attribute that
  is attached to a communicator that manages the sequential phase code below.
*/
static int Petsc_Seq_keyval = MPI_KEYVAL_INVALID;

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseBegin" 
/*@C
   PetscSequentialPhaseBegin - Begins a sequential section of code.  

   Collective on MPI_Comm

   Input Parameters:
+  comm - Communicator to sequentialize.  
-  ng   - Number in processor group.  This many processes are allowed to execute
   at the same time (usually 1)

   Notes:
   PetscSequentialPhaseBegin() and PetscSequentialPhaseEnd() provide a
   way to force a section of code to be executed by the processes in
   rank order.  Typically, this is done with
.vb
      PetscSequentialPhaseBegin( comm, 1 );
      <code to be executed sequentially>
      PetscSequentialPhaseEnd( comm, 1 );
.ve

   Often, the sequential code contains output statements (e.g., printf) to
   be executed.  Note that you may need to flush the I/O buffers before
   calling PetscSequentialPhaseEnd().  Also, note that some systems do
   not propagate I/O in any order to the controling terminal (in other words, 
   even if you flush the output, you may not get the data in the order
   that you want).

.seealso: PetscSequentialPhaseEnd()

.keywords: sequential, phase, begin
@*/
int PetscSequentialPhaseBegin(MPI_Comm comm,int ng )
{
  int        ierr, np;
  MPI_Comm   local_comm,*addr_local_comm;

  PetscFunctionBegin;
  MPI_Comm_size( comm, &np );
  if (np == 1) PetscFunctionReturn(0);

  /* Get the private communicator for the sequential operations */
  if (Petsc_Seq_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,MPI_NULL_DELETE_FN,&Petsc_Seq_keyval,0);CHKERRQ(ierr);
  }

  ierr = MPI_Comm_dup( comm, &local_comm );CHKERRQ(ierr);
  addr_local_comm  = (MPI_Comm *) PetscMalloc(sizeof(MPI_Comm));CHKPTRQ(addr_local_comm);
  *addr_local_comm = local_comm;
  ierr = MPI_Attr_put( comm, Petsc_Seq_keyval, (void *) addr_local_comm );CHKERRQ(ierr);
  ierr = PetscSequentialPhaseBegin_Private(local_comm,ng); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscSequentialPhaseEnd" 
/*@C
   PetscSequentialPhaseEnd - Ends a sequential section of code.

   Collective on MPI_Comm

   Input Parameters:
+  comm - Communicator to sequentialize.  
-  ng   - Number in processor group.  This many processes are allowed to execute
   at the same time (usually 1)

   Notes:
   See PetscSequentialPhaseBegin() for more details.

.seealso: PetscSequentialPhaseBegin()

.keywords: sequential, phase, end
@*/
int PetscSequentialPhaseEnd(MPI_Comm comm,int ng )
{
  int        ierr, np, flag;
  MPI_Comm   local_comm,*addr_local_comm;

  PetscFunctionBegin;
  MPI_Comm_size( comm, &np );
  if (np == 1) PetscFunctionReturn(0);

  ierr = MPI_Attr_get( comm, Petsc_Seq_keyval, (void **)&addr_local_comm, &flag );CHKERRQ(ierr);
  if (!flag) MPI_Abort( comm, MPI_ERR_UNKNOWN );
  local_comm = *addr_local_comm;

  ierr = PetscSequentialPhaseEnd_Private(local_comm,ng); CHKERRQ(ierr);

  PetscFree(addr_local_comm); 
  ierr = MPI_Comm_free(&local_comm);CHKERRQ(ierr);
  ierr = MPI_Attr_delete(comm,Petsc_Seq_keyval);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#ifndef lint
static char vcid[] = "$Id: zerodiag.c,v 1.10 1996/12/16 22:40:46 balay Exp balay $";
#endif

/*
    This file contains routines to reorder a matrix so that the diagonal
    elements are nonzero.
 */

#include "src/mat/matimpl.h"       /*I  "mat.h"  I*/
#include <math.h>

#define SWAP(a,b) {int _t; _t = a; a = b; b = _t; }

#undef __FUNCTION__  
#define __FUNCTION__ "MatZeroFindPre_Private"
/* Given a current row and current permutation, find a column permutation
   that removes a zero diagonal */
int MatZeroFindPre_Private(Mat mat,int prow,int* row,int* col,double repla,
                           double atol,int* rc,double* rcv )
{
  int      k, nz, repl, *j, kk, nnz, *jj,ierr;
  Scalar   *v, *vv;

  ierr = MatGetRow( mat, row[prow], &nz, &j, &v ); CHKERRQ(ierr);
  for (k=0; k<nz; k++) {
    if (col[j[k]] < prow && PetscAbsScalar(v[k]) > repla) {
      /* See if this one will work */
      repl  = col[j[k]];
      ierr = MatGetRow( mat, row[repl], &nnz, &jj, &vv ); CHKERRQ(ierr);
      for (kk=0; kk<nnz; kk++) {
	if (col[jj[kk]] == prow && PetscAbsScalar(vv[kk]) > atol) {
	  *rcv = PetscAbsScalar(v[k]);
	  *rc  = repl;
          ierr = MatRestoreRow( mat, row[repl], &nnz, &jj, &vv ); CHKERRQ(ierr);
          ierr = MatRestoreRow( mat, row[prow], &nz, &j, &v ); CHKERRQ(ierr);
	  return 1;
	}
      }
      ierr = MatRestoreRow( mat, row[repl], &nnz, &jj, &vv ); CHKERRQ(ierr);
    }
  }
  ierr = MatRestoreRow( mat, row[prow], &nz, &j, &v ); CHKERRQ(ierr);
  return 0;
}

#undef __FUNCTION__  
#define __FUNCTION__ "MatReorderForNonzeroDiagonal"
/*@
    MatReorderForNonzeroDiagonal - Changes matrix ordering to remove
        zeros from diagonal. This may help in the LU factorization to 
        prevent a zero pivot.

    Input Parameters:
.   mat  - matrix to reorder
.   rmap,cmap - row and column permutations.  Usually obtained from 
.               MatGetReordering().

    Notes:
    This is not intended as a replacement for pivoting for matrices that
    have ``bad'' structure. It is only a stop-gap measure.

    Algorithm:
    Column pivoting is used.  Choice of column is made by looking at the
    non-zero elements in the row.  This algorithm is simple and fast but
    does NOT guarentee that a non-singular or well conditioned
    principle submatrix will be produced.
@*/
int MatReorderForNonzeroDiagonal(Mat mat,double atol,IS ris,IS cis )
{
  int      ierr, prow, k, nz, n, repl, *j, *col, *row, m;
  Scalar   *v;
  double   repla;

  PetscValidHeaderSpecific(mat,MAT_COOKIE);
  PetscValidHeaderSpecific(ris,IS_COOKIE);
  PetscValidHeaderSpecific(cis,IS_COOKIE);
  
  ierr = ISGetIndices(ris,&row); CHKERRQ(ierr);
  ierr = ISGetIndices(cis,&col); CHKERRQ(ierr);
  ierr = MatGetSize(mat,&m,&n); CHKERRQ(ierr);

  for (prow=0; prow<n; prow++) {
    ierr = MatGetRow( mat, row[prow], &nz, &j, &v ); CHKERRQ(ierr);
    for (k=0; k<nz; k++) {if (col[j[k]] == prow) break;}
    if (k >= nz || PetscAbsScalar(v[k]) <= atol) {
      /* Element too small or zero; find the best candidate */
      repl  = prow;
      repla = (k >= nz) ? 0.0 : PetscAbsScalar(v[k]);
      for (k=0; k<nz; k++) {
	if (col[j[k]] > prow && PetscAbsScalar(v[k]) > repla) {
	  repl = col[j[k]];
	  repla = PetscAbsScalar(v[k]);
        }
      }
      if (prow == repl) {
	    /* Now we need to look for an element that allows us
	       to pivot with a previous column.  To do this, we need
	       to be sure that we don't introduce a zero in a previous
	       diagonal */
        if (!MatZeroFindPre_Private(mat,prow,row,col,repla,atol,&repl,&repla)){
	  SETERRQ(1,"Can not reorder matrix");
	}
      }
      SWAP(col[prow],col[repl]); 
    }
    ierr = MatRestoreRow( mat, row[prow], &nz, &j, &v ); CHKERRQ(ierr);
  }
  ierr = ISRestoreIndices(ris,&row); CHKERRQ(ierr);
  ierr = ISRestoreIndices(cis,&col); CHKERRQ(ierr);
  return 0;
}




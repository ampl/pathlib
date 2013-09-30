/*  The problem solved is:

        Mx + q  perp l <= x <= u

    M is a square matrix.  M should also be a positive semidefinite matrix.

    The LCP uses the (i, j, data) format for inputing sparse matrice (M).  
    We adopt the FORTRAN convention that the indices begin at 1.

    Here is a description of each of the arguments to the LCP_Create functions:

        - variables - the number of variables in the problem
        - m_nnz - the number of nonzeros in the M matrix
        - m_i - a vector of size m_nnz containing the row indices for M
        - m_j - a vector of size m_nnz containing the column indices for M
        - m_ij - a vector of size m_nnz containing the data for M
        - q - a vector of size variables
        - lb - a vector of size variables containing the lower bounds on x
        - ub - a vector of size variables containing the upper bounds on x
*/

#ifndef SIMPLELCP_H
#define SIMPLELCP_H

#include "Types.h"

void SimpleLCP(int variables,
	       int m_nnz, int *m_i, int *m_j, double *m_ij, double *q,
	       double *lb, double *ub,
               MCP_Termination *status, double *z);

#endif


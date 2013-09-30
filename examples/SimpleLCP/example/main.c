
#include "SimpleLCP.h"
#include <stdlib.h>
#include <stdio.h>

int main()
{
     MCP_Termination termination;
     char filename[1024];
     FILE *fp;

     int variables;

     double *x_end;

     int m_nnz;
     int *m_i;
     int *m_j;
     double *m_ij;
     double *q;

     double *lb;
     double *ub;
    
     int i;

     /* Read in data from a file. */

     printf("Enter filename: ");
     scanf("%s", filename);

     if ((fp = fopen(filename, "r")) == NULL)
     {
	  printf("Could not open file.\n");
	  return 0;
     }

     fscanf(fp, "%d", &variables);

     x_end = (double *)malloc(sizeof(double)*variables + 1);

     fscanf(fp, "%d", &m_nnz);

     m_i = (int *)malloc(sizeof(int)*m_nnz + 1);
     m_j = (int *)malloc(sizeof(int)*m_nnz + 1);
     m_ij = (double *)malloc(sizeof(double)*m_nnz + 1);
     q = (double *)malloc(sizeof(double)*variables + 1);

     for (i = 0; i < m_nnz; i++)
     {
	  fscanf(fp, "%d%d%lf", &(m_i[i]), &(m_j[i]), &(m_ij[i]));
     }

     for (i = 0; i < variables; i++)
     {
	  x_end[i] = 0.0;
	  fscanf(fp, "%lf", &(q[i]));
     }
 
     lb = (double *)malloc(sizeof(double)*variables + 1);
     ub = (double *)malloc(sizeof(double)*variables + 1);

     for (i = 0; i < variables; i++)
     {
	  fscanf(fp, "%lf%lf", &(lb[i]), &(ub[i]));
     }

     SimpleLCP(variables, m_nnz, m_i, m_j, m_ij, q, lb, ub, 
               &termination, x_end);

     if (termination == MCP_Error)
     { 
	  printf("Error in the solution.\n");
     } else if (termination == MCP_Solved)
     {
          printf("LCP Solved.\n");
     } else
     {
          printf("Other error: %d\n", termination);
     }

     for (i = 0; i < variables; i++)
     {
	  printf("x[%3d] = %5.2f\n", i+1, x_end[i]);
     }

     free(x_end);

     free(m_i);
     free(m_j);
     free(m_ij);
     free(q);

     free(lb);
     free(ub);

     return 0;
}

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "MCP_Interface.h"
#include "Install.h"

#include "Path.h"
#include "PathOptions.h"
#include "Options.h"

#include "Error.h"
#include "Macros.h"
#include "Output.h"
#include "Output_Interface.h"
#include "Timer.h"

#include "mex.h"

static char *cpfj;
static int nMax;
static int nnzMax;
static double *zp, *lp, *up;

static CB_FUNC(void) problem_size(void *v, int *n, int *nnz)
{
  *n = nMax;
  *nnz = nnzMax;
  return;
}

static CB_FUNC(void) bounds(void *v, int n, double *z, double *lb, double *ub)
{
  int i;
     
  for (i = 0; i < n; i++) {
    z[i] = zp[i];
    lb[i] = lp[i];
    ub[i] = up[i];
  }
  return;
}

static CB_FUNC(int) function_evaluation(void *v, int n, double *z, double *f)
{
  /* Function evaluation calls the M-file specified */

  int mrhs, mlhs, domerr, i;
  double *jacflag, *zval, *fval;
  mxArray *lhs[3], *rhs[2];

  /* To call the function, we need to set up some input arguments.
     0 - current point
     1 - jacFlag (0 = no jacobian, 1 = evaluate jacobian)
  */

  rhs[1] =  mxCreateDoubleMatrix(1, 1, mxREAL);
  jacflag = mxGetPr(rhs[1]); 
  *jacflag = 0.;

  rhs[0] =  mxCreateDoubleMatrix(n, 1, mxREAL);
  zval = mxGetPr(rhs[0]);
  for (i = 0; i < n; i++) {
    zval[i] = z[i];
  }

  mrhs = 2;
  mlhs = 3;

  /* Call the matlab function, cpfj - m-file to execute */

  mexCallMATLAB(mlhs, lhs, mrhs, rhs, cpfj);

  /* Returns three arguments
     0 - function vector
     1 - jacobian matrix
     2 - number of domain violations
  */

  domerr = (int) mxGetScalar(lhs[2]);

  if (!domerr) {
    fval = mxGetPr(lhs[0]);
    for (i = 0; i < n; i++) {
      f[i] = fval[i];
    }
  }

  /* Deallocate space */
  mxDestroyArray(lhs[0]);
  mxDestroyArray(lhs[1]);
  mxDestroyArray(lhs[2]);
     
  mxDestroyArray(rhs[0]);
  mxDestroyArray(rhs[1]);

  /* Return number of domain violations */
  return(domerr);
}

static CB_FUNC(int) jacobian_evaluation(void *v, int n, double *z, int wantf, 
			       	        double *f, int *nnz,
			       	        int *col_start, int *col_len, 
	                       	        int *row, double *data)
{
  /* Similar to function evaluation.  Need to fill in jacobian.
     MATLAB - C style indices, SparseMatrix - FORTRAN indices. */

  int mrhs, mlhs, i, actualNnz, domerr, j;
  int *ia, *ja;
  double *jacflag, *zval, *fval, *vala;
  mxArray *lhs[3], *rhs[2];
     
  rhs[1] =  mxCreateDoubleMatrix(1, 1, mxREAL);
  jacflag = mxGetPr(rhs[1]); 
  *jacflag = 1.;
     
  rhs[0] =  mxCreateDoubleMatrix(n, 1, mxREAL);
  zval = mxGetPr(rhs[0]);
  for (i = 0; i < n; i++) {
    zval[i] = z[i];
  }

  mrhs = 2;
  mlhs = 3;
  mexCallMATLAB(mlhs, lhs, mrhs, rhs, cpfj);

  domerr = (int) mxGetScalar(lhs[2]);
  if (!domerr) {
    if (wantf) {
      fval = mxGetPr(lhs[0]);
      for (i = 0; i < n; i++) {
	f[i] = fval[i];
      }
    }

    ia = mxGetIr(lhs[1]);
    ja = mxGetJc(lhs[1]);
    actualNnz = ja[n];
    if (actualNnz > *nnz) {
      mexErrMsgTxt("Jacobian has too many entries: "
		   "update nnz in pathsol.m\n");
    }
    *nnz = actualNnz;

    vala = mxGetPr(lhs[1]);
    for (j = 0; j < n; j++) {
      col_start[j] = ja[j]+1;
      col_len[j] = ja[j+1]-ja[j];
    }

    for (j = 0; j < actualNnz; j++) {
      row[j] = ia[j]+1;
      data[j] = vala[j];
    }
  }
  else {
    for (i = 0; i < n; i++) {
      col_start[j] = 1;
      col_len[j] = 0;
    }
  }

  mxDestroyArray(lhs[0]);
  mxDestroyArray(lhs[1]);
  mxDestroyArray(lhs[2]);
     
  mxDestroyArray(rhs[0]);
  mxDestroyArray(rhs[1]);
  return(domerr);
}

static MCP_Interface m_interface =
{
  NULL,
  problem_size, 
  bounds,
  function_evaluation, 
  jacobian_evaluation,
  NULL, NULL,
  NULL, NULL,
  NULL
};

static void install_interface(MCP *m)
{
  MCP_SetInterface(m, &m_interface);
  return;
}

/* mexFunction - called by MATLAB
   
   arguments:

       nlhs - number of left-hand side arguments (values returned)
       plhs - the actual left-hand side arguments
       nrhs - number of right-hand side arguments (values input)
       prhs - the actual right-hand side arguments

   structure:
      1.  Set up the interfaces for error, memory, output, and problem
      2.  Check matlab input and output arguments
      3.  Create problem structure and read the options
      4.  Solve the problem
      5.  Report the results
      6.  Deallocate memory and close file
*/
 
void mexFunction(int nlhs, mxArray *plhs[], int	nrhs, const mxArray *prhs[])
{
  FILE *log;
  Options_Interface *o;
  Void *timer;
  MCP *m;
  MCP_Termination t;
  Information info;

  double *tempZ;
  double *status, *tot_time;

  double dnnz;
  int i, len;
     
  /* Code needs to know some things
     1.  How are the errors handled?
     2.  How is memory allocation performed?
     3.  How is output done?
     4.  How do you obtain information about the problem?
     These are told to the algorithm by the install_* functions.  */
     
  mat_install_error();
  mat_install_memory();

  /* The default for output is to files.  We do not need to install a
     new output interface.  But we do need to specify a file.  */

  log = fopen("logfile.tmp", "w"); /* Open a file for writing */
  Output_SetLog(log);              /* Set the log file appropriately */

  /* Options.
     1.  Get options associated with the algorithm
     2.  Set to default values
     3.  Read in an options file
     4.  Print out the options */

  o = Options_Create();
  Path_AddOptions(o);
  Options_Default(o);

  /* Rest of the code deals with checking the arguments to mexFunction.
      
     numer of input arguments (nrhs) should be 6
     0 - dimension of the problem (nMax)
     1 - nonzeros in Jacobian (nnzMax)
     2 - vector of length nMax, starting point
     3 - vector of length nMax, lower bounds
     4 - vector of length nMax, upper bounds
     5 - character string contains the name of the function
     this is an m-file that evaluates function/jacobian
  */

  if (nrhs < 6) {
    Error("Not enough input arguments.\n");
  }

  if (nlhs < 2) {
    Error("Not enough output arguments.\n");
  }

  /* Create a timer and start it. */
  timer = Timer_Create();
  Timer_Start(timer);

  nMax = (int) mxGetScalar(prhs[0]);
  nnzMax = (int) mxGetScalar(prhs[1]);
  zp = mxGetPr(prhs[2]);
  lp = mxGetPr(prhs[3]);
  up = mxGetPr(prhs[4]);
  len = mxGetN(prhs[5])+1;
  cpfj = mxCalloc(len,sizeof(char));
  mxGetString(prhs[5],cpfj,len);

  /* Output arguments (nlhs).  Variable number of arguments 
     minimum 2, maximum 4
        
     0 - status (1 = solved, 0 = failed to solve)
     1 - total time
     2 - function evaluation at solution
     3 - jacobian evaluation at the solution

     Solution vector is returned in zp (one of the input arguments).

     Need to allocate space for the return arguments.
  */

  plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
  plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);

  if (nlhs > 2) {
    plhs[2] = mxCreateDoubleMatrix(nMax,1,mxREAL);
  }

  if (nlhs > 3) {
    plhs[3] = mxCreateSparse(nMax,nMax,nnzMax,mxREAL);
  }

  /* Derefencing to get the status argument. */
  status = mxGetPr(plhs[0]);
  tot_time = mxGetPr(plhs[1]);
  
  /* First print out some version information.  */

  Output_Printf(Output_Log | Output_Status | Output_Listing,
		"%s: Matlab Link\n", Path_Version());

  /* Now check the arguments.

     If the dimension of the problem is 0, there is nothing to do.
  */

  if (nMax == 0) {
    Output_Printf(Output_Log | Output_Status,
		  "\n ** EXIT - solution found (degenerate model).\n");
    (*status) = 1.0;
    (*tot_time) = Timer_Query(timer);

    Timer_Destroy(timer);
    Options_Destroy(o);
    fclose(log);
    return;
  }

  /* Check size of the jacobian.  nnz < n*n.  Need to convert to double
     so we do not run out of integers.

     dnnz - max number of nonzeros as a double
  */

  dnnz = MIN(1.0*nnzMax, 1.0*nMax*nMax);
  if (dnnz > INT_MAX) {
    Output_Printf(Output_Log | Output_Status,
		  "\n ** EXIT - model too large.\n");
    (*status) = 0;
    (*tot_time) = Timer_Query(timer);

    Timer_Destroy(timer);
    Options_Destroy(o);
    fclose(log);
    return;
  }
  nnzMax = (int) dnnz;

  /* Have correct number of nonzeros.  Print some statistics.
     Print out the density.  */

  Output_Printf(Output_Log | Output_Status | Output_Listing,
		"%d row/cols, %d non-zeros, %3.2f%% dense.\n\n",
		nMax, nnzMax, 100.0*nnzMax/(1.0*nMax*nMax));
  nnzMax++;

  /* Create a structure for the problem. */
  m = MCP_Create(nMax, nnzMax);
  install_interface(m);

  Options_Read(o, "path.opt");
  Options_Display(o);

  info.generate_output = Output_Log | Output_Status | Output_Listing;
  info.use_start = True;
  info.use_basics = True;

  /* Solve the problem.  
     m - MCP to solve, info - information, t - termination */

  t = Path_Solve(m, &info);

  /* Read in the results.  First get the solution vector. The report. */

  tempZ = MCP_GetX(m);
  for (i = 0; i < nMax; i++) {
    zp[i] = tempZ[i];
  }

  /* If the final function evaluation is wanted, report it. */
  if (nlhs > 2) {
    double *fval = mxGetPr(plhs[2]);

    tempZ = MCP_GetF(m);
    for (i = 0; i < nMax; i++) {
      fval[i] = tempZ[i];
    }
  }

  /* If the final jacobian value is wanted, report it. */
  if (nlhs > 3) {
    /* SparseMatrices are in compressed sparse column format.
       cs - column starts
       cl - lenght of the colum
       rv - row value
       dv - data value
       Note: SparseMatrixes in the algorithm use FORTRAN indices.
       (All indices go from 1 to n.)
    */

    Int *cs, *cl, *rv;
    Double *dv;

    /* MATLAB storage - compressed sparse column, C indices */

    Int *ia = mxGetIr(plhs[3]);
    Int *ja = mxGetJc(plhs[3]);
    Double *vala = mxGetPr(plhs[3]);

    Int start, end, nz;

    MCP_GetJ(m, &nz, &cs, &cl, &rv, &dv);

    if (nnzMax < nz) {
      Error("Jacobian has too many entries.\n");
    }

    /* Loop through the columns, setting up the matrix. */
    nz = 0;
    for(i = 0; i < nMax; i++) {
      ja[i] = nz;

      /* Get start and end for the SparseMatrix */
      start = cs[i] - 1;
      end = start + cl[i];

      /* Loop through the elements in the column */
      while(start < end) {
	ia[nz] = rv[start] - 1; /* Converts from FORTRAN to C */
	vala[nz] = dv[start];
	nz++;
	start++;
      }
    }
    ja[i] = nz;
  }

  /* Fill in the final status. 1 = solved, 0 = failed to solve. */
  switch(t) {
  case MCP_Solved:
    *status = 1.0;
    break;
	  
  default:
    *status = 0.0;
    break;
  }
     
  /* Stop the timer and report the results. */
  (*tot_time) = Timer_Query(timer);
  Timer_Destroy(timer);

  /* Clean up.  Deallocate memory used and close the log. */
  MCP_Destroy(m);
  Options_Destroy(o);

  mxFree(cpfj);
  fclose(log);
  return;
}


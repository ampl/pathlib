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
#include "Memory.h"
#include "Output.h"
#include "Output_Interface.h"
#include "Timer.h"

#include "mex.h"

static int nMax;
static int nnzMax, actualNnz;
static double *zp, *lp, *up, *q, *m_data;
static int *m_start, *m_len, *m_row;
static int filled;

static CB_FUNC(void) start(void *v)
{
  filled = 0;
  return;
}

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
  int col, colStart, colEnd, row;
  double value;

  for (col = 0; col < nMax; col++) {
    f[col] = q[col];
  }

  for (col = 0; col < nMax; col++) {
    value = z[col];

    if (value != 0) {
      colStart = m_start[col] - 1;
      colEnd = colStart + m_len[col];

      while(colStart < colEnd) {
	row = m_row[colStart] - 1;
	f[row] += m_data[colStart]*value;
	colStart++;
      }
    }
  }
  return 0;
}

static CB_FUNC(int) jacobian_evaluation(void *v, int n, double *z, int wantf, 
			                double *f, int *nnz,
			                int *col_start, int *col_len, 
			                int *row, double *data)
{
  int element;

  if (wantf) {
    function_evaluation(v, n, z, f);
  }

  if (!filled) {
    for (element = 0; element < nMax; element++) {
      col_start[element] = m_start[element];
      col_len[element] = m_len[element];
    }

    for (element = 0; element < actualNnz; element++) {
      row[element] = m_row[element];
      data[element] = m_data[element];
    }

    filled = 1;
  }

  *nnz = actualNnz;
  return 0;
}

static CB_FUNC(void) mcp_typ(void *d, int nnz, int *typ)
{
  int i;

  for (i = 0; i < nnz; i++) {
    typ[i] = PRESOLVE_LINEAR;
  }
  return;
}

static MCP_Interface m_interface =
{
  NULL,
  problem_size, bounds,
  function_evaluation, jacobian_evaluation,
  start, NULL,
  NULL,
  NULL
};

static Presolve_Interface p_interface =
{
  NULL,
  NULL, NULL,
  NULL, NULL,
  mcp_typ,
  NULL
};

static void install_interface(MCP *m)
{
  MCP_SetInterface(m, &m_interface);
  MCP_SetPresolveInterface(m, &p_interface);
  return;
}

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

  mat_install_error();
  mat_install_memory();

  log = fopen("logfile.tmp", "w");
  Output_SetLog(log);

  /* Options.
     1.  Get options associated with the algorithm
     2.  Set to default values
     3.  Read in an options file
     4.  Print out the options */

  o = Options_Create();
  Path_AddOptions(o);
  Options_Default(o);

  if (nrhs < 7) {
    Error("Not enough input arguments.\n");
  }

  if (nlhs < 2) {
    Error("Not enough output arguments.\n");
  }

  nMax = (int) mxGetScalar(prhs[0]);
  nnzMax = (int) mxGetScalar(prhs[1]);

  plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
  plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);

  if (nlhs > 2) {
    plhs[2] = mxCreateDoubleMatrix(nMax,1,mxREAL);
  }

  if (nlhs > 3) {
    plhs[3] = mxCreateDoubleMatrix(nMax,1,mxREAL);
  }

  /* Create a timer and start it. */
  timer = Timer_Create();
  Timer_Start(timer);

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
    (*status) = 0.0;
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

  if (nnzMax == 0) {
    nnzMax++;
  }

  zp = mxGetPr(prhs[2]);
  lp = mxGetPr(prhs[3]);
  up = mxGetPr(prhs[4]);

  m_start = mxGetJc(prhs[5]);
  m_row = mxGetIr(prhs[5]);
  m_data = mxGetPr(prhs[5]);
  m_len = (int *)Memory_Allocate(nMax*sizeof(int));

  for (i = 0; i < nMax; i++) {
    m_len[i] = m_start[i+1] - m_start[i];
    m_start[i]++;
  }
  actualNnz = m_start[nMax];

  for (i = 0; i < actualNnz; i++) {
    m_row[i]++;
  }

  q = mxGetPr(prhs[6]);
     
  /* Create a structure for the problem. */
  m = MCP_Create(nMax, nnzMax);
  MCP_Jacobian_Structure_Constant(m, 1);
  MCP_Jacobian_Data_Contiguous(m, 1);
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

  /* If the final basis is wanted, report it. */
  if (nlhs > 3) {
    double *bval = mxGetPr(plhs[3]);
    int *tempB;

    tempB = MCP_GetB(m);
    for (i = 0; i < nMax; i++) {
      switch(tempB[i]) {
      case Basis_LowerBound:
	bval[i] = -1;
	break;

      case Basis_UpperBound:
	bval[i] = 1;
	break;

      default:
	bval[i] = 0;
	break;
      }
    }
  }

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
  Memory_Free(m_len);
  Options_Destroy(o);

  fclose(log);
  return;
}


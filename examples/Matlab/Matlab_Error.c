#include <stdio.h>
#include "Install.h"
#include "Error_Interface.h"

#include "mex.h"

static CB_FUNC(void) mat_error(void *v, char *msg)
{
  mexErrMsgTxt(msg);
  return;
}

static Error_Interface mat_error_interface =
{
  NULL,
  mat_error, 
  NULL
};

void mat_install_error(void)
{
  Error_SetInterface(&mat_error_interface);
  return;
}


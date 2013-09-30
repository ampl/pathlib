#include <stdlib.h>
#include "Install.h"
#include "Memory_Interface.h"

#include "mex.h"

static CB_FUNC(void *)mat_allocate(void *data, unsigned long n)
{
  void *v;

  v = (void *)mxMalloc(n);
  return v;
}

static CB_FUNC(void) mat_free(void *data, void *v)
{
  mxFree(v);
  return;
}

static Memory_Interface mat_memory_interface =
{
  NULL,
  mat_allocate,  /* normal memory allocation */
  mat_allocate,  /* allocation for the LU factors */
  mat_free,      /* normal free */
  mat_free	 /* free for the LU factors */
};

void mat_install_memory(void)
{
  Memory_SetInterface(&mat_memory_interface);
  return;
}


/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id$
 *----------------------------------------------------------------------------
 */

#include "config.h"

/* ------------------------------------------------------------------------ */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/* ------------------------------------------------------------------------ */

void _failed_assertion(char * file, int lineno)
{
  fprintf(stderr, "*** %s:%d: assertion failed!\n", file, lineno);
  fflush(stderr); 
  kill(getpid(),SIGSEGV);
}

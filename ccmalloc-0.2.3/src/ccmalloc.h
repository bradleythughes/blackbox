/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id$
 *----------------------------------------------------------------------------
 */

#ifndef _ccmalloc_h_INCLUDED
#define _ccmalloc_h_INCLUDED

#include <sys/types.h>

extern void * ccmalloc_malloc(size_t);
extern void ccmalloc_free(void *);
extern void ccmalloc_init();
extern void ccmalloc_report();

#endif

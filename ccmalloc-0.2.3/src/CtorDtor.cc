/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id$
 *----------------------------------------------------------------------------
 */

extern "C" {
#include "ccmalloc.h"
};

class CCMalloc_InitAndReport
{
public:
  CCMalloc_InitAndReport() { ccmalloc_init(); }
  ~CCMalloc_InitAndReport() { ccmalloc_report(); }
};

static CCMalloc_InitAndReport ccmalloc_initAndReport;

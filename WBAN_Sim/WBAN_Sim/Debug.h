#include "Sched.h"

#define DEBUG 0
#define Timeline 0

#if (DEBUG>0)
#define debug(x) printf x
#else
#define debug
#endif

#if (Timeline>0)
#define Time(x) printSched(x)
#else
#define Time
#endif
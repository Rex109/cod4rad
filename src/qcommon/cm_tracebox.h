/* Original: c:\trees\cod3\cod3src\src\qcommon\cm_tracebox.cpp */

#ifndef CM_TRACEBOX_H
#define CM_TRACEBOX_H

#include "q_shared.h"
#include "com_vector.h"


typedef struct
{
    vec3_t start;       /* +0x00 */
    vec3_t end;         /* +0x0c */
    vec3_t invDelta;    /* +0x18 */
} TraceLine_t;


void     CM_SetTraceInvDelta( TraceLine_t *trace );             /* 0x00421ca0 */

qboolean CM_TraceLineMissesBounds( const TraceLine_t *trace,
                                   const vec3_t mins, const vec3_t maxs,
                                   float maxT );                /* 0x00421d30 */

#endif

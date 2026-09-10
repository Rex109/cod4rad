/* Original: c:\trees\cod3\cod3src\src\qcommon\cm_tracebox.cpp */

#include "cod4rad.h"
#include "cm_tracebox.h"


/* CM_SetTraceInvDelta  0x00421ca0 */
void CM_SetTraceInvDelta( TraceLine_t *trace )
{
    int   axis;
    float delta;

    for ( axis = 0; axis < 3; axis++ )
    {
        delta = trace->start[axis] - trace->end[axis];

        trace->invDelta[axis] = ( delta != 0.0f ) ? 1.0f / delta : 0.0f;
    }
}

/* CM_TraceLineMissesBounds  0x00421d30 */
qboolean CM_TraceLineMissesBounds( const TraceLine_t *trace,
                                   const vec3_t mins, const vec3_t maxs,
                                   float maxT )
{
    const float *bound;
    float        tEnter;
    float        tExit;
    float        sign;
    int          axis;

    tEnter = 0.0f;
    tExit  = maxT;
    bound  = mins;
    sign   = -1.0f;

    for ( ;; )
    {
        Assert( !IS_NAN( bound[0] ) && !IS_NAN( bound[1] ) && !IS_NAN( bound[2] ) );

        for ( axis = 0; axis < 3; axis++ )
        {
            float d0 = ( trace->start[axis] - bound[axis] ) * sign;
            float d1 = ( trace->end[axis]   - bound[axis] ) * sign;

            if ( d0 > 0.0f )
            {
                float t;

                if ( d1 > 0.0f )
                    return qtrue;

                t = d0 * trace->invDelta[axis] * sign;

                if ( t >= tExit )
                    return qtrue;

                if ( t > tEnter )
                    tEnter = t;
            }
            else if ( d1 > 0.0f )
            {
                float t = d0 * trace->invDelta[axis] * sign;

                if ( t <= tEnter )
                    return qtrue;

                if ( t < tExit )
                    tExit = t;
            }
        }

        if ( sign == 1.0f )
            return qfalse;

        bound = maxs;
        sign  = 1.0f;
    }
}

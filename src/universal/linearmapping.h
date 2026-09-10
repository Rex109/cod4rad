/* Original: c:\trees\cod3\cod3src\src\universal\linearmapping.cpp */

#ifndef LINEARMAPPING_H
#define LINEARMAPPING_H

#include "q_shared.h"
#include "com_vector.h"


typedef struct
{
    double matrix[3][3];    /* +0x00 */
    double lu[3][3];        /* +0x48 */
    int    pivot[3];        /* +0x90 */
    int    axis0;           /* +0x9c */
    int    axis1;           /* +0xa0 */
    int    axis2;           /* +0xa4 */
} LinearMapping_t;          /* sizeof == 0xa8 */


qboolean LinearMapping_Solve( const vec3_t normal, const vec3_t p0, const vec3_t p1,
                              const vec3_t p2, LinearMapping_t *mapping ); /* 0x00417650 */

void LinearMapping_Apply( const LinearMapping_t *mapping, float v0, float v1,
                          float v2, vec4_t out );                    /* 0x004176e0 */


float Vec3ScaleToMax( vec3_t v );                       /* 0x004177b0 */

#endif

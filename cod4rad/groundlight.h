/* Original: c:\trees\cod3\cod3src\cod2rad\groundlight.cpp */

#ifndef GROUNDLIGHT_H
#define GROUNDLIGHT_H

#include "q_shared.h"
#include "com_vector.h"
#include "bspfile.h"


#define GROUND_MAX_POINTS   8

#define GROUND_TRACE_EXTRA  4.0

#define GROUND_MIN_OPEN     0.10000000149011612


typedef struct GroundLight_s
{
    Entity_t             *ent;                          /* +0x00 */
    vec3_t                origin;                       /* +0x04 */
    float                 distance;                     /* +0x10 */
    vec3_t                dir;                          /* +0x14 */
    vec3_t                points[GROUND_MAX_POINTS];    /* +0x20 */
    unsigned              pointCount;                   /* +0x80 */
    struct GroundLight_s *next;                         /* +0x84 */
} GroundLight_t;                                        /* sizeof == 0x88 */


extern GroundLight_t *groundLights;     /* 0x130632a4 */


void GroundLight_Add( Entity_t *ent, const vec3_t *points, unsigned pointCount,
                      const vec3_t origin, float distance,
                      const vec3_t dir );               /* 0x0040e560 */

void GroundLight_LightModels( void );                   /* 0x0040e4e0 */

#endif

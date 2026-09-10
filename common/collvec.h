/* Original: ..\common\collvec.cpp */

#ifndef COLLVEC_H
#define COLLVEC_H

#include "../src/universal/q_shared.h"

void MakeCollisionVec( const vec3_t dir0, const vec3_t dir1,
                       const vec3_t edgePt0, const vec3_t apexPt, const vec3_t edgePt1,
                       vec4_t out );                                       /* 0x00412400 */

void MakeCollisionVecs( const vec3_t v0, const vec3_t v1, const vec3_t v2,
                        const vec3_t normal, vec4_t svec, vec4_t tvec );   /* 0x00412380 */

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up ); /* 0x00412530 */

#endif

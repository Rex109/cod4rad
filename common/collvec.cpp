/* Original: ..\common\collvec.cpp */

#include "../src/universal/q_shared.h"

#include <math.h>
#include "../src/universal/assertive.h"
#include "../src/universal/com_math.h"
#include "../src/universal/com_vector.h"
#include "collvec.h"

/* MakeCollisionVecs  0x00412380 */
void MakeCollisionVecs( const vec3_t v0, const vec3_t v1, const vec3_t v2,
                        const vec3_t normal, vec4_t svec, vec4_t tvec )
{
    vec3_t d1;
    vec3_t d2;

    Vec3Sub( v1, v0, d1 );
    Vec3Sub( v2, v0, d2 );

    MakeCollisionVec( normal, d2, v0, v1, v2, svec );
    MakeCollisionVec( d1, normal, v0, v2, v1, tvec );
}

/* MakeCollisionVec  0x00412400 */
void MakeCollisionVec( const vec3_t dir0, const vec3_t dir1,
                       const vec3_t edgePt0, const vec3_t apexPt, const vec3_t edgePt1,
                       vec4_t out )
{
    double nx;
    double ny;
    double nz;
    double dist;
    double scale;
    double invScale;

    nx = dir0[1] * dir1[2] - dir0[2] * dir1[1];
    ny = dir0[2] * dir1[0] - dir0[0] * dir1[2];
    nz = dir0[0] * dir1[1] - dir0[1] * dir1[0];

    dist = ( ( edgePt0[0] + edgePt1[0] ) * nx
           + ( edgePt0[1] + edgePt1[1] ) * ny
           + ( edgePt0[2] + edgePt1[2] ) * nz ) * 0.5;

    scale = apexPt[0] * nx + apexPt[1] * ny + apexPt[2] * nz - dist;

    Assertx( scale > 0, "(scale) = %lg", scale );

    invScale = 1.0 / scale;

    out[0] = nx * invScale;
    out[1] = ny * invScale;
    out[2] = nz * invScale;
    out[3] = dist * invScale;
}

/* AngleVectors  0x00412530 */
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
    float angle;
    float sr;
    float sp;
    float sy;
    float cr;
    float cp;
    float cy;

    angle = angles[1] * DEG2RAD;
    SinCos( angle, &sy, &cy );

    angle = angles[0] * DEG2RAD;
    SinCos( angle, &sp, &cp );

    if ( forward )
    {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }

    if ( right || up )
    {
        angle = angles[2] * DEG2RAD;
        SinCos( angle, &sr, &cr );

        if ( right )
        {
            right[0] = -1.0 * sr * sp * cy + -1.0 * cr * -sy;
            right[1] = -1.0 * sr * sp * sy + -1.0 * cr * cy;
            right[2] = -1.0 * sr * cp;
        }

        if ( up )
        {
            up[0] = cr * sp * cy + -sr * -sy;
            up[1] = cr * sp * sy + -sr * cy;
            up[2] = cr * cp;
        }
    }
}

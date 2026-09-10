/* Original: ..\src\xanim\xanim_public.h */

#ifndef XANIM_PUBLIC_H
#define XANIM_PUBLIC_H

#include "../universal/q_shared.h"
#include "../universal/assertive.h"
#include "../universal/com_math.h"
#include "../universal/com_vector.h"

struct DObjAnimMat
{
    vec4_t quat;            /* +0x00 */
    vec3_t trans;           /* +0x10 */
    float  transWeight;     /* +0x1c */
};

/* DObjAnimMat_GetAxis  0x004759e0 */
inline void DObjAnimMat_GetAxis( const DObjAnimMat *mat, vec3_t *axis )
{
    vec3_t xyz2;
    float  xx, xy, xz, xw;
    float  yy, yz, yw;
    float  zz, zw;

    Assert( !IS_NAN((mat->quat)[0]) && !IS_NAN((mat->quat)[1]) &&
            !IS_NAN((mat->quat)[2]) && !IS_NAN((mat->quat)[3]) );
    Assert( !IS_NAN(mat->transWeight) );

    Vec3Scale( mat->quat, mat->transWeight, xyz2 );

    xx = xyz2[0] * mat->quat[0];
    xy = xyz2[0] * mat->quat[1];
    xz = xyz2[0] * mat->quat[2];
    xw = xyz2[0] * mat->quat[3];
    yy = xyz2[1] * mat->quat[1];
    yz = xyz2[1] * mat->quat[2];
    yw = xyz2[1] * mat->quat[3];
    zz = xyz2[2] * mat->quat[2];
    zw = xyz2[2] * mat->quat[3];

    axis[0][0] = 1.0f - ( yy + zz );
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;

    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - ( xx + zz );
    axis[1][2] = yz + xw;

    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - ( xx + yy );
}

/* DObjAnimMat_TransformPoint  0x00475940 */
inline void DObjAnimMat_TransformPoint( const vec3_t point, const DObjAnimMat *mat, vec3_t out )
{
    vec3_t axis[3];

    DObjAnimMat_GetAxis( mat, axis );

    out[0] = point[0] * axis[0][0] + point[1] * axis[1][0] + point[2] * axis[2][0] + mat->trans[0];
    out[1] = point[0] * axis[0][1] + point[1] * axis[1][1] + point[2] * axis[2][1] + mat->trans[1];
    out[2] = point[0] * axis[0][2] + point[1] * axis[1][2] + point[2] * axis[2][2] + mat->trans[2];
}

#endif

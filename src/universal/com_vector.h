
#ifndef COM_VECTOR_H
#define COM_VECTOR_H


#include "assertive.h"
#include "q_shared.h"
#include "com_math.h"


extern const vec3_t vec3_origin;    /* 0x004f1324 */
extern const vec4_t vec4_origin;    /* 0x004f1330 */

#define VEC_EPSILON         0.001f

#define NORMALIZE_EPSILON   0.002f


/* Vec2Set  0x0042eed0 */
void Vec2Set( vec2_t v, float x, float y );

inline void Vec2Clear( vec2_t v )
{
    v[0] = 0.0f;
    v[1] = 0.0f;
}

/* Vec2Copy  0x0042b630 */
void Vec2Copy( const vec2_t from, vec2_t to );

/* Vec2Add  0x0042b670 */
void Vec2Add( const vec2_t v0, const vec2_t v1, vec2_t out );

/* Vec2Sub  0x0042ef20 */
void Vec2Sub( const vec2_t v0, const vec2_t v1, vec2_t out );

inline void Vec2Scale( const vec2_t v, float scale, vec2_t out )
{
    out[0] = scale * v[0];
    out[1] = scale * v[1];
}

inline void Vec2Mad( const vec2_t v, float scale, const vec2_t dir, vec2_t out )
{
    out[0] = scale * dir[0] + v[0];
    out[1] = scale * dir[1] + v[1];
}

/* Vec2Dot  0x00412320 */
float Vec2Dot( const vec2_t v0, const vec2_t v1 );

/* Vec2LengthSq  0x0044c060 */
float Vec2LengthSq( const vec2_t v );

/* Vec2Length  0x0042ef50 */
float Vec2Length( const vec2_t v );

inline void Vec2Negate( const vec2_t v, vec2_t out )
{
    out[0] = -v[0];
    out[1] = -v[1];
}

inline bool Vec2Compare( const vec2_t v0, const vec2_t v1 )
{
    if ( v0[0] != v1[0] )
        return false;

    if ( v0[1] != v1[1] )
        return false;

    return true;
}


/* Vec3Set  0x00408850 */
void Vec3Set( vec3_t v, float x, float y, float z );

/* Vec3Clear  0x0040dd90 */
void Vec3Clear( vec3_t v );

/* Vec3Copy  0x004052a0 */
void Vec3Copy( const vec3_t from, vec3_t to );

/* Vec3Add  0x00414e50 */
void Vec3Add( const vec3_t v0, const vec3_t v1, vec3_t out );

/* Vec3Sub  0x00405460 */
void Vec3Sub( const vec3_t v0, const vec3_t v1, vec3_t out );

/* Vec3Scale  0x00414e90 */
void Vec3Scale( const vec3_t v, float scale, vec3_t out );

/* Vec3Mad  0x00414ec0 */
void Vec3Mad( const vec3_t v, float scale, const vec3_t dir, vec3_t out );

/* Vec3Negate  0x00412350 */
void Vec3Negate( const vec3_t v, vec3_t out );

/* Vec3Mul  0x00475540 */
void Vec3Mul( const vec3_t v0, const vec3_t v1, vec3_t out );

/* Vec3Dot  0x004052d0 */
float Vec3Dot( const vec3_t v0, const vec3_t v1 );

/* Vec3LengthSq  0x004054a0 */
float Vec3LengthSq( const vec3_t v );

/* Vec3Length  0x004076c0 */
float Vec3Length( const vec3_t v );

/* Vec3Cross  0x004075e0 */
void Vec3Cross( const vec3_t v0, const vec3_t v1, vec3_t cross );

/* Vec3DistanceSq  0x00405430 */
float Vec3DistanceSq( const vec3_t v0, const vec3_t v1 );

/* Vec3Distance  0x00407690 */
float Vec3Distance( const vec3_t v0, const vec3_t v1 );

/* Vec3Normalize  0x00407760 */
float Vec3Normalize( vec3_t v );

/* Vec3Mid  0x0041d830 */
void Vec3Mid( const vec3_t v0, const vec3_t v1, vec3_t out );

/* Vec3Lerp  0x00451230 */
void Vec3Lerp( const vec3_t from, const vec3_t to, float t, vec3_t out );

/* Vec3Combine  0x004393b0 */
void Vec3Combine( float s0, const vec3_t v0, float s1, const vec3_t v1, vec3_t out );

/* Vec3MadCombine  0x0042ef90 */
void Vec3MadCombine( const vec3_t base, float s0, const vec3_t v0,
                            float s1, const vec3_t v1, vec3_t out );

/* Vec3Compare  0x00412220 */
bool Vec3Compare( const vec3_t v0, const vec3_t v1 );

/* Vec3CompareEpsilon  0x00407800 */
bool Vec3CompareEpsilon( const vec3_t v0, const vec3_t v1, float epsilon );

/* Vec3Equal  0x00405370 */
bool Vec3Equal( const vec3_t v0, const vec3_t v1 );

/* Vec3IsNormalized  0x0041d7b0 */
bool Vec3IsNormalized( const vec3_t v );

/* Vec3SetPerpXY  0x0041d800 */
void Vec3SetPerpXY( const vec3_t v, vec3_t out );


/* Vec4Set  0x0042eef0 */
void Vec4Set( vec4_t v, float x, float y, float z, float w );

/* Vec4Copy  0x00412280 */
void Vec4Copy( const vec4_t from, vec4_t to );

/* Vec4Scale  0x0041d880 */
void Vec4Scale( const vec4_t v, float scale, vec4_t out );

/* Vec4Negate  0x00439370 */
void Vec4Negate( const vec4_t v, vec4_t out );

/* Vec4Lerp  0x00475ba0 */
void Vec4Lerp( const vec4_t from, const vec4_t to, float t, vec4_t out );

/* Vec4Dot  0x00475700 */
float Vec4Dot( const vec4_t v0, const vec4_t v1 );

/* Vec4LengthSq  0x00475690 */
float Vec4LengthSq( const vec4_t v );

/* Vec4Compare  0x0044bfd0 */
bool Vec4Compare( const vec4_t v0, const vec4_t v1 );

/* Vec4IsNormalized  0x00475640 */
bool Vec4IsNormalized( const vec4_t v );

/* Vec2CompareEpsilon  0x00455580 */
bool Vec2CompareEpsilon( const vec2_t v0, const vec2_t v1, float epsilon );

bool Vec4CompareEpsilon( const vec4_t v0, const vec4_t v1, float epsilon );

/* Vec4bSet  0x0044c090 */
void Vec4bSet( byte *out, byte a, byte b, byte c, byte d );

void SetVertexColor( byte *out, byte r, byte g, byte b, byte a );

void PackVertexColor( const byte *rgba, byte *out );

#endif

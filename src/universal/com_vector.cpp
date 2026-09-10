
#include "q_shared.h"
#include "assertive.h"
#include "com_math.h"

__declspec( noinline ) float I_fsel( float comparand, float valGE, float valLT )
{
    if ( comparand < 0.0f )
        return valLT;

    return valGE;
}

__declspec( noinline ) float I_fmin( float a, float b )
{
    return I_fsel( b - a, a, b );
}

__declspec( noinline ) float I_fmax( float a, float b )
{
    return I_fsel( a - b, a, b );
}

float Clamp( float value, float min, float max )
{
    Assert( min < max );

    if ( value < min )
        return min;

    if ( value > max )
        return max;

    return value;
}

__declspec( noinline ) float I_fclamp( float value, float min, float max )
{
    return I_fsel( min - I_fsel( value - max, max, value ), min,
                   I_fsel( value - max, max, value ) );
}

__declspec( noinline ) float I_fsign( float x )
{
    return I_fsel( x, 1.0f, -1.0f );
}

__declspec( noinline ) float I_fabs( float x )
{
    float result = ( float )fabs( x );
    return result;
}

__declspec( noinline ) float I_sqrt( float x )
{
    float result = ( float )sqrt( x );
    return result;
}

__declspec( noinline ) int I_min( int a, int b )
{
    if ( b < a )
        return b;

    return a;
}

__declspec( noinline ) int I_max( int a, int b )
{
    if ( a < b )
        return b;

    return a;
}

__declspec( noinline ) int Float_GetBits( const float *f )
{
    return *( const int * )f;
}

__declspec( noinline ) bool Float_IsNanOrInf( float f )
{
    return ( Float_GetBits( &f ) & 0x7f800000 ) == 0x7f800000;
}

__declspec( noinline ) int RoundFloatToInt( float value )
{
    return Fistp( ( double )value + FISTP_BIAS );
}

__declspec( noinline ) int FloorFloatToInt( float value )
{
    return Fistp( ( double )value - FISTP_HALF_BIAS );
}

__declspec( noinline ) int RoundFloatToIntHalf( float value )
{
    return Fistp( ( double )value + FISTP_HALF_BIAS );
}

__declspec( noinline ) void SinCos( float angle, float *sinOut, float *cosOut )
{
    *cosOut = ( float )cos( angle );
    *sinOut = ( float )sin( angle );
}

__declspec( noinline ) void SinCos( double angle, double *sinOut, double *cosOut )
{
    *cosOut = cos( angle );
    *sinOut = sin( angle );
}

__declspec( noinline ) float AngleNormalize180( float angle )
{
    float frac;
    float biased;
    float wrapped;

    frac    = angle * ( 1.0f / 360.0f );
    biased  = frac + 0.5;
    wrapped = ( frac - floor( biased ) ) * 360.0f;

    return wrapped;
}

__declspec( noinline ) float AngleSubtract( float a1, float a2 )
{
    return AngleNormalize180( a1 - a2 );
}

__declspec( noinline ) void MatrixTransformVector( const vec3_t in1, const float in2[3][3], vec3_t out )
{
    Assert( in1 != out );

    out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
    out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
    out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}

__declspec( noinline ) void QuatMultiply( const vec4_t in1, const vec4_t in2, vec4_t out )
{
    Assert( in1 != out );
    Assert( in2 != out );

    out[0] = ( in1[2] * in2[1] + in1[3] * in2[0] + in1[0] * in2[3] ) - in1[1] * in2[2];
    out[1] = in1[0] * in2[2] + in1[3] * in2[1] + ( in1[1] * in2[3] - in1[2] * in2[0] );
    out[2] = in1[3] * in2[2] + ( ( in1[1] * in2[0] + in1[2] * in2[3] ) - in1[0] * in2[1] );
    out[3] = ( ( in1[3] * in2[3] - in1[0] * in2[0] ) - in1[1] * in2[1] ) - in1[2] * in2[2];
}

__declspec( noinline ) void Vec2Set( vec2_t v, float x, float y )
{
    v[0] = x;
    v[1] = y;
}

__declspec( noinline ) void Vec2Copy( const vec2_t from, vec2_t to )
{
    to[0] = from[0];
    to[1] = from[1];
}

__declspec( noinline ) void Vec2Add( const vec2_t v0, const vec2_t v1, vec2_t out )
{
    out[0] = v0[0] + v1[0];
    out[1] = v0[1] + v1[1];
}

__declspec( noinline ) void Vec2Sub( const vec2_t v0, const vec2_t v1, vec2_t out )
{
    out[0] = v0[0] - v1[0];
    out[1] = v0[1] - v1[1];
}

__declspec( noinline ) float Vec2Dot( const vec2_t v0, const vec2_t v1 )
{
    float dot = v0[0] * v1[0] + v0[1] * v1[1];
    return dot;
}

__declspec( noinline ) float Vec2LengthSq( const vec2_t v )
{
    float lengthSq = v[0] * v[0] + v[1] * v[1];
    return lengthSq;
}

__declspec( noinline ) float Vec2Length( const vec2_t v )
{
    float length = I_sqrt( v[0] * v[0] + v[1] * v[1] );
    return length;
}

__declspec( noinline ) void Vec3Set( vec3_t v, float x, float y, float z )
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

__declspec( noinline ) void Vec3Clear( vec3_t v )
{
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
}

__declspec( noinline ) void Vec3Copy( const vec3_t from, vec3_t to )
{
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
}

__declspec( noinline ) void Vec3Add( const vec3_t v0, const vec3_t v1, vec3_t out )
{
    out[0] = v0[0] + v1[0];
    out[1] = v0[1] + v1[1];
    out[2] = v0[2] + v1[2];
}

__declspec( noinline ) void Vec3Sub( const vec3_t v0, const vec3_t v1, vec3_t out )
{
    out[0] = v0[0] - v1[0];
    out[1] = v0[1] - v1[1];
    out[2] = v0[2] - v1[2];
}

__declspec( noinline ) void Vec3Scale( const vec3_t v, float scale, vec3_t out )
{
    out[0] = scale * v[0];
    out[1] = scale * v[1];
    out[2] = scale * v[2];
}

__declspec( noinline ) void Vec3Mad( const vec3_t v, float scale, const vec3_t dir, vec3_t out )
{
    out[0] = scale * dir[0] + v[0];
    out[1] = scale * dir[1] + v[1];
    out[2] = scale * dir[2] + v[2];
}

__declspec( noinline ) void Vec3Negate( const vec3_t v, vec3_t out )
{
    out[0] = -v[0];
    out[1] = -v[1];
    out[2] = -v[2];
}

__declspec( noinline ) void Vec3Mul( const vec3_t v0, const vec3_t v1, vec3_t out )
{
    out[0] = v0[0] * v1[0];
    out[1] = v0[1] * v1[1];
    out[2] = v0[2] * v1[2];
}

__declspec( noinline ) float Vec3Dot( const vec3_t v0, const vec3_t v1 )
{
    float dot = v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
    return dot;
}

__declspec( noinline ) float Vec3LengthSq( const vec3_t v )
{
    float lengthSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    return lengthSq;
}

__declspec( noinline ) float Vec3Length( const vec3_t v )
{
    float length = I_sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
    return length;
}

__declspec( noinline ) void Vec3Cross( const vec3_t v0, const vec3_t v1, vec3_t cross )
{
    Assert( v0 != cross );
    Assert( v1 != cross );

    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

__declspec( noinline ) float Vec3DistanceSq( const vec3_t v0, const vec3_t v1 )
{
    vec3_t delta;

    Vec3Sub( v1, v0, delta );
    return Vec3LengthSq( delta );
}

__declspec( noinline ) float Vec3Distance( const vec3_t v0, const vec3_t v1 )
{
    vec3_t delta;

    Vec3Sub( v1, v0, delta );
    return Vec3Length( delta );
}

__declspec( noinline ) float Vec3Normalize( vec3_t v )
{
    float length;
    float invLength;

    length    = I_sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
    invLength = 1.0f / I_fsel( -length, 1.0f, length );

    v[0] = v[0] * invLength;
    v[1] = v[1] * invLength;
    v[2] = v[2] * invLength;

    return length;
}

__declspec( noinline ) void Vec3Mid( const vec3_t v0, const vec3_t v1, vec3_t out )
{
    out[0] = ( v0[0] + v1[0] ) * 0.5f;
    out[1] = ( v0[1] + v1[1] ) * 0.5f;
    out[2] = ( v0[2] + v1[2] ) * 0.5f;
}

__declspec( noinline ) void Vec3Lerp( const vec3_t from, const vec3_t to, float t, vec3_t out )
{
    out[0] = ( to[0] - from[0] ) * t + from[0];
    out[1] = ( to[1] - from[1] ) * t + from[1];
    out[2] = ( to[2] - from[2] ) * t + from[2];
}

__declspec( noinline ) void Vec3Combine( float s0, const vec3_t v0, float s1, const vec3_t v1, vec3_t out )
{
    out[0] = s0 * v0[0] + s1 * v1[0];
    out[1] = s0 * v0[1] + s1 * v1[1];
    out[2] = s0 * v0[2] + s1 * v1[2];
}

__declspec( noinline ) void Vec3MadCombine( const vec3_t base, float s0, const vec3_t v0,
                            float s1, const vec3_t v1, vec3_t out )
{
    out[0] = s0 * v0[0] + base[0] + s1 * v1[0];
    out[1] = s0 * v0[1] + base[1] + s1 * v1[1];
    out[2] = s0 * v0[2] + base[2] + s1 * v1[2];
}

__declspec( noinline ) bool Vec3Compare( const vec3_t v0, const vec3_t v1 )
{
    if ( v0[0] != v1[0] )
        return false;

    if ( v0[1] != v1[1] )
        return false;

    if ( v0[2] != v1[2] )
        return false;

    return true;
}

__declspec( noinline ) bool Vec3CompareEpsilon( const vec3_t v0, const vec3_t v1, float epsilon )
{
    return VecNCompareEpsilon( v0, v1, epsilon, 3 ) != 0;
}

__declspec( noinline ) bool Vec3Equal( const vec3_t v0, const vec3_t v1 )
{
    return VecNCompareEpsilon( v0, v1, VEC_EPSILON, 3 ) != 0;
}

__declspec( noinline ) bool Vec3IsNormalized( const vec3_t v )
{
    return I_fabs( Vec3LengthSq( v ) - 1.0f ) < NORMALIZE_EPSILON;
}

__declspec( noinline ) void Vec3SetPerpXY( const vec3_t v, vec3_t out )
{
    Vec3Set( out, v[1], -v[0], 0.0f );
}

__declspec( noinline ) void Vec4Set( vec4_t v, float x, float y, float z, float w )
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

__declspec( noinline ) void Vec4Copy( const vec4_t from, vec4_t to )
{
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
    to[3] = from[3];
}

__declspec( noinline ) void Vec4Scale( const vec4_t v, float scale, vec4_t out )
{
    out[0] = scale * v[0];
    out[1] = scale * v[1];
    out[2] = scale * v[2];
    out[3] = scale * v[3];
}

__declspec( noinline ) void Vec4Negate( const vec4_t v, vec4_t out )
{
    out[0] = -v[0];
    out[1] = -v[1];
    out[2] = -v[2];
    out[3] = -v[3];
}

__declspec( noinline ) void Vec4Lerp( const vec4_t from, const vec4_t to, float t, vec4_t out )
{
    out[0] = ( to[0] - from[0] ) * t + from[0];
    out[1] = ( to[1] - from[1] ) * t + from[1];
    out[2] = ( to[2] - from[2] ) * t + from[2];
    out[3] = ( to[3] - from[3] ) * t + from[3];
}

__declspec( noinline ) float Vec4Dot( const vec4_t v0, const vec4_t v1 )
{
    float dot = v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2] + v0[3] * v1[3];
    return dot;
}

__declspec( noinline ) float Vec4LengthSq( const vec4_t v )
{
    float lengthSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
    return lengthSq;
}

__declspec( noinline ) bool Vec4Compare( const vec4_t v0, const vec4_t v1 )
{
    if ( v0[0] != v1[0] )
        return false;

    if ( v0[1] != v1[1] )
        return false;

    if ( v0[2] != v1[2] )
        return false;

    if ( v0[3] != v1[3] )
        return false;

    return true;
}

__declspec( noinline ) bool Vec4IsNormalized( const vec4_t v )
{
    return I_fabs( Vec4LengthSq( v ) - 1.0f ) < NORMALIZE_EPSILON;
}

__declspec( noinline ) bool Vec2CompareEpsilon( const vec2_t v0, const vec2_t v1, float epsilon )
{
    return VecNCompareEpsilon( v0, v1, epsilon, 2 ) != 0;
}

__declspec( noinline ) bool Vec4CompareEpsilon( const vec4_t v0, const vec4_t v1, float epsilon )
{
    return VecNCompareEpsilon( v0, v1, epsilon, 4 ) != 0;
}

__declspec( noinline ) void Vec4bSet( byte *out, byte a, byte b, byte c, byte d )
{
    *( unsigned int * )out = ( unsigned int )a
                           | ( ( unsigned int )b << 8 )
                           | ( ( unsigned int )c << 16 )
                           | ( ( unsigned int )d << 24 );
}

__declspec( noinline ) void SetVertexColor( byte *out, byte r, byte g, byte b, byte a )
{
    *( unsigned int * )out = ( unsigned int )b
                           | ( ( unsigned int )g << 8 )
                           | ( ( unsigned int )r << 16 )
                           | ( ( unsigned int )a << 24 );
}

__declspec( noinline ) void PackVertexColor( const byte *rgba, byte *out )
{
    SetVertexColor( out, rgba[0], rgba[1], rgba[2], rgba[3] );
}

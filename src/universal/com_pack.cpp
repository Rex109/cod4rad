/* Original: c:\trees\cod3\cod3src\src\universal\com_pack.cpp */

#include "cod4rad.h"
#include "com_pack.h"

#include <string.h>


#define UNITVEC_SCALE_BIAS  ( -192.0 )
#define UNITVEC_SCALE_DIV   32385.0
#define UNITVEC_MID         127.0


/* Vec3UnpackUnitVec  0x00441b60 */
void Vec3UnpackUnitVec( unsigned int packed, vec3_t out )
{
    float scale;

    scale = ( float )( ( ( int )( packed >> 24 ) - UNITVEC_SCALE_BIAS ) / UNITVEC_SCALE_DIV );

    out[0] = ( float )( ( ( int )( ( packed       ) & 0xff ) - UNITVEC_MID ) * scale );
    out[1] = ( float )( ( ( int )( ( packed >>  8 ) & 0xff ) - UNITVEC_MID ) * scale );
    out[2] = ( float )( ( ( int )( ( packed >> 16 ) & 0xff ) - UNITVEC_MID ) * scale );
}

/* Vec3PackUnitVec  0x00441ad0 */
unsigned int Vec3PackUnitVec( const vec3_t v )
{
    byte out[4];

    out[0] = ( byte )( int )( v[0] * 127.0f + 127.5f );
    out[1] = ( byte )( int )( v[1] * 127.0f + 127.5f );
    out[2] = ( byte )( int )( v[2] * 127.0f + 127.5f );
    out[3] = 0x3f;

    return *( const unsigned int * )out;
}

static unsigned short PackTexCoordHalf( float value )
{
    int bits;
    int high;
    int mantissa;

    bits = *( const int * )&value;
    high = ( bits >> 16 ) & 0xc000;

    mantissa  = ( int )( ( unsigned int )( bits + bits ) ^ 0x80003fffu );
    mantissa >>= 14;

    if ( mantissa >= 0x3fff )
        mantissa = 0x3fff;
    else if ( mantissa <= ( int )0xffffc000 )
        mantissa = ( int )0xffffc000;

    return ( unsigned short )( ( mantissa & 0x3fff ) | high );
}

/* Vec2PackTexCoords  0x00442150 */
unsigned int Vec2PackTexCoords( const float *uv )
{
    return ( ( unsigned int )PackTexCoordHalf( uv[0] ) << 16 )
         +   ( unsigned int )PackTexCoordHalf( uv[1] );
}

static float UnpackTexCoordHalf( unsigned int half )
{
    int   bits;
    float result;

    if ( !half )
        return 0.0f;

    bits  = ( int )( ( half & 0x3fff ) << 14 );
    bits -= ( int )( ~( half << 14 ) & 0x10000000 );
    bits ^= 0x80000001;
    bits  = ( int )( ( unsigned int )bits >> 1 );
    bits |= ( int )( ( half & 0xffff8000 ) << 16 );

    memcpy( &result, &bits, sizeof( result ) );
    return result;
}

/* Vec2UnpackTexCoords  0x004421f0 */
void Vec2UnpackTexCoords( unsigned int packed, float *out )
{
    out[0] = UnpackTexCoordHalf( packed >> 16 );
    out[1] = UnpackTexCoordHalf( packed & 0xffff );
}

/* Original: c:\trees\cod3\cod3src\cod2rad\lightmap_bleed.cpp */

#include "cod4rad.h"
#include "lightmap_bleed.h"
#include "geometry.h"
#include "lighting.h"
#include "progress.h"

#include <string.h>
#include <new>


static const byte bleedDirBits[BLEED_DIR_COUNT] =
{
    BLEED_NORTHWEST, BLEED_NORTH, BLEED_NORTHEAST,
    BLEED_WEST,      0,           BLEED_EAST,
    BLEED_SOUTHWEST, BLEED_SOUTH, BLEED_SOUTHEAST
};

static const float bleedWeights[BLEED_DIR_COUNT] =
{
    1.0f / 64.0f,  6.0f / 64.0f,  1.0f / 64.0f,
    6.0f / 64.0f, 36.0f / 64.0f,  6.0f / 64.0f,
    1.0f / 64.0f,  6.0f / 64.0f,  1.0f / 64.0f
};

static const byte bleedQuadrant[4] =
{
    BLEED_NORTHWEST | BLEED_NORTH | BLEED_WEST,
    BLEED_NORTHEAST | BLEED_NORTH | BLEED_EAST,
    BLEED_SOUTHWEST | BLEED_SOUTH | BLEED_WEST,
    BLEED_SOUTHEAST | BLEED_SOUTH | BLEED_EAST
};


static byte *lmapTexelMasks;    /* 0x13063dd0 */
static byte *lmapSubMasks;      /* 0x13063dd4 */


/* Bleed_SetMask  0x00416d10 */
static void Bleed_SetMask( int lmapIndex, byte *masks, int width, int height,
                           float s, float t )
{
    int maskS = ( int )s;
    int maskT = ( int )t;
    int quadrant;

    Assertx( ( unsigned )maskS < ( unsigned )width,
             "s doesn't index lmapWidth\n\t%i not in [0, %i)", maskS, width );
    Assertx( ( unsigned )maskT < ( unsigned )height,
             "t doesn't index lmapHeight\n\t%i not in [0, %i)", maskT, height );

    quadrant = ( s - maskS >= 0.5f ) + ( t - maskT >= 0.5f ) * 2;

    masks[( lmapIndex * height + maskT ) * width + maskS] |= bleedQuadrant[quadrant];
}

/* Bleed_MarkSample  0x00416dd0 */
static void Bleed_MarkSample( float area, const vec2_t center, const vec2_t *coords,
                              int vertCount, void *userData, int cellIndex )
{
    int lmapIndex = *( const byte * )userData;

    Bleed_SetMask( lmapIndex, lmapSubMasks, LMAP_WIDTH_MAX, LMAP_HEIGHT_MAX,
                   ( float )( center[0] * 2.0 ), ( float )( center[1] * 2.0 ) );

    Bleed_SetMask( lmapIndex, lmapTexelMasks, LMAP_WIDTH_MIN, LMAP_HEIGHT_MIN,
                   center[0], center[1] );
}

/* Bleed_FindBleeding  0x00416eb0 */
void Bleed_FindBleeding( int threads )
{
    Geo_ForEachSample( Bleed_MarkSample, 2, threads );
}

/* Bleed_AllocMasks  0x00416e50 */
void Bleed_AllocMasks( int lmapCount )
{
    int bytes = lmapCount * ( LMAP_WIDTH_MIN * LMAP_HEIGHT_MIN
                            + LMAP_WIDTH_MAX * LMAP_HEIGHT_MAX );

    lmapTexelMasks = new ( std::nothrow ) byte[bytes];

    if ( !lmapTexelMasks )
        Error( "Out of memory trying to allocate lightmap bleed info (%i bytes)\n", bytes );

    memset( lmapTexelMasks, 0, bytes );

    lmapSubMasks = lmapTexelMasks + lmapCount * LMAP_WIDTH_MIN * LMAP_HEIGHT_MIN;
}

/* Bleed_SampleWeight  0x00416ed0 */
static float Bleed_SampleWeight( const byte *masks, int width, int height,
                                 int lmapIndex, int s, int t, int maskS, int maskT )
{
    int  index = BLEED_DIR_INDEX( s - maskS, t - maskT );
    byte mask  = masks[( lmapIndex * height + maskT ) * width + maskS];

    if ( !( bleedDirBits[index] & mask ) )
        return 0.0f;

    return bleedWeights[index];
}

/* Bleed_SubSampleWeight  0x00416f20 */
float Bleed_SubSampleWeight( int lmapIndex, int s, int t, int maskS, int maskT )
{
    return Bleed_SampleWeight( lmapSubMasks, LMAP_WIDTH_MAX, LMAP_HEIGHT_MAX,
                               lmapIndex, s, t, maskS, maskT );
}

/* Bleed_TexelWeight  0x00416f80 */
float Bleed_TexelWeight( int lmapIndex, int s, int t, int maskS, int maskT )
{
    return Bleed_SampleWeight( lmapTexelMasks, LMAP_WIDTH_MIN, LMAP_HEIGHT_MIN,
                               lmapIndex, s, t, maskS, maskT );
}

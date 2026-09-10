/* Original: c:\trees\cod3\cod3src\cod2rad\lightmap_bleed.cpp */

#ifndef LIGHTMAP_BLEED_H
#define LIGHTMAP_BLEED_H

#include "q_shared.h"
#include "com_vector.h"


#define BLEED_NORTH         0x01
#define BLEED_NORTHEAST     0x02
#define BLEED_EAST          0x04
#define BLEED_SOUTHEAST     0x08
#define BLEED_SOUTH         0x10
#define BLEED_SOUTHWEST     0x20
#define BLEED_WEST          0x40
#define BLEED_NORTHWEST     0x80

#define BLEED_DIR_COUNT     9
#define BLEED_DIR_INDEX( ds, dt )   ( ( ds ) + 1 + ( ( dt ) + 1 ) * 3 )


void  Bleed_AllocMasks( int lmapCount );        /* 0x00416e50 */

void  Bleed_FindBleeding( int threads );        /* 0x00416eb0 */

float Bleed_TexelWeight( int lmapIndex, int s, int t,
                         int maskS, int maskT );        /* 0x00416f80 */
float Bleed_SubSampleWeight( int lmapIndex, int s, int t,
                             int maskS, int maskT );    /* 0x00416f20 */

#endif

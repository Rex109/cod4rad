/* Original: ..\src\universal\surfaceflags.cpp */

#ifndef SURFACEFLAGS_H
#define SURFACEFLAGS_H

#include "q_shared.h"

#define SURF_TYPESHIFT  20
#define SURF_TYPEBITS   0x01F00000

#define SURF_TYPEINDEX( surfaceFlags ) \
    ( ( byte )( ( ( surfaceFlags ) & SURF_TYPEBITS ) >> SURF_TYPESHIFT ) )

typedef enum
{
    SURF_TYPE_DEFAULT,
    SURF_TYPE_BARK,
    SURF_TYPE_BRICK,
    SURF_TYPE_CARPET,
    SURF_TYPE_CLOTH,
    SURF_TYPE_CONCRETE,
    SURF_TYPE_DIRT,
    SURF_TYPE_FLESH,
    SURF_TYPE_FOLIAGE,
    SURF_TYPE_GLASS,
    SURF_TYPE_GRASS,
    SURF_TYPE_GRAVEL,
    SURF_TYPE_ICE,
    SURF_TYPE_METAL,
    SURF_TYPE_MUD,
    SURF_TYPE_PAPER,
    SURF_TYPE_PLASTER,
    SURF_TYPE_ROCK,
    SURF_TYPE_SAND,
    SURF_TYPE_SNOW,
    SURF_TYPE_WATER,
    SURF_TYPE_WOOD,
    SURF_TYPE_ASPHALT,
    SURF_TYPE_CERAMIC,
    SURF_TYPE_PLASTIC,
    SURF_TYPE_RUBBER,
    SURF_TYPE_CUSHION,
    SURF_TYPE_FRUIT,
    SURF_TYPE_PAINTED_METAL,

    SURF_TYPE_NUM
} surfaceType_t;

#define SURF_NODAMAGE       0x00000001
#define SURF_SLICK          0x00000002
#define SURF_SKY            0x00000004
#define SURF_LADDER         0x00000008
#define SURF_NOIMPACT       0x00000010
#define SURF_NOMARKS        0x00000020
#define SURF_NODRAW         0x00000080
#define SURF_NOPENETRATE    0x00000100
#define SURF_NOLIGHTMAP     0x00000400
#define SURF_NOSTEPS        0x00002000
#define SURF_NONSOLID       0x00004000
#define SURF_NODLIGHT       0x00020000
#define SURF_NOCASTSHADOW   0x00040000
#define SURF_MANTLEON       0x02000000
#define SURF_MANTLEOVER     0x04000000
#define SURF_PORTAL         ( ( int )0x80000000 )

#define CONTENTS_NONE           0x00000000
#define CONTENTS_SOLID          0x00000001
#define CONTENTS_FOLIAGE        0x00000002
#define CONTENTS_NONCOLLIDING   0x00000004
#define CONTENTS_GLASS          0x00000010
#define CONTENTS_WATER          0x00000020
#define CONTENTS_CANSHOOTCLIP   0x00000040
#define CONTENTS_MISSILECLIP    0x00000080
#define CONTENTS_VEHICLECLIP    0x00000200
#define CONTENTS_ITEMCLIP       0x00000400
#define CONTENTS_SKY            0x00000800
#define CONTENTS_AI_NOSIGHT     0x00001000
#define CONTENTS_CLIPSHOT       0x00002000
#define CONTENTS_PLAYERCLIP     0x00010000
#define CONTENTS_MONSTERCLIP    0x00020000
#define CONTENTS_MANTLE         0x01000000
#define CONTENTS_DETAIL         0x08000000
#define CONTENTS_STRUCTURAL     0x10000000
#define CONTENTS_TRANSLUCENT    0x20000000
#define CONTENTS_NODROP         ( ( int )0x80000000 )

#define TOOLFLAG_ORIGIN         0x00000004
#define TOOLFLAG_PHYSICSGEOM    0x00000400
#define TOOLFLAG_LIGHTPORTAL    0x00002000

typedef struct
{
    const char *name;          /* +0x00 */
    int         clearSolid;    /* +0x04 */
    int         surfaceFlags;  /* +0x08 */
    int         contents;      /* +0x0c */
    int         toolFlags;     /* +0x10 */
} infoParm_t;

extern infoParm_t infoParms[];                              /* 0x00529148 */

const char *SURF_NameFromTypeIndex( int iTypeIndex );       /* 0x0043a8e0 */
int         SURF_TypeIndexFromName( const char *name );     /* 0x0043a860 */

#endif

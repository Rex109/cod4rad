/* Original: ..\src\universal\surfaceflags.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "surfaceflags.h"

/* infoParms  0x00529148 */
infoParm_t infoParms[] =
{

    { "bark",            0,          SURF_TYPE_BARK          << SURF_TYPESHIFT, 0,                    0 },
    { "brick",           0,          SURF_TYPE_BRICK         << SURF_TYPESHIFT, 0,                    0 },
    { "carpet",          0,          SURF_TYPE_CARPET        << SURF_TYPESHIFT, 0,                    0 },
    { "cloth",           0,          SURF_TYPE_CLOTH         << SURF_TYPESHIFT, 0,                    0 },
    { "concrete",        0,          SURF_TYPE_CONCRETE      << SURF_TYPESHIFT, 0,                    0 },
    { "dirt",            0,          SURF_TYPE_DIRT          << SURF_TYPESHIFT, 0,                    0 },
    { "flesh",           0,          SURF_TYPE_FLESH         << SURF_TYPESHIFT, 0,                    0 },
    { "foliage",         1,          SURF_TYPE_FOLIAGE       << SURF_TYPESHIFT, CONTENTS_FOLIAGE,     0 },
    { "glass",           1,          SURF_TYPE_GLASS         << SURF_TYPESHIFT, CONTENTS_GLASS,       0 },
    { "grass",           0,          SURF_TYPE_GRASS         << SURF_TYPESHIFT, 0,                    0 },
    { "gravel",          0,          SURF_TYPE_GRAVEL        << SURF_TYPESHIFT, 0,                    0 },
    { "ice",             0,          SURF_TYPE_ICE           << SURF_TYPESHIFT, 0,                    0 },
    { "metal",           0,          SURF_TYPE_METAL         << SURF_TYPESHIFT, 0,                    0 },
    { "mud",             0,          SURF_TYPE_MUD           << SURF_TYPESHIFT, 0,                    0 },
    { "paper",           0,          SURF_TYPE_PAPER         << SURF_TYPESHIFT, 0,                    0 },
    { "plaster",         0,          SURF_TYPE_PLASTER       << SURF_TYPESHIFT, 0,                    0 },
    { "rock",            0,          SURF_TYPE_ROCK          << SURF_TYPESHIFT, 0,                    0 },
    { "sand",            0,          SURF_TYPE_SAND          << SURF_TYPESHIFT, 0,                    0 },
    { "snow",            0,          SURF_TYPE_SNOW          << SURF_TYPESHIFT, 0,                    0 },
    { "water",           1,          SURF_TYPE_WATER         << SURF_TYPESHIFT, CONTENTS_WATER,       0 },
    { "wood",            0,          SURF_TYPE_WOOD          << SURF_TYPESHIFT, 0,                    0 },
    { "asphalt",         0,          SURF_TYPE_ASPHALT       << SURF_TYPESHIFT, 0,                    0 },
    { "ceramic",         0,          SURF_TYPE_CERAMIC       << SURF_TYPESHIFT, 0,                    0 },
    { "plastic",         0,          SURF_TYPE_PLASTIC       << SURF_TYPESHIFT, 0,                    0 },
    { "rubber",          0,          SURF_TYPE_RUBBER        << SURF_TYPESHIFT, 0,                    0 },
    { "cushion",         0,          SURF_TYPE_CUSHION       << SURF_TYPESHIFT, 0,                    0 },
    { "fruit",           0,          SURF_TYPE_FRUIT         << SURF_TYPESHIFT, 0,                    0 },
    { "paintedmetal",    0,          SURF_TYPE_PAINTED_METAL << SURF_TYPESHIFT, 0,                    0 },

    { "opaqueglass",     0,          SURF_TYPE_GLASS         << SURF_TYPESHIFT, 0,                    0 },

    { "clipmissile",     1,          0,                  CONTENTS_MISSILECLIP,     0 },
    { "ai_nosight",      1,          0,                  CONTENTS_AI_NOSIGHT,      0 },
    { "clipshot",        1,          0,                  CONTENTS_CLIPSHOT,        0 },
    { "playerclip",      1,          0,                  CONTENTS_PLAYERCLIP,      0 },
    { "monsterclip",     1,          0,                  CONTENTS_MONSTERCLIP,     0 },
    { "vehicleclip",     1,          0,                  CONTENTS_VEHICLECLIP,     0 },
    { "itemclip",        1,          0,                  CONTENTS_ITEMCLIP,        0 },
    { "nodrop",          1,          0,                  CONTENTS_NODROP,          0 },
    { "nonsolid",        1,          SURF_NONSOLID,      0,                        0 },

    { "trans",           0,          0,                  CONTENTS_TRANSLUCENT,     0 },
    { "noncolliding",    1,          0,                  CONTENTS_NONCOLLIDING,    0 },
    { "detail",          0,          0,                  CONTENTS_DETAIL,          0 },
    { "structural",      0,          0,                  CONTENTS_STRUCTURAL,      0 },
    { "portal",          1,          SURF_PORTAL,        0,                        0 },
    { "canshootclip",    0,          0,                  CONTENTS_CANSHOOTCLIP,    0 },
    { "origin",          1,          0,                  0,                        TOOLFLAG_ORIGIN },
    { "sky",             0,          SURF_SKY,           CONTENTS_SKY,             0 },
    { "nocastshadow",    0,          SURF_NOCASTSHADOW,  0,                        0 },
    { "physicsGeom",     0,          0,                  0,                        TOOLFLAG_PHYSICSGEOM },
    { "lightPortal",     0,          0,                  0,                        TOOLFLAG_LIGHTPORTAL },

    { "slick",           0,          SURF_SLICK,         0,                        0 },
    { "noimpact",        0,          SURF_NOIMPACT,      0,                        0 },
    { "nomarks",         0,          SURF_NOMARKS,       0,                        0 },
    { "nopenetrate",     0,          SURF_NOPENETRATE,   0,                        0 },
    { "ladder",          0,          SURF_LADDER,        0,                        0 },
    { "nodamage",        0,          SURF_NODAMAGE,      0,                        0 },
    { "mantleOn",        0,          SURF_MANTLEON,      CONTENTS_MANTLE,          0 },
    { "mantleOver",      0,          SURF_MANTLEOVER,    CONTENTS_MANTLE,          0 },
    { "nosteps",         0,          SURF_NOSTEPS,       0,                        0 },
    { "nodraw",          0,          SURF_NODRAW,        0,                        0 },
    { "nolightmap",      0,          SURF_NOLIGHTMAP,    0,                        0 },
    { "nodlight",        0,          SURF_NODLIGHT,      0,                        0 }
};

/* SURF_TypeIndexFromName  0x0043a860 */
int SURF_TypeIndexFromName( const char *name )
{
    int i;

    if ( !Q_stricmp( name, "default" ) )
        return SURF_TYPE_DEFAULT;

    for ( i = 0; i < SURF_TYPE_NUM - 1; i++ )
    {
        if ( !Q_stricmp( name, infoParms[i].name ) )
            return SURF_TYPEINDEX( infoParms[i].surfaceFlags );
    }
    return -1;
}

/* SURF_NameFromTypeIndex  0x0043a8e0 */
const char *SURF_NameFromTypeIndex( int iTypeIndex )
{
    if ( iTypeIndex > SURF_TYPE_DEFAULT && iTypeIndex < SURF_TYPE_NUM )
    {
        Assert( SURF_TYPEINDEX( infoParms[iTypeIndex - 1].surfaceFlags ) == iTypeIndex );
        return infoParms[iTypeIndex - 1].name;
    }
    return "default";
}

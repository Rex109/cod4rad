#ifndef R_MATERIAL_H
#define R_MATERIAL_H

#include "q_shared.h"


#ifndef COD4MAP_MATERIAL_T_DEFINED
#define COD4MAP_MATERIAL_T_DEFINED
typedef struct
{
    int            nameOffset;      /* +0x00 */
    int            imageNameOffset; /* +0x04 */
    byte           techSetFlags;    /* +0x08 */
    byte           sortOrder;       /* +0x09 */
    byte           pad0a[0x08];     /* +0x0a */
    unsigned short toolFlags;       /* +0x12 */
    byte           pad14[0x04];     /* +0x14 */
    unsigned short textureWidth;    /* +0x18 */
    unsigned short textureHeight;   /* +0x1a */
    float          subdivisions;    /* +0x1c */
    int            surfaceFlags;    /* +0x20 */
    int            contentFlags;    /* +0x24 */
    int            pad28;           /* +0x28 */
    unsigned int   layerFlags;      /* +0x2c */
    unsigned short textureCount;    /* +0x30 */
    unsigned short constantCount;   /* +0x32 */
    int            techSetNameOffset;  /* +0x34 */
    int            textureTableOffset; /* +0x38 */
    int            constantTableOffset;/* +0x3c */
} Material_t;
#endif

#endif

/* Original: c:\trees\cod3\cod3src\cod2rad\maskedmaterial.cpp */

#ifndef MASKEDMATERIAL_H
#define MASKEDMATERIAL_H

#include "q_shared.h"
#include "r_material.h"
#include "materials.h"


#define MAX_MASKED_MATERIALS    0x400


typedef struct
{
    const Material_t *material;     /* +0x00 */
    TechSet_t        *techSet;      /* +0x04 */
    int               usage;        /* +0x08 */
    int               maskWidth;    /* +0x0c */
    int               maskHeight;   /* +0x10 */
    const byte       *mask;         /* +0x14 */
    const byte       *colorMask;    /* +0x18 */
} MskMaterial_t;                    /* sizeof == 0x1c */


extern int           mskMtlCount;                       /* 0x13063dd8 */
extern MskMaterial_t mskMtls[MAX_MASKED_MATERIALS];     /* 0x13063de0 */


MskMaterial_t *FindMaskMaterial( const char *mtlName, int usage );  /* 0x00418b90 */

#endif

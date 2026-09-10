/* Original: c:\trees\cod3\cod3src\src\xanim\xmodel_load_obj.cpp */

#ifndef XMODEL_LOAD_OBJ_H
#define XMODEL_LOAD_OBJ_H

#include "q_shared.h"
#include "com_vector.h"


#define XMODEL_CONFIG_NAME_SIZE     1024

typedef struct
{
    char   filename[XMODEL_CONFIG_NAME_SIZE];   /* +0x000 */
    float  lodDist;                             /* +0x400 */
} XModelConfigEntry_t;                          /* sizeof == 0x404 */

typedef struct
{
    XModelConfigEntry_t entries[4];             /* +0x0000 */
    vec3_t              mins;                   /* +0x1010 */
    vec3_t              maxs;                   /* +0x101c */
    int                 collLod;                /* +0x1028 */
    byte                flags;                  /* +0x102c */
    char                name[XMODEL_CONFIG_NAME_SIZE]; /* +0x102d */
} XModelConfig_t;

#endif

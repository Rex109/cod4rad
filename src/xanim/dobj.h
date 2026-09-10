/* Original: c:\trees\cod3\cod3src\src\xanim\dobj.cpp */

#ifndef DOBJ_H
#define DOBJ_H

#include "q_shared.h"
#include "xmodel.h"


#define DOBJ_MAX_PARTS      128

#define DOBJ_MAX_MODELS     32


typedef struct
{
    XModel_t       *model;      /* +0x00 */
    unsigned short  boneName;   /* +0x04 */
    byte            flags;      /* +0x06 */
    byte            pad07;      /* +0x07 */
} DObjModel_t;                  /* sizeof == 0x08 */

typedef struct
{
    void           *ptr00;              /* +0x00 */
    unsigned short  skelName;           /* +0x04 */
    unsigned short  pad06;              /* +0x06 */
    byte            skelNameLen;        /* +0x08 */
    byte            numModels;          /* +0x09 */
    byte            numBones;           /* +0x0a */
    byte            pad0b;              /* +0x0b */
    int             ignoreCollision;    /* +0x0c */
    int             pad10;              /* +0x10 */
    int             partBits[3][4];     /* +0x14 */
    const DObjModel_t *dobjModels;      /* +0x44 */
    DObjAnimMat_t  *animMats;           /* +0x48 */
    float           radius;             /* +0x4c */
    int             pad50[4];           /* +0x50 */
    XModel_t      **models;             /* +0x60 */
} DObj_t;                               /* sizeof == 0x64 */


void DObjAnimMatToAxis( const DObjAnimMat_t *mat, float axis[3][3] ); /* 0x004220a0 */

void DObj_Create( const DObjModel_t *dobjModels, unsigned numModels, void *ptr00,
                  DObj_t *obj, unsigned short pad06 );          /* 0x00422ea0 */

void DObj_Free( DObj_t *obj );                                  /* 0x00423030 */

void DObj_SetAnimMats( DObj_t *obj, DObjAnimMat_t *animMats,
                       const DObjModel_t *dobjModels );         /* 0x00426fa0 */

void DObj_SetPartBits( DObj_t *obj, const int *partBits );      /* 0x00425d70 */

int  DObj_GetSurfCount( const DObj_t *obj, int *partBits,
                        const char *lods );                     /* 0x00426470 */

XSurface_t *DObj_GetSurface( const DObj_t *obj, int modelIndex, int surfIndex,
                             int lod );                         /* 0x00423600 */

const char *DObj_GetSurfaceMaterialName( const DObj_t *obj, int modelIndex,
                                         int surfIndex, int lod ); /* 0x00423680 */

#endif

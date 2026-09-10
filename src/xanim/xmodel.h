/* Original: c:\trees\cod3\cod3src\src\xanim\xmodel.cpp */

#ifndef XMODEL_H
#define XMODEL_H

#include "q_shared.h"
#include "com_vector.h"
#include "r_xsurface.h"


#define MAX_XMODEL_LODS     4

#define XMODEL_VERSION      25

#define ASSET_TYPE_XMODELSURFS  3
#define ASSET_TYPE_XMODELPARTS  4
#define ASSET_TYPE_XMODEL       5

#define XMODEL_MAX_BONES    128


typedef struct
{
    vec4_t          quat;               /* +0x00 */
    vec3_t          trans;              /* +0x10 */
    float           transWeight;        /* +0x1c */
} DObjAnimMat_t;                        /* sizeof == 0x20 */

typedef struct
{
    float           dist;               /* +0x00 */
    unsigned short  numSurfaces;        /* +0x04 */
    unsigned short  surfIndex;          /* +0x06 */
    int             partBits[4];        /* +0x08 */
} XModelLodInfo_t;                      /* sizeof == 0x18 */

typedef struct
{
    vec3_t          mins;               /* +0x00 */
    vec3_t          maxs;               /* +0x0c */
    vec3_t          midPoint;           /* +0x18 */
    float           radiusSquared;      /* +0x24 */
} XBoneInfo_t;                          /* sizeof == 0x28 */

typedef struct
{
    vec4_t          plane;              /* +0x00 */
    vec4_t          svec;               /* +0x10 */
    vec4_t          tvec;               /* +0x20 */
} XModelCollTri_t;                      /* sizeof == 0x30 */

typedef struct
{
    XModelCollTri_t *collTris;          /* +0x00 */
    int              numCollTris;       /* +0x04 */
    vec3_t           mins;              /* +0x08 */
    vec3_t           maxs;              /* +0x14 */
    int              boneIdx;           /* +0x20 */
    int              contents;          /* +0x24 */
    int              surfFlags;         /* +0x28 */
} XModelCollSurf_t;                     /* sizeof == 0x2c */

typedef struct XModel_s
{
    const char      *name;              /* +0x00 */
    byte             numBones;          /* +0x04 */
    byte             numRootBones;      /* +0x05 */
    byte             numSurfaces;       /* +0x06 */
    byte             lodRampType;       /* +0x07 */
    unsigned short  *boneNames;         /* +0x08 */
    byte            *parentList;        /* +0x0c */
    short           *quats;             /* +0x10 */
    float           *trans;             /* +0x14 */
    byte            *partClassification;/* +0x18 */
    DObjAnimMat_t   *baseMat;           /* +0x1c */
    XSurface_t      *surfs;             /* +0x20 */
    char           **materialNames;     /* +0x24 */
    XModelLodInfo_t  lodInfo[MAX_XMODEL_LODS]; /* +0x28 */
    XModelCollSurf_t *collSurfs;        /* +0x88 */
    int              numCollSurfs;      /* +0x8c */
    int              contents;          /* +0x90 */
    XBoneInfo_t     *boneInfo;          /* +0x94 */
    float            radius;            /* +0x98 */
    vec3_t           mins;              /* +0x9c */
    vec3_t           maxs;              /* +0xa8 */
    short            numLods;           /* +0xb4 */
    short            collLod;           /* +0xb6 */
    void            *streamInfo;        /* +0xb8 */
    int              memUsage;          /* +0xbc */
    byte             flags;             /* +0xc0 */
    byte             badLoad;           /* +0xc1 */
    byte             pad0c2[2];         /* +0xc2 */
    void            *physPreset;        /* +0xc4 */
    void            *physCollmap;       /* +0xc8 */
} XModel_t;                             /* sizeof == 0xcc */

typedef struct
{
    byte             numBones;          /* +0x00 */
    byte             numRootBones;      /* +0x01 */
    byte             pad02[2];          /* +0x02 */
    unsigned short  *boneNames;         /* +0x04 */
    byte            *parentList;        /* +0x08 */
    short           *quats;             /* +0x0c */
    float           *trans;             /* +0x10 */
    byte            *partClassification;/* +0x14 */
    DObjAnimMat_t   *baseMat;           /* +0x18 */
} XModelParts_t;                        /* sizeof == 0x1c */

typedef struct
{
    XSurface_t      *surfs;             /* +0x00 */
    int              partBits[4];       /* +0x04 */
} XModelSurfs_t;                        /* sizeof == 0x14 */


typedef void *( *XModelAllocFunc_t )( int size );


XModel_t *XModel_Register( const char *name, XModelAllocFunc_t allocFunc,
                           XModelAllocFunc_t collAllocFunc );   /* 0x0042ae80 */

XModel_t *XModel_Load( const char *name, XModelAllocFunc_t allocFunc,
                       XModelAllocFunc_t collAllocFunc );       /* 0x0042dc00 */

void  XModelPartsFree( XModelParts_t *modelParts );             /* 0x0042acf0 */

const char *XModelName( const XModel_t *model );                 /* 0x00430260 */
int   XModelBadLoad( const XModel_t *model );                   /* 0x0042acc0 */
unsigned short *XModelBoneNames( const XModel_t *model );        /* 0x0042aed0 */
float XModelRadius( const XModel_t *model );                     /* 0x0042aee0 */
void  XModelGetBounds( const XModel_t *model,
                       vec3_t mins, vec3_t maxs );               /* 0x0042aef0 */
int   XModelMemUsage( const XModel_t *model );                   /* 0x0042af40 */

int   XModelNumBones( const XModel_t *model );                   /* 0x004304f0 */
int   XModelLodRampType( const XModel_t *model );                /* 0x004304b0 */
int   XModelNumLods( const XModel_t *model );                    /* 0x004304c0 */

const XModelLodInfo_t *XModelGetLodInfo( const XModel_t *model, int lod ); /* 0x004303f0 */
int   XModelLodNumSurfaces( const XModel_t *model, int lod );    /* 0x00430450 */

int   XModelGetSurfaces( const XModel_t *model,
                         XSurface_t **surfaces, int lod );        /* 0x00430290 */
XSurface_t *XModelGetSurface( const XModel_t *model,
                              int lod, int surfIndex );           /* 0x00430380 */

void  XModelGetTransformedBounds( const XModel_t *model, const float axis[3][3],
                                  vec3_t mins, vec3_t maxs );     /* 0x0042d1a0 */

unsigned int XModelGetSamplePoints( const XModel_t *model, vec3_t *points,
                                   unsigned int count );          /* 0x0042d5a0 */

#endif

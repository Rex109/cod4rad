/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_xsurface.cpp */

#ifndef R_XSURFACE_H
#define R_XSURFACE_H

#include "q_shared.h"
#include "com_vector.h"


typedef struct
{
    vec3_t       xyz;               /* +0x00 */
    float        binormalSign;      /* +0x0c */
    unsigned int color;             /* +0x10 */
    unsigned int texCoord;          /* +0x14 */
    unsigned int normal;            /* +0x18 */
    unsigned int tangent;           /* +0x1c */
} GfxPackedVertex;                  /* sizeof == 0x20 */

typedef struct
{
    unsigned short  boneOffset;     /* +0x00 */
    unsigned short  vertCount;      /* +0x02 */
    unsigned short  triOffset;      /* +0x04 */
    unsigned short  triCount;       /* +0x06 */
    void           *collisionTree;  /* +0x08 */
} XRigidVertList;                   /* sizeof == 0x0c */

typedef struct
{
    unsigned short  vertCount[4];   /* +0x00 */
    unsigned short *vertsBlend;     /* +0x08 */
} XSurfaceVertexInfo;               /* sizeof == 0x0c */

typedef struct
{
    byte            tileMode;       /* +0x00 */
    byte            deformed;       /* +0x01 */
    unsigned short  vertCount;      /* +0x02 */
    unsigned short  triCount;       /* +0x04 */
    unsigned short  pad06;          /* +0x06 */
    unsigned short *triIndices;     /* +0x08 */
    XSurfaceVertexInfo vertInfo;    /* +0x0c */
    GfxPackedVertex *verts0;        /* +0x18 */
    int             pad1c;          /* +0x1c */
    int             vertListCount;  /* +0x20 */
    XRigidVertList *vertList;       /* +0x24 */
    int             pad28;          /* +0x28 */
    int             partBits[4];    /* +0x2c */
} XSurface_t;                       /* sizeof == 0x3c */

typedef struct
{
    vec3_t          normal;         /* +0x00 */
    unsigned int    color;          /* +0x0c */
    vec3_t          binormal;       /* +0x10 */
    float           texCoord0;      /* +0x1c */
    vec3_t          tangent;        /* +0x20 */
    float           texCoord1;      /* +0x2c */
    vec3_t          xyz;            /* +0x30 */
    byte            numWeights;     /* +0x3c */
    byte            pad3d;          /* +0x3d */
    unsigned short  boneOffset;     /* +0x3e */
} XVertUnpacked_t;                  /* sizeof == 0x40 */


int  XSurfaceGetVertCount( const XSurface_t *surface );         /* 0x00454700 */
int  XSurfaceGetTriCount( const XSurface_t *surface );          /* 0x00454730 */

void XSurfaceCopyTriIndices( const XSurface_t *surface,
                             unsigned short *dstIndices,
                             unsigned short baseVertex );       /* 0x00454760 */

void XSurfaceCopyVertexData( const XSurface_t *surface,
                             vec3_t *xyzOut,
                             float *texCoordOut,
                             vec3_t *normalOut );                /* 0x00454840 */

void XSurfaceSetupVertexes( const XVertUnpacked_t *verts,
                            GfxPackedVertex *packedVerts0,
                            GfxPackedVertex *packedVerts1,
                            int vertCount );                     /* 0x004548e0 */

#endif

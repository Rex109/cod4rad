/* Original: c:\trees\cod3\cod3src\cod2rad\geometry.cpp */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "q_shared.h"
#include "maskedmaterial.h"
#include "com_vector.h"
#include "poly2d.h"
#include "r_material.h"
#include "bspfile.h"
#include "surfaceflags.h"
#include "cm_tracebox.h"
#include "threads.h"
#include "q_shared.h"


#define GEO_MAX_BSP_NODE_BYTES  0x600000
#define GEO_MAX_BSP_LEAF_BYTES  0x200008

#define GEO_MAX_NODES           ( 32 * 1024 * 8 )

#define GEO_MAX_DRAW_VERTS      0x90000
#define GEO_MAX_TRIANGLES       0x60000
#define GEO_MAX_MODELS          0xfff

#define GEO_VERTEX_SEARCH_DEPTH 2000

#define GEO_HUNK_SIZE           0x80000
#define GEO_MAX_HUNKS           THREAD_COUNT_MAX

#define GEO_MAX_SUPERSAMPLE_ALPHA 31

#define GEO_MAX_LEAF_TRIS       16

#define GEO_MAX_SPLIT_CANDIDATES 128

#define GEO_SPLIT_EPSILON       0.001f

#define GEO_COLLISION_EPSILON   0.1f

#define GEO_BRUSH_EPSILON       0.1f

#define GEO_CONTENTS_SKIP_BRUSH 0x20000000

#define GEO_GROUND_MIN_NORMAL_Z 0.5f

#define GEO_GROUND_PROBE_OFFSET 0.1f

#define GEO_GROUND_OVER_SKY     1
#define GEO_GROUND_NONE         2

#define GEO_LIGHTMAP_NONE       0x1f

#define GEO_SKY_TRACE_DIST      262144.0

#define GEO_ALPHA_ANGLE_SCALE   ( TWO_PI / 32768.0 )

#define GEO_SWEEP_START         -131073.0f

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2
#define SIDE_CROSS  3

#define GEO_CHILD_EMPTY                 ( -1 )
#define GEO_LEAF_TO_CHILD( leafIndex )  ( -2 - ( leafIndex ) )
#define GEO_CHILD_TO_LEAF( child )      ( -2 - ( child ) )
#define GEO_CHILD_IS_LEAF( child )      ( ( child ) < GEO_CHILD_EMPTY )


typedef struct
{
    vec3_t xyz;             /* +0x00 */
    vec3_t normal;          /* +0x0c */
    int    pad18;           /* +0x18 */
    vec2_t texCoord;        /* +0x1c */
    vec2_t lmapCoord;       /* +0x24 */
    vec3_t tangent;         /* +0x2c */
    vec3_t binormal;        /* +0x38 */
} GeoVertex_t;              /* sizeof == 0x44 */

typedef struct
{
    vec3_t normal;          /* +0x00 */
    float  dist;            /* +0x0c */
} GeoPlane_t;

typedef struct
{
    GeoPlane_t plane;       /* +0x00 */
    int        children[2]; /* +0x10 */
} GeoBspNode_t;

typedef struct
{
    int triCount;           /* +0x00 */
    int firstTriRef;        /* +0x04 */
} GeoBspLeaf_t;

typedef struct
{
    vec3_t center;          /* +0x00 */
    vec3_t halfSize;        /* +0x0c */
} GeoBox_t;

typedef struct
{
    int   used;             /* +0x00 */
    int   size;             /* +0x04 */
    byte *base;             /* +0x08 */
} GeoHunk_t;

typedef struct
{
    int           indices[3];     /* +0x00 */
    short         modelIndex;     /* +0x0c */
    byte          primaryLightIndex; /* +0x0e */
    byte          lightmapIndex;  /* +0x0f */
    byte          groundType;     /* +0x10 */
    byte          pad11[3];       /* +0x11 */
    vec3_t        normal;         /* +0x14 */
    MskMaterial_t *mskMtl;        /* +0x20 */
} GeoTriangle_t;                  /* sizeof == 0x24 */

typedef struct GeoLeafLink_s
{
    struct GeoLeafLink_s      *next;        /* +0x00 */
    struct GeoCollisionNode_s *node;        /* +0x04 */
} GeoLeafLink_t;                            /* sizeof == 0x08 */

typedef struct GeoCollisionNode_s
{
    GeoLeafLink_t             *links;       /* +0x00 */
    int                        visitMark;   /* +0x04 */
    struct GeoCollisionNode_s *parent;      /* +0x08 */
    struct GeoCollisionNode_s *children[2]; /* +0x0c */
    int                        triCount;    /* +0x14 */
    GeoTriangle_t             *tris[1];     /* +0x18 */
} GeoCollisionNode_t;

#define GEO_COLLISION_NODE_SIZE( triCount )                         \
    ( ( int )( sizeof( GeoCollisionNode_t )                         \
               - sizeof( GeoTriangle_t * ) )                        \
      + ( triCount ) * ( int )sizeof( GeoTriangle_t * ) )

#define GEO_EQUAL_EPSILON   0.001f


typedef struct
{
    vec3_t base;            /* +0x00 */
    vec3_t ds;              /* +0x0c */
    vec3_t dt;              /* +0x18 */
} LmapPlane_t;              /* sizeof == 0x24 */

typedef struct
{
    int         threadIndex;    /* +0x00 */
    int         triIndex;       /* +0x04 */
    int         lightmapIndex;  /* +0x08 */
    LmapPlane_t xyz;            /* +0x0c */
    LmapPlane_t tangent;        /* +0x30 */
    LmapPlane_t binormal;       /* +0x54 */
    LmapPlane_t normal;         /* +0x78 */
} TransportTri_t;               /* sizeof == 0x9c */

#define TRANSPORT_CELL_AREA_SCALE   0.125f
#define TRANSPORT_CELL_RADIUS       0.75f


typedef struct
{
    int             worldSpaceXyzCount;     /* 0x1297315c */
    vec3_t          worldSpaceXyz[GEO_MAX_DRAW_VERTS];  /* 0x12973160 */
    vec3_t          mins;                   /* 0x13033160 */
    vec3_t          maxs;                   /* 0x1303316c */
    orientation_t   modelOrientation[GEO_MAX_MODELS];  /* 0x13033178 */

    int             triRefUsed;             /* 0x13063148 */
    int             triRefCount;            /* +0x004 */
    GeoTriangle_t **triRefs;                /* +0x008 */
    int             bspNodeCount;           /* +0x00c */
    GeoBspNode_t   *bspNodes;               /* +0x010 */
    int             bspLeafCount;           /* +0x014 */
    GeoBspLeaf_t   *bspLeafs;               /* +0x018 */
    int                  sampleSubdivision; /* +0x01c */
    Poly2dGridCallback_t sampleFunc;        /* +0x020 */
    int            *firstBrushSide;         /* +0x024 */
    GeoHunk_t       hunk[GEO_MAX_HUNKS];    /* +0x028 */
    float           supersampleAlphaOffset[GEO_MAX_SUPERSAMPLE_ALPHA][2];  /* +0x058 */
    float          *triMin;                 /* +0x150 */
    float          *triMax;                 /* +0x154 */
    float          *triFlat;                /* +0x158 */
} geoGlob_t;


extern geoGlob_t geoGlob;                               /* 0x1297315c */

extern int           geoTriCount;                       /* 0x11bf3158 */
extern GeoTriangle_t geoTris[GEO_MAX_TRIANGLES];        /* 0x11bf315c */
extern GeoVertex_t *const geoVertices;                  /* 0x0a8a4510 */


#define TRACE_HIT_WORLD_GEO     0
#define TRACE_HIT_MODEL_GEO     1

typedef struct
{
    int                  geoType;    /* +0x00 */
    const GeoTriangle_t *tri;        /* +0x04 */
    float                u;          /* +0x08 */
    float                v;          /* +0x0c */
    float                frac;       /* +0x10 */
    int                  alphaMask;  /* +0x14 */
} GeoHit_t;                          /* sizeof == 0x18 */

#define GEO_MAX_TRACE_HITS  31

typedef struct
{
    float    frac;                      /* +0x00 */
    int      hitCount;                  /* +0x04 */
    GeoHit_t hits[GEO_MAX_TRACE_HITS];  /* +0x08 */
} GeoTraceResult_t;

typedef struct
{
    TraceLine_t      line;              /* +0x00 */
    vec3_t           delta;             /* +0x24 */
    byte             supersampleAlpha;  /* +0x30 */
    byte             pad31[3];          /* +0x31 */
    vec3_t           alphaAxis[2];      /* +0x34 */
    GeoTraceResult_t result;            /* +0x4c */
} GeoTrace_t;                           /* sizeof == 0x33c */


struct ModelCollTri_s;

void Geo_TraceModelTriangle( const struct ModelCollTri_s *tri, int scripted,
                             GeoTrace_t *trace );                /* 0x00409620 */

qboolean SurfaceCastsShadow( const MskMaterial_t *mskMtl );  /* 0x00408b50 */

void Geo_ConvertBspNode( const BspNode_t *dnode, GeoBspNode_t *bspNode ); /* 0x0040b120 */
void Geo_CopyBspNodes( void );                                        /* 0x0040b1a0 */

int  Geo_AllocBspLeaf( GeoTriangle_t *const *triRefs, int triCount );  /* 0x0040ad90 */

void Geo_AddTriangle( MskMaterial_t *mskMtl,
                      const vec3_t xyz0, const vec3_t xyz1, const vec3_t xyz2,
                      const vec2_t st0, const vec2_t st1, const vec2_t st2 ); /* 0x00408e70 */

void Geo_Init( void );                                                /* 0x0040a200 */
void Geo_BuildCollisionData( void );                                  /* 0x0040dca0 */
void Geo_AddModel( const BspTriSoup_t *surf, int modelIndex,
                   const orientation_t *orient );                     /* 0x00408b80 */
void Geo_BuildBspTree( void );                                        /* 0x0040afa0 */

void Geo_ForEachTriangleSample( const GeoTriangle_t *tri, int subdivision,
                                Poly2dGridCallback_t func,
                                void *userData );                     /* 0x0040b220 */

int  Geo_TriangleCount( void );                                       /* 0x0040de90 */

void Geo_CalcSampleAreas( int threads );                              /* 0x0040c6c0 */

void Geo_CalcRadiosityColors( int threads );                          /* 0x0040c6e0 */

void Geo_PrintGpuProfile( int threads );
void Geo_BuildTransport( int threads );                               /* 0x0040c700 */

void Geo_ForEachSample( Poly2dGridCallback_t func, int subdivision,
                        int threads );                                /* 0x0040c760 */

void Geo_SetupTrace( const vec3_t start, const vec3_t end,
                     const vec3_t axis0, const vec3_t axis1,
                     GeoTrace_t *trace );                              /* 0x00409a00 */
void Geo_TraceRay( GeoTrace_t *trace );                                /* 0x004099d0 */
void Geo_TraceLine( const vec3_t start, const vec3_t end,
                    GeoTrace_t *trace );                               /* 0x00409c50 */

float    Geo_SampleMaskFraction( int sampleMask );                     /* 0x00409b60 */
void     Geo_ClassifyGroundTriangle( GeoTriangle_t *tri );             /* 0x0040a100 */
float    Geo_TraceOpenFraction( const GeoTraceResult_t *result );      /* 0x00409c00 */
qboolean Geo_TraceSeesSkyUp( const vec3_t point );                     /* 0x00409c90 */
qboolean Geo_TraceSeesSkyDown( const vec3_t point );                   /* 0x00409d50 */
qboolean Geo_PointIsCoveredOverSky( const vec3_t point );              /* 0x00409e10 */
qboolean Geo_PointIsOutsideWorld( const vec3_t point );                /* 0x00409e40 */
qboolean Geo_PointIsSolid( const vec3_t point );                       /* 0x00409f70 */

qboolean Geo_GridCornerIsCutOff( int threadIndex, const vec3_t origin,
                                 int corner );                         /* 0x0040da70 */


#endif

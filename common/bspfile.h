/* Original: ..\common\bspfile.cpp */

#ifndef BSPFILE_H
#define BSPFILE_H

#include "q_shared.h"

#define BSP_IDENT           ( ( 'P' << 24 ) + ( 'S' << 16 ) + ( 'B' << 8 ) + 'I' )  /* 0x50534249 */
#define BSP_VERSION         22
#define BSP_VERSION_OLDEST  6

#define BSP_CHUNK_LIMIT     100

#define BSP_OLD_LUMP_COUNT  0x2f

typedef enum
{
    LUMP_MATERIALS           = 0,
    LUMP_LIGHTBYTES          = 1,
    LUMP_LIGHTGRIDENTRIES    = 2,
    LUMP_LIGHTGRIDCOLORS     = 3,
    LUMP_PLANES              = 4,
    LUMP_BRUSHSIDES          = 5,
    LUMP_BRUSHSIDEEDGECOUNTS = 6,
    LUMP_BRUSHEDGES          = 7,
    LUMP_BRUSHES             = 8,
    LUMP_TRIANGLES           = 9,
    LUMP_DRAWVERTS           = 10,
    LUMP_DRAWINDICES         = 11,
    LUMP_CULLGROUPS          = 12,
    LUMP_CULLGROUPINDICES    = 13,
    LUMP_PORTALVERTS         = 19,
    LUMP_AABBTREES           = 24,
    LUMP_CELLS               = 25,
    LUMP_PORTALS             = 26,
    LUMP_NODES               = 27,
    LUMP_LEAFS               = 28,
    LUMP_LEAFBRUSHES         = 29,
    LUMP_LEAFSURFACES        = 30,
    LUMP_COLLISIONVERTS      = 31,
    LUMP_COLLISIONTRIS       = 32,
    LUMP_COLLISIONEDGEWALK   = 33,
    LUMP_COLLISIONBORDERS    = 34,
    LUMP_COLLISIONPARTITIONS = 35,
    LUMP_COLLISIONAABBS      = 36,
    LUMP_MODELS              = 37,
    LUMP_VISIBILITY          = 38,
    LUMP_ENTITIES            = 39,
    LUMP_PATHCONNECTIONS     = 40,
    LUMP_REFLECTIONPROBES    = 41,
    LUMP_VERTEXLAYERDATA     = 42,
    LUMP_PRIMARYLIGHTS       = 43,
    LUMP_LIGHTGRIDHEADER     = 44,
    LUMP_LIGHTGRIDROWS       = 45,
    LUMP_SIMPLETRIANGLES     = 47,
    LUMP_SIMPLEDRAWVERTS     = 48,
    LUMP_SIMPLEDRAWINDICES   = 49,
    LUMP_SIMPLECULLGROUPS    = 50,
    LUMP_SIMPLEAABBTREES     = 51,
    LUMP_LIGHTREGIONS        = 52,
    LUMP_LIGHTREGIONHULLS    = 53,
    LUMP_LIGHTREGIONAXES     = 54
} bspLump_t;

#define TRIS_TYPE_LAYERED   0
#define TRIS_TYPE_SIMPLE    1
#define TRIS_TYPE_COUNT     2

#define MAX_MAP_MATERIALS            1224        /* 0x15840 / 0x48 */
#define MAX_MAP_LIGHTMAPS            31          /* 0x5d00000 / 0x300000 */
#define LIGHTMAP_BYTES               0x300000
#define MAX_MAP_LIGHTGRIDENTRIES     0x100000    /* 0x400000 */
#define MAX_MAP_LIGHTGRIDCOLORS      0xffff      /* 0xa7ff58 / 0xa8 */
#define MAX_MAP_LIGHTGRIDROWBYTES    0x40000
#define MAX_MAP_PLANES               0x10000
#define MAX_MAP_BRUSHSIDES           0xa0000
#define MAX_MAP_BRUSHEDGES           0x200000
#define MAX_MAP_BRUSHES              0x8000
#define MAX_MAP_TRISOUPS             0x8000
#define MAX_MAP_DRAWVERTS            0x90000     /* 0x2640000 / 0x44 */
#define MAX_MAP_DRAWINDICES          0x120000
#define MAX_MAP_LAYERED_DRAWINDICES  0x100000
#define MAX_MAP_VERTEXLAYERBYTES     0x200000
#define MAX_MAP_CULLGROUPS           0x800       /* 0x10000 / 0x20 */
#define MAX_MAP_CULLGROUPINDICES     0x1000      /* 0x4000 */
#define MAX_MAP_PORTALVERTS          0x4000      /* 0x30000 / 0xc */
#define MAX_MAP_AABBTREES            0x8000
#define MAX_MAP_CELLS                0x400       /* 0x1c000 / 0x70 */
#define MAX_MAP_PORTALS              0x800       /* 0x8000 / 0x10 */
#define MAX_MAP_NODES                0x8000
#define MAX_MAP_LEAFS                0x8000
#define MAX_MAP_LEAFBRUSHES          0x40000
#define MAX_MAP_LEAFSURFACES         0x20000
#define MAX_MAP_COLLISIONVERTS       0x10000
#define MAX_MAP_COLLISIONTRIS        0x20000
#define MAX_MAP_COLLISIONBORDERS     0x8000
#define MAX_MAP_COLLISIONPARTITIONS  0x20000
#define MAX_MAP_COLLISIONAABBS       0x40000
#define MAX_MAP_MODELS               0xfff
#define MAX_MAP_VISIBILITY           0x200000
#define MAX_MAP_ENTSTRING            0x1000000
#define MAX_MAP_PATHBYTES            0x200000
#define MAX_MAP_REFLECTION_PROBES    0xff
#define MAX_MAP_PRIMARY_LIGHTS       0xff        /* 0x7f80 / 0x80 */
#define MAX_MAP_LIGHTREGIONHULLS     0x7f8       /* 0x25da0 / 0x4c */
#define MAX_MAP_LIGHTREGIONAXES      0x3fc0      /* 0x4fb00 / 0x14 */
#define MAX_MAP_ENTITIES             0x10000

#define MAX_MAP_LIGHTGRIDROWOFFSETS  0x2000      /* 0x4000 */

#define MAX_CELL_REFLECTION_PROBES   64

#define MAX_HULLS_PER_LIGHT          8

#define BSP_EXTENSION_SUFFIX         "bsp"
#define PORTAL_EXTENSION_SUFFIX      "prt"
#define POLYFILE_EXTENSION_SUFFIX    "poly"

typedef struct
{
    int size;       /* +0x00 */
    int offset;     /* +0x04 */
} BspLumpEntry_t;

typedef struct
{
    int type;       /* +0x00 */
    int size;       /* +0x04 */
} BspChunkEntry_t;

typedef struct
{
    int ident;                                  /* +0x00 */
    int version;                                /* +0x04 */
    union
    {
        struct
        {
            int             chunkCount;              /* +0x08 */
            BspChunkEntry_t chunks[BSP_CHUNK_LIMIT]; /* +0x0c */
        };
        BspLumpEntry_t      lumps[BSP_OLD_LUMP_COUNT];  /* +0x08 */
    };
} BspHeader_t;

typedef struct
{
    char     name[64];        /* +0x00 */
    unsigned surfaceFlags;    /* +0x40 */
    unsigned contentFlags;    /* +0x44 */
} Dmaterial_t;

typedef struct
{
    vec3_t normal;            /* +0x00 */
    float  dist;              /* +0x0c */
} BspPlane_t;

typedef struct
{
    union
    {
        float distance;       /* +0x00 */
        int   planeNum;       /* +0x00 */
    } u;
    unsigned materialNum;     /* +0x04 */
} BspBrushSide_t;

typedef struct
{
    unsigned short numSides;    /* +0x00 */
    unsigned short materialNum; /* +0x02 */
} BspBrush_t;

typedef struct
{
    unsigned short materialIndex;        /* +0x00 */
    unsigned char  lightmapIndex;        /* +0x02 */
    unsigned char  reflectionProbeIndex; /* +0x03 */
    unsigned char  primaryLightIndex;    /* +0x04 */
    unsigned char  vertexLayerType;      /* +0x05 */
    unsigned short pad06;                /* +0x06 */
    int            vertexLayerData;      /* +0x08 */
    int            firstVertex;          /* +0x0c */
    unsigned short vertexCount;          /* +0x10 */
    unsigned short indexCount;           /* +0x12 */
    int            firstIndex;           /* +0x14 */
} BspTriSoup_t;

typedef struct
{
    vec3_t xyz;               /* +0x00 */
    vec3_t normal;            /* +0x0c */
    byte   color[4];          /* +0x18 */
    vec2_t texCoord;          /* +0x1c */
    vec2_t lmapCoord;         /* +0x24 */
    vec3_t tangent;           /* +0x2c */
    vec3_t binormal;          /* +0x38 */
} BspDrawVert_t;

typedef struct
{
    vec3_t mins;              /* +0x00 */
    vec3_t maxs;              /* +0x0c */
    int    firstSurface;      /* +0x18 */
    int    surfaceCount;      /* +0x1c */
} BspCullGroup_t;

typedef struct
{
    int firstSurface;         /* +0x00 */
    int surfaceCount;         /* +0x04 */
    int childCount;           /* +0x08 */
} BspAabbTree_t;

typedef struct
{
    vec3_t         mins;                                    /* +0x00 */
    vec3_t         maxs;                                    /* +0x0c */
    unsigned short aabbTreeIndex[TRIS_TYPE_COUNT];          /* +0x18 */
    int            firstPortal;                             /* +0x1c */
    int            portalCount;                             /* +0x20 */
    int            firstCullGroup;                          /* +0x24 */
    int            cullGroupCount;                          /* +0x28 */
    unsigned char  reflectionProbeCount;                    /* +0x2c */
    unsigned char  reflectionProbes[MAX_CELL_REFLECTION_PROBES]; /* +0x2d */
    unsigned char  pad6d[3];                                /* +0x6d */
} BspCell_t;

typedef struct
{
    int planeIndex;           /* +0x00 */
    int cellIndex;            /* +0x04 */
    int firstPortalVert;      /* +0x08 */
    int portalVertCount;      /* +0x0c */
} BspPortal_t;

typedef struct
{
    int planeNum;             /* +0x00 */
    int children[2];          /* +0x04 */
    int mins[3];              /* +0x0c */
    int maxs[3];              /* +0x18 */
} BspNode_t;

typedef struct
{
    int cluster;              /* +0x00 */
    int firstCollAabbIndex;   /* +0x04 */
    int collAabbCount;        /* +0x08 */
    int firstLeafBrush;       /* +0x0c */
    int leafBrushCount;       /* +0x10 */
    int cellNum;              /* +0x14 */
} BspLeaf_t;

typedef struct
{
    vec3_t         mins;                            /* +0x00 */
    vec3_t         maxs;                            /* +0x0c */
    unsigned short firstTriSoup[TRIS_TYPE_COUNT];   /* +0x18 */
    unsigned short triSoupCount[TRIS_TYPE_COUNT];   /* +0x1c */
    int            firstSurface;                    /* +0x20 */
    int            numSurfaces;                     /* +0x24 */
    int            firstBrush;                      /* +0x28 */
    int            numBrushes;                      /* +0x2c */
} BspModel_t;

typedef struct
{
    unsigned short vertIndices[3];  /* +0x00 */
} BspCollisionTri_t;

typedef struct
{
    float distEq[3];          /* +0x00 */
    float zBase;              /* +0x0c */
    float zSlope;             /* +0x10 */
    float start;              /* +0x14 */
    float length;             /* +0x18 */
} BspCollisionBorder_t;

typedef struct
{
    unsigned short checkStamp;   /* +0x00 */
    unsigned char  triCount;     /* +0x02 */
    unsigned char  borderCount;  /* +0x03 */
    int            firstTri;     /* +0x04 */
    int            firstBorder;  /* +0x08 */
} BspCollisionPart_t;

typedef struct
{
    vec3_t         midPoint;      /* +0x00 */
    vec3_t         halfSize;      /* +0x0c */
    unsigned short materialIndex; /* +0x18 */
    unsigned short childCount;    /* +0x1a */
    int            u;             /* +0x1c */
} BspCollisionAabb_t;

typedef struct
{
    unsigned char type;              /* +0x00 */
    unsigned char canUseShadowMap;   /* +0x01 */
    unsigned char exponent;          /* +0x02 */
    unsigned char unused;            /* +0x03 */
    vec3_t        color;             /* +0x04 */
    vec3_t        dir;               /* +0x10 */
    vec3_t        origin;            /* +0x1c */
    float         radius;            /* +0x28 */
    float         cosHalfFovOuter;   /* +0x2c */
    float         cosHalfFovInner;   /* +0x30 */
    int           coneExponent;      /* +0x34 */
    float         rotationLimit;     /* +0x38 */
    float         translationLimit;  /* +0x3c */
    char          defName[64];       /* +0x40 */
} BspPrimaryLight_t;

#define GFX_LIGHT_TYPE_NONE   0
#define GFX_LIGHT_TYPE_DIR    1
#define GFX_LIGHT_TYPE_SPOT   2
#define GFX_LIGHT_TYPE_OMNI   3
#define PRIMARY_LIGHT_NONE    0

typedef struct
{
    float    kdopMidPoint[9];  /* +0x00 */
    float    kdopHalfSize[9];  /* +0x24 */
    unsigned axisCount;        /* +0x48 */
} BspLightRegionHull_t;

typedef struct
{
    vec3_t dir;                /* +0x00 */
    float  midPoint;           /* +0x0c */
    float  halfSize;           /* +0x10 */
} BspLightRegionAxis_t;

typedef struct
{
    unsigned short mins[3];   /* +0x00 */
    unsigned short maxs[3];   /* +0x06 */
    int            rowAxis;   /* +0x0c */
    int            colAxis;   /* +0x10 */
    unsigned short rowDataStart[MAX_MAP_LIGHTGRIDROWOFFSETS];  /* +0x14 */
} BspLightGridHeader_t;

typedef struct
{
    byte data[0xa8];
} BspLightGridColor_t;

typedef struct
{
    unsigned short colorsIndex;         /* +0x00 */
    byte           primaryLightIndex;   /* +0x02 */
    byte           needsTrace;          /* +0x03 */
} BspLightGridEntry_t;

typedef struct
{
    vec3_t origin;            /* +0x00 */
    char   name[64];          /* +0x0c */
    byte   cubeMap[0x1fff8];  /* +0x4c */
} BspReflectionProbe_t;

typedef struct epair_s
{
    struct epair_s *next;     /* +0x00 */
    char           *key;      /* +0x04 */
    char           *value;    /* +0x08 */
} epair_t;

struct Brush_s;
struct parseMesh_s;
struct TerrainNode_s;

typedef struct Entity_s
{
    vec3_t              origin;         /* +0x00 */
    struct Brush_s     *brushes;        /* +0x0c */
    struct Brush_s     *physicsBrushes; /* +0x10 */
    struct parseMesh_s *patches;        /* +0x14 */
    struct TerrainNode_s *terrain;      /* +0x18 */
    int                 firstDrawSurf;  /* +0x1c */
    int                 firstTriSoup[TRIS_TYPE_COUNT];  /* +0x20 */
    epair_t            *epairs;         /* +0x28 */
    int                 mapIndex;       /* +0x2c */
    int                 entityNum;      /* +0x30 */
} Entity_t;                   /* sizeof == 0x34 */

extern Dmaterial_t          bspMaterials[MAX_MAP_MATERIALS];              /* 0x06981758 */
extern int                  numBSPMaterials;                              /* 0x06981750 */

extern byte                 bspLightBytes[MAX_MAP_LIGHTMAPS * LIGHTMAP_BYTES]; /* 0x00a35c10 */
extern int                  numBSPLightBytes;                             /* 0x0897f35c */

extern BspLightGridHeader_t bspLightGridHeader;                           /* 0x06735c18 */
extern byte                 bspLightGridRowBytes[MAX_MAP_LIGHTGRIDROWBYTES]; /* 0x0fb27420 */
extern int                  numBSPLightGridRowBytes;                      /* 0x08e7f460 */
extern BspLightGridEntry_t  bspLightGridEntries[MAX_MAP_LIGHTGRIDENTRIES]; /* 0x0987f478 */
extern int                  numBSPLightGridEntries;                       /* 0x0a7df3f4 */
extern BspLightGridColor_t  bspLightGridColors[MAX_MAP_LIGHTGRIDCOLORS];  /* 0x09c7f480 */
extern int                  numBSPLightGridColors;                        /* 0x0fb973f4 */

extern BspPlane_t           bspPlanes[MAX_MAP_PLANES];                    /* 0x06851748 */
extern int                  numBSPPlanes;                                 /* 0x0a6ff3dc */

extern BspBrushSide_t       bspBrushSides[MAX_MAP_BRUSHSIDES];            /* 0x0897f460 */
extern int                  numBSPBrushSides;                             /* 0x0fb97404 */
extern byte                 bspBrushSideEdgeCounts[MAX_MAP_BRUSHSIDES];   /* 0x0a9df3f8 */
extern byte                 bspBrushEdges[MAX_MAP_BRUSHEDGES];            /* 0x0aa9b408 */
extern int                  numBSPBrushEdges;                             /* 0x0987f474 */
extern BspBrush_t           bspBrushes[MAX_MAP_BRUSHES];                  /* 0x10a97410 */
extern int                  numBSPBrushes;                                /* 0x0fb973fc */

extern BspTriSoup_t         bspTriSoups[TRIS_TYPE_COUNT][MAX_MAP_TRISOUPS];      /* 0x0aca7418 */
extern int                  numBSPTriSoups[TRIS_TYPE_COUNT];                     /* 0x06981748 */
extern BspDrawVert_t        bspDrawVerts[TRIS_TYPE_COUNT][MAX_MAP_DRAWVERTS];    /* 0x0ae27420 */
extern int                  numBSPDrawVerts[TRIS_TYPE_COUNT];                    /* 0x06791740 */
extern unsigned short       bspDrawIndices[TRIS_TYPE_COUNT][MAX_MAP_DRAWINDICES];/* 0x10037408 */
extern int                  numBSPDrawIndices[TRIS_TYPE_COUNT];                  /* 0x0ac9b408 */
extern BspAabbTree_t        bspAabbTrees[TRIS_TYPE_COUNT][MAX_MAP_AABBTREES];    /* 0x0a6ff3e8 */
extern int                  numBSPAabbTrees[TRIS_TYPE_COUNT];                    /* 0x0ac9b410 */
extern BspCullGroup_t       bspCullGroups[TRIS_TYPE_COUNT][MAX_MAP_CULLGROUPS];  /* 0x0a7bf3e8 */
extern int                  numCullGroups;                                    /* 0x06741c30 */
extern byte                 bspVertexLayerData[MAX_MAP_VERTEXLAYERBYTES];        /* 0x08e7f470 */
extern int                  numBSPVertexLayerBytes;                              /* 0x0aa9b404 */

extern int                  bspCullGroupIndices[MAX_MAP_CULLGROUPINDICES]; /* 0x06996fa0 */
extern int                  numBSPCullGroupIndices;                        /* 0x0aa7f3f8 */

extern vec3_t               bspPortalVerts[MAX_MAP_PORTALVERTS];          /* 0x06951748 */
extern int                  numBSPPortalVerts;                            /* 0x0aa9b400 */
extern BspCell_t            bspCells[MAX_MAP_CELLS];                      /* 0x0aa7f400 */
extern int                  numCells;                                  /* 0x06996f98 */
extern BspPortal_t          bspPortals[MAX_MAP_PORTALS];                  /* 0x06739c30 */
extern int                  numBSPPortals;                                /* 0x0a6ff3e4 */

extern BspNode_t            bspNodes[MAX_MAP_NODES];                      /* 0x10637408 */
extern int                  numBSPNodes;                                  /* 0x06735c10 */
extern BspLeaf_t            bspLeafs[MAX_MAP_LEAFS];                      /* 0x06791748 */
extern int                  numBSPLeafs;                                  /* 0x0a6ff3e0 */
extern int                  bspLeafBrushes[MAX_MAP_LEAFBRUSHES];          /* 0x0ff37408 */
extern int                  numBSPLeafBrushes;                            /* 0x09c7f47c */
extern int                  bspLeafSurfaces[MAX_MAP_LEAFSURFACES];        /* 0x0faa7420 */
extern int                  numBSPLeafSurfaces;                           /* 0x06739c2c */

extern vec3_t               bspCollisionVerts[MAX_MAP_COLLISIONVERTS];    /* 0x11ab7410 */
extern int                  numBSPCollisionVerts;                         /* 0x0ae2741c */
extern BspCollisionTri_t    bspCollisionTris[MAX_MAP_COLLISIONTRIS];      /* 0x0fb97408 */
extern int                  numBSPCollisionTris;                          /* 0x06735c14 */
extern byte                 bspCollisionEdgeWalk[0xc000];                 /* 0x0ac9b418 */
extern BspCollisionBorder_t bspCollisionBorders[MAX_MAP_COLLISIONBORDERS]; /* 0x0fc57408 */
extern int                  numBSPCollisionBorders;                       /* 0x0fb973f0 */
extern BspCollisionPart_t   bspCollisionParts[MAX_MAP_COLLISIONPARTITIONS]; /* 0x104b7408 */
extern int                  numBSPCollisionParts;                         /* 0x09c7f478 */
extern BspCollisionAabb_t   bspCollisionAabbs[MAX_MAP_COLLISIONAABBS];    /* 0x0907f470 */
extern int                  numBSPCollisionAabbs;                         /* 0x0ae27418 */

extern BspModel_t           bspModels[MAX_MAP_MODELS];                    /* 0x0fb67420 */
extern int                  numBSPModels;                                 /* 0x06996f9c */

extern byte                 bspVisBytes[MAX_MAP_VISIBILITY];              /* 0x0a7df3f8 */
extern int                  numBSPVisBytes;                               /* 0x0a6ff3d8 */

extern char                 bspEntData[MAX_MAP_ENTSTRING];                /* 0x10ab7410 */
extern int                  bspEntDataSize;                               /* 0x10a97408 */

extern byte                 bspPathData[MAX_MAP_PATHBYTES];               /* 0x0fd37408 */
extern int                  numBSPPathBytes;                              /* 0x11ba5130 */

extern BspReflectionProbe_t bspReflectionProbes[MAX_MAP_REFLECTION_PROBES]; /* 0x0699afa0 */
extern int                  numBSPReflectionProbes;                       /* 0x10a9740c */

extern BspPrimaryLight_t    bspPrimaryLights[MAX_MAP_PRIMARY_LIGHTS];     /* 0x11b77410 */
extern int                  numBSPPrimaryLights;                          /* 0x06981754 */

extern byte                 bspLightRegions[MAX_MAP_PRIMARY_LIGHTS];      /* 0x0897f360 */
extern int                  numBSPLightRegionBytes;                       /* 0x0fb973f8 */
extern BspLightRegionHull_t bspLightRegionHulls[MAX_MAP_LIGHTREGIONHULLS]; /* 0x11b7f390 */
extern int                  numBSPLightRegionHulls;                       /* 0x0aa7f3fc */
extern BspLightRegionAxis_t bspLightRegionAxes[MAX_MAP_LIGHTREGIONAXES];  /* 0x06741c40 */
extern int                  numBSPLightRegionAxes;                        /* 0x0987f470 */

extern Entity_t             entities[MAX_MAP_ENTITIES];                   /* 0x10757408 */
extern int                  num_entities;                                 /* 0x0fb97400 */
extern int entity_num;                              /* 0x00a34fe4 */

extern char                 bspFileExtension[10];                         /* 0x06741c34 */
extern char                 prtFileExtension[10];                         /* 0x08e7f464 */
extern char                 polyFileExtension[10];                        /* 0x0a7df3e8 */

int      CopyLump( BspHeader_t *header, int type, void *dest,
                   int elemSize, int maxSize );                      /* 0x0040a3b0 */
void    *GetLump( BspHeader_t *header, int type, unsigned elemSize,
                  unsigned maxSize, unsigned *count );               /* 0x0040a400 */
qboolean LoadBSPFile( const char *filename, BspHeader_t **header,
                      qboolean quiet );                              /* 0x0040a550 */
void     LoadBSPFileLumps( const char *filename, qboolean anyVersion ); /* 0x0040a690 */
void     WriteBSPFile( const char *filename );                       /* 0x0040b840 */
void     AddLump( BspHeader_t *header, void **chunkData, int type,
                  const void *data, int length );                    /* 0x0040c1d0 */
void     LoadAndWriteBSPFile( const char *inPath, const char *outPath ); /* 0x0040dba0 */

int      GetCollisionEdgeWalkSize( int triCount );                   /* 0x0040c170 */
int      GetLightGridHeaderSize( void );                             /* 0x0040c190 */
int      GetLightGridRowCount( void );                               /* 0x0040c1a0 */

void     PrintBSPLumpSize( const char *name, int count, int size,
                           int maxSize, float pctMultiplier );       /* 0x0040c420 */
void     PrintBSPFileSizes( int totalSize );                         /* 0x0040c4f0 */

epair_t *ParseEPair( char *key, const char **parsePos );             /* 0x0040cc40 */
void     FreeEPairs( epair_t *epairs );                              /* 0x0040ce00 */
qboolean ParseEntity( const char **parsePos, Entity_t *ents, int *numEnts ); /* 0x0040ce50 */
void     ParseEntities( void );                                      /* 0x0040cf50 */
void     ParseEntitiesFromBuffer( const char *buf, Entity_t *ents,
                                  int *numEnts );                    /* 0x0040cf70 */
void     UnparseEntities( void );                                    /* 0x0040cfb0 */
void     UnparseEntitiesWithOrigins( void );                         /* 0x0040d2e0 */
void     PrintEntity( const Entity_t *ent );                         /* 0x0040d710 */

void        RemoveKey( Entity_t *ent, const char *key );             /* 0x0040d760 */
void        SetKeyValue( Entity_t *ent, const char *key, const char *value ); /* 0x0040d7e0 */
const char *ValueForKey( const Entity_t *ent, const char *key );     /* 0x0040d810 */
float       FloatForKey( const Entity_t *ent, const char *key );     /* 0x0040da90 */
int         IntForKey( const Entity_t *ent, const char *key );       /* 0x0040dac0 */
void        GetVectorForKey( const Entity_t *ent, const char *key, vec3_t out ); /* 0x0040daf0 */

epair_t    *SetKeyValueInPairs( epair_t *epairs, const char *key, const char *value ); /* 0x0040d830 */
void        RemoveKeyFromPairs( epair_t **epairs, const char *key );  /* 0x0040d8d0 */
qboolean    KeyExistsInPairs( const epair_t *epairs, const char *key );/* 0x0040d940 */
const char *ValueForKeyInPairs( const epair_t *epairs, const char *key ); /* 0x0040d980 */
void        GetVectorForKeyInPairs( const epair_t *epairs, const char *key, vec3_t out ); /* 0x0040d9c0 */
int         IntForKeyInPairs( const epair_t *epairs, const char *key );/* 0x0040da60 */

void        SetBSPFileExtensions( const char *root );                /* 0x0040dbc0 */
const char *GetBSPFileExtension( void );                             /* 0x0040dcd0 */
const char *GetPRTFileExtension( void );                             /* 0x0040dd10 */
const char *GetPolyFileExtension( void );                            /* 0x0040dd50 */

Entity_t *FindEntityWithPair( Entity_t *ents, int numEnts, const char *key, const char *value ); /* 0x0040d1e0 */

#endif

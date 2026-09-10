/* Original: ..\common\bspfile.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "q_parse.h"
#include "cmdlib.h"
#include "bspfile.h"

#define MAX_KEY     32

Dmaterial_t          bspMaterials[MAX_MAP_MATERIALS];                       /* 0x06981758 */
int                  numBSPMaterials;                                       /* 0x06981750 */

byte                 bspLightBytes[MAX_MAP_LIGHTMAPS * LIGHTMAP_BYTES];     /* 0x00a35c10 */
int                  numBSPLightBytes;                                      /* 0x0897f35c */

BspLightGridHeader_t bspLightGridHeader;                                    /* 0x06735c18 */
byte                 bspLightGridRowBytes[MAX_MAP_LIGHTGRIDROWBYTES];       /* 0x0fb27420 */
int                  numBSPLightGridRowBytes;                               /* 0x08e7f460 */
BspLightGridEntry_t  bspLightGridEntries[MAX_MAP_LIGHTGRIDENTRIES];         /* 0x0987f478 */
int                  numBSPLightGridEntries;                                /* 0x0a7df3f4 */
BspLightGridColor_t  bspLightGridColors[MAX_MAP_LIGHTGRIDCOLORS];           /* 0x09c7f480 */
int                  numBSPLightGridColors;                                 /* 0x0fb973f4 */

BspPlane_t           bspPlanes[MAX_MAP_PLANES];                             /* 0x06851748 */
int                  numBSPPlanes;                                          /* 0x0a6ff3dc */

BspBrushSide_t       bspBrushSides[MAX_MAP_BRUSHSIDES];                     /* 0x0897f460 */
int                  numBSPBrushSides;                                      /* 0x0fb97404 */
byte                 bspBrushSideEdgeCounts[MAX_MAP_BRUSHSIDES];            /* 0x0a9df3f8 */
byte                 bspBrushEdges[MAX_MAP_BRUSHEDGES];                     /* 0x0aa9b408 */
int                  numBSPBrushEdges;                                      /* 0x0987f474 */
BspBrush_t           bspBrushes[MAX_MAP_BRUSHES];                           /* 0x10a97410 */
int                  numBSPBrushes;                                         /* 0x0fb973fc */

BspTriSoup_t         bspTriSoups[TRIS_TYPE_COUNT][MAX_MAP_TRISOUPS];        /* 0x0aca7418 */
int                  numBSPTriSoups[TRIS_TYPE_COUNT];                       /* 0x06981748 */
BspDrawVert_t        bspDrawVerts[TRIS_TYPE_COUNT][MAX_MAP_DRAWVERTS];      /* 0x0ae27420 */
int                  numBSPDrawVerts[TRIS_TYPE_COUNT];                      /* 0x06791740 */
unsigned short       bspDrawIndices[TRIS_TYPE_COUNT][MAX_MAP_DRAWINDICES];  /* 0x10037408 */
int                  numBSPDrawIndices[TRIS_TYPE_COUNT];                    /* 0x0ac9b408 */
BspAabbTree_t        bspAabbTrees[TRIS_TYPE_COUNT][MAX_MAP_AABBTREES];      /* 0x0a6ff3e8 */
int                  numBSPAabbTrees[TRIS_TYPE_COUNT];                      /* 0x0ac9b410 */
BspCullGroup_t       bspCullGroups[TRIS_TYPE_COUNT][MAX_MAP_CULLGROUPS];    /* 0x0a7bf3e8 */
int                  numCullGroups;                                      /* 0x06741c30 */
byte                 bspVertexLayerData[MAX_MAP_VERTEXLAYERBYTES];          /* 0x08e7f470 */
int                  numBSPVertexLayerBytes;                                /* 0x0aa9b404 */

int                  bspCullGroupIndices[MAX_MAP_CULLGROUPINDICES];         /* 0x06996fa0 */
int                  numBSPCullGroupIndices;                                /* 0x0aa7f3f8 */

vec3_t               bspPortalVerts[MAX_MAP_PORTALVERTS];                   /* 0x06951748 */
int                  numBSPPortalVerts;                                     /* 0x0aa9b400 */
BspCell_t            bspCells[MAX_MAP_CELLS];                               /* 0x0aa7f400 */
int                  numCells;                                           /* 0x06996f98 */
BspPortal_t          bspPortals[MAX_MAP_PORTALS];                           /* 0x06739c30 */
int                  numBSPPortals;                                         /* 0x0a6ff3e4 */

BspNode_t            bspNodes[MAX_MAP_NODES];                               /* 0x10637408 */
int                  numBSPNodes;                                           /* 0x06735c10 */
BspLeaf_t            bspLeafs[MAX_MAP_LEAFS];                               /* 0x06791748 */
int                  numBSPLeafs;                                           /* 0x0a6ff3e0 */
int                  bspLeafBrushes[MAX_MAP_LEAFBRUSHES];                   /* 0x0ff37408 */
int                  numBSPLeafBrushes;                                     /* 0x09c7f47c */
int                  bspLeafSurfaces[MAX_MAP_LEAFSURFACES];                 /* 0x0faa7420 */
int                  numBSPLeafSurfaces;                                    /* 0x06739c2c */

vec3_t               bspCollisionVerts[MAX_MAP_COLLISIONVERTS];             /* 0x11ab7410 */
int                  numBSPCollisionVerts;                                  /* 0x0ae2741c */
BspCollisionTri_t    bspCollisionTris[MAX_MAP_COLLISIONTRIS];               /* 0x0fb97408 */
int                  numBSPCollisionTris;                                   /* 0x06735c14 */
byte                 bspCollisionEdgeWalk[0xc000];                          /* 0x0ac9b418 */
BspCollisionBorder_t bspCollisionBorders[MAX_MAP_COLLISIONBORDERS];         /* 0x0fc57408 */
int                  numBSPCollisionBorders;                                /* 0x0fb973f0 */
BspCollisionPart_t   bspCollisionParts[MAX_MAP_COLLISIONPARTITIONS];        /* 0x104b7408 */
int                  numBSPCollisionParts;                                  /* 0x09c7f478 */
BspCollisionAabb_t   bspCollisionAabbs[MAX_MAP_COLLISIONAABBS];             /* 0x0907f470 */
int                  numBSPCollisionAabbs;                                  /* 0x0ae27418 */

BspModel_t           bspModels[MAX_MAP_MODELS];                             /* 0x0fb67420 */
int                  numBSPModels;                                          /* 0x06996f9c */

byte                 bspVisBytes[MAX_MAP_VISIBILITY];                       /* 0x0a7df3f8 */
int                  numBSPVisBytes;                                        /* 0x0a6ff3d8 */

char                 bspEntData[MAX_MAP_ENTSTRING];                         /* 0x10ab7410 */
int                  bspEntDataSize;                                        /* 0x10a97408 */

byte                 bspPathData[MAX_MAP_PATHBYTES];                        /* 0x0fd37408 */
int                  numBSPPathBytes;                                       /* 0x11ba5130 */

BspReflectionProbe_t bspReflectionProbes[MAX_MAP_REFLECTION_PROBES];        /* 0x0699afa0 */
int                  numBSPReflectionProbes;                                /* 0x10a9740c */

BspPrimaryLight_t    bspPrimaryLights[MAX_MAP_PRIMARY_LIGHTS];              /* 0x11b77410 */
int                  numBSPPrimaryLights;                                   /* 0x06981754 */

byte                 bspLightRegions[MAX_MAP_PRIMARY_LIGHTS];               /* 0x0897f360 */
int                  numBSPLightRegionBytes;                                /* 0x0fb973f8 */
BspLightRegionHull_t bspLightRegionHulls[MAX_MAP_LIGHTREGIONHULLS];         /* 0x11b7f390 */
int                  numBSPLightRegionHulls;                                /* 0x0aa7f3fc */
BspLightRegionAxis_t bspLightRegionAxes[MAX_MAP_LIGHTREGIONAXES];           /* 0x06741c40 */
int                  numBSPLightRegionAxes;                                 /* 0x0987f470 */

Entity_t             entities[MAX_MAP_ENTITIES];                            /* 0x10757408 */
int                  num_entities;                                          /* 0x0fb97400 */

char                 bspFileExtension[10];                                  /* 0x06741c34 */
char                 prtFileExtension[10];                                  /* 0x08e7f464 */
char                 polyFileExtension[10];                                 /* 0x0a7df3e8 */

static void *GetLumpData( BspHeader_t *header, int type, int offset, unsigned length,
                          unsigned elemSize, unsigned maxSize, unsigned *count );
void         CopyReflectionProbeLumpPreV12( BspHeader_t *header );
static int   CopyTriSoupLumpV9( BspHeader_t *header, int type, BspTriSoup_t *dest,
                                unsigned elemSize, unsigned maxSize );
static int   CopyTriSoupLumpPreV9( BspHeader_t *header, int type, BspTriSoup_t *dest,
                                   unsigned elemSize, unsigned maxSize );
static int   CopyCellLumpV15( BspHeader_t *header, int type, BspCell_t *dest,
                              unsigned elemSize, unsigned maxSize );
static int   CopyCellLumpPreV15( BspHeader_t *header, int type, BspCell_t *dest,
                                 unsigned elemSize, unsigned maxSize );
static int   CopyLeafLumpPreV15( BspHeader_t *header, int type, BspLeaf_t *dest,
                                 unsigned elemSize, unsigned maxSize );
static void  FixMaterialNames( void );
static int   CopyPrimaryLightLumpPreV17( BspHeader_t *header, int type, BspPrimaryLight_t *dest,
                                         unsigned elemSize, unsigned maxSize );
static void  ValidateTriSoups( const BspTriSoup_t *triSoups, unsigned triSoupCount,
                               unsigned diskVertCount, const unsigned short *diskIndices,
                               int diskIndexCount );
static void  StripTrailing( char *e );
Entity_t *FindEntityWithPair( Entity_t *ents, int numEnts, const char *key, const char *value );
static void  CopyMiscModelKeys( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt );
static Entity_t *FindOldMiscModelEntity( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt );
static void  CopyBrushModelOrigin( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt );

typedef struct
{
    unsigned char unknown00[3];      /* +0x00 */
    unsigned char type;              /* +0x03 */
    vec3_t        color;             /* +0x04 */
    vec3_t        dir;               /* +0x10 */
    vec3_t        origin;            /* +0x1c */
    float         radius;            /* +0x28 */
    float         cosHalfFovOuter;   /* +0x2c */
    float         cosHalfFovInner;   /* +0x30 */
    int           coneExponent;      /* +0x34 */
    char          defName[40];       /* +0x38 */
} BspPrimaryLightV14_t;              /* sizeof == 0x60 */

static int   InsertNonePrimaryLight( BspPrimaryLightV14_t *lights, int lightCount );

/* CopyLump  0x0040a3b0 */
int CopyLump( BspHeader_t *header, int type, void *dest, int elemSize, int maxSize )
{
    void    *src;
    unsigned count;

    src = GetLump( header, type, elemSize, maxSize, &count );
    if ( count )
        memcpy( dest, src, count * elemSize );

    return count;
}

/* GetLump  0x0040a400 */
void *GetLump( BspHeader_t *header, int type, unsigned elemSize, unsigned maxSize, unsigned *count )
{
    const BspLumpEntry_t  *lump;
    const BspChunkEntry_t *chunk;
    unsigned               chunkIter;
    int                    offset;

    if ( (unsigned)header->version < 19 )
    {
        lump = &header->lumps[type];
        if ( type < BSP_OLD_LUMP_COUNT )
            return GetLumpData( header, type, lump->offset, lump->size, elemSize, maxSize, count );

        *count = 0;
        return NULL;
    }

    offset = header->chunkCount * sizeof( BspChunkEntry_t ) + 12;
    for ( chunkIter = 0; chunkIter < (unsigned)header->chunkCount; chunkIter++ )
    {
        chunk = &header->chunks[chunkIter];
        if ( chunk->type == type )
            return GetLumpData( header, type, offset, chunk->size, elemSize, maxSize, count );

        offset += ( chunk->size + 3 ) & ~3;
    }

    *count = 0;
    return NULL;
}

/* GetLumpData  0x0040a4f0 */
static void *GetLumpData( BspHeader_t *header, int type, int offset, unsigned length,
                          unsigned elemSize, unsigned maxSize, unsigned *count )
{
    if ( length % elemSize )
        Com_Error( "LoadBspFile: lump %i has odd size", type );

    if ( length > maxSize )
        Com_Error( "LoadBspFile: buffer for lump %i is too small (%i < %i)", type, maxSize, length );

    *count = length / elemSize;
    return (byte *)header + offset;
}

/* LoadBSPFile  0x0040a550 */
qboolean LoadBSPFile( const char *filename, BspHeader_t **header, qboolean quiet )
{
    if ( LoadFile( filename, (void **)header ) < 0 )
    {
        if ( !quiet )
            Com_Error( "Could not load file '%s'\n", filename );
        return qfalse;
    }

    if ( (*header)->ident == BSP_IDENT )
        return qtrue;

    free( *header );
    if ( !quiet )
        Com_Error( "%s is not a IBSP file", filename );

    return qfalse;
}

/* CopyReflectionProbeLumpPreV12  0x0040a5c0 */
void CopyReflectionProbeLumpPreV12( BspHeader_t *header )
{
    byte    *src;
    unsigned probeIter;

    src = (byte *)GetLump( header, LUMP_REFLECTIONPROBES, 0x20004, 4,
                           (unsigned *)&numBSPReflectionProbes );

    if ( numBSPReflectionProbes > MAX_MAP_REFLECTION_PROBES )
        Com_Error( "Map has too many reflection probes %d > %d",
                   numBSPReflectionProbes, MAX_MAP_REFLECTION_PROBES );

    for ( probeIter = 0; probeIter < (unsigned)numBSPReflectionProbes; probeIter++ )
    {
        memcpy( bspReflectionProbes[probeIter].origin,  src + probeIter * 0x20004,       sizeof( vec3_t ) );
        memcpy( bspReflectionProbes[probeIter].cubeMap, src + probeIter * 0x20004 + 0xc, 0x1fff8 );
        bspReflectionProbes[probeIter].name[0] = '\0';
    }
}

/* LoadBSPFileLumps  0x0040a690 */
void LoadBSPFileLumps( const char *filename, qboolean anyVersion )
{
    BspHeader_t *header;
    int          edgeCountCount;
    int          unlayeredCount;

    LoadBSPFile( filename, &header, qfalse );

    if ( (unsigned)header->version < BSP_VERSION_OLDEST )
        Com_Error( "%s is version %i, but %i is the oldest supported version",
                   filename, header->version, BSP_VERSION_OLDEST );

    if ( (unsigned)header->version > BSP_VERSION )
        Com_Error( "%s is version %i, but the compiler uses older version %i",
                   filename, header->version, BSP_VERSION );

    if ( header->version != BSP_VERSION && !anyVersion )
        Com_Error( "bsp is version %i, not %i - recompile to fix", header->version, BSP_VERSION );

    numBSPMaterials = CopyLump( header, LUMP_MATERIALS, bspMaterials,
                                sizeof( bspMaterials[0] ), sizeof( bspMaterials ) );
    if ( (unsigned)header->version < 10 )
        FixMaterialNames();

    if ( (unsigned)header->version < 7 )
    {
        numBSPLightBytes = 0;
    }
    else
    {
        numBSPLightBytes = CopyLump( header, LUMP_LIGHTBYTES, bspLightBytes,
                                     sizeof( bspLightBytes[0] ), sizeof( bspLightBytes ) );
    }

    if ( (unsigned)header->version < 16 )
    {
        numBSPLightGridRowBytes = 0;
        numBSPLightGridEntries  = 0;
        numBSPLightGridColors   = 0;
    }
    else
    {
        CopyLump( header, LUMP_LIGHTGRIDHEADER, &bspLightGridHeader,
                  1, sizeof( bspLightGridHeader ) );
        numBSPLightGridRowBytes = CopyLump( header, LUMP_LIGHTGRIDROWS, bspLightGridRowBytes,
                                            sizeof( bspLightGridRowBytes[0] ), sizeof( bspLightGridRowBytes ) );
        numBSPLightGridEntries  = CopyLump( header, LUMP_LIGHTGRIDENTRIES, bspLightGridEntries,
                                            sizeof( bspLightGridEntries[0] ), sizeof( bspLightGridEntries ) );
        numBSPLightGridColors   = CopyLump( header, LUMP_LIGHTGRIDCOLORS, bspLightGridColors,
                                            sizeof( bspLightGridColors[0] ), sizeof( bspLightGridColors ) );
    }

    numBSPPlanes     = CopyLump( header, LUMP_PLANES, bspPlanes,
                                 sizeof( bspPlanes[0] ), sizeof( bspPlanes ) );
    numBSPBrushSides = CopyLump( header, LUMP_BRUSHSIDES, bspBrushSides,
                                 sizeof( bspBrushSides[0] ), sizeof( bspBrushSides ) );

    edgeCountCount   = CopyLump( header, LUMP_BRUSHSIDEEDGECOUNTS, bspBrushSideEdgeCounts,
                                 sizeof( bspBrushSideEdgeCounts[0] ), sizeof( bspBrushSideEdgeCounts ) );
    if ( numBSPBrushSides != edgeCountCount )
        Com_Error( "Number of brush side edge counts does not equal the number of brush sides" );

    numBSPBrushEdges = CopyLump( header, LUMP_BRUSHEDGES, bspBrushEdges,
                                 sizeof( bspBrushEdges[0] ), sizeof( bspBrushEdges ) );
    numBSPBrushes    = CopyLump( header, LUMP_BRUSHES, bspBrushes,
                                 sizeof( bspBrushes[0] ), sizeof( bspBrushes ) );

    if ( (unsigned)header->version < 9 )
    {
        numBSPTriSoups[TRIS_TYPE_LAYERED] =
            CopyTriSoupLumpPreV9( header, LUMP_TRIANGLES, bspTriSoups[TRIS_TYPE_LAYERED],
                                  sizeof( bspTriSoups[0][0] ), sizeof( bspTriSoups[0] ) );
    }
    else if ( (unsigned)header->version < 13 )
    {
        numBSPTriSoups[TRIS_TYPE_LAYERED] =
            CopyTriSoupLumpV9( header, LUMP_TRIANGLES, bspTriSoups[TRIS_TYPE_LAYERED],
                               sizeof( bspTriSoups[0][0] ), sizeof( bspTriSoups[0] ) );
    }
    else
    {
        numBSPTriSoups[TRIS_TYPE_LAYERED] =
            CopyLump( header, LUMP_TRIANGLES, bspTriSoups[TRIS_TYPE_LAYERED],
                      sizeof( bspTriSoups[0][0] ), sizeof( bspTriSoups[0] ) );
    }

    numBSPTriSoups[TRIS_TYPE_SIMPLE] =
        CopyLump( header, LUMP_SIMPLETRIANGLES, bspTriSoups[TRIS_TYPE_SIMPLE],
                  sizeof( bspTriSoups[0][0] ), sizeof( bspTriSoups[0] ) );

    numBSPDrawVerts[TRIS_TYPE_LAYERED] =
        CopyLump( header, LUMP_DRAWVERTS, bspDrawVerts[TRIS_TYPE_LAYERED],
                  sizeof( bspDrawVerts[0][0] ), sizeof( bspDrawVerts[0] ) );
    numBSPDrawVerts[TRIS_TYPE_SIMPLE] =
        CopyLump( header, LUMP_SIMPLEDRAWVERTS, bspDrawVerts[TRIS_TYPE_SIMPLE],
                  sizeof( bspDrawVerts[0][0] ), sizeof( bspDrawVerts[0] ) );

    numBSPDrawIndices[TRIS_TYPE_LAYERED] =
        CopyLump( header, LUMP_DRAWINDICES, bspDrawIndices[TRIS_TYPE_LAYERED],
                  sizeof( bspDrawIndices[0][0] ),
                  MAX_MAP_LAYERED_DRAWINDICES * sizeof( bspDrawIndices[0][0] ) );
    numBSPDrawIndices[TRIS_TYPE_SIMPLE] =
        CopyLump( header, LUMP_SIMPLEDRAWINDICES, bspDrawIndices[TRIS_TYPE_SIMPLE],
                  sizeof( bspDrawIndices[0][0] ), sizeof( bspDrawIndices[0] ) );

    numCullGroups = CopyLump( header, LUMP_CULLGROUPS, bspCullGroups[TRIS_TYPE_LAYERED],
                                 sizeof( bspCullGroups[0][0] ), sizeof( bspCullGroups[0] ) );
    unlayeredCount   = CopyLump( header, LUMP_SIMPLECULLGROUPS, bspCullGroups[TRIS_TYPE_SIMPLE],
                                 sizeof( bspCullGroups[0][0] ), sizeof( bspCullGroups[0] ) );
    Assert( unlayeredCount == numCullGroups || unlayeredCount == 0 );

    numBSPCullGroupIndices = CopyLump( header, LUMP_CULLGROUPINDICES, bspCullGroupIndices,
                                       sizeof( bspCullGroupIndices[0] ), sizeof( bspCullGroupIndices ) );
    numBSPPortalVerts      = CopyLump( header, LUMP_PORTALVERTS, bspPortalVerts,
                                       sizeof( bspPortalVerts[0] ), sizeof( bspPortalVerts ) );

    numBSPAabbTrees[TRIS_TYPE_LAYERED] =
        CopyLump( header, LUMP_AABBTREES, bspAabbTrees[TRIS_TYPE_LAYERED],
                  sizeof( bspAabbTrees[0][0] ), sizeof( bspAabbTrees[0] ) );
    numBSPAabbTrees[TRIS_TYPE_SIMPLE] =
        CopyLump( header, LUMP_SIMPLEAABBTREES, bspAabbTrees[TRIS_TYPE_SIMPLE],
                  sizeof( bspAabbTrees[0][0] ), sizeof( bspAabbTrees[0] ) );

    if ( (unsigned)header->version < 15 )
    {
        numCells = CopyCellLumpPreV15( header, LUMP_CELLS, bspCells,
                                          sizeof( bspCells[0] ), sizeof( bspCells ) );
    }
    else if ( (unsigned)header->version < BSP_VERSION )
    {
        numCells = CopyCellLumpV15( header, LUMP_CELLS, bspCells,
                                       sizeof( bspCells[0] ), sizeof( bspCells ) );
    }
    else
    {
        numCells = CopyLump( header, LUMP_CELLS, bspCells,
                                sizeof( bspCells[0] ), sizeof( bspCells ) );
    }

    numBSPPortals = CopyLump( header, LUMP_PORTALS, bspPortals,
                              sizeof( bspPortals[0] ), sizeof( bspPortals ) );
    numBSPNodes   = CopyLump( header, LUMP_NODES, bspNodes,
                              sizeof( bspNodes[0] ), sizeof( bspNodes ) );

    if ( (unsigned)header->version < 15 )
    {
        numBSPLeafs = CopyLeafLumpPreV15( header, LUMP_LEAFS, bspLeafs,
                                          sizeof( bspLeafs[0] ), sizeof( bspLeafs ) );
    }
    else
    {
        numBSPLeafs = CopyLump( header, LUMP_LEAFS, bspLeafs,
                                sizeof( bspLeafs[0] ), sizeof( bspLeafs ) );
    }

    numBSPLeafBrushes      = CopyLump( header, LUMP_LEAFBRUSHES, bspLeafBrushes,
                                       sizeof( bspLeafBrushes[0] ), sizeof( bspLeafBrushes ) );
    numBSPLeafSurfaces     = CopyLump( header, LUMP_LEAFSURFACES, bspLeafSurfaces,
                                       sizeof( bspLeafSurfaces[0] ), sizeof( bspLeafSurfaces ) );
    numBSPCollisionVerts   = CopyLump( header, LUMP_COLLISIONVERTS, bspCollisionVerts,
                                       sizeof( bspCollisionVerts[0] ), sizeof( bspCollisionVerts ) );
    numBSPCollisionTris    = CopyLump( header, LUMP_COLLISIONTRIS, bspCollisionTris,
                                       sizeof( bspCollisionTris[0] ), sizeof( bspCollisionTris ) );
    CopyLump( header, LUMP_COLLISIONEDGEWALK, bspCollisionEdgeWalk,
              sizeof( bspCollisionEdgeWalk[0] ), sizeof( bspCollisionEdgeWalk ) );
    numBSPCollisionBorders = CopyLump( header, LUMP_COLLISIONBORDERS, bspCollisionBorders,
                                       sizeof( bspCollisionBorders[0] ), sizeof( bspCollisionBorders ) );
    numBSPCollisionParts   = CopyLump( header, LUMP_COLLISIONPARTITIONS, bspCollisionParts,
                                       sizeof( bspCollisionParts[0] ), sizeof( bspCollisionParts ) );
    numBSPCollisionAabbs   = CopyLump( header, LUMP_COLLISIONAABBS, bspCollisionAabbs,
                                       sizeof( bspCollisionAabbs[0] ), sizeof( bspCollisionAabbs ) );
    numBSPModels           = CopyLump( header, LUMP_MODELS, bspModels,
                                       sizeof( bspModels[0] ), sizeof( bspModels ) );
    numBSPVisBytes         = CopyLump( header, LUMP_VISIBILITY, bspVisBytes,
                                       sizeof( bspVisBytes[0] ), sizeof( bspVisBytes ) );
    bspEntDataSize         = CopyLump( header, LUMP_ENTITIES, bspEntData,
                                       sizeof( bspEntData[0] ), sizeof( bspEntData ) );

    if ( (unsigned)header->version < 14 )
    {
        numBSPPrimaryLights          = 2;
        bspPrimaryLights[1].type     = 1;
        Vec3Set( bspPrimaryLights[1].dir, 0.0f, 0.0f, 1.0f );
    }
    else if ( (unsigned)header->version < 17 )
    {
        numBSPPrimaryLights = CopyPrimaryLightLumpPreV17( header, LUMP_PRIMARYLIGHTS, bspPrimaryLights,
                                                          sizeof( bspPrimaryLights[0] ), sizeof( bspPrimaryLights ) );
    }
    else
    {
        numBSPPrimaryLights = CopyLump( header, LUMP_PRIMARYLIGHTS, bspPrimaryLights,
                                        sizeof( bspPrimaryLights[0] ), sizeof( bspPrimaryLights ) );
    }

    numBSPLightRegionBytes = CopyLump( header, LUMP_LIGHTREGIONS, bspLightRegions,
                                       sizeof( bspLightRegions[0] ), sizeof( bspLightRegions ) );
    numBSPLightRegionHulls = CopyLump( header, LUMP_LIGHTREGIONHULLS, bspLightRegionHulls,
                                       sizeof( bspLightRegionHulls[0] ), sizeof( bspLightRegionHulls ) );
    numBSPLightRegionAxes  = CopyLump( header, LUMP_LIGHTREGIONAXES, bspLightRegionAxes,
                                       sizeof( bspLightRegionAxes[0] ), sizeof( bspLightRegionAxes ) );
    numBSPPathBytes        = CopyLump( header, LUMP_PATHCONNECTIONS, bspPathData,
                                       sizeof( bspPathData[0] ), sizeof( bspPathData ) );

    if ( (unsigned)header->version < 8 )
    {
        numBSPReflectionProbes = 0;
    }
    else if ( (unsigned)header->version < 12 )
    {
        CopyReflectionProbeLumpPreV12( header );
    }
    else
    {
        numBSPReflectionProbes = CopyLump( header, LUMP_REFLECTIONPROBES, bspReflectionProbes,
                                           sizeof( bspReflectionProbes[0] ), sizeof( bspReflectionProbes ) );
    }

    if ( (unsigned)header->version < 9 )
    {
        numBSPVertexLayerBytes = 0;
    }
    else
    {
        numBSPVertexLayerBytes = CopyLump( header, LUMP_VERTEXLAYERDATA, bspVertexLayerData,
                                           sizeof( bspVertexLayerData[0] ), sizeof( bspVertexLayerData ) );
    }

    free( header );
}

/* CopyTriSoupLumpV9  0x0040aeb0 */
static int CopyTriSoupLumpV9( BspHeader_t *header, int type, BspTriSoup_t *dest,
                              unsigned elemSize, unsigned maxSize )
{
    const byte *src;
    unsigned    count;
    unsigned    soupIter;

    src = (const byte *)GetLump( header, type, elemSize, maxSize, &count );
    if ( !count )
        return 0;

    for ( soupIter = 0; soupIter < count; soupIter++ )
    {
        const byte *old = src + soupIter * 0x14;

        dest[soupIter].materialIndex        = *(const unsigned short *)( old + 0x00 );
        dest[soupIter].lightmapIndex        = old[0x02];
        dest[soupIter].reflectionProbeIndex = old[0x03];
        dest[soupIter].primaryLightIndex    = 0;
        dest[soupIter].vertexLayerType      = 1;
        dest[soupIter].vertexLayerData      = *(const int *)( old + 0x04 );
        dest[soupIter].firstVertex          = *(const int *)( old + 0x08 );
        dest[soupIter].vertexCount          = *(const unsigned short *)( old + 0x0c );
        dest[soupIter].indexCount           = *(const unsigned short *)( old + 0x0e );
        dest[soupIter].firstIndex           = *(const int *)( old + 0x10 );
    }

    return count;
}

/* CopyTriSoupLumpPreV9  0x0040afd0 */
static int CopyTriSoupLumpPreV9( BspHeader_t *header, int type, BspTriSoup_t *dest,
                                 unsigned elemSize, unsigned maxSize )
{
    const byte *src;
    unsigned    count;
    unsigned    soupIter;

    src = (const byte *)GetLump( header, type, elemSize, maxSize, &count );
    if ( !count )
        return 0;

    for ( soupIter = 0; soupIter < count; soupIter++ )
    {
        const byte *old = src + soupIter * 0x10;

        dest[soupIter].materialIndex        = *(const unsigned short *)( old + 0x00 );
        dest[soupIter].lightmapIndex        = old[0x02];
        dest[soupIter].reflectionProbeIndex = old[0x03];
        dest[soupIter].primaryLightIndex    = 0;
        dest[soupIter].vertexLayerType      = 1;
        dest[soupIter].vertexLayerData      = 0;
        dest[soupIter].firstVertex          = *(const int *)( old + 0x04 );
        dest[soupIter].vertexCount          = *(const unsigned short *)( old + 0x08 );
        dest[soupIter].indexCount           = *(const unsigned short *)( old + 0x0a );
        dest[soupIter].firstIndex           = *(const int *)( old + 0x0c );
    }

    return count;
}

/* CopyCellLumpV15  0x0040b0f0 */
static int CopyCellLumpV15( BspHeader_t *header, int type, BspCell_t *dest,
                            unsigned elemSize, unsigned maxSize )
{
    const byte *src;
    unsigned    count;
    unsigned    cellIter;

    src = (const byte *)GetLump( header, type, elemSize, maxSize, &count );
    if ( !count )
        return 0;

    for ( cellIter = 0; cellIter < count; cellIter++ )
    {
        const byte *old = src + cellIter * 0x2c;

        Vec3Copy( (const float *)( old + 0x00 ), dest[cellIter].mins );
        Vec3Copy( (const float *)( old + 0x0c ), dest[cellIter].maxs );

        dest[cellIter].aabbTreeIndex[TRIS_TYPE_LAYERED] =
            checked_cast< unsigned short >( *(const unsigned short *)( old + 0x18 ) );
        dest[cellIter].aabbTreeIndex[TRIS_TYPE_SIMPLE] =
            checked_cast< unsigned short >( *(const unsigned short *)( old + 0x1a ) );

        dest[cellIter].firstPortal    = *(const int *)( old + 0x1c );
        dest[cellIter].portalCount    = *(const int *)( old + 0x20 );
        dest[cellIter].firstCullGroup = *(const int *)( old + 0x24 );
        dest[cellIter].cullGroupCount = *(const int *)( old + 0x28 );

        memset( dest[cellIter].reflectionProbes, 0, sizeof( dest[cellIter].reflectionProbes ) );
        dest[cellIter].reflectionProbeCount = 1;
    }

    return count;
}

/* CopyCellLumpPreV15  0x0040b240 */
static int CopyCellLumpPreV15( BspHeader_t *header, int type, BspCell_t *dest,
                               unsigned elemSize, unsigned maxSize )
{
    const byte *src;
    unsigned    count;
    unsigned    cellIter;

    src = (const byte *)GetLump( header, type, elemSize, maxSize, &count );
    if ( !count )
        return 0;

    for ( cellIter = 0; cellIter < count; cellIter++ )
    {
        const byte *old = src + cellIter * 0x34;

        Vec3Copy( (const float *)( old + 0x00 ), dest[cellIter].mins );
        Vec3Copy( (const float *)( old + 0x0c ), dest[cellIter].maxs );

        dest[cellIter].aabbTreeIndex[TRIS_TYPE_LAYERED] =
            checked_cast< unsigned short >( *(const int *)( old + 0x18 ) );
        dest[cellIter].aabbTreeIndex[TRIS_TYPE_SIMPLE] = 0xffff;

        dest[cellIter].firstPortal    = *(const int *)( old + 0x1c );
        dest[cellIter].portalCount    = *(const int *)( old + 0x20 );
        dest[cellIter].firstCullGroup = *(const int *)( old + 0x24 );
        dest[cellIter].cullGroupCount = *(const int *)( old + 0x28 );

        memset( dest[cellIter].reflectionProbes, 0, sizeof( dest[cellIter].reflectionProbes ) );
    }

    return count;
}

/* CopyLeafLumpPreV15  0x0040b370 */
static int CopyLeafLumpPreV15( BspHeader_t *header, int type, BspLeaf_t *dest,
                               unsigned elemSize, unsigned maxSize )
{
    const int *src;
    unsigned   count;
    unsigned   leafIter;

    src = (const int *)GetLump( header, type, elemSize, maxSize, &count );
    if ( !count )
        return 0;

    for ( leafIter = 0; leafIter < count; leafIter++ )
    {
        const int *old = src + leafIter * ( 0x24 / 4 );

        dest[leafIter].cluster            = old[0];
        dest[leafIter].firstCollAabbIndex = old[2];
        dest[leafIter].collAabbCount      = old[3];
        dest[leafIter].firstLeafBrush     = old[4];
        dest[leafIter].leafBrushCount     = old[5];
        dest[leafIter].cellNum            = old[6];
    }

    return count;
}

/* FixMaterialNames  0x0040b450 */
static void FixMaterialNames( void )
{
    int   i;
    char *name;
    int   to;
    int   from;

    for ( i = 0; i < numBSPMaterials; i++ )
    {
        name = bspMaterials[i].name;
        if ( name[0] != '*' )
            continue;

        from = 0;
        to   = 0;
        do
        {
            do
            {
                to++;
                from++;
                name[to] = name[from];
            }
            while ( isdigit( name[from] ) );

            to  += ( name[from] == 'n' );
            from += 7;

            Assert( name[from] == '_' || name[from] == '\0' );
            name[to] = name[from];
        }
        while ( name[to] != '\0' );
    }
}

/* CopyPrimaryLightLumpPreV17  0x0040b560 */
static int CopyPrimaryLightLumpPreV17( BspHeader_t *header, int type, BspPrimaryLight_t *dest,
                                       unsigned elemSize, unsigned maxSize )
{
    BspPrimaryLightV14_t lights[MAX_MAP_PRIMARY_LIGHTS];
    int                  count;
    int                  lightIter;

    count = CopyLump( header, type, lights, sizeof( lights[0] ), sizeof( lights ) );
    if ( (unsigned)header->version < 15 )
        count = InsertNonePrimaryLight( lights, count );

    for ( lightIter = 0; lightIter < count; lightIter++ )
    {
        dest[lightIter].type            = lights[lightIter].type;
        dest[lightIter].canUseShadowMap = 0;

        Vec3Copy( lights[lightIter].color,  dest[lightIter].color );
        Vec3Copy( lights[lightIter].dir,    dest[lightIter].dir );
        Vec3Copy( lights[lightIter].origin, dest[lightIter].origin );

        dest[lightIter].radius             = lights[lightIter].radius;
        dest[lightIter].cosHalfFovOuter    = lights[lightIter].cosHalfFovOuter;
        dest[lightIter].cosHalfFovInner    = lights[lightIter].cosHalfFovInner;
        dest[lightIter].coneExponent       = lights[lightIter].coneExponent;
        dest[lightIter].rotationLimit      = 1.0f;
        dest[lightIter].translationLimit   = 0.0f;

        strcpy( dest[lightIter].defName, lights[lightIter].defName );
    }

    return count;
}

/* InsertNonePrimaryLight  0x0040b730 */
static int InsertNonePrimaryLight( BspPrimaryLightV14_t *lights, int lightCount )
{
    int      lightIter;
    unsigned soupIter;

    for ( lightIter = lightCount; lightIter != 0; lightIter-- )
    {
        lights[lightIter] = lights[lightIter - 1];
        lights[lightIter].type++;
    }

    memset( lights, 0, 0x80 );
    SanityCheck( lights[0].type == GFX_LIGHT_TYPE_NONE );

    for ( soupIter = 0; soupIter < (unsigned)numBSPTriSoups[TRIS_TYPE_LAYERED]; soupIter++ )
    {
        if ( (char)bspTriSoups[TRIS_TYPE_LAYERED][soupIter].primaryLightIndex == -1 )
            bspTriSoups[TRIS_TYPE_LAYERED][soupIter].primaryLightIndex = 0;
        else
            bspTriSoups[TRIS_TYPE_LAYERED][soupIter].primaryLightIndex++;
    }

    return lightCount + 1;
}

/* WriteBSPFile  0x0040b840 */
void WriteBSPFile( const char *filename )
{
    BspHeader_t   headerStorage;
    BspHeader_t  *header;
    void         *lumpData[BSP_CHUNK_LIMIT];
    char          text[MAX_OS_PATH];
    DWORD         fileAttrs;
    FILE         *file;
    unsigned      chunkIter;
    unsigned      padCount;

    char          padding[4];

    if ( numBSPNodes != (short)numBSPNodes )
        Com_Error( "numnodes is %d, exceeds limit of %d.\n"
                   "Blocksize is a possible cause, try increasing it (0 will set it as large as possible).\n"
                   "Also, you might try making geometry detail.\n",
                   numBSPNodes, SHRT_MAX );

    header = &headerStorage;
    memset( header, 0, 12 + BSP_CHUNK_LIMIT * sizeof( BspChunkEntry_t ) );

    file = fopen( filename, "wb" );
    if ( !file )
    {
        fileAttrs = GetFileAttributesA( filename );
        if ( fileAttrs == INVALID_FILE_ATTRIBUTES || !( fileAttrs & FILE_ATTRIBUTE_READONLY ) )
            Com_Error( "could not open '%s' for writing\n", filename );

        sprintf( text, "could not open '%s' for writing; replace read-only file?", filename );
        if ( MessageBoxA( GetActiveWindow(), text, "OUTPUT FILE IS READ ONLY",
                          MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK )
            return;

        SetFileAttributesA( filename, fileAttrs & ~FILE_ATTRIBUTE_READONLY );
        file = fopen( filename, "wb" );
        if ( !file )
            Com_Error( "could not open '%s' for writing\n", filename );
    }

    header->ident      = LittleLong( BSP_IDENT );
    header->version    = LittleLong( BSP_VERSION );
    header->chunkCount = 0;

    ValidateTriSoups( bspTriSoups[TRIS_TYPE_LAYERED], numBSPTriSoups[TRIS_TYPE_LAYERED],
                      numBSPDrawVerts[TRIS_TYPE_LAYERED], bspDrawIndices[TRIS_TYPE_LAYERED],
                      numBSPDrawIndices[TRIS_TYPE_LAYERED] );
    ValidateTriSoups( bspTriSoups[TRIS_TYPE_SIMPLE], numBSPTriSoups[TRIS_TYPE_SIMPLE],
                      numBSPDrawVerts[TRIS_TYPE_SIMPLE], bspDrawIndices[TRIS_TYPE_SIMPLE],
                      numBSPDrawIndices[TRIS_TYPE_SIMPLE] );

    AddLump( header, lumpData, LUMP_MATERIALS,           bspMaterials,          numBSPMaterials * sizeof( bspMaterials[0] ) );
    AddLump( header, lumpData, LUMP_LIGHTBYTES,          bspLightBytes,         numBSPLightBytes );
    AddLump( header, lumpData, LUMP_LIGHTGRIDHEADER,     &bspLightGridHeader,   GetLightGridHeaderSize() );
    AddLump( header, lumpData, LUMP_LIGHTGRIDROWS,       bspLightGridRowBytes,  numBSPLightGridRowBytes );
    AddLump( header, lumpData, LUMP_LIGHTGRIDENTRIES,    bspLightGridEntries,   numBSPLightGridEntries * sizeof( bspLightGridEntries[0] ) );
    AddLump( header, lumpData, LUMP_LIGHTGRIDCOLORS,     bspLightGridColors,    numBSPLightGridColors * sizeof( bspLightGridColors[0] ) );
    AddLump( header, lumpData, LUMP_PLANES,              bspPlanes,             numBSPPlanes * sizeof( bspPlanes[0] ) );
    AddLump( header, lumpData, LUMP_BRUSHSIDES,          bspBrushSides,         numBSPBrushSides * sizeof( bspBrushSides[0] ) );
    AddLump( header, lumpData, LUMP_BRUSHSIDEEDGECOUNTS, bspBrushSideEdgeCounts, numBSPBrushSides );
    AddLump( header, lumpData, LUMP_BRUSHEDGES,          bspBrushEdges,         numBSPBrushEdges );
    AddLump( header, lumpData, LUMP_BRUSHES,             bspBrushes,            numBSPBrushes * sizeof( bspBrushes[0] ) );
    AddLump( header, lumpData, LUMP_TRIANGLES,           bspTriSoups[TRIS_TYPE_LAYERED],
                                                         numBSPTriSoups[TRIS_TYPE_LAYERED] * sizeof( bspTriSoups[0][0] ) );
    AddLump( header, lumpData, LUMP_DRAWVERTS,           bspDrawVerts[TRIS_TYPE_LAYERED],
                                                         numBSPDrawVerts[TRIS_TYPE_LAYERED] * sizeof( bspDrawVerts[0][0] ) );
    AddLump( header, lumpData, LUMP_VERTEXLAYERDATA,     bspVertexLayerData,    numBSPVertexLayerBytes );
    AddLump( header, lumpData, LUMP_DRAWINDICES,         bspDrawIndices[TRIS_TYPE_LAYERED],
                                                         numBSPDrawIndices[TRIS_TYPE_LAYERED] * sizeof( bspDrawIndices[0][0] ) );
    AddLump( header, lumpData, LUMP_CULLGROUPS,          bspCullGroups[TRIS_TYPE_LAYERED],
                                                         numCullGroups * sizeof( bspCullGroups[0][0] ) );
    AddLump( header, lumpData, LUMP_CULLGROUPINDICES,    bspCullGroupIndices,   numBSPCullGroupIndices * sizeof( bspCullGroupIndices[0] ) );
    AddLump( header, lumpData, LUMP_PORTALVERTS,         bspPortalVerts,        numBSPPortalVerts * sizeof( bspPortalVerts[0] ) );
    AddLump( header, lumpData, LUMP_AABBTREES,           bspAabbTrees[TRIS_TYPE_LAYERED],
                                                         numBSPAabbTrees[TRIS_TYPE_LAYERED] * sizeof( bspAabbTrees[0][0] ) );
    AddLump( header, lumpData, LUMP_CELLS,               bspCells,              numCells * sizeof( bspCells[0] ) );
    AddLump( header, lumpData, LUMP_PORTALS,             bspPortals,            numBSPPortals * sizeof( bspPortals[0] ) );
    AddLump( header, lumpData, LUMP_NODES,               bspNodes,              numBSPNodes * sizeof( bspNodes[0] ) );
    AddLump( header, lumpData, LUMP_LEAFS,               bspLeafs,              numBSPLeafs * sizeof( bspLeafs[0] ) );
    AddLump( header, lumpData, LUMP_LEAFBRUSHES,         bspLeafBrushes,        numBSPLeafBrushes * sizeof( bspLeafBrushes[0] ) );
    AddLump( header, lumpData, LUMP_LEAFSURFACES,        bspLeafSurfaces,       numBSPLeafSurfaces * sizeof( bspLeafSurfaces[0] ) );
    AddLump( header, lumpData, LUMP_COLLISIONVERTS,      bspCollisionVerts,     numBSPCollisionVerts * sizeof( bspCollisionVerts[0] ) );
    AddLump( header, lumpData, LUMP_COLLISIONTRIS,       bspCollisionTris,      numBSPCollisionTris * sizeof( bspCollisionTris[0] ) );
    AddLump( header, lumpData, LUMP_COLLISIONEDGEWALK,   bspCollisionEdgeWalk,  GetCollisionEdgeWalkSize( numBSPCollisionTris ) );
    AddLump( header, lumpData, LUMP_COLLISIONBORDERS,    bspCollisionBorders,   numBSPCollisionBorders * sizeof( bspCollisionBorders[0] ) );
    AddLump( header, lumpData, LUMP_COLLISIONPARTITIONS, bspCollisionParts,     numBSPCollisionParts * sizeof( bspCollisionParts[0] ) );
    AddLump( header, lumpData, LUMP_COLLISIONAABBS,      bspCollisionAabbs,     numBSPCollisionAabbs * sizeof( bspCollisionAabbs[0] ) );
    AddLump( header, lumpData, LUMP_MODELS,              bspModels,             numBSPModels * sizeof( bspModels[0] ) );
    AddLump( header, lumpData, LUMP_VISIBILITY,          bspVisBytes,           numBSPVisBytes );
    AddLump( header, lumpData, LUMP_ENTITIES,            bspEntData,            bspEntDataSize );
    AddLump( header, lumpData, LUMP_PRIMARYLIGHTS,       bspPrimaryLights,      numBSPPrimaryLights * sizeof( bspPrimaryLights[0] ) );
    AddLump( header, lumpData, LUMP_LIGHTREGIONS,        bspLightRegions,       numBSPLightRegionBytes );
    AddLump( header, lumpData, LUMP_LIGHTREGIONHULLS,    bspLightRegionHulls,   numBSPLightRegionHulls * sizeof( bspLightRegionHulls[0] ) );
    AddLump( header, lumpData, LUMP_LIGHTREGIONAXES,     bspLightRegionAxes,    numBSPLightRegionAxes * sizeof( bspLightRegionAxes[0] ) );
    AddLump( header, lumpData, LUMP_SIMPLETRIANGLES,     bspTriSoups[TRIS_TYPE_SIMPLE],
                                                         numBSPTriSoups[TRIS_TYPE_SIMPLE] * sizeof( bspTriSoups[0][0] ) );
    AddLump( header, lumpData, LUMP_SIMPLEDRAWVERTS,     bspDrawVerts[TRIS_TYPE_SIMPLE],
                                                         numBSPDrawVerts[TRIS_TYPE_SIMPLE] * sizeof( bspDrawVerts[0][0] ) );
    AddLump( header, lumpData, LUMP_SIMPLEDRAWINDICES,   bspDrawIndices[TRIS_TYPE_SIMPLE],
                                                         numBSPDrawIndices[TRIS_TYPE_SIMPLE] * sizeof( bspDrawIndices[0][0] ) );
    AddLump( header, lumpData, LUMP_SIMPLECULLGROUPS,    bspCullGroups[TRIS_TYPE_SIMPLE],
                                                         numCullGroups * sizeof( bspCullGroups[0][0] ) );
    AddLump( header, lumpData, LUMP_SIMPLEAABBTREES,     bspAabbTrees[TRIS_TYPE_SIMPLE],
                                                         numBSPAabbTrees[TRIS_TYPE_SIMPLE] * sizeof( bspAabbTrees[0][0] ) );
    if ( numBSPPathBytes > 0 )
        AddLump( header, lumpData, LUMP_PATHCONNECTIONS, bspPathData,           numBSPPathBytes );
    AddLump( header, lumpData, LUMP_REFLECTIONPROBES,    bspReflectionProbes,   numBSPReflectionProbes * sizeof( bspReflectionProbes[0] ) );

    SafeWrite( file, header, header->chunkCount * sizeof( BspChunkEntry_t ) + 12 );

    for ( chunkIter = 0; chunkIter < (unsigned)header->chunkCount; chunkIter++ )
    {
        SafeWrite( file, lumpData[chunkIter], header->chunks[chunkIter].size );

        padCount = -header->chunks[chunkIter].size & 3;
        if ( padCount )
            SafeWrite( file, padding, padCount );
    }

    fclose( file );
}

/* GetCollisionEdgeWalkSize  0x0040c170 */
int GetCollisionEdgeWalkSize( int triCount )
{
    return ( ( triCount * 3 + 7 ) / 8 + 3 ) & ~3;
}

/* GetLightGridHeaderSize  0x0040c190 */
int GetLightGridHeaderSize( void )
{
    return GetLightGridRowCount() * sizeof( bspLightGridHeader.rowDataStart[0] ) + 0x14;
}

/* GetLightGridRowCount  0x0040c1a0 */
int GetLightGridRowCount( void )
{
    return bspLightGridHeader.maxs[bspLightGridHeader.rowAxis] + 1
         - bspLightGridHeader.mins[bspLightGridHeader.rowAxis];
}

/* AddLump  0x0040c1d0 */
void AddLump( BspHeader_t *header, void **chunkData, int type, const void *data, int length )
{
    unsigned chunkIter;

    Assert( header->chunkCount < BSP_CHUNK_LIMIT );

    if ( !length )
        return;

    for ( chunkIter = 0; chunkIter < (unsigned)header->chunkCount; chunkIter++ )
        Assertx( ( header->chunks[chunkIter].type != type ), "(type) = %i", type );

    header->chunks[header->chunkCount].type = type;
    header->chunks[header->chunkCount].size = length;
    chunkData[header->chunkCount]           = (void *)data;
    header->chunkCount++;
}

/* ValidateTriSoups  0x0040c2a0 */
static void ValidateTriSoups( const BspTriSoup_t *triSoups, unsigned triSoupCount,
                              unsigned diskVertCount, const unsigned short *diskIndices,
                              int diskIndexCount )
{
    unsigned triSoupIter;
    unsigned indexIter;

    for ( triSoupIter = 0; triSoupIter < triSoupCount; triSoupIter++ )
    {
        AssertCmp( triSoups[triSoupIter].firstVertex + triSoups[triSoupIter].vertexCount,
                   <=, diskVertCount );

        AssertCmp( triSoups[triSoupIter].firstIndex + triSoups[triSoupIter].indexCount,
                   <=, diskIndexCount );

        for ( indexIter = 0; indexIter < triSoups[triSoupIter].indexCount; indexIter++ )
        {
            AssertIn( diskIndices[triSoups[triSoupIter].firstIndex + indexIter],
                      triSoups[triSoupIter].vertexCount );
        }
    }
}

/* PrintBSPLumpSize  0x0040c420 */
void PrintBSPLumpSize( const char *name, int count, int size, int maxSize, float pctMultiplier )
{
    char  countStr[16];
    float limitPct;

    countStr[0] = '\0';
    if ( count >= 0 )
        _itoa( count, countStr, 10 );

    if ( maxSize == 0 )
        limitPct = 0.0f;
    else
        limitPct = ( (float)size / (float)maxSize ) * 100.0f;

    if ( limitPct > 75.0f )
        printf( "^1", limitPct );

    printf( "%6.2f%% ", limitPct );
    printf( "%6s %-18s %8i B %6.0f KB %5.1f%%\n",
            countStr, name, size, size * 1.0 / 1024.0, size * pctMultiplier );
}

/* PrintBSPFileSizes  0x0040c4f0 */
void PrintBSPFileSizes( int totalSize )
{
    float pct;

    if ( !num_entities )
        ParseEntities();

    pct = 100.0f / (float)totalSize;

    printf( "\n" );
    printf( " Limit%%  Count Lump                    Bytes Kilobytes   BSP%%\n" );
    printf( "------------------------------------------------------------\n" );

    PrintBSPLumpSize( "models",     numBSPModels,     numBSPModels * sizeof( bspModels[0] ),         MAX_MAP_MODELS * sizeof( bspModels[0] ),        pct );
    PrintBSPLumpSize( "materials",  numBSPMaterials,  numBSPMaterials * sizeof( bspMaterials[0] ),   MAX_MAP_MATERIALS * sizeof( bspMaterials[0] ),  pct );
    PrintBSPLumpSize( "brushes",    numBSPBrushes,    numBSPBrushes * sizeof( bspBrushes[0] ),       MAX_MAP_BRUSHES * sizeof( bspBrushes[0] ),      pct );
    PrintBSPLumpSize( "brushsides", numBSPBrushSides, numBSPBrushSides * sizeof( bspBrushSides[0] ), MAX_MAP_BRUSHSIDES * sizeof( bspBrushSides[0] ), pct );
    PrintBSPLumpSize( "planes",     numBSPPlanes,     numBSPPlanes * sizeof( bspPlanes[0] ),         MAX_MAP_PLANES * sizeof( bspPlanes[0] ),        pct );
    PrintBSPLumpSize( "entdata",    num_entities,     bspEntDataSize,                                MAX_MAP_ENTSTRING,                              pct );
    printf( "\n" );

    PrintBSPLumpSize( "nodes",             numBSPNodes,            numBSPNodes * sizeof( bspNodes[0] ),                       MAX_MAP_NODES * sizeof( bspNodes[0] ),                     pct );
    PrintBSPLumpSize( "leafs",             numBSPLeafs,            numBSPLeafs * sizeof( bspLeafs[0] ),                       MAX_MAP_LEAFS * sizeof( bspLeafs[0] ),                     pct );
    PrintBSPLumpSize( "leafbrushes",       numBSPLeafBrushes,      numBSPLeafBrushes * sizeof( bspLeafBrushes[0] ),           MAX_MAP_LEAFBRUSHES * sizeof( bspLeafBrushes[0] ),         pct );
    PrintBSPLumpSize( "leafsurfaces",      numBSPLeafSurfaces,     numBSPLeafSurfaces * sizeof( bspLeafSurfaces[0] ),         MAX_MAP_LEAFSURFACES * sizeof( bspLeafSurfaces[0] ),       pct );
    PrintBSPLumpSize( "collisionverts",    numBSPCollisionVerts,   numBSPCollisionVerts * sizeof( bspCollisionVerts[0] ),     MAX_MAP_COLLISIONVERTS * sizeof( bspCollisionVerts[0] ),   pct );
    PrintBSPLumpSize( "collisiontris",     numBSPCollisionTris,    numBSPCollisionTris * sizeof( bspCollisionTris[0] ),       MAX_MAP_COLLISIONTRIS * sizeof( bspCollisionTris[0] ),     pct );
    PrintBSPLumpSize( "collisionedgewalk", numBSPCollisionTris * 3, GetCollisionEdgeWalkSize( numBSPCollisionTris ),          GetCollisionEdgeWalkSize( MAX_MAP_COLLISIONTRIS ),         pct );
    PrintBSPLumpSize( "collisionborders",  numBSPCollisionBorders, numBSPCollisionBorders * sizeof( bspCollisionBorders[0] ), MAX_MAP_COLLISIONBORDERS * sizeof( bspCollisionBorders[0] ), pct );
    PrintBSPLumpSize( "collisionparts",    numBSPCollisionParts,   numBSPCollisionParts * sizeof( bspCollisionParts[0] ),     MAX_MAP_COLLISIONPARTITIONS * sizeof( bspCollisionParts[0] ), pct );
    PrintBSPLumpSize( "collisionaabbs",    numBSPCollisionAabbs,   numBSPCollisionAabbs * sizeof( bspCollisionAabbs[0] ),     MAX_MAP_COLLISIONAABBS * sizeof( bspCollisionAabbs[0] ),   pct );

    PrintBSPLumpSize( "layered verts",     numBSPDrawVerts[TRIS_TYPE_LAYERED],   numBSPDrawVerts[TRIS_TYPE_LAYERED] * sizeof( bspDrawVerts[0][0] ),     0x2200000,                                            pct );
    PrintBSPLumpSize( "layered data",      numBSPDrawVerts[TRIS_TYPE_LAYERED],   numBSPVertexLayerBytes,                                                MAX_MAP_VERTEXLAYERBYTES,                             pct );
    PrintBSPLumpSize( "simple verts",      numBSPDrawVerts[TRIS_TYPE_SIMPLE],    numBSPDrawVerts[TRIS_TYPE_SIMPLE] * sizeof( bspDrawVerts[0][0] ),      MAX_MAP_DRAWVERTS * sizeof( bspDrawVerts[0][0] ),     pct );
    PrintBSPLumpSize( "layered indexes",   numBSPDrawIndices[TRIS_TYPE_LAYERED], numBSPDrawIndices[TRIS_TYPE_LAYERED] * sizeof( bspDrawIndices[0][0] ), MAX_MAP_LAYERED_DRAWINDICES * sizeof( bspDrawIndices[0][0] ), pct );
    PrintBSPLumpSize( "simple indexes",    numBSPDrawIndices[TRIS_TYPE_SIMPLE],  numBSPDrawIndices[TRIS_TYPE_SIMPLE] * sizeof( bspDrawIndices[0][0] ),  MAX_MAP_DRAWINDICES * sizeof( bspDrawIndices[0][0] ), pct );
    PrintBSPLumpSize( "layered tri soups", numBSPTriSoups[TRIS_TYPE_LAYERED],    numBSPTriSoups[TRIS_TYPE_LAYERED] * sizeof( bspTriSoups[0][0] ),       MAX_MAP_TRISOUPS * sizeof( bspTriSoups[0][0] ),       pct );
    PrintBSPLumpSize( "simple tri soups",  numBSPTriSoups[TRIS_TYPE_SIMPLE],     numBSPTriSoups[TRIS_TYPE_SIMPLE] * sizeof( bspTriSoups[0][0] ),        MAX_MAP_TRISOUPS * sizeof( bspTriSoups[0][0] ),       pct );

    PrintBSPLumpSize( "lightmaps",         numBSPLightBytes / LIGHTMAP_BYTES, numBSPLightBytes,                                          sizeof( bspLightBytes ),                                 pct );
    PrintBSPLumpSize( "light grid header", 1,                                 GetLightGridHeaderSize(),                                  0,                                                       pct );
    PrintBSPLumpSize( "light grid rows",   GetLightGridRowCount(),            numBSPLightGridRowBytes,                                   MAX_MAP_LIGHTGRIDROWBYTES,                               pct );
    PrintBSPLumpSize( "light grid points", numBSPLightGridEntries,            numBSPLightGridEntries * sizeof( bspLightGridEntries[0] ),  MAX_MAP_LIGHTGRIDENTRIES * sizeof( bspLightGridEntries[0] ), pct );
    PrintBSPLumpSize( "light grid colors", numBSPLightGridColors,             numBSPLightGridColors * sizeof( bspLightGridColors[0] ),    MAX_MAP_LIGHTGRIDCOLORS * sizeof( bspLightGridColors[0] ),   pct );

    PrintBSPLumpSize( "visibility",        -1,                                numBSPVisBytes,                                            MAX_MAP_VISIBILITY,                                      pct );
    PrintBSPLumpSize( "portalverts",       numBSPPortalVerts,                 numBSPPortalVerts * sizeof( bspPortalVerts[0] ),            MAX_MAP_PORTALVERTS * sizeof( bspPortalVerts[0] ),       pct );
    PrintBSPLumpSize( "layered aabbtrees", numBSPAabbTrees[TRIS_TYPE_LAYERED], numBSPAabbTrees[TRIS_TYPE_LAYERED] * sizeof( bspAabbTrees[0][0] ), MAX_MAP_AABBTREES * sizeof( bspAabbTrees[0][0] ), pct );
    PrintBSPLumpSize( "simple aabbtrees",  numBSPAabbTrees[TRIS_TYPE_SIMPLE],  numBSPAabbTrees[TRIS_TYPE_SIMPLE] * sizeof( bspAabbTrees[0][0] ),  MAX_MAP_AABBTREES * sizeof( bspAabbTrees[0][0] ), pct );
    PrintBSPLumpSize( "cells",             numCells,                       numCells * sizeof( bspCells[0] ),                       MAX_MAP_CELLS * sizeof( bspCells[0] ),                   pct );
    PrintBSPLumpSize( "portals",           numBSPPortals,                     numBSPPortals * sizeof( bspPortals[0] ),                   MAX_MAP_PORTALS * sizeof( bspPortals[0] ),               pct );
    PrintBSPLumpSize( "cullgroups",        numCullGroups,                  numCullGroups * sizeof( bspCullGroups[0][0] ),          MAX_MAP_CULLGROUPS * sizeof( bspCullGroups[0][0] ),      pct );
    PrintBSPLumpSize( "cullgroupindexes",  numBSPCullGroupIndices,            numBSPCullGroupIndices * sizeof( bspCullGroupIndices[0] ), MAX_MAP_CULLGROUPINDICES * sizeof( bspCullGroupIndices[0] ), pct );
    PrintBSPLumpSize( "reflection_probes", numBSPReflectionProbes,            numBSPReflectionProbes * sizeof( bspReflectionProbes[0] ), MAX_MAP_REFLECTION_PROBES * sizeof( bspReflectionProbes[0] ), pct );
    PrintBSPLumpSize( "primary lights",    numBSPPrimaryLights,               numBSPPrimaryLights * sizeof( bspPrimaryLights[0] ),       MAX_MAP_PRIMARY_LIGHTS * sizeof( bspPrimaryLights[0] ),  pct );
    PrintBSPLumpSize( "light regions",     numBSPLightRegionBytes,            numBSPLightRegionBytes,                                    MAX_MAP_PRIMARY_LIGHTS,                                  pct );
    PrintBSPLumpSize( "light region hulls", numBSPLightRegionHulls,           numBSPLightRegionHulls * sizeof( bspLightRegionHulls[0] ), MAX_MAP_LIGHTREGIONHULLS * sizeof( bspLightRegionHulls[0] ), pct );
    PrintBSPLumpSize( "light region axes", numBSPLightRegionAxes,             numBSPLightRegionAxes * sizeof( bspLightRegionAxes[0] ),   MAX_MAP_LIGHTREGIONAXES * sizeof( bspLightRegionAxes[0] ), pct );
    printf( "\n" );

    PrintBSPLumpSize( "paths", numBSPPathBytes != 0, numBSPPathBytes, MAX_MAP_PATHBYTES, pct );
}

/* StripTrailing  0x0040cbf0 */
static void StripTrailing( char *e )
{
    char *s;

    for ( s = e + strlen( e ) - 1; s >= e && *s <= ' '; s-- )
        *s = '\0';
}

/* ParseEPair  0x0040cc40 */
epair_t *ParseEPair( char *key, const char **parsePos )
{
    epair_t *e;
    char    *value;

    e = (epair_t *)malloc( sizeof( epair_t ) );
    memset( e, 0, sizeof( epair_t ) );

    if ( strlen( key ) >= MAX_KEY - 1 )
        Com_Error( "ParseEpair: token too long" );
    if ( key[strlen( key ) - 2] == '\\' )
        Com_Error( "ParseEpair: key '%s' ends with a '\\'\n", key );
    if ( strchr( key, '\n' ) || strchr( key, '\r' ) )
        Com_Error( "ParseEpair: key '%s' contains a newline character\n", key );
    if ( strchr( key, '\"' ) )
        Com_Error( "ParseEpair: key '%s' contains a \" character, will cause parsing errors\n", key );

    e->key = _strdup( key );

    value = COM_ParseExt( parsePos );

    if ( strlen( value ) >= MAX_TOKEN_CHARS - 1 )
        Com_Error( "ParseEpair: token too long" );
    if ( value[strlen( value ) - 2] == '\\' )
        Com_Error( "ParseEpair: value '%s' ends with a '\\'\n", value );
    if ( strchr( value, '\n' ) || strchr( value, '\r' ) )
        Com_Error( "ParseEpair: value '%s' contains a newline character (use of '\\' at end of value?)\n", value );
    if ( strchr( value, '\"' ) )
        Com_Error( "ParseEpair: value '%s' contains a \" character, will cause parsing errors\n", value );

    e->value = _strdup( value );

    StripTrailing( e->key );
    StripTrailing( e->value );

    return e;
}

/* FreeEPairs  0x0040ce00 */
void FreeEPairs( epair_t *epairs )
{
    epair_t *ep;
    epair_t *next;

    for ( ep = epairs; ep; ep = next )
    {
        next = ep->next;
        free( ep->key );
        free( ep->value );
        free( ep );
    }
}

/* ParseEntity  0x0040ce50 */
qboolean ParseEntity( const char **parsePos, Entity_t *ents, int *numEnts )
{
    char     *token;
    Entity_t *mapent;
    epair_t  *e;

    token = COM_Parse( parsePos );
    if ( !token[0] )
        return qfalse;

    if ( strcmp( token, "{" ) )
        Com_Error( "ParseEntity: { not found" );

    if ( *numEnts == MAX_MAP_ENTITIES )
        Com_Error( "MAX_MAP_ENTITIES exceeded" );

    mapent = &ents[*numEnts];
    ( *numEnts )++;

    while ( 1 )
    {
        token = COM_Parse( parsePos );
        if ( !token[0] )
            Com_Error( "ParseEntity: EOF without closing brace" );
        if ( !strcmp( token, "}" ) )
            break;

        e = ParseEPair( token, parsePos );
        e->next = mapent->epairs;
        mapent->epairs = e;
    }

    return qtrue;
}

/* ParseEntities  0x0040cf50 */
void ParseEntities( void )
{
    ParseEntitiesFromBuffer( bspEntData, entities, &num_entities );
}

/* ParseEntitiesFromBuffer  0x0040cf70 */
void ParseEntitiesFromBuffer( const char *buf, Entity_t *ents, int *numEnts )
{
    *numEnts = 0;

    Com_BeginParseSession( "LUMP_ENTITIES" );
    while ( ParseEntity( &buf, ents, numEnts ) )
        ;
    Com_EndParseSession();
}

/* UnparseEntities  0x0040cfb0 */
void UnparseEntities( void )
{
    char     *buf;
    char     *end;
    Entity_t *ent;
    epair_t  *ep;
    char      line[2048];
    char      key[MAX_TOKEN_CHARS + 4];
    char      value[MAX_TOKEN_CHARS + 4];
    int       i;

    buf = bspEntData;
    end = buf;
    *end = '\0';

    for ( i = 0; i < num_entities; i++ )
    {
        ent = &entities[i];
        if ( !ent->epairs )
            continue;

        strcat( end, "{\n" );
        end += 2;

        for ( ep = ent->epairs; ep; ep = ep->next )
        {
            strcpy( key, ep->key );
            StripTrailing( key );
            strcpy( value, ep->value );
            StripTrailing( value );

            sprintf( line, "\"%s\" \"%s\"\n", key, value );
            strcat( end, line );
            end += strlen( line );
        }

        strcat( end, "}\n" );
        end += 2;

        Assert( end == buf + strlen( buf ) );

        if ( end > buf + MAX_MAP_ENTSTRING )
            Com_Error( "Entity string buffer overflow.  This is caused by too many entities and/or "
                       "too many key pairs.  Max may need to be increased." );
    }

    bspEntDataSize = end - buf + 1;
}

/* FindEntityWithPair  0x0040d1e0 */
Entity_t *FindEntityWithPair( Entity_t *ents, int numEnts, const char *key, const char *value )
{
    Entity_t *ent;
    epair_t  *ep;
    int       entIter;

    Assert( ents );
    Assert( key );
    Assert( value );

    for ( entIter = 0; entIter < numEnts; entIter++ )
    {
        ent = &ents[entIter];
        for ( ep = ent->epairs; ep; ep = ep->next )
        {
            if ( !strcmp( ep->key, key ) && !strcmp( ep->value, value ) )
                return ent;
        }
    }

    return NULL;
}

/* UnparseEntitiesWithOrigins  0x0040d2e0 */
void UnparseEntitiesWithOrigins( void )
{
    Entity_t  oldEnts[MAX_MAP_ENTITIES];
    int       numOldEnts;
    Entity_t *ent;
    epair_t  *ep;
    int       entIter;

    ParseEntitiesFromBuffer( bspEntData, oldEnts, &numOldEnts );

    for ( entIter = 0; entIter < num_entities; entIter++ )
    {
        ent = &entities[entIter];
        for ( ep = ent->epairs; ep; ep = ep->next )
        {
            if ( !strcmp( ep->key, "classname" ) && !strcmp( ep->value, "misc_model" ) )
                CopyMiscModelKeys( oldEnts, numOldEnts, ent );

            if ( !strcmp( ep->key, "model" ) && ep->value[0] == '*' )
                CopyBrushModelOrigin( oldEnts, numOldEnts, ent );
        }
    }

    UnparseEntities();
}

/* CopyMiscModelKeys  0x0040d410 */
static void CopyMiscModelKeys( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt )
{
    Entity_t   *oldEnt;
    const char *gndLt;

    Assert( oldEnts );
    Assert( newEnt );

    oldEnt = FindOldMiscModelEntity( oldEnts, numOldEnts, newEnt );
    if ( !oldEnt )
        return;

    gndLt = ValueForKey( oldEnt, "gndLt" );
    if ( gndLt[0] )
        SetKeyValue( newEnt, "gndLt", gndLt );
}

/* FindOldMiscModelEntity  0x0040d4c0 */
static Entity_t *FindOldMiscModelEntity( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt )
{
    const char *newOrigin;
    const char *newModel;
    Entity_t   *oldEnt;
    int         entIter;

    Assert( oldEnts );
    Assert( newEnt );

    newOrigin = ValueForKey( newEnt, "origin" );
    newModel  = ValueForKey( newEnt, "model" );

    if ( !newOrigin[0] || !newModel[0] )
        return NULL;

    for ( entIter = 0; entIter < numOldEnts; entIter++ )
    {
        oldEnt = &oldEnts[entIter];

        if ( !strcmp( ValueForKey( oldEnt, "classname" ), "misc_model" ) )
        {
            if ( !strcmp( ValueForKey( oldEnt, "origin" ), newOrigin ) )
            {
                if ( !strcmp( ValueForKey( oldEnt, "model" ), newModel ) )
                    return oldEnt;
            }
        }
    }

    return NULL;
}

/* CopyBrushModelOrigin  0x0040d620 */
static void CopyBrushModelOrigin( Entity_t *oldEnts, int numOldEnts, Entity_t *newEnt )
{
    const char *oldEntityModel;
    const char *origin;
    Entity_t   *oldEnt;

    Assert( oldEnts );
    Assert( newEnt );

    oldEntityModel = ValueForKey( newEnt, "model" );
    Assert( oldEntityModel[0] == '*' );

    oldEnt = FindEntityWithPair( oldEnts, numOldEnts, "model", oldEntityModel );
    if ( !oldEnt )
        return;

    origin = ValueForKey( oldEnt, "origin" );
    if ( origin[0] )
        SetKeyValue( newEnt, "origin", origin );
}

/* PrintEntity  0x0040d710 */
void PrintEntity( const Entity_t *ent )
{
    const epair_t *ep;

    printf( "------- entity %p -------\n", ent );
    for ( ep = ent->epairs; ep; ep = ep->next )
        printf( "%s = %s\n", ep->key, ep->value );
}

/* RemoveKey  0x0040d760 */
void RemoveKey( Entity_t *ent, const char *key )
{
    epair_t **prev;
    epair_t  *ep;

    prev = &ent->epairs;
    while ( *prev )
    {
        ep = *prev;
        if ( !_stricmp( ep->key, key ) )
        {
            *prev = ep->next;
            free( ep->key );
            free( ep->value );
            free( ep );
            return;
        }
        prev = &ep->next;
    }
}

/* SetKeyValue  0x0040d7e0 */
void SetKeyValue( Entity_t *ent, const char *key, const char *value )
{
    ent->epairs = SetKeyValueInPairs( ent->epairs, key, value );
}

/* ValueForKey  0x0040d810 */
const char *ValueForKey( const Entity_t *ent, const char *key )
{
    return ValueForKeyInPairs( ent->epairs, key );
}

/* SetKeyValueInPairs  0x0040d830 */
epair_t *SetKeyValueInPairs( epair_t *epairs, const char *key, const char *value )
{
    epair_t *ep;

    for ( ep = epairs; ep; ep = ep->next )
    {
        if ( !_stricmp( ep->key, key ) )
        {
            free( ep->value );
            ep->value = _strdup( value );
            return epairs;
        }
    }

    ep = (epair_t *)malloc( sizeof( epair_t ) );
    ep->next  = epairs;
    ep->key   = _strdup( key );
    ep->value = _strdup( value );

    return ep;
}

/* RemoveKeyFromPairs  0x0040d8d0 */
void RemoveKeyFromPairs( epair_t **epairs, const char *key )
{
    epair_t **prev;
    epair_t  *ep;

    prev = epairs;
    while ( *prev )
    {
        ep = *prev;
        if ( !_stricmp( ep->key, key ) )
        {
            *prev = ep->next;
            free( ep->key );
            free( ep->value );
            free( ep );
            return;
        }
        prev = &ep->next;
    }
}

/* KeyExistsInPairs  0x0040d940 */
qboolean KeyExistsInPairs( const epair_t *epairs, const char *key )
{
    const epair_t *ep;

    for ( ep = epairs; ep; ep = ep->next )
    {
        if ( !_stricmp( ep->key, key ) )
            return qtrue;
    }

    return qfalse;
}

/* ValueForKeyInPairs  0x0040d980 */
const char *ValueForKeyInPairs( const epair_t *epairs, const char *key )
{
    const epair_t *ep;

    for ( ep = epairs; ep; ep = ep->next )
    {
        if ( !_stricmp( ep->key, key ) )
            return ep->value;
    }

    return "";
}

/* GetVectorForKeyInPairs  0x0040d9c0 */
void GetVectorForKeyInPairs( const epair_t *epairs, const char *key, vec3_t out )
{
    const char *value;
    int         parsed;

    value = ValueForKeyInPairs( epairs, key );

    Vec3Clear( out );
    parsed = sscanf( value, "%f %f %f", &out[0], &out[1], &out[2] );
    if ( parsed == 1 || parsed == 2 )
    {
        printf( "WARNING: key '%s' with value '%s', when it should be a vector; "
                "treating as '%g %g %g'\n",
                key, value, out[0], out[1], out[2] );
    }
}

/* IntForKeyInPairs  0x0040da60 */
int IntForKeyInPairs( const epair_t *epairs, const char *key )
{
    return atoi( ValueForKeyInPairs( epairs, key ) );
}

/* FloatForKey  0x0040da90 */
float FloatForKey( const Entity_t *ent, const char *key )
{
    return (float)atof( ValueForKey( ent, key ) );
}

/* IntForKey  0x0040dac0 */
int IntForKey( const Entity_t *ent, const char *key )
{
    return atoi( ValueForKey( ent, key ) );
}

/* GetVectorForKey  0x0040daf0 */
void GetVectorForKey( const Entity_t *ent, const char *key, vec3_t out )
{
    const char *value;
    int         parsed;

    value = ValueForKey( ent, key );

    Vec3Clear( out );
    parsed = sscanf( value, "%f %f %f", &out[0], &out[1], &out[2] );
    if ( parsed == 1 || parsed == 2 )
    {
        printf( "WARNING: entity %i: key '%s' with value '%s', when it should be a vector; "
                "treating as '%g %g %g'\n",
                ent - entities, key, value, out[0], out[1], out[2] );
    }
}

/* LoadAndWriteBSPFile  0x0040dba0 */
void LoadAndWriteBSPFile( const char *inPath, const char *outPath )
{
    LoadBSPFileLumps( inPath, qfalse );
    WriteBSPFile( outPath );
}

/* SetBSPFileExtensions  0x0040dbc0 */
void SetBSPFileExtensions( const char *root )
{
    Assert( strlen( root ) + strlen( BSP_EXTENSION_SUFFIX ) < ARRAY_COUNT( bspFileExtension ) );
    Assert( strlen( root ) + strlen( PORTAL_EXTENSION_SUFFIX ) < ARRAY_COUNT( prtFileExtension ) );
    Assert( strlen( root ) + strlen( POLYFILE_EXTENSION_SUFFIX ) < ARRAY_COUNT( polyFileExtension ) );

    sprintf( bspFileExtension,  ".%s" BSP_EXTENSION_SUFFIX,      root );
    sprintf( prtFileExtension,  ".%s" PORTAL_EXTENSION_SUFFIX,   root );
    sprintf( polyFileExtension, ".%s" POLYFILE_EXTENSION_SUFFIX, root );
}

/* GetBSPFileExtension  0x0040dcd0 */
const char *GetBSPFileExtension( void )
{
    Assert( bspFileExtension[0] );
    return bspFileExtension;
}

/* GetPRTFileExtension  0x0040dd10 */
const char *GetPRTFileExtension( void )
{
    Assert( prtFileExtension[0] );
    return prtFileExtension;
}

/* GetPolyFileExtension  0x0040dd50 */
const char *GetPolyFileExtension( void )
{
    Assert( polyFileExtension[0] );
    return polyFileExtension;
}

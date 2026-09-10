/* Original: c:\trees\cod3\cod3src\cod2rad\geometry.cpp */

#include "cod4rad.h"
#include "geometry.h"
#include "modelcollision.h"
#include "progress.h"
#include "materials.h"
#include "lighting.h"
#include "threads.h"

#include "progress.h"
#include "cmdline.h"
#include "com_math.h"
#include "cm_tracebox.h"
#include "linearmapping.h"
#include "compile.h"
#include "polylib.h"
#include "lightgrid.h"

#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <new>
#include <float.h>


#define GEO_CHECK( name, cond )  typedef char name[( cond ) ? 1 : -1]

GEO_CHECK( geo_xyz_at_4,        offsetof( geoGlob_t, worldSpaceXyz ) == 0x4 );
GEO_CHECK( geo_mins_after_xyz,  offsetof( geoGlob_t, mins ) == 0x6C0004 );
GEO_CHECK( geo_modelorient,     offsetof( geoGlob_t, modelOrientation ) == 0x6C001C );
GEO_CHECK( geo_trirefused,      offsetof( geoGlob_t, triRefUsed ) == 0x6EFFEC );

GEO_CHECK( geo_firstbrushside,  offsetof( geoGlob_t, firstBrushSide )
                                - offsetof( geoGlob_t, triRefUsed ) == 0x024 );
GEO_CHECK( geo_hunk,            offsetof( geoGlob_t, hunk )
                                - offsetof( geoGlob_t, triRefUsed ) == 0x028 );
GEO_CHECK( geo_ssaa_offset,     offsetof( geoGlob_t, supersampleAlphaOffset )
                                - offsetof( geoGlob_t, triRefUsed ) == 0x058 );
GEO_CHECK( geo_ssaa_fills,      sizeof( geoGlob.supersampleAlphaOffset ) == 0x150 - 0x058 );
GEO_CHECK( geo_trimin,          offsetof( geoGlob_t, triMin )
                                - offsetof( geoGlob_t, triRefUsed ) == 0x150 );

GEO_CHECK( geo_hunk_size,       sizeof( GeoHunk_t ) == 12 );
GEO_CHECK( geo_mskmtl_size,     sizeof( MskMaterial_t ) == 28 );
GEO_CHECK( geo_orient_size,     sizeof( orientation_t ) == 48 );
GEO_CHECK( geo_tri_size,        sizeof( GeoTriangle_t ) == 36 );
GEO_CHECK( geo_vert_size,       sizeof( GeoVertex_t ) == 68 );
GEO_CHECK( geo_hit_size,        sizeof( GeoHit_t ) == 24 );
GEO_CHECK( geo_node_size,       sizeof( GeoBspNode_t ) == 24 );
GEO_CHECK( geo_leaf_size,       sizeof( GeoBspLeaf_t ) == 8 );
GEO_CHECK( geo_lmap_plane_size, sizeof( LmapPlane_t ) == 36 );
GEO_CHECK( geo_transport_size,  sizeof( TransportTri_t ) == 156 );
GEO_CHECK( geo_transport_xyz,   offsetof( TransportTri_t, xyz ) == 0x0c );

GEO_CHECK( geo_trace_size,      sizeof( GeoTrace_t ) == 0x33C );
GEO_CHECK( geo_trace_result,    offsetof( GeoTrace_t, result ) == 0x4C );
GEO_CHECK( geo_trace_hits,      offsetof( GeoTrace_t, result.hits ) == 0x54 );


geoGlob_t geoGlob;                          /* 0x13063148 */

int           geoTriCount;                  /* 0x11bf3158 */
GeoVertex_t *const geoVertices =
    ( GeoVertex_t * )bspDrawVerts[TRIS_TYPE_LAYERED];  /* 0x0a8a4510 */
GeoTriangle_t geoTris[GEO_MAX_TRIANGLES];   /* 0x11bf315c */


/* Geo_HunkAlloc  0x00408830 */
static byte *Geo_HunkAlloc( GeoHunk_t *hunk, int size )
{
    byte *at;

    if ( hunk->used + size > hunk->size )
        Error( "Small hunk ran out of memory" );

    at = hunk->base + hunk->used;
    hunk->used += size;

    return at;
}

/* SurfaceCastsShadow  0x00408b50 */
qboolean SurfaceCastsShadow( const MskMaterial_t *mskMtl )
{
    const Material_t *material = mskMtl->material;

    if ( material->surfaceFlags & SURF_SKY )
        return qtrue;

    if ( material->surfaceFlags & SURF_NOCASTSHADOW )
        return qfalse;

    if ( ( material->toolFlags & TOOLFLAG_USAGE_MASK ) == TOOLFLAG_USAGE_LIT )
        return qtrue;

    if ( !mskMtl->mask )
        return qfalse;

    return qtrue;
}

/* Geo_ConvertBspNode  0x0040b120 */
void Geo_ConvertBspNode( const BspNode_t *dnode, GeoBspNode_t *bspNode )
{
    const BspPlane_t *plane;

    Assert( dnode );
    Assert( bspNode );

    plane = &bspPlanes[dnode->planeNum];

    bspNode->plane.normal[0] = plane->normal[0];
    bspNode->plane.normal[1] = plane->normal[1];
    bspNode->plane.normal[2] = plane->normal[2];
    bspNode->plane.dist      = plane->dist;

    bspNode->children[0] = dnode->children[0];
    bspNode->children[1] = dnode->children[1];
}

/* Geo_CopyBspNodes  0x0040b1a0 */
void Geo_CopyBspNodes( void )
{
    int i;

    Assert( geoGlob.bspNodes );

    geoGlob.bspNodeCount = numBSPNodes;

    for ( i = 0; i < geoGlob.bspNodeCount; i++ )
        Geo_ConvertBspNode( &bspNodes[i], &geoGlob.bspNodes[i] );
}

/* Geo_AllocBspLeaf  0x0040ad90 */
int Geo_AllocBspLeaf( GeoTriangle_t *const *triRefs, int triCount )
{
    GeoBspLeaf_t *leaf;
    int           index;

    Assert( triRefs );
    Assert( triCount );

    if ( geoGlob.triRefUsed + triCount > geoGlob.triRefCount )
        Error( "Ran out of triangle refs.\n"
               "This can happen in a big map with simple geometry and default blocksize.\n"
               "Try using -blocksize 0 in cod2map.\n" );

    Assertx( geoGlob.bspLeafCount < GEO_MAX_NODES + 1,
             "(geoGlob.bspLeafCount) = %i", geoGlob.bspLeafCount );

    index = geoGlob.bspLeafCount;
    geoGlob.bspLeafCount = index + 1;

    leaf = &geoGlob.bspLeafs[index];

    leaf->triCount    = triCount;
    leaf->firstTriRef = geoGlob.triRefUsed;

    geoGlob.triRefUsed += triCount;

    memcpy( &geoGlob.triRefs[leaf->firstTriRef], triRefs,
            triCount * sizeof( GeoTriangle_t * ) );

    return GEO_LEAF_TO_CHILD( index );
}


/* Geo_ComputeTriangleNormal  0x004088b0 */
static qboolean Geo_ComputeTriangleNormal( GeoTriangle_t *tri )
{
    const float *v0 = geoGlob.worldSpaceXyz[tri->indices[0]];
    const float *v1 = geoGlob.worldSpaceXyz[tri->indices[1]];
    const float *v2 = geoGlob.worldSpaceXyz[tri->indices[2]];
    vec3_t       d1;
    vec3_t       d2;

    d1[0] = v1[0] - v0[0];
    d1[1] = v1[1] - v0[1];
    d1[2] = v1[2] - v0[2];

    d2[0] = v2[0] - v0[0];
    d2[1] = v2[1] - v0[1];
    d2[2] = v2[2] - v0[2];

    Vec3Cross( d2, d1, tri->normal );

    if ( Vec3Normalize( tri->normal ) == 0.0f )
        return qfalse;

    return qtrue;
}

/* Geo_AddVertex  0x00408cb0 */
static int Geo_AddVertex( const vec3_t xyz, const vec2_t st )
{
    GeoVertex_t *vert;
    unsigned     i;
    unsigned     lo;
    int          index;

    lo = geoGlob.worldSpaceXyzCount - GEO_VERTEX_SEARCH_DEPTH;

    if ( lo <= ( unsigned )numBSPDrawVerts[0] )
        lo = numBSPDrawVerts[0];

    for ( i = geoGlob.worldSpaceXyzCount - 1; i >= lo; i-- )
    {
        if ( geoGlob.worldSpaceXyz[i][0] != xyz[0] )
            continue;

        if ( geoGlob.worldSpaceXyz[i][1] != xyz[1] )
            continue;

        if ( geoGlob.worldSpaceXyz[i][2] != xyz[2] )
            continue;

        if ( geoVertices[i].texCoord[0] != st[0] )
            continue;

        if ( geoVertices[i].texCoord[1] != st[1] )
            continue;

        return i;
    }

    if ( geoGlob.worldSpaceXyzCount == GEO_MAX_DRAW_VERTS )
        Error( "MAX_MAP_DRAW_VERTS (%i) exceeded.  The BSP has %i verts for visible"
               " geometry, leaving only %i verts for invisible shadow casters.",
               GEO_MAX_DRAW_VERTS, numBSPDrawVerts[0],
               GEO_MAX_DRAW_VERTS - numBSPDrawVerts[0] );

    if ( geoTriCount == GEO_MAX_TRIANGLES )
        Error( "MAX_MAP_TRIANGLES (%i) exceeded.  The BSP has %i triangles for visible"
               " geometry, leaving only %i triangles for invisible shadow casters.",
               GEO_MAX_TRIANGLES, GEO_MAX_TRIANGLES, 0 );

    index = geoGlob.worldSpaceXyzCount;
    geoGlob.worldSpaceXyzCount = index + 1;

    geoGlob.worldSpaceXyz[index][0] = xyz[0];
    geoGlob.worldSpaceXyz[index][1] = xyz[1];
    geoGlob.worldSpaceXyz[index][2] = xyz[2];

    vert = &geoVertices[index];

    vert->xyz[0] = xyz[0];
    vert->xyz[1] = xyz[1];
    vert->xyz[2] = xyz[2];

    vert->texCoord[0] = st[0];
    vert->texCoord[1] = st[1];

    vert->normal[0] = 0.0f;
    vert->normal[1] = 0.0f;
    vert->normal[2] = 1.0f;

    vert->lmapCoord[0] = 0.0f;
    vert->lmapCoord[1] = 0.0f;

    vert->tangent[0] = 1.0f;
    vert->tangent[1] = 0.0f;
    vert->tangent[2] = 0.0f;

    vert->binormal[0] = 0.0f;
    vert->binormal[1] = 1.0f;
    vert->binormal[2] = 0.0f;

    vert->pad18 = -1;

    return index;
}

/* Geo_AddTriangle  0x00408e70 */
void Geo_AddTriangle( MskMaterial_t *mskMtl,
                      const vec3_t xyz0, const vec3_t xyz1, const vec3_t xyz2,
                      const vec2_t st0, const vec2_t st1, const vec2_t st2 )
{
    GeoTriangle_t *tri;

    Assert( mskMtl );

    if ( !SurfaceCastsShadow( mskMtl ) )
        return;

    tri = &geoTris[geoTriCount];

    tri->indices[0] = Geo_AddVertex( xyz0, st0 );
    tri->indices[1] = Geo_AddVertex( xyz1, st1 );
    tri->indices[2] = Geo_AddVertex( xyz2, st2 );

    tri->mskMtl = mskMtl;

    tri->modelIndex        = 0;
    tri->primaryLightIndex = 0;
    tri->lightmapIndex     = GEO_LIGHTMAP_NONE;

    if ( Geo_ComputeTriangleNormal( tri ) )
        geoTriCount++;
}

/* Geo_TriangleSide  0x0040a360 */
static int Geo_TriangleSide( const GeoTriangle_t *tri, const GeoPlane_t *plane, float epsilon )
{
    qboolean front = qfalse;
    qboolean back  = qfalse;
    int      i;

    for ( i = 0; i < 3; i++ )
    {
        const float *xyz = geoGlob.worldSpaceXyz[tri->indices[i]];
        float        d;

        d = xyz[0] * plane->normal[0]
          + xyz[1] * plane->normal[1]
          + xyz[2] * plane->normal[2];
        d = d - plane->dist;

        if ( d > epsilon )
            front = qtrue;
        else if ( d < -epsilon )
            back = qtrue;
    }

    if ( front )
        return back ? SIDE_CROSS : SIDE_FRONT;

    return back ? SIDE_BACK : SIDE_ON;
}

/* Geo_SplitCost  0x0040a490 */
static int Geo_SplitCost( const int *sideCount )
{
    return abs( sideCount[SIDE_FRONT] - sideCount[SIDE_BACK] )
         + 2 * ( sideCount[SIDE_ON] + sideCount[SIDE_CROSS] );
}

/* Geo_PartitionBySide  0x0040ae70 */
static int Geo_PartitionBySide( GeoTriangle_t **triList, int triCount,
                                const GeoPlane_t *plane, float epsilon, int side )
{
    int i;

    if ( triCount <= 0 )
        return triCount;

    i = 0;

    do
    {
        GeoTriangle_t *tri = triList[i];

        if ( Geo_TriangleSide( tri, plane, epsilon ) == side )
        {
            i++;
        }
        else
        {
            triCount--;
            triList[i]        = triList[triCount];
            triList[triCount] = tri;
        }
    }
    while ( i < triCount );

    return triCount;
}

/* Geo_CompareFloat  0x0040a530 */
static int Geo_CompareFloat( const void *a, const void *b )
{
    float d = *(const float *)a - *(const float *)b;

    if ( d < 0 )
        return -1;

    if ( d > 0 )
        return 1;

    return 0;
}

/* Geo_TriangleAxisExtent  0x0040a570 */
static void Geo_TriangleAxisExtent( const GeoTriangle_t *tri, int axis, float *minMax )
{
    int i;

    minMax[0] = geoGlob.worldSpaceXyz[tri->indices[0]][axis];
    minMax[1] = minMax[0];

    for ( i = 1; i < 3; i++ )
    {
        float v = geoGlob.worldSpaceXyz[tri->indices[i]][axis];

        if ( v < minMax[0] )
            minMax[0] = v;
        else if ( v > minMax[1] )
            minMax[1] = v;
    }
}

/* Geo_TrianglePlane  0x0040ac20 */
static void Geo_TrianglePlane( const GeoTriangle_t *tri, GeoPlane_t *plane )
{
    const float *xyz = geoGlob.worldSpaceXyz[tri->indices[0]];

    plane->normal[0] = tri->normal[0];
    plane->normal[1] = tri->normal[1];
    plane->normal[2] = tri->normal[2];

    plane->dist = xyz[0] * tri->normal[0]
                + xyz[1] * tri->normal[1]
                + xyz[2] * tri->normal[2];
}

/* Geo_ChooseAxialSplitPlane  0x0040a610 */
static int Geo_ChooseAxialSplitPlane( GeoTriangle_t **triList, int triCount, GeoBspNode_t *node )
{
    float minMax[2];
    float bestDist;
    int   bestCost;
    int   bestAxis;
    int   axis;

    bestDist = 0.0f;
    bestCost = 0x7fffffff;
    bestAxis = -1;

    for ( axis = 0; axis < 3; axis++ )
    {
        int   countMinMax;
        int   countCoplanar;
        int   iMin, iMax, iFlat;
        int   startCount, onCount;
        int   sideCount[4];
        float pos, prevPos, nextPos;
        int   i;

        countMinMax   = 0;
        countCoplanar = 0;

        for ( i = 0; i < triCount; i++ )
        {
            Geo_TriangleAxisExtent( triList[i], axis, minMax );

            if ( minMax[0] == minMax[1] )
            {
                geoGlob.triFlat[countCoplanar++] = minMax[0];
            }
            else
            {
                geoGlob.triMin[countMinMax] = minMax[0];
                geoGlob.triMax[countMinMax] = minMax[1];
                countMinMax++;
            }
        }

        Assert( countMinMax <= triCount );
        Assert( countCoplanar <= triCount );
        Assert( countMinMax + countCoplanar == triCount );

        qsort( geoGlob.triMin,  countMinMax,   sizeof( float ), Geo_CompareFloat );
        qsort( geoGlob.triMax,  countMinMax,   sizeof( float ), Geo_CompareFloat );
        qsort( geoGlob.triFlat, countCoplanar, sizeof( float ), Geo_CompareFloat );

        sideCount[SIDE_FRONT] = triCount;
        sideCount[SIDE_BACK]  = 0;
        sideCount[SIDE_ON]    = 0;
        sideCount[SIDE_CROSS] = 0;

        iMin       = 0;
        iMax       = 0;
        iFlat      = 0;
        startCount = 0;
        onCount    = 0;

        prevPos = GEO_SWEEP_START;

        nextPos = geoGlob.triFlat[0] <= geoGlob.triMin[0] ? geoGlob.triFlat[0]
                                                          : geoGlob.triMin[0];

        while ( nextPos < FLT_MAX )
        {
            sideCount[SIDE_CROSS] += startCount;
            sideCount[SIDE_FRONT] -= startCount;

            pos     = nextPos;
            nextPos = FLT_MAX;

            startCount = 0;

            while ( iMin < countMinMax && geoGlob.triMin[iMin] == pos )
            {
                iMin++;
                startCount++;
            }

            if ( iMin < countMinMax && geoGlob.triMin[iMin] < nextPos )
                nextPos = geoGlob.triMin[iMin];

            while ( iMax < countMinMax && geoGlob.triMax[iMax] == pos )
            {
                iMax++;
                sideCount[SIDE_CROSS]--;
                sideCount[SIDE_BACK]++;
            }

            if ( iMax < countMinMax && geoGlob.triMax[iMax] < nextPos )
                nextPos = geoGlob.triMax[iMax];

            sideCount[SIDE_ON]   -= onCount;
            sideCount[SIDE_BACK] += onCount;

            onCount = 0;

            while ( iFlat < countCoplanar && geoGlob.triFlat[iFlat] == pos )
            {
                iFlat++;
                onCount++;
            }

            sideCount[SIDE_ON]    += onCount;
            sideCount[SIDE_FRONT] -= onCount;

            if ( iFlat < countCoplanar && geoGlob.triFlat[iFlat] < nextPos )
                nextPos = geoGlob.triFlat[iFlat];

            Assert( sideCount[SIDE_FRONT] + sideCount[SIDE_BACK]
                  + sideCount[SIDE_CROSS] + sideCount[SIDE_ON] == triCount );
            Assert( sideCount[SIDE_FRONT] >= 0 );
            Assert( sideCount[SIDE_BACK] >= 0 );
            Assert( sideCount[SIDE_CROSS] >= 0 );
            Assert( sideCount[SIDE_ON] >= 0 );

            if ( nextPos - pos >= 2 * GEO_SPLIT_EPSILON
              && pos - prevPos >= 2 * GEO_SPLIT_EPSILON
              && sideCount[SIDE_BACK]
              && sideCount[SIDE_FRONT]
              && abs( sideCount[SIDE_BACK] - sideCount[SIDE_FRONT] ) <= triCount / 2 )
            {
                int cost = Geo_SplitCost( sideCount );

                if ( cost < bestCost )
                {
                    bestDist = pos;
                    bestCost = cost;
                    bestAxis = axis;
                }
            }

            prevPos = pos;
        }
    }

    if ( bestAxis >= 0 )
    {
        node->plane.normal[0] = 0.0f;
        node->plane.normal[1] = 0.0f;
        node->plane.normal[2] = 0.0f;
        node->plane.normal[bestAxis] = 1.0f;
        node->plane.dist = bestDist;
    }

    return bestCost;
}

/* Geo_ChooseSplitPlane  0x0040ac60 */
static qboolean Geo_ChooseSplitPlane( GeoTriangle_t **triList, int triCount, GeoBspNode_t *node )
{
    GeoPlane_t plane;
    int        sideCount[4];
    int        bestCost;
    int        cost;
    int        i, j;

    if ( triCount <= GEO_MAX_LEAF_TRIS )
        return qfalse;

    bestCost = Geo_ChooseAxialSplitPlane( triList, triCount, node );

    if ( triCount <= GEO_MAX_SPLIT_CANDIDATES )
    {
        for ( i = 0; i < triCount; i++ )
        {
            Geo_TrianglePlane( triList[i], &plane );

            sideCount[SIDE_FRONT] = 0;
            sideCount[SIDE_BACK]  = 0;
            sideCount[SIDE_ON]    = 0;
            sideCount[SIDE_CROSS] = 0;

            for ( j = 0; j < triCount; j++ )
                sideCount[Geo_TriangleSide( triList[j], &plane, GEO_SPLIT_EPSILON )]++;

            if ( !sideCount[SIDE_FRONT] || !sideCount[SIDE_BACK] )
                cost = 0x7fffffff;
            else
                cost = Geo_SplitCost( sideCount );

            if ( cost < bestCost )
            {
                bestCost = cost;
                node->plane = plane;
            }
        }
    }

    return bestCost < 0x7fffffff;
}

/* Geo_BuildBspTree_r  0x0040aec0 */
static int Geo_BuildBspTree_r( int child, GeoTriangle_t **triList, int triCount )
{
    GeoBspNode_t *node;
    int           side;
    int           matchCount;

    if ( !triCount )
        return GEO_CHILD_EMPTY;

    if ( child >= 0 )
    {
        node = &geoGlob.bspNodes[child];
    }
    else
    {
        child = geoGlob.bspNodeCount;
        node  = &geoGlob.bspNodes[child];

        if ( child == GEO_MAX_NODES || !Geo_ChooseSplitPlane( triList, triCount, node ) )
            return Geo_AllocBspLeaf( triList, triCount );

        node->children[0] = GEO_CHILD_EMPTY;
        node->children[1] = GEO_CHILD_EMPTY;
        geoGlob.bspNodeCount++;
    }

    for ( side = 1; side >= 0; side-- )
    {
        matchCount = Geo_PartitionBySide( triList, triCount, &node->plane,
                                          GEO_SPLIT_EPSILON, side );

        node->children[1 - side] = Geo_BuildBspTree_r( node->children[1 - side],
                                                       &triList[matchCount],
                                                       triCount - matchCount );
    }

    return child;
}

/* Geo_BuildBspTree  0x0040afa0 */
void Geo_BuildBspTree( void )
{
    GeoTriangle_t **triList;
    int             i;
    int             count;

    triList = new ( std::nothrow ) GeoTriangle_t *[geoTriCount];

    if ( !triList )
        Error( "Couldn't allocate %i bytes for triangle refs\n",
               geoTriCount * sizeof( GeoTriangle_t * ) );

    count = 0;

    for ( i = 0; i < geoTriCount; i++ )
    {
        GeoTriangle_t *tri = &geoTris[i];

        if ( tri->modelIndex )
            continue;

        if ( !SurfaceCastsShadow( tri->mskMtl ) )
            continue;

        triList[count++] = tri;
    }

    geoGlob.triMin  = new ( std::nothrow ) float[count];
    geoGlob.triMax  = new ( std::nothrow ) float[count];
    geoGlob.triFlat = new ( std::nothrow ) float[count];

    if ( !geoGlob.triMin || !geoGlob.triMax || !geoGlob.triFlat )
        Error( "Couldn't allocate %i bytes for triangle bounds\n",
               count * 3 * sizeof( float ) );

    Geo_BuildBspTree_r( 0, triList, count );

    delete[] geoGlob.triFlat;
    delete[] geoGlob.triMax;
    delete[] geoGlob.triMin;
    delete[] triList;
}

/* Geo_SampleAlphaMask  0x00408f20 */
static qboolean Geo_SampleAlphaMask( const MskMaterial_t *mskMtl, const vec2_t st )
{
    int x, y, bit;

    Assert( ( ( mskMtl->maskWidth ) & ( ( mskMtl->maskWidth ) - 1 ) ) == 0 );
    Assert( ( ( mskMtl->maskHeight ) & ( ( mskMtl->maskHeight ) - 1 ) ) == 0 );

    x = RoundFloatToInt( mskMtl->maskWidth * st[0] );
    y = RoundFloatToInt( st[1] * mskMtl->maskHeight );

    x &= mskMtl->maskWidth - 1;
    y &= mskMtl->maskHeight - 1;

    bit = y * mskMtl->maskWidth + x;

    return ( mskMtl->mask[bit >> 3] & ( 1 << ( bit & 7 ) ) ) != 0;
}

/* Geo_SampleAlphaMaskAtHit  0x00409040 */
static int Geo_SampleAlphaMaskAtHit( const GeoTrace_t *trace, const vec2_t st0,
                                     const vec3_t edge2, const MskMaterial_t *mskMtl,
                                     const vec2_t st1, const vec2_t st2,
                                     float scale, const vec3_t edge1,
                                     float u, float v, int mask )
{
    vec2_t st;
    float  ds1, dt1, ds2, dt2;
    float  a, b;
    int    blocked;
    int    bit;
    int    i;

    ds1 = st1[0] - st0[0];
    dt1 = st1[1] - st0[1];
    ds2 = st2[0] - st0[0];
    dt2 = st2[1] - st0[1];

    st[0] = st0[0] + ds1 * u + ds2 * v;
    st[1] = st0[1] + dt1 * u + dt2 * v;

    if ( !trace->supersampleAlpha )
        return Geo_SampleAlphaMask( mskMtl, st ) ? mask : 0;

    a = -( float )( edge2[0] * trace->alphaAxis[0][0]
                  + edge2[1] * trace->alphaAxis[0][1]
                  + edge2[2] * trace->alphaAxis[0][2] ) * scale;

    ds1 = ds1 * a;
    dt1 = dt1 * a;

    b = ( float )( edge1[0] * trace->alphaAxis[1][0]
                 + edge1[1] * trace->alphaAxis[1][1]
                 + edge1[2] * trace->alphaAxis[1][2] ) * scale;

    ds2 = ds2 * b;
    dt2 = dt2 * b;

    blocked = 0;
    bit     = 1;

    for ( i = 0; i < options.supersampleAlphaCount; i++ )
    {
        if ( mask & bit )
        {
            vec2_t sample;
            float  ju = geoGlob.supersampleAlphaOffset[i][0];
            float  jv = geoGlob.supersampleAlphaOffset[i][1];

            sample[0] = st[0] + ju * ds1 + jv * ds2;
            sample[1] = st[1] + ju * dt1 + jv * dt2;

            if ( Geo_SampleAlphaMask( mskMtl, sample ) )
                blocked |= bit;
        }

        bit <<= 1;
    }

    return blocked;
}

/* Geo_AddHit  0x004091e0 */
static void Geo_AddHit( int geoType, const GeoTriangle_t *tri,
                        const MskMaterial_t *mskMtl, float frac,
                        float edgeDot2, float edgeDot1, float sign, float edgeDot3,
                        const vec3_t edge1, const vec3_t edge2,
                        const vec2_t st0, const vec2_t st1, const vec2_t st2,
                        GeoTrace_t *trace )
{
    GeoHit_t *hit;
    int       index;
    int       alive;
    int       blocked;
    int       i;

    index = trace->result.hitCount;

    while ( index && trace->result.hits[index - 1].frac > frac )
    {
        trace->result.hits[index] = trace->result.hits[index - 1];
        index--;
    }

    hit = &trace->result.hits[index];

    hit->geoType    = geoType;
    hit->tri        = tri;
    hit->u          = -edgeDot2 / edgeDot3;
    hit->v          = edgeDot1 / edgeDot3;
    hit->frac       = frac;

    alive = ( 1 << options.supersampleAlphaCount ) - 1;

    for ( i = 0; i < index; i++ )
        alive &= ~trace->result.hits[i].alphaMask;

    if ( !alive )
        return;

    if ( !mskMtl->mask )
    {
        hit->alphaMask  = alive;
        trace->result.hitCount = index + 1;
        trace->result.frac  = frac;
        return;
    }

    blocked = Geo_SampleAlphaMaskAtHit( trace, st0, edge2, mskMtl, st1, st2,
                                        sign / edgeDot3, edge1,
                                        hit->u, hit->v, alive );

    hit->alphaMask = blocked;
    alive &= ~blocked;

    index += ( blocked != 0 );

    for ( i = index; i <= trace->result.hitCount; i++ )
    {
        trace->result.hits[i].alphaMask &= ~blocked;

        if ( !trace->result.hits[i].alphaMask )
            continue;

        trace->result.hits[index] = trace->result.hits[i];
        alive &= ~trace->result.hits[i].alphaMask;
        index++;

        if ( !alive )
            break;
    }

    trace->result.hitCount = index;
}

/* Geo_TraceTriangle  0x00409390 */
static void Geo_TraceTriangle( GeoTrace_t *trace, const GeoTriangle_t *tri )
{
    const float *v0;
    const float *v1;
    const float *v2;
    vec3_t       edge1;
    vec3_t       edge2;
    vec3_t       cross;
    float        denom, sign, dist;
    float        edgeDot1, edgeDot2, edgeDot3;

    denom = trace->delta[0] * tri->normal[0]
          + trace->delta[1] * tri->normal[1]
          + trace->delta[2] * tri->normal[2];

    sign = 1.0f;

    if ( denom >= 0.0f )
    {
        denom = -denom;
        sign  = -1.0f;
    }

    v0 = geoGlob.worldSpaceXyz[tri->indices[0]];

    edge1[0] = v0[0] - trace->line.start[0];
    edge1[1] = v0[1] - trace->line.start[1];
    edge1[2] = v0[2] - trace->line.start[2];

    dist = edge1[0] * tri->normal[0]
         + edge1[1] * tri->normal[1]
         + edge1[2] * tri->normal[2];

    dist = dist * sign;

    if ( dist > 0.0f )
        return;

    if ( !( denom * trace->result.frac < dist ) )
        return;

    Vec3Cross( trace->delta, edge1, cross );

    v1 = geoGlob.worldSpaceXyz[tri->indices[1]];

    edge1[0] = v0[0] - v1[0];
    edge1[1] = v0[1] - v1[1];
    edge1[2] = v0[2] - v1[2];

    edgeDot1 = ( cross[0] * edge1[0] + cross[1] * edge1[1] + cross[2] * edge1[2] ) * sign;

    if ( 0.0f < edgeDot1 )
        return;

    v2 = geoGlob.worldSpaceXyz[tri->indices[2]];

    edge2[0] = v0[0] - v2[0];
    edge2[1] = v0[1] - v2[1];
    edge2[2] = v0[2] - v2[2];

    edgeDot2 = sign * ( cross[0] * edge2[0] + cross[1] * edge2[1] + cross[2] * edge2[2] );

    if ( 0.0f > edgeDot2 )
        return;

    Vec3Cross( edge2, edge1, cross );

    edgeDot3 = ( trace->delta[0] * cross[0]
               + trace->delta[1] * cross[1]
               + trace->delta[2] * cross[2] ) * sign;

    if ( edgeDot3 > edgeDot1 - edgeDot2 )
        return;

    Geo_AddHit( TRACE_HIT_WORLD_GEO, tri, tri->mskMtl, dist / denom,
                edgeDot2, edgeDot1, sign, edgeDot3,
                edge1, edge2,
                geoVertices[tri->indices[0]].texCoord,
                geoVertices[tri->indices[1]].texCoord,
                geoVertices[tri->indices[2]].texCoord,
                trace );
}

/* Geo_TraceModelTriangle  0x00409620 */
void Geo_TraceModelTriangle( const ModelCollTri_t *tri, int scripted, GeoTrace_t *trace )
{
    vec3_t normal;
    vec3_t tvec;
    vec3_t cross;
    float  denom, sign, dist;
    float  edgeDot1, edgeDot2;

    Vec3Cross( tri->edge1, tri->edge0, normal );

    denom = normal[0] * trace->delta[0]
          + normal[1] * trace->delta[1]
          + normal[2] * trace->delta[2];

    sign = 1.0f;

    if ( denom >= 0.0f )
    {
        denom = -denom;
        sign  = -1.0f;
    }

    tvec[0] = tri->xyz[0] - trace->line.start[0];
    tvec[1] = tri->xyz[1] - trace->line.start[1];
    tvec[2] = tri->xyz[2] - trace->line.start[2];

    dist = normal[0] * tvec[0] + normal[1] * tvec[1] + normal[2] * tvec[2];

    dist = dist * sign;

    if ( dist > 0.0f )
        return;

    if ( !( denom < dist ) )
        return;

    Vec3Cross( trace->delta, tvec, cross );

    edgeDot1 = cross[0] * tri->edge0[0] + cross[1] * tri->edge0[1]
             + cross[2] * tri->edge0[2];

    edgeDot1 = edgeDot1 * sign;

    if ( 0.0f < edgeDot1 )
        return;

    if ( denom > edgeDot1 )
        return;

    edgeDot2 = cross[0] * tri->edge1[0] + cross[1] * tri->edge1[1]
             + cross[2] * tri->edge1[2];

    edgeDot2 = edgeDot2 * sign;

    if ( 0.0f > edgeDot2 )
        return;

    if ( edgeDot1 - edgeDot2 < denom )
        return;

    Geo_AddHit( TRACE_HIT_MODEL_GEO, ( const GeoTriangle_t * )scripted,
                ( const MskMaterial_t * )tri->mskMtl,
                dist / denom, edgeDot2, edgeDot1, sign, denom,
                tri->edge0, tri->edge1, tri->st[0], tri->st[1], tri->st[2],
                trace );
}

/* Geo_TraceLeafTriangles  0x00409830 */
static void Geo_TraceLeafTriangles( GeoTrace_t *trace,
                                    GeoTriangle_t *const *triRefs, int triCount )
{
    int i;

    for ( i = 0; i < triCount; i++ )
        Geo_TraceTriangle( trace, triRefs[i] );
}

/* Geo_TraceNode_r  0x00409860 */
static void Geo_TraceNode_r( int child, double t0, double t1, GeoTrace_t *trace )
{
    while ( child >= 0 )
    {
        const GeoBspNode_t *node = &geoGlob.bspNodes[child];
        float               dot0, dot1;
        double              d0, d1, frac;
        int                 nearSide;

        dot0 = trace->line.start[0] * node->plane.normal[0]
             + trace->line.start[1] * node->plane.normal[1]
             + trace->line.start[2] * node->plane.normal[2];

        dot1 = trace->line.end[0] * node->plane.normal[0]
             + trace->line.end[1] * node->plane.normal[1]
             + trace->line.end[2] * node->plane.normal[2];

        d0 = dot0 - node->plane.dist;
        d1 = dot1 - node->plane.dist;

        if ( d0 - d1 == 0.0 )
        {
            child = node->children[d0 > 0.0 ? 0 : 1];
            continue;
        }

        frac = d0 / ( d0 - d1 );

        if ( frac < t0 )
        {
            child = node->children[d1 > 0.0 ? 0 : 1];
            continue;
        }

        nearSide = d0 < 0.0 ? 1 : 0;

        if ( t1 < frac )
        {
            child = node->children[nearSide];
            continue;
        }

        Geo_TraceNode_r( node->children[nearSide], t0, frac, trace );

        if ( frac >= trace->result.frac )
            return;

        child = node->children[1 - nearSide];
        t0    = frac;
    }

    if ( child == GEO_CHILD_EMPTY )
        return;

    {
        const GeoBspLeaf_t *leaf = &geoGlob.bspLeafs[GEO_CHILD_TO_LEAF( child )];

        Geo_TraceLeafTriangles( trace, &geoGlob.triRefs[leaf->firstTriRef],
                                leaf->triCount );
    }
}

/* Geo_TraceRay  0x004099d0 */
void Geo_TraceRay( GeoTrace_t *trace )
{
    Geo_TraceNode_r( 0, 0.0, 1.0, trace );
}

/* Geo_SetupTrace  0x00409a00 */
void Geo_SetupTrace( const vec3_t start, const vec3_t end,
                     const vec3_t axis0, const vec3_t axis1, GeoTrace_t *trace )
{
    trace->line.start[0] = start[0];
    trace->line.start[1] = start[1];
    trace->line.start[2] = start[2];

    trace->line.end[0] = end[0];
    trace->line.end[1] = end[1];
    trace->line.end[2] = end[2];

    trace->delta[0] = end[0] - start[0];
    trace->delta[1] = end[1] - start[1];
    trace->delta[2] = end[2] - start[2];

    CM_SetTraceInvDelta( &trace->line );

    trace->supersampleAlpha =
        ( byte )( options.supersampleAlphaCount > 1 && axis0 && axis1 );

    if ( trace->supersampleAlpha )
    {
        vec3_t rot0;
        vec3_t rot1;
        float  angle;
        float  s, c;

        angle = rand() * GEO_ALPHA_ANGLE_SCALE;

        SinCos( angle, &s, &c );

        rot0[0] = axis0[0] * c + axis1[0] * -s;
        rot0[1] = axis0[1] * c + axis1[1] * -s;
        rot0[2] = axis0[2] * c + axis1[2] * -s;

        rot1[0] = axis0[0] * s + axis1[0] * c;
        rot1[1] = axis0[1] * s + axis1[1] * c;
        rot1[2] = axis0[2] * s + axis1[2] * c;

        Vec3Cross( trace->delta, rot0, trace->alphaAxis[0] );
        Vec3Cross( trace->delta, rot1, trace->alphaAxis[1] );
    }

    trace->result.frac  = 1.0f;
    trace->result.hitCount = 0;
}

/* Geo_TraceLine  0x00409c50 */
void Geo_TraceLine( const vec3_t start, const vec3_t end, GeoTrace_t *trace )
{
    Geo_SetupTrace( start, end, NULL, NULL, trace );
    Geo_TraceRay( trace );
}

/* Geo_SampleMaskFraction  0x00409b60 */
float Geo_SampleMaskFraction( int sampleMask )
{
    unsigned count;

    if ( sampleMask == ( 1 << options.supersampleAlphaCount ) - 1 )
        return 1.0f;

    Assertx( sampleMask, "sampleMask" );

    count = 0;

    do
    {
        count++;
        sampleMask &= ( 1 << Log2ForBitCount( sampleMask ) ) - 1;
    }
    while ( sampleMask );

    return ( float )count / options.supersampleAlphaCount;
}

/* Geo_TraceOpenFraction  0x00409c00 */
float Geo_TraceOpenFraction( const GeoTraceResult_t *result )
{
    int      alive = ( 1 << options.supersampleAlphaCount ) - 1;
    unsigned i;

    for ( i = 0; i < ( unsigned )result->hitCount; i++ )
        alive &= ~result->hits[i].alphaMask;

    if ( !alive )
        return 0.0f;

    return Geo_SampleMaskFraction( alive );
}

static const Material_t *Geo_LastHitMaterial( const GeoTrace_t *trace )
{
    const GeoHit_t *lastHit;

    if ( !trace->result.hitCount )
        return NULL;

    lastHit = &trace->result.hits[trace->result.hitCount - 1];

    AssertCmp( lastHit->geoType, ==, TRACE_HIT_WORLD_GEO );

    return lastHit->tri->mskMtl->material;
}

/* Geo_TraceSeesSkyUp  0x00409c90 */
qboolean Geo_TraceSeesSkyUp( const vec3_t point )
{
    GeoTrace_t        trace;
    vec3_t            end;
    const Material_t *material;

    end[0] = point[0];
    end[1] = point[1];
    end[2] = point[2] + GEO_SKY_TRACE_DIST;

    Geo_SetupTrace( point, end, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    material = Geo_LastHitMaterial( &trace );

    if ( !material )
        return qtrue;

    return ( material->surfaceFlags >> 2 ) & 1;
}

/* Geo_TraceSeesSkyDown  0x00409d50 */
qboolean Geo_TraceSeesSkyDown( const vec3_t point )
{
    GeoTrace_t        trace;
    vec3_t            end;
    const Material_t *material;

    end[0] = point[0];
    end[1] = point[1];
    end[2] = point[2] - GEO_SKY_TRACE_DIST;

    Geo_SetupTrace( point, end, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    material = Geo_LastHitMaterial( &trace );

    if ( !material )
        return qtrue;

    if ( material->surfaceFlags & SURF_SKY )
        return qtrue;

    if ( material->contentFlags == CONTENTS_SOLID
         && ( material->surfaceFlags & SURF_NODRAW ) )
        return qtrue;

    return qfalse;
}

/* Geo_PointIsCoveredOverSky  0x00409e10 */
qboolean Geo_PointIsCoveredOverSky( const vec3_t point )
{
    if ( !Geo_TraceSeesSkyDown( point ) )
        return qfalse;

    if ( Geo_TraceSeesSkyUp( point ) )
        return qfalse;

    return qtrue;
}

/* Geo_PointIsOutsideWorld  0x00409e40 */
qboolean Geo_PointIsOutsideWorld( const vec3_t point )
{
    return !PointInBounds( point, geoGlob.mins, geoGlob.maxs );
}

/* Geo_PointInBrush  0x00409e70 */
static qboolean Geo_PointInBrush( int brushIndex, const vec3_t point )
{
    const BspBrushSide_t *sides = &bspBrushSides[geoGlob.firstBrushSide[brushIndex]];
    int                   sideCount;
    int                   i;

    if ( point[0] < sides[0].u.distance + GEO_BRUSH_EPSILON )
        return qfalse;

    if ( point[0] > sides[1].u.distance - GEO_BRUSH_EPSILON )
        return qfalse;

    if ( point[1] < sides[2].u.distance + GEO_BRUSH_EPSILON )
        return qfalse;

    if ( point[1] > sides[3].u.distance - GEO_BRUSH_EPSILON )
        return qfalse;

    if ( point[2] < sides[4].u.distance + GEO_BRUSH_EPSILON )
        return qfalse;

    if ( point[2] > sides[5].u.distance - GEO_BRUSH_EPSILON )
        return qfalse;

    sideCount = ( short )bspBrushes[brushIndex].numSides;

    for ( i = 6; i < sideCount; i++ )
    {
        const BspPlane_t *plane = &bspPlanes[sides[i].u.planeNum];
        float             d;

        d = point[0] * plane->normal[0]
          + point[1] * plane->normal[1]
          + point[2] * plane->normal[2];

        if ( d - plane->dist > -GEO_BRUSH_EPSILON )
            return qfalse;
    }

    return qtrue;
}

/* Geo_PointIsSolid  0x00409f70 */
qboolean Geo_PointIsSolid( const vec3_t point )
{
    const BspLeaf_t *leaf;
    const int       *leafBrushes;
    int              node;
    unsigned         i;

    node = 0;

    do
    {
        const BspNode_t  *bspNode = &bspNodes[node];
        const BspPlane_t *plane   = &bspPlanes[bspNode->planeNum];
        float             d;

        d = point[0] * plane->normal[0]
          + point[1] * plane->normal[1]
          + point[2] * plane->normal[2];

        node = bspNode->children[plane->dist > d ? 1 : 0];
    }
    while ( node >= 0 );

    leaf        = &bspLeafs[-1 - node];
    leafBrushes = &bspLeafBrushes[leaf->firstLeafBrush];

    for ( i = 0; i < ( unsigned )leaf->leafBrushCount; i++ )
    {
        int brushIndex = leafBrushes[i];

        if ( bspMaterials[bspBrushes[brushIndex].materialNum].contentFlags
             & GEO_CONTENTS_SKIP_BRUSH )
            continue;

        if ( Geo_PointInBrush( brushIndex, point ) )
            return qtrue;
    }

    return qfalse;
}

/* Geo_BoxPlaneSide  0x0040a040 */
static int Geo_BoxPlaneSide( const vec3_t normal, const GeoBox_t *box, float dist )
{
    float radius;
    float d;

    radius = I_fabs( normal[0] ) * box->halfSize[0]
           + I_fabs( normal[1] ) * box->halfSize[1]
           + I_fabs( normal[2] ) * box->halfSize[2];

    Assertx( radius > 0.0f, "(radius > 0.0f)" );

    d = box->center[0] * normal[0]
      + box->center[1] * normal[1]
      + box->center[2] * normal[2];

    d = d - dist;

    if ( !( d < radius ) )
        return SIDE_FRONT;

    if ( -radius < d )
        return SIDE_CROSS;

    return SIDE_BACK;
}

/* Geo_ClassifyGroundTriangle  0x0040a100 */
void Geo_ClassifyGroundTriangle( GeoTriangle_t *tri )
{
    const float *v0;
    const float *v1;
    const float *v2;
    vec3_t       point;

    if ( tri->normal[2] < GEO_GROUND_MIN_NORMAL_Z )
    {
        tri->groundType = GEO_GROUND_NONE;
        return;
    }

    v0 = geoGlob.worldSpaceXyz[tri->indices[0]];
    v1 = geoGlob.worldSpaceXyz[tri->indices[1]];
    v2 = geoGlob.worldSpaceXyz[tri->indices[2]];

    point[0] = ( 0.0f + v0[0] + v1[0] + v2[0] ) * ( 1.0f / 3.0f );
    point[1] = ( 0.0f + v0[1] + v1[1] + v2[1] ) * ( 1.0f / 3.0f );
    point[2] = ( 0.0f + v0[2] + v1[2] + v2[2] ) * ( 1.0f / 3.0f );

    point[0] = point[0] - tri->normal[0] * GEO_GROUND_PROBE_OFFSET;
    point[1] = point[1] - tri->normal[1] * GEO_GROUND_PROBE_OFFSET;
    point[2] = point[2] - tri->normal[2] * GEO_GROUND_PROBE_OFFSET;

    tri->groundType = ( byte )( Geo_TraceSeesSkyDown( point ) ? GEO_GROUND_OVER_SKY
                                                              : GEO_GROUND_NONE );
}

/* Geo_Init  0x0040a200 */
void Geo_Init( void )
{
    int i;

    geoGlob.worldSpaceXyzCount = numBSPDrawVerts[0];
    geoTriCount = 0;

    geoGlob.firstBrushSide = new ( std::nothrow ) int[numBSPBrushes];

    if ( !geoGlob.firstBrushSide )
        Error( "Couldn't allocate %i bytes for first brush sides\n",
               numBSPBrushes * sizeof( int ) );

    geoGlob.firstBrushSide[0] = 0;

    for ( i = 1; i < numBSPBrushes; i++ )
        geoGlob.firstBrushSide[i] = geoGlob.firstBrushSide[i - 1]
                                  + ( short )bspBrushes[i - 1].numSides;

    Assertx( geoGlob.firstBrushSide[i - 1] + ( short )bspBrushes[i - 1].numSides
             == ( int )numBSPBrushSides,
             "geoGlob.firstBrushSide[brushIndex - 1] + dbrushes[brushIndex - 1]."
             "numSides == static_cast< int >( numbrushsides )" );

    for ( i = 0; i < options.threadCount; i++ )
    {
        Assertx( geoGlob.hunk[i].used == 0, "geoGlob.hunk[threadIndex].used == 0" );

        geoGlob.hunk[i].size = GEO_HUNK_SIZE;
        geoGlob.hunk[i].base = new ( std::nothrow ) byte[GEO_HUNK_SIZE];

        if ( !geoGlob.hunk[i].base )
            Error( "Couldn't allocate %i bytes for small hunk memory",
                   geoGlob.hunk[i].size );
    }

    ClearBounds( geoGlob.mins, geoGlob.maxs );
}

/* Geo_TransformModelVertex  0x00408950 */
static void Geo_TransformModelVertex( int vertIndex, const orientation_t *orient )
{
    Assertx( vertIndex < numBSPDrawVerts[0], "%i %i", vertIndex, numBSPDrawVerts[0] );
    Assertx( orient, "orient" );

    TransformPoint( orient, geoVertices[vertIndex].xyz,
                    geoGlob.worldSpaceXyz[vertIndex] );

    AddPointToBounds( geoGlob.worldSpaceXyz[vertIndex], geoGlob.mins, geoGlob.maxs );
}

/* Geo_AddModelTriangle  0x00408a70 */
static void Geo_AddModelTriangle( const BspTriSoup_t *surf, int modelIndex,
                                  const orientation_t *orient, MskMaterial_t *mskMtl,
                                  int triIndex )
{
    GeoTriangle_t        *tri;
    const unsigned short *indices;
    int                   i;

    Assertx( ( surf->primaryLightIndex >= 0
               && surf->primaryLightIndex < numBSPPrimaryLights )
             || surf->primaryLightIndex == 0,
             "((surf->primaryLightIndex >= 0 && surf->primaryLightIndex <"
             " numBSPPrimaryLights) || surf->primaryLightIndex == 0)" );

    tri = &geoTris[geoTriCount];

    tri->mskMtl            = mskMtl;
    tri->modelIndex        = ( short )modelIndex;
    tri->primaryLightIndex = surf->primaryLightIndex;
    tri->lightmapIndex     = surf->lightmapIndex;

    if ( surf->lightmapIndex != LIGHTMAP_NONE )
        Lighting_UseLightmap( surf->lightmapIndex );

    indices = &bspDrawIndices[0][surf->firstIndex + triIndex * 3];

    for ( i = 0; i < 3; i++ )
    {
        tri->indices[i] = indices[i] + surf->firstVertex;
        Geo_TransformModelVertex( tri->indices[i], orient );
    }

    if ( Geo_ComputeTriangleNormal( tri ) )
        geoTriCount++;
}

/* Geo_AddModel  0x00408b80 */
void Geo_AddModel( const BspTriSoup_t *surf, int modelIndex, const orientation_t *orient )
{
    MskMaterial_t *mskMtl;
    const char    *name;
    int            triCount;
    int            i;

    Assertx( modelIndex >= 0 && modelIndex < GEO_MAX_MODELS,
             "(modelIndex >= 0 && modelIndex < ARRAY_COUNT( geoGlob.modelOrientation ))" );

    geoGlob.modelOrientation[modelIndex] = *orient;

    name = bspMaterials[surf->materialIndex].name;

    if ( name[0] == '*' )
    {
        int index = 0;

        name++;

        while ( isdigit( ( unsigned char )*name ) )
            index = index * 10 + ( *name++ - '0' );

        name = bspMaterials[index].name;

        Assertx( name[0] != '*', "mtlName[0] != '*'" );
    }

    mskMtl = FindMaskMaterial( name, MTL_USAGE_WORLD_VCOL );

    triCount = ( short )surf->indexCount / 3;

    for ( i = 0; i < triCount; i++ )
        Geo_AddModelTriangle( surf, modelIndex, orient, mskMtl, i );
}

/* Geo_BuildCollisionData  0x0040dca0 */
void Geo_BuildCollisionData( void )
{
    int    startTime;
    vec3_t mins;
    vec3_t maxs;

    Print( "building collision data...\n" );

    startTime = ( int )timeGetTime();

    geoGlob.triRefCount = ( geoTriCount * 16 + numBSPNodes ) * 2;
    geoGlob.triRefs = new ( std::nothrow ) GeoTriangle_t *[geoGlob.triRefCount];

    if ( !geoGlob.triRefs )
        Error( "Couldn't allocate %i bytes for triangle refs\n",
               geoGlob.triRefCount * sizeof( GeoTriangle_t * ) );

    Assertx( GEO_MAX_NODES >= numBSPNodes, "(((32 * 1024) * 8) >= numnodes)" );

    geoGlob.bspNodes = ( GeoBspNode_t * )malloc( GEO_MAX_BSP_NODE_BYTES );

    if ( !geoGlob.bspNodes )
        Error( "Couldn't allocate %i bytes for internal bsp nodes\n",
               GEO_MAX_BSP_NODE_BYTES );

    geoGlob.bspLeafs = ( GeoBspLeaf_t * )malloc( GEO_MAX_BSP_LEAF_BYTES );

    if ( !geoGlob.bspLeafs )
        Error( "Couldn't allocate %i bytes for internal bsp leafs\n",
               GEO_MAX_BSP_LEAF_BYTES );

    Geo_CopyBspNodes();
    Geo_BuildBspTree();

    Print( "building collision took %.1f seconds\n",
           ( ( int )timeGetTime() - startTime ) * 0.001f );

    geoGlob.bspNodes = ( GeoBspNode_t * )realloc( geoGlob.bspNodes,
                                                  geoGlob.bspNodeCount * sizeof( GeoBspNode_t ) );
    geoGlob.bspLeafs = ( GeoBspLeaf_t * )realloc( geoGlob.bspLeafs,
                                                  geoGlob.bspLeafCount * sizeof( GeoBspLeaf_t ) );

    mins[0] = ( float )bspNodes[0].mins[0];
    mins[1] = ( float )bspNodes[0].mins[1];
    mins[2] = ( float )bspNodes[0].mins[2];
    maxs[0] = ( float )bspNodes[0].maxs[0];
    maxs[1] = ( float )bspNodes[0].maxs[1];
    maxs[2] = ( float )bspNodes[0].maxs[2];

    ModelCollision_BuildCellTree( mins, maxs );

    Assertx( options.supersampleAlphaCount >= 1
             && options.supersampleAlphaCount <= GEO_MAX_SUPERSAMPLE_ALPHA,
             "options.supersampleAlphaCount not in [1, ARRAY_COUNT("
             " geoGlob.supersampleAlphaOffset )]" );

    SpreadPointsInCircle( options.supersampleAlphaCount,
                          geoGlob.supersampleAlphaOffset, sizeof( vec2_t ) );

}

/* Geo_ForEachTriangleSample  0x0040b220 */
void Geo_ForEachTriangleSample( const GeoTriangle_t *tri, int subdivision,
                                Poly2dGridCallback_t func, void *userData )
{
    poly2dBuf_t scratch[4];
    vec3_t      xyz[3];
    vec2_t      mins;
    vec2_t      maxs;
    float       step;
    float       invStep;
    int         firstS;
    int         firstT;
    int         countS;
    int         countT;
    int         i;

    ClearBounds2D( mins, maxs );

    for ( i = 0; i < 3; i++ )
    {
        const GeoVertex_t *vert = &geoVertices[tri->indices[i]];

        scratch[0][i][0] = vert->lmapCoord[0] * LMAP_WIDTH_MIN;
        scratch[0][i][1] = vert->lmapCoord[1] * LMAP_WIDTH_MIN;

        AddPointToBounds2D( scratch[0][i], mins, maxs );

        Vec3Copy( geoGlob.worldSpaceXyz[tri->indices[i]], xyz[i] );
    }

    Assertx( mins[0] >= 0.0f && maxs[0] <= LMAP_WIDTH_MIN,
             "%s", va( "lightmap s out of range (%g to %g)", mins[0], maxs[0] ) );

    Assertx( mins[1] >= 0.0f && maxs[1] <= LMAP_HEIGHT_MIN,
             "%s", va( "lightmap t out of range (%g to %g)", mins[1], maxs[1] ) );

    subdivision = subdivision * 2;
    step        = ( float )subdivision;

    firstS = ( int )( float )floor( mins[0] * step );
    firstT = ( int )( float )floor( mins[1] * step );
    countS = ( int )( float )ceil( maxs[0] * step ) - firstS;
    countT = ( int )( float )ceil( maxs[1] * step ) - firstT;

    Assertx( firstS >= 0 && firstS < subdivision * 1024,
             "%s", va( "first sample s out of range (%i)", firstS ) );
    Assertx( firstT >= 0 && firstT < subdivision * 1024,
             "%s", va( "first sample t out of range (%i)", firstT ) );
    Assertx( firstS + countS >= 0 && firstS + countS <= subdivision * 1024,
             "%s", va( "sample s range (%i, %i)", firstS, countS ) );
    Assertx( firstT + countT >= 0 && firstT + countT <= subdivision * 1024,
             "%s", va( "sample t range (%i, %i)", firstT, countT ) );

    invStep = 1.0f / step;

    Poly2dSubdivideGrid( scratch, 3, countS, countT,
                         firstS * invStep, firstT * invStep, invStep, invStep,
                         func, userData );
}

/* Geo_ClampedMin  0x0040b570 */
static float Geo_ClampedMin( float a, float b )
{
    float smallest = b - a > 0.0f ? a : b;

    return -smallest < 0.0f ? smallest : 0.0f;
}

/* Geo_AddSampleArea  0x0040b5d0 */
static void Geo_AddSampleArea( float area, const vec2_t center, const vec2_t *coords,
                               int vertCount, void *userData, int cellIndex )
{
    const GeoTriangle_t *tri = ( const GeoTriangle_t * )userData;
    LmapSubSample_t      subSample;

    Lighting_SetSubSample( tri->lightmapIndex,
                           ( float )( center[0] * 2.0 ),
                           ( float )( center[1] * 2.0 ),
                           &subSample );

    Lock( subSample.sample );

    subSample.sample->areaX2 += area;
    subSample.sample->subWeight[subSample.fracS + subSample.fracT * 2] += area;

    Unlock( subSample.sample );
}

/* Geo_CalcTriangleSampleAreas  0x0040c190 */
static void Geo_CalcTriangleSampleAreas( int triIndex, int threadIndex )
{
    GeoTriangle_t *tri = &geoTris[triIndex];

    if ( tri->lightmapIndex == LIGHTMAP_NONE )
        return;

    Geo_ForEachTriangleSample( tri, 1, Geo_AddSampleArea, tri );
}

/* Geo_CalcSampleAreas  0x0040c6c0 */
void Geo_CalcSampleAreas( int threads )
{
    RunThreadsOn( geoTriCount, Geo_CalcTriangleSampleAreas, threads );
}

#define RADIOSITY_MAP_AXES  2

typedef struct
{
    const GeoTriangle_t *tri;                               /* +0x00 */
    float                coeff[RADIOSITY_MAP_AXES][3];      /* +0x04 */
} RadiosityColorMap_t;

typedef struct
{
    float                totalArea;     /* +0x00 */
    vec3_t               color;         /* +0x04 */
    const MskMaterial_t *mskMtl;        /* +0x10 */
} ColorMaskAccum_t;

/* Geo_AccumulateColorMask  0x0040b650 */
static void Geo_AccumulateColorMask( float area, const vec2_t center, const vec2_t *coords,
                                     int vertCount, void *userData, int cellIndex )
{
    ColorMaskAccum_t    *accum = ( ColorMaskAccum_t * )userData;
    const MskMaterial_t *mskMtl = accum->mskMtl;
    int                  x;
    int                  y;
    int                  index;

    Assert( mskMtl->colorMask );

    y = ( int )center[1] & ( mskMtl->maskHeight - 1 );
    x = ( int )center[0] & ( mskMtl->maskWidth - 1 );

    index = ( y / 4 * ( mskMtl->maskWidth / 4 ) + x / 4 ) * 3;

    accum->color[0] += Lighting_GammaToLinear( mskMtl->colorMask[index + 0] * ( 1.0f / 255.0f ) ) * area;
    accum->color[1] += Lighting_GammaToLinear( mskMtl->colorMask[index + 1] * ( 1.0f / 255.0f ) ) * area;
    accum->color[2] += Lighting_GammaToLinear( mskMtl->colorMask[index + 2] * ( 1.0f / 255.0f ) ) * area;

    accum->totalArea += area;
}

/* Geo_AverageCellColor  0x0040b770 */
static void Geo_AverageCellColor( const vec2_t *coords, const RadiosityColorMap_t *map,
                                  int vertCount, vec3_t out )
{
    poly2dBuf_t      scratch[4];
    ColorMaskAccum_t accum;
    vec2_t           mins;
    vec2_t           maxs;
    float            startX;
    float            startY;
    float            stepX;
    float            stepY;
    int              countX;
    int              countY;
    int              i;

    ClearBounds2D( mins, maxs );

    for ( i = 0; i < vertCount; i++ )
    {
        float s = coords[i][0] * map->coeff[0][0] + coords[i][1] * map->coeff[0][1];
        float t = coords[i][0] * map->coeff[1][0] + coords[i][1] * map->coeff[1][1];

        scratch[0][i][0] = s + map->coeff[0][2];
        scratch[0][i][1] = t + map->coeff[1][2];

        AddPointToBounds2D( scratch[0][i], mins, maxs );
    }

    startX = ( float )floor( mins[0] );
    stepX  = ( float )ceil( ( float )( ( maxs[0] - startX ) * 0.25f ) );
    stepX  = 1.0f - stepX < 0.0f ? stepX : 1.0f;
    countX = ( int )( float )ceil( ( float )( ( maxs[0] - startX ) / stepX ) );

    startY = ( float )floor( mins[1] );
    stepY  = ( float )ceil( ( float )( ( maxs[1] - startY ) * 0.25f ) );
    stepY  = 1.0f - stepY < 0.0f ? stepY : 1.0f;
    countY = ( int )( float )ceil( ( float )( ( maxs[1] - startY ) / stepY ) );

    accum.totalArea = 0.0f;
    accum.color[0]  = 0.0f;
    accum.color[1]  = 0.0f;
    accum.color[2]  = 0.0f;
    accum.mskMtl    = map->tri->mskMtl;

    Poly2dSubdivideGrid( scratch, vertCount, countX, countY,
                         startX, startY, stepX, stepY,
                         Geo_AccumulateColorMask, &accum );

    if ( accum.totalArea > 0.0f )
    {
        float scale = 1.0f / accum.totalArea;

        out[0] = accum.color[0] * scale;
        out[1] = accum.color[1] * scale;
        out[2] = accum.color[2] * scale;
    }
    else
    {
        out[0] = 1.0f;
        out[1] = 1.0f;
        out[2] = 1.0f;
    }
}

/* Geo_AddRadiosityColor  0x0040b9b0 */
static void Geo_AddRadiosityColor( float area, const vec2_t center, const vec2_t *coords,
                                   int vertCount, void *userData, int cellIndex )
{
    const RadiosityColorMap_t *map = ( const RadiosityColorMap_t * )userData;
    LmapDef_t                 *sample;
    vec3_t                     color;
    float                      weight;

    sample = Lighting_Sample( map->tri->lightmapIndex,
                              ( int )( float )floor( center[0] ),
                              ( int )( float )floor( center[1] ) );

    Assert( sample->vars );
    Assertx( sample->areaX2 > 0.0f, "%s\n\t(sample->areaX2) = %g",
             "(sample->areaX2 > 0.0f)", sample->areaX2 );

    weight = Geo_ClampedMin( area, sample->areaX2 ) / sample->areaX2;

    Geo_AverageCellColor( coords, map, vertCount, color );

    Lock( sample );

    sample->vars->reflectance[0] += color[0] * weight;
    sample->vars->reflectance[1] += color[1] * weight;
    sample->vars->reflectance[2] += color[2] * weight;

    Unlock( sample );
}

/* Geo_CalcTriangleRadiosityColor  0x0040c1c0 */
static void Geo_CalcTriangleRadiosityColor( int triIndex, int threadIndex )
{
    GeoTriangle_t       *tri = &geoTris[triIndex];
    const MskMaterial_t *mskMtl;
    RadiosityColorMap_t  map;
    LinearMapping_t      mapping;
    vec3_t               base;
    vec3_t               lmap[3];
    vec2_t               tex[3];
    vec4_t               solved;
    int                  i;

    if ( tri->lightmapIndex == LIGHTMAP_NONE )
        return;

    mskMtl = tri->mskMtl;

    if ( !mskMtl->colorMask )
        return;

    base[0] = 0.0f;
    base[1] = 0.0f;
    base[2] = 1.0f;

    for ( i = 0; i < 3; i++ )
    {
        const GeoVertex_t *vert = &geoVertices[tri->indices[i]];

        lmap[i][0] = vert->lmapCoord[0] * LMAP_WIDTH_MIN;
        lmap[i][1] = vert->lmapCoord[1] * LMAP_WIDTH_MIN;
        lmap[i][2] = 0.0f;

        tex[i][0] = vert->texCoord[0] * mskMtl->maskWidth;
        tex[i][1] = vert->texCoord[1] * mskMtl->maskHeight;
    }

    if ( !LinearMapping_Solve( base, lmap[0], lmap[1], lmap[2], &mapping ) )
        return;

    map.tri = tri;

    for ( i = 0; i < RADIOSITY_MAP_AXES; i++ )
    {
        LinearMapping_Apply( &mapping, tex[0][i], tex[1][i], tex[2][i], solved );

        map.coeff[i][0] = solved[0];
        map.coeff[i][1] = solved[1];
        map.coeff[i][2] = solved[3];
    }

    Geo_ForEachTriangleSample( tri, 1, Geo_AddRadiosityColor, &map );
}

/* Geo_CalcRadiosityColors  0x0040c6e0 */
void Geo_CalcRadiosityColors( int threads )
{
    RunThreadsOn( geoTriCount, Geo_CalcTriangleRadiosityColor, threads );
}

/* Geo_TriangleLmapDegenerate  0x004089d0 */
static qboolean Geo_TriangleLmapDegenerate( const GeoTriangle_t *tri )
{
    const float *lmap0 = geoVertices[tri->indices[0]].lmapCoord;
    const float *lmap1 = geoVertices[tri->indices[1]].lmapCoord;
    const float *lmap2 = geoVertices[tri->indices[2]].lmapCoord;

    if ( lmap0[0] == lmap1[0] && lmap0[1] == lmap1[1] )
        return qtrue;

    if ( lmap1[0] == lmap2[0] && lmap1[1] == lmap2[1] )
        return qtrue;

    if ( lmap2[0] == lmap0[0] && lmap2[1] == lmap0[1] )
        return qtrue;

    return qfalse;
}

/* Geo_OrthonormalizeBasis  0x0040bae0 */
static void Geo_OrthonormalizeBasis( vec3_t *basis )
{
    float project;

    Vec3Normalize( basis[0] );

    project = -Vec3Dot( basis[1], basis[0] );
    Vec3Mad( basis[1], project, basis[0], basis[1] );
    Vec3Normalize( basis[1] );

    project = -Vec3Dot( basis[2], basis[0] );
    Vec3Mad( basis[2], project, basis[0], basis[2] );

    project = -Vec3Dot( basis[2], basis[1] );
    Vec3Mad( basis[2], project, basis[1], basis[2] );
    Vec3Normalize( basis[2] );
}

/* Geo_EvalLmapPlane  0x0040bbe0 */
static void Geo_EvalLmapPlane( const LmapPlane_t *plane, const vec2_t coord, vec3_t out )
{
    float s = coord[0];
    float t = coord[1];

    out[0] = plane->base[0] + plane->ds[0] * s + plane->dt[0] * t;
    out[1] = plane->base[1] + plane->ds[1] * s + plane->dt[1] * t;
    out[2] = plane->base[2] + plane->ds[2] * s + plane->dt[2] * t;
}

/* Geo_RemoveSampleArea  0x0040bc40 */
static void Geo_RemoveSampleArea( const LmapSubSample_t *subSample, float area, float subArea )
{
    Lock( subSample->sample );

    subSample->sample->areaX2 -= area;
    subSample->sample->subWeight[subSample->fracS + subSample->fracT * 2] -= subArea;

    Unlock( subSample->sample );
}

/* Geo_AddTransport  0x0040bc80 */
static void Geo_AddTransport( float areaX2, const vec2_t centroid, const vec2_t *coords,
                              int vertCount, void *userData, int cellIndex )
{
    const TransportTri_t *transport = ( const TransportTri_t * )userData;
    LmapSubSample_t       subSample;
    vec3_t                pos;
    vec3_t                basis[3];
    vec3_t                axis0;
    vec3_t                axis1;
    float                 area;
    float                 subArea;
    float                 radius;
    int                   lightType;

    Assertx( areaX2 > 0, "%s\n\t(areaX2) = %g", "(areaX2 > 0)", areaX2 );
    Assertx( userData, "%s", "userData" );

    Assertx( !IS_NAN( centroid[0] ), "%s", "!IS_NAN(centroid[0])" );
    Assertx( !IS_NAN( centroid[1] ), "%s", "!IS_NAN(centroid[1])" );

    Lighting_SetSubSample( transport->lightmapIndex,
                           ( float )( centroid[0] * 2.0 ),
                           ( float )( centroid[1] * 2.0 ),
                           &subSample );

    subArea = Geo_ClampedMin( areaX2,
                              subSample.sample->subWeight[subSample.fracS + subSample.fracT * 2] );

    if ( subArea == 0.0f )
        return;

    area = Geo_ClampedMin( areaX2, subSample.sample->areaX2 );

    Geo_EvalLmapPlane( &transport->xyz,      centroid, pos );
    Geo_EvalLmapPlane( &transport->tangent,  centroid, basis[0] );
    Geo_EvalLmapPlane( &transport->binormal, centroid, basis[1] );
    Geo_EvalLmapPlane( &transport->normal,   centroid, basis[2] );

    Assertx( !IS_NAN( basis[0][0] ) && !IS_NAN( basis[0][1] ) && !IS_NAN( basis[0][2] ),
             "%s", "!IS_NAN((basis[0])[0]) && !IS_NAN((basis[0])[1]) && !IS_NAN((basis[0])[2])" );
    Assertx( !IS_NAN( basis[1][0] ) && !IS_NAN( basis[1][1] ) && !IS_NAN( basis[1][2] ),
             "%s", "!IS_NAN((basis[1])[0]) && !IS_NAN((basis[1])[1]) && !IS_NAN((basis[1])[2])" );
    Assertx( !IS_NAN( basis[2][0] ) && !IS_NAN( basis[2][1] ) && !IS_NAN( basis[2][2] ),
             "%s", "!IS_NAN((basis[2])[0]) && !IS_NAN((basis[2])[1]) && !IS_NAN((basis[2])[2])" );

    Geo_OrthonormalizeBasis( basis );

    Assertx( !IS_NAN( basis[0][0] ) && !IS_NAN( basis[0][1] ) && !IS_NAN( basis[0][2] ),
             "%s", "!IS_NAN((basis[0])[0]) && !IS_NAN((basis[0])[1]) && !IS_NAN((basis[0])[2])" );
    Assertx( !IS_NAN( basis[1][0] ) && !IS_NAN( basis[1][1] ) && !IS_NAN( basis[1][2] ),
             "%s", "!IS_NAN((basis[1])[0]) && !IS_NAN((basis[1])[1]) && !IS_NAN((basis[1])[2])" );
    Assertx( !IS_NAN( basis[2][0] ) && !IS_NAN( basis[2][1] ) && !IS_NAN( basis[2][2] ),
             "%s", "!IS_NAN((basis[2])[0]) && !IS_NAN((basis[2])[1]) && !IS_NAN((basis[2])[2])" );

    radius = ( float )sqrt( ( float )( subArea * TRANSPORT_CELL_AREA_SCALE ) ) * TRANSPORT_CELL_RADIUS;

    Vec3Scale( transport->xyz.ds, radius, axis0 );
    Vec3Scale( transport->xyz.dt, radius, axis1 );

    lightType = geoTris[transport->triIndex].primaryLightIndex;

    if ( options.relight )
    {
        if ( Lighting_IsSuppressed( transport->triIndex, cellIndex ) )
        {
            Geo_RemoveSampleArea( &subSample, area, subArea );
            return;
        }

        Compile_TraceSubSample( transport->threadIndex, lightType, pos, axis0, axis1,
                                basis, area, subArea, &subSample );
        return;
    }

    if ( !Compile_TraceSubSample( transport->threadIndex, lightType, pos, axis0, axis1,
                                  basis, area, subArea, &subSample ) )
    {
        Geo_RemoveSampleArea( &subSample, area, subArea );
        Lighting_Suppress( transport->threadIndex, transport->triIndex, cellIndex );
    }
}

/* Geo_BuildTriangleTransport  0x0040c350 */
static qboolean Geo_BuildTriangleTransport( const GeoTriangle_t *tri, TransportTri_t *transport )
{
    LinearMapping_t  mapping;
    const float     *xyz[3];
    vec3_t           lmap[3];
    vec3_t           tangent[3];
    vec3_t           binormal[3];
    vec3_t           normal[3];
    vec3_t           base;
    vec4_t           solved;
    int              i;

    base[0] = 0.0f;
    base[1] = 0.0f;
    base[2] = 1.0f;

    for ( i = 0; i < 3; i++ )
    {
        const GeoVertex_t   *vert   = &geoVertices[tri->indices[i]];
        const orientation_t *orient = &geoGlob.modelOrientation[tri->modelIndex];

        xyz[i] = geoGlob.worldSpaceXyz[tri->indices[i]];

        lmap[i][0] = vert->lmapCoord[0] * LMAP_WIDTH_MIN;
        lmap[i][1] = vert->lmapCoord[1] * LMAP_WIDTH_MIN;
        lmap[i][2] = 0.0f;

        TransformDir( orient, vert->tangent,  tangent[i] );
        TransformDir( orient, vert->binormal, binormal[i] );
        TransformDir( orient, vert->normal,   normal[i] );
    }

    if ( !LinearMapping_Solve( base, lmap[0], lmap[1], lmap[2], &mapping ) )
        return qfalse;

    for ( i = 0; i < 3; i++ )
    {
        LinearMapping_Apply( &mapping, xyz[0][i], xyz[1][i], xyz[2][i], solved );

        transport->xyz.base[i] = solved[3];
        transport->xyz.ds[i]   = solved[0];
        transport->xyz.dt[i]   = solved[1];

        LinearMapping_Apply( &mapping, tangent[0][i], tangent[1][i], tangent[2][i], solved );

        transport->tangent.base[i] = solved[3];
        transport->tangent.ds[i]   = solved[0];
        transport->tangent.dt[i]   = solved[1];

        LinearMapping_Apply( &mapping, binormal[0][i], binormal[1][i], binormal[2][i], solved );

        transport->binormal.base[i] = solved[3];
        transport->binormal.ds[i]   = solved[0];
        transport->binormal.dt[i]   = solved[1];

        LinearMapping_Apply( &mapping, normal[0][i], normal[1][i], normal[2][i], solved );

        transport->normal.base[i] = solved[3];
        transport->normal.ds[i]   = solved[0];
        transport->normal.dt[i]   = solved[1];
    }

    return qtrue;
}

/* Geo_ForEachTransportSample  0x0040c610 */
static void Geo_ForEachTransportSample( int triIndex, int threadIndex, int subdivision,
                                        Poly2dGridCallback_t func )
{
    GeoTriangle_t *tri = &geoTris[triIndex];
    TransportTri_t transport;

    if ( tri->lightmapIndex == LIGHTMAP_NONE )
        return;

    if ( Geo_TriangleLmapDegenerate( tri ) )
        return;

    if ( !Geo_BuildTriangleTransport( tri, &transport ) )
        return;

    transport.threadIndex   = threadIndex;
    transport.triIndex      = triIndex;
    transport.lightmapIndex = tri->lightmapIndex;

    Geo_ForEachTriangleSample( tri, subdivision, func, &transport );
}

/* Geo_TransportTriangle  0x0040c690 */
static void Geo_TransportTriangle( int triIndex, int threadIndex )
{
    Geo_ForEachTransportSample( triIndex, threadIndex, options.supersampleCount,
                                Geo_AddTransport );
}

/* Geo_BuildTransport  0x0040c700 */
void Geo_BuildTransport( int threads )
{
    RunThreadsOn( geoTriCount, Geo_TransportTriangle, threads );
}

/* Geo_SampleTriangle  0x0040c720 */
static void Geo_SampleTriangle( int triIndex, int threadIndex )
{
    GeoTriangle_t *tri = &geoTris[triIndex];

    if ( tri->lightmapIndex == LIGHTMAP_NONE )
        return;

    Geo_ForEachTriangleSample( tri, geoGlob.sampleSubdivision, geoGlob.sampleFunc,
                               &tri->lightmapIndex );
}

/* Geo_ForEachSample  0x0040c760 */
void Geo_ForEachSample( Poly2dGridCallback_t func, int subdivision, int threads )
{
    geoGlob.sampleFunc        = func;
    geoGlob.sampleSubdivision = subdivision;

    RunThreadsOn( geoTriCount, Geo_SampleTriangle, threads );
}

/* Geo_SpansSlab  0x0040c790 */
static qboolean Geo_SpansSlab( float d0, float d1, float d2, float dist )
{
    if ( dist < d0 )
        return d1 < dist || d2 < dist;

    dist = -dist;

    if ( dist <= d0 )
        return qtrue;

    return d1 > dist || d2 > dist;
}

static qboolean Geo_EdgeSeparates( float p0, float p1, float rad )
{
    return I_fmin( p0, p1 ) > rad || I_fmax( p0, p1 ) < -rad;
}

/* DoesTriangleIntersectBox  0x0040c800 */
static qboolean DoesTriangleIntersectBox( const GeoTriangle_t *tri, const GeoBox_t *aabb )
{
    vec3_t v0;
    vec3_t v1;
    vec3_t v2;
    vec3_t e0;
    vec3_t e1;
    vec3_t e2;
    float  fex0, fey0, fez0;
    float  fex1, fey1, fez1;
    float  fex2, fey2, fez2;
    float  dot;
    float  rad;
    int    i;

    for ( i = 0; i < 3; i++ )
    {
        v0[i] = geoGlob.worldSpaceXyz[tri->indices[0]][i] - aabb->center[i];
        v1[i] = geoGlob.worldSpaceXyz[tri->indices[1]][i] - aabb->center[i];
        v2[i] = geoGlob.worldSpaceXyz[tri->indices[2]][i] - aabb->center[i];

        if ( !Geo_SpansSlab( v0[i], v1[i], v2[i], aabb->halfSize[i] ) )
            return qfalse;
    }

    e0[0] = v1[0] - v0[0];
    e0[1] = v1[1] - v0[1];
    e0[2] = v1[2] - v0[2];

    dot = tri->normal[0] * v0[0] + tri->normal[1] * v0[1] + tri->normal[2] * v0[2];

    rad = I_fabs( tri->normal[0] ) * aabb->halfSize[0]
        + I_fabs( tri->normal[1] ) * aabb->halfSize[1]
        + I_fabs( tri->normal[2] ) * aabb->halfSize[2];

    if ( I_fabs( dot ) > rad )
        return qfalse;

    e1[0] = v2[0] - v1[0];
    e1[1] = v2[1] - v1[1];
    e1[2] = v2[2] - v1[2];

    e2[0] = v0[0] - v2[0];
    e2[1] = v0[1] - v2[1];
    e2[2] = v0[2] - v2[2];

    fey0 = I_fabs( e0[1] );
    fez0 = I_fabs( e0[2] );

    if ( Geo_EdgeSeparates( v0[2] * v1[1] - v0[1] * v1[2],
                            e0[1] * v2[2] - e0[2] * v2[1],
                            fey0 * aabb->halfSize[2] + fez0 * aabb->halfSize[1] ) )
        return qfalse;

    fey1 = I_fabs( e1[1] );
    fez1 = I_fabs( e1[2] );

    if ( Geo_EdgeSeparates( v1[2] * v2[1] - v1[1] * v2[2],
                            e1[1] * v0[2] - e1[2] * v0[1],
                            fey1 * aabb->halfSize[2] + fez1 * aabb->halfSize[1] ) )
        return qfalse;

    fey2 = I_fabs( e2[1] );
    fez2 = I_fabs( e2[2] );

    if ( Geo_EdgeSeparates( v2[2] * v0[1] - v2[1] * v0[2],
                            e2[1] * v1[2] - e2[2] * v1[1],
                            fey2 * aabb->halfSize[2] + fez2 * aabb->halfSize[1] ) )
        return qfalse;

    fex0 = I_fabs( e0[0] );

    if ( Geo_EdgeSeparates( v0[0] * v1[2] - v1[0] * v0[2],
                            v2[0] * e0[2] - e0[0] * v2[2],
                            fex0 * aabb->halfSize[2] + fez0 * aabb->halfSize[0] ) )
        return qfalse;

    fex1 = I_fabs( e1[0] );

    if ( Geo_EdgeSeparates( v1[0] * v2[2] - v2[0] * v1[2],
                            v0[0] * e1[2] - e1[0] * v0[2],
                            fex1 * aabb->halfSize[2] + fez1 * aabb->halfSize[0] ) )
        return qfalse;

    fex2 = I_fabs( e2[0] );

    if ( Geo_EdgeSeparates( v2[0] * v0[2] - v0[0] * v2[2],
                            v1[0] * e2[2] - e2[0] * v1[2],
                            fex2 * aabb->halfSize[2] + fez2 * aabb->halfSize[0] ) )
        return qfalse;

    if ( Geo_EdgeSeparates( v1[0] * v0[1] - v0[0] * v1[1],
                            e0[0] * v2[1] - v2[0] * e0[1],
                            fex0 * aabb->halfSize[1] + fey0 * aabb->halfSize[0] ) )
        return qfalse;

    if ( Geo_EdgeSeparates( v2[0] * v1[1] - v1[0] * v2[1],
                            e1[0] * v0[1] - v0[0] * e1[1],
                            fex1 * aabb->halfSize[1] + fey1 * aabb->halfSize[0] ) )
        return qfalse;

    if ( Geo_EdgeSeparates( v0[0] * v2[1] - v2[0] * v0[1],
                            e2[0] * v1[1] - v1[0] * e2[1],
                            fex2 * aabb->halfSize[1] + fey2 * aabb->halfSize[0] ) )
        return qfalse;

    return qtrue;
}

/* Geo_AddUniqueTri  0x0040ced0 */
static int Geo_AddUniqueTri( GeoTriangle_t **list, int count, GeoTriangle_t *tri )
{
    int i;

    for ( i = 0; i < count; i++ )
    {
        if ( list[i] == tri )
            return count;
    }

    list[count] = tri;

    return count + 1;
}

/* Geo_AddTrianglesInBox  0x0040cef0 */
static int Geo_AddTrianglesInBox( GeoTriangle_t *const *triRefs, int triCount,
                                  GeoTriangle_t **out, int outCount, int maxOut,
                                  const GeoBox_t *aabb )
{
    int i;

    for ( i = 0; i < triCount; i++ )
    {
        if ( outCount >= maxOut )
            break;

        if ( !DoesTriangleIntersectBox( triRefs[i], aabb ) )
            continue;

        outCount = Geo_AddUniqueTri( out, outCount, triRefs[i] );
    }

    return outCount;
}

/* Geo_TrianglesInBox_r  0x0040cf50 */
static int Geo_TrianglesInBox_r( int child, const GeoBox_t *aabb, GeoTriangle_t **out,
                                 int outCount, int maxOut )
{
    while ( child >= 0 )
    {
        const GeoBspNode_t *node = &geoGlob.bspNodes[child];
        int                 side = Geo_BoxPlaneSide( node->plane.normal, aabb,
                                                     node->plane.dist );

        if ( side == SIDE_FRONT )
        {
            child = node->children[0];
            continue;
        }

        if ( side == SIDE_CROSS )
            outCount = Geo_TrianglesInBox_r( node->children[0], aabb, out, outCount, maxOut );

        child = node->children[1];
    }

    if ( child == GEO_CHILD_EMPTY )
        return outCount;

    {
        const GeoBspLeaf_t *leaf = &geoGlob.bspLeafs[GEO_CHILD_TO_LEAF( child )];

        return Geo_AddTrianglesInBox( &geoGlob.triRefs[leaf->firstTriRef], leaf->triCount,
                                      out, outCount, maxOut, aabb );
    }
}

/* Geo_TrianglesInBox  0x0040cff0 */
static int Geo_TrianglesInBox( const GeoBox_t *aabb, GeoTriangle_t **out, int maxOut )
{
    return Geo_TrianglesInBox_r( 0, aabb, out, 0, maxOut );
}

/* Geo_AllocCollisionNode  0x0040d000 */
static GeoCollisionNode_t *Geo_AllocCollisionNode( GeoHunk_t *hunk,
                                                   GeoCollisionNode_t *parent,
                                                   GeoTriangle_t *const *tris, int triCount )
{
    GeoCollisionNode_t *node;
    int                 size = GEO_COLLISION_NODE_SIZE( triCount );

    node = ( GeoCollisionNode_t * )Geo_HunkAlloc( hunk, size );

    if ( !node )
        Error( "Out of memory" );

    node->links       = NULL;
    node->visitMark   = 0;
    node->parent      = parent;
    node->children[0] = NULL;
    node->children[1] = NULL;
    node->triCount    = triCount;

    if ( triCount )
        memcpy( node->tris, tris, triCount * sizeof( GeoTriangle_t * ) );

    return node;
}

/* Geo_BuildCollisionNode  0x0040d080 */
static GeoCollisionNode_t *Geo_BuildCollisionNode( GeoCollisionNode_t *parent,
                                                   GeoTriangle_t **triList, int triCount,
                                                   GeoHunk_t *hunk )
{
    GeoCollisionNode_t *node;
    GeoPlane_t          bestPlane;
    GeoTriangle_t      *tri;
    int                 bestTriIndex = -1;
    int                 bestCost     = 0x7fffffff;
    int                 onCount;
    int                 i;

    if ( !triCount )
        return Geo_AllocCollisionNode( hunk, parent, NULL, 0 );

    for ( i = 0; i < triCount; i++ )
    {
        GeoPlane_t plane;
        int        sideCount[4];
        int        cost;
        int        j;

        Geo_TrianglePlane( triList[i], &plane );

        sideCount[SIDE_FRONT] = 0;
        sideCount[SIDE_BACK]  = 0;
        sideCount[SIDE_ON]    = 0;
        sideCount[SIDE_CROSS] = 0;

        for ( j = 0; j < triCount; j++ )
            sideCount[Geo_TriangleSide( triList[j], &plane, GEO_COLLISION_EPSILON )]++;

        cost = abs( sideCount[SIDE_FRONT] - sideCount[SIDE_BACK] )
             + ( sideCount[SIDE_ON] + sideCount[SIDE_CROSS] ) * 2;

        if ( bestCost > cost )
        {
            bestCost     = cost;
            bestTriIndex = i;
            bestPlane    = plane;
        }
    }

    Assertx( bestTriIndex >= 0, "%s", "bestTriIndex >= 0" );

    tri                    = triList[bestTriIndex];
    triList[bestTriIndex]  = triList[0];
    triList[0]             = tri;

    onCount = Geo_PartitionBySide( &triList[1], triCount - 1, &bestPlane,
                                   GEO_COLLISION_EPSILON, SIDE_ON ) + 1;

    node = Geo_AllocCollisionNode( hunk, parent, triList, onCount );

    triCount -= onCount;
    triList  += onCount;

    for ( i = 0; i < 2; i++ )
    {
        int sideCount = Geo_PartitionBySide( triList, triCount, &bestPlane,
                                             GEO_COLLISION_EPSILON, i );

        node->children[i] = Geo_BuildCollisionNode( node, &triList[sideCount],
                                                    triCount - sideCount, hunk );
    }

    return node;
}

/* Geo_LeafLinked  0x0040d290 */
static qboolean Geo_LeafLinked( const GeoCollisionNode_t *node,
                                const GeoCollisionNode_t *other )
{
    const GeoLeafLink_t *link;

    for ( link = node->links; link; link = link->next )
    {
        if ( link->node == other )
            return qtrue;
    }

    return qfalse;
}

/* Geo_AddLeafLink  0x0040d2b0 */
static void Geo_AddLeafLink( GeoHunk_t *hunk, GeoCollisionNode_t *node,
                             GeoCollisionNode_t *other )
{
    GeoLeafLink_t *link = ( GeoLeafLink_t * )Geo_HunkAlloc( hunk, sizeof( GeoLeafLink_t ) );

    link->node  = other;
    link->next  = node->links;
    node->links = link;
}

/* Geo_ConnectLeaves  0x0040d2f0 */
static void Geo_ConnectLeaves( GeoCollisionNode_t *node0, GeoCollisionNode_t *node1,
                               winding_t *w, GeoHunk_t *hunk )
{
    Assertx( node0, "%s", "node0" );
    Assertx( node1, "%s", "node1" );
    Assertx( w, "%s", "w" );

    for ( ;; )
    {
        while ( node0->triCount )
        {
            GeoPlane_t plane;
            winding_t *front;
            winding_t *back;

            Geo_TrianglePlane( node0->tris[0], &plane );

            ClipWindingEpsilon( w, plane.normal, plane.dist, GEO_COLLISION_EPSILON,
                                &front, &back, qfalse );
            FreeWinding( w );

            if ( front )
            {
                if ( back )
                    Geo_ConnectLeaves( node0->children[0], node1, back, hunk );

                w     = front;
                node0 = node0->children[1];
            }
            else
            {
                w     = back;
                node0 = node0->children[0];
            }

            Assertx( node0, "%s", "node0" );
            Assertx( w, "%s", "w" );
        }

        if ( !node1->triCount )
            break;

        {
            GeoCollisionNode_t *swap = node0;

            node0 = node1;
            node1 = swap;
        }
    }

    FreeWinding( w );

    if ( Geo_LeafLinked( node0, node1 ) )
        return;

    Geo_AddLeafLink( hunk, node0, node1 );
    Geo_AddLeafLink( hunk, node1, node0 );
}

/* Geo_TriangleSidePlane  0x0040d510 */
static void Geo_TriangleSidePlane( const GeoTriangle_t *tri, int ptIndex, int ptIndexPrev,
                                   GeoPlane_t *sidePlane )
{
    const float *xyz     = geoGlob.worldSpaceXyz[tri->indices[ptIndex]];
    const float *xyzPrev = geoGlob.worldSpaceXyz[tri->indices[ptIndexPrev]];
    vec3_t       dir;

    dir[0] = xyz[0] - xyzPrev[0];
    dir[1] = xyz[1] - xyzPrev[1];
    dir[2] = xyz[2] - xyzPrev[2];

    Vec3Cross( tri->normal, dir, sidePlane->normal );
    Vec3Normalize( sidePlane->normal );

    sidePlane->dist = Vec3Dot( xyzPrev, sidePlane->normal );
}

/* Geo_ClipWindingByNodeTris  0x0040d5a0 */
static void Geo_ClipWindingByNodeTris( GeoCollisionNode_t *node, int triIndex,
                                       winding_t *w, GeoHunk_t *hunk )
{
    const GeoTriangle_t *tri;
    int                  ptIndex;
    int                  ptIndexPrev;

    if ( triIndex == node->triCount )
    {
        Geo_ConnectLeaves( node->children[0], node->children[1], w, hunk );
        return;
    }

    tri         = node->tris[triIndex];
    ptIndexPrev = 2;

    for ( ptIndex = 0; ptIndex < 3 && w; ptIndex++ )
    {
        GeoPlane_t sidePlane;
        winding_t *front;
        winding_t *back;
        float      opposite;

        Geo_TriangleSidePlane( tri, ptIndex, ptIndexPrev, &sidePlane );

        opposite = Vec3Dot( geoGlob.worldSpaceXyz[tri->indices[3 - ptIndexPrev - ptIndex]],
                            sidePlane.normal );

        Assertx( opposite - sidePlane.dist <= GEO_EQUAL_EPSILON,
                 "Vec3Dot( geoGlob.worldSpaceXyz[tri->indices[3 - ptIndexPrev - ptIndex]],"
                 " sidePlane ) - sidePlane[3] <= EQUAL_EPSILON\n\t%g, %g",
                 opposite - sidePlane.dist, GEO_EQUAL_EPSILON );

        ClipWindingEpsilon( w, sidePlane.normal, sidePlane.dist, GEO_COLLISION_EPSILON,
                            &front, &back, qfalse );
        FreeWinding( w );

        if ( front )
            Geo_ClipWindingByNodeTris( node, triIndex + 1, front, hunk );

        w           = back;
        ptIndexPrev = ptIndex;
    }

    if ( w )
        FreeWinding( w );
}

/* Geo_NodeWinding  0x0040d740 */
static winding_t *Geo_NodeWinding( const GeoBox_t *aabb, const GeoCollisionNode_t *node )
{
    GeoPlane_t plane;
    winding_t *w;
    int        axis;
    int        side;

    Geo_TrianglePlane( node->tris[0], &plane );

    w = BaseWindingForPlane( plane.normal, plane.dist );

    Assertx( DoesTriangleIntersectBox( node->tris[0], aabb ),
             "%s", "DoesTriangleIntersectBox( node->tris[0], aabb )" );

    plane.normal[0] = 0.0f;
    plane.normal[1] = 0.0f;
    plane.normal[2] = 0.0f;

    for ( axis = 0; axis < 3; axis++ )
    {
        for ( side = 0; side < 2; side++ )
        {
            float sign = side ? -1.0f : 1.0f;

            plane.normal[axis] = sign;
            plane.dist         = sign * aabb->center[axis] - aabb->halfSize[axis];

            ClipWindingByPlane( &w, plane.normal, plane.dist, GEO_COLLISION_EPSILON );

            if ( !w )
                return NULL;
        }

        plane.normal[axis] = 0.0f;
    }

    while ( node->parent )
    {
        Geo_TrianglePlane( node->parent->tris[0], &plane );

        if ( node == node->parent->children[0] )
        {
            plane.normal[0] = -plane.normal[0];
            plane.normal[1] = -plane.normal[1];
            plane.normal[2] = -plane.normal[2];
            plane.dist      = -plane.dist;
        }

        ClipWindingByPlane( &w, plane.normal, plane.dist, GEO_COLLISION_EPSILON );

        if ( !w )
            return NULL;

        node = node->parent;
    }

    return w;
}

/* Geo_MakeLeafLinks  0x0040d920 */
static void Geo_MakeLeafLinks( GeoCollisionNode_t *node, const GeoBox_t *aabb,
                               GeoHunk_t *hunk )
{
    for ( ;; )
    {
        winding_t *w;

        Assertx( node, "%s", "node" );

        if ( !node->triCount )
            return;

        w = Geo_NodeWinding( aabb, node );

        if ( w )
            Geo_ClipWindingByNodeTris( node, 0, w, hunk );

        Geo_MakeLeafLinks( node->children[0], aabb, hunk );

        node = node->children[1];
    }
}

/* Geo_LeafReaches  0x0040d990 */
static qboolean Geo_LeafReaches( GeoCollisionNode_t *from, const GeoCollisionNode_t *to,
                                 int mark )
{
    const GeoLeafLink_t *link;

    if ( from == to )
        return qtrue;

    from->visitMark = mark;

    for ( link = from->links; link; link = link->next )
    {
        if ( link->node->visitMark == mark )
            continue;

        if ( Geo_LeafReaches( link->node, to, mark ) )
            return qtrue;
    }

    return qfalse;
}

/* Geo_FindCollisionLeaf  0x0040d9e0 */
static GeoCollisionNode_t *Geo_FindCollisionLeaf( GeoCollisionNode_t *node,
                                                  const vec3_t point )
{
    while ( node->triCount )
    {
        GeoPlane_t plane;

        Geo_TrianglePlane( node->tris[0], &plane );

        node = node->children[plane.dist < Vec3Dot( point, plane.normal )];
    }

    return node;
}

/* Geo_GridCornerIsCutOff  0x0040da70 */
qboolean Geo_GridCornerIsCutOff( int threadIndex, const vec3_t origin, int corner )
{
    GeoHunk_t          *hunk = &geoGlob.hunk[threadIndex];
    GeoBox_t            box;
    GeoTriangle_t     **triList;
    GeoCollisionNode_t *node;
    GeoCollisionNode_t *leaf;
    vec3_t              probe;
    int                 triCount;
    int                 other;
    int                 opposite;

    box.halfSize[0] = 16.0f;
    box.halfSize[1] = 16.0f;
    box.halfSize[2] = 32.0f;

    box.center[0] = origin[0] + ( ( corner & 1 ) ? -16.0f : 16.0f );
    box.center[1] = origin[1] + ( ( corner & 2 ) ? -16.0f : 16.0f );
    box.center[2] = origin[2] + ( ( corner & 4 ) ? -32.0f : 32.0f );

    hunk->used = 0;

    triCount = Geo_TrianglesInBox_r( 0, &box, ( GeoTriangle_t ** )hunk->base, 0,
                                     hunk->size / sizeof( GeoTriangle_t * ) );

    if ( !triCount )
        return qfalse;

    triList = ( GeoTriangle_t ** )Geo_HunkAlloc( hunk,
                                                 triCount * sizeof( GeoTriangle_t * ) );

    node = Geo_BuildCollisionNode( NULL, triList, triCount, hunk );

    Geo_MakeLeafLinks( node, &box, hunk );

    box.halfSize[0] -= 0.125f;
    box.halfSize[1] -= 0.125f;
    box.halfSize[2] -= 0.125f;

    opposite = corner ^ 7;

    probe[0] = box.center[0] + ( ( opposite & 1 ) ? -box.halfSize[0] : box.halfSize[0] );
    probe[1] = box.center[1] + ( ( opposite & 2 ) ? -box.halfSize[1] : box.halfSize[1] );
    probe[2] = box.center[2] + ( ( opposite & 4 ) ? -box.halfSize[2] : box.halfSize[2] );

    leaf = Geo_FindCollisionLeaf( node, probe );

    for ( other = 0; other < 8; other++ )
    {
        if ( other == opposite )
            continue;

        probe[0] = box.center[0] + ( ( other & 1 ) ? -box.halfSize[0] : box.halfSize[0] );
        probe[1] = box.center[1] + ( ( other & 2 ) ? -box.halfSize[1] : box.halfSize[1] );
        probe[2] = box.center[2] + ( ( other & 4 ) ? -box.halfSize[2] : box.halfSize[2] );

        if ( !LightGrid_HasPointAtOrigin( probe ) )
            continue;

        if ( Geo_PointIsSolid( probe ) )
            continue;

        if ( !Geo_LeafReaches( leaf, Geo_FindCollisionLeaf( node, probe ), other + 1 ) )
            return qtrue;
    }

    return qfalse;
}

/* Geo_TriangleCount  0x0040de90 */
int Geo_TriangleCount( void )
{
    return geoTriCount;
}

/* Original: c:\trees\cod3\cod3src\cod2rad\modelcollision.cpp */

#include "cod4rad.h"
#include "modelcollision.h"
#include "progress.h"
#include "com_math.h"
#include "cmdline.h"
#include "maskedmaterial.h"
#include "materials.h"
#include "xmodel.h"
#include "dobj.h"
#include "cm_tracebox.h"

#include <new>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


float           modelMaxZ;                              /* 0x130a3df4 */
RadModel_t     *radModels;                              /* 0x130a3df8 */
RadModelDef_t  *radModelDefs;                           /* 0x130a3df0 */
ModelCellNode_t modelCells[MODEL_CELL_NODE_COUNT];      /* 0x130a3dfc */


#if defined( _WIN64 ) && !defined( SHOW_LAYOUT_CHECKS )
/* The layouts below describe the 32-bit binary; pointers are twice as big on x64 */
#define MC_CHECK( name, cond )  typedef char name[1]
#else
#define MC_CHECK( name, cond )  typedef char name[( cond ) ? 1 : -1]
#endif

MC_CHECK( mc_cell_size, sizeof( ModelCellNode_t ) == 16 );
MC_CHECK( mc_node_size, sizeof( ModelAabbNode_t ) == 40 );
MC_CHECK( mc_model_next, offsetof( RadModel_t, next ) == 0x60 );
MC_CHECK( mc_model_mins, offsetof( RadModel_t, mins ) == 0x44 );
MC_CHECK( mc_model_maxs, offsetof( RadModel_t, maxs ) == 0x50 );
MC_CHECK( mc_model_axis, offsetof( RadModel_t, axis ) == 0x20 );
MC_CHECK( mc_model_size, sizeof( RadModel_t ) == 100 );
MC_CHECK( mc_tri_size, sizeof( ModelCollTri_t ) == 64 );


/* ModelCollision_BuildTri  0x0041a450 */
static void ModelCollision_BuildTri( ModelCollTri_t *tri, const unsigned short *indices,
                                     const vec3_t *xyz, const float *texCoords,
                                     MskMaterial_t *mskMtl,
                                     vec3_t triMins, vec3_t triMaxs )
{
    const float *v0 = xyz[indices[0]];
    const float *v1 = xyz[indices[1]];
    const float *v2 = xyz[indices[2]];

    tri->xyz[0] = v0[0];
    tri->xyz[1] = v0[1];
    tri->xyz[2] = v0[2];

    tri->edge0[0] = tri->xyz[0] - v1[0];
    tri->edge0[1] = tri->xyz[1] - v1[1];
    tri->edge0[2] = tri->xyz[2] - v1[2];

    tri->edge1[0] = tri->xyz[0] - v2[0];
    tri->edge1[1] = tri->xyz[1] - v2[1];
    tri->edge1[2] = tri->xyz[2] - v2[2];

    tri->mskMtl = mskMtl;

    tri->st[0][0] = texCoords[indices[0] * 2 + 0];
    tri->st[0][1] = texCoords[indices[0] * 2 + 1];
    tri->st[1][0] = texCoords[indices[1] * 2 + 0];
    tri->st[1][1] = texCoords[indices[1] * 2 + 1];
    tri->st[2][0] = texCoords[indices[2] * 2 + 0];
    tri->st[2][1] = texCoords[indices[2] * 2 + 1];

    triMins[0] = v0[0];
    triMins[1] = v0[1];
    triMins[2] = v0[2];

    triMaxs[0] = v0[0];
    triMaxs[1] = v0[1];
    triMaxs[2] = v0[2];

    AddPointToBounds( v1, triMins, triMaxs );
    AddPointToBounds( v2, triMins, triMaxs );
}

static qboolean ModelCollision_SurfaceCasts( const MskMaterial_t *mskMtl )
{
    const Material_t *material = mskMtl->material;

    if ( ( material->toolFlags & TOOLFLAG_USAGE_MASK ) != 0x10 && !mskMtl->mask )
        return qfalse;

    if ( material->surfaceFlags & MODEL_SURF_NO_CAST )
        return qfalse;

    return ( material->contentFlags & MODEL_CONTENTS_CAST ) != 0;
}

/* ModelCollision_BuildModelDef  0x0041a590 */
static RadModelDef_t *ModelCollision_BuildModelDef( XModel_t *xmodel )
{
    MskMaterial_t  *mskMtls[MODEL_MAX_SURFACES];
    vec3_t          xyz[MODEL_MAX_MESH_VERTS];
    float           texCoords[MODEL_MAX_MESH_VERTS * 2];
    unsigned short  indices[MODEL_MAX_MESH_INDICES];
    DObjModel_t     dobjModels[1];
    DObjAnimMat_t   animMats[DOBJ_MAX_PARTS];
    DObj_t          obj;
    char            lods[8];
    int             partBits[4];
    RadModelDef_t  *def;
    ModelCollTri_t *tris;
    vec3_t         *triMins;
    vec3_t         *triMaxs;
    int             surfCount;
    int             triCount;
    int             triIndex;
    int             i;
    int             t;

    def = ( RadModelDef_t * )malloc( sizeof( RadModelDef_t ) );

    if ( !def )
        Error( "Out of memory for model collision def (%i bytes)",
               sizeof( RadModelDef_t ) );

    def->xmodel = xmodel;
    def->tree   = NULL;
    def->next   = radModelDefs;

    radModelDefs = def;

    if ( XModelBadLoad( xmodel ) )
        return def;

    dobjModels[0].model    = xmodel;
    dobjModels[0].boneName = 0;
    dobjModels[0].flags    = 0;

    DObj_Create( dobjModels, 1, NULL, &obj, 0 );

    DObj_SetAnimMats( &obj, animMats, dobjModels );

    partBits[0] = -1;
    partBits[1] = -1;
    partBits[2] = -1;
    partBits[3] = -1;

    DObj_SetPartBits( &obj, partBits );

    memset( lods, 0, sizeof( lods ) );

    surfCount = DObj_GetSurfCount( &obj, partBits, lods );

    Assertx( surfCount == XModelLodNumSurfaces( xmodel, 0 ),
             "model name: '%s'", XModelName( xmodel ) );

    Assert( surfCount <= MODEL_MAX_SURFACES );

    triCount = 0;

    for ( i = 0; i < surfCount; i++ )
    {
        MskMaterial_t *mskMtl =
            FindMaskMaterial( DObj_GetSurfaceMaterialName( &obj, 0, i, 0 ),
                              MTL_USAGE_MODEL_VCOL );

        mskMtls[i] = mskMtl;

        if ( !ModelCollision_SurfaceCasts( mskMtl ) )
        {
            mskMtls[i] = NULL;
            continue;
        }

        triCount += XSurfaceGetTriCount( DObj_GetSurface( &obj, 0, i, 0 ) );
    }

    if ( !triCount )
    {
        DObj_Free( &obj );
        return def;
    }

    tris = ( ModelCollTri_t * )malloc( triCount * sizeof( ModelCollTri_t ) );

    if ( !tris )
        Error( "Out of memory for model collision mesh (%i bytes)",
               triCount * sizeof( ModelCollTri_t ) );

    triMins = ( vec3_t * )malloc( triCount * 2 * sizeof( vec3_t ) );

    if ( !triMins )
        Error( "Out of memory for model collision mesh (%i bytes)",
               triCount * 2 * sizeof( vec3_t ) );

    triMaxs = triMins + triCount;

    triIndex = 0;

    for ( i = 0; i < surfCount; i++ )
    {
        XSurface_t *surface;
        int         surfTris;

        if ( !mskMtls[i] )
            continue;

        surface = DObj_GetSurface( &obj, 0, i, 0 );

        XSurfaceCopyTriIndices( surface, indices, 0 );
        XSurfaceCopyVertexData( surface, xyz, texCoords, NULL );

        surfTris = XSurfaceGetTriCount( surface );

        if ( indices[surfTris * 3 - 3] == indices[surfTris * 3 - 2] )
            surfTris--;

        for ( t = 0; t < surfTris; t++ )
            ModelCollision_BuildTri( &tris[triIndex + t], &indices[t * 3],
                                     xyz, texCoords, mskMtls[i],
                                     triMins[triIndex + t], triMaxs[triIndex + t] );

        triIndex += surfTris;
    }

    def->tree = ModelCollision_BuildAabbTree( triIndex, tris, triMins, triMaxs );

    free( triMins );

    DObj_Free( &obj );

    return def;
}

/* ModelCollision_FindModelDef  0x0041a950 */
static RadModelDef_t *ModelCollision_FindModelDef( XModel_t *xmodel )
{
    RadModelDef_t *def;

    for ( def = radModelDefs; def; def = def->next )
    {
        if ( def->xmodel == xmodel )
            break;
    }

    if ( !def )
        def = ModelCollision_BuildModelDef( xmodel );

    return def->tree ? def : NULL;
}

/* ModelCollision_AddStaticModel  0x0041a990 */
void ModelCollision_AddStaticModel( XModel_t *xmodel, const vec3_t scale,
                                    const orientation_t *orient, qboolean scripted )
{
    RadModelDef_t *def;
    RadModel_t    *model;
    float          localAxis[3][3];
    vec3_t         absBounds[2];
    int            i;

    def = ModelCollision_FindModelDef( xmodel );

    if ( !def )
        return;

    for ( i = 0; i < 3; i++ )
    {
        localAxis[i][0] = orient->axis[i][0] * scale[i];
        localAxis[i][1] = orient->axis[i][1] * scale[i];
        localAxis[i][2] = orient->axis[i][2] * scale[i];
    }

    MatrixTransformBounds( ( const vec3_t * )def->tree, orient->origin,
                           localAxis, absBounds );

    if ( !options.modelShadow )
        return;

    Assert( absBounds[1][0] - absBounds[0][0] >= 0 );
    Assert( absBounds[1][1] - absBounds[0][1] >= 0 );
    Assert( absBounds[1][2] - absBounds[0][2] >= 0 );

    model = ( RadModel_t * )malloc( sizeof( RadModel_t ) );

    if ( !model )
        Error( "Out of memory on static model instance" );

    model->next = radModels;
    radModels = model;

    model->def      = def;
    model->scripted = ( byte )scripted;

    model->origin[0] = orient->origin[0];
    model->origin[1] = orient->origin[1];
    model->origin[2] = orient->origin[2];

    MatrixInverse33( localAxis, model->axis );

    model->mins[0] = absBounds[0][0];
    model->mins[1] = absBounds[0][1];
    model->mins[2] = absBounds[0][2];

    model->maxs[0] = absBounds[1][0];
    model->maxs[1] = absBounds[1][1];
    model->maxs[2] = absBounds[1][2];
}

/* ModelCollision_BuildCellTree_r  0x0041a100 */
static void ModelCollision_BuildCellTree_r( unsigned nodeIndex,
                                            const float *mins, const float *maxs )
{
    float bound[2];
    float dx, dy;
    int   axis;
    float mid;

    if ( nodeIndex > MODEL_CELL_NODE_COUNT - 1 )
        return;

    dx = maxs[0] - mins[0];
    dy = maxs[1] - mins[1];

    axis = ( dx < dy ) ? 1 : 0;

    mid = ( mins[axis] + maxs[axis] ) * 0.5f;

    modelCells[nodeIndex].axis = axis;
    modelCells[nodeIndex].dist = mid;

    bound[axis] = mid;

    bound[1 - axis] = maxs[1 - axis];
    ModelCollision_BuildCellTree_r( nodeIndex * 2 + 1, mins, bound );

    bound[1 - axis] = mins[1 - axis];
    ModelCollision_BuildCellTree_r( nodeIndex * 2 + 2, bound, maxs );
}

/* ModelCollision_InsertModel  0x0041a1c0 */
static void ModelCollision_InsertModel( RadModel_t *model )
{
    ModelCellNode_t *cell;
    unsigned         nodeIndex;

    if ( modelMaxZ < model->maxs[2] )
        modelMaxZ = model->maxs[2];

    nodeIndex = 0;
    cell      = &modelCells[0];

    cell->itemCount++;

    for ( ;; )
    {
        int axis = cell->axis;

        if ( model->maxs[axis] < cell->dist - MODEL_CELL_EPSILON )
            nodeIndex = nodeIndex * 2 + 1;
        else if ( model->mins[axis] > cell->dist + MODEL_CELL_EPSILON )
            nodeIndex = nodeIndex * 2 + 2;
        else
            break;

        cell = &modelCells[nodeIndex];
        cell->itemCount++;

        if ( nodeIndex * 2 + 2 >= MODEL_CELL_NODE_COUNT - 1 )
            break;
    }

    model->next  = cell->models;
    cell->models = model;
}

/* ModelCollision_BuildCellTree  0x0041a250 */
void ModelCollision_BuildCellTree( const vec3_t mins, const vec3_t maxs )
{
    RadModel_t *model;

    ModelCollision_BuildCellTree_r( 0, mins, maxs );

    modelMaxZ = MODEL_MAX_Z_START;

    model = radModels;

    while ( model )
    {
        RadModel_t *next = model->next;

        ModelCollision_InsertModel( model );

        model = next;
    }

    radModels = NULL;
}

/* ModelCollision_BuildAabbTree  0x0041a2a0 */
ModelAabbNode_t *ModelCollision_BuildAabbTree( int triCount, void *tris,
                                               vec3_t *triMins, vec3_t *triMaxs )
{
    AabbTreeNode_t    treeNodes[MODEL_AABB_MAX_NODES];
    AabbTreeBuilder_t builder;
    ModelAabbNode_t  *nodes;
    int               nodeCount;
    int               i;
    int               item;

    builder.itemData         = tris;
    builder.itemCount        = triCount;
    builder.itemStride       = MODEL_AABB_ITEM_STRIDE;
    builder.reorderBounds    = 1;
    builder.itemMins         = triMins;
    builder.itemMaxs         = triMaxs;
    builder.nodes            = treeNodes;
    builder.maxNodes         = MODEL_AABB_MAX_NODES;
    builder.minPartitionSize = MODEL_AABB_MIN_PARTITION;
    builder.minLeafItems     = MODEL_AABB_MIN_LEAF_ITEMS;

    nodeCount = AabbBuildTree( &builder );

    nodes = new ( std::nothrow ) ModelAabbNode_t[nodeCount];

    if ( !nodes )
        Error( "Couldn't allocate %i bytes for model collision AABB tree",
               nodeCount * sizeof( ModelAabbNode_t ) );

    for ( i = 0; i < nodeCount; i++ )
    {
        Assertx( treeNodes[i].firstItem >= 0, "treeNodes[nodeIndex].firstItem >= 0" );
        Assertx( treeNodes[i].firstItem + treeNodes[i].itemCount <= triCount,
                 "treeNodes[nodeIndex].firstItem + treeNodes[nodeIndex].itemCount"
                 " <= triCount" );

        nodes[i].itemCount  = treeNodes[i].itemCount;
        nodes[i].items      = ( byte * )tris
                            + treeNodes[i].firstItem * MODEL_AABB_ITEM_STRIDE;
        nodes[i].childCount = treeNodes[i].childCount;
        nodes[i].children   = &nodes[treeNodes[i].firstChild];

        ClearBounds( nodes[i].mins, nodes[i].maxs );

        for ( item = treeNodes[i].firstItem;
              item < treeNodes[i].firstItem + treeNodes[i].itemCount;
              item++ )
            AddBoundsToBounds( triMins[item], triMaxs[item],
                               nodes[i].mins, nodes[i].maxs );
    }

    return nodes;
}

/* ModelCollision_TraceAabbNode  0x0041ab80 */
static void ModelCollision_TraceAabbNode( const ModelAabbNode_t *node, int scripted,
                                          GeoTrace_t *trace )
{
    int i;

    if ( CM_TraceLineMissesBounds( &trace->line, node->mins, node->maxs,
                                   trace->result.frac ) )
        return;

    if ( node->childCount )
    {
        for ( i = 0; i < node->childCount; i++ )
            ModelCollision_TraceAabbNode( ( const ModelAabbNode_t * )node->children + i,
                                          scripted, trace );
        return;
    }

    for ( i = 0; i < node->itemCount; i++ )
        Geo_TraceModelTriangle( ( const ModelCollTri_t * )node->items + i,
                                scripted, trace );
}

/* ModelCollision_TraceModel  0x0041ac10 */
void ModelCollision_TraceModel( RadModel_t *model, GeoTrace_t *trace )
{
    GeoTrace_t saved;

    memcpy( &saved, trace, offsetof( GeoTrace_t, result ) );

    while ( model )
    {
        if ( !CM_TraceLineMissesBounds( &saved.line, model->mins, model->maxs,
                                        trace->result.frac ) )
        {
            vec3_t local;

            local[0] = saved.line.start[0] - model->origin[0];
            local[1] = saved.line.start[1] - model->origin[1];
            local[2] = saved.line.start[2] - model->origin[2];

            MatrixTransformVector( local, model->axis, trace->line.start );

            local[0] = saved.line.end[0] - model->origin[0];
            local[1] = saved.line.end[1] - model->origin[1];
            local[2] = saved.line.end[2] - model->origin[2];

            MatrixTransformVector( local, model->axis, trace->line.end );

            trace->delta[0] = trace->line.end[0] - trace->line.start[0];
            trace->delta[1] = trace->line.end[1] - trace->line.start[1];
            trace->delta[2] = trace->line.end[2] - trace->line.start[2];

            CM_SetTraceInvDelta( &trace->line );

            if ( saved.supersampleAlpha )
            {
                MatrixTransformVector( saved.alphaAxis[0], model->axis,
                                       trace->alphaAxis[0] );
                MatrixTransformVector( saved.alphaAxis[1], model->axis,
                                       trace->alphaAxis[1] );
            }

            ModelCollision_TraceAabbNode( model->def->tree, model->scripted, trace );
        }

        model = model->next;
    }

    memcpy( trace, &saved, offsetof( GeoTrace_t, result ) );
}

/* ModelCollision_TraceNode_r  0x0041ad30 */
void ModelCollision_TraceNode_r( int nodeIndex, const vec3_t start,
                                 const vec3_t end, GeoTrace_t *trace )
{
    ModelCellNode_t *node;

    if ( !( modelMaxZ > start[2] ) && !( end[2] < modelMaxZ ) )
        return;

    node = &modelCells[nodeIndex];

    while ( node->itemCount )
    {
        int child;

        if ( node->models )
            ModelCollision_TraceModel( node->models, trace );

        child = nodeIndex * 2 + 2;

        if ( ( unsigned )child >= MODEL_CELL_NODE_COUNT - 1 )
            return;

        if ( !( node->dist < start[node->axis] ) && !( node->dist < end[node->axis] ) )
        {
            nodeIndex = nodeIndex * 2 + 1;
        }
        else if ( !( node->dist > start[node->axis] ) && !( node->dist > end[node->axis] ) )
        {
            nodeIndex = child;
        }
        else
        {
            vec3_t mid;
            float  frac;

            Assertx( end[node->axis] - start[node->axis] != 0,
                     "end[node->axis] - start[node->axis] != 0" );

            frac = ( node->dist - start[node->axis] )
                     / ( end[node->axis] - start[node->axis] );

            mid[0] = ( end[0] - start[0] ) * frac + start[0];
            mid[1] = ( end[1] - start[1] ) * frac + start[1];
            mid[2] = ( end[2] - start[2] ) * frac + start[2];

            mid[node->axis] = node->dist;

            if ( node->dist > start[node->axis] )
            {
                ModelCollision_TraceNode_r( nodeIndex * 2 + 1, start, mid, trace );
                ModelCollision_TraceNode_r( child, mid, end, trace );
            }
            else
            {
                ModelCollision_TraceNode_r( child, start, mid, trace );
                ModelCollision_TraceNode_r( nodeIndex * 2 + 1, mid, end, trace );
            }

            return;
        }

        node = &modelCells[nodeIndex];
    }
}

/* Model_TraceLine  0x0041af10 */
void Model_TraceLine( GeoTrace_t *trace )
{
    vec3_t start;
    vec3_t end;
    float  frac = trace->result.frac;

    start[0] = trace->line.start[0];
    start[1] = trace->line.start[1];
    start[2] = trace->line.start[2];

    end[0] = ( trace->line.end[0] - trace->line.start[0] ) * frac + trace->line.start[0];
    end[1] = ( trace->line.end[1] - trace->line.start[1] ) * frac + trace->line.start[1];
    end[2] = ( trace->line.end[2] - trace->line.start[2] ) * frac + trace->line.start[2];

    ModelCollision_TraceNode_r( 0, start, end, trace );
}

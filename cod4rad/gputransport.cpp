/* GPU acceleration of the radiosity transport trace (-gpu) */

#include "cod4rad.h"
#include "gputransport.h"
#include "gputrace.h"
#include "geometry.h"
#include "maskedmaterial.h"
#include "progress.h"

#include <string.h>
#include <map>
#include <vector>


bool gpuTransportRequested;

static bool gpuTransportEnabled;


bool GpuTransport_Enabled( void )
{
    return gpuTransportEnabled;
}

/* Registers the alpha mask of a material, returning its index (or -1 if opaque) */
static int GpuTransport_AddMaterial( const MskMaterial_t *mskMtl,
                                     std::map<const MskMaterial_t *, int> &mtlIndex,
                                     std::vector<GpuMtl_t> &mtls,
                                     std::vector<unsigned> &masks )
{
    std::map<const MskMaterial_t *, int>::const_iterator found;
    GpuMtl_t   mtl;
    size_t     byteCount;
    size_t     wordCount;
    size_t     firstWord;

    if ( !mskMtl->mask )
        return -1;

    found = mtlIndex.find( mskMtl );
    if ( found != mtlIndex.end() )
        return found->second;

    byteCount = ( ( size_t )mskMtl->maskWidth * ( size_t )mskMtl->maskHeight + 7 ) / 8;
    wordCount = ( byteCount + 3 ) / 4;
    firstWord = masks.size();

    masks.resize( firstWord + wordCount, 0u );
    memcpy( &masks[firstWord], mskMtl->mask, byteCount );

    mtl.offset = ( unsigned )firstWord;
    mtl.width  = ( unsigned )mskMtl->maskWidth;
    mtl.height = ( unsigned )mskMtl->maskHeight;
    mtl.pad    = 0;

    mtls.push_back( mtl );
    mtlIndex[mskMtl] = ( int )mtls.size() - 1;

    return ( int )mtls.size() - 1;
}

bool GpuTransport_Init( void )
{
    std::vector<GpuNode_t> nodes;
    std::vector<GpuLeaf_t> leafs;
    std::vector<int>       triRefs;
    std::vector<GpuTri_t>  tris;
    std::vector<GpuMtl_t>  mtls;
    std::vector<unsigned>  masks;
    std::map<const MskMaterial_t *, int> mtlIndex;
    GpuScene_t scene;
    char       err[1024];
    int        i;

    gpuTransportEnabled = false;

    if ( !geoGlob.bspNodes || geoGlob.bspNodeCount < 1 )
    {
        Print( "GPU: no collision tree available, using the CPU\n" );
        return false;
    }

    nodes.resize( geoGlob.bspNodeCount );
    for ( i = 0; i < geoGlob.bspNodeCount; i++ )
    {
        const GeoBspNode_t *src = &geoGlob.bspNodes[i];

        nodes[i].normal[0] = src->plane.normal[0];
        nodes[i].normal[1] = src->plane.normal[1];
        nodes[i].normal[2] = src->plane.normal[2];
        nodes[i].dist      = src->plane.dist;
        nodes[i].child[0]  = src->children[0];
        nodes[i].child[1]  = src->children[1];
    }

    leafs.resize( geoGlob.bspLeafCount > 0 ? geoGlob.bspLeafCount : 0 );
    for ( i = 0; i < geoGlob.bspLeafCount; i++ )
    {
        leafs[i].triCount    = geoGlob.bspLeafs[i].triCount;
        leafs[i].firstTriRef = geoGlob.bspLeafs[i].firstTriRef;
    }

    /* The leaves only ever index below triRefUsed */
    triRefs.resize( geoGlob.triRefUsed );
    for ( i = 0; i < geoGlob.triRefUsed; i++ )
    {
        const GeoTriangle_t *tri = geoGlob.triRefs[i];

        if ( tri < geoTris || tri >= geoTris + geoTriCount )
            Error( "GPU: triangle reference %i points outside the triangle list\n", i );

        triRefs[i] = ( int )( tri - geoTris );
    }

    /* Every triangle is uploaded so that indices match geoTris.  Triangles that
       aren't in the tree (models, non shadow casters) are simply never visited. */
    tris.resize( geoTriCount > 0 ? geoTriCount : 0 );
    for ( i = 0; i < geoTriCount; i++ )
    {
        const GeoTriangle_t *src = &geoTris[i];
        GpuTri_t            *dst = &tris[i];
        const float         *v0  = geoGlob.worldSpaceXyz[src->indices[0]];
        const float         *v1  = geoGlob.worldSpaceXyz[src->indices[1]];
        const float         *v2  = geoGlob.worldSpaceXyz[src->indices[2]];

        dst->v0[0] = v0[0]; dst->v0[1] = v0[1]; dst->v0[2] = v0[2];
        dst->v1[0] = v1[0]; dst->v1[1] = v1[1]; dst->v1[2] = v1[2];
        dst->v2[0] = v2[0]; dst->v2[1] = v2[1]; dst->v2[2] = v2[2];

        dst->nx = src->normal[0];
        dst->ny = src->normal[1];
        dst->nz = src->normal[2];

        dst->st0[0] = geoVertices[src->indices[0]].texCoord[0];
        dst->st0[1] = geoVertices[src->indices[0]].texCoord[1];
        dst->st1[0] = geoVertices[src->indices[1]].texCoord[0];
        dst->st1[1] = geoVertices[src->indices[1]].texCoord[1];
        dst->st2[0] = geoVertices[src->indices[2]].texCoord[0];
        dst->st2[1] = geoVertices[src->indices[2]].texCoord[1];

        dst->mtl = GpuTransport_AddMaterial( src->mskMtl, mtlIndex, mtls, masks );
        dst->pad = 0;
    }

    scene.nodes   = &nodes[0];   scene.nodeCount     = ( int )nodes.size();
    scene.leafs   = leafs.empty()   ? NULL : &leafs[0];   scene.leafCount    = ( int )leafs.size();
    scene.triRefs = triRefs.empty() ? NULL : &triRefs[0]; scene.triRefCount  = ( int )triRefs.size();
    scene.tris    = tris.empty()    ? NULL : &tris[0];    scene.triCount     = ( int )tris.size();
    scene.mtls    = mtls.empty()    ? NULL : &mtls[0];    scene.mtlCount     = ( int )mtls.size();
    scene.masks   = masks.empty()   ? NULL : &masks[0];   scene.maskWordCount = ( int )masks.size();

    if ( !GpuTrace_Init( &scene, err, sizeof( err ) ) )
    {
        Print( "GPU: %s\nGPU: falling back to the CPU\n", err );
        return false;
    }

    Print( "GPU: tracing radiosity on %s (%i triangles, %i tree nodes)\n",
           GpuTrace_AdapterName(), geoTriCount, geoGlob.bspNodeCount );

    gpuTransportEnabled = true;
    return true;
}

void GpuTransport_Shutdown( void )
{
    if ( gpuTransportEnabled )
    {
        double rays;
        double busy;
        double wait;

        GpuTrace_Stats( &rays, &busy, &wait );

        Print( "GPU: traced %.1f million rays, GPU busy for %.1f seconds"
               " (threads waited %.1f seconds for it)\n",
               rays * 1.0e-6, busy, wait );

        GpuTrace_Shutdown();
    }

    gpuTransportEnabled = false;
}

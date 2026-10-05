/* Original: c:\trees\cod3\cod3src\cod2rad\compile.cpp */

#include "cod4rad.h"
#include "compile.h"
#include "progress.h"
#include "pointlights.h"
#include "lightgrid.h"
#include "groundlight.h"

#include <new>
#include <string.h>
#include <math.h>
#include <intrin.h>
#include <stddef.h>
#include <stdlib.h>
#include "lighting.h"
#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "cmdline.h"
#include "geometry.h"
#include "surfaceflags.h"
#include "modelcollision.h"
#include "cmdline.h"
#include "geometry.h"
#include "surfaceflags.h"
#include "modelcollision.h"
#include "gputrace.h"
#include "gputransport.h"


#define RADIOSITY_JITTER_SCALE      ( 2.0f / 32767.0f )

#define RADIOSITY_MIN_ELEVATION     0.001f

#define RADIOSITY_TRACE_DISTANCE    262144.0f


compileGlob_t compileGlob;              /* 0x1162302c */

int   compileLightSlots;                /* 0x11623010 */
float compileBounceEnergy[THREAD_COUNT_MAX];    /* 0x11623014 */
int   compilePingPongSrc;               /* 0x1162314c */
int   compilePingPongDest;              /* 0x11623150 */

TransportHit_t compileTraceHits[THREAD_COUNT_MAX][RAD_TRACE_COUNT_MAX]
                               [TRANSPORT_MAX_HITS];   /* 0x11623158 */

RadiositySample_t *radiositySamples;    /* 0x11623028 */
float              radiositySampleScale;/* 0x11623024 */


#if defined( _WIN64 ) && !defined( SHOW_LAYOUT_CHECKS )
/* The layouts below describe the 32-bit binary; pointers are twice as big on x64 */
#define CP_CHECK( name, cond )  typedef char name[1]
#else
#define CP_CHECK( name, cond )  typedef char name[( cond ) ? 1 : -1]
#endif

CP_CHECK( cp_alloc_size, sizeof( BlockAllocator_t ) == 0x24 );
CP_CHECK( cp_glob_light, offsetof( compileGlob_t, lightAlloc ) == THREAD_COUNT_MAX * 0x24 );
CP_CHECK( cp_lightref,   sizeof( DirectTransport_t ) == 0x18 );
CP_CHECK( cp_bounceref,  sizeof( BounceRef_t ) == 8 );


static int Com_Log2( unsigned value )
{
    unsigned long index;

    return _BitScanReverse( &index, value ) ? ( int )index : -1;
}

/* BlockAlloc_AddFree  0x004066f0 */
static void BlockAlloc_AddFree( BlockAllocator_t *blockAlloc,
                                BlockAllocatorFreeList *block, unsigned count )
{
    unsigned units = ( blockAlloc->granularityMask + count ) >> blockAlloc->granularityShift;

    if ( units <= blockAlloc->smallListCount )
    {
        block->next = blockAlloc->smallLists[units - 1];
        blockAlloc->smallLists[units - 1] = block;
        return;
    }

    block->count = units;
    block->next  = blockAlloc->largeList;
    blockAlloc->largeList = block;
}

/* BlockAlloc_Init  0x004065a0 */
void BlockAlloc_Init( BlockAllocator_t *blockAlloc, int blockSize,
                      int blockGranularity, unsigned poolCount,
                      unsigned smallListCount )
{
    Assertx( blockAlloc, "blockAlloc" );

    Assertx( ( unsigned )( blockSize * blockGranularity ) >= sizeof( BlockAllocatorFreeList ),
             "blockSize * blockGranularity >= sizeof( BlockAllocatorFreeList )" );

    Assertx( blockGranularity > 0 && !( blockGranularity & ( blockGranularity - 1 ) ),
             "blockGranularity > 0 && IsPowerOf2( blockGranularity )" );

    Assertx( !( poolCount & ( blockGranularity - 1 ) ),
             "(poolCount & (blockGranularity - 1)) == 0" );

    Assertx( poolCount > 0, "poolCount > 0" );

    blockAlloc->blockSize        = blockSize;
    blockAlloc->granularityMask  = blockGranularity - 1;
    blockAlloc->granularityShift = Com_Log2( blockGranularity );
    blockAlloc->smallListCount   = smallListCount;
    blockAlloc->largeList        = NULL;

    blockAlloc->smallLists =
        new ( std::nothrow ) BlockAllocatorFreeList *[smallListCount];

    if ( !blockAlloc->smallLists )
        Error( "Out of memory initializing small lists for custom allocator" );

    memset( blockAlloc->smallLists, 0,
            smallListCount * sizeof( BlockAllocatorFreeList * ) );

    blockAlloc->pool      = NULL;
    blockAlloc->poolTotal = poolCount / blockGranularity;
    blockAlloc->poolUsed  = blockAlloc->poolTotal;
}

/* BlockAlloc_Alloc  0x00406730 */
void *BlockAlloc_Alloc( BlockAllocator_t *blockAlloc, unsigned count )
{
    unsigned                 units;
    unsigned                 granularity;
    BlockAllocatorFreeList  *block;
    BlockAllocatorFreeList **link;
    byte                    *result;

    units = ( blockAlloc->granularityMask + count ) >> blockAlloc->granularityShift;

    if ( units <= blockAlloc->smallListCount )
    {
        block = blockAlloc->smallLists[units - 1];

        if ( block )
        {
            blockAlloc->smallLists[units - 1] = block->next;
            return block;
        }
    }
    else
    {
        link = &blockAlloc->largeList;

        for ( block = blockAlloc->largeList; block; block = block->next )
        {
            if ( block->count >= units )
            {
                *link = block->next;

                if ( block->count > units )
                    BlockAlloc_AddFree( blockAlloc,
                                        ( BlockAllocatorFreeList * )
                                            ( ( byte * )block
                                              + blockAlloc->blockSize * units
                                                * ( blockAlloc->granularityMask + 1 ) ),
                                        ( block->count - units )
                                            * ( blockAlloc->granularityMask + 1 ) );

                return block;
            }

            link = &block->next;
        }
    }

    granularity = blockAlloc->granularityMask + 1;

    if ( blockAlloc->poolUsed + units > blockAlloc->poolTotal )
    {
        if ( blockAlloc->poolTotal != blockAlloc->poolUsed )
            BlockAlloc_AddFree( blockAlloc,
                                ( BlockAllocatorFreeList * )
                                    ( blockAlloc->pool
                                      + blockAlloc->blockSize * blockAlloc->poolUsed
                                        * granularity ),
                                ( blockAlloc->poolTotal - blockAlloc->poolUsed )
                                    * granularity );

        blockAlloc->pool = new ( std::nothrow )
            byte[blockAlloc->poolTotal * blockAlloc->blockSize * granularity];

        if ( !blockAlloc->pool )
            Error( "Out of memory allocating pool for custom allocator" );

        blockAlloc->poolUsed = 0;
    }

    Assertx( units <= blockAlloc->poolTotal,
             "blockCount <= blockAlloc->poolCount\n\t%i not <= %i",
             units, blockAlloc->poolTotal );

    result = blockAlloc->pool + blockAlloc->blockSize * blockAlloc->poolUsed * granularity;

    blockAlloc->poolUsed += units;

    return result;
}

/* BlockAlloc_Realloc  0x004068a0 */
void *BlockAlloc_Realloc( BlockAllocator_t *blockAlloc, void *block,
                          unsigned count, unsigned extraCount )
{
    unsigned oldUnits, newUnits, extraUnits;
    void    *result;

    if ( !count )
        return BlockAlloc_Alloc( blockAlloc, extraCount );

    oldUnits = ( blockAlloc->granularityMask + count ) >> blockAlloc->granularityShift;
    newUnits = ( blockAlloc->granularityMask + count + extraCount )
                 >> blockAlloc->granularityShift;

    extraUnits = newUnits - oldUnits;

    if ( !extraUnits )
        return block;

    if ( block == blockAlloc->pool
                    + ( blockAlloc->poolUsed - oldUnits ) * ( blockAlloc->granularityMask + 1 )
                      * blockAlloc->blockSize
      && blockAlloc->poolUsed + extraUnits <= blockAlloc->poolTotal )
    {
        blockAlloc->poolUsed += extraUnits;
        return block;
    }

    result = BlockAlloc_Alloc( blockAlloc, count + extraCount );

    memcpy( result, block, blockAlloc->blockSize * count );

    BlockAlloc_AddFree( blockAlloc, ( BlockAllocatorFreeList * )block, count );

    return result;
}

/* Compile_BasisWeights  0x00406960 */
static void Compile_BasisWeights( const vec3_t dir, float *weights )
{
    float angle;
    float sector;
    float up;
    float tilt;

    angle = ( float )atan2( dir[1], dir[0] );

    sector = ( float )( angle * BASIS_SECTORS_PER_TURN - 0.75 );

    if ( sector < 0.0f )
        sector = ( float )( sector + 3.0 );
    else if ( 3.0f < sector )
        sector = ( float )( sector - 3.0 );

    if ( sector < 1.0f )
    {
        weights[1] = 1.0f - sector;
        weights[2] = sector;
        weights[3] = 0.0f;
    }
    else if ( sector < 2.0f )
    {
        weights[1] = 0.0f;
        weights[2] = 2.0f - sector;
        weights[3] = sector - 1.0f;
    }
    else
    {
        weights[1] = sector - 2.0f;
        weights[2] = 0.0f;
        weights[3] = 3.0f - sector;
    }

    tilt = ( float )acos( dir[2] );
    up   = ( float )( 1.0 - tilt / BASIS_TILT_ANGLE );

    if ( up < 0.0f )
        up = 0.0f;
    else if ( 1.0f < up )
        up = 1.0f;

    weights[0] = up;

    weights[1] = ( 1.0f - up ) * weights[1];
    weights[2] = ( 1.0f - up ) * weights[2];
    weights[3] = ( 1.0f - up ) * weights[3];
}


/* GatherSampleToBasis  0x00406a80 */
void GatherSampleToBasis( const SampleVars_t *vars, vec3_t *basis )
{
    vec3_t   color;
    int      i;
    unsigned n;

    for ( i = 0; i < options.radiosityTraceCount; i++ )
    {
        float influence = vars->skyInfluence[i];

        if ( influence <= 0.0f )
            continue;

        color[0] = options.sunRadiosityColor[0] * influence;
        color[1] = options.sunRadiosityColor[1] * influence;
        color[2] = options.sunRadiosityColor[2] * influence;

        Lighting_AddToBasis( color, radiositySamples[i].dir, basis );
    }

    for ( n = 0; n < vars->directTransportCount; n++ )
        Lighting_AddToBasis( vars->directTransport[n].color, vars->directTransport[n].dir, basis );

    for ( n = 0; n < ( unsigned )vars->bounceCount; n++ )
    {
        const BounceRef_t *bounce = &vars->bounces[n];
        vec3_t             source;
        float              weight;

        Lighting_SampleColor( Lighting_PackedSampleVars( bounce->packed ), source );

        weight = bounce->weight;

        color[0] = source[0] * weight;
        color[1] = source[1] * weight;
        color[2] = source[2] * weight;

        Lighting_AddToBasis( color, radiositySamples[LMAP_PACKED_DIR( bounce->packed )].dir,
                             basis );
    }
}


static void Compile_RunBounces( int threads, float threshold );

/* RunLightCompile  0x004080b0 */
void RunLightCompile( int threadCount )
{
    compileLightSlots = PointLight_Count() + 2;

    Lighting_AllocBasisDirs();
    Lighting_AllocBleedMasks();

    Compile_InitAllocators( threadCount );

    Geo_BuildCollisionData();

    if ( gpuTransportRequested )
        GpuTransport_Init();

    StartProgress( "Calculating sample areas..." );
    Geo_CalcSampleAreas( threadCount );
    EndProgress();

    Lighting_AllocSamples();

    StartProgress( "Getting radiosity color for each sample..." );
    Geo_CalcRadiosityColors( threadCount );
    EndProgress();

    StartProgress( "Applying radiosity scale..." );
    Lighting_ScaleSamples();
    EndProgress();

    Compile_AllocRadiositySamples();

    if ( options.relight && Lighting_LoadTransfers() )
    {
        options.relightSave = 0;
        StartProgress( "Building light transport for light sources..." );
    }
    else
    {
        options.relight = 0;
        StartProgress( "Building light transport for everything..." );
    }

    Geo_BuildTransport( threadCount );
    EndProgress();

    GpuTransport_Shutdown();

    if ( options.relightSave )
        Lighting_SaveTransfers();

    StartProgress( "Normalizing transport weights..." );
    Lighting_NormaliseSamples( threadCount );
    EndProgress();

    StartProgress( "Seeding sky light..." );
    Lighting_SeedSkyLight( threadCount );
    EndProgress();

    if ( options.maxBounceCount > 1 )
        Compile_RunBounces( threadCount, 0.001f );

    StartProgress( "Finding lightmap bleeding..." );
    Lighting_FindBleeding( threadCount );
    EndProgress();

    StartProgress( "Building final lightmaps..." );
    Lighting_WriteSamples( threadCount );
    EndProgress();

    StartProgress( "Calculating ground lighting for static models..." );
    GroundLight_LightModels();
    EndProgress();

    LightGrid_Compile( threadCount );
}

/* Compile_SeedSkyLight  0x00406ba0 */
void Compile_SeedSkyLight( SampleVars_t *sampleVars )
{
    int i;

    for ( i = 0; i < options.radiosityTraceCount; i++ )
    {
        vec3_t transferredEnergy;
        float  weight;

        weight = sampleVars->skyInfluence[i] * radiositySamples[i].dir[2];

        transferredEnergy[0] = options.sunRadiosityColor[0] * weight;
        transferredEnergy[1] = options.sunRadiosityColor[1] * weight;
        transferredEnergy[2] = options.sunRadiosityColor[2] * weight;

        Assertx( !IS_NAN( transferredEnergy[0] ) && !IS_NAN( transferredEnergy[1] )
                 && !IS_NAN( transferredEnergy[2] ),
                 "!IS_NAN((transferredEnergy)[0]) && !IS_NAN((transferredEnergy)[1])"
                 " && !IS_NAN((transferredEnergy)[2])" );

        sampleVars->color[0] += transferredEnergy[0];
        sampleVars->color[1] += transferredEnergy[1];
        sampleVars->color[2] += transferredEnergy[2];

        sampleVars->unscatteredIncidentLight[compilePingPongDest][0] += transferredEnergy[0];
        sampleVars->unscatteredIncidentLight[compilePingPongDest][1] += transferredEnergy[1];
        sampleVars->unscatteredIncidentLight[compilePingPongDest][2] += transferredEnergy[2];

        Assertx( !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][0] )
                 && !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][1] )
                 && !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][2] ),
                 "!IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[0])"
                 " && !IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[1])"
                 " && !IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[2])" );
    }
}

/* Compile_AddSampleLight  0x00406f00 */
static void Compile_AddSampleLight( SampleVars_t *sampleVars, int threadIndex,
                                    const vec3_t color, const vec3_t dir )
{
    DirectTransport_t *light;

    sampleVars->directTransport = ( DirectTransport_t * )
        BlockAlloc_Realloc( &compileGlob.lightAlloc[threadIndex], sampleVars->directTransport,
                            sampleVars->directTransportCount, 1 );

    light = &sampleVars->directTransport[sampleVars->directTransportCount];

    light->color[0] = color[0];
    light->color[1] = color[1];
    light->color[2] = color[2];

    light->dir[0] = dir[0];
    light->dir[1] = dir[1];
    light->dir[2] = dir[2];

    sampleVars->directTransportCount++;

    Assertx( sampleVars->directTransportCount, "sampleVars->directTransportCount != 0" );
}

/* Compile_InitAllocators  0x00408000 */
void Compile_InitAllocators( int threads )
{
    unsigned lightPool;
    unsigned bouncePool;
    int      i;

    lightPool  = ( ( compileLightSlots * 0x1800 + 0xfffff ) & 0xfff00000 )
                 / ( sizeof( DirectTransport_t ) * 8 ) * 8;

    bouncePool = ( ( options.traceFilterWidth * options.radiosityTraceCount * 2048
                     + 0xfffff ) & 0xfff00000 )
                 / ( sizeof( BounceRef_t ) * 0x10 ) * 0x10;

    for ( i = 0; i < threads; i++ )
    {
        BlockAlloc_Init( &compileGlob.lightAlloc[i], sizeof( DirectTransport_t ), 8,
                         lightPool, 0x20 );

        BlockAlloc_Init( &compileGlob.bounceAlloc[i], sizeof( BounceRef_t ), 0x10,
                         bouncePool, 0x100 );
    }
}

static int Compile_ResolveTransport( GeoTrace_t &trace, const vec3_t traceStart,
                                     const vec3_t traceEnd, const vec3_t dir, int gpuFlags,
                                     qboolean pointFilter, TransportHit_t *hits,
                                     int *hitCount, vec3_t hitPos, vec3_t hitNormal );

/* Compile_TraceTransport  0x00407080 */
int Compile_TraceTransport( const vec3_t start, const vec3_t dir, float distance,
                                   qboolean pointFilter, TransportHit_t *hits,
                                   int *hitCount, vec3_t hitPos, vec3_t hitNormal )
{
    GeoTrace_t          trace;
    vec3_t              traceStart;
    vec3_t              traceEnd;

    *hitCount = 0;

    traceStart[0] = start[0] + dir[0] * TRANSPORT_START_OFFSET;
    traceStart[1] = start[1] + dir[1] * TRANSPORT_START_OFFSET;
    traceStart[2] = start[2] + dir[2] * TRANSPORT_START_OFFSET;

    traceEnd[0] = dir[0] * distance + traceStart[0];
    traceEnd[1] = dir[1] * distance + traceStart[1];
    traceEnd[2] = dir[2] * distance + traceStart[2];

    Geo_SetupTrace( traceStart, traceEnd, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    return Compile_ResolveTransport( trace, traceStart, traceEnd, dir, 0, pointFilter,
                                     hits, hitCount, hitPos, hitNormal );
}

/* Turns a finished trace into lightmap transport hits.  Shared by the CPU trace
   and the GPU path, which fabricates the trace from its single hit. */
static int Compile_ResolveTransport( GeoTrace_t &trace, const vec3_t traceStart,
                                     const vec3_t traceEnd, const vec3_t dir, int gpuFlags,
                                     qboolean pointFilter, TransportHit_t *hits,
                                     int *hitCount, vec3_t hitPos, vec3_t hitNormal )
{
    /* The CPU path passes the ray direction.  The GPU path (dir == NULL) passes the two
       things the direction is used for: whether the ray pointed down and whether it
       hit a back face. */
    const GeoHit_t     *last;
    const GeoTriangle_t *tri;
    TransportHit_t     *out = hits;
    unsigned            i;

    *hitCount = 0;

    if ( !trace.result.hitCount )
        return TRANSPORT_MISS;

    last = &trace.result.hits[trace.result.hitCount - 1];

    if ( hitPos )
    {
        float frac = last->frac;

        hitPos[0] = ( traceEnd[0] - traceStart[0] ) * frac + traceStart[0];
        hitPos[1] = ( traceEnd[1] - traceStart[1] ) * frac + traceStart[1];
        hitPos[2] = ( traceEnd[2] - traceStart[2] ) * frac + traceStart[2];
    }

    if ( hitNormal )
        Vec3Copy( last->tri->normal, hitNormal );

    tri = last->tri;

    if ( tri->mskMtl->material->surfaceFlags & SURF_SKY )
    {
        if ( dir ? ( 0.0f > dir[2] ) : ( ( gpuFlags & GPUHIT_DOWNWARD ) != 0 ) )
            return TRANSPORT_MISS;

        if ( hitNormal )
        {
            Model_TraceLine( &trace );

            if ( trace.result.hits[trace.result.hitCount - 1].geoType
                 == TRACE_HIT_MODEL_GEO )
                return TRANSPORT_MISS;
        }

        return TRANSPORT_SKY;
    }

    {
        bool backface = dir ? !( Vec3Dot( tri->normal, dir ) < 0.0f )
                            : ( ( gpuFlags & GPUHIT_BACKFACE ) != 0 );

        if ( backface )
        {
            if ( !tri->groundType )
                Geo_ClassifyGroundTriangle( ( GeoTriangle_t * )tri );

            return last->tri->groundType == GEO_GROUND_OVER_SKY ? TRANSPORT_GROUND
                                                                : TRANSPORT_BACKFACE;
        }
    }

    for ( i = 0; i < ( unsigned )trace.result.hitCount; i++ )
    {
        const GeoHit_t *hit = &trace.result.hits[i];
        const float    *lmap0;
        const float    *lmap1;
        const float    *lmap2;
        float           w;
        float           s;
        float           t;
        float           coverage;

        tri = hit->tri;

        if ( tri->lightmapIndex == LIGHTMAP_NONE )
            continue;

        w = 1.0f - hit->u - hit->v;

        lmap0 = geoVertices[tri->indices[0]].lmapCoord;
        lmap1 = geoVertices[tri->indices[1]].lmapCoord;
        lmap2 = geoVertices[tri->indices[2]].lmapCoord;

        s = lmap0[0] * w;
        t = lmap0[1] * w;

        s = lmap1[0] * hit->u + s;
        t = lmap1[1] * hit->u + t;

        s = lmap2[0] * hit->v + s;
        t = lmap2[1] * hit->v + t;

        s = s * LMAP_WIDTH_MIN;
        t = t * LMAP_HEIGHT_MIN;

        coverage = Geo_SampleMaskFraction( hit->alphaMask );

        if ( pointFilter )
        {
            int si, ti;

            out->lmapIndex         = tri->lightmapIndex;
            out->primaryLightIndex = tri->primaryLightIndex;

            si = ( int )( float )floor( s );

            if ( si >= LMAP_WIDTH_MIN - 1 )
                si = LMAP_WIDTH_MIN - 1;
            else if ( si <= 0 )
                si = 0;

            out->s = si;

            ti = ( int )( float )floor( t );

            if ( ti >= LMAP_HEIGHT_MIN - 1 )
                ti = LMAP_HEIGHT_MIN - 1;
            else if ( ti <= 0 )
                ti = 0;

            out->t      = ti;
            out->weight = coverage;
            out->sample = Lighting_Sample( out->lmapIndex, si, ti );

            if ( 0.0f < out->sample->areaX2 )
            {
                out++;
            }

            continue;
        }

        {
            TransportHit_t *first = out;
            int             si    = ( int )( float )floor( s );
            int             ti    = ( int )( float )floor( t );
            float           sWeight[2];
            float           tWeight[2];
            float           total = 0.0f;
            int             row;

            sWeight[1] = s - si;
            sWeight[0] = 1.0f - sWeight[1];

            tWeight[1] = t - ti;
            tWeight[0] = 1.0f - tWeight[1];

            for ( row = 0; row < 2; row++, ti++ )
            {
                int col;

                if ( ti == LMAP_HEIGHT_MIN || tWeight[row] == 0.0f )
                    continue;

                Assertx( ti < LMAP_HEIGHT_MIN,
                         "t doesn't index LMAP_HEIGHT_MIN\n\t%i not in [0, %i)",
                         ti, LMAP_HEIGHT_MIN );

                for ( col = 0; col < 2; col++ )
                {
                    int colS = si + col;

                    if ( colS == LMAP_WIDTH_MIN || sWeight[col] == 0.0f )
                        continue;

                    Assertx( colS < LMAP_WIDTH_MIN,
                             "s doesn't index LMAP_WIDTH_MIN\n\t%i not in [0, %i)",
                             colS, LMAP_WIDTH_MIN );

                    out->lmapIndex         = hit->tri->lightmapIndex;
                    out->primaryLightIndex = hit->tri->primaryLightIndex;
                    out->s                 = colS;
                    out->t                 = ti;
                    out->weight            = tWeight[row] * sWeight[col];
                    out->sample            = Lighting_Sample( out->lmapIndex, colS, ti );

                    if ( 0.0f < out->sample->areaX2 )
                    {
                        total += out->weight;
                        out++;
                    }
                }
            }

            if ( first != out && total * coverage < TRANSPORT_WEIGHT_EPSILON )
            {
                float           scale = coverage / total;
                TransportHit_t *fix;

                for ( fix = first; fix != out; fix++ )
                    fix->weight = fix->weight * scale;
            }
        }
    }

    if ( out == hits )
        return TRANSPORT_MISS;

    *hitCount = out - hits;

    return TRANSPORT_HIT;
}

/* Compile_SampleJitterRadius  0x00407ad0 */
static float Compile_SampleJitterRadius( int index )
{
    float horizon;
    float nearest;
    int   i;

    horizon = I_sqrt( radiositySamples[index].dir[0] * radiositySamples[index].dir[0]
                    + radiositySamples[index].dir[1] * radiositySamples[index].dir[1] );

    nearest = ( float )( TRANSPORT_WEIGHT_EPSILON - horizon );
    nearest = nearest * nearest;

    for ( i = 0; i < options.radiosityTraceCount; i++ )
    {
        float distSq;

        if ( i == index )
            continue;

        distSq = ( float )( Vec2DistanceSq( radiositySamples[index].dir,
                                            radiositySamples[i].dir ) * 0.25 );

        if ( distSq < nearest )
            nearest = distSq;
    }

    return I_sqrt( nearest );
}

/* Compile_AllocRadiositySamples  0x00407b90 */
void Compile_AllocRadiositySamples( void )
{
    int i;

    radiositySamples = new ( std::nothrow ) RadiositySample_t[options.radiosityTraceCount];

    if ( !radiositySamples )
        Error( "Out of memory allocating %i radiosity samples\n",
               options.radiosityTraceCount );

    SpreadPointsOnHemisphere( options.radiosityTraceCount,
                              ( vec3_t * )radiositySamples,
                              sizeof( RadiositySample_t ) );

    if ( options.radiosityTraceCount == 1 )
    {
        radiositySamples[0].dir[0] = 0.0f;
        radiositySamples[0].dir[1] = 0.0f;
    }

    for ( i = 0; i < options.radiosityTraceCount; i++ )
        radiositySamples[i].jitterRadius = Compile_SampleJitterRadius( i ) * options.jitter;

    radiositySampleScale = ( float )( 2.0 / options.radiosityTraceCount );
}

/* Compile_AddIncidentLight  0x004075d0 */
static void Compile_AddIncidentLight( int threadIndex, int influenceType,
                                      const vec3_t lightDir, float weight,
                                      float primaryVisibility,
                                      LmapSubSample_t *subSample,
                                      const vec3_t reflected, const vec3_t incident,
                                      const vec3_t *axis )
{
    SampleVars_t *sampleVars = subSample->sample->vars;

    sampleVars->color[0] += reflected[0] * weight;
    sampleVars->color[1] += reflected[1] * weight;
    sampleVars->color[2] += reflected[2] * weight;

    sampleVars->unscatteredIncidentLight[compilePingPongDest][0] += incident[0] * weight;
    sampleVars->unscatteredIncidentLight[compilePingPongDest][1] += incident[1] * weight;
    sampleVars->unscatteredIncidentLight[compilePingPongDest][2] += incident[2] * weight;

    Assertx( !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][0] )
             && !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][1] )
             && !IS_NAN( sampleVars->unscatteredIncidentLight[compilePingPongDest][2] ),
             "!IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[0])"
             " && !IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[1])"
             " && !IS_NAN((sampleVars->unscatteredIncidentLight[compileGlob.pingPongDest])[2])" );

    if ( influenceType == LIGHT_INFLUENCE_PRIMARY )
    {
        sampleVars->subValue[subSample->fracS + subSample->fracT * 2] += primaryVisibility;
        return;
    }

    if ( influenceType == LIGHT_INFLUENCE_DIRECTIONAL )
    {
        vec3_t localDir;

        localDir[0] = axis[0][0] * lightDir[0] + axis[0][1] * lightDir[1]
                    + axis[0][2] * lightDir[2];
        localDir[1] = axis[1][0] * lightDir[0] + axis[1][1] * lightDir[1]
                    + axis[1][2] * lightDir[2];
        localDir[2] = axis[2][0] * lightDir[0] + axis[2][1] * lightDir[1]
                    + axis[2][2] * lightDir[2];

        Compile_AddSampleLight( sampleVars, threadIndex, reflected, localDir );
        return;
    }

    Assertx( influenceType == LIGHT_INFLUENCE_COINCIDENT,
             "influenceType == LIGHT_INFLUENCE_COINCIDENT" );

    sampleVars->coincident[0] += reflected[0];
    sampleVars->coincident[1] += reflected[1];
    sampleVars->coincident[2] += reflected[2];
}

/* Compile_AddSunLight  0x00407770 */
static void Compile_AddSunLight( LmapSubSample_t *subSample, const vec3_t *axis,
                                 const vec3_t pos, int threadIndex, int lightIndex,
                                 const vec3_t axis0, const vec3_t axis1,
                                 float scale, float subAreaX2 )
{
    GeoTrace_t      trace;
    vec3_t          start;
    vec3_t          end;
    const GeoHit_t *last;
    float           facing;
    float           coverage;
    float           visibility;
    vec3_t          reflected;
    vec3_t          incident;
    int             influenceType;

    Assertx( subSample, "subSample" );
    Assertx( subSample->sample, "subSample->sample" );
    Assertx( subSample->sample->vars, "subSample->sample->vars" );
    Assertx( subAreaX2 > 0, "(subAreaX2 > 0)" );

    facing = axis[2][0] * options.sunDirection[0]
           + axis[2][1] * options.sunDirection[1]
           + axis[2][2] * options.sunDirection[2];

    if ( !( 0.0f < facing ) )
        return;

    start[0] = axis[2][0] * TRANSPORT_START_OFFSET + pos[0];
    start[1] = axis[2][1] * TRANSPORT_START_OFFSET + pos[1];
    start[2] = axis[2][2] * TRANSPORT_START_OFFSET + pos[2];

    end[0] = options.sunDirection[0] * SUN_TRACE_DISTANCE + start[0];
    end[1] = options.sunDirection[1] * SUN_TRACE_DISTANCE + start[1];
    end[2] = options.sunDirection[2] * SUN_TRACE_DISTANCE + start[2];

    Geo_SetupTrace( start, end, axis0, axis1, &trace );
    Geo_TraceRay( &trace );

    if ( !trace.result.hitCount )
        return;

    last = &trace.result.hits[trace.result.hitCount - 1];

    if ( !( last->tri->mskMtl->material->surfaceFlags & SURF_SKY ) )
        return;

    Model_TraceLine( &trace );

    Assertx( trace.result.hitCount, "trace.out.uniqueHitCount" );

    last = &trace.result.hits[trace.result.hitCount - 1];

    if ( last->geoType == TRACE_HIT_MODEL_GEO )
    {
        if ( !last->tri )
            return;

        coverage = SUN_MODEL_COVERAGE;
    }
    else
    {
        if ( !( last->tri->mskMtl->material->surfaceFlags & SURF_SKY ) )
            return;

        coverage = Geo_SampleMaskFraction( last->alphaMask );
    }

    visibility = coverage * scale;

    influenceType = ( lightIndex != options.sunPrimaryLightIndex ) + 1;

    reflected[0] = options.sunColor[0] * visibility;
    reflected[1] = options.sunColor[1] * visibility;
    reflected[2] = options.sunColor[2] * visibility;

    incident[0] = options.sunDiffuseColor[0] * visibility;
    incident[1] = options.sunDiffuseColor[1] * visibility;
    incident[2] = options.sunDiffuseColor[2] * visibility;

    Compile_AddIncidentLight( threadIndex, influenceType, options.sunDirection,
                              facing, coverage * subAreaX2, subSample,
                              reflected, incident, axis );
}

/* Compile_AddPointLight  0x00407a40 */
static void Compile_AddPointLight( const vec3_t *axis, int threadIndex, int lightType,
                                   int lightSlot, const vec3_t pos,
                                   const vec3_t axis0, const vec3_t axis1,
                                   float scale, float primaryVisibility,
                                   LmapSubSample_t *subSample )
{
    vec3_t lightDir;
    vec3_t color;
    vec3_t scaled;
    float  weight;
    int    influenceType;

    influenceType = PointLight_Sample( lightType, lightSlot - 2, pos, axis0, axis1,
                                       axis[2], lightDir, color, &weight );

    if ( !influenceType )
        return;

    scaled[0] = color[0] * scale;
    scaled[1] = color[1] * scale;
    scaled[2] = color[2] * scale;

    Compile_AddIncidentLight( threadIndex, influenceType, lightDir, weight,
                              primaryVisibility, subSample, scaled, scaled, axis );
}

/* Compile_BounceSample  0x00407c50 */
static void Compile_BounceSample( LmapDef_t *toSample, int threadIndex )
{
    SampleVars_t *sampleVars;
    float        *gatheredIncidentLight;
    float         energy;
    unsigned      i;

    Assertx( toSample, "toSample" );
    Assertx( toSample->vars, "toSample->vars" );

    sampleVars = toSample->vars;

    gatheredIncidentLight = sampleVars->unscatteredIncidentLight[compilePingPongDest];

    gatheredIncidentLight[0] = 0.0f;
    gatheredIncidentLight[1] = 0.0f;
    gatheredIncidentLight[2] = 0.0f;

    for ( i = 0; i < ( unsigned )sampleVars->bounceCount; i++ )
    {
        const BounceRef_t  *bounce = &sampleVars->bounces[i];
        const SampleVars_t *fromVars;
        vec3_t              transferredEnergy;
        float               weight;

        weight = radiositySamples[LMAP_PACKED_DIR( bounce->packed )].dir[2];

        fromVars = Lighting_PackedSampleVars( bounce->packed );

        transferredEnergy[0] = fromVars->unscatteredIncidentLight[compilePingPongSrc][0]
                             * fromVars->reflectance[0];
        transferredEnergy[1] = fromVars->unscatteredIncidentLight[compilePingPongSrc][1]
                             * fromVars->reflectance[1];
        transferredEnergy[2] = fromVars->unscatteredIncidentLight[compilePingPongSrc][2]
                             * fromVars->reflectance[2];

        weight = bounce->weight * weight;

        gatheredIncidentLight[0] += transferredEnergy[0] * weight;
        gatheredIncidentLight[1] += transferredEnergy[1] * weight;
        gatheredIncidentLight[2] += transferredEnergy[2] * weight;
    }

    Assertx( !IS_NAN( gatheredIncidentLight[0] ) && !IS_NAN( gatheredIncidentLight[1] )
             && !IS_NAN( gatheredIncidentLight[2] ),
             "!IS_NAN((gatheredIncidentLight)[0]) && !IS_NAN((gatheredIncidentLight)[1])"
             " && !IS_NAN((gatheredIncidentLight)[2])" );

    sampleVars->color[0] += gatheredIncidentLight[0];
    sampleVars->color[1] += gatheredIncidentLight[1];
    sampleVars->color[2] += gatheredIncidentLight[2];

    energy = ( float )( ( gatheredIncidentLight[0] + gatheredIncidentLight[1]
                          + gatheredIncidentLight[2] ) * BOUNCE_ENERGY_SCALE );

    if ( energy > compileBounceEnergy[threadIndex] )
        compileBounceEnergy[threadIndex] = energy;
}

/* Compile_RunBounce  0x00407e20 */
static float Compile_RunBounce( int threads )
{
    int i;

    compilePingPongSrc  = compilePingPongDest;
    compilePingPongDest = 1 - compilePingPongDest;

    for ( i = 0; i < THREAD_COUNT_MAX; i++ )
        compileBounceEnergy[i] = 0.0f;

    Lighting_ForEachSample( Compile_BounceSample, threads );

    for ( i = 1; i < threads; i++ )
        if ( compileBounceEnergy[0] < compileBounceEnergy[i] )
            compileBounceEnergy[0] = compileBounceEnergy[i];

    return compileBounceEnergy[0];
}

/* Picks the jittered direction of radiosity trace index about the surface basis */
static void Compile_RadiosityDir( int index, const vec3_t *axis, vec3_t dir )
{
    const RadiositySample_t *sample = &radiositySamples[index];
    vec3_t local;
    float  u, v;
    float  lengthSq;

    do
    {
        u = rand() * RADIOSITY_JITTER_SCALE - 1.0f;
        v = rand() * RADIOSITY_JITTER_SCALE - 1.0f;

        lengthSq = u * u + v * v;
    }
    while ( 1.0f < lengthSq );

    local[0] = u * sample->jitterRadius + sample->dir[0];
    local[1] = v * sample->jitterRadius + sample->dir[1];

    lengthSq = local[0] * local[0] + local[1] * local[1];

    if ( lengthSq > 1.0f )
    {
        Vec2Normalize( local );
        local[2] = RADIOSITY_MIN_ELEVATION;
    }
    else
    {
        float elevationSq = 1.0f - lengthSq;

        local[2] = ( float )sqrt( elevationSq );
    }

    dir[0] = axis[0][0] * local[0];
    dir[1] = axis[0][1] * local[0];
    dir[2] = local[0] * axis[0][2];

    dir[0] = axis[1][0] * local[1] + dir[0];
    dir[1] = axis[1][1] * local[1] + dir[1];
    dir[2] = local[1] * axis[1][2] + dir[2];

    dir[0] = axis[2][0] * local[2] + dir[0];
    dir[1] = axis[2][1] * local[2] + dir[1];
    dir[2] = local[2] * axis[2][2] + dir[2];
}

/* Compile_TraceRadiositySample  0x00408210 */
static int Compile_TraceRadiositySample( int index, const vec3_t pos,
                                         TransportHit_t *hits, int *hitCount,
                                         const vec3_t *axis )
{
    vec3_t dir;

    Compile_RadiosityDir( index, axis, dir );

    return Compile_TraceTransport( pos, dir, RADIOSITY_TRACE_DISTANCE,
                                   options.traceFilterWidth == TRACE_FILTER_POINT,
                                   hits, hitCount, NULL, NULL );
}

/* ---- GPU path ---------------------------------------------------------- */

/* Per thread: when set, Compile_TraceSubSample uses these traced results
   instead of tracing on the CPU. */
static const GpuHit_t *compileGpuHits[THREAD_COUNT_MAX];

/* Seconds spent per thread in the stages of Compile_TraceSubSample on the GPU path */
static double compileGpuProfile[THREAD_COUNT_MAX][4];

void Compile_PrintGpuProfile( int threads )
{
    double total[4] = { 0.0, 0.0, 0.0, 0.0 };
    int    i;
    int    j;

    for ( i = 0; i < threads; i++ )
        for ( j = 0; j < 4; j++ )
            total[j] += compileGpuProfile[i][j];

    Print( "GPU: CPU time over all threads: radiosity hit lookup %.1fs, bounce records %.1fs,"
           " sun %.1fs, point lights %.1fs\n",
           total[0], total[1], total[2], total[3] );

    memset( compileGpuProfile, 0, sizeof( compileGpuProfile ) );
}

/* Compile_GpuUploadDirections: the GPU jitters its rays around the same hemisphere
   points the CPU path uses */
bool Compile_GpuUploadDirections( void )
{
    float dirX[RAD_TRACE_COUNT_MAX];
    float dirY[RAD_TRACE_COUNT_MAX];
    float jitter[RAD_TRACE_COUNT_MAX];
    int   i;

    for ( i = 0; i < options.radiosityTraceCount; i++ )
    {
        dirX[i]   = radiositySamples[i].dir[0];
        dirY[i]   = radiositySamples[i].dir[1];
        jitter[i] = radiositySamples[i].jitterRadius;
    }

    return GpuTrace_SetDirections( dirX, dirY, jitter, options.radiosityTraceCount ) != 0;
}

/* Compile_GpuNextSeed: every job needs its own jitter seed */
unsigned Compile_GpuNextSeed( void )
{
    static volatile LONG counter;

    return ( unsigned )InterlockedIncrement( &counter );
}

/* Compile_SetGpuTrace */
void Compile_SetGpuTrace( int threadIndex, const GpuHit_t *hits )
{
    compileGpuHits[threadIndex] = hits;
}

/* Compile_ResolveGpuHit */
static int Compile_ResolveGpuHit( const vec3_t pos, const GpuHit_t *gpuHit,
                                  TransportHit_t *hits, int *hitCount )
{
    GeoTrace_t trace;
    int        flags = 0;

    trace.result.hitCount = 0;
    trace.result.frac     = 1.0f;

    if ( gpuHit->tri >= 0 )
    {
        GeoHit_t *hit = &trace.result.hits[0];

        flags = gpuHit->tri & ~GPUHIT_TRI_MASK;

        hit->geoType   = TRACE_HIT_WORLD_GEO;
        hit->tri       = &geoTris[gpuHit->tri & GPUHIT_TRI_MASK];
        hit->u         = gpuHit->u;
        hit->v         = gpuHit->v;
        hit->frac      = gpuHit->frac;
        hit->alphaMask = ( 1 << options.supersampleAlphaCount ) - 1;

        trace.result.hitCount = 1;
        trace.result.frac     = gpuHit->frac;
    }

    /* The start and end points are only needed to report a hit position, which
       radiosity traces never ask for */
    return Compile_ResolveTransport( trace, pos, pos, NULL, flags,
                                     options.traceFilterWidth == TRACE_FILTER_POINT,
                                     hits, hitCount, NULL, NULL );
}

/* Compile_MergeBounces: adds all the bounce hits of one sub-sample to a sample in a
   single pass.  The sample's list is kept sorted by key, so finding the entries that
   already exist is a merge instead of a search per hit. */
static void Compile_MergeBounces( int threadIndex, LmapDef_t *toSample,
                                  BounceRef_t *pending, int pendingCount )
{
    SampleVars_t *vars = toSample->vars;
    BounceRef_t  *list;
    int           existing;
    int           newCount;
    int           e;
    int           i;
    int           j;
    int           k;

    /* The hits arrive almost sorted already, so an insertion sort is enough */
    for ( i = 1; i < pendingCount; i++ )
    {
        BounceRef_t item = pending[i];

        for ( j = i - 1; j >= 0 && pending[j].packed > item.packed; j-- )
            pending[j + 1] = pending[j];

        pending[j + 1] = item;
    }

    existing = vars->bounceCount;
    list     = vars->bounces;
    newCount = 0;
    e        = 0;

    /* Hits that already have an entry add to it; the rest are kept in pending */
    for ( i = 0; i < pendingCount; i++ )
    {
        while ( e < existing && list[e].packed < pending[i].packed )
            e++;

        if ( e < existing && list[e].packed == pending[i].packed )
            list[e].weight += pending[i].weight;
        else if ( newCount && pending[newCount - 1].packed == pending[i].packed )
            pending[newCount - 1].weight += pending[i].weight;
        else
            pending[newCount++] = pending[i];
    }

    if ( !newCount )
        return;

    vars->bounces = ( BounceRef_t * )
        BlockAlloc_Realloc( &compileGlob.bounceAlloc[threadIndex], vars->bounces,
                            existing, newCount );
    list = vars->bounces;

    /* Merge the new entries in from the back, keeping the list sorted */
    i = existing - 1;
    j = newCount - 1;
    k = existing + newCount - 1;

    while ( j >= 0 )
    {
        if ( i >= 0 && list[i].packed > pending[j].packed )
            list[k--] = list[i--];
        else
            list[k--] = pending[j--];
    }

    vars->bounceCount = existing + newCount;
}

/* Compile_AddBounce  0x00406f90 */
static void Compile_AddBounce( int threadIndex, LmapDef_t *toSample, float weight,
                               int dirIndex, const TransportHit_t *from )
{
    SampleVars_t *sampleVars;
    unsigned      packed;
    unsigned      i;

    Assertx( weight >= -1.0e-5f, "(weight >= -1.0e-5f)\n\t(weight) = %g", weight );

    if ( !( 0.0f < weight ) )
        return;

    packed = ( ( ( ( ( unsigned )dirIndex << 5 ) | ( from->lmapIndex & LIGHTMAP_NONE ) )
                 << 9 ) | ( from->t & 0x1ff ) ) << 9 | ( from->s & 0x1ff );

    sampleVars = toSample->vars;

    for ( i = 0; i < ( unsigned )sampleVars->bounceCount; i++ )
    {
        if ( sampleVars->bounces[i].packed == packed )
        {
            sampleVars->bounces[i].weight += weight;
            return;
        }
    }

    sampleVars->bounces = ( BounceRef_t * )
        BlockAlloc_Realloc( &compileGlob.bounceAlloc[threadIndex], sampleVars->bounces,
                            sampleVars->bounceCount, 1 );

    sampleVars->bounces[sampleVars->bounceCount].packed = packed;
    sampleVars->bounces[sampleVars->bounceCount].weight = weight;

    sampleVars->bounceCount++;
}

/* Compile_TraceSubSample  0x004083a0 */
qboolean Compile_TraceSubSample( int threadIndex, int lightType, const vec3_t pos,
                                 const vec3_t axis0, const vec3_t axis1,
                                 const vec3_t *axis, float scale, float subAreaX2,
                                 LmapSubSample_t *subSample )
{
    int           results[RAD_TRACE_COUNT_MAX];
    int           hitCounts[RAD_TRACE_COUNT_MAX];
    SampleVars_t *sampleVars;
    qboolean      anyGround;
    qboolean      allBlocked;
    float         traceScale;
    int           i;
    int           slot;

    BounceRef_t pending[RAD_TRACE_COUNT_MAX * 4];   /* GPU path only */
    int         pendingCount = 0;
    double prof0 = 0.0;
    double prof1 = 0.0;
    double *prof = compileGpuHits[threadIndex] ? compileGpuProfile[threadIndex] : NULL;

    sampleVars = subSample->sample->vars;

    Assertx( sampleVars, "sampleVars" );

    if ( prof )
        prof0 = GpuTrace_Seconds();

    if ( options.relight )
    {
        Lock( sampleVars );
    }
    else
    {
        anyGround  = qfalse;
        allBlocked = qtrue;

        for ( i = 0; i < options.radiosityTraceCount; i++ )
        {
            if ( compileGpuHits[threadIndex] )
                results[i] = Compile_ResolveGpuHit( pos, &compileGpuHits[threadIndex][i],
                                                    compileTraceHits[threadIndex][i],
                                                    &hitCounts[i] );
            else
                results[i] = Compile_TraceRadiositySample( i, pos, compileTraceHits[threadIndex][i],
                                                           &hitCounts[i], axis );

            if ( results[i] == TRANSPORT_GROUND )
                anyGround = qtrue;
            else if ( results[i] > TRANSPORT_BACKFACE )
                allBlocked = qfalse;
        }

        if ( allBlocked )
            return qfalse;

        if ( anyGround )
        {
            vec3_t probe;

            probe[0] = axis[2][0] * GROUND_PROBE_OFFSET + pos[0];
            probe[1] = axis[2][1] * GROUND_PROBE_OFFSET + pos[1];
            probe[2] = axis[2][2] * GROUND_PROBE_OFFSET + pos[2];

            if ( Geo_PointIsCoveredOverSky( probe ) )
                return qfalse;
        }

        Lock( sampleVars );

        if ( prof )
        {
            prof1 = GpuTrace_Seconds();
            prof[0] += prof1 - prof0;
            prof0 = prof1;
        }

        traceScale = radiositySampleScale * scale;

        for ( i = 0; i < options.radiosityTraceCount; i++ )
        {
            if ( results[i] == TRANSPORT_SKY )
            {
                sampleVars->skyInfluence[i] += traceScale;
            }
            else if ( results[i] == TRANSPORT_HIT )
            {
                int j;

                for ( j = 0; j < hitCounts[i]; j++ )
                {
                    const TransportHit_t *hit = &compileTraceHits[threadIndex][i][j];

                    if ( prof )
                    {
                        float weight = hit->weight * traceScale;

                        if ( 0.0f < weight )
                        {
                            pending[pendingCount].weight = weight;
                            pending[pendingCount].packed =
                                ( ( ( ( ( ( unsigned )i << 5 ) | ( hit->lmapIndex & LIGHTMAP_NONE ) )
                                       << 9 ) | ( hit->t & 0x1ff ) ) << 9 ) | ( hit->s & 0x1ff );
                            pendingCount++;
                        }
                    }
                    else
                    {
                        Compile_AddBounce( threadIndex, subSample->sample,
                                           hit->weight * traceScale, i, hit );
                    }
                }
            }
        }

        if ( prof )
            Compile_MergeBounces( threadIndex, subSample->sample, pending, pendingCount );
    }

    if ( prof )
    {
        prof1 = GpuTrace_Seconds();
        prof[1] += prof1 - prof0;
        prof0 = prof1;
    }

    sampleVars->subMask |= 1 << ( subSample->fracS + subSample->fracT * 2 );

    Compile_AddSunLight( subSample, axis, pos, threadIndex, lightType,
                         axis0, axis1, scale, subAreaX2 );

    if ( prof )
    {
        prof1 = GpuTrace_Seconds();
        prof[2] += prof1 - prof0;
        prof0 = prof1;
    }

    for ( slot = 2; slot < compileLightSlots; slot++ )
        Compile_AddPointLight( axis, threadIndex, lightType, slot, pos,
                               axis0, axis1, scale, subAreaX2, subSample );

    if ( prof )
        prof[3] += GpuTrace_Seconds() - prof0;

    Unlock( sampleVars );

    return qtrue;
}

/* Compile_RunBounces  0x00407f50 */
static void Compile_RunBounces( int threads, float threshold )
{
    float firstEnergy = FLT_MAX;
    int   bounce      = 0;

    for ( ;; )
    {
        float energy;

        bounce++;

        StartProgress( va( "Radiosity bounce %i...", bounce ) );
        energy = Compile_RunBounce( threads );
        EndProgress();

        if ( bounce == 1 )
        {
            firstEnergy = energy;
        }
        else if ( firstEnergy + firstEnergy < energy )
        {
            Print( "\n\nAborting radiosity due to a positive feedback loop.\n" );
            Print( "This can usually be fixed by changing '-traces' slightly"
                   " (currently %i).\n", options.radiosityTraceCount );
            Print( "Reducing the radiosity scale can also help this (currently %g).\n",
                   options.radiosityScale );
            return;
        }

        if ( !( threshold < energy ) )
            return;

        if ( bounce + 1 == options.maxBounceCount )
            return;
    }
}


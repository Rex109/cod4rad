/* Original: c:\trees\cod3\cod3src\cod2rad\compile.cpp */

#ifndef COMPILE_H
#define COMPILE_H

#include "q_shared.h"
#include "com_vector.h"
#include "threads.h"
#include "pointlights.h"
#include "cmdline.h"


typedef struct
{
    vec3_t dir;             /* +0x00 */
    float  jitterRadius;    /* +0x0c */
} RadiositySample_t;        /* sizeof == 0x10 */


#define BASIS_WEIGHT_COUNT      4
#define BASIS_HORIZONTAL_COUNT  3

#define BASIS_TILT_ANGLE        0.9553166031837463

#define BASIS_SECTORS_PER_TURN  0.47746482491493225


typedef struct BlockAllocatorFreeList_s
{
    unsigned                         count;  /* +0x00 */
    struct BlockAllocatorFreeList_s *next;   /* +0x04 */
} BlockAllocatorFreeList;   /* sizeof == 0x08 */


typedef struct
{
    int                       blockSize;         /* +0x00 */
    unsigned                  granularityMask;   /* +0x04 */
    int                       granularityShift;  /* +0x08 */
    unsigned                  smallListCount;    /* +0x0c */
    BlockAllocatorFreeList   *largeList;         /* +0x10 */
    BlockAllocatorFreeList  **smallLists;        /* +0x14 */
    unsigned                  poolTotal;         /* +0x18 */
    unsigned                  poolUsed;          /* +0x1c */
    byte                     *pool;              /* +0x20 */
} BlockAllocator_t;


typedef struct
{
    int              lmapIndex;         /* +0x00 */
    int              primaryLightIndex; /* +0x04 */
    int              s;                 /* +0x08 */
    int              t;                 /* +0x0c */
    float            weight;            /* +0x10 */
    struct LmapDef_s *sample;           /* +0x14 */
} TransportHit_t;                       /* sizeof == 0x18 */


#define TRANSPORT_MISS          0
#define TRANSPORT_GROUND        1
#define TRANSPORT_BACKFACE      2
#define TRANSPORT_SKY           3
#define TRANSPORT_HIT           4

#define TRANSPORT_START_OFFSET  0.125

#define TRANSPORT_MAX_HITS      124

#define GROUND_PROBE_OFFSET     0.10000000149011612

#define RAND_TO_UNIT_SPAN       6.103701889514923e-05

#define JITTER_MIN_Z            0.001f

#define BOUNCE_ENERGY_SCALE     0.3333333432674408

#define SUN_TRACE_DISTANCE      262144.0

#define SUN_MODEL_COVERAGE      0.01f

#define TRANSPORT_WEIGHT_EPSILON    0.9990000128746033


typedef struct
{
    BlockAllocator_t bounceAlloc[THREAD_COUNT_MAX];     /* 0x1162302c */
    BlockAllocator_t lightAlloc[THREAD_COUNT_MAX];      /* 0x116230bc */
} compileGlob_t;


extern compileGlob_t compileGlob;               /* 0x1162302c */

extern int   compileLightSlots;                 /* 0x11623010 */

extern float compileBounceEnergy[THREAD_COUNT_MAX];     /* 0x11623014 */

extern int compilePingPongSrc;                  /* 0x1162314c */
extern int compilePingPongDest;                 /* 0x11623150 */

extern TransportHit_t compileTraceHits[THREAD_COUNT_MAX][RAD_TRACE_COUNT_MAX]
                                      [TRANSPORT_MAX_HITS];    /* 0x11623158 */

extern RadiositySample_t *radiositySamples;     /* 0x11623028 */

extern float radiositySampleScale;              /* 0x11623024 */


void  BlockAlloc_Init( BlockAllocator_t *blockAlloc, int blockSize,
                       int blockGranularity, unsigned poolCount,
                       unsigned smallListCount );           /* 0x004065a0 */
void *BlockAlloc_Alloc( BlockAllocator_t *blockAlloc, unsigned count );  /* 0x00406730 */
void *BlockAlloc_Realloc( BlockAllocator_t *blockAlloc, void *block,
                          unsigned count, unsigned extraCount ); /* 0x004068a0 */

void GatherSampleToBasis( const struct SampleVars_s *vars, vec3_t *basis );  /* 0x00406a80 */

void Compile_SeedSkyLight( struct SampleVars_s *sampleVars );   /* 0x00406ba0 */

int Compile_TraceTransport( const vec3_t start, const vec3_t dir, float distance,
                            qboolean pointFilter, TransportHit_t *hits,
                            int *hitCount, vec3_t hitPos,
                            vec3_t hitNormal );                 /* 0x00407080 */

void Compile_AllocRadiositySamples( void );     /* 0x00407b90 */
void Compile_InitAllocators( int threads );     /* 0x00408000 */

qboolean Compile_TraceSubSample( int threadIndex, int lightType, const vec3_t pos,
                                 const vec3_t axis0, const vec3_t axis1,
                                 const vec3_t *axis, float scale, float subAreaX2,
                                 struct LmapSubSample_s *subSample );   /* 0x004083a0 */

void RunLightCompile( int threadCount );        /* 0x004080b0 */

#endif

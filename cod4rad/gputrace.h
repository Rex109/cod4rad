/* GPU ray tracing backend (Direct3D 11 compute) */

#ifndef GPUTRACE_H
#define GPUTRACE_H

/* Plain-old-data scene description.  This header intentionally does not pull
   in any game headers so the D3D11 code stays isolated. */

typedef struct
{
    float normal[3];
    float dist;
    int   child[2];
} GpuNode_t;                /* 24 bytes, same layout as GeoBspNode_t */

typedef struct
{
    int triCount;
    int firstTriRef;
} GpuLeaf_t;

typedef struct
{
    float v0[3]; float nx;
    float v1[3]; float ny;
    float v2[3]; float nz;
    float st0[2];
    float st1[2];
    float st2[2];
    int   mtl;              /* index into mtls, or -1 when the triangle is opaque */
    int   pad;
} GpuTri_t;                 /* 80 bytes */

typedef struct
{
    unsigned offset;        /* first 32-bit word of the mask in the mask buffer */
    unsigned width;         /* power of two */
    unsigned height;        /* power of two */
    unsigned pad;
} GpuMtl_t;

typedef struct
{
    const GpuNode_t  *nodes;     int nodeCount;
    const GpuLeaf_t  *leafs;     int leafCount;
    const int        *triRefs;   int triRefCount;
    const GpuTri_t   *tris;      int triCount;
    const GpuMtl_t   *mtls;      int mtlCount;
    const unsigned   *masks;     int maskWordCount;
} GpuScene_t;

/* One lightmap sub-sample: where it is and the surface basis its rays are built around.
   The GPU picks the radiosity ray directions itself (jittered around the directions
   given to GpuTrace_SetDirections), so a job is all that has to be uploaded. */
typedef struct
{
    float    pos[3];
    unsigned seed;          /* different for every job; drives the jitter */
    float    axis0[3]; float pad0;
    float    axis1[3]; float pad1;
    float    axis2[3]; float pad2;   /* the surface normal */
} GpuJob_t;                 /* 64 bytes */

typedef struct
{
    int   tri;              /* -1 for a miss, otherwise the triangle index OR'd with flags below */
    float u;
    float v;
    float frac;             /* 0..1 along the ray */
} GpuHit_t;

#define GPUHIT_TRI_MASK     0x0fffffff
#define GPUHIT_BACKFACE     0x10000000      /* the ray hit the back of the triangle */
#define GPUHIT_DOWNWARD     0x20000000      /* the ray pointed down (dir z < 0) */


/* Returns 0 on failure and writes a message to err. */
int  GpuTrace_Init( const GpuScene_t *scene, char *err, int errSize );
void GpuTrace_Shutdown( void );

const char *GpuTrace_AdapterName( void );

/* The hemisphere directions every job traces: (x, y) of each unit-hemisphere point
   and the jitter radius around it.  Must be called before GpuTrace_TraceJobs. */
int  GpuTrace_SetDirections( const float *dirX, const float *dirY, const float *jitter, int count );

/* Traces directionCount rays for each job and writes jobCount * directionCount hits,
   job-major.  Safe to call from several threads at once.  Returns 0 if the GPU
   failed (e.g. device removed). */
int  GpuTrace_TraceJobs( const GpuJob_t *jobs, int jobCount, GpuHit_t *hits );

/* Monotonic clock in seconds, for the -gpu timing report */
double GpuTrace_Seconds( void );

/* Totals since GpuTrace_Init: rays traced, seconds the GPU path was busy
   (dispatch + readback) and seconds threads spent queued behind it. */
void GpuTrace_Stats( double *rays, double *busySeconds, double *waitSeconds );

#endif

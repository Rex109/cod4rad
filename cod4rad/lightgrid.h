/* Original: c:\trees\cod3\cod3src\cod2rad\lightgrid.cpp */

#ifndef LIGHTGRID_H
#define LIGHTGRID_H

#include "q_shared.h"
#include "com_vector.h"


#define MAX_MAP_LIGHTGRID_POINTS        0x100000

#define LIGHTGRID_SPACING_XY            32
#define LIGHTGRID_SPACING_Z             64
#define LIGHTGRID_BIAS_XY               0x1000
#define LIGHTGRID_BIAS_Z                0x0800

#define LIGHTGRID_WORLD_MIN             ( -131072.0f )
#define LIGHTGRID_MAX_XY                0x2000
#define LIGHTGRID_MAX_Z                 0x1000

#define GFX_LIGHTGRID_SAMPLE_COUNT      56

#define LIGHTGRID_CORNER_OFFSET         0.125f

#define LIGHTGRID_TRACE_DISTANCE        262144.0f

#define LIGHTGRID_RAND_SCALE            ( 1.0f / 32768.0f )

#define LIGHTGRID_MAX_RUN               0xff


typedef struct
{
    unsigned short pos[3];              /* +0x00 */
    byte           needsTrace;          /* +0x06 */
    byte           pad07;               /* +0x07 */
    unsigned short colorsIndex;         /* +0x08 */
    byte           primaryLightIndex;   /* +0x0a */
    byte           cornerMask;          /* +0x0b */
} LightGridPoint_t;                     /* sizeof == 0x0c */

typedef struct
{
    byte rgb[GFX_LIGHTGRID_SAMPLE_COUNT][3];
} LightGridSample_t;                    /* sizeof == 0xa8 */

typedef struct
{
    vec3_t color[GFX_LIGHTGRID_SAMPLE_COUNT];
} LightGridColors_t;

typedef struct
{
    short pos[3];
} LightGridPointFile_t;                 /* sizeof == 0x06 */

typedef struct
{
    int pos[3];                         /* +0x00 */
    int unused[3];                      /* +0x0c */
} LightGridVisCacheEntry_t;             /* sizeof == 0x18 */

typedef struct StaticModelOrigin_s
{
    vec3_t                      origin;     /* +0x00 */
    struct StaticModelOrigin_s *next;       /* +0x0c */
} StaticModelOrigin_t;                      /* sizeof == 0x10 */

typedef struct
{
    int   first;                        /* +0x00 */
    int   count;                        /* +0x04 */
    int   splitAxis;                    /* +0x08 */
    float splitValue;                   /* +0x0c */
    float maxDev;                       /* +0x10 */
} LightGridCluster_t;                   /* sizeof == 0x14 */


typedef struct
{
    int                pointCount;      /* +0x00 */
    int                maxPoints;       /* +0x04 */
    LightGridPoint_t  *points;          /* +0x08 */
    LightGridSample_t *samples;         /* +0x0c */
    int                pad10[3];        /* +0x10 */
    int                staticModelOriginCount;  /* +0x1c */
    StaticModelOrigin_t *staticModelOrigins;    /* +0x20 */
    vec3_t             skyColor;        /* +0x24 */
    float              skyScale;        /* +0x30 */
    int                pad34[2];        /* +0x34 */
} lightGridGlob_t;


extern lightGridGlob_t lightGridGlob;                           /* 0x13063950 */

extern vec3_t              lightGridBasis[GFX_LIGHTGRID_SAMPLE_COUNT]; /* 0x130632b0 */
extern int                 lightGridHullStart[];                /* 0x13063550 */
extern int                 lightGridClusterCount;               /* 0x13063960 */
extern LightGridCluster_t *lightGridClusters;                   /* 0x13063964 */
extern int                *lightGridIndexes;                    /* 0x13063968 */
extern vec3_t             *lightGridSkyTraceDirs;               /* 0x13063984 */
extern int                 lightGridAxisStart[];                /* 0x13063988 */


void LightGrid_AddStaticModelOrigin( const vec3_t origin );     /* 0x0040fc90 */

qboolean LightGrid_HasPointAtOrigin( const vec3_t origin );     /* 0x00410280 */

void LightGrid_Compile( int threads );                          /* 0x00413630 */

#endif

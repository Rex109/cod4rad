/* Original: c:\trees\cod3\cod3src\cod2rad\cmdline.cpp */

#ifndef CMDLINE_H
#define CMDLINE_H

#include "q_shared.h"
#include "com_vector.h"


#define RAD_THREAD_COUNT_MIN        1
#define RAD_THREAD_COUNT_MAX        16

/* Without -gpu the default stays at the original limit of 4 threads */
#define RAD_THREAD_COUNT_DEFAULT_MAX 4

#define RAD_TRACE_COUNT_MIN         16
#define RAD_TRACE_COUNT_MAX         512

#define RAD_BASIS_DIR_COUNT_MIN     16
#define RAD_BASIS_DIR_COUNT_MAX     256

#define RAD_BOUNCE_COUNT_MIN        1
#define RAD_BOUNCE_COUNT_MAX        256

#define RAD_SUPERSAMPLE_MIN         1
#define RAD_SUPERSAMPLE_MAX         8

#define RAD_SUPERSAMPLE_ALPHA_MIN   1
#define RAD_SUPERSAMPLE_ALPHA_MAX   31

#define RAD_WARNING_LEVEL_MIN       0
#define RAD_WARNING_LEVEL_MAX       4

#define RAD_JITTER_MIN              0.0f
#define RAD_JITTER_MAX              1.0f

#define RAD_GAMMA_MIN               0.25f
#define RAD_GAMMA_MAX               4.0f

#define RAD_RADIOSITY_SCALE_MIN     0.0f
#define RAD_RADIOSITY_SCALE_MAX     10.0f

#define RAD_CONTRAST_GAIN_MIN       0.0f
#define RAD_CONTRAST_GAIN_MAX       1.0f

#define PRIMARY_LIGHT_NONE          0
#define PRIMARY_LIGHT_SUN           1

#define TRACE_FILTER_POINT          1
#define TRACE_FILTER_LINEAR         4

#define MAX_MAP_NAME                256


typedef struct
{
    int    threadCount;                 /* +0x000 */
    byte   modelShadow;                 /* +0x004 */
    byte   extraQuality;                /* +0x005 */
    byte   relightSave;                 /* +0x006 */
    byte   relight;                     /* +0x007 */
    int    supersampleAlphaCount;       /* +0x008 */
    int    supersampleCount;            /* +0x00c */
    float  jitter;                      /* +0x010 */
    int    radiosityTraceCount;         /* +0x014 */
    int    skyTraceCount;               /* +0x018 */
    int    traceFilterWidth;            /* +0x01c */
    int    basisDirCount;               /* +0x020 */
    int    lightGridColorLimit;         /* +0x024 */
    float  lightGridColorTolerance;     /* +0x028 */
    vec3_t sunDirection;                /* +0x02c */
    vec3_t sunColor;                    /* +0x038 */
    vec3_t sunDiffuseColor;             /* +0x044 */
    vec3_t sunRadiosityColor;           /* +0x050 */
    vec3_t ambientColor;                /* +0x05c */
    int    sunPrimaryLightIndex;        /* +0x068 */
    int    maxBounceCount;              /* +0x06c */
    float  radiosityScale;              /* +0x070 */
    float  gamma;                       /* +0x074 */
    float  contrastGain;                /* +0x078 */
    byte   radiosityScaleSet;           /* +0x07c */
    byte   contrastGainSet;             /* +0x07d */
    byte   verbose;                     /* +0x07e */
    byte   quiet;                       /* +0x07f */
    int    warningLevel;                /* +0x080 */
    char   mapName[MAX_MAP_NAME];       /* +0x084 */
    char   baseGame[64];                /* +0x184 */
} RadOptions_t;


extern RadOptions_t options;            /* 0x11622e48 */


bool ParseCommandLine( int argc, const char **argv );

#endif

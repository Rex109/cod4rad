/* Original: c:\trees\cod3\cod3src\cod2rad\lighting.cpp */

#ifndef LIGHTING_H
#define LIGHTING_H

#include "q_shared.h"
#include "com_vector.h"
#include "bspfile.h"
#include "threads.h"


#define LIGHTMAP_NONE   0x1f


#define LMAP_WIDTH_MIN          512
#define LMAP_HEIGHT_MIN         512

#define LMAP_WIDTH_MAX          1024
#define LMAP_HEIGHT_MAX         1024

#define LIGHTING_SCALE_HEADROOM 0.99f

#define LIGHTING_MAX_CONTRAST_SAMPLES   0x100

#define LIGHTING_LUMA_R         0.299f
#define LIGHTING_LUMA_G         0.587f
#define LIGHTING_LUMA_B         0.114f

#define LIGHTING_CONTRAST_LIMIT 0.5f


#define LIGHTING_BASIS_COUNT_MIN    0x10
#define LIGHTING_BASIS_COUNT_MAX    0x100

#define LIGHTING_GAMMA_EPSILON  -1.0e-4f


#define LIGHTING_GOLDEN_COS         -0.7373688817024231
#define LIGHTING_GOLDEN_SIN          0.6754903197288513

#define LIGHTMAP_SIZE           LMAP_WIDTH_MIN
#define LIGHTMAP_SAMPLE_COUNT   ( LMAP_WIDTH_MIN * LMAP_HEIGHT_MIN )

#define LMAP_PACKED_S( p )      ( ( p ) & 0x1ff )
#define LMAP_PACKED_T( p )      ( ( ( p ) >> 9 ) & 0x1ff )
#define LMAP_PACKED_INDEX( p )  ( ( ( p ) >> 18 ) & 0x1f )

#define LMAP_PACKED_DIR( p )    ( ( p ) >> 23 )

#ifdef _WIN64
/* 0x600000 in the 32-bit build; an LmapDef_t is bigger with 8 byte pointers */
#define LIGHTMAP_BYTES_PER_MAP  ( ( size_t )LIGHTMAP_SAMPLE_COUNT * sizeof( LmapDef_t ) )
#else
#define LIGHTMAP_BYTES_PER_MAP  0x600000
#endif

#define LIGHTMAP_PLANE_COUNT    3
#define LIGHTMAP_PLANE_BYTES    ( LMAP_WIDTH_MIN * LMAP_HEIGHT_MIN * 4 )
#define LIGHTMAP_BYTES_PER_MAP2 ( LIGHTMAP_PLANE_COUNT * LIGHTMAP_PLANE_BYTES )

#define LIGHTMAP_SUB_ROW_BYTES  LMAP_WIDTH_MAX
#define LIGHTMAP_SUB_BLOCK      4

#define LIGHTMAP_DIR_SCALE      ( 4.0f / 255.0f )
#define LIGHTMAP_DIR_BIAS       2.0f


typedef struct LmapDef_s
{
    struct SampleVars_s *vars;  /* +0x00 */
    float areaX2;           /* +0x04 */
    float subWeight[LIGHTMAP_SUB_BLOCK];    /* +0x08 */
} LmapDef_t;                /* sizeof == 0x18 */


typedef struct
{
    vec3_t color;           /* +0x00 */
    vec3_t dir;             /* +0x0c */
} DirectTransport_t;               /* sizeof == 0x18 */

typedef struct
{
    unsigned packed;        /* +0x00 */
    float    weight;        /* +0x04 */
} BounceRef_t;              /* sizeof == 0x08 */

typedef struct SampleVars_s
{
    float         *skyInfluence;  /* +0x00 */
    float          subValue[LIGHTMAP_SUB_BLOCK];    /* +0x04 */
    vec3_t         reflectance;   /* +0x14 */
    vec3_t         color;         /* +0x20 */
    vec3_t         unscatteredIncidentLight[2];     /* +0x2c, +0x38 */
    vec3_t         coincident;    /* +0x44 */
    byte           subMask;       /* +0x50 */
    byte           pad51;         /* +0x51 */
    unsigned short directTransportCount;    /* +0x52 */
    DirectTransport_t *directTransport;     /* +0x54 */
    int            bounceCount;   /* +0x58 */
    BounceRef_t   *bounces;       /* +0x5c */
} SampleVars_t;                   /* sizeof == 0x60 */


#define RADTRANS_FILE       "radtrans.bin"
#define RADTRANS_VERSION    3


typedef struct SuppressedRange_s
{
    unsigned __int64 key;   /* +0x00 */
    int              count; /* +0x08 */
    int              pad0c; /* +0x0c */
} SuppressedRange_t;        /* sizeof == 0x10 */


typedef struct LmapSubSample_s
{
    LmapDef_t *sample;      /* +0x00 */
    int        fracS;       /* +0x04 */
    int        fracT;       /* +0x08 */
} LmapSubSample_t;          /* sizeof == 0x0c */


typedef void ( *LmapSampleFunc_t )( LmapDef_t *def, int threadIndex );
typedef void ( *LmapPixelFunc_t )( int lmapIndex, int x, int y, int threadIndex );
typedef void ( *LmapMapFunc_t )( LmapDef_t *lmap, int lmapIndex, int threadIndex );


typedef struct
{
    LmapSampleFunc_t sampleFunc;    /* 0x13063d88 */
    LmapPixelFunc_t  pixelFunc;     /* 0x13063d8c */
    LmapMapFunc_t    mapFunc;       /* 0x13063d90 */
    int              lightSlots;    /* 0x13063d94 */
    int              lmapCount;     /* 0x13063d98 */
    LmapDef_t       *lmapDefs;      /* 0x13063d9c */
    int              totalSampleCount;  /* 0x13063da0 */
    int              usefulSampleCount; /* 0x13063da4 */
    SampleVars_t    *sampleVars;        /* 0x13063da8 */
    float           *skyInfluences;     /* 0x13063dac */
    int              basisDirCount;     /* 0x13063db0 */
    vec3_t          *basisDirs;         /* 0x13063db4 */
    int              lastSuppressed[THREAD_COUNT_MAX];  /* 0x13063db8 */
    int              suppressedCount;               /* 0x13063dc8 */
    struct SuppressedRange_s *suppressed;           /* 0x13063dcc */
} lightingGlob_t;


extern lightingGlob_t lightingGlob;     /* 0x13063d88 */

extern byte *const lmapBytes;           /* 0x004b2d00 */
extern byte *const lmapSubBytes;        /* 0x006b2d00 */


void Lighting_UseLightmap( int lmapIndex );     /* 0x00413b60 */
void Lighting_AllocLightmaps( void );           /* 0x00413bd0 */
void Lighting_DispatchSample( int sampleIndex, int threadIndex ); /* 0x00413ed0 */
void Lighting_DispatchPixel( int sampleIndex, int threadIndex );  /* 0x00413f00 */
void Lighting_DispatchMap( int lmapIndex, int threadIndex );      /* 0x00413f60 */

void Lighting_ForEachSample( LmapSampleFunc_t func, int threads ); /* 0x00413f90 */
void Lighting_ForEachPixel( LmapPixelFunc_t func, int threads );   /* 0x00413fc0 */
void Lighting_ForEachMap( LmapMapFunc_t func, int threads );       /* 0x00413ff0 */

void Lighting_AllocSamples( void );                                /* 0x004140a0 */
void Lighting_ScaleSamples( void );                                /* 0x004142f0 */
void Lighting_SampleColor( const SampleVars_t *vars, vec3_t out );  /* 0x00414320 */
void Lighting_AllocBasisDirs( void );                              /* 0x00414370 */
void Lighting_AddToBasis( const vec3_t color, const vec3_t dir,
                          vec3_t *basis );                         /* 0x004144f0 */

float Lighting_GammaToLinear( float color );                       /* 0x00414580 */
void  Lighting_Vec3GammaToLinear( vec3_t color );                  /* 0x004145d0 */
float Lighting_LinearToGamma( float color );                       /* 0x00414610 */
void  Lighting_Vec3LinearToGamma( vec3_t color );                  /* 0x00414680 */
byte  Lighting_ColorToByte( float color );                         /* 0x004146c0 */
void  Lighting_ColorPlanesToBytes( const float *colors, int count,
                                   const vec2_t *ranges, byte *out ); /* 0x00414720 */
float Lighting_ClampedDot( const vec3_t a, const vec3_t b );        /* 0x004147f0 */
float Lighting_BasisFitError( const vec3_t *basis, const vec3_t dir,
                              const vec3_t ambient, const vec3_t direct ); /* 0x00414850 */
qboolean Lighting_ClampColorToUnit( const vec3_t in, vec3_t out );  /* 0x00414940 */
void Lighting_EvalBasisFit( const vec3_t basisDir, const vec3_t dir,
                            const vec3_t ambient, const vec3_t direct,
                            vec3_t out );                          /* 0x00414c90 */
void Lighting_SolveBasisFit( vec3_t ambient, vec3_t direct,
                             const vec3_t *basis, const vec3_t dir ); /* 0x004149d0 */
void Lighting_BasisFitGradient( vec2_t grad, const vec3_t *basis,
                                const vec3_t dir, const vec3_t ambient,
                                const vec3_t direct );                /* 0x00414cf0 */
void Lighting_DominantDirection( const vec3_t *basis, vec3_t dir );   /* 0x00414e70 */
void Lighting_DecodeDirection( int x, int y, vec3_t dir );            /* 0x00413ae0 */
void Lighting_ReadLightmapTexel( int s, int t, int lmapIndex,
                                 vec3_t colorA, vec3_t colorB,
                                 vec3_t dir );                        /* 0x004150b0 */
void Lighting_SampleLightmap( int lmapIndex, int s, int t,
                              const vec3_t normal, vec3_t out );      /* 0x00415160 */
float Lighting_AverageSubSamples( int lmapIndex, int s, int t );      /* 0x00415220 */
void  Lighting_ApplyContrast( int sampleCount, int baseIndex,
                              vec3_t *colors );                      /* 0x00415270 */
byte  Lighting_ColorToGammaByte( float color );                      /* 0x00415770 */
void  Lighting_BuildBasis( const SampleVars_t *vars, vec3_t *basis ); /* 0x004157b0 */
void  Lighting_BuildBasisFromNeighbours( int lmapIndex, int s, int t,
                                         vec3_t *basis );             /* 0x00415990 */
void  Lighting_NormaliseSamples( int threads );                       /* 0x00416020 */
void  Lighting_SaveTransfers( void );                                 /* 0x004160e0 */
qboolean Lighting_LoadTransfers( void );                              /* 0x00416460 */
qboolean Lighting_IsSuppressed( int keyHigh, int keyLow );            /* 0x00416620 */
void     Lighting_Suppress( int threadIndex, int keyHigh, int keyLow );/* 0x004166c0 */
void  Lighting_SeedSkyLight( int threads );                           /* 0x00416090 */
void  Lighting_WriteSamples( int threads );                           /* 0x00416ce0 */
void  Lighting_AllocBleedMasks( void );                               /* 0x00416050 */
void  Lighting_FindBleeding( int threads );                           /* 0x00416060 */

SampleVars_t *Lighting_PackedSampleVars( unsigned packed );     /* 0x00413cb0 */
LmapDef_t *Lighting_Sample( unsigned lmapIndex, unsigned s, unsigned t ); /* 0x00413d10 */
void       Lighting_SetSubSample( unsigned lmapIndex, float s, float t,
                                  LmapSubSample_t *subSample );  /* 0x00413db0 */

#endif

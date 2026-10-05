/* Original: c:\trees\cod3\cod3src\cod2rad\lighting.cpp */

#include "cod4rad.h"
#include "lighting.h"
#include "pointlights.h"
#include "progress.h"
#include "threads.h"
#include "cmdline.h"
#include "compile.h"
#include "lightmap_bleed.h"
#include "geometry.h"
#include "com_crc32.h"
#include "linearmapping.h"

#include <string.h>
#include <new>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>


lightingGlob_t lightingGlob;        /* 0x13063d88 */


/* Lighting_UseLightmap  0x00413b60 */
void Lighting_UseLightmap( int lmapIndex )
{
    Assertx( lmapIndex != LIGHTMAP_NONE, "lmapIndex != LIGHTMAP_NONE" );
    Assertx( lightingGlob.lmapDefs == NULL, "lightingGlob.lmapDefs == NULL" );

    if ( lightingGlob.lmapCount < lmapIndex + 1 )
        lightingGlob.lmapCount = lmapIndex + 1;
}


byte *const lmapBytes    = bspLightBytes;                   /* 0x004b2d00 */
byte *const lmapSubBytes = bspLightBytes
                           + LIGHTMAP_PLANE_BYTES * 2;      /* 0x006b2d00 */


#if defined( _WIN64 ) && !defined( SHOW_LAYOUT_CHECKS )
/* The layouts below describe the 32-bit binary; pointers are twice as big on x64 */
#define LT_CHECK( name, cond )  typedef char name[1]
#else
#define LT_CHECK( name, cond )  typedef char name[( cond ) ? 1 : -1]
#endif

LT_CHECK( lt_def_size,  sizeof( LmapDef_t ) == 24 );
LT_CHECK( lt_def_fits,  LIGHTMAP_BYTES_PER_MAP == LIGHTMAP_SAMPLE_COUNT * sizeof( LmapDef_t ) );
LT_CHECK( lt_glob_lmap, offsetof( lightingGlob_t, lmapCount ) == 0x10 );
LT_CHECK( lt_glob_defs, offsetof( lightingGlob_t, lmapDefs ) == 0x14 );
LT_CHECK( lt_glob_cach, offsetof( lightingGlob_t, lastSuppressed ) == 0x30 );
LT_CHECK( lt_sub_size,  sizeof( LmapSubSample_t ) == 12 );
LT_CHECK( lt_sub_grid,  LMAP_WIDTH_MAX == LMAP_WIDTH_MIN * 2 );
LT_CHECK( lt_vars_size, sizeof( SampleVars_t ) == 96 );
LT_CHECK( lt_vars_refl,  offsetof( SampleVars_t, reflectance ) == 0x14 );
LT_CHECK( lt_vars_color, offsetof( SampleVars_t, color ) == 0x20 );
LT_CHECK( lt_vars_lights, offsetof( SampleVars_t, directTransport ) == 0x54 );
LT_CHECK( lt_vars_bounce, offsetof( SampleVars_t, bounces ) == 0x5c );
LT_CHECK( lt_vars_coinc,  offsetof( SampleVars_t, coincident ) == 0x44 );
LT_CHECK( lt_lightref,   sizeof( DirectTransport_t ) == 24 );
LT_CHECK( lt_bounceref,  sizeof( BounceRef_t ) == 8 );
LT_CHECK( lt_radsamp,    sizeof( RadiositySample_t ) == 16 );
LT_CHECK( lt_glob_total, offsetof( lightingGlob_t, totalSampleCount ) == 0x18 );
LT_CHECK( lt_glob_vars,  offsetof( lightingGlob_t, sampleVars ) == 0x20 );


/* Lighting_AllocLightmaps  0x00413bd0 */
void Lighting_AllocLightmaps( void )
{
    Assertx( lightingGlob.lmapCount >= 0, "(lightingGlob.lmapCount >= 0)" );

    lightingGlob.lmapDefs =
        ( LmapDef_t * )new ( std::nothrow ) byte[lightingGlob.lmapCount * LIGHTMAP_BYTES_PER_MAP];

    if ( !lightingGlob.lmapDefs )
        Error( "Couldn't allocate %g MB for lightmap data in %i lightmaps\n",
               lightingGlob.lmapCount * 6.0, lightingGlob.lmapCount );

    memset( lightingGlob.lmapDefs, 0, lightingGlob.lmapCount * LIGHTMAP_BYTES_PER_MAP );

    lightingGlob.lightSlots = PointLight_Count() + 2;

    numBSPLightBytes = lightingGlob.lmapCount * LIGHTMAP_BYTES_PER_MAP2;

    for ( int threadIndex = 0; threadIndex < THREAD_COUNT_MAX; threadIndex++ )
        lightingGlob.lastSuppressed[threadIndex] = -1;

    lightingGlob.suppressedCount = 0;
}

/* Lighting_DispatchSample  0x00413ed0 */
void Lighting_DispatchSample( int sampleIndex, int threadIndex )
{
    LmapDef_t *def = &lightingGlob.lmapDefs[sampleIndex];

    if ( 0.0f == def->areaX2 )
        return;

    lightingGlob.sampleFunc( def, threadIndex );
}

/* Lighting_DispatchPixel  0x00413f00 */
void Lighting_DispatchPixel( int sampleIndex, int threadIndex )
{
    int lmapIndex;
    int x;
    int y;

    lmapIndex   = sampleIndex / LIGHTMAP_SAMPLE_COUNT;
    sampleIndex = sampleIndex % LIGHTMAP_SAMPLE_COUNT;

    y           = sampleIndex / LIGHTMAP_SIZE;
    x           = sampleIndex % LIGHTMAP_SIZE;

    lightingGlob.pixelFunc( lmapIndex, x, y, threadIndex );
}

/* Lighting_PackedSampleVars  0x00413cb0 */
SampleVars_t *Lighting_PackedSampleVars( unsigned packed )
{
    LmapDef_t *sample;
    unsigned   index;

    index = ( LMAP_PACKED_INDEX( packed ) * LMAP_HEIGHT_MIN + LMAP_PACKED_T( packed ) )
            * LMAP_WIDTH_MIN + LMAP_PACKED_S( packed );

    sample = &lightingGlob.lmapDefs[index];

    Assertx( sample->vars, "sample->vars" );

    return sample->vars;
}

/* Lighting_Sample  0x00413d10 */
LmapDef_t *Lighting_Sample( unsigned lmapIndex, unsigned s, unsigned t )
{
    Assertx( lmapIndex < ( unsigned )lightingGlob.lmapCount,
             "lmapIndex doesn't index lightingGlob.lmapCount\n\t%i not in [0, %i)",
             lmapIndex, lightingGlob.lmapCount );

    Assertx( s < LMAP_WIDTH_MIN,
             "s doesn't index LMAP_WIDTH_MIN\n\t%i not in [0, %i)", s, LMAP_WIDTH_MIN );

    Assertx( t < LMAP_HEIGHT_MIN,
             "t doesn't index LMAP_HEIGHT_MIN\n\t%i not in [0, %i)", t, LMAP_HEIGHT_MIN );

    return &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + t ) * LMAP_WIDTH_MIN + s];
}

/* Lighting_SetSubSample  0x00413db0 */
void Lighting_SetSubSample( unsigned lmapIndex, float s, float t,
                            LmapSubSample_t *subSample )
{
    int si;
    int ti;

    Assertx( lmapIndex < MAX_MAP_LIGHTMAPS,
             "lmapIndex doesn't index MAX_MAP_LIGHTMAPS\n\t%i not in [0, %i)",
             lmapIndex, MAX_MAP_LIGHTMAPS );

    Assertx( subSample, "subSample" );

    si = ( int )floor( s );
    ti = ( int )floor( t );

    Assertx( si >= 0 && si < LMAP_WIDTH_MAX && ti >= 0 && ti < LMAP_HEIGHT_MAX,
             "%i: %g %g\n", lmapIndex, s, t );

    subSample->fracS = si & 1;
    subSample->fracT = ti & 1;

    subSample->sample = &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + ti / 2 )
                                            * LMAP_WIDTH_MIN + si / 2];
}

/* Lighting_DispatchMap  0x00413f60 */
void Lighting_DispatchMap( int lmapIndex, int threadIndex )
{
    LmapDef_t *lmap = ( LmapDef_t * )( ( byte * )lightingGlob.lmapDefs
                                       + lmapIndex * LIGHTMAP_BYTES_PER_MAP );

    lightingGlob.mapFunc( lmap, lmapIndex, threadIndex );
}

/* Lighting_ForEachSample  0x00413f90 */
void Lighting_ForEachSample( LmapSampleFunc_t func, int threads )
{
    lightingGlob.sampleFunc = func;

    RunThreadsOn( lightingGlob.lmapCount << 18, Lighting_DispatchSample, threads );
}

/* Lighting_ForEachPixel  0x00413fc0 */
void Lighting_ForEachPixel( LmapPixelFunc_t func, int threads )
{
    lightingGlob.pixelFunc = func;

    RunThreadsOn( lightingGlob.lmapCount << 18, Lighting_DispatchPixel, threads );
}

/* Lighting_ForEachMap  0x00413ff0 */
void Lighting_ForEachMap( LmapMapFunc_t func, int threads )
{
    lightingGlob.mapFunc = func;

    RunThreadsOn( lightingGlob.lmapCount, Lighting_DispatchMap, threads );
}

/* Lighting_CountUsefulSample  0x00414020 */
static void Lighting_CountUsefulSample( LmapDef_t *def, int threadIndex )
{
    ( void )def;
    ( void )threadIndex;

    lightingGlob.usefulSampleCount++;
}

/* Lighting_AssignSampleVars  0x00414030 */
static void Lighting_AssignSampleVars( LmapDef_t *sample, int threadIndex )
{
    ( void )threadIndex;

    SampleVars_t *vars;

    Assertx( sample->vars == NULL, "sample->vars == NULL" );

    vars = &lightingGlob.sampleVars[lightingGlob.usefulSampleCount];

    sample->vars = vars;

    vars->skyInfluence = &lightingGlob.skyInfluences[options.radiosityTraceCount
                                                     * lightingGlob.usefulSampleCount];

    lightingGlob.usefulSampleCount++;
}

/* Lighting_AllocSamples  0x004140a0 */
void Lighting_AllocSamples( void )
{
    size_t bytes;

    Assertx( lightingGlob.totalSampleCount == 0, "lightingGlob.totalSampleCount == 0" );

    lightingGlob.totalSampleCount = lightingGlob.lmapCount << 18;

    Assertx( lightingGlob.usefulSampleCount == 0, "lightingGlob.usefulSampleCount == 0" );

    lightingGlob.sampleFunc = Lighting_CountUsefulSample;

    RunThreadsOn( lightingGlob.lmapCount << 18, Lighting_DispatchSample, 1 );

    Assertx( lightingGlob.usefulSampleCount <= lightingGlob.totalSampleCount,
             "lightingGlob.usefulSampleCount <= lightingGlob.totalSampleCount\n\t%i, %i",
             lightingGlob.usefulSampleCount, lightingGlob.totalSampleCount );

    bytes = ( size_t )lightingGlob.usefulSampleCount * sizeof( SampleVars_t );

    lightingGlob.sampleVars =
        ( SampleVars_t * )new ( std::nothrow ) byte[bytes];

    if ( !lightingGlob.sampleVars )
        Error( "Couldn't allocate %.2g MB for %i useful lighting samples",
               bytes / 1048576.0, lightingGlob.usefulSampleCount );

    memset( lightingGlob.sampleVars, 0, bytes );

    bytes = ( size_t )options.radiosityTraceCount * lightingGlob.usefulSampleCount
            * sizeof( float );

    lightingGlob.skyInfluences = ( float * )new ( std::nothrow ) byte[bytes];

    if ( !lightingGlob.skyInfluences )
        Error( "Couldn't allocate %.2g MB for sky influences on %i useful lighting"
               " samples", bytes / 1048576.0, lightingGlob.usefulSampleCount );

    memset( lightingGlob.skyInfluences, 0, bytes );

    lightingGlob.usefulSampleCount = 0;
    lightingGlob.sampleFunc = Lighting_AssignSampleVars;

    RunThreadsOn( lightingGlob.lmapCount << 18, Lighting_DispatchSample, 1 );
}

/* Lighting_ScaleSample  0x00414260 */
static void Lighting_ScaleSample( LmapDef_t *def, int threadIndex )
{
    SampleVars_t *vars = ( SampleVars_t * )def->vars;
    float         max;
    float         scale;

    ( void )threadIndex;

    if ( !vars )
        return;

    max = Vec3MaxElement( vars->reflectance );

    if ( max == 0.0f )
        return;

    scale = LIGHTING_SCALE_HEADROOM / max;

    if ( options.radiosityScale < scale )
        scale = options.radiosityScale;

    vars->reflectance[0] = vars->reflectance[0] * scale;
    vars->reflectance[1] = vars->reflectance[1] * scale;
    vars->reflectance[2] = vars->reflectance[2] * scale;
}

/* Lighting_ScaleSamples  0x004142f0 */
void Lighting_ScaleSamples( void )
{
    lightingGlob.sampleFunc = Lighting_ScaleSample;

    RunThreadsOn( lightingGlob.lmapCount << 18, Lighting_DispatchSample,
                  options.threadCount );
}

/* Lighting_SampleColor  0x00414320 */
void Lighting_SampleColor( const SampleVars_t *vars, vec3_t out )
{
    Assertx( vars, "sampleVars" );

    out[0] = vars->color[0] * vars->reflectance[0];
    out[1] = vars->color[1] * vars->reflectance[1];
    out[2] = vars->color[2] * vars->reflectance[2];
}

/* Lighting_AllocBasisDirs  0x00414370 */
void Lighting_AllocBasisDirs( void )
{
    float radius;
    float step;
    float c;
    float s;
    int   i;

    Assertx( options.basisDirCount >= LIGHTING_BASIS_COUNT_MIN
             && options.basisDirCount <= LIGHTING_BASIS_COUNT_MAX,
             "options.basisDirCount not in [LIGHTING_BASIS_COUNT_MIN,"
             " LIGHTING_BASIS_COUNT_MAX]\n\t%i not in [%i, %i]",
             options.basisDirCount,
             LIGHTING_BASIS_COUNT_MIN, LIGHTING_BASIS_COUNT_MAX );

    lightingGlob.basisDirCount = options.basisDirCount;

    lightingGlob.basisDirs =
        ( vec3_t * )new ( std::nothrow ) byte[lightingGlob.basisDirCount * sizeof( vec3_t )];

    if ( !lightingGlob.basisDirs )
        Error( "Out of memory allocating %i lighting basis directions",
               lightingGlob.basisDirCount );

    step   = 0.5f / ( unsigned )lightingGlob.basisDirCount;
    radius = 0.5f * step;

    s = 0.0f;
    c = 1.0f;

    for ( i = 0; i < lightingGlob.basisDirCount; i++ )
    {
        float oldS;

        lightingGlob.basisDirs[i][0] = c * radius;
        lightingGlob.basisDirs[i][1] = s * radius;
        lightingGlob.basisDirs[i][2] = ( float )sqrt( ( float )( 1.0f - radius * radius ) );

        radius = radius + step;

        oldS = s;

        s = s * LIGHTING_GOLDEN_COS - c * LIGHTING_GOLDEN_SIN;
        c = c * LIGHTING_GOLDEN_COS + oldS * LIGHTING_GOLDEN_SIN;
    }
}

/* Lighting_AddToBasis  0x004144f0 */
void Lighting_AddToBasis( const vec3_t color, const vec3_t dir, vec3_t *basis )
{
    unsigned i;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        float d;

        d = lightingGlob.basisDirs[i][0] * dir[0]
          + lightingGlob.basisDirs[i][1] * dir[1]
          + lightingGlob.basisDirs[i][2] * dir[2];

        if ( d <= 0.0f )
            continue;

        basis[i][0] = color[0] * d + basis[i][0];
        basis[i][1] = color[1] * d + basis[i][1];
        basis[i][2] = color[2] * d + basis[i][2];
    }
}

/* Lighting_GammaToLinear  0x00414580 */
float Lighting_GammaToLinear( float color )
{
    Assertx( color >= 0, "(color >= 0)", color );

    return ( float )pow( color, options.gamma );
}

/* Lighting_Vec3GammaToLinear  0x004145d0 */
void Lighting_Vec3GammaToLinear( vec3_t color )
{
    color[0] = Lighting_GammaToLinear( color[0] );
    color[1] = Lighting_GammaToLinear( color[1] );
    color[2] = Lighting_GammaToLinear( color[2] );
}

/* Lighting_LinearToGamma  0x00414610 */
float Lighting_LinearToGamma( float color )
{
    float invGamma;

    Assertx( color >= LIGHTING_GAMMA_EPSILON, "(color >= -1.0e-4f)", color );

    if ( !( 0.0f < color ) )
        return 0.0f;

    invGamma = 1.0f / options.gamma;

    return ( float )pow( color, invGamma );
}

/* Lighting_Vec3LinearToGamma  0x00414680 */
void Lighting_Vec3LinearToGamma( vec3_t color )
{
    color[0] = Lighting_LinearToGamma( color[0] );
    color[1] = Lighting_LinearToGamma( color[1] );
    color[2] = Lighting_LinearToGamma( color[2] );
}

/* Lighting_ColorToByte  0x004146c0 */
byte Lighting_ColorToByte( float color )
{
    if ( !( color > 0.0f ) )
        return 0;

    if ( !( color < 1.0f ) )
        return 255;

    return ( byte )RoundFloatToInt( color * 255.0f );
}

/* Lighting_ColorPlanesToBytes  0x00414720 */
void Lighting_ColorPlanesToBytes( const float *colors, int count,
                                  const vec2_t *ranges, byte *out )
{
    int plane;
    int i;

    for ( plane = 0; plane < 3; plane++ )
    {
        for ( i = 0; i < count; i++ )
        {
            float t = ( colors[i] - ranges[i][0] ) / ( ranges[i][1] - ranges[i][0] );

            out[i] = Lighting_ColorToByte( t );
        }

        colors += count;
        out    += count;
    }
}

/* Lighting_ClampedDot  0x004147f0 */
float Lighting_ClampedDot( const vec3_t a, const vec3_t b )
{
    float d;

    d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

    d = d - 0.0f;

    if ( !( d >= 0.0f ) )
        return 0.0f;

    return d;
}

/* Lighting_BasisFitError  0x00414850 */
float Lighting_BasisFitError( const vec3_t *basis, const vec3_t dir,
                              const vec3_t ambient, const vec3_t direct )
{
    float    error = 0.0f;
    unsigned i;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        vec3_t fit;
        float  up;
        float  facing;

        up = lightingGlob.basisDirs[i][2] * 0.5f + 0.5f;

        facing = Lighting_ClampedDot( lightingGlob.basisDirs[i], dir );

        fit[0] = ambient[0] * up + direct[0] * facing;
        fit[1] = ambient[1] * up + direct[1] * facing;
        fit[2] = ambient[2] * up + direct[2] * facing;

        error = Vec3DistanceSq( fit, basis[i] ) + error;
    }

    return error;
}

/* Lighting_ClampColorToUnit  0x00414940 */
qboolean Lighting_ClampColorToUnit( const vec3_t in, vec3_t out )
{
    qboolean clamped = qfalse;
    int      i;

    for ( i = 0; i < 3; i++ )
    {
        if ( 0.0f > in[i] )
        {
            out[i]  = 0.0f;
            clamped = qtrue;
        }
        else if ( 1.0f < in[i] )
        {
            out[i]  = 1.0f;
            clamped = qtrue;
        }
        else
        {
            out[i] = in[i];
        }
    }

    return clamped;
}

/* Lighting_EvalBasisFit  0x00414c90 */
void Lighting_EvalBasisFit( const vec3_t basisDir, const vec3_t dir,
                            const vec3_t ambient, const vec3_t direct, vec3_t out )
{
    float up;
    float facing;

    up = basisDir[2] * 0.5f + 0.5f;

    facing = Lighting_ClampedDot( basisDir, dir );

    out[0] = ambient[0] * up + direct[0] * facing;
    out[1] = ambient[1] * up + direct[1] * facing;
    out[2] = ambient[2] * up + direct[2] * facing;
}

/* Lighting_SolveBasisFit  0x004149d0 */
void Lighting_SolveBasisFit( vec3_t ambient, vec3_t direct,
                             const vec3_t *basis, const vec3_t dir )
{
    float    suu = 0.0f;
    float    suf = 0.0f;
    float    sff = 0.0f;
    vec3_t   bu;
    vec3_t   bf;
    vec3_t   amb;
    vec3_t   dct;
    float    det;
    float    scale;
    unsigned i;
    int      c;

    bu[0] = 0.0f; bu[1] = 0.0f; bu[2] = 0.0f;
    bf[0] = 0.0f; bf[1] = 0.0f; bf[2] = 0.0f;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        float up     = lightingGlob.basisDirs[i][2] * 0.5f + 0.5f;
        float facing = Lighting_ClampedDot( lightingGlob.basisDirs[i], dir );

        suu = up * up + suu;
        suf = facing * up + suf;
        sff = facing * facing + sff;

        bu[0] = basis[i][0] * up + bu[0];
        bu[1] = basis[i][1] * up + bu[1];
        bu[2] = basis[i][2] * up + bu[2];

        bf[0] = basis[i][0] * facing + bf[0];
        bf[1] = basis[i][1] * facing + bf[1];
        bf[2] = basis[i][2] * facing + bf[2];
    }

    det = suu * sff - suf * suf;

    if ( det == 0.0f )
    {
        ambient[0] = 0.0f; ambient[1] = 0.0f; ambient[2] = 0.0f;
        direct[0]  = 0.0f; direct[1]  = 0.0f; direct[2]  = 0.0f;
        return;
    }

    scale = 1.0f / det;

    for ( c = 0; c < 3; c++ )
    {
        amb[c] = ( bu[c] * sff - bf[c] * suf ) * scale;
        dct[c] = ( bf[c] * suu - bu[c] * suf ) * scale;
    }

    if ( Lighting_ClampColorToUnit( dct, direct ) )
    {
        for ( c = 0; c < 3; c++ )
            amb[c] = direct[c] * -suf + bu[c];

        scale = 1.0f / suu;

        for ( c = 0; c < 3; c++ )
            amb[c] = amb[c] * scale;
    }

    if ( Lighting_ClampColorToUnit( amb, ambient ) )
    {
        for ( c = 0; c < 3; c++ )
            dct[c] = ambient[c] * -suf + bf[c];

        scale = 1.0f / sff;

        for ( c = 0; c < 3; c++ )
            dct[c] = dct[c] * scale;

        Lighting_ClampColorToUnit( dct, direct );
    }
}

/* Lighting_BasisFitGradient  0x00414cf0 */
void Lighting_BasisFitGradient( vec2_t grad, const vec3_t *basis, const vec3_t dir,
                                const vec3_t ambient, const vec3_t direct )
{
    float    gx = 0.0f;
    float    gy = 0.0f;
    float    scale;
    unsigned i;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        const float *bd = lightingGlob.basisDirs[i];
        vec3_t       fit;
        vec3_t       residual;
        float        up;
        float        facing;
        float        dot;
        float        resDot;
        float        tx;
        float        ty;

        up     = bd[2] * 0.5f + 0.5f;
        facing = Lighting_ClampedDot( bd, dir );

        fit[0] = ambient[0] * up + direct[0] * facing;
        fit[1] = ambient[1] * up + direct[1] * facing;
        fit[2] = ambient[2] * up + direct[2] * facing;

        residual[0] = basis[i][0] - fit[0];
        residual[1] = basis[i][1] - fit[1];
        residual[2] = basis[i][2] - fit[2];

        resDot = direct[0] * residual[0]
               + direct[1] * residual[1]
               + direct[2] * residual[2];

        dot = bd[0] * dir[0] + bd[1] * dir[1] + bd[2] * dir[2];
        dot = -dot;

        tx = dir[0] * dot + bd[0];
        ty = dir[1] * dot + bd[1];

        gx = tx * resDot + gx;
        gy = resDot * ty + gy;
    }

    scale = dir[2] + dir[2];

    grad[0] = scale * gx;
    grad[1] = scale * gy;
}

/* Lighting_DominantDirection  0x00414e70 */
void Lighting_DominantDirection( const vec3_t *basis, vec3_t dir )
{
    float    residual[LIGHTING_BASIS_COUNT_MAX];
    float    sumWeighted = 0.0f;
    float    sumWeight   = 0.0f;
    float    average;
    unsigned i;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
        residual[i] = basis[i][0] + basis[i][1] + basis[i][2];

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        float up = lightingGlob.basisDirs[i][2] * 0.5f + 0.5f;

        sumWeighted = residual[i] * up + sumWeighted;
        sumWeight   = up + sumWeight;
    }

    average = sumWeighted / sumWeight;

    dir[0] = 0.0f;
    dir[1] = 0.0f;
    dir[2] = 0.0f;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        const float *bd = lightingGlob.basisDirs[i];
        float        up = bd[2] * 0.5f + 0.5f;

        residual[i] = residual[i] - up * average;

        dir[0] = bd[0] * residual[i] + dir[0];
        dir[1] = bd[1] * residual[i] + dir[1];
        dir[2] = bd[2] * residual[i] + dir[2];
    }

    Vec3Normalize( dir );
}

/* Lighting_DecodeDirection  0x00413ae0 */
void Lighting_DecodeDirection( int x, int y, vec3_t dir )
{
    float fx;
    float fy;
    float len;
    float inv;

    fx = x * LIGHTMAP_DIR_SCALE - LIGHTMAP_DIR_BIAS;
    fy = y * LIGHTMAP_DIR_SCALE - LIGHTMAP_DIR_BIAS;

    len = fx * fx + fy * fy;
    len = len + 1.0f;
    len = ( float )sqrt( len );

    inv = 1.0f / len;

    dir[2] = inv;
    dir[0] = fx * inv;
    dir[1] = fy * inv;
}

/* Lighting_ReadLightmapTexel  0x004150b0 */
void Lighting_ReadLightmapTexel( int s, int t, int lmapIndex,
                                 vec3_t colorA, vec3_t colorB, vec3_t dir )
{
    const byte *a;
    const byte *b;
    int         index;
    float       scale = 1.0f / 255.0f;

    index = ( lmapIndex * ( LIGHTMAP_PLANE_COUNT * LMAP_HEIGHT_MIN ) + t )
            * LMAP_WIDTH_MIN + s;

    a = &lmapBytes[index * 4];
    b = a + LIGHTMAP_PLANE_BYTES;

    colorA[2] = a[0] * scale;
    colorA[1] = a[1] * scale;
    colorA[0] = a[2] * scale;

    colorB[2] = b[0] * scale;
    colorB[1] = b[1] * scale;
    colorB[0] = b[2] * scale;

    Lighting_DecodeDirection( a[3], b[3], dir );
}

/* Lighting_SampleLightmap  0x00415160 */
void Lighting_SampleLightmap( int lmapIndex, int s, int t,
                              const vec3_t normal, vec3_t out )
{
    vec3_t ambient;
    vec3_t direct;
    vec3_t dir;
    float  up;
    float  facing;

    Lighting_ReadLightmapTexel( s, t, lmapIndex, ambient, direct, dir );

    up = normal[2] * 0.5f + 0.5f;

    facing = Lighting_ClampedDot( dir, normal );

    out[0] = direct[0] * facing + ambient[0] * up;
    out[1] = direct[1] * facing + ambient[1] * up;
    out[2] = direct[2] * facing + ambient[2] * up;
}

/* Lighting_AverageSubSamples  0x00415220 */
float Lighting_AverageSubSamples( int lmapIndex, int s, int t )
{
    const byte *p;
    int         sum;
    int         row;

    p = &lmapSubBytes[( ( lmapIndex * ( LIGHTMAP_PLANE_COUNT * LMAP_HEIGHT_MIN ) + t )
                        * LMAP_WIDTH_MAX + s ) * 2];

    sum = 0;

    for ( row = 0; row < 2; row++ )
    {
        sum = sum + p[0] + p[1];
        p += LIGHTMAP_SUB_ROW_BYTES;
    }

    return sum * ( 1.0f / ( LIGHTMAP_SUB_BLOCK * 255.0f ) );
}

/* Lighting_ApplyContrast  0x00415270 */
void Lighting_ApplyContrast( int sampleCount, int baseIndex, vec3_t *colors )
{
    float intensity[LIGHTING_MAX_CONTRAST_SAMPLES];
    float minIntensity;
    float maxIntensity;
    float base;
    float contrast;
    float scale;
    int   i;

    Assertx( sampleCount > 0 && sampleCount <= LIGHTING_MAX_CONTRAST_SAMPLES,
             "(sampleCount > 0 && sampleCount <= ARRAY_COUNT( intensity ))" );

    Assertx( ( baseIndex >= 0 && baseIndex < sampleCount ) || baseIndex == -1,
             "((baseIndex >= 0 && baseIndex < sampleCount) || baseIndex == (-1))" );

    Assertx( colors, "colors" );

    minIntensity =  FLT_MAX;
    maxIntensity = -FLT_MAX;

    for ( i = 0; i < sampleCount; i++ )
    {
        intensity[i] = colors[i][0] * LIGHTING_LUMA_R
                     + colors[i][1] * LIGHTING_LUMA_G
                     + colors[i][2] * LIGHTING_LUMA_B;

        if ( intensity[i] < minIntensity )
            minIntensity = intensity[i];

        if ( intensity[i] > maxIntensity )
            maxIntensity = intensity[i];
    }

    if ( baseIndex == -1 )
        base = ( minIntensity + maxIntensity ) * 0.5f;
    else
        base = intensity[baseIndex];

    contrast = maxIntensity - minIntensity;

    Assertx( contrast >= 0.0f, "contrast >= 0.0f" );

    if ( contrast == 0.0f )
        return;

    if ( !( LIGHTING_CONTRAST_LIMIT > contrast ) )
        return;

    scale = ( float )pow( contrast + contrast, -options.contrastGain );

    for ( i = 0; i < sampleCount; i++ )
    {
        float adjusted = ( intensity[i] - base ) * scale + base;

        if ( !( adjusted > 0.0f ) )
        {
            colors[i][0] = 0.0f;
            colors[i][1] = 0.0f;
            colors[i][2] = 0.0f;
            continue;
        }

        adjusted = adjusted / intensity[i];

        colors[i][0] = colors[i][0] * adjusted;
        colors[i][1] = colors[i][1] * adjusted;
        colors[i][2] = colors[i][2] * adjusted;
    }
}

/* Lighting_ColorToGammaByte  0x00415770 */
byte Lighting_ColorToGammaByte( float color )
{
    if ( !( color > 0.0f ) )
        return 0;

    if ( !( color < 1.0f ) )
        return 255;

    return Lighting_ColorToByte( Lighting_LinearToGamma( color ) );
}


/* Lighting_BuildBasis  0x004157b0 */
void Lighting_BuildBasis( const SampleVars_t *vars, vec3_t *basis )
{
    unsigned i;

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
    {
        basis[i][0] = vars->coincident[0] + options.ambientColor[0];
        basis[i][1] = vars->coincident[1] + options.ambientColor[1];
        basis[i][2] = vars->coincident[2] + options.ambientColor[2];
    }

    GatherSampleToBasis( vars, basis );

    for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
        Lighting_Vec3LinearToGamma( basis[i] );

    Lighting_ApplyContrast( lightingGlob.basisDirCount, 0, basis );
}

/* Lighting_BuildBasisFromNeighbours  0x00415990 */
void Lighting_BuildBasisFromNeighbours( int lmapIndex, int s, int t, vec3_t *basis )
{
    vec3_t   neighbourBasis[LIGHTING_BASIS_COUNT_MAX];
    float    totalWeight = 0.0f;
    int      sLo, sHi, tLo, tHi;
    int      sCur, tCur;
    unsigned i;

    memset( basis, 0, lightingGlob.basisDirCount * sizeof( vec3_t ) );

    sLo = s - 1 < 0 ? 0 : s - 1;
    tLo = t - 1 < 0 ? 0 : t - 1;
    sHi = s + 1 > LMAP_WIDTH_MIN - 1 ? LMAP_WIDTH_MIN - 1 : s + 1;
    tHi = t + 1 > LMAP_HEIGHT_MIN - 1 ? LMAP_HEIGHT_MIN - 1 : t + 1;

    for ( tCur = tLo; tCur <= tHi; tCur++ )
    {
        for ( sCur = sLo; sCur <= sHi; sCur++ )
        {
            const LmapDef_t *def;
            float            weight;

            if ( sCur == s && tCur == t )
                continue;

            def = &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + tCur )
                                        * LMAP_WIDTH_MIN + sCur];

            if ( !def->vars || !def->vars->subMask )
                continue;

            weight = Bleed_TexelWeight( lmapIndex, s, t, sCur, tCur );

            if ( weight == 0.0f )
                continue;

            totalWeight += weight;

            Lighting_BuildBasis( def->vars, neighbourBasis );

            for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
            {
                basis[i][0] += neighbourBasis[i][0] * weight;
                basis[i][1] += neighbourBasis[i][1] * weight;
                basis[i][2] += neighbourBasis[i][2] * weight;
            }
        }
    }

    if ( totalWeight == 0.0f || totalWeight == 1.0f )
        return;

    {
        float scale = 1.0f / totalWeight;

        for ( i = 0; i < ( unsigned )lightingGlob.basisDirCount; i++ )
        {
            basis[i][0] *= scale;
            basis[i][1] *= scale;
            basis[i][2] *= scale;
        }
    }
}

/* Lighting_SubSampleValue  0x00415b80 */
static qboolean Lighting_SubSampleValue( const LmapDef_t *def, int lmapIndex,
                                         int lmapS, int lmapT,
                                         int fracS, int fracT, float *out )
{
    int   subS, subT;
    int   sLo, sHi, tLo, tHi;
    int   s, t;
    float total;

    Assertx( ( unsigned )lmapS < LMAP_WIDTH_MIN,
             "lmapS doesn't index LMAP_WIDTH_MIN\n\t%i not in [0, %i)",
             lmapS, LMAP_WIDTH_MIN );

    Assertx( ( unsigned )lmapT < LMAP_HEIGHT_MIN,
             "lmapT doesn't index LMAP_HEIGHT_MIN\n\t%i not in [0, %i)",
             lmapT, LMAP_HEIGHT_MIN );

    if ( def->vars && ( def->vars->subMask & ( 1 << ( fracS + fracT * 2 ) ) ) )
    {
        *out = def->vars->subValue[fracS + fracT * 2];
        return qtrue;
    }

    *out  = 0.0f;
    total = 0.0f;

    subS = lmapS * 2 + fracS;
    subT = lmapT * 2 + fracT;

    sLo = subS - 1 < 0 ? 0 : subS - 1;
    tLo = subT - 1 < 0 ? 0 : subT - 1;
    sHi = subS + 1 > LMAP_WIDTH_MAX - 1 ? LMAP_WIDTH_MAX - 1 : subS + 1;
    tHi = subT + 1 > LMAP_HEIGHT_MAX - 1 ? LMAP_HEIGHT_MAX - 1 : subT + 1;

    for ( t = tLo; t <= tHi; t++ )
    {
        for ( s = sLo; s <= sHi; s++ )
        {
            const LmapDef_t *other;
            int              index;
            float            weight;

            if ( s == subS && t == subT )
                continue;

            index = s % 2 + t % 2 * 2;

            other = &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + t / 2 )
                                          * LMAP_WIDTH_MIN + s / 2];

            if ( !other->vars || !( other->vars->subMask & ( 1 << index ) ) )
                continue;

            weight = Bleed_SubSampleWeight( lmapIndex, subS, subT, s, t );

            if ( weight == 0.0f )
                continue;

            total += weight;

            *out += other->vars->subValue[index];
        }
    }

    if ( total == 0.0f )
        return qfalse;

    *out /= total;

    return qtrue;
}

/* Lighting_GatherSubSampleValues  0x00415d90 */
static void Lighting_GatherSubSampleValues( const LmapDef_t *def, int lmapIndex,
                                            int lmapS, int lmapT, float *out )
{
    qboolean known[LIGHTMAP_SUB_BLOCK];
    float    total = 0.0f;
    int      knownCount = 0;
    int      fracS, fracT;

    for ( fracT = 0; fracT < 2; fracT++ )
    {
        for ( fracS = 0; fracS < 2; fracS++ )
        {
            known[fracS + fracT * 2] =
                Lighting_SubSampleValue( def, lmapIndex, lmapS, lmapT,
                                         fracS, fracT, &out[fracS + fracT * 2] );

            total      += out[fracS + fracT * 2];
            knownCount += known[fracS + fracT * 2];
        }
    }

    if ( knownCount != 0 && knownCount != LIGHTMAP_SUB_BLOCK )
    {
        float average = total / knownCount;

        for ( fracT = 0; fracT < 2; fracT++ )
            for ( fracS = 0; fracS < 2; fracS++ )
                if ( !known[fracS + fracT * 2] )
                    out[fracS + fracT * 2] = average;
    }
}

/* Lighting_NormaliseSample  0x00415e50 */
static void Lighting_NormaliseSample( int lmapIndex, int s, int t, int threadIndex )
{
    LmapDef_t    *def;
    SampleVars_t *vars;
    float         scale;
    int           i;
    unsigned      n;

    def  = &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + t ) * LMAP_WIDTH_MIN + s];
    vars = def->vars;

    if ( !vars || !vars->subMask )
        return;

    if ( !( 0.0f < def->areaX2 ) )
    {
        vars->subMask = 0;
        return;
    }

    scale = 1.0f / def->areaX2;

    vars->color[0] *= scale;
    vars->color[1] *= scale;
    vars->color[2] *= scale;

    vars->unscatteredIncidentLight[0][0] *= scale;
    vars->unscatteredIncidentLight[0][1] *= scale;
    vars->unscatteredIncidentLight[0][2] *= scale;

    vars->coincident[0] *= scale;
    vars->coincident[1] *= scale;
    vars->coincident[2] *= scale;

    for ( i = 0; i < options.radiosityTraceCount; i++ )
        vars->skyInfluence[i] *= scale;

    for ( n = 0; n < vars->directTransportCount; n++ )
    {
        vars->directTransport[n].color[0] *= scale;
        vars->directTransport[n].color[1] *= scale;
        vars->directTransport[n].color[2] *= scale;
    }

    for ( n = 0; n < ( unsigned )vars->bounceCount; n++ )
        vars->bounces[n].weight *= scale;

    for ( i = 0; i < LIGHTMAP_SUB_BLOCK; i++ )
    {
        if ( !( vars->subMask & ( 1 << i ) ) )
            continue;

        if ( !( 0.0f < def->subWeight[i] ) )
        {
            vars->subMask &= ~( 1 << i );
            continue;
        }

        vars->subValue[i] /= def->subWeight[i];
    }
}

/* Lighting_NormaliseSamples  0x00416020 */
void Lighting_NormaliseSamples( int threads )
{
    Lighting_ForEachPixel( Lighting_NormaliseSample, threads );
}

/* Lighting_AllocBleedMasks  0x00416050 */
void Lighting_AllocBleedMasks( void )
{
    Bleed_AllocMasks( lightingGlob.lmapCount );
}

/* Lighting_FindBleeding  0x00416060 */
void Lighting_FindBleeding( int threads )
{
    Bleed_FindBleeding( threads );
}

/* Lighting_VertexChecksum  0x004160b0 */
static int Lighting_VertexChecksum( void )
{
    return Com_BlockChecksum32( geoVertices, numBSPDrawVerts[TRIS_TYPE_LAYERED] * sizeof( GeoVertex_t ), 0 );
}

/* Lighting_SaveTransfers  0x004160e0 */
void Lighting_SaveTransfers( void )
{
    FILE *file;
    int   value;
    int   i;

    file = fopen( RADTRANS_FILE, "wb" );

    if ( !file )
        return;

    value = RADTRANS_VERSION;
    fwrite( &value, sizeof( value ), 1, file );

    fwrite( &lightingGlob.totalSampleCount, 4, 1, file );
    fwrite( &lightingGlob.usefulSampleCount, 4, 1, file );
    fwrite( &options.radiosityTraceCount, 4, 1, file );
    fwrite( &options.supersampleCount, 4, 1, file );
    fwrite( &options.traceFilterWidth, 4, 1, file );
    fwrite( &options.jitter, 4, 1, file );
    fwrite( &options.modelShadow, 1, 1, file );

    value = Geo_TriangleCount();
    fwrite( &value, 4, 1, file );

    value = Lighting_VertexChecksum();
    fwrite( &value, 4, 1, file );

    fwrite( &lightingGlob.suppressedCount, 4, 1, file );
    fwrite( lightingGlob.suppressed, sizeof( SuppressedRange_t ),
            lightingGlob.suppressedCount, file );

    fwrite( lightingGlob.skyInfluences, 4,
            options.radiosityTraceCount * lightingGlob.usefulSampleCount, file );

    for ( i = 0; i < lightingGlob.usefulSampleCount; i++ )
    {
        SampleVars_t *vars = &lightingGlob.sampleVars[i];

        fwrite( &vars->bounceCount, 4, 1, file );
        fwrite( vars->bounces, sizeof( BounceRef_t ), vars->bounceCount, file );
    }

    fclose( file );
}

/* Lighting_MatchByte  0x00416280 */
static qboolean Lighting_MatchByte( FILE *file, byte expected )
{
    byte value;

    fread( &value, sizeof( value ), 1, file );

    return value == expected;
}

/* Lighting_MatchInt  0x004162b0 */
static qboolean Lighting_MatchInt( FILE *file, int expected )
{
    int value;

    fread( &value, sizeof( value ), 1, file );

    return value == expected;
}

/* Lighting_MatchFloat  0x004162e0 */
static qboolean Lighting_MatchFloat( FILE *file, float expected )
{
    float value;

    fread( &value, sizeof( value ), 1, file );

    return value == expected;
}

/* Lighting_TransfersMatch  0x00416320 */
static qboolean Lighting_TransfersMatch( FILE *file )
{
    return Lighting_MatchInt( file, RADTRANS_VERSION )
        && Lighting_MatchInt( file, lightingGlob.totalSampleCount )
        && Lighting_MatchInt( file, lightingGlob.usefulSampleCount )
        && Lighting_MatchInt( file, options.radiosityTraceCount )
        && Lighting_MatchInt( file, options.supersampleCount )
        && Lighting_MatchInt( file, options.traceFilterWidth )
        && Lighting_MatchFloat( file, options.jitter )
        && Lighting_MatchByte( file, options.modelShadow )
        && Lighting_MatchInt( file, Geo_TriangleCount() )
        && Lighting_MatchInt( file, Lighting_VertexChecksum() );
}

/* Lighting_CompareSuppressed  0x00416430 */
static int Lighting_CompareSuppressed( const void *a, const void *b )
{
    const SuppressedRange_t *rangeA = ( const SuppressedRange_t * )a;
    const SuppressedRange_t *rangeB = ( const SuppressedRange_t * )b;

    if ( rangeA->key >= rangeB->key )
        return 1;

    return -1;
}

/* Lighting_LoadTransfers  0x00416460 */
qboolean Lighting_LoadTransfers( void )
{
    FILE *file;
    int   i;

    file = fopen( RADTRANS_FILE, "rb" );

    if ( !file )
        return qfalse;

    if ( !Lighting_TransfersMatch( file ) )
    {
        fclose( file );
        return qfalse;
    }

    Print( "----------------------------------------\n"
           "Loading saved light transport for sky and radiosity...\n" );

    fread( &lightingGlob.suppressedCount, 4, 1, file );

    if ( !lightingGlob.suppressedCount )
    {
        lightingGlob.suppressed = NULL;
    }
    else
    {
        lightingGlob.suppressed = ( SuppressedRange_t * )
            malloc( lightingGlob.suppressedCount * sizeof( SuppressedRange_t ) );

        if ( !lightingGlob.suppressed )
            Error( "Out of memory loading relight file" );

        fread( lightingGlob.suppressed, sizeof( SuppressedRange_t ),
               lightingGlob.suppressedCount, file );

        qsort( lightingGlob.suppressed, lightingGlob.suppressedCount,
               sizeof( SuppressedRange_t ), Lighting_CompareSuppressed );
    }

    fread( lightingGlob.skyInfluences, 4,
           options.radiosityTraceCount * lightingGlob.usefulSampleCount, file );

    for ( i = 0; i < lightingGlob.usefulSampleCount; i++ )
    {
        SampleVars_t *vars = &lightingGlob.sampleVars[i];

        fread( &vars->bounceCount, 4, 1, file );

        if ( !vars->bounceCount )
        {
            vars->bounces = NULL;
            continue;
        }

        vars->bounces = new ( std::nothrow ) BounceRef_t[vars->bounceCount];

        if ( !vars->bounces )
            Error( "Out of memory loading relight file" );

        fread( vars->bounces, sizeof( BounceRef_t ), vars->bounceCount, file );
    }

    fclose( file );

    return qtrue;
}

/* Lighting_SuppressKey  0x00416600 */
static __int64 Lighting_SuppressKey( int keyHigh, int keyLow )
{
    return ( ( __int64 )keyHigh << 32 ) | keyLow;
}

/* Lighting_IsSuppressed  0x00416620 */
qboolean Lighting_IsSuppressed( int keyHigh, int keyLow )
{
    __int64 key = Lighting_SuppressKey( keyHigh, keyLow );
    int     lo  = 0;
    int     hi  = lightingGlob.suppressedCount;

    while ( lo < hi )
    {
        int                      mid   = ( lo + hi ) / 2;
        const SuppressedRange_t *range = &lightingGlob.suppressed[mid];

        if ( ( unsigned __int64 )key < range->key )
            hi = mid;
        else if ( ( unsigned __int64 )key < range->key + range->count )
            return qtrue;
        else
            lo = mid + 1;
    }

    return qfalse;
}

/* Lighting_Suppress  0x004166c0 */
void Lighting_Suppress( int threadIndex, int keyHigh, int keyLow )
{
    __int64 key = Lighting_SuppressKey( keyHigh, keyLow );
    int     last;

    Lock( &lightingGlob.suppressedCount );

    last = lightingGlob.lastSuppressed[threadIndex];

    if ( last >= 0 )
    {
        SuppressedRange_t *prevSuppressed = &lightingGlob.suppressed[last];

        Assertx( prevSuppressed->key + prevSuppressed->count <= ( unsigned __int64 )key,
                 "prevSuppressed->key + prevSuppressed->count <= key" );

        if ( prevSuppressed->key + prevSuppressed->count == ( unsigned __int64 )key )
        {
            prevSuppressed->count++;
            Unlock( &lightingGlob.suppressedCount );
            return;
        }
    }

    if ( !( lightingGlob.suppressedCount & 0x3ff ) )
    {
        lightingGlob.suppressed = ( SuppressedRange_t * )
            realloc( lightingGlob.suppressed,
                     ( lightingGlob.suppressedCount + 0x400 ) * sizeof( SuppressedRange_t ) );

        if ( !lightingGlob.suppressed )
            Error( "Out of memory for %i suppressed lighting samples",
                   lightingGlob.suppressedCount + 0x400 );
    }

    lightingGlob.lastSuppressed[threadIndex] = lightingGlob.suppressedCount;

    lightingGlob.suppressed[lightingGlob.suppressedCount].key   = key;
    lightingGlob.suppressed[lightingGlob.suppressedCount].count = 1;

    lightingGlob.suppressedCount++;

    Unlock( &lightingGlob.suppressedCount );
}

/* Lighting_EncodeDirectionByte  0x00416800 */
static byte Lighting_EncodeDirectionByte( const vec3_t dir, int axis )
{
    float value = ( float )( dir[axis] / dir[2] * 0.25 + 0.5 );

    return Lighting_ColorToByte( value );
}

/* Lighting_RefineBasisFit  0x00416830 */
static void Lighting_RefineBasisFit( const vec3_t *basis, vec3_t ambient,
                                     vec3_t direct, vec3_t dir )
{
    float bestError;
    byte  dirX, dirY;
    int   step;

    bestError = Lighting_BasisFitError( basis, dir, ambient, direct );

    if ( bestError == 0.0f )
        return;

    dirX = Lighting_EncodeDirectionByte( dir, 0 );
    dirY = Lighting_EncodeDirectionByte( dir, 1 );

    step = 4;

    for ( ;; )
    {
        vec2_t gradient;
        float  largest;

        Lighting_BasisFitGradient( gradient, basis, dir, ambient, direct );

        largest = fabsf( gradient[0] ) - fabsf( gradient[1] ) < 0.0f
                    ? fabsf( gradient[1] ) : fabsf( gradient[0] );

        for ( ;; )
        {
            vec3_t testDir, testAmbient, testDirect;
            double scale = step / largest;
            float  testError;
            int    testX, testY;

            testX = dirX + RoundFloatToInt( ( float )( gradient[0] * scale ) );
            testY = dirY + RoundFloatToInt( ( float )( gradient[1] * scale ) );

            if ( testX > 255 )
                testX = 255;
            else if ( testX < 0 )
                testX = 0;

            if ( testY > 255 )
                testY = 255;
            else if ( testY < 0 )
                testY = 0;

            Lighting_DecodeDirection( testX, testY, testDir );

            Lighting_SolveBasisFit( testAmbient, testDirect, basis, testDir );

            testError = Lighting_BasisFitError( basis, testDir, testAmbient, testDirect );

            if ( bestError > testError )
            {
                Vec3Copy( testAmbient, ambient );
                Vec3Copy( testDirect, direct );
                Vec3Copy( testDir, dir );

                dirX      = ( byte )testX;
                dirY      = ( byte )testY;
                bestError = testError;
                break;
            }

            step /= 2;

            if ( !step )
                return;
        }
    }
}

/* Lighting_FitBasis  0x00416aa0 */
static void Lighting_FitBasis( const vec3_t *basis, vec3_t ambient,
                               vec3_t direct, vec3_t dir )
{
    Lighting_DominantDirection( basis, dir );
    Lighting_SolveBasisFit( ambient, direct, basis, dir );
    Lighting_RefineBasisFit( basis, ambient, direct, dir );
}

/* Lighting_WriteTexel  0x00416ad0 */
static void Lighting_WriteTexel( int lmapIndex, int t, int s, const vec3_t *basis,
                                 const float *subValues )
{
    vec3_t dir, ambient, direct;
    byte  *out;
    int    fracS, fracT;

    out = &lmapBytes[( ( lmapIndex * LIGHTMAP_PLANE_COUNT * LMAP_HEIGHT_MIN + t )
                       * LMAP_WIDTH_MIN + s ) * 4];

    Lighting_FitBasis( basis, ambient, direct, dir );

    out[0] = Lighting_ColorToByte( ambient[2] );
    out[1] = Lighting_ColorToByte( ambient[1] );
    out[2] = Lighting_ColorToByte( ambient[0] );
    out[3] = Lighting_EncodeDirectionByte( dir, 0 );

    out += LIGHTMAP_PLANE_BYTES;

    out[0] = Lighting_ColorToByte( direct[2] );
    out[1] = Lighting_ColorToByte( direct[1] );
    out[2] = Lighting_ColorToByte( direct[0] );
    out[3] = Lighting_EncodeDirectionByte( dir, 1 );

    {
        byte *sub = &lmapSubBytes[( ( lmapIndex * LIGHTMAP_PLANE_COUNT * LMAP_HEIGHT_MIN + t )
                                    * LMAP_WIDTH_MAX + s ) * 2];

        for ( fracT = 0; fracT < 2; fracT++ )
        {
            for ( fracS = 0; fracS < 2; fracS++ )
            {
                *sub++ = Lighting_ColorToByte( *subValues++ );
            }

            sub += LIGHTMAP_SUB_ROW_BYTES - 2;
        }
    }
}

/* Lighting_WriteSample  0x00416c50 */
static void Lighting_WriteSample( int lmapIndex, int s, int t, int threadIndex )
{
    vec3_t     basis[LIGHTING_BASIS_COUNT_MAX];
    float      subValues[LIGHTMAP_SUB_BLOCK];
    LmapDef_t *def;

    def = &lightingGlob.lmapDefs[( lmapIndex * LMAP_HEIGHT_MIN + t ) * LMAP_WIDTH_MIN + s];

    if ( def->vars && def->vars->subMask )
        Lighting_BuildBasis( def->vars, basis );
    else
        Lighting_BuildBasisFromNeighbours( lmapIndex, s, t, basis );

    Lighting_GatherSubSampleValues( def, lmapIndex, s, t, subValues );

    Lighting_WriteTexel( lmapIndex, t, s, basis, subValues );
}

/* Lighting_SeedSampleSkyLight  0x00416070 */
static void Lighting_SeedSampleSkyLight( int sampleIndex, int threadIndex )
{
    Compile_SeedSkyLight( &lightingGlob.sampleVars[sampleIndex] );
}

/* Lighting_SeedSkyLight  0x00416090 */
void Lighting_SeedSkyLight( int threads )
{
    RunThreadsOn( lightingGlob.usefulSampleCount, Lighting_SeedSampleSkyLight, threads );
}

/* Lighting_WriteSamples  0x00416ce0 */
void Lighting_WriteSamples( int threads )
{
    Lighting_ForEachPixel( Lighting_WriteSample, threads );
}

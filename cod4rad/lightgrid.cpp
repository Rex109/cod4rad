/* Original: c:\trees\cod3\cod3src\cod2rad\lightgrid.cpp */

#include "cod4rad.h"
#include "lightgrid.h"
#include "progress.h"
#include "cmdline.h"
#include "compile.h"
#include "geometry.h"
#include "lighting.h"
#include "pointlights.h"
#include "bspfile.h"
#include "threads.h"

#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "surfaceflags.h"
#include "r_material.h"
#include "q_shared.h"

#include <new>
#include <algorithm>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


lightGridGlob_t lightGridGlob;                          /* 0x13063950 */

vec3_t              lightGridBasis[GFX_LIGHTGRID_SAMPLE_COUNT];  /* 0x130632b0 */

int                 lightGridHullStart[MAX_MAP_PRIMARY_LIGHTS + 1];  /* 0x13063550 */
int                 lightGridAxisStart[MAX_MAP_PRIMARY_LIGHTS + 1];  /* 0x13063988 */

int                 lightGridClusterCount;              /* 0x13063960 */
LightGridCluster_t *lightGridClusters;                  /* 0x13063964 */
int                *lightGridIndexes;                   /* 0x13063968 */
vec3_t             *lightGridSkyTraceDirs;              /* 0x13063984 */


#define LG_CHECK( name, cond )  typedef char name[( cond ) ? 1 : -1]

LG_CHECK( lg_point_size,   sizeof( LightGridPoint_t ) == 12 );
LG_CHECK( lg_sample_size,  sizeof( LightGridSample_t ) == 0xa8 );
LG_CHECK( lg_origin_size,  sizeof( StaticModelOrigin_t ) == 16 );
LG_CHECK( lg_cluster_size, sizeof( LightGridCluster_t ) == 20 );
LG_CHECK( lg_file_size,    sizeof( LightGridPointFile_t ) == 6 );
LG_CHECK( lg_vclog_size,   sizeof( LightGridVisCacheEntry_t ) == 24 );
LG_CHECK( lg_entry_size,   sizeof( BspLightGridEntry_t ) == 4 );


/* CosOfAngleSum  0x0040f860 */
static float CosOfAngleSum( float cosA, float cosB )
{
    double product;
    float  sinASq;
    float  sinBSq;
    float  sinPart;

    product = cosA * cosB;

    sinASq  = 1.0f - cosA * cosA;
    sinBSq  = 1.0f - cosB * cosB;
    sinPart = sinASq * sinBSq;
    sinPart = ( float )sqrt( sinPart );

    return ( float )( product - sinPart );
}


static void LightGrid_PointToOrigin( vec3_t origin, const LightGridPoint_t *point )
{
    origin[0] = ( float )( ( point->pos[0] - LIGHTGRID_BIAS_XY ) * LIGHTGRID_SPACING_XY );
    origin[1] = ( float )( ( point->pos[1] - LIGHTGRID_BIAS_XY ) * LIGHTGRID_SPACING_XY );
    origin[2] = ( float )( ( point->pos[2] - LIGHTGRID_BIAS_Z )  * LIGHTGRID_SPACING_Z );
}

/* LightGrid_OpenPointFile  0x0040f970 */
static qboolean LightGrid_OpenPointFile( const char *verb, const char *ext,
                                         FILE **file, int *count, char *path )
{
    int size;

    COM_StripExtension( options.mapName, path );
    strcat( path, ext );

    *file = fopen( path, "rb" );

    if ( !*file )
    {
        Print( "Light grid sample point file '%s' not found.\n", path );
        return qfalse;
    }

    fseek( *file, 0, SEEK_END );
    size = ftell( *file );
    fseek( *file, 0, SEEK_SET );

    if ( size && size % sizeof( LightGridPointFile_t ) == 0 )
    {
        *count = size / sizeof( LightGridPointFile_t );
        Print( "%s %i grid points from grid logfile '%s'.\n", verb, *count, path );
        return qtrue;
    }

    Print( "Ignoring grid logfile '%s': size %i is not a multiple of %i.\n",
           path, size, sizeof( LightGridPointFile_t ) );

    fclose( *file );

    return qfalse;
}

/* LightGrid_ReadPointFile  0x0040fa60 */
static void LightGrid_ReadPointFile( unsigned count, const char *path, FILE *file )
{
    LightGridPointFile_t *entries;
    unsigned              i;

    entries = new ( std::nothrow ) LightGridPointFile_t[count];

    if ( !entries )
    {
        fclose( file );
        Error( "couldn't allocate %.2f MB for the light grid",
               count * ( sizeof( LightGridPointFile_t ) / ( 1024.0 * 1024.0 ) ) );
    }

    if ( fread( entries, sizeof( *entries ), count, file ) != count )
    {
        fclose( file );
        Error( "Error while reading %s", path );
    }

    fclose( file );

    for ( i = 0; i < count; i++ )
    {
        LightGridPoint_t *point = &lightGridGlob.points[lightGridGlob.pointCount];

        lightGridGlob.pointCount++;

        point->pos[0]            = entries[i].pos[0];
        point->pos[1]            = entries[i].pos[1];
        point->pos[2]            = entries[i].pos[2];
        point->needsTrace        = 1;
        point->colorsIndex       = 0;
        point->primaryLightIndex = 0;
        point->cornerMask        = 0;
    }

    delete[] entries;
}

/* LightGrid_AllocPoints  0x0040fb70 */
static void LightGrid_AllocPoints( int extra )
{
    char     path[0x400];
    FILE    *file;
    int      count;
    qboolean haveFile;

    Assert( lightGridGlob.points == NULL );
    Assert( lightGridGlob.pointCount == 0 );

    haveFile = LightGrid_OpenPointFile( "Using", ".grid_auto", &file, &count, path );

    if ( !haveFile )
        count = 0;

    lightGridGlob.maxPoints =
        ( count
          + 8 * ( numBSPPrimaryLights * 128 + lightGridGlob.staticModelOriginCount )
          + extra ) * 5 / 4;

    lightGridGlob.points =
        new ( std::nothrow ) LightGridPoint_t[lightGridGlob.maxPoints];

    if ( !lightGridGlob.points )
        Error( "couldn't allocate %.2f MB for the light grid",
               lightGridGlob.maxPoints
               * ( sizeof( LightGridPoint_t ) / ( 1024.0 * 1024.0 ) ) );

    if ( haveFile )
        LightGrid_ReadPointFile( count, path, file );
}

/* LightGrid_AddStaticModelOrigin  0x0040fc90 */
void LightGrid_AddStaticModelOrigin( const vec3_t origin )
{
    StaticModelOrigin_t *entry;

    entry = new ( std::nothrow ) StaticModelOrigin_t;

    if ( !entry )
        Error( "Out of memory on %i bytes for a static model origin\n",
               sizeof( StaticModelOrigin_t ) );

    entry->origin[0] = origin[0];
    entry->origin[1] = origin[1];
    entry->origin[2] = origin[2];

    entry->next = lightGridGlob.staticModelOrigins;

    lightGridGlob.staticModelOriginCount++;
    lightGridGlob.staticModelOrigins = entry;
}

/* LightGrid_ReadGridFile  0x0040fcf0 */
static void LightGrid_ReadGridFile( void )
{
    char  path[0x400];
    FILE *file;
    int   count;

    if ( !LightGrid_OpenPointFile( "Using", ".grid", &file, &count, path ) )
        return;

    LightGrid_AllocPoints( count );
    LightGrid_ReadPointFile( count, path, file );
}

/* LightGrid_ReadVisCacheLog  0x0040fd50 */
static void LightGrid_ReadVisCacheLog( void )
{
    char  path[0x400];
    FILE *file;
    int   size;
    int   count;

    Assert( lightGridGlob.pointCount == 0 );
    Assert( lightGridGlob.points == NULL );

    COM_StripExtension( options.mapName, path );
    strcat( path, ".vclog" );

    file = fopen( path, "rb" );

    if ( !file )
    {
        Print( "Vis cache logfile '%s' not found;"
               " using static model origins only.\n", path );
        return;
    }

    fseek( file, 0, SEEK_END );
    size = ftell( file );
    fseek( file, 0, SEEK_SET );

    if ( !size || size % sizeof( LightGridVisCacheEntry_t ) != 0 )
    {
        Print( "Ignoring vis cache logfile '%s':"
               " size %i is not a multiple of %i.\n",
               path, size, sizeof( LightGridVisCacheEntry_t ) );
        fclose( file );
        return;
    }

    count = size / sizeof( LightGridVisCacheEntry_t );

    Assert( !lightGridGlob.points );

    LightGrid_AllocPoints( count );

    Print( "Using %i grid points from vis cache logfile '%s'.\n", count, path );

    while ( count )
    {
        LightGridVisCacheEntry_t entry;
        LightGridPoint_t        *point;

        if ( fread( &entry, sizeof( entry ), 1, file ) != 1 )
        {
            fclose( file );
            Error( "Error while reading %s", path );
        }

        point = &lightGridGlob.points[lightGridGlob.pointCount];

        lightGridGlob.pointCount++;

        point->pos[0]            = ( unsigned short )entry.pos[0];
        point->pos[1]            = ( unsigned short )entry.pos[1];
        point->pos[2]            = ( unsigned short )entry.pos[2];
        point->needsTrace        = 1;
        point->colorsIndex       = 0;
        point->primaryLightIndex = 0;
        point->cornerMask        = 0;

        count--;
    }

    fclose( file );
}

/* LightGrid_AddStaticModelPoints  0x0040ff60 */
static void LightGrid_AddStaticModelPoints( void )
{
    while ( lightGridGlob.staticModelOrigins )
    {
        StaticModelOrigin_t *entry = lightGridGlob.staticModelOrigins;
        int                  base[3];
        int                  corner;

        lightGridGlob.staticModelOrigins = entry->next;

        float x = ( float )( ( entry->origin[0] - LIGHTGRID_WORLD_MIN )
                             * ( 1.0 / LIGHTGRID_SPACING_XY ) );
        float y = ( float )( ( entry->origin[1] - LIGHTGRID_WORLD_MIN )
                             * ( 1.0 / LIGHTGRID_SPACING_XY ) );
        float z = ( float )( ( entry->origin[2] - LIGHTGRID_WORLD_MIN )
                             * ( 1.0 / LIGHTGRID_SPACING_Z ) );

        base[0] = ( int )( float )floor( x );
        base[1] = ( int )( float )floor( y );
        base[2] = ( int )( float )floor( z );

        for ( corner = 0; corner < 8; corner++ )
        {
            LightGridPoint_t *point = &lightGridGlob.points[lightGridGlob.pointCount];

            lightGridGlob.pointCount++;

            AssertCmp( lightGridGlob.pointCount, <=, lightGridGlob.maxPoints );

            point->pos[0]            = ( unsigned short )( base[0] + ( corner & 1 ) );
            point->pos[1]            = ( unsigned short )( base[1] + ( ( corner >> 1 ) & 1 ) );
            point->pos[2]            = ( unsigned short )( base[2] + ( ( corner >> 2 ) & 1 ) );
            point->needsTrace        = 1;
            point->colorsIndex       = 0;
            point->primaryLightIndex = 0;
            point->cornerMask        = 0;
        }

        delete entry;
    }
}

/* LightGrid_LoadPoints  0x00410c90 */
static void LightGrid_LoadPoints( void )
{
    SanityCheck( lightGridGlob.pointCount == 0 );

    LightGrid_ReadGridFile();

    if ( !lightGridGlob.pointCount )
    {
        LightGrid_ReadVisCacheLog();

        if ( !lightGridGlob.pointCount )
            LightGrid_AllocPoints( 0 );
    }

    LightGrid_AddStaticModelPoints();
}


/* LightGrid_PointLess  0x00410110 */
static qboolean LightGrid_PointLess( const LightGridPoint_t *a,
                                     const LightGridPoint_t *b )
{
    int rowAxis = bspLightGridHeader.rowAxis;
    int colAxis;

    if ( a->pos[rowAxis] < b->pos[rowAxis] )
        return qtrue;

    if ( a->pos[rowAxis] > b->pos[rowAxis] )
        return qfalse;

    colAxis = bspLightGridHeader.colAxis;

    if ( a->pos[colAxis] < b->pos[colAxis] )
        return qtrue;

    if ( a->pos[colAxis] > b->pos[colAxis] )
        return qfalse;

    return a->pos[2] < b->pos[2];
}

static bool LightGrid_PointLessRef( const LightGridPoint_t &a,
                                    const LightGridPoint_t &b )
{
    return LightGrid_PointLess( &a, &b ) != 0;
}

/* LightGrid_FindPoint  0x00410160 */
static LightGridPoint_t *LightGrid_FindPoint( const LightGridPoint_t *key )
{
    LightGridPoint_t *found;
    unsigned          lo;
    unsigned          hi;

    Assert( lightGridGlob.pointCount );

    hi = lightGridGlob.pointCount - 1;

    if ( LightGrid_PointLess( &lightGridGlob.points[hi], key ) )
        return NULL;

    lo = 0;

    while ( lo != hi )
    {
        unsigned mid = ( hi + lo ) >> 1;

        if ( LightGrid_PointLess( &lightGridGlob.points[mid], key ) )
            lo = mid + 1;
        else
            hi = mid;
    }

    found = &lightGridGlob.points[lo];

    if ( LightGrid_PointLess( key, found ) )
        return NULL;

    return found;
}

/* LightGrid_HasPointAtOrigin  0x00410280 */
qboolean LightGrid_HasPointAtOrigin( const vec3_t origin )
{
    LightGridPoint_t key;

    float x = ( float )( ( origin[0] - LIGHTGRID_WORLD_MIN )
                         * ( 1.0 / LIGHTGRID_SPACING_XY ) + 0.5 );
    float y = ( float )( ( origin[1] - LIGHTGRID_WORLD_MIN )
                         * ( 1.0 / LIGHTGRID_SPACING_XY ) + 0.5 );
    float z = ( float )( ( origin[2] - LIGHTGRID_WORLD_MIN )
                         * ( 1.0 / LIGHTGRID_SPACING_Z ) + 0.5 );

    key.pos[0] = ( unsigned short )( int )floor( x );
    key.pos[1] = ( unsigned short )( int )floor( y );
    key.pos[2] = ( unsigned short )( int )floor( z );

    return LightGrid_FindPoint( &key ) != NULL;
}


/* LightGrid_PointIsBuried  0x00410370 */
static qboolean LightGrid_PointIsBuried( const LightGridPoint_t *point,
                                         const vec3_t origin )
{
    qboolean found[2][3];
    qboolean any;
    int      x, y, z;

    if ( Geo_PointIsSolid( origin ) )
        return qtrue;

    memset( found, 0, sizeof( found ) );

    for ( z = -1; z <= 1; z++ )
    {
        for ( y = -1; y <= 1; y++ )
        {
            for ( x = -1; x <= 1; x++ )
            {
                LightGridPoint_t key;

                if ( !x && !y && !z )
                    continue;

                key.pos[0] = ( unsigned short )( point->pos[0] + x );
                key.pos[1] = ( unsigned short )( point->pos[1] + y );
                key.pos[2] = ( unsigned short )( point->pos[2] + z );

                if ( !LightGrid_FindPoint( &key ) )
                    continue;

                if ( x < 0 )
                    found[0][0] = 1;
                else if ( x > 0 )
                    found[1][0] = 1;

                if ( y < 0 )
                    found[0][1] = 1;
                else if ( y > 0 )
                    found[1][1] = 1;

                if ( z < 0 )
                    found[0][2] = 1;
                else if ( z > 0 )
                    found[1][2] = 1;
            }
        }
    }

    any = qfalse;

    for ( z = 0; z < 2; z++ )
    {
        if ( !found[z][2] )
            continue;

        for ( y = 0; y < 2; y++ )
        {
            if ( !found[y][1] )
                continue;

            for ( x = 0; x < 2; x++ )
            {
                vec3_t corner;

                if ( !found[x][0] )
                    continue;

                any = qtrue;

                corner[0] = origin[0] + ( x ? LIGHTGRID_CORNER_OFFSET
                                            : -LIGHTGRID_CORNER_OFFSET );
                corner[1] = origin[1] + ( y ? LIGHTGRID_CORNER_OFFSET
                                            : -LIGHTGRID_CORNER_OFFSET );
                corner[2] = origin[2] + ( z ? LIGHTGRID_CORNER_OFFSET
                                            : -LIGHTGRID_CORNER_OFFSET );

                if ( !Geo_PointIsSolid( corner ) )
                    return qfalse;
            }
        }
    }

    return any;
}

static const vec3_t lightGridEscapeDirs[6] =      /* 0x00478838 */
{
    {  0.0f,                       0.0f, -LIGHTGRID_TRACE_DISTANCE },
    {  0.0f,                       0.0f,  LIGHTGRID_TRACE_DISTANCE },
    { -LIGHTGRID_TRACE_DISTANCE,   0.0f,  0.0f },
    {  LIGHTGRID_TRACE_DISTANCE,   0.0f,  0.0f },
    {  0.0f, -LIGHTGRID_TRACE_DISTANCE,   0.0f },
    {  0.0f,  LIGHTGRID_TRACE_DISTANCE,   0.0f }
};

/* LightGrid_PointIsEnclosed  0x00410510 */
static qboolean LightGrid_PointIsEnclosed( const vec3_t origin )
{
    GeoTrace_t trace;
    int        i;

    for ( i = 0; i < 6; i++ )
    {
        const GeoHit_t   *lastHit;
        const Material_t *material;
        vec3_t            end;

        end[0] = origin[0] + lightGridEscapeDirs[i][0];
        end[1] = origin[1] + lightGridEscapeDirs[i][1];
        end[2] = lightGridEscapeDirs[i][2] + origin[2];

        Geo_SetupTrace( origin, end, NULL, NULL, &trace );
        Geo_TraceRay( &trace );

        if ( !trace.result.hitCount )
            continue;

        lastHit = &trace.result.hits[trace.result.hitCount - 1];

        AssertCmp( lastHit->geoType, ==, TRACE_HIT_WORLD_GEO );

        material = lastHit->tri->mskMtl->material;

        if ( !( material->surfaceFlags & SURF_NODRAW )
             && Vec3Dot( lastHit->tri->normal, lightGridEscapeDirs[i] ) < 0.0f )
            return qfalse;

        if ( material->surfaceFlags & SURF_SKY )
            return qfalse;
    }

    return qtrue;
}

/* LightGrid_SuppressPoint  0x00410610 */
static qboolean LightGrid_SuppressPoint( const LightGridPoint_t *point )
{
    vec3_t origin;

    LightGrid_PointToOrigin( origin, point );

    if ( Geo_PointIsOutsideWorld( origin ) )
        return qtrue;

    if ( LightGrid_PointIsBuried( point, origin ) )
        return qtrue;

    if ( Geo_PointIsCoveredOverSky( origin ) )
        return qtrue;

    return LightGrid_PointIsEnclosed( origin ) != 0;
}

/* LightGrid_PointsEqual  0x004106a0 */
static qboolean LightGrid_PointsEqual( const LightGridPoint_t *a,
                                       const LightGridPoint_t *b )
{
    return a->pos[0] == b->pos[0]
        && a->pos[1] == b->pos[1]
        && a->pos[2] == b->pos[2];
}

/* LightGrid_RemoveDuplicates  0x004106d0 */
static void LightGrid_RemoveDuplicates( int first )
{
    int src = first;
    int count = 0;

    for ( ;; )
    {
        lightGridGlob.points[count] = lightGridGlob.points[src];
        count++;

        do
        {
            src++;

            if ( src == lightGridGlob.pointCount )
            {
                lightGridGlob.pointCount = count;
                return;
            }
        }
        while ( LightGrid_PointsEqual( &lightGridGlob.points[src],
                                       &lightGridGlob.points[src - 1] ) );
    }
}

/* LightGrid_SuppressPoints  0x00410760 */
static void LightGrid_SuppressPoints( void )
{
    int firstKept = -1;
    int i = 0;

    if ( !lightGridGlob.pointCount )
    {
        lightGridGlob.pointCount = 0;
        return;
    }

    for ( ;; )
    {
        LightGridPoint_t *points = lightGridGlob.points;
        int               next   = i + 1;

        while ( next < lightGridGlob.pointCount
                && LightGrid_PointsEqual( &points[i], &points[next] ) )
            next++;

        if ( LightGrid_SuppressPoint( &points[i] ) )
        {
            if ( firstKept != -1 )
            {
                int k;

                for ( k = i; k < next; k++ )
                    lightGridGlob.points[k] = lightGridGlob.points[i - 1];
            }
        }
        else if ( firstKept == -1 )
        {
            firstKept = next - 1;
        }

        i = next;

        if ( i >= lightGridGlob.pointCount )
            break;
    }

    if ( firstKept == -1 )
    {
        lightGridGlob.pointCount = 0;
        return;
    }

    LightGrid_RemoveDuplicates( firstKept );
}

/* LightGrid_ExcludePoints  0x00410870 */
static void LightGrid_ExcludePoints( void )
{
    char     path[0x400];
    FILE    *file;
    int      count;
    qboolean removed = qfalse;

    if ( !LightGrid_OpenPointFile( "Excluding", ".grid_not", &file, &count, path ) )
        return;

    while ( count )
    {
        LightGridPointFile_t entry;
        LightGridPoint_t     key;
        LightGridPoint_t    *found;
        LightGridPoint_t    *end;
        LightGridPoint_t    *from;

        if ( fread( &entry, sizeof( entry ), 1, file ) != 1 )
        {
            Print( "unexpected end-of-file in '%s'\n", path );
            break;
        }

        count--;

        key.pos[0] = entry.pos[0];
        key.pos[1] = entry.pos[1];
        key.pos[2] = entry.pos[2];

        found = LightGrid_FindPoint( &key );

        if ( !found )
            continue;

        Assert( found >= &lightGridGlob.points[0]
                && found < &lightGridGlob.points[lightGridGlob.pointCount] );

        end = &lightGridGlob.points[lightGridGlob.pointCount];

        if ( found != lightGridGlob.points )
        {
            from = found - 1;
        }
        else
        {
            from = found;

            while ( from + 1 != end )
            {
                if ( !LightGrid_PointsEqual( from + 1, &key ) )
                {
                    from = from + 1;
                    break;
                }

                from = from + 1;
            }
        }

        for ( ;; )
        {
            *found = *from;
            found++;

            if ( found == end )
                break;

            Assert( found >= &lightGridGlob.points[0]
                    && found < &lightGridGlob.points[lightGridGlob.pointCount] );

            if ( !LightGrid_PointsEqual( found, &key ) )
                break;
        }

        removed = qtrue;
    }

    fclose( file );

    if ( removed )
        LightGrid_RemoveDuplicates( 0 );
}

/* LightGrid_InsertMissingPoints  0x00410a60 */
static void LightGrid_InsertMissingPoints( void )
{
    int insertCount = 0;
    int i;
    int next;

    if ( !lightGridGlob.pointCount )
        return;

    for ( i = 0; i < lightGridGlob.pointCount; i = next )
    {
        LightGridPoint_t *points = lightGridGlob.points;

        for ( next = i + 1; next < lightGridGlob.pointCount; next++ )
            if ( points[next].pos[0] != points[i].pos[0]
                 || points[next].pos[1] != points[i].pos[1] )
                break;

        insertCount += points[next - 1].pos[2] - points[i].pos[2] - ( next - i ) + 1;
    }

    if ( !insertCount )
        return;

    if ( lightGridGlob.pointCount + insertCount > lightGridGlob.maxPoints )
        Error( "Needed to insert too many points.  Using 'lightgrid_sky' brushes"
               " in the sky where vehicles fly may fix this." );

    {
        LightGridPoint_t *points = lightGridGlob.points;
        int               dest   = lightGridGlob.pointCount + insertCount;
        int               srcEnd = lightGridGlob.pointCount;

        while ( srcEnd )
        {
            int            first = srcEnd - 1;
            int            z;
            unsigned short lowZ;

            while ( first
                    && points[first - 1].pos[0] == points[srcEnd - 1].pos[0]
                    && points[first - 1].pos[1] == points[srcEnd - 1].pos[1] )
                first--;

            lowZ = ( unsigned short )( points[first].pos[2] - 1 );

            for ( z = points[srcEnd - 1].pos[2]; ( unsigned short )z != lowZ; z-- )
            {
                LightGridPoint_t *out;
                byte              have;

                dest--;
                out = &points[dest];

                out->pos[0]            = points[first].pos[0];
                out->pos[1]            = points[first].pos[1];
                out->pos[2]            = ( unsigned short )z;
                out->colorsIndex       = 0;
                out->primaryLightIndex = 0;
                out->cornerMask        = 0;

                have = ( byte )( points[srcEnd - 1].pos[2] == ( unsigned short )z );
                out->needsTrace = have;

                if ( have )
                    srcEnd--;
            }

            AssertCmp( srcEnd, ==, first );
        }

        Assert( dest == 0 );

        lightGridGlob.pointCount += insertCount;
    }
}


/* LightGrid_ComputeCornerMask  0x00410cf0 */
static void LightGrid_ComputeCornerMask( int threadIndex, const vec3_t origin,
                                         byte *needsTrace )
{
    int corner;

    Assert( needsTrace );

    *needsTrace = 0;

    for ( corner = 0; corner < 8; corner++ )
    {
        int rowAxis;
        int bit;

        if ( !Geo_GridCornerIsCutOff( threadIndex, origin, corner ) )
            continue;

        rowAxis = bspLightGridHeader.rowAxis;
        bit     = 0;

        if ( corner & 1 )
            bit = ( rowAxis == 0 ) * 2 + 2;

        if ( corner & 2 )
            bit |= ( rowAxis != 0 ) * 2 + 2;

        if ( corner & 4 )
            bit |= 1;

        *needsTrace |= ( byte )( 1 << bit );
    }
}


/* LightGrid_AddDirectionalColor  0x00410d80 */
static void LightGrid_AddDirectionalColor( LightGridColors_t *colors,
                                           const vec3_t dir, const vec3_t color )
{
    int i;

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        float dot = lightGridBasis[i][0] * dir[0]
                  + lightGridBasis[i][1] * dir[1]
                  + lightGridBasis[i][2] * dir[2];

        if ( dot > 0.0f )
        {
            colors->color[i][0] = color[0] * dot + colors->color[i][0];
            colors->color[i][1] = color[1] * dot + colors->color[i][1];
            colors->color[i][2] = color[2] * dot + colors->color[i][2];
        }
    }
}

/* LightGrid_AddAmbientColor  0x00410f20 */
static void LightGrid_AddAmbientColor( LightGridColors_t *colors, const vec3_t color )
{
    int i;

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        colors->color[i][0] += color[0];
        colors->color[i][1] += color[1];
        colors->color[i][2] += color[2];
    }
}

/* LightGrid_PointInHull  0x00411010 */
static qboolean LightGrid_PointInHull( const vec3_t local,
                                       const BspLightRegionHull_t *hull,
                                       const BspLightRegionAxis_t *axes )
{
    unsigned i;

    if ( !( hull->kdopHalfSize[0] > ( float )fabs( local[0] - hull->kdopMidPoint[0] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[1] > ( float )fabs( local[1] - hull->kdopMidPoint[1] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[2] > ( float )fabs( local[2] - hull->kdopMidPoint[2] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[3]
            > ( float )fabs( local[1] + local[0] - hull->kdopMidPoint[3] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[4]
            > ( float )fabs( local[0] - local[1] - hull->kdopMidPoint[4] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[5]
            > ( float )fabs( local[0] + local[2] - hull->kdopMidPoint[5] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[6]
            > ( float )fabs( local[0] - local[2] - hull->kdopMidPoint[6] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[7]
            > ( float )fabs( local[1] + local[2] - hull->kdopMidPoint[7] ) ) )
        return qfalse;

    if ( !( hull->kdopHalfSize[8]
            > ( float )fabs( local[1] - local[2] - hull->kdopMidPoint[8] ) ) )
        return qfalse;

    for ( i = 0; i < hull->axisCount; i++ )
        if ( !( axes[i].halfSize
                > ( float )fabs( Vec3Dot( axes[i].dir, local ) - axes[i].midPoint ) ) )
            return qfalse;

    return qtrue;
}

/* LightGrid_PointLitByLight  0x004111c0 */
static qboolean LightGrid_PointLitByLight( const vec3_t origin, int lightIndex )
{
    const BspPrimaryLight_t *light = &bspPrimaryLights[lightIndex];
    vec3_t                   local;
    float                    lengthSq;
    unsigned                 hullCount;
    unsigned                 i;
    int                      hull;
    int                      axis;

    AssertCmp( light->type, ==, GFX_LIGHT_TYPE_OMNI );

    local[0] = origin[0] - light->origin[0];
    local[1] = origin[1] - light->origin[1];
    local[2] = origin[2] - light->origin[2];

    lengthSq = local[0] * local[0] + local[1] * local[1] + local[2] * local[2];

    if ( !( light->radius * light->radius > lengthSq ) )
        return qfalse;

    if ( light->type == GFX_LIGHT_TYPE_SPOT
         && -light->cosHalfFovOuter < light->rotationLimit )
    {
        float cosSwept = CosOfAngleSum( light->cosHalfFovOuter, light->rotationLimit );

        if ( cosSwept < 0.0f )
        {
            float dot = light->dir[0] * local[0]
                      + light->dir[1] * local[1]
                      + light->dir[2] * local[2];

            if ( dot < cosSwept * light->radius )
                return qfalse;
        }
        else if ( Vec3ConeDistance( light->origin, light->dir, cosSwept, origin ) > 0.0f )
        {
            return qfalse;
        }
    }

    hullCount = bspLightRegions[lightIndex];

    if ( !hullCount )
        return qtrue;

    hull = lightGridHullStart[lightIndex];
    axis = lightGridAxisStart[lightIndex];

    for ( i = 0; i < hullCount; i++ )
    {
        if ( LightGrid_PointInHull( local, &bspLightRegionHulls[hull],
                                    &bspLightRegionAxes[axis] ) )
            return qtrue;

        axis += bspLightRegionHulls[hull].axisCount;
        hull++;
    }

    return qfalse;
}

/* LightGrid_PointSeesSky  0x00411380 */
static qboolean LightGrid_PointSeesSky( const vec3_t origin )
{
    GeoTrace_t      trace;
    vec3_t          start;
    vec3_t          end;
    const GeoHit_t *lastHit;

    start[0] = options.sunDirection[0] * LIGHTGRID_CORNER_OFFSET + origin[0];
    start[1] = options.sunDirection[1] * LIGHTGRID_CORNER_OFFSET + origin[1];
    start[2] = options.sunDirection[2] * LIGHTGRID_CORNER_OFFSET + origin[2];

    end[0] = options.sunDirection[0] * LIGHTGRID_TRACE_DISTANCE + origin[0];
    end[1] = options.sunDirection[1] * LIGHTGRID_TRACE_DISTANCE + origin[1];
    end[2] = options.sunDirection[2] * LIGHTGRID_TRACE_DISTANCE + origin[2];

    Geo_SetupTrace( start, end, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    if ( !trace.result.hitCount )
        return qtrue;

    lastHit = &trace.result.hits[trace.result.hitCount - 1];

    AssertCmp( lastHit->geoType, ==, TRACE_HIT_WORLD_GEO );

    return ( lastHit->tri->mskMtl->material->surfaceFlags >> 2 ) & 1;
}

/* LightGrid_BoxSeesSky  0x00411470 */
static byte LightGrid_BoxSeesSky( const vec3_t origin )
{
    vec3_t mins;
    vec3_t maxs;
    vec3_t at;

    mins[0] = origin[0] - 16.0f;
    mins[1] = origin[1] - 16.0f;
    mins[2] = origin[2] - 32.0f;

    maxs[0] = origin[0] + 16.0f;
    maxs[1] = origin[1] + 16.0f;
    maxs[2] = origin[2] + 32.0f;

    for ( at[2] = mins[2]; at[2] <= maxs[2]; at[2] += 16.0f )
        for ( at[1] = mins[1]; at[1] <= maxs[1]; at[1] += 8.0f )
            for ( at[0] = mins[0]; at[0] <= maxs[0]; at[0] += 8.0f )
                if ( LightGrid_PointSeesSky( at ) )
                    return 0xff;

    return 0;
}

/* LightGrid_ChoosePrimaryLight  0x004115b0 */
static byte LightGrid_ChoosePrimaryLight( const vec3_t origin )
{
    int found = 0;
    int i;

    Assert( options.sunPrimaryLightIndex == PRIMARY_LIGHT_NONE
            || options.sunPrimaryLightIndex == PRIMARY_LIGHT_SUN );

    for ( i = options.sunPrimaryLightIndex + 1; i < numBSPPrimaryLights; i++ )
    {
        if ( !LightGrid_PointLitByLight( origin, i ) )
            continue;

        if ( found )
            return 0;

        found = i;
    }

    if ( found )
        return ( byte )found;

    return LightGrid_BoxSeesSky( origin );
}

/* LightGrid_QuantizeColors  0x00411630 */
static void LightGrid_QuantizeColors( const LightGridColors_t *colors,
                                      LightGridSample_t *out )
{
    const float *at;
    int          i;
    int          k;

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
        Lighting_Vec3LinearToGamma( ( float * )colors->color[i] );

    Lighting_ApplyContrast( GFX_LIGHTGRID_SAMPLE_COUNT, -1,
                            ( vec3_t * )colors->color );

    at = colors->color[0];

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        for ( k = 0; k < 3; k++ )
        {
            out->rgb[i][k] = Lighting_ColorToByte( ( float )( *at * 0.5 ) );
            at++;
        }
    }
}

/* LightGrid_TracePoint  0x004116b0 */
static void LightGrid_TracePoint( int pointIndex, int threadIndex )
{
    LightGridColors_t colors;
    TransportHit_t    hits[TRANSPORT_MAX_HITS];
    LightGridPoint_t *point = &lightGridGlob.points[pointIndex];
    vec3_t            origin;
    int               hitCount;
    int               lightType;
    int               lightCount;
    int               i;
    int               j;

    LightGrid_PointToOrigin( origin, point );

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        colors.color[i][0] = options.ambientColor[0];
        colors.color[i][1] = options.ambientColor[1];
        colors.color[i][2] = options.ambientColor[2];
    }

    for ( i = 0; i < options.skyTraceCount; i++ )
    {
        int result = Compile_TraceTransport( origin, lightGridSkyTraceDirs[i],
                                             LIGHTGRID_TRACE_DISTANCE, qtrue,
                                             hits, &hitCount, NULL, NULL );

        if ( result < TRANSPORT_SKY )
            continue;

        if ( result == TRANSPORT_SKY )
        {
            LightGrid_AddDirectionalColor( &colors, lightGridSkyTraceDirs[i],
                                           lightGridGlob.skyColor );
            continue;
        }

        for ( j = 0; j < hitCount; j++ )
        {
            vec3_t color;
            float  weight;

            Lighting_SampleColor( hits[j].sample->vars, color );

            weight = hits[j].weight * lightGridGlob.skyScale;

            color[0] = color[0] * weight;
            color[1] = color[1] * weight;
            color[2] = color[2] * weight;

            LightGrid_AddDirectionalColor( &colors, lightGridSkyTraceDirs[i], color );
        }
    }

    point->primaryLightIndex = 0;

    if ( LightGrid_PointSeesSky( origin ) )
    {
        if ( options.sunPrimaryLightIndex )
            point->primaryLightIndex = ( byte )options.sunPrimaryLightIndex;
        else
            LightGrid_AddDirectionalColor( &colors, options.sunDirection,
                                           options.sunColor );
    }

    if ( !point->primaryLightIndex )
        point->primaryLightIndex = LightGrid_ChoosePrimaryLight( origin );

    lightType  = point->primaryLightIndex;
    lightCount = PointLight_Count();

    for ( i = 0; i < lightCount; i++ )
    {
        vec3_t dir;
        vec3_t color;
        int    influence;

        influence = PointLight_Sample( lightType, i, origin, NULL, NULL, NULL,
                                       dir, color, NULL );

        if ( influence == LIGHT_INFLUENCE_DIRECTIONAL )
            LightGrid_AddDirectionalColor( &colors, dir, color );
        else if ( influence == LIGHT_INFLUENCE_COINCIDENT )
            LightGrid_AddAmbientColor( &colors, color );
    }

    LightGrid_QuantizeColors( &colors, &lightGridGlob.samples[pointIndex] );

    LightGrid_ComputeCornerMask( threadIndex, origin, &point->cornerMask );
}

/* LightGrid_AddSkySample  0x00411970 */
static void LightGrid_AddSkySample( void )
{
    LightGridColors_t colors;
    LightGridPoint_t *point;
    int               i;

    if ( lightGridGlob.pointCount == lightGridGlob.maxPoints )
        Error( "Couldn't allocate sky lighting light grid sample" );

    point = &lightGridGlob.points[lightGridGlob.pointCount];

    point->pos[0]      = 0xffff;
    point->pos[1]      = 0xffff;
    point->pos[2]      = 0xffff;
    point->needsTrace  = 0;
    point->colorsIndex = 0;
    point->cornerMask  = 0;

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        colors.color[i][0] = options.ambientColor[0];
        colors.color[i][1] = options.ambientColor[1];
        colors.color[i][2] = options.ambientColor[2];
    }

    for ( i = 0; i < options.skyTraceCount; i++ )
        if ( lightGridSkyTraceDirs[i][2] > 0.0f )
            LightGrid_AddDirectionalColor( &colors, lightGridSkyTraceDirs[i],
                                           lightGridGlob.skyColor );

    if ( options.sunPrimaryLightIndex )
    {
        point->primaryLightIndex = ( byte )options.sunPrimaryLightIndex;
    }
    else
    {
        point->primaryLightIndex = 0;
        LightGrid_AddDirectionalColor( &colors, options.sunDirection,
                                       options.sunColor );
    }

    LightGrid_QuantizeColors( &colors,
                              &lightGridGlob.samples[lightGridGlob.pointCount] );

    lightGridGlob.pointCount++;
}

/* LightGrid_AllocSkyTraces  0x00411aa0 */
static void LightGrid_AllocSkyTraces( void )
{
    int basisIndex;
    int x, y, z;
    int i;

    lightGridSkyTraceDirs = new ( std::nothrow ) vec3_t[options.skyTraceCount];

    if ( !lightGridSkyTraceDirs )
        Error( "Couldn't allocate %i bytes for grid trace directions",
               options.skyTraceCount * sizeof( vec3_t ) );

    for ( i = 0; i < options.skyTraceCount; i++ )
    {
        float heightFrac = rand() * LIGHTGRID_RAND_SCALE;
        float angleFrac  = rand() * LIGHTGRID_RAND_SCALE;

        SpreadPointOnSphere( heightFrac, angleFrac, lightGridSkyTraceDirs[i] );
    }

    basisIndex = 0;

    for ( z = 0; z < 4; z++ )
    {
        for ( y = 0; y < 4; y++ )
        {
            for ( x = 0; x < 4; x++ )
            {
                vec3_t dir;

                if ( x >= 1 && x <= 2 && y >= 1 && y <= 2 && z >= 1 && z <= 2 )
                    continue;

                dir[0] = x * ( 2.0f / 3.0f ) - 1.0f;
                dir[1] = y * ( 2.0f / 3.0f ) - 1.0f;
                dir[2] = z * ( 2.0f / 3.0f ) - 1.0f;

                Vec3NormalizeTo( dir, lightGridBasis[basisIndex] );

                basisIndex++;
            }
        }
    }

    SanityCheck( basisIndex == GFX_LIGHTGRID_SAMPLE_COUNT );

    SpreadPointsOnSphere( options.skyTraceCount, lightGridSkyTraceDirs,
                          sizeof( vec3_t ) );
}

/* LightGrid_BuildLightRegionStarts  0x0040f8f0 */
static void LightGrid_BuildLightRegionStarts( void )
{
    int i;

    lightGridHullStart[0] = 0;
    lightGridHullStart[1] = 0;

    for ( i = 1; i < numBSPPrimaryLights; i++ )
    {
        int start = lightGridHullStart[i];
        int total = lightGridAxisStart[i];
        int hull;

        lightGridHullStart[i + 1] = bspLightRegions[i - 1] + start;
        lightGridAxisStart[i + 1] = total;

        if ( start == lightGridHullStart[i + 1] )
            continue;

        for ( hull = start; hull < lightGridHullStart[i + 1]; hull++ )
            total += bspLightRegionHulls[hull].axisCount;

        lightGridAxisStart[i + 1] = total;
    }
}


/* LightGrid_ClusterAverage  0x00411c50 */
static void LightGrid_ClusterAverage( float *mean, const LightGridCluster_t *cluster )
{
    unsigned sums[sizeof( LightGridSample_t )];
    unsigned i;
    unsigned k;

    Assert( cluster );
    Assert( cluster->count );

    memset( sums, 0, sizeof( sums ) );

    for ( i = 0; i < ( unsigned )cluster->count; i++ )
    {
        const byte *sample = ( const byte * )
            &lightGridGlob.samples[lightGridIndexes[cluster->first + i]];

        for ( k = 0; k < sizeof( LightGridSample_t ); k++ )
            sums[k] += sample[k];
    }

    for ( k = 0; k < sizeof( LightGridSample_t ); k++ )
        mean[k] = ( float )( ( double )sums[k] / ( double )( unsigned )cluster->count );
}

/* LightGrid_ClusterVariance  0x00411ee0 */
static void LightGrid_ClusterVariance( LightGridCluster_t *cluster )
{
    float mean[sizeof( LightGridSample_t )];
    float var[sizeof( LightGridSample_t )];
    float count;
    int   i;
    int   k;

    Assert( cluster );
    Assert( cluster->count );

    LightGrid_ClusterAverage( mean, cluster );

    memset( var, 0, sizeof( var ) );

    for ( i = 0; i < cluster->count; i++ )
    {
        const byte *sample = ( const byte * )
            &lightGridGlob.samples[lightGridIndexes[cluster->first + i]];

        for ( k = 0; k < ( int )sizeof( LightGridSample_t ); k++ )
        {
            float diff = ( float )sample[k] - mean[k];

            var[k] += diff * diff;
        }
    }

    cluster->maxDev = -FLT_MAX;

    count = ( float )( unsigned )cluster->count;

    for ( k = 0; k < ( int )sizeof( LightGridSample_t ); k++ )
    {
        float variance = var[k] / count;
        float dev      = ( float )sqrt( variance );

        if ( cluster->maxDev < dev )
        {
            cluster->maxDev     = dev;
            cluster->splitAxis  = k;
            cluster->splitValue = mean[k];
        }
    }
}

/* LightGrid_WorstCluster  0x00412160 */
static LightGridCluster_t *LightGrid_WorstCluster( void )
{
    int best = 0;
    int i;

    for ( i = 1; i < lightGridClusterCount; i++ )
        if ( lightGridClusters[i].maxDev > lightGridClusters[best].maxDev )
            best = i;

    return &lightGridClusters[best];
}

/* LightGrid_SplitCluster  0x00412280 */
static void LightGrid_SplitCluster( LightGridCluster_t *cluster )
{
    LightGridCluster_t *other;
    unsigned            head;
    unsigned            tail;

    Assert( cluster );
    AssertCmp( cluster->count, >=, 2 );

    if ( cluster->maxDev > 0.0f )
    {
        head = cluster->first;
        tail = cluster->first + cluster->count - 1;

        for ( ;; )
        {
            const byte *sample;

            do
            {
                sample = ( const byte * )&lightGridGlob.samples[lightGridIndexes[head]];

                if ( ( float )sample[cluster->splitAxis] > cluster->splitValue )
                    break;

                head++;

                if ( head > tail )
                    goto done;
            }
            while ( 1 );

            for ( ;; )
            {
                sample = ( const byte * )&lightGridGlob.samples[lightGridIndexes[tail]];

                if ( !( ( float )sample[cluster->splitAxis] >= cluster->splitValue ) )
                    break;

                tail--;

                if ( head > tail )
                    goto done;
            }

            Assert( head < tail );

            {
                int swap = lightGridIndexes[tail];

                lightGridIndexes[tail] = lightGridIndexes[head];
                lightGridIndexes[head] = swap;
            }

            head++;
            tail--;

            if ( head > tail )
                goto done;
        }
    }
    else
    {
        head = cluster->first + ( ( unsigned )( cluster->count + 1 ) >> 1 );
        tail = head - 1;
    }

done:
    tail++;

    AssertCmp( head, ==, tail );
    Assert( head != 0 );
    Assert( head != ( unsigned )( cluster->first + cluster->count ) );

    other = &lightGridClusters[lightGridClusterCount];

    lightGridClusterCount++;

    other->first = head;
    other->count = cluster->first + cluster->count - head;

    cluster->count = head - cluster->first;

    LightGrid_ClusterVariance( other );
    LightGrid_ClusterVariance( cluster );
}

/* LightGrid_ClusterColor  0x004124b0 */
static void LightGrid_ClusterColor( const LightGridCluster_t *cluster,
                                    BspLightGridColor_t *out )
{
    float mean[sizeof( LightGridSample_t )];
    int   i;

    LightGrid_ClusterAverage( mean, cluster );

    for ( i = 0; i < ( int )sizeof( LightGridSample_t ); i++ )
        out->data[i] = ( byte )( int )( mean[i] + 0.5 );
}

/* LightGrid_ColorDistance  0x00412510 */
static qboolean LightGrid_ColorDistance( const byte *a, int *best, const byte *b )
{
    int total = 0;
    int i;
    int k;

    for ( i = 0; i < GFX_LIGHTGRID_SAMPLE_COUNT; i++ )
    {
        for ( k = 0; k < 3; k++ )
        {
            int diff = a[i * 3 + k] - b[i * 3 + k];

            total += diff * diff;

            if ( total >= *best )
                return qfalse;
        }
    }

    *best = total;

    return qtrue;
}

/* LightGrid_MapColor  0x00412580 */
static unsigned short LightGrid_MapColor( const byte *color, unsigned short index )
{
    unsigned short best;
    unsigned short i;
    int            dist = INT_MAX;

    LightGrid_ColorDistance( color, &dist,
                             ( const byte * )&lightGridGlob.samples[index] );

    if ( !dist )
        return index;

    best = index;

    for ( i = 0; i < ( unsigned short )lightGridClusterCount; i++ )
    {
        if ( !LightGrid_ColorDistance( color, &dist,
                                       ( const byte * )&lightGridGlob.samples[i] ) )
            continue;

        best = i;

        if ( !dist )
            break;
    }

    return best;
}

/* LightGrid_RemapPoint  0x00412610 */
static void LightGrid_RemapPoint( int pointIndex, int threadIndex )
{
    LightGridPoint_t *point = &lightGridGlob.points[pointIndex];

    ( void )threadIndex;

    point->colorsIndex =
        LightGrid_MapColor( ( const byte * )&lightGridGlob.samples[pointIndex],
                            point->colorsIndex );
}

/* LightGrid_SwapClusters  0x00412650 */
static void LightGrid_SwapClusters( int a, int b )
{
    BspLightGridColor_t color;
    LightGridCluster_t  cluster;
    int                 i;

    color                 = bspLightGridColors[b];
    bspLightGridColors[b] = bspLightGridColors[a];
    bspLightGridColors[a] = color;

    cluster                = lightGridClusters[b];
    lightGridClusters[b]   = lightGridClusters[a];
    lightGridClusters[a]   = cluster;

    for ( i = 0; i < lightGridGlob.pointCount; i++ )
    {
        if ( lightGridGlob.points[i].colorsIndex == b )
            lightGridGlob.points[i].colorsIndex = ( unsigned short )a;
        else if ( lightGridGlob.points[i].colorsIndex == a )
            lightGridGlob.points[i].colorsIndex = ( unsigned short )b;
    }
}

/* LightGrid_ImproveQuantization  0x00412760 */
static void LightGrid_ImproveQuantization( int threads )
{
    unsigned ( *sums )[sizeof( LightGridSample_t )];
    int      *counts;
    int       i;
    int       c;

    sums = ( unsigned ( * )[sizeof( LightGridSample_t )] )
        new ( std::nothrow ) unsigned[lightGridClusterCount
                                      * sizeof( LightGridSample_t )];

    if ( !sums )
        Error( "Couldn't allocate %i bytes for light grid color sums",
               lightGridClusterCount * sizeof( LightGridSample_t )
               * sizeof( unsigned ) );

    memset( sums, 0, lightGridClusterCount * sizeof( LightGridSample_t )
                     * sizeof( unsigned ) );

    counts = new ( std::nothrow ) int[lightGridClusterCount];

    if ( !counts )
        Error( "Couldn't allocate %i bytes for light grid color counts",
               options.lightGridColorLimit * sizeof( int ) );

    memset( counts, 0, lightGridClusterCount * sizeof( int ) );

    StartProgress( "Improving quantization..." );
    RunThreadsOn( lightGridGlob.pointCount, LightGrid_RemapPoint, threads );
    EndProgress();

    for ( i = 0; i < lightGridGlob.pointCount; i++ )
    {
        int         index  = lightGridGlob.points[i].colorsIndex;
        const byte *sample = ( const byte * )&lightGridGlob.samples[i];
        unsigned    k;

        counts[index]++;

        for ( k = 0; k < sizeof( LightGridSample_t ); k++ )
            sums[index][k] += sample[k];
    }

    for ( c = 0; c < numBSPLightGridColors; )
    {
        unsigned k;

        if ( !counts[c] )
        {
            numBSPLightGridColors--;

            counts[c] = counts[numBSPLightGridColors];

            memcpy( sums[c], sums[numBSPLightGridColors], sizeof( sums[0] ) );

            for ( i = 0; i < lightGridGlob.pointCount; i++ )
                if ( lightGridGlob.points[i].colorsIndex == numBSPLightGridColors )
                    lightGridGlob.points[i].colorsIndex = ( unsigned short )c;

            continue;
        }

        for ( k = 0; k < sizeof( LightGridSample_t ); k++ )
            bspLightGridColors[c].data[k] =
                ( byte )( ( sums[c][k] + counts[c] / 2 ) / ( unsigned )counts[c] );

        c++;
    }

    delete[] counts;
    delete[] ( unsigned * )sums;
}

/* LightGrid_ClusterSunCount  0x00412a60 */
static int LightGrid_ClusterSunCount( const LightGridCluster_t *cluster )
{
    int count = 0;
    int i;

    for ( i = 0; i < cluster->count; i++ )
    {
        int index = lightGridIndexes[cluster->first + i];

        if ( index == lightGridGlob.pointCount - 1 )
            return 0;

        if ( lightGridGlob.points[index].primaryLightIndex
             == options.sunPrimaryLightIndex )
            count++;
    }

    return count;
}

/* LightGrid_Quantize  0x00412ad0 */
static void LightGrid_Quantize( int threads )
{
    int best      = 0;
    int bestCount = 0;
    int i;
    int c;

    lightGridIndexes = new ( std::nothrow ) int[lightGridGlob.pointCount];

    if ( !lightGridIndexes )
        Error( "Couldn't allocate %i bytes for light grid color mapping",
               lightGridGlob.pointCount * sizeof( int ) );

    lightGridClusters =
        new ( std::nothrow ) LightGridCluster_t[options.lightGridColorLimit];

    if ( !lightGridClusters )
        Error( "Couldn't allocate %i bytes for light grid colors",
               options.lightGridColorLimit * sizeof( LightGridCluster_t ) );

    for ( i = 0; i < lightGridGlob.pointCount; i++ )
        lightGridIndexes[i] = i;

    lightGridClusterCount     = 1;
    lightGridClusters[0].first = 0;
    lightGridClusters[0].count = lightGridGlob.pointCount;

    LightGrid_ClusterVariance( &lightGridClusters[0] );

    while ( lightGridClusterCount < options.lightGridColorLimit )
    {
        LightGridCluster_t *worst = LightGrid_WorstCluster();

        if ( !( options.lightGridColorTolerance < worst->maxDev )
             && lightGridClusterCount >= 2 )
            break;

        LightGrid_SplitCluster( worst );
    }

    for ( c = 0; c < lightGridClusterCount; c++ )
    {
        LightGridCluster_t *cluster = &lightGridClusters[c];
        int                 sunCount;
        int                 k;

        for ( k = cluster->first; k < cluster->first + cluster->count; k++ )
            lightGridGlob.points[lightGridIndexes[k]].colorsIndex =
                ( unsigned short )c;

        LightGrid_ClusterColor( cluster, &bspLightGridColors[c] );

        sunCount = LightGrid_ClusterSunCount( cluster );

        if ( bestCount < sunCount )
        {
            bestCount = sunCount;
            best      = c;
        }
    }

    LightGrid_SwapClusters( best, 0 );
    LightGrid_SwapClusters(
        lightGridGlob.points[lightGridGlob.pointCount - 1].colorsIndex, 1 );

    numBSPLightGridColors = lightGridClusterCount;

    if ( options.extraQuality )
        LightGrid_ImproveQuantization( threads );

    delete[] lightGridClusters;
    lightGridClusters = NULL;

    delete[] lightGridIndexes;
    lightGridIndexes = NULL;

    lightGridGlob.pointCount--;
}


/* LightGrid_AddSkipRun  0x00412cf0 */
static void LightGrid_AddSkipRun( unsigned count )
{
    while ( count > LIGHTGRID_MAX_RUN )
    {
        AssertCmp( numBSPLightGridRowBytes + 2, <, MAX_MAP_LIGHTGRID_POINTS * 3 );

        bspLightGridRowBytes[numBSPLightGridRowBytes]     = LIGHTGRID_MAX_RUN;
        bspLightGridRowBytes[numBSPLightGridRowBytes + 1] = 0;
        numBSPLightGridRowBytes += 2;

        count -= LIGHTGRID_MAX_RUN;
    }

    AssertCmp( numBSPLightGridRowBytes + 2, <, MAX_MAP_LIGHTGRID_POINTS * 3 );

    bspLightGridRowBytes[numBSPLightGridRowBytes]     = ( byte )count;
    bspLightGridRowBytes[numBSPLightGridRowBytes + 1] = 0;
    numBSPLightGridRowBytes += 2;
}

/* LightGrid_AddSkyEntry  0x00412dc0 */
static void LightGrid_AddSkyEntry( void )
{
    AssertCmp( numBSPLightGridEntries, <, MAX_MAP_LIGHTGRID_POINTS );

    bspLightGridEntries[numBSPLightGridEntries].colorsIndex       = 0;
    bspLightGridEntries[numBSPLightGridEntries].primaryLightIndex =
        ( byte )options.sunPrimaryLightIndex;
    bspLightGridEntries[numBSPLightGridEntries].needsTrace        = 0;

    numBSPLightGridEntries++;
}

/* LightGrid_AddEntry  0x00412e20 */
static void LightGrid_AddEntry( const LightGridPoint_t *point )
{
    AssertCmp( numBSPLightGridEntries, <, MAX_MAP_LIGHTGRID_POINTS );

    bspLightGridEntries[numBSPLightGridEntries].colorsIndex       = point->colorsIndex;
    bspLightGridEntries[numBSPLightGridEntries].primaryLightIndex = point->primaryLightIndex;
    bspLightGridEntries[numBSPLightGridEntries].needsTrace        = point->cornerMask;

    numBSPLightGridEntries++;
}

/* LightGrid_EncodeBlock  0x00412e80 */
static void LightGrid_EncodeBlock( int runCount, const unsigned short *blockRange,
                                   const unsigned short *rowRange,
                                   int beginBlock, int endBlock )
{
    LightGridPoint_t *points = lightGridGlob.points;
    int               height = blockRange[1] - blockRange[0] + 1;
    int               pointIndex;
    int               colOffset;
    int               row;
    int               rowHeight;
    int               offset;
    qboolean          wideOffset;
    int               left;

    if ( height > LIGHTGRID_MAX_RUN )
        Error( "light grid vertical variation is too extreme" );

    AssertCmp( runCount * height, >=, endBlock - beginBlock );

    pointIndex = beginBlock;

    for ( colOffset = 0; colOffset < runCount; colOffset++ )
    {
        for ( row = 0; row < height; row++ )
        {
            if ( points[pointIndex].pos[2] != points[beginBlock].pos[2] + row )
            {
                LightGrid_AddSkyEntry();
                continue;
            }

            AssertCmp( points[pointIndex].pos[bspLightGridHeader.colAxis], ==,
                       points[beginBlock].pos[bspLightGridHeader.colAxis] + colOffset );

            LightGrid_AddEntry( &points[pointIndex] );

            pointIndex++;
        }
    }

    AssertCmp( pointIndex, ==, endBlock );

    rowHeight  = rowRange[1] - rowRange[0] + 1;
    offset     = blockRange[0] - rowRange[0];
    wideOffset = rowHeight > LIGHTGRID_MAX_RUN;

    left = runCount;

    do
    {
        int chunk = left > LIGHTGRID_MAX_RUN ? LIGHTGRID_MAX_RUN : left;

        AssertCmp( numBSPLightGridRowBytes + 3, <, MAX_MAP_LIGHTGRID_POINTS * 3 );

        bspLightGridRowBytes[numBSPLightGridRowBytes]     = ( byte )chunk;
        bspLightGridRowBytes[numBSPLightGridRowBytes + 1] = ( byte )height;
        bspLightGridRowBytes[numBSPLightGridRowBytes + 2] = ( byte )offset;
        numBSPLightGridRowBytes += 3;

        if ( wideOffset )
        {
            bspLightGridRowBytes[numBSPLightGridRowBytes] = ( byte )( offset >> 8 );
            numBSPLightGridRowBytes++;
        }

        left -= chunk;
    }
    while ( left );
}

/* LightGrid_EncodeRow  0x00413160 */
static void LightGrid_EncodeRow( int rowFirst, int rowEnd,
                                 const unsigned short *rowRange )
{
    LightGridPoint_t *points  = lightGridGlob.points;
    int               colAxis = bspLightGridHeader.colAxis;
    unsigned short    header[6];
    unsigned short    blockRange[2];
    unsigned short    range[2];
    int               blockStart;
    unsigned short    prevCol;
    unsigned short    runCount;
    int               i;
    int               next;

    header[0] = points[rowFirst].pos[colAxis];
    header[1] = ( unsigned short )( points[rowEnd - 1].pos[colAxis]
                                    - points[rowFirst].pos[colAxis] + 1 );
    header[2] = rowRange[0];
    header[3] = ( unsigned short )( rowRange[1] - rowRange[0] + 1 );
    *( int * )&header[4] = numBSPLightGridEntries;

    memcpy( &bspLightGridRowBytes[numBSPLightGridRowBytes], header, sizeof( header ) );
    numBSPLightGridRowBytes += sizeof( header );

    blockStart    = 0;
    blockRange[0] = 0xffff;
    blockRange[1] = 0;
    prevCol       = 0xffff;
    runCount      = 0;

    for ( i = rowFirst; i != rowEnd; i = next )
    {
        unsigned short col = points[i].pos[colAxis];

        for ( next = i + 1; next != rowEnd; next++ )
            if ( points[next].pos[colAxis] != col )
                break;

        range[0] = points[i].pos[2];
        range[1] = points[next - 1].pos[2];

        if ( col == ( unsigned short )( prevCol + runCount )
             && range[0] == blockRange[0]
             && range[1] == blockRange[1]
             && runCount < LIGHTGRID_MAX_RUN )
        {
            runCount++;
            continue;
        }

        if ( runCount )
        {
            LightGrid_EncodeBlock( runCount, blockRange, rowRange, blockStart, i );

            if ( col != ( unsigned short )( prevCol + runCount ) )
                LightGrid_AddSkipRun( col - prevCol - runCount );
        }

        blockRange[0] = range[0];
        blockRange[1] = range[1];
        blockStart    = i;
        prevCol       = col;
        runCount      = 1;
    }

    LightGrid_EncodeBlock( runCount, blockRange, rowRange, blockStart, rowEnd );

    numBSPLightGridRowBytes = ( numBSPLightGridRowBytes + 3 ) & ~3;
}

/* LightGrid_EncodeRows  0x00413320 */
static void LightGrid_EncodeRows( void )
{
    LightGridPoint_t *points  = lightGridGlob.points;
    int               rowAxis = bspLightGridHeader.rowAxis;
    int               first   = 0;

    Assert( numBSPLightGridEntries == 0 );

    while ( first < lightGridGlob.pointCount )
    {
        unsigned short rowRange[2];
        unsigned short row = points[first].pos[rowAxis];
        int            rowIndex;
        int            count;
        int            i;

        rowRange[0] = points[first].pos[2];
        rowRange[1] = points[first].pos[2];
        count       = 1;

        for ( i = first + 1; i < lightGridGlob.pointCount; i++ )
        {
            unsigned short z;

            if ( points[i].pos[rowAxis] != row )
                break;

            z = points[i].pos[2];

            if ( rowRange[0] > z )
                rowRange[0] = z;

            if ( rowRange[1] < z )
                rowRange[1] = z;

            count++;
        }

        rowIndex = row - bspLightGridHeader.mins[rowAxis];

        bspLightGridHeader.rowDataStart[rowIndex] =
            ( unsigned short )( numBSPLightGridRowBytes / 4 );

        AssertCmp( ( unsigned )( bspLightGridHeader.rowDataStart[rowIndex] * 4 ), ==,
                   ( unsigned )numBSPLightGridRowBytes );

        LightGrid_EncodeRow( first, first + count, rowRange );

        Assert( bspLightGridHeader.rowDataStart[rowIndex] != numBSPLightGridRowBytes );

        first += count;
    }

    AssertCmp( numBSPLightGridEntries, >=, lightGridGlob.pointCount );
}


/* LightGrid_SortPoints  0x004134a0 */
static void LightGrid_SortPoints( void )
{
    int i;

    bspLightGridHeader.maxs[2] = 0;

    if ( !lightGridGlob.pointCount )
    {
        bspLightGridHeader.mins[0] = 0;
        bspLightGridHeader.mins[1] = 0;
        bspLightGridHeader.mins[2] = 0;
        bspLightGridHeader.maxs[0] = 0;
        bspLightGridHeader.maxs[1] = 0;
        return;
    }

    bspLightGridHeader.mins[0] = 0xffff;
    bspLightGridHeader.mins[1] = 0xffff;
    bspLightGridHeader.mins[2] = 0xffff;
    bspLightGridHeader.maxs[0] = 0;
    bspLightGridHeader.maxs[1] = 0;

    for ( i = 0; i < lightGridGlob.pointCount; i++ )
    {
        const LightGridPoint_t *point = &lightGridGlob.points[i];
        int                     axis;

        for ( axis = 0; axis < 3; axis++ )
        {
            if ( bspLightGridHeader.mins[axis] > point->pos[axis] )
                bspLightGridHeader.mins[axis] = point->pos[axis];

            if ( bspLightGridHeader.maxs[axis] < point->pos[axis] )
                bspLightGridHeader.maxs[axis] = point->pos[axis];
        }
    }

    if ( bspLightGridHeader.maxs[1] - bspLightGridHeader.mins[1]
         < bspLightGridHeader.maxs[0] - bspLightGridHeader.mins[0] )
    {
        bspLightGridHeader.rowAxis = 1;
        bspLightGridHeader.colAxis = 0;
    }
    else
    {
        bspLightGridHeader.rowAxis = 0;
        bspLightGridHeader.colAxis = 1;
    }

    memset( bspLightGridHeader.rowDataStart, 0xff,
            sizeof( bspLightGridHeader.rowDataStart ) );

    std::sort( lightGridGlob.points,
               lightGridGlob.points + lightGridGlob.pointCount,
               LightGrid_PointLessRef );
}

/* LightGrid_Compile  0x00413630 */
void LightGrid_Compile( int threads )
{
    float skyScale;

    numBSPLightGridColors   = 0;
    numBSPLightGridEntries  = 0;
    numBSPLightGridRowBytes = 0;

    LightGrid_LoadPoints();
    LightGrid_SortPoints();
    LightGrid_SuppressPoints();
    LightGrid_ExcludePoints();
    LightGrid_SortPoints();
    LightGrid_InsertMissingPoints();

    if ( !lightGridGlob.pointCount )
        return;

    if ( lightGridGlob.pointCount > MAX_MAP_LIGHTGRID_POINTS )
        Error( "Need %i light grid sample points, but the limit is %i\n",
               lightGridGlob.pointCount, MAX_MAP_LIGHTGRID_POINTS );

    AssertCmp( bspLightGridHeader.mins[0], <=, bspLightGridHeader.maxs[0] );
    AssertCmp( bspLightGridHeader.mins[1], <=, bspLightGridHeader.maxs[1] );
    AssertCmp( bspLightGridHeader.mins[2], <=, bspLightGridHeader.maxs[2] );

    if ( bspLightGridHeader.maxs[0] > LIGHTGRID_MAX_XY
         || bspLightGridHeader.maxs[1] > LIGHTGRID_MAX_XY
         || bspLightGridHeader.maxs[2] > LIGHTGRID_MAX_Z )
        Error( "light grid extends past world bounds" );

    LightGrid_AllocSkyTraces();

    skyScale = ( float )( 4.0 / options.skyTraceCount );

    lightGridGlob.skyColor[0] = options.sunRadiosityColor[0] * skyScale;
    lightGridGlob.skyColor[1] = options.sunRadiosityColor[1] * skyScale;
    lightGridGlob.skyColor[2] = options.sunRadiosityColor[2] * skyScale;
    lightGridGlob.skyScale    = skyScale;

    lightGridGlob.samples =
        new ( std::nothrow ) LightGridSample_t[lightGridGlob.pointCount + 1];

    if ( !lightGridGlob.samples )
        Error( "Couldn't allocate %i bytes to hold light grid colors",
               ( lightGridGlob.pointCount + 1 ) * sizeof( LightGridSample_t ) );

    LightGrid_BuildLightRegionStarts();

    StartProgress( "Calculating light grid..." );
    RunThreadsOn( lightGridGlob.pointCount, LightGrid_TracePoint, threads );
    EndProgress();

    LightGrid_AddSkySample();

    Print( "----------------------------------------\n"
           "Quantizing light grid colors...\n" );

    LightGrid_Quantize( threads );

    Print( "----------------------------------------\n"
           "Encoding light grid...\n" );

    LightGrid_EncodeRows();

    delete[] lightGridGlob.samples;
}

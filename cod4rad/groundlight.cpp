/* Original: c:\trees\cod3\cod3src\cod2rad\groundlight.cpp */

#include "groundlight.h"
#include "geometry.h"
#include "lighting.h"
#include "compile.h"
#include "pointlights.h"
#include "cmdline.h"
#include "progress.h"
#include "com_math.h"

#include <string.h>
#include <new>


#define GROUND_MAX_HITS     ( GROUND_MAX_POINTS * TRANSPORT_MAX_HITS )


GroundLight_t *groundLights;                /* 0x130632a4 */


/* GroundLight_DominantLight  0x0040df60 */
static int GroundLight_DominantLight( int hitCount, const TransportHit_t *hits )
{
    int   lightIndexes[GROUND_MAX_HITS];
    float weights[GROUND_MAX_HITS];
    float best        = 0.0f;
    int   bestIndex   = 0;
    int   uniqueCount = 0;
    int   i;

    for ( i = 0; i < hitCount; i++ )
    {
        int slot;

        for ( slot = 0; slot < uniqueCount; slot++ )
        {
            if ( lightIndexes[slot] == hits[i].primaryLightIndex )
                break;
        }

        if ( slot < uniqueCount )
        {
            weights[slot] += hits[i].weight;
        }
        else
        {
            lightIndexes[slot] = hits[i].primaryLightIndex;
            weights[slot]      = hits[i].weight;
            uniqueCount++;
        }

        if ( weights[slot] > best )
        {
            best      = weights[slot];
            bestIndex = lightIndexes[slot];
        }
    }

    return bestIndex;
}

/* GroundLight_Compute  0x0040e080 */
static qboolean GroundLight_Compute( const GroundLight_t *gl, int *outLightIndex,
                                     float *outScale, vec3_t outColor )
{
    TransportHit_t hits[GROUND_MAX_HITS];
    GeoTrace_t     trace;
    vec3_t         dir;
    vec3_t         hitPos;
    vec3_t         normal;
    vec3_t         color;
    float          distance;
    float          totalWeight;
    float          visibility;
    int            lightIndex;
    int            hitCount = 0;
    unsigned       i;

    dir[0] = -gl->dir[0];
    dir[1] = -gl->dir[1];
    dir[2] = -gl->dir[2];

    distance = ( float )( gl->distance + GROUND_TRACE_EXTRA );

    for ( i = 0; i < gl->pointCount; i++ )
    {
        int count;

        Geo_SetupTrace( gl->origin, gl->points[i], NULL, NULL, &trace );
        Geo_TraceRay( &trace );

        if ( !( Geo_TraceOpenFraction( &trace.result ) > GROUND_MIN_OPEN ) )
            continue;

        if ( Compile_TraceTransport( gl->points[i], dir, distance, qfalse,
                                     &hits[hitCount], &count, hitPos, normal )
             >= TRANSPORT_HIT )
            hitCount += count;
    }

    if ( !hitCount )
        return qfalse;

    lightIndex = GroundLight_DominantLight( hitCount, hits );

    totalWeight = 0.0f;
    *outScale   = 0.0f;

    outColor[0] = 0.0f;
    outColor[1] = 0.0f;
    outColor[2] = 0.0f;

    for ( i = 0; i < ( unsigned )hitCount; i++ )
    {
        const TransportHit_t *hit = &hits[i];

        if ( hit->primaryLightIndex != lightIndex )
            continue;

        totalWeight += hit->weight;

        *outScale += Lighting_AverageSubSamples( hit->lmapIndex, hit->s, hit->t )
                   * hit->weight;

        Lighting_SampleLightmap( hit->lmapIndex, hit->s, hit->t, normal, color );

        outColor[0] += color[0] * hit->weight;
        outColor[1] += color[1] * hit->weight;
        outColor[2] += color[2] * hit->weight;
    }

    Assertx( totalWeight > 0.0f, "%s", "totalWeight > 0.0f" );

    *outScale /= totalWeight;

    {
        float scale = 1.0f / totalWeight;

        outColor[0] = outColor[0] * scale;
        outColor[1] = outColor[1] * scale;
        outColor[2] = outColor[2] * scale;
    }

    if ( lightIndex == options.sunPrimaryLightIndex )
        visibility = Clamp( Vec3Dot( normal, options.sunDirection ), 0.0f, 1.0f );
    else if ( lightIndex )
        visibility = PointLight_TypeVisibility( lightIndex, hitPos, normal );
    else
        visibility = 0.0f;

    *outScale = Clamp( Lighting_LinearToGamma( *outScale ) * visibility, 0.0f, 1.0f );

    outColor[0] = Clamp( outColor[0], 0.0f, 1.0f );
    outColor[1] = Clamp( outColor[1], 0.0f, 1.0f );
    outColor[2] = Clamp( outColor[2], 0.0f, 1.0f );

    *outLightIndex = lightIndex;

    return qtrue;
}

/* GroundLight_Format  0x0040e3f0 */
static void GroundLight_Format( const vec3_t color, char *value, int size,
                                int lightIndex, float scale )
{
    byte red   = Lighting_ColorToByte( color[0] * 0.5f );
    byte green = Lighting_ColorToByte( color[1] * 0.5f );
    byte blue  = Lighting_ColorToByte( color[2] * 0.5f );
    byte level = Lighting_ColorToByte( scale );

    Com_sprintf( value, size, "%02x%02x%02x%02x%02x",
                 red, green, blue, level, lightIndex );
}

/* GroundLight_LightModel  0x0040e480 */
static void GroundLight_LightModel( GroundLight_t *gl )
{
    char   value[16];
    vec3_t color;
    float  scale;
    int    lightIndex;

    if ( !GroundLight_Compute( gl, &lightIndex, &scale, color ) )
        return;

    GroundLight_Format( color, value, sizeof( value ), lightIndex, scale );

    SetKeyValue( gl->ent, "gndLt", value );
}

/* GroundLight_LightModels  0x0040e4e0 */
void GroundLight_LightModels( void )
{
    while ( groundLights )
    {
        GroundLight_t *gl = groundLights;
        char           value[16];
        vec3_t         color;
        float          scale;
        int            lightIndex;

        if ( GroundLight_Compute( gl, &lightIndex, &scale, color ) )
        {
            GroundLight_Format( color, value, sizeof( value ), lightIndex, scale );

            SetKeyValue( gl->ent, "gndLt", value );
        }

        groundLights = gl->next;

        delete gl;
    }
}

/* GroundLight_Add  0x0040e560 */
void GroundLight_Add( Entity_t *ent, const vec3_t *points, unsigned pointCount,
                      const vec3_t origin, float distance, const vec3_t dir )
{
    GroundLight_t *gl = new ( std::nothrow ) GroundLight_t;

    if ( !gl )
        Error( "Out of memory on ground lit model" );

    gl->ent = ent;

    gl->origin[0] = origin[0];
    gl->origin[1] = origin[1];
    gl->origin[2] = origin[2];

    gl->dir[0] = dir[0];
    gl->dir[1] = dir[1];
    gl->dir[2] = dir[2];

    gl->distance = distance;

    memcpy( gl->points, points, pointCount * sizeof( vec3_t ) );

    gl->pointCount = pointCount;
    gl->next       = groundLights;

    groundLights = gl;
}

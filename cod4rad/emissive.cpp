/* Emissive brushes: glowing faces treated as area lights.

   Each point being lit picks random points on the glowing faces and adds up their
   light the way a Lambertian area light does:

       light from one sample = L * cos(emitter) * area / distance^2

   where the surface shows color C when the brush is large compared to its distance,
   which makes the radiance L = C / pi.  Every sample is tested for shadows with the
   same traces the point lights use. */

#include "cod4rad.h"
#include "emissive.h"
#include "geometry.h"
#include "modelcollision.h"
#include "progress.h"

#include <algorithm>
#include <math.h>
#include <vector>


#define EMISSIVE_PI             3.14159265f
#define EMISSIVE_SURFACE_OFFSET 0.5f        /* sample points sit this far off the face */
#define EMISSIVE_START_OFFSET   0.125f      /* shadow rays start this far off the lit point */
#define EMISSIVE_MIN_DIST_SQ    16.0f       /* stops a point right next to a face going to infinity */


typedef struct
{
    vec3_t v0;
    vec3_t e1;
    vec3_t e2;
    vec3_t normal;
} EmissiveTri_t;

typedef struct
{
    vec3_t color;
    float  radius;
    int    sampleCount;
    float  totalArea;
    vec3_t mins;
    vec3_t maxs;
    size_t firstTri;
    size_t triCount;
} EmissiveBrush_t;

static std::vector<EmissiveBrush_t> emissiveBrushes;
static std::vector<EmissiveTri_t>   emissiveTris;
static std::vector<float>           emissiveCumulativeArea;     /* running total, per triangle */


void Emissive_BeginBrush( const vec3_t color, float radius, int sampleCount )
{
    EmissiveBrush_t brush;

    Vec3Copy( color, brush.color );
    brush.radius      = radius;
    brush.sampleCount = sampleCount;
    brush.totalArea   = 0.0f;
    brush.firstTri    = emissiveTris.size();
    brush.triCount    = 0;

    ClearBounds( brush.mins, brush.maxs );

    emissiveBrushes.push_back( brush );
}

void Emissive_AddTriangle( const vec3_t v0, const vec3_t v1, const vec3_t v2,
                           const vec3_t outwardNormal )
{
    EmissiveBrush_t *brush = &emissiveBrushes.back();
    EmissiveTri_t    tri;
    vec3_t           cross;
    float            area;

    Vec3Copy( v0, tri.v0 );
    Vec3Sub( v1, v0, tri.e1 );
    Vec3Sub( v2, v0, tri.e2 );
    Vec3Copy( outwardNormal, tri.normal );

    Vec3Cross( tri.e1, tri.e2, cross );
    area = Vec3Length( cross ) * 0.5f;

    if ( !( area > 0.0f ) )
        return;

    brush->totalArea += area;
    brush->triCount++;

    AddPointToBounds( v0, brush->mins, brush->maxs );
    AddPointToBounds( v1, brush->mins, brush->maxs );
    AddPointToBounds( v2, brush->mins, brush->maxs );

    emissiveTris.push_back( tri );
    emissiveCumulativeArea.push_back( brush->totalArea );
}

void Emissive_EndBrush( void )
{
    EmissiveBrush_t *brush = &emissiveBrushes.back();

    if ( !brush->triCount )
        emissiveBrushes.pop_back();
}

bool Emissive_Active( void )
{
    return !emissiveBrushes.empty();
}

/* A small repeatable random number generator: the same point and seed give the same samples */
static float Emissive_Random( unsigned *state )
{
    unsigned s = *state * 747796405u + 2891336453u;
    unsigned w = ( ( s >> ( ( s >> 28u ) + 4u ) ) ^ s ) * 277803737u;

    *state = s;

    return ( float )( ( ( w >> 22u ) ^ w ) >> 8 ) * ( 1.0f / 16777216.0f );
}

/* How much of the way from start to end is open (0 to 1) */
static float Emissive_Visibility( const vec3_t start, const vec3_t end, bool withModels )
{
    GeoTrace_t trace;
    float      open;

    Geo_SetupTrace( start, end, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    open = Geo_TraceOpenFraction( &trace.result );

    if ( open == 0.0f || !withModels )
        return open;

    Model_TraceLine( &trace );

    return Geo_TraceOpenFraction( &trace.result );
}

bool Emissive_Gather( const vec3_t pos, const vec3_t normal, unsigned seed,
                      vec3_t outColor, vec3_t outDir, float *outWeight )
{
    vec3_t sum        = { 0.0f, 0.0f, 0.0f };
    vec3_t dirSum     = { 0.0f, 0.0f, 0.0f };
    float  lumSum     = 0.0f;
    float  cosLumSum  = 0.0f;
    size_t b;

    for ( b = 0; b < emissiveBrushes.size(); b++ )
    {
        const EmissiveBrush_t *brush = &emissiveBrushes[b];
        const float            radiusSq = brush->radius * brush->radius;
        unsigned               rng = seed ^ ( ( unsigned )b * 2654435761u );
        int                    i;

        if ( pos[0] < brush->mins[0] - brush->radius || pos[0] > brush->maxs[0] + brush->radius
          || pos[1] < brush->mins[1] - brush->radius || pos[1] > brush->maxs[1] + brush->radius
          || pos[2] < brush->mins[2] - brush->radius || pos[2] > brush->maxs[2] + brush->radius )
            continue;

        for ( i = 0; i < brush->sampleCount; i++ )
        {
            /* Pick a triangle in proportion to its area, one sample per slice of the total */
            const float  target = ( i + Emissive_Random( &rng ) ) / brush->sampleCount
                                * brush->totalArea;
            const float *first  = &emissiveCumulativeArea[brush->firstTri];
            const float *last   = first + brush->triCount;
            const float *found  = std::lower_bound( first, last, target );
            size_t       triIndex;
            const EmissiveTri_t *tri;
            float        r1, r2;
            vec3_t       point, delta, dir, start;
            float        distSq, dist, cosEmit, cosRecv, falloff, amount, open, lum;
            vec3_t       contribution;

            if ( found == last )
                found = last - 1;

            triIndex = brush->firstTri + ( size_t )( found - first );
            tri      = &emissiveTris[triIndex];

            /* A random point on that triangle */
            r1 = Emissive_Random( &rng );
            r2 = Emissive_Random( &rng );

            if ( r1 + r2 > 1.0f )
            {
                r1 = 1.0f - r1;
                r2 = 1.0f - r2;
            }

            point[0] = tri->v0[0] + tri->e1[0] * r1 + tri->e2[0] * r2
                     + tri->normal[0] * EMISSIVE_SURFACE_OFFSET;
            point[1] = tri->v0[1] + tri->e1[1] * r1 + tri->e2[1] * r2
                     + tri->normal[1] * EMISSIVE_SURFACE_OFFSET;
            point[2] = tri->v0[2] + tri->e1[2] * r1 + tri->e2[2] * r2
                     + tri->normal[2] * EMISSIVE_SURFACE_OFFSET;

            Vec3Sub( point, pos, delta );

            distSq = Vec3Dot( delta, delta );

            if ( distSq >= radiusSq || distSq < 1.0e-6f )
                continue;

            dist = ( float )sqrt( distSq );

            dir[0] = delta[0] / dist;
            dir[1] = delta[1] / dist;
            dir[2] = delta[2] / dist;

            /* The face only emits out of its front, and the lit surface has to face it */
            cosEmit = -Vec3Dot( tri->normal, dir );

            if ( !( cosEmit > 0.0f ) )
                continue;

            cosRecv = normal ? Vec3Dot( normal, dir ) : 1.0f;

            if ( !( cosRecv > 0.0f ) )
                continue;

            /* Fade to nothing at the radius */
            falloff = 1.0f - distSq / radiusSq;
            falloff = falloff * falloff;

            if ( distSq < EMISSIVE_MIN_DIST_SQ )
                distSq = EMISSIVE_MIN_DIST_SQ;

            amount = brush->totalArea / ( float )brush->sampleCount * cosEmit / distSq
                   * falloff / EMISSIVE_PI;

            start[0] = pos[0] + dir[0] * EMISSIVE_START_OFFSET;
            start[1] = pos[1] + dir[1] * EMISSIVE_START_OFFSET;
            start[2] = pos[2] + dir[2] * EMISSIVE_START_OFFSET;

            open = Emissive_Visibility( start, point, normal != NULL );

            if ( open == 0.0f )
                continue;

            contribution[0] = brush->color[0] * amount * open;
            contribution[1] = brush->color[1] * amount * open;
            contribution[2] = brush->color[2] * amount * open;

            lum = contribution[0] + contribution[1] + contribution[2];

            sum[0] += contribution[0];
            sum[1] += contribution[1];
            sum[2] += contribution[2];

            dirSum[0] += dir[0] * lum;
            dirSum[1] += dir[1] * lum;
            dirSum[2] += dir[2] * lum;

            lumSum    += lum;
            cosLumSum += lum * cosRecv;
        }
    }

    if ( !( lumSum > 0.0f ) )
        return false;

    if ( Vec3Normalize( dirSum ) == 0.0f )
        return false;

    Vec3Copy( sum, outColor );
    Vec3Copy( dirSum, outDir );

    if ( outWeight )
        *outWeight = cosLumSum / lumSum;

    return true;
}

/* Original: c:\trees\cod3\cod3src\cod2rad\pointlights.cpp */

#include "cod4rad.h"
#include "pointlights.h"
#include "progress.h"
#include "geometry.h"
#include "modelcollision.h"
#include "com_math.h"
#include "lighting.h"
#include "assertive.h"

#include "r_light_load_obj.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


pointLightGlob_t pointLightGlob;              /* 0x130a81f4 */
PointLight_t     pointLights[POINTLIGHT_MAX]; /* 0x130a81f8 */

int              pointLightDefCount;                    /* 0x130a7df0 */
PointLightDef_t  pointLightDefs[POINTLIGHT_MAX_DEFS];   /* 0x130a7df4 */


/* PointLight_BuildFalloff  0x0041af80 */
static void PointLight_BuildFalloff( const Image_t *image, PointLightDef_t *def )
{
    int texelCount;
    int i;

    def->count  = image->width;
    def->height = image->height;

    texelCount = image->width * image->height;

    def->colors = ( vec3_t * )malloc( texelCount * sizeof( vec3_t ) );

    if ( !def->colors )
        Error( "out of memory" );

    for ( i = 0; i < texelCount; i++ )
    {
        def->colors[i][0] = Lighting_GammaToLinear( image->pixels[i * 4 + 0] * ( 1.0f / 255.0f ) );
        def->colors[i][1] = Lighting_GammaToLinear( image->pixels[i * 4 + 1] * ( 1.0f / 255.0f ) );
        def->colors[i][2] = Lighting_GammaToLinear( image->pixels[i * 4 + 2] * ( 1.0f / 255.0f ) );
    }
}

/* PointLight_FindDef  0x0041b090 */
static PointLightDef_t *PointLight_FindDef( const char *defName )
{
    PointLightDef_t *def;
    Image_t          image;
    int              i;

    for ( i = 0; i < pointLightDefCount; i++ )
    {
        if ( !Q_stricmp( pointLightDefs[i].name, defName ) )
            return &pointLightDefs[i];
    }

    if ( pointLightDefCount == POINTLIGHT_MAX_DEFS )
        Error( "More than %i lightDefs used by all lights combined; can't load '%s'\n",
               POINTLIGHT_MAX_DEFS, defName );

    def = &pointLightDefs[pointLightDefCount];

    if ( !Light_TryRegister( defName, &image ) )
        Error( "Couldn't get light def images for '%s'\n", defName );

    if ( image.height != 1 )
        Error( "Falloff image %s in light def %s has dimensions %ix%i; "
               "height should be 1\n",
               image.name, defName, image.width, image.height );

    def->name = _strdup( defName );

    PointLight_BuildFalloff( &image, def );

    pointLightDefCount++;

    return def;
}

/* PointLight_Add  0x0041b160 */
static PointLight_t *PointLight_Add( int primaryLightIndex, const vec3_t origin,
                                     float radius, const vec3_t color,
                                     const char *defName )
{
    PointLight_t *light;

    if ( pointLightGlob.lightCount == POINTLIGHT_MAX )
        Error( "Too many point lights (%i)\n", pointLightGlob.lightCount );

    light = &pointLights[pointLightGlob.lightCount];

    light->def = PointLight_FindDef( defName );

    if ( !light->def )
        return NULL;

    light->primaryLightIndex = primaryLightIndex ? primaryLightIndex : -1;

    light->origin[0] = origin[0];
    light->origin[1] = origin[1];
    light->origin[2] = origin[2];

    light->radius       = radius;
    light->falloffScale = light->def->count / radius;

    light->color[0] = color[0];
    light->color[1] = color[1];
    light->color[2] = color[2];

    Lighting_Vec3GammaToLinear( light->color );

    light->isSpot = 0;

    pointLightGlob.lightCount++;

    return light;
}

/* PointLight_AddOmni  0x0041b200 */
void PointLight_AddOmni( int primaryLightIndex, const vec3_t origin, float radius,
                         const vec3_t color, const char *defName )
{
    PointLight_Add( primaryLightIndex, origin, radius, color, defName );
}

/* PointLight_AddSpot  0x0041b230 */
void PointLight_AddSpot( int primaryLightIndex, const vec3_t origin, float radius,
                         const vec3_t color, const char *defName, const vec3_t dir,
                         float cosHalfFovOuter, float cosHalfFovInner, int exponent )
{
    PointLight_t *light = PointLight_Add( primaryLightIndex, origin, radius, color,
                                          defName );
    float         coneScale;

    if ( !light )
        return;

    light->isSpot = 1;

    light->spotDir[0] = dir[0];
    light->spotDir[1] = dir[1];
    light->spotDir[2] = dir[2];

    light->exponent  = exponent;
    light->cosCutoff = cosHalfFovOuter;

    coneScale = 1.0f / ( cosHalfFovInner - cosHalfFovOuter );

    light->coneScale = coneScale;
    light->coneBias  = -cosHalfFovOuter * coneScale;
}

/* PointLight_Count  0x0041b2a0 */
int PointLight_Count( void )
{
    return pointLightGlob.lightCount;
}

/* PointLight_Visibility  0x0041b2b0 */
float PointLight_Visibility( int lightIndex, const vec3_t pos, const vec3_t normal )
{
    GeoTrace_t    trace;
    PointLight_t *light;
    vec3_t        delta;
    vec3_t        dir;
    vec3_t        start;
    float         radiusSq;
    float         distSq;
    float         dist;
    float         invDist;
    float         facing;
    float         cone;
    float         openFraction;

    Assertx( lightIndex >= 0 && lightIndex < pointLightGlob.lightCount,
             "(lightIndex >= 0 && lightIndex < pointLightGlob.lightCount)\n\t"
             "(lightIndex) = %i", lightIndex );

    light = &pointLights[lightIndex];

    Assertx( light->def, "light->def" );

    delta[0] = light->origin[0] - pos[0];
    delta[1] = light->origin[1] - pos[1];
    delta[2] = light->origin[2] - pos[2];

    distSq = ( delta[0] * delta[0] + delta[1] * delta[1] ) + delta[2] * delta[2];

    radiusSq = light->radius * light->radius;

    if ( radiusSq < distSq )
        return 0.0f;

    dist = I_sqrt( distSq );

    if ( ( int )( float )floor( ( float )( light->falloffScale * dist - 0.5 ) )
             == light->def->count - 1 )
        return 0.0f;

    if ( dist < POINTLIGHT_COINCIDENT_DIST )
        return 1.0f;

    invDist = 1.0f / dist;

    dir[0] = delta[0] * invDist;
    dir[1] = delta[1] * invDist;
    dir[2] = delta[2] * invDist;

    facing = normal[0] * dir[0] + normal[1] * dir[1] + normal[2] * dir[2];

    if ( !( 0.0f < facing ) )
        return 0.0f;

    if ( light->isSpot )
    {
        cone = Vec3Dot( dir, light->spotDir );

        if ( !( light->cosCutoff < cone ) )
            return 0.0f;

        cone = cone * light->coneScale + light->coneBias;

        if ( 1.0f > cone )
        {
            int i;

            for ( i = 0; i < light->exponent; i++ )
                facing = cone * facing;
        }
    }

    start[0] = dir[0] * POINTLIGHT_TRACE_OFFSET + pos[0];
    start[1] = dir[1] * POINTLIGHT_TRACE_OFFSET + pos[1];
    start[2] = dir[2] * POINTLIGHT_TRACE_OFFSET + pos[2];

    Geo_SetupTrace( start, light->origin, NULL, NULL, &trace );
    Geo_TraceRay( &trace );

    openFraction = Geo_TraceOpenFraction( &trace.result );

    if ( openFraction == 0.0f )
        return 0.0f;

    Model_TraceLine( &trace );

    return Geo_TraceOpenFraction( &trace.result ) * facing;
}

/* PointLight_Sample  0x0041b580 */
int PointLight_Sample( int lightType, int lightIndex, const vec3_t pos,
                       const vec3_t axis0, const vec3_t axis1,
                       const vec3_t normal, vec3_t outDir, vec3_t outColor,
                       float *outWeight )
{
    GeoTrace_t    trace;
    PointLight_t *light;
    vec3_t        delta;
    vec3_t        start;
    vec3_t        color;
    float         radiusSq;
    float         distSq;
    float         dist;
    float         invDist;
    float         facing;
    float         falloff;
    float         coneAttenuation;
    float         openFraction;
    int           falloffIndex;
    int           influenceType;
    int           i;

    Assertx( lightIndex >= 0 && lightIndex < pointLightGlob.lightCount,
             "(lightIndex >= 0 && lightIndex < pointLightGlob.lightCount)\n\t"
             "(lightIndex) = %i", lightIndex );

    light = &pointLights[lightIndex];

    Assertx( light->def, "light->def" );

    delta[0] = light->origin[0] - pos[0];
    delta[1] = light->origin[1] - pos[1];
    delta[2] = light->origin[2] - pos[2];

    distSq = ( delta[0] * delta[0] + delta[1] * delta[1] ) + delta[2] * delta[2];

    radiusSq = light->radius * light->radius;

    if ( radiusSq < distSq )
        return 0;

    dist = I_sqrt( distSq );

    falloff      = light->falloffScale * dist - 0.5f;
    falloffIndex = ( int )( float )floor( falloff );

    if ( falloffIndex == light->def->count - 1 )
        return 0;

    if ( dist < POINTLIGHT_COINCIDENT_DIST )
    {
        outDir[0] = 0.0f;
        outDir[1] = 0.0f;
        outDir[2] = 0.0f;

        if ( outWeight )
            *outWeight = 1.0f;

        coneAttenuation = 1.0f;
        openFraction    = 1.0f;

        influenceType = ( light->primaryLightIndex != lightType ) * 2 + LIGHT_INFLUENCE_PRIMARY;
    }
    else
    {
        invDist = 1.0f / dist;

        outDir[0] = delta[0] * invDist;
        outDir[1] = delta[1] * invDist;
        outDir[2] = delta[2] * invDist;

        if ( normal )
        {
            facing = normal[0] * outDir[0] + normal[1] * outDir[1]
                   + normal[2] * outDir[2];

            if ( facing < 0.0f )
                return 0;

            if ( outWeight )
                *outWeight = facing;
        }

        coneAttenuation = 1.0f;

        if ( light->isSpot )
        {
            float cone = Vec3Dot( outDir, light->spotDir );

            if ( !( light->cosCutoff < cone ) )
            {
                if ( light->primaryLightIndex != lightType )
                    return 0;

                coneAttenuation = 0.0f;
            }
            else
            {
                cone = cone * light->coneScale + light->coneBias;

                if ( cone < 1.0f )
                    for ( i = 0; i < light->exponent; i++ )
                        coneAttenuation = cone * coneAttenuation;
            }
        }

        start[0] = outDir[0] * POINTLIGHT_TRACE_OFFSET + pos[0];
        start[1] = outDir[1] * POINTLIGHT_TRACE_OFFSET + pos[1];
        start[2] = outDir[2] * POINTLIGHT_TRACE_OFFSET + pos[2];

        Geo_SetupTrace( start, light->origin, axis0, axis1, &trace );
        Geo_TraceRay( &trace );

        openFraction = Geo_TraceOpenFraction( &trace.result );

        if ( openFraction == 0.0f )
            return 0;

        if ( normal )
        {
            Model_TraceLine( &trace );

            openFraction = Geo_TraceOpenFraction( &trace.result );

            if ( openFraction == 0.0f )
                return 0;
        }

        influenceType = ( light->primaryLightIndex != lightType ) + LIGHT_INFLUENCE_PRIMARY;
    }

    if ( falloffIndex < 0 )
    {
        color[0] = light->def->colors[0][0];
        color[1] = light->def->colors[0][1];
        color[2] = light->def->colors[0][2];
    }
    else
    {
        Vec3Lerp( light->def->colors[falloffIndex],
                  light->def->colors[falloffIndex + 1],
                  falloff - falloffIndex, color );

        if ( Vec3Compare( color, vec3_origin ) )
            return 0;
    }

    outColor[0] = light->color[0] * color[0];
    outColor[1] = light->color[1] * color[1];
    outColor[2] = light->color[2] * color[2];

    coneAttenuation = openFraction * coneAttenuation;

    outColor[0] = outColor[0] * coneAttenuation;
    outColor[1] = outColor[1] * coneAttenuation;
    outColor[2] = outColor[2] * coneAttenuation;

    return influenceType;
}

/* PointLight_TypeVisibility  0x0041b990 */
float PointLight_TypeVisibility( int lightType, const vec3_t pos, const vec3_t normal )
{
    int i;

    i = pointLightGlob.lightCount - numBSPPrimaryLights + lightType;

    if ( i <= 0 )
        i = 0;

    for ( ; i < pointLightGlob.lightCount; i++ )
    {
        if ( pointLights[i].primaryLightIndex == lightType )
            return PointLight_Visibility( i, pos, normal );

        if ( pointLights[i].primaryLightIndex > lightType )
            break;
    }

    return 0.0f;
}

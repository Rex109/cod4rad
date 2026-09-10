/* Original: c:\trees\cod3\cod3src\cod2rad\mapio.cpp */

#include "cod4rad.h"
#include "mapio.h"
#include "linearmapping.h"
#include "com_math.h"
#include "cmdline.h"
#include "collvec.h"
#include "com_memory.h"
#include "maskedmaterial.h"
#include "pointlights.h"
#include "lighting.h"
#include "geometry.h"
#include "bspfile.h"
#include "progress.h"
#include "materials.h"
#include "r_imagedecode.h"
#include "groundlight.h"
#include "lightgrid.h"
#include "modelcollision.h"
#include "xmodel.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_POLY_FILE_POINTS    16
#define MAX_POLY_FILE_LINE      0x400


/* BuildAlphaMask  0x00418890 */
byte *BuildAlphaMask( const Material_t *material, const Image_t *image )
{
    int   threshold;
    int   invert;
    int   texelCount;
    int   maskBytes;
    byte *mask;
    int   i;

    switch ( material->toolFlags & TOOLFLAG_USAGE_MASK )
    {
    case 0x30:
        threshold = 1;
        invert    = 0;
        break;

    case 0x40:
        threshold = 0x80;
        invert    = 0;
        break;

    case 0x50:
        threshold = 0x80;
        invert    = 0xff;
        break;

    default:
        return NULL;
    }

    texelCount = image->width * image->height;
    maskBytes  = ( texelCount + 7 ) >> 3;

    mask = ( byte * )malloc( maskBytes );
    if ( !mask )
        Error( "Couldn't allocate %i bytes for an alpha mask\n", maskBytes );

    memset( mask, 0, maskBytes );

    for ( i = 0; i < image->width * image->height; i++ )
    {
        if ( ( image->pixels[i * 4 + 3] ^ invert ) >= threshold )
            mask[i >> 3] |= ( byte )( 1 << ( i & 7 ) );
    }

    return mask;
}

/* BuildColorMask  0x00418960 */
byte *BuildColorMask( const Image_t *image )
{
    int   width;
    int   height;
    int   mapBytes;
    byte *colorMask;
    byte *dst;
    int   x;
    int   y;

    if ( ( image->width | image->height ) & 3 )
        return NULL;

    width  = image->width / 4;
    height = image->height / 4;

    mapBytes = width * height * 3;

    colorMask = ( byte * )malloc( mapBytes );
    if ( !colorMask )
        Error( "Couldn't allocate %i bytes for a color map\n", mapBytes );

    dst = colorMask;

    for ( y = 0; y < height; y++ )
    {
        for ( x = 0; x < width; x++ )
        {
            const byte *src = image->pixels + ( y * 4 * image->width + x * 4 ) * 4;
            int         red   = 0;
            int         green = 0;
            int         blue  = 0;
            int         row;
            int         col;

            for ( row = 0; row < 4; row++ )
            {
                for ( col = 0; col < 4; col++ )
                {
                    red   += src[col * 4 + 0];
                    green += src[col * 4 + 1];
                    blue  += src[col * 4 + 2];
                }

                src += image->width * 4;
            }

            dst[0] = ( byte )( ( red   + 8 ) / 16 );
            dst[1] = ( byte )( ( green + 8 ) / 16 );
            dst[2] = ( byte )( ( blue  + 8 ) / 16 );

            dst += 3;
        }
    }

    return colorMask;
}

/* MapIO_ValueForKey  0x00417820 */
static const char *MapIO_ValueForKey( const Entity_t *ent, const char *key )
{
    const epair_t *pair;

    Assertx( ent, "%s", "ent" );
    Assertx( key, "%s", "key" );

    for ( pair = ent->epairs; pair; pair = pair->next )
    {
        if ( !_stricmp( pair->key, key ) )
            return pair->value;
    }

    return NULL;
}

/* MapIO_VectorForKey  0x00417890 */
static qboolean MapIO_VectorForKey( const Entity_t *ent, const char *key, vec3_t out )
{
    const char *value = MapIO_ValueForKey( ent, key );

    if ( !value )
        return qfalse;

    if ( sscanf( value, "%g %g %g", &out[0], &out[1], &out[2] ) == 3 )
        return qtrue;

    if ( sscanf( value, "%g, %g, %g", &out[0], &out[1], &out[2] ) == 3 )
        return qtrue;

    if ( sscanf( value, "( %g %g %g )", &out[0], &out[1], &out[2] ) == 3 )
        return qtrue;

    if ( sscanf( value, "( %g, %g, %g )", &out[0], &out[1], &out[2] ) == 3 )
        return qtrue;

    Warning( 2, "key '%s' has value '%s' which is not a vector\n", key, value );

    return qfalse;
}

/* MapIO_FloatForKey  0x00417930 */
static float MapIO_FloatForKey( const Entity_t *ent, const char *key )
{
    const char *value = MapIO_ValueForKey( ent, key );

    if ( !value )
        return 0.0f;

    return ( float )atof( value );
}

/* MapIO_FindEntity  0x00417960 */
static Entity_t *MapIO_FindEntity( const char *key, const char *value )
{
    int i;

    for ( i = 0; i < num_entities; i++ )
    {
        const char *found = MapIO_ValueForKey( &entities[i], key );

        if ( found && !_stricmp( value, found ) )
            return &entities[i];
    }

    return NULL;
}

/* MapIO_IntForKey  0x004179b0 */
static int MapIO_IntForKey( const Entity_t *ent, const char *key )
{
    const char *value = MapIO_ValueForKey( ent, key );

    if ( !value )
        return 0;

    return atoi( value );
}

/* MapIO_GetOrientation  0x004179d0 */
static void MapIO_GetOrientation( orientation_t *orient, const Entity_t *ent )
{
    vec3_t angles;

    if ( !MapIO_VectorForKey( ent, "origin", orient->origin ) )
    {
        orient->origin[0] = 0.0f;
        orient->origin[1] = 0.0f;
        orient->origin[2] = 0.0f;
    }

    if ( !MapIO_VectorForKey( ent, "angles", angles ) )
    {
        angles[0] = 0.0f;
        angles[1] = 0.0f;
        angles[2] = 0.0f;
    }

    AnglesToAxis( angles, orient->axis );
}

/* MapIO_AddModelSurfaces  0x00417a40 */
static void MapIO_AddModelSurfaces( const Entity_t *ent, int modelIndex )
{
    orientation_t orient;
    const BspModel_t *model;
    int i;

    if ( ( unsigned )modelIndex >= ( unsigned )numBSPModels )
    {
        Warning( 1, "ignoring bad model index %i\n", modelIndex );
        return;
    }

    MapIO_GetOrientation( &orient, ent );

    model = &bspModels[modelIndex];

    for ( i = 0; i < model->triSoupCount[0]; i++ )
        Geo_AddModel( &bspTriSoups[0][model->firstTriSoup[0] + i], modelIndex, &orient );
}

/* MapIO_OverrideOption  0x00417ac0 */
static void MapIO_OverrideOption( const Entity_t *ent, const char *name, float *option,
                                  qboolean fromCommandLine, float min, float max )
{
    const char *value;
    float       parsed;

    if ( fromCommandLine )
    {
        Print( "\nusing command line %s %g\n\n", name, *option );
        return;
    }

    value = MapIO_ValueForKey( ent, name );

    if ( !value )
    {
        Print( "\nusing default %s %g\n\n", name, *option );
        return;
    }

    parsed  = ( float )atof( value );
    *option = parsed;

    if ( min > parsed )
    {
        Print( "map %s clamped from %g to %g\n\n", name, parsed, min );
        *option = min;
        return;
    }

    if ( max < parsed )
    {
        Print( "map %s clamped from %g to %g\n\n", name, parsed, max );
        *option = max;
        return;
    }

    Print( "\nusing map %s %g\n\n", name, parsed );
}

/* MapIO_AddWorldspawn  0x00417b90 */
static void MapIO_AddWorldspawn( Entity_t *ent )
{
    vec3_t sunColor;
    vec3_t sunDiffuseColor;
    vec3_t tint;
    float  sunlight;
    float  sunRadiosity;
    float  diffuseFraction;
    float  ambient;
    float  diffuse;
    float  direct;
    float  radiosityDirect;
    int    i;

    MapIO_OverrideOption( ent, "radiosityScale", &options.radiosityScale,
                          options.radiosityScaleSet,
                          RAD_RADIOSITY_SCALE_MIN, RAD_RADIOSITY_SCALE_MAX );

    MapIO_OverrideOption( ent, "contrastGain", &options.contrastGain,
                          options.contrastGainSet,
                          RAD_CONTRAST_GAIN_MIN, RAD_CONTRAST_GAIN_MAX );

    sunlight = MapIO_FloatForKey( ent, "sunlight" );

    sunRadiosity = MapIO_FloatForKey( ent, "sunradiosity" );

    if ( !( sunRadiosity > 0.0f ) )
        sunRadiosity = sunlight;

    if ( MapIO_VectorForKey( ent, "suncolor", sunColor ) )
    {
        Vec3ScaleToMax( sunColor );
    }
    else
    {
        sunColor[0] = 0.0f;
        sunColor[1] = 0.0f;
        sunColor[2] = 0.0f;
    }

    diffuseFraction = MapIO_FloatForKey( ent, "diffusefraction" );

    if ( MapIO_VectorForKey( ent, "sundiffusecolor", sunDiffuseColor ) )
    {
        Vec3ScaleToMax( sunDiffuseColor );
    }
    else
    {
        sunDiffuseColor[0] = 0.0f;
        sunDiffuseColor[1] = 0.0f;
        sunDiffuseColor[2] = 0.0f;
    }

    ambient = MapIO_FloatForKey( ent, "ambient" );

    if ( MapIO_VectorForKey( ent, "_color", tint ) )
    {
        Vec3ScaleToMax( tint );
    }
    else
    {
        tint[0] = 0.0f;
        tint[1] = 0.0f;
        tint[2] = 0.0f;
    }

    if ( ambient > sunlight )
    {
        Warning( 0, "WARNING: ambient %g > sunlight %g,"
                    " increasing sunlight to match ambient\n", ambient, sunlight );

        sunlight = ambient;
    }

    if ( diffuseFraction < 0.0f || diffuseFraction > 1.0f )
    {
        Warning( 0, "WARNING: clamping diffuseFraction %g to the range [0, 1]\n",
                 diffuseFraction );

        diffuseFraction = I_fclamp( diffuseFraction, 0.0f, 1.0f );
    }

    diffuse         = diffuseFraction * ( sunlight - ambient );
    direct          = ( sunlight - ambient ) - diffuse;
    radiosityDirect = sunRadiosity - ambient - diffuse;

    if ( direct < 0.0f )
    {
        Warning( 0, "WARNING: increasing sunlight %g to %g to match ambient + diffuse\n",
                 sunlight, ambient + diffuse );

        direct = 0.0f;
    }

    if ( radiosityDirect < 0.0f )
    {
        Warning( 0, "WARNING: increasing sunradiosity %g to %g"
                    " to match ambient + diffuse\n", sunRadiosity, ambient + diffuse );

        radiosityDirect = 0.0f;
    }

    for ( i = 0; i < 3; i++ )
    {
        float ambientPart   = ambient * tint[i];
        float diffusePart   = diffuse * sunDiffuseColor[i];
        float directPart    = sunColor[i] * direct;
        float radiosityPart = sunColor[i] * radiosityDirect;
        double total;

        options.ambientColor[i] = Lighting_GammaToLinear( ambientPart );

        total = directPart + diffusePart;

        if ( total == 0.0 )
        {
            options.sunColor[i] = 0.0f;
        }
        else
        {
            float lit = ( float )( total + ambientPart );

            options.sunColor[i] = ( float )
                ( ( Lighting_GammaToLinear( lit ) - options.ambientColor[i] )
                  * directPart / total );
        }

        total = radiosityPart + diffusePart;

        if ( total == 0.0 )
        {
            options.sunRadiosityColor[i] = 0.0f;
            options.sunDiffuseColor[i]   = 0.0f;
        }
        else
        {
            float lit = ( float )( total + ambientPart );

            options.sunRadiosityColor[i] = ( float )
                ( ( Lighting_GammaToLinear( lit ) - options.ambientColor[i] )
                  * diffusePart / total );

            options.sunDiffuseColor[i] = ( float )
                ( ( Lighting_GammaToLinear( lit ) - options.ambientColor[i] )
                  * radiosityPart / total );
        }
    }

    {
        vec3_t sunAngles;

        if ( MapIO_VectorForKey( ent, "sundirection", sunAngles ) )
            AngleVectors( sunAngles, options.sunDirection, NULL, NULL );
    }

    if ( numBSPPrimaryLights && bspPrimaryLights[1].type == GFX_LIGHT_TYPE_DIR )
        options.sunPrimaryLightIndex = PRIMARY_LIGHT_SUN;
    else
        options.sunPrimaryLightIndex = PRIMARY_LIGHT_NONE;

    ent->origin[0] = 0.0f;
    ent->origin[1] = 0.0f;
    ent->origin[2] = 0.0f;

    MapIO_AddModelSurfaces( ent, 0 );
}

/* MapIO_AddBrushModel  0x00417fa0 */
static void MapIO_AddBrushModel( const Entity_t *ent, const char *model )
{
    int modelIndex = atoi( model + 1 );

    if ( modelIndex <= 0 )
    {
        Warning( 1, "Entity %i has bad brush model %s\n",
                 ( int )( ent - entities ), modelIndex );
        return;
    }

    MapIO_AddModelSurfaces( ent, modelIndex );
}

/* MapIO_AllocModelMemory  0x00417ff0 */
static void *MapIO_AllocModelMemory( int size )
{
    void *mem = malloc( size );

    if ( !mem )
        Error( "Out of memory allocating %i bytes for an xmodel", size );

    memset( mem, 0, size );

    return mem;
}

/* MapIO_AddStaticModel  0x00418030 */
static void MapIO_AddStaticModel( const Entity_t *ent, qboolean scripted )
{
    char          shadowName[128];
    orientation_t orient;
    XModel_t     *model;
    const char   *value;
    const char   *modelName;
    vec3_t        points[GROUND_MAX_POINTS];
    vec3_t        worldPoints[GROUND_MAX_POINTS];
    vec3_t        scale;
    vec3_t        mins, maxs;
    vec3_t        center;
    vec3_t        delta;
    vec3_t        scaledPoint;
    float         modelscale;
    float         distance;
    unsigned int  pointCount;
    unsigned int  i;

    if ( MapIO_IntForKey( ent, "spawnflags" ) & 6 )
        return;

    modelName = MapIO_ValueForKey( ent, "model" );

    if ( !modelName )
        return;

    if ( Com_IsXModelName( modelName ) )
        modelName += 7;

    strcpy( shadowName, "shadow_" );
    strcpy( shadowName + 7, modelName );

    model = XModel_Register( shadowName, MapIO_AllocModelMemory, MapIO_AllocModelMemory );

    if ( !model )
    {
        model = XModel_Register( modelName, MapIO_AllocModelMemory, MapIO_AllocModelMemory );

        if ( !model )
        {
            Warning( 1, "failed to load misc_model '%s'\n", modelName );
            return;
        }
    }

    MapIO_GetOrientation( &orient, ent );

    value = MapIO_ValueForKey( ent, "modelscale" );

    if ( value )
    {
        modelscale = ( float )atof( value );

        if ( modelscale == 0.0f )
            modelscale = 1.0f;
    }
    else
    {
        modelscale = 1.0f;
    }

    scale[0] = modelscale;
    scale[1] = modelscale;
    scale[2] = modelscale;

    ModelCollision_AddStaticModel( model, scale, &orient, scripted );

    XModelGetTransformedBounds( model, orient.axis, mins, maxs );

    center[0] = ( maxs[0] + mins[0] ) * 0.5f;
    center[1] = ( mins[1] + maxs[1] ) * 0.5f;
    center[2] = ( mins[2] + maxs[2] ) * 0.5f;

    center[0] = center[0] * modelscale + orient.origin[0];
    center[1] = center[1] * modelscale + orient.origin[1];
    center[2] = center[2] * modelscale + orient.origin[2];

    LightGrid_AddStaticModelOrigin( center );

    pointCount = XModelGetSamplePoints( model, points, GROUND_MAX_POINTS );

    for ( i = 0; i < pointCount; i++ )
    {
        scaledPoint[0] = points[i][0] * modelscale;
        scaledPoint[1] = points[i][1] * modelscale;
        scaledPoint[2] = points[i][2] * modelscale;

        MatrixTransformVector43( ( const float ( * )[3] )&orient, scaledPoint,
                                 worldPoints[i] );
    }

    delta[0] = maxs[0] - mins[0];
    delta[1] = maxs[1] - mins[1];
    delta[2] = maxs[2] - mins[2];

    distance = Vec3Length( delta ) * modelscale;

    GroundLight_Add( ( Entity_t * )ent, worldPoints, pointCount, center, distance,
                     orient.axis[2] );
}

/* MapIO_AddLight  0x004182c0 */
static void MapIO_AddLight( const Entity_t *ent )
{
    const Entity_t *targetEnt;
    const char     *def;
    const char     *value;
    const char     *target;
    vec3_t          origin;
    vec3_t          color;
    vec3_t          targetOrigin;
    vec3_t          dir;
    float           radius;
    float           intensity;
    float           distance;
    float           cosHalfFovOuter;
    float           cosHalfFovInner;
    float           fov;
    int             exponent;

    if ( MapIO_IntForKey( ent, "spawnflags" ) & 3 )
        return;

    if ( !MapIO_VectorForKey( ent, "origin", origin ) )
        return;

    def = MapIO_ValueForKey( ent, "def" );

    if ( !def || !def[0] )
        def = "light_point_linear";

    value = MapIO_ValueForKey( ent, "radius" );

    if ( value )
        radius = ( float )atof( value );

    if ( !value || radius == 0.0f )
    {
        Warning( 1, "WARNING: ignoring light at (%.0f %.0f %.0f): no 'radius' key\n",
                 origin[0], origin[1], origin[2] );
        return;
    }

    if ( !MapIO_VectorForKey( ent, "_color", color ) )
    {
        Warning( 1, "WARNING: ignoring light at (%.0f %.0f %.0f): no '_color' key\n",
                 origin[0], origin[1], origin[2] );
        return;
    }

    intensity = MapIO_FloatForKey( ent, "intensity" );

    if ( !( 0.0f < intensity ) )
        intensity = 1.0f;

    Vec3ScaleToMax( color );

    color[0] = color[0] * intensity;
    color[1] = color[1] * intensity;
    color[2] = color[2] * intensity;

    target = MapIO_ValueForKey( ent, "target" );

    if ( !target )
    {
        PointLight_AddOmni( PRIMARY_LIGHT_NONE, origin, radius, color, def );
        return;
    }

    targetEnt = MapIO_FindEntity( "targetname", target );

    if ( !targetEnt )
    {
        Warning( 1, "WARNING: ignoring light at (%.0f %.0f %.0f): target entity "
                    "'%s' not found\n",
                 origin[0], origin[1], origin[2], target );
        return;
    }

    if ( !MapIO_VectorForKey( targetEnt, "origin", targetOrigin ) )
        return;

    dir[0] = origin[0] - targetOrigin[0];
    dir[1] = origin[1] - targetOrigin[1];
    dir[2] = origin[2] - targetOrigin[2];

    distance = Vec3Normalize( dir );

    fov = MapIO_FloatForKey( ent, "fov_outer" );

    if ( fov == 0.0f )
    {
        float length = distance * distance + 4096.0f;

        length = ( float )sqrt( length );

        cosHalfFovOuter = distance / length;
    }
    else
    {
        float halfAngle = fov * DEG2RAD * 0.5f;

        cosHalfFovOuter = ( float )cos( halfAngle );
    }

    fov = MapIO_FloatForKey( ent, "fov_inner" ) * DEG2RAD * 0.5f;

    cosHalfFovInner = ( float )cos( fov );

    if ( cosHalfFovOuter > cosHalfFovInner )
    {
        Warning( 1, "WARNING: ignoring spotlight at (%.0f %.0f %.0f): "
                    "fov_inner > fov_outer\n",
                 origin[0], origin[1], origin[2] );
        return;
    }

    exponent = MapIO_IntForKey( ent, "exponent" );

    PointLight_AddSpot( PRIMARY_LIGHT_NONE, origin, radius, color, def, dir,
                        cosHalfFovOuter, cosHalfFovInner, exponent );
}

/* MapIO_LoadPolyFile  0x0041c1f0 */
static void MapIO_LoadPolyFile( const char *mapName )
{
    vec3_t         xyz[MAX_POLY_FILE_POINTS];
    char           filename[256];
    vec2_t         st[MAX_POLY_FILE_POINTS];
    char           line[MAX_POLY_FILE_LINE];
    char           token[MAX_POLY_FILE_LINE];
    MskMaterial_t *mskMtl;
    FILE          *f;
    int            size;
    int            count;
    int            i;

    strcpy( filename, mapName );

    StripExtension( filename );
    strcat( filename, GetPolyFileExtension() );

    f = fopen( filename, "r" );

    if ( !f )
        return;

    while ( fgets( line, sizeof( line ), f ) )
    {
        size = I_strlen( line );

        if ( isspace( line[size - 1] ) )
        {
            do
            {
                size--;
            }
            while ( isspace( line[size - 1] ) );
        }

        line[size] = 0;

        if ( !fgets( token, sizeof( token ), f ) )
            Error( "Unexpected end of file in %s\n", filename );

        count = atoi( token );

        for ( i = 0; i < count; i++ )
        {
            if ( !fgets( token, sizeof( token ), f ) )
                Error( "Unexpected end of file in %s\n", filename );

            if ( sscanf( token, "( %g %g %g %g %g )\n",
                         &xyz[i][0], &xyz[i][1], &xyz[i][2],
                         &st[i][0], &st[i][1] ) != 5 )
                Error( "File %s has been corrupted\n", filename );
        }

        mskMtl = FindMaskMaterial( line, 4 );

        for ( i = 0; i < count - 2; i++ )
            Geo_AddTriangle( mskMtl, xyz[0], xyz[1], xyz[i + 2],
                             st[0], st[1], st[i + 2] );
    }

    fclose( f );
}

/* MapIO_AddEntity  0x004185e0 */
static void MapIO_AddEntity( const Entity_t *ent )
{
    const char *classname = MapIO_ValueForKey( ent, "classname" );
    const char *model;

    if ( !classname )
    {
        const char *origin = MapIO_ValueForKey( ent, "origin" );

        if ( origin )
            Warning( 1, "WARNING: entity at (%s) is missing a classname.\n", origin );
        else
            Warning( 1, "WARNING: entity is missing a classname.\n" );

        return;
    }

    if ( !_stricmp( classname, "worldspawn" ) )
    {
        MapIO_AddWorldspawn( ( Entity_t * )ent );
        return;
    }

    if ( !_stricmp( classname, "misc_model" ) )
    {
        MapIO_AddStaticModel( ent, qfalse );
        return;
    }

    if ( !_stricmp( classname, "script_model" ) )
    {
        MapIO_AddStaticModel( ent, qtrue );
        return;
    }

    if ( !_stricmp( classname, "light" ) )
    {
        MapIO_AddLight( ent );
        return;
    }

    model = MapIO_ValueForKey( ent, "model" );

    if ( !model || model[0] != '*' )
        return;

    MapIO_AddBrushModel( ent, model );
}

/* MapIO_AddEntities  0x004186d0 */
static void MapIO_AddEntities( void )
{
    int i;

    for ( i = 0; i < num_entities; i++ )
        MapIO_AddEntity( &entities[i] );
}

/* MapIO_AddPrimaryLights  0x00418700 */
static void MapIO_AddPrimaryLights( void )
{
    int i;

    for ( i = 0; i < numBSPPrimaryLights; i++ )
    {
        const BspPrimaryLight_t *light = &bspPrimaryLights[i];

        if ( light->type == GFX_LIGHT_TYPE_OMNI )
        {
            PointLight_AddOmni( i, light->origin, light->radius, light->color,
                                light->defName );
        }
        else if ( light->type == GFX_LIGHT_TYPE_SPOT )
        {
            PointLight_AddSpot( i, light->origin, light->radius, light->color,
                                light->defName, light->dir, light->cosHalfFovOuter,
                                light->cosHalfFovInner, light->coneExponent );
        }
    }
}

/* MapIO_WarnMissingMaterials  0x00418790 */
static void MapIO_WarnMissingMaterials( void )
{
    int i;

    for ( i = 0; i < numBSPMaterials; i++ )
    {
        const char *name = bspMaterials[i].name;

        if ( name[0] == '*' )
            continue;

        if ( !memcmp( name, "noshader", sizeof( "noshader" ) ) )
            continue;

        if ( FindMaskMaterial( name, MTL_USAGE_WORLD_VCOL ) )
            continue;

        Warning( 1, "couldn't load material '%s'\n", name );
    }
}

/* LoadMapFile  0x00418810 */
bool LoadMapFile( const char *mapName )
{
    int i;

    LoadBSPFileLumps( mapName, qfalse );
    ParseEntities();

    Geo_Init();

    MapIO_WarnMissingMaterials();

    for ( i = 0; i < num_entities; i++ )
        MapIO_AddEntity( &entities[i] );

    MapIO_AddPrimaryLights();

    Lighting_AllocLightmaps();

    MapIO_LoadPolyFile( mapName );

    return true;
}

/* WriteMapFile  0x00418880 */
void WriteMapFile( const char *mapName )
{
    UnparseEntities();

    WriteBSPFile( mapName );
}

/* Original: .\materials.cpp */

#include "cod4rad.h"

#include "materials.h"

#include <algorithm>
#include "sort_vc8.h"
#include "qsort_vc8.h"

extern const char *StringFromOffset( const void *base );                /* 0x004053a0 */
extern const char *StringFromOffset( const void *base, int offset );    /* 0x004053c0 */


const char *s_techSetUsagePrefix[MTL_USAGE_COUNT] =                     /* 0x0052910c */
{
    "",
    "m_",
    "mc_",
    "w_",
    "wc_"
};


static const ShaderNameEntry_t s_shaderNames[SHADER_NAME_ENTRY_COUNT] =  /* 0x004fc228 */
{
    { "l_sm_a0c0",        "l_sm_", "l_[hsm|sm]_", "a0c0"       },
    { "l_sm_a0c0d0",      "l_sm_", "l_[hsm|sm]_", "a0c0d0"     },
    { "l_sm_a0c0d0n0",    "l_sm_", "l_[hsm|sm]_", "a0c0d0n0"   },
    { "l_sm_a0c0d0n0s0",  "l_sm_", "l_[hsm|sm]_", "a0c0d0n0s0" },
    { "l_sm_a0c0d0s0",    "l_sm_", "l_[hsm|sm]_", "a0c0d0s0"   },
    { "l_sm_a0c0n0",      "l_sm_", "l_[hsm|sm]_", "a0c0n0"     },
    { "l_sm_a0c0n0s0",    "l_sm_", "l_[hsm|sm]_", "a0c0n0s0"   },
    { "l_sm_a0c0s0",      "l_sm_", "l_[hsm|sm]_", "a0c0s0"     },
    { "l_sm_b0c0",        "l_sm_", "l_[hsm|sm]_", "b0c0"       },
    { "l_sm_b0c0d0",      "l_sm_", "l_[hsm|sm]_", "b0c0d0"     },
    { "l_sm_b0c0d0n0",    "l_sm_", "l_[hsm|sm]_", "b0c0d0n0"   },
    { "l_sm_b0c0d0n0s0",  "l_sm_", "l_[hsm|sm]_", "b0c0d0n0s0" },
    { "l_sm_b0c0d0s0",    "l_sm_", "l_[hsm|sm]_", "b0c0d0s0"   },
    { "l_sm_b0c0n0",      "l_sm_", "l_[hsm|sm]_", "b0c0n0"     },
    { "l_sm_b0c0n0s0",    "l_sm_", "l_[hsm|sm]_", "b0c0n0s0"   },
    { "l_sm_b0c0s0",      "l_sm_", "l_[hsm|sm]_", "b0c0s0"     },
    { "l_sm_r0c0",        "l_sm_", "l_[hsm|sm]_", "r0c0"       },
    { "l_sm_r0c0d0",      "l_sm_", "l_[hsm|sm]_", "r0c0d0"     },
    { "l_sm_r0c0d0n0",    "l_sm_", "l_[hsm|sm]_", "r0c0d0n0"   },
    { "l_sm_r0c0d0n0s0",  "l_sm_", "l_[hsm|sm]_", "r0c0d0n0s0" },
    { "l_sm_r0c0d0s0",    "l_sm_", "l_[hsm|sm]_", "r0c0d0s0"   },
    { "l_sm_r0c0n0",      "l_sm_", "l_[hsm|sm]_", "r0c0n0"     },
    { "l_sm_r0c0n0s0",    "l_sm_", "l_[hsm|sm]_", "r0c0n0s0"   },
    { "l_sm_r0c0s0",      "l_sm_", "l_[hsm|sm]_", "r0c0s0"     },
    { "l_sm_t0c0",        "l_sm_", "l_[hsm|sm]_", "t0c0"       },
    { "l_sm_t0c0d0",      "l_sm_", "l_[hsm|sm]_", "t0c0d0"     },
    { "l_sm_t0c0d0n0",    "l_sm_", "l_[hsm|sm]_", "t0c0d0n0"   },
    { "l_sm_t0c0d0n0s0",  "l_sm_", "l_[hsm|sm]_", "t0c0d0n0s0" },
    { "l_sm_t0c0d0s0",    "l_sm_", "l_[hsm|sm]_", "t0c0d0s0"   },
    { "l_sm_t0c0n0",      "l_sm_", "l_[hsm|sm]_", "t0c0n0"     },
    { "l_sm_t0c0n0s0",    "l_sm_", "l_[hsm|sm]_", "t0c0n0s0"   },
    { "l_sm_t0c0s0",      "l_sm_", "l_[hsm|sm]_", "t0c0s0"     },
    { "unlit_multiply",   NULL,    NULL,          "m0c0"       }
};


MaterialRef_t s_materials[MAX_REGISTERED_MATERIALS];   /* 0x123c2900 */
int           s_materialCount;                         /* 0x123c28fc */
TechSet_t     s_techSets[MAX_TECHNIQUE_SETS];          /* 0x123aa8f8 */
int           s_techSetCount;                          /* 0x123c28f8 */

float         defaultTessSize;                         /* 0x123ce900 */


static void        PrintLayeredMaterialParts( const char *mtlName );
static int         CompareBSPMaterialNames( const void *a, const void *b );
static bool        SortMaterialRefs( const MaterialRef_t &a, const MaterialRef_t &b );
static int         MaterialRefDepthKind( const MaterialRef_t *ref );
static const char *Material_TextureTable( const Material_t *material );
static MaterialRef_t *FindRegisteredMaterial( const char *name, int usage );
static TechSet_t  *TechSet_Register( const char *name, int usage );
static void        TechSet_Load( TechSet_t *techSet, int usage );
static const char *FindCaption( char *text, const char *caption );
static void        TechSet_LoadTechnique( TechSet_t *techSet, char *text,
                                          const char *caption );
static qboolean    ExtractNameBeforeSemicolon( char *text, const char *caption,
                                               char *out );
static const char *ExtractQuotedValue( const char *text, const char *key );
static int         TechSetDepthPrepassKind( char *text );
static const ShaderNameEntry_t *FindShaderNameEntry( const Material_t *material );


/* PrintBSPMaterialList  0x00423f70 */
void PrintBSPMaterialList( void )
{
    const char *names[MAX_MAP_MATERIALS];
    int         i;

    for ( i = 0; i < numBSPMaterials; i++ )
    {
        names[i] = bspMaterials[i].name;
    }

    qsort_vc8( names, numBSPMaterials, sizeof( names[0] ), CompareBSPMaterialNames );

    printf( "\n\nMaterial list:\n" );

    for ( i = 0; i < numBSPMaterials; i++ )
    {
        printf( "%4i: %s\n", i + 1, names[i] );

        if ( names[i][0] == '*' )
        {
            PrintLayeredMaterialParts( names[i] );
        }
    }
}


/* PrintLayeredMaterialParts  0x00424070 */
static void PrintLayeredMaterialParts( const char *mtlName )
{
    const char *nameIter;
    int         index;

    Assertx( mtlName[0] == '*' && isdigit( mtlName[1] ), "(mtlName) = %s", mtlName );

    nameIter = mtlName + 1;

    for ( ;; )
    {
        index = 0;

        do
        {
            index = index * 10 + ( *nameIter - '0' );
            nameIter++;
        }
        while ( isdigit( *nameIter ) );

        printf( "        %s\n", bspMaterials[index].name );

        if ( *nameIter == 'n' )
        {
            nameIter++;
        }

        if ( !*nameIter )
        {
            break;
        }

        Assert( *nameIter == '_' );
        nameIter++;
    }
}


/* CompareBSPMaterialNames  0x00424170 */
static int CompareBSPMaterialNames( const void *a, const void *b )
{
    const char *nameA = *( const char * const * )a;
    const char *nameB = *( const char * const * )b;

    if ( nameA[0] == '*' )
    {
        if ( nameB[0] != '*' )
        {
            return 1;
        }
    }
    else if ( nameB[0] == '*' )
    {
        return -1;
    }

    return _stricmp( nameA, nameB );
}


/* Materials_Finish  0x004241d0 */
void Materials_Finish( void )
{
    Sort_VC8( s_materials, s_materials + s_materialCount, SortMaterialRefs );
}


/* SortMaterialRefs  0x00424200 */
static bool SortMaterialRefs( const MaterialRef_t &a, const MaterialRef_t &b )
{
    const TechSet_t *techSet[2];
    bool             hasLightmap[2];
    int              depthKind[2];
    int              comparison;

    techSet[0] = a.techSet;
    techSet[1] = b.techSet;

    if ( a.material == b.material )
    {
        return false;
    }

    if ( techSet[0]->hasLitTechnique != techSet[1]->hasLitTechnique )
    {
        return techSet[0]->hasLitTechnique != 0;
    }

    hasLightmap[0] = ( a.material->techSetFlags & 2 ) != 0;
    hasLightmap[1] = ( b.material->techSetFlags & 2 ) != 0;

    if ( techSet[1]->hasLitTechnique )
    {
        Assert( !techSet[0]->hasEmissiveTechnique );
        Assert( !techSet[1]->hasEmissiveTechnique );

        comparison = a.material->sortOrder - b.material->sortOrder;
        if ( comparison )
        {
            return comparison < 0;
        }

        if ( hasLightmap[0] != hasLightmap[1] )
        {
            return hasLightmap[0];
        }
    }
    else
    {
        Assert( !hasLightmap[0] );
        Assert( !hasLightmap[1] );

        if ( techSet[0]->hasEmissiveTechnique != techSet[1]->hasEmissiveTechnique )
        {
            return techSet[0]->hasEmissiveTechnique != 0;
        }

        comparison = a.material->sortOrder - b.material->sortOrder;
        if ( comparison )
        {
            return comparison < 0;
        }
    }

    depthKind[0] = MaterialRefDepthKind( &a );
    depthKind[1] = MaterialRefDepthKind( &b );

    if ( depthKind[0] != depthKind[1] )
    {
        return ( unsigned int )depthKind[0] < ( unsigned int )depthKind[1];
    }

    if ( techSet[0] != techSet[1] )
    {
        comparison = strcmp( techSet[0]->pixelShader, techSet[1]->pixelShader );
        if ( comparison )
        {
            return comparison < 0;
        }

        comparison = strcmp( techSet[0]->vertexShader, techSet[1]->vertexShader );
        if ( comparison )
        {
            return comparison < 0;
        }

        comparison = strcmp( Material_TechSetName( a.material ),
                             Material_TechSetName( b.material ) );
        Assert( comparison );
        return comparison < 0;
    }

    comparison = strcmp( StringFromOffset( a.material ),
                         StringFromOffset( b.material ) );
    Assert( comparison );
    return comparison < 0;
}


/* MaterialRefDepthKind  0x00424520 */
static int MaterialRefDepthKind( const MaterialRef_t *ref )
{
    if ( ( ref->material->layerFlags & MTL_LAYER_MASK ) == 0 )
    {
        return TECHSET_ZPREPASS_ABSENT;
    }

    return ref->techSet->depthPrepassKind;
}


/* Material_SortsBefore  0x00424550 */
bool Material_SortsBefore( const Material_t *a, const Material_t *b )
{
    int i;

    for ( i = 0; i < s_materialCount; i++ )
    {
        if ( s_materials[i].material == a )
        {
            return true;
        }

        if ( s_materials[i].material == b )
        {
            return false;
        }
    }

    SanityCheckMsg( "inconceivable" );
    return false;
}


/* Material_SortIndex  0x004245d0 */
int Material_SortIndex( const Material_t *material )
{
    int i;

    for ( i = 0; i < s_materialCount; i++ )
    {
        if ( s_materials[i].material == material )
        {
            return i;
        }
    }

    SanityCheckMsg( "inconceivable" );
    return 0x7fffffff;
}


/* Material_HasDetailMap  0x00424640 */
bool Material_HasDetailMap( const Material_t *material )
{
    return Material_TextureValue( material, "detailMap" ) != NULL;
}


/* Material_TextureValue  0x00424660 */
const char *Material_TextureValue( const Material_t *material, const char *name )
{
    const int *table;
    int        i;

    table = ( const int * )Material_TextureTable( material );

    for ( i = 0; i < ( int )material->textureCount; i++ )
    {
        if ( !strcmp( StringFromOffset( material, table[i * 3 + 0] ), name ) )
        {
            return StringFromOffset( material, table[i * 3 + 2] );
        }
    }

    return NULL;
}


/* Material_HasNormalMap  0x004246f0 */
bool Material_HasNormalMap( const Material_t *material )
{
    const char *value;

    value = Material_TextureValue( material, "normalMap" );

    if ( value && strcmp( value, "$identitynormalmap" ) )
    {
        return true;
    }

    return false;
}


/* Material_HasSpecularMap  0x00424740 */
bool Material_HasSpecularMap( const Material_t *material )
{
    return Material_TextureValue( material, "specularMap" ) != NULL;
}


/* Material_Register  0x00424760 */
MaterialRef_t *Material_Register( const char *name, int usage )
{
    MaterialRef_t *ref;
    char           path[1024];
    qboolean       isDefault;
    void          *fileData;
    int            length;

    ref = FindRegisteredMaterial( name, usage );

    if ( ref )
    {
        return ref;
    }

    isDefault = !strcmp( name, "$default" );

    sprintf( path, "materials/%s", name );
    length = FS_ReadFile( path, &fileData );

    if ( length < 0 )
    {
        if ( isDefault )
        {
            Com_Error( "Cannot find material '$default'" );
        }

        printf( "Material '%s' is missing\n", name );
        return Material_Register( "$default", usage );
    }

    if ( length < 0x40 )
    {
        if ( isDefault )
        {
            Com_Error( "Material '$default' is corrupt" );
        }

        printf( "Material '%s' is corrupt\n", name );
        return Material_Register( "$default", usage );
    }

    ref           = &s_materials[s_materialCount];
    ref->material = ( Material_t * )fileData;
    ref->usage    = usage;
    ref->techSet  = TechSet_Register( Material_TechSetName( ref->material ), usage );
    s_materialCount++;

    return ref;
}


/* FindRegisteredMaterial  0x004248e0 */
static MaterialRef_t *FindRegisteredMaterial( const char *name, int usage )
{
    int i;

    for ( i = 0; i < s_materialCount; i++ )
    {
        if ( s_materials[i].usage == usage
             && !strcmp( name, StringFromOffset( s_materials[i].material ) ) )
        {
            return &s_materials[i];
        }
    }

    return NULL;
}


/* TechSet_Register  0x00424960 */
static TechSet_t *TechSet_Register( const char *name, int usage )
{
    TechSet_t *techSet;
    int        i;

    for ( i = 0; i < s_techSetCount; i++ )
    {
        if ( s_techSets[i].usage == usage && !strcmp( name, s_techSets[i].name ) )
        {
            return &s_techSets[i];
        }
    }

    techSet = &s_techSets[s_techSetCount];
    s_techSetCount++;

    techSet->name  = name;
    techSet->usage = usage;
    TechSet_Load( techSet, usage );

    return techSet;
}


/* TechSet_Load  0x00424a10 */
static void TechSet_Load( TechSet_t *techSet, int usage )
{
    char  path[1024];
    char *text;

    Com_sprintf( path, sizeof( path ), "techsets/%s%s.techset",
                 s_techSetUsagePrefix[usage], techSet->name );

    if ( FS_ReadFile( path, ( void ** )&text ) < 0 )
    {
        techSet->hasShadowMapTechnique = 0;
        techSet->hasLitTechnique       = 0;
        techSet->hasEmissiveTechnique  = 0;
        techSet->depthPrepassKind      = TECHSET_ZPREPASS_ABSENT;
        techSet->pixelShader           = "";
        techSet->vertexShader          = "";
        return;
    }

    Assert( !strchr( text, '/' ) );

    techSet->hasShadowMapTechnique = FindCaption( text, "\"build shadowmap depth\":" ) != NULL;
    techSet->hasLitTechnique       = FindCaption( text, "\"lit\":" ) != NULL;
    techSet->hasEmissiveTechnique  = FindCaption( text, "\"emissive\":" ) != NULL;
    techSet->depthPrepassKind      = TechSetDepthPrepassKind( text );

    if ( techSet->hasLitTechnique )
    {
        TechSet_LoadTechnique( techSet, text, "\"lit\":" );
    }
    else if ( techSet->hasEmissiveTechnique )
    {
        TechSet_LoadTechnique( techSet, text, "\"emissive\":" );
    }
    else
    {
        techSet->pixelShader  = "";
        techSet->vertexShader = "";
    }

    FS_FreeFile( text );
}


/* FindCaption  0x00424bb0 */
static const char *FindCaption( char *text, const char *caption )
{
    return strstr( text, caption );
}


/* TechSet_LoadTechnique  0x00424bd0 */
static void TechSet_LoadTechnique( TechSet_t *techSet, char *text, const char *caption )
{
    char  path[1024];
    char  techniqueName[64];
    char *techText;

    ExtractNameBeforeSemicolon( text, caption, techniqueName );

    sprintf( path, "techniques/%s.tech", techniqueName );

    if ( FS_ReadFile( path, ( void ** )&techText ) < 0 )
    {
        techSet->pixelShader  = "";
        techSet->vertexShader = "";
        return;
    }

    techSet->pixelShader  = ExtractQuotedValue( techText, "pixelShader" );
    techSet->vertexShader = ExtractQuotedValue( techText, "vertexShader" );

    FS_FreeFile( techText );
}


/* ExtractNameBeforeSemicolon  0x00424c90 */
static qboolean ExtractNameBeforeSemicolon( char *text, const char *caption, char *out )
{
    const char *at;
    const char *endOfName;
    const char *nameIter;

    at = FindCaption( text, caption );

    if ( !at )
    {
        return 0;
    }

    endOfName = strchr( at, ';' );
    Assert( endOfName );

    for ( nameIter = endOfName;
          isalnum( nameIter[-1] ) || nameIter[-1] == '_';
          nameIter-- )
    {
    }

    memcpy( out, nameIter, endOfName - nameIter );
    out[endOfName - nameIter] = 0;

    return 1;
}


/* ExtractQuotedValue  0x00424d50 */
static const char *ExtractQuotedValue( const char *text, const char *key )
{
    const char *at;
    const char *start;
    const char *end;
    char        name[64];
    int         length;

    at = strstr( text, key );

    if ( !at )
    {
        return "";
    }

    at = strchr( at, '"' );

    if ( !at )
    {
        return "";
    }

    start = at + 1;
    end   = strchr( start, '"' );

    if ( !end )
    {
        return "";
    }

    length = I_min( end - start, 63 );
    memcpy( name, start, length );
    name[length] = 0;

    return _strdup( name );
}


/* TechSetDepthPrepassKind  0x00424e20 */
static int TechSetDepthPrepassKind( char *text )
{
    char name[64];
    char floatzName[64];

    if ( ExtractNameBeforeSemicolon( text, "\"depth prepass\":", name ) )
    {
        if ( strcmp( name, "zprepass" ) )
        {
            return TECHSET_ZPREPASS_NONE;
        }

        return TECHSET_ZPREPASS_ZPREPASS;
    }

    if ( ExtractNameBeforeSemicolon( text, "build floatz", floatzName ) )
    {
        return TECHSET_ZPREPASS_FLOATZ;
    }

    return TECHSET_ZPREPASS_ABSENT;
}


/* Material_WantsShadowMap  0x00424eb0 */
bool Material_WantsShadowMap( const Material_t *material )
{
    int i;

    if ( material->surfaceFlags & SURF_SKY )
    {
        return false;
    }

    if ( Material_IsLayered( material ) )
    {
        return false;
    }

    for ( i = 0; i < s_materialCount; i++ )
    {
        if ( s_materials[i].material == material )
        {
            return s_materials[i].techSet->hasShadowMapTechnique != 0;
        }
    }

    SanityCheckMsg( "inconceivable" );
    return false;
}


/* Material_IsLayered  0x00424f50 */
bool Material_IsLayered( const Material_t *material )
{
    return ( material->layerFlags & MTL_LAYER_MASK ) != 0;
}


/* Material_CastsShadow  0x00424f70 */
bool Material_CastsShadow( const Material_t *material )
{
    if ( material->toolFlags & TOOLFLAG_LIGHTPORTAL )
    {
        return true;
    }

    if ( ( material->toolFlags & TOOLFLAG_USAGE_MASK ) != TOOLFLAG_USAGE_LIT )
    {
        return false;
    }

    if ( material->surfaceFlags & SURF_NOCASTSHADOW )
    {
        return false;
    }

    return true;
}


/* Material_HasKnownTechSet  0x00424fb0 */
bool Material_HasKnownTechSet( const Material_t *material )
{
    return FindShaderNameEntry( material ) != NULL;
}


/* FindShaderNameEntry  0x00424fd0 */
static const ShaderNameEntry_t *FindShaderNameEntry( const Material_t *material )
{
    const char *techSetName;
    int         lo;
    int         hi;
    int         mid;
    int         comparison;

    techSetName = Material_TechSetName( material );

    lo = 0;
    hi = SHADER_NAME_ENTRY_COUNT;

    while ( lo < hi )
    {
        mid        = ( lo + hi ) / 2;
        comparison = Q_strcmp( techSetName, s_shaderNames[mid].techSetName );

        if ( comparison < 0 )
        {
            hi = mid;
        }
        else if ( comparison > 0 )
        {
            lo = mid + 1;
        }
        else
        {
            return &s_shaderNames[mid];
        }
    }

    return NULL;
}


/* Material_LayerCombineCount  0x00425060 */
int Material_LayerCombineCount( Material_t * const *materials, int layerCount,
                                qboolean force )
{
    const ShaderNameEntry_t *entry[MTL_LAYER_LIMIT];
    bool                     hasNormalMap[MTL_LAYER_LIMIT];
    int                      normalMapCount;
    int                      i;

    if ( layerCount == 1 )
    {
        return 1;
    }

    AssertRange( layerCount, 2, MTL_LAYER_LIMIT );

    if ( !force )
    {
        if ( ( materials[0]->toolFlags & TOOLFLAG_USAGE_MASK ) == TOOLFLAG_USAGE_NOCOMBINE )
        {
            return 1;
        }

        for ( i = 1; i < layerCount; i++ )
        {
            if ( ( materials[i]->toolFlags & TOOLFLAG_USAGE_MASK ) == TOOLFLAG_USAGE_NOCOMBINE )
            {
                layerCount = i;

                if ( layerCount == 1 )
                {
                    return 1;
                }

                break;
            }
        }
    }

    entry[0] = FindShaderNameEntry( materials[0] );

    if ( !entry[0] )
    {
        return 1;
    }

    if ( entry[0]->layerCode[0] == 'm' )
    {
        return 1;
    }

    hasNormalMap[0] = strchr( entry[0]->layerCode, 'n' ) != NULL;
    normalMapCount  = hasNormalMap[0] ? 1 : 0;

    for ( i = 1; i < layerCount; i++ )
    {
        entry[i] = FindShaderNameEntry( materials[i] );

        if ( !entry[i] )
        {
            return i;
        }

        if ( entry[i]->layerCode[0] == 'm' && entry[0]->layerCode[0] != 'r' )
        {
            return i;
        }

        hasNormalMap[i] = strchr( entry[i]->layerCode, 'n' ) != NULL;

        if ( hasNormalMap[i] )
        {
            if ( i >= 3 )
            {
                return i;
            }

            normalMapCount++;

            if ( normalMapCount == 3 )
            {
                return i + 1;
            }
        }
    }

    return layerCount;
}


/* Material_TechSetName  0x00425250 */
const char *Material_TechSetName( const Material_t *material )
{
    return StringFromOffset( material, material->techSetNameOffset );
}


/* Material_TextureTable  0x00425270 */
static const char *Material_TextureTable( const Material_t *material )
{
    return ( const char * )material + material->textureTableOffset;
}

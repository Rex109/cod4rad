
#ifndef MATERIALS_H
#define MATERIALS_H

#include "q_shared.h"
#include "r_material.h"


#define MTL_LAYER_LIMIT         5

#define MTL_LAYER_MASK          0x00000030

#define TOOLFLAG_USAGE_MASK     0x0070
#define TOOLFLAG_USAGE_LIT      0x0010
#define TOOLFLAG_USAGE_VCOLOR   0x0060
#define TOOLFLAG_USAGE_NOCOMBINE 0x0070

#define MTL_USAGE_COUNT         5
#define MTL_USAGE_NONE          0
#define MTL_USAGE_MODEL         1
#define MTL_USAGE_MODEL_VCOL    2
#define MTL_USAGE_WORLD         3
#define MTL_USAGE_WORLD_VCOL    4

#define MAX_REGISTERED_MATERIALS  0x1000
#define MAX_TECHNIQUE_SETS        0x0200

#define TECHSET_ZPREPASS_NONE      0
#define TECHSET_ZPREPASS_ZPREPASS  1
#define TECHSET_ZPREPASS_FLOATZ    2
#define TECHSET_ZPREPASS_ABSENT    3


typedef struct
{
    const char *name;                      /* +0x00 */
    int         usage;                     /* +0x04 */
    byte        hasShadowMapTechnique;     /* +0x08 */
    byte        hasLitTechnique;           /* +0x09 */
    byte        hasEmissiveTechnique;      /* +0x0a */
    byte        pad0b;                     /* +0x0b */
    int         depthPrepassKind;          /* +0x0c */
    const char *pixelShader;               /* +0x10 */
    const char *vertexShader;              /* +0x14 */
} TechSet_t;


typedef struct
{
    Material_t *material;   /* +0x00 */
    TechSet_t  *techSet;    /* +0x04 */
    int         usage;      /* +0x08 */
} MaterialRef_t;


typedef struct
{
    const char *techSetName;   /* +0x00 */
    const char *prefix;        /* +0x04 */
    const char *prefixPattern; /* +0x08 */
    const char *layerCode;     /* +0x0c */
} ShaderNameEntry_t;

#define SHADER_NAME_ENTRY_COUNT  33


extern MaterialRef_t s_materials[MAX_REGISTERED_MATERIALS];   /* 0x123c2900 */
extern int           s_materialCount;                         /* 0x123c28fc */
extern TechSet_t     s_techSets[MAX_TECHNIQUE_SETS];          /* 0x123aa8f8 */
extern int           s_techSetCount;                          /* 0x123c28f8 */


MaterialRef_t *Material_Register( const char *name, int usage );      /* 0x00424760 */
void           Materials_Finish( void );                              /* 0x004241d0 */

bool Material_SortsBefore( const Material_t *a, const Material_t *b ); /* 0x00424550 */
int  Material_SortIndex( const Material_t *material );                /* 0x004245d0 */
bool Material_CastsShadow( const Material_t *material );              /* 0x00424f70 */
bool Material_WantsShadowMap( const Material_t *material );           /* 0x00424eb0 */
bool Material_IsLayered( const Material_t *material );                /* 0x00424f50 */
bool Material_HasKnownTechSet( const Material_t *material );          /* 0x00424fb0 */
int  Material_LayerCombineCount( Material_t * const *materials,
                                 int layerCount, qboolean force );    /* 0x00425060 */

const char *Material_TextureValue( const Material_t *material,
                                   const char *name );                /* 0x00424660 */
bool  Material_HasDetailMap( const Material_t *material );            /* 0x00424640 */
bool  Material_HasNormalMap( const Material_t *material );            /* 0x004246f0 */
bool  Material_HasSpecularMap( const Material_t *material );          /* 0x00424740 */

void  PrintBSPMaterialList( void );                                   /* 0x00423f70 */

const char *Material_TechSetName( const Material_t *material );       /* 0x00425250 */

#endif

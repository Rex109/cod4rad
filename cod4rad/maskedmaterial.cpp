/* Original: c:\trees\cod3\cod3src\cod2rad\maskedmaterial.cpp */

#include "cod4rad.h"
#include "maskedmaterial.h"
#include "mapio.h"

#include "com_memory.h"
#include "r_imagedecode.h"

#include <string.h>


int           mskMtlCount;                      /* 0x13063dd8 */
MskMaterial_t mskMtls[MAX_MASKED_MATERIALS];    /* 0x13063de0 */


static const char *MaterialImageName( const Material_t *material )
{
    return ( const char * )material + material->imageNameOffset;
}

static const char *MaterialName( const Material_t *material )
{
    return ( const char * )material + material->nameOffset;
}

/* FindLoadedMaskMaterial  0x00418b10 */
static MskMaterial_t *FindLoadedMaskMaterial( const char *mtlName, int usage )
{
    int i;

    for ( i = 0; i < mskMtlCount; i++ )
    {
        if ( mskMtls[i].usage != usage )
            continue;

        if ( !strcmp( mtlName, MaterialName( mskMtls[i].material ) ) )
            return &mskMtls[i];
    }

    return NULL;
}

/* FindMaskMaterial  0x00418b90 */
MskMaterial_t *FindMaskMaterial( const char *mtlName, int usage )
{
    MskMaterial_t *mskMtl;
    MaterialRef_t *ref;
    const Material_t *material;
    const char       *registeredName;
    Image_t           image;

    mskMtl = FindLoadedMaskMaterial( mtlName, usage );

    if ( mskMtl )
        return mskMtl;

    ref = Material_Register( mtlName, usage );

    Assertx( ref, "mtlLoaded" );

    material       = ref->material;
    registeredName = MaterialName( material );

    if ( strcmp( mtlName, registeredName ) )
        return FindMaskMaterial( registeredName, usage );

    mskMtl = &mskMtls[mskMtlCount];
    mskMtlCount++;

    mskMtl->material   = material;
    mskMtl->techSet    = ref->techSet;
    mskMtl->usage      = usage;

    Image_Register( MaterialImageName( material ), &image );

    if ( !image.pixels )
    {
        mskMtl->maskWidth  = 0;
        mskMtl->maskHeight = 0;
        mskMtl->mask       = NULL;
        mskMtl->colorMask   = NULL;
        return mskMtl;
    }

    mskMtl->mask     = BuildAlphaMask( material, &image );
    mskMtl->colorMask = BuildColorMask( &image );

    mskMtl->maskWidth  = image.width;
    mskMtl->maskHeight = image.height;

    Z_Free( image.pixels );

    return mskMtl;
}

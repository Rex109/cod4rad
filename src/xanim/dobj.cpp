/* Original: c:\trees\cod3\cod3src\src\xanim\dobj.cpp */

#include "cod4rad.h"
#include "dobj.h"
#include "scr_stringlist.h"
#include "progress.h"

#include <stdlib.h>
#include <string.h>


/* dobjEmptySkelName  0x130cd224 */
static unsigned short dobjEmptySkelName;

#define DOBJ_EMPTY_SKEL_LEN     0x11
#define DOBJ_SKEL_STRING_TYPE   0x0c


static qboolean IsFiniteFloat( float value )
{
    return ( *( const int * )&value & 0x7f800000 ) != 0x7f800000;
}

/* DObjAnimMatToAxis  0x004220a0 */
void DObjAnimMatToAxis( const DObjAnimMat_t *mat, float axis[3][3] )
{
    float s;
    float xs, ys, zs;
    float xx, xy, xz, xw;
    float yy, yz, yw;
    float zz, zw;

    Assert( IsFiniteFloat( mat->quat[0] ) );
    Assert( IsFiniteFloat( mat->quat[1] ) );
    Assert( IsFiniteFloat( mat->quat[2] ) );
    Assert( IsFiniteFloat( mat->quat[3] ) );
    Assert( IsFiniteFloat( mat->transWeight ) );

    s = mat->transWeight;

    xs = mat->quat[0] * s;
    ys = s * mat->quat[1];
    zs = s * mat->quat[2];

    xx = mat->quat[0] * xs;
    xy = xs * mat->quat[1];
    xz = mat->quat[2] * xs;
    xw = xs * mat->quat[3];

    yy = ys * mat->quat[1];
    yz = mat->quat[2] * ys;
    yw = ys * mat->quat[3];

    zz = mat->quat[2] * zs;
    zw = zs * mat->quat[3];

    axis[0][0] = 1.0f - ( yy + zz );
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;

    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - ( xx + zz );
    axis[1][2] = yz + xw;

    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - ( xx + yy );
}

/* DObj_SetupSkeleton  0x004225b0 */
static void DObj_SetupSkeleton( DObj_t *obj, const DObjModel_t *dobjModels,
                                unsigned numModels )
{
    XModel_t *modelPtrs[DOBJ_MAX_MODELS];
    byte      boneOffsets[DOBJ_MAX_MODELS];
    char      emptyName[20];
    byte     *models;
    int       totalBones;
    unsigned  i;

    if ( numModels != 1 )
        Error( "dobj: DObj_SetupSkeleton (0x004225b0) only has its one-model "
               "path reconstructed; this one has %i models\n", numModels );

    totalBones = 0;

    for ( i = 0; i < numModels; i++ )
    {
        XModel_t *model = dobjModels[i].model;

        Assert( model );

        totalBones += XModelNumBones( model );

        if ( totalBones > DOBJ_MAX_PARTS )
            Com_ErrorLevel( ERR_DROP, "Too many bones in DObj" );

        modelPtrs[i] = model;
        boneOffsets[i] = 0xff;

        if ( dobjModels[i].flags )
            obj->ignoreCollision |= 1 << i;
    }

    obj->numModels = ( byte )numModels;
    obj->numBones  = ( byte )totalBones;

    models = ( byte * )malloc( numModels * 5 );

    obj->models = ( XModel_t ** )models;

    memcpy( models, modelPtrs, numModels * sizeof( XModel_t * ) );
    memcpy( models + numModels * sizeof( XModel_t * ), boneOffsets, numModels );

    if ( !dobjEmptySkelName )
    {
        memset( emptyName, 0, sizeof( emptyName ) );

        dobjEmptySkelName = ( unsigned short )
            SL_GetStringOfSize( emptyName, 0, DOBJ_EMPTY_SKEL_LEN,
                                DOBJ_SKEL_STRING_TYPE );

        if ( !dobjEmptySkelName )
            Error( "dobj: could not register the empty skeleton name\n" );
    }

    obj->skelNameLen = DOBJ_EMPTY_SKEL_LEN;
    obj->skelName    = dobjEmptySkelName;
}

/* DObj_ComputeRadius  0x00422e00 */
static void DObj_ComputeRadius( DObj_t *obj )
{
    float radius;
    int   i;

    Assert( obj );

    radius = 0.0f;

    for ( i = 0; i < obj->numModels; i++ )
    {
        Assert( obj->models[i] );

        radius = XModelRadius( obj->models[i] ) + radius;
    }

    obj->radius = radius;
}

/* DObj_Create  0x00422ea0 */
void DObj_Create( const DObjModel_t *dobjModels, unsigned numModels, void *ptr00,
                  DObj_t *obj, unsigned short pad06 )
{
    Assert( dobjModels );
    Assert( numModels > 0 );
    Assert( numModels <= DOBJ_MAX_MODELS );
    Assert( obj );

    memset( &obj->partBits, 0, 0x38 );

    obj->skelNameLen     = 0;
    obj->skelName        = 0;
    obj->ignoreCollision = 0;
    obj->pad10           = 0;
    obj->pad06           = pad06;

    obj->pad50[0] = 0;
    obj->pad50[1] = 0;
    obj->pad50[2] = 0;
    obj->pad50[3] = 0;

    DObj_SetupSkeleton( obj, dobjModels, numModels );
    DObj_ComputeRadius( obj );

    obj->ptr00 = ptr00;
}

/* DObj_Free  0x00423030 */
void DObj_Free( DObj_t *obj )
{
    Assert( obj );

    if ( obj->models )
    {
        free( obj->models );
        obj->models = NULL;
    }

    if ( obj->ptr00 )
        obj->ptr00 = NULL;

    Assert( dobjEmptySkelName );

    if ( obj->skelName )
    {
        if ( obj->skelName != dobjEmptySkelName )
            SL_RemoveRefToStringOfSize( obj->skelName, obj->skelNameLen );

        obj->skelNameLen = 0;
        obj->skelName    = 0;
    }
}

/* DObj_SetAnimMats  0x00426fa0 */
void DObj_SetAnimMats( DObj_t *obj, DObjAnimMat_t *animMats,
                       const DObjModel_t *dobjModels )
{
    int i;

    memset( animMats, 0xff, obj->numBones * sizeof( DObjAnimMat_t ) );

    obj->animMats   = animMats;
    obj->dobjModels = dobjModels;

    for ( i = 0; i < 4; i++ )
    {
        Assert( !obj->partBits[0][i] );
        Assert( !obj->partBits[1][i] );
        Assert( !obj->partBits[2][i] );
    }
}

/* DObj_SetPartBits  0x00425d70 */
void DObj_SetPartBits( DObj_t *obj, const int *partBits )
{
    Assert( obj );
    Assert( partBits );

    obj->partBits[2][0] |= partBits[0];
    obj->partBits[2][1] |= partBits[1];
    obj->partBits[2][2] |= partBits[2];
    obj->partBits[2][3] |= partBits[3];
}

/* DObj_GetSurfCount  0x00426470 */
int DObj_GetSurfCount( const DObj_t *obj, int *partBits, const char *lods )
{
    unsigned window[7];
    int      surfCount;
    int      boneOffset;
    int      i;

    Assert( obj->numModels );

    boneOffset = XModelNumBones( obj->models[0] );

    if ( lods[0] >= 0 )
    {
        const XModelLodInfo_t *lodInfo = &obj->models[0]->lodInfo[( int )lods[0]];

        surfCount = lodInfo->numSurfaces;

        partBits[0] = lodInfo->partBits[0];
        partBits[1] = lodInfo->partBits[1];
        partBits[2] = lodInfo->partBits[2];
        partBits[3] = lodInfo->partBits[3];
    }
    else
    {
        surfCount = 0;

        partBits[0] = 0;
        partBits[1] = 0;
        partBits[2] = 0;
        partBits[3] = 0;
    }

    window[0] = 0;
    window[1] = 0;
    window[2] = 0;

    for ( i = 1; i < obj->numModels; i++ )
    {
        int bones = XModelNumBones( obj->models[i] );
        int lod   = lods[i];

        if ( lod >= 0 )
        {
            const XModelLodInfo_t *lodInfo = &obj->models[i]->lodInfo[lod];
            int words = boneOffset >> 5;
            int bits  = boneOffset & 31;
            int k;

            surfCount += lodInfo->numSurfaces;

            window[3] = lodInfo->partBits[0];
            window[4] = lodInfo->partBits[1];
            window[5] = lodInfo->partBits[2];
            window[6] = lodInfo->partBits[3];

            if ( bits )
            {
                for ( k = 0; k < 4; k++ )
                    partBits[k] |= ( window[3 + k - words] >> bits )
                                 | ( window[4 + k - words] << ( 32 - bits ) );
            }
            else
            {
                for ( k = 0; k < 4; k++ )
                    partBits[k] |= window[3 + k - words];
            }
        }

        boneOffset += bones;
    }

    return surfCount;
}

/* DObj_GetSurface  0x00423600 */
XSurface_t *DObj_GetSurface( const DObj_t *obj, int modelIndex, int surfIndex, int lod )
{
    const XModel_t *model;
    int             index;

    Assert( lod >= 0 );

    model = obj->models[modelIndex];
    index = model->lodInfo[lod].surfIndex + surfIndex;

    AssertIn( index, model->numSurfaces );

    return &model->surfs[index];
}

/* DObj_GetSurfaceMaterialName  0x00423680 */
const char *DObj_GetSurfaceMaterialName( const DObj_t *obj, int modelIndex,
                                         int surfIndex, int lod )
{
    const XModel_t *model;
    int             index;

    Assert( lod >= 0 );

    model = obj->models[modelIndex];

    AssertCmp( surfIndex, <, ( int )model->lodInfo[lod].numSurfaces );

    index = model->lodInfo[lod].surfIndex + surfIndex;

    AssertIn( index, model->numSurfaces );

    return model->materialNames[index];
}

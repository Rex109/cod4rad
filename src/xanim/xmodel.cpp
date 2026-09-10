/* Original: c:\trees\cod3\cod3src\src\xanim\xmodel.cpp */

#include "cod4rad.h"
#include "xmodel.h"
#include "scr_stringlist.h"


/* XModel_Register  0x0042ae80 */
XModel_t *XModel_Register( const char *name, XModelAllocFunc_t allocFunc,
                           XModelAllocFunc_t collAllocFunc )
{
    XModel_t *model;

    model = ( XModel_t * )Hunk_FindDataForFile( ASSET_TYPE_XMODEL, name );

    if ( model )
        return model;

    model = XModel_Load( name, allocFunc, collAllocFunc );

    if ( !model )
        return NULL;

    model->name = Hunk_AddDataForFile( ASSET_TYPE_XMODEL, name, model,
                                       ( HunkAllocFunc_t )allocFunc );

    return model;
}

/* XModelBadLoad  0x0042acc0 */
int XModelBadLoad( const XModel_t *model )
{
    Assert( model );

    return model->badLoad;
}

/* XModelPartsFree  0x0042acf0 */
void XModelPartsFree( XModelParts_t *modelParts )
{
    int count;
    int i;

    Assert( modelParts );

    count = modelParts->numBones;

    for ( i = 0; i < count; i++ )
        SL_RemoveRefToString( modelParts->boneNames[i] );
}

/* XModelBoneNames  0x0042aed0 */
unsigned short *XModelBoneNames( const XModel_t *model )
{
    return model->boneNames;
}

/* XModelRadius  0x0042aee0 */
float XModelRadius( const XModel_t *model )
{
    return model->radius;
}

/* XModelGetBounds  0x0042aef0 */
void XModelGetBounds( const XModel_t *model, vec3_t mins, vec3_t maxs )
{
    mins[0] = model->mins[0];
    mins[1] = model->mins[1];
    mins[2] = model->mins[2];

    maxs[0] = model->maxs[0];
    maxs[1] = model->maxs[1];
    maxs[2] = model->maxs[2];
}

/* XModelMemUsage  0x0042af40 */
int XModelMemUsage( const XModel_t *model )
{
    return model->memUsage;
}

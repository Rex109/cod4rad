/* Original: c:\trees\cod3\cod3src\src\xanim\xmodel_utils.cpp */

#include "cod4rad.h"
#include "xmodel.h"


/* XModelGetSurfaces  0x00430290 */
int XModelGetSurfaces( const XModel_t *model, XSurface_t **surfaces, int lod )
{
    const XModelLodInfo_t *lodInfo;

    Assert( model );
    Assert( surfaces );
    Assert( lod >= 0 );

    lodInfo = &model->lodInfo[lod];

    AssertIn( lodInfo->surfIndex, model->numSurfaces );
    Assert( lodInfo->surfIndex + lodInfo->numSurfaces <= model->numSurfaces );

    *surfaces = &model->surfs[lodInfo->surfIndex];

    return lodInfo->numSurfaces;
}

/* XModelGetSurface  0x00430380 */
XSurface_t *XModelGetSurface( const XModel_t *model, int lod, int surfIndex )
{
    int index;

    Assert( lod >= 0 );

    index = model->lodInfo[lod].surfIndex + surfIndex;
    AssertIn( index, model->numSurfaces );

    return &model->surfs[index];
}

/* XModelGetLodInfo  0x004303f0 */
const XModelLodInfo_t *XModelGetLodInfo( const XModel_t *model, int lod )
{
    Assert( model );
    Assert( lod >= 0 );

    return &model->lodInfo[lod];
}

/* XModelLodNumSurfaces  0x00430450 */
int XModelLodNumSurfaces( const XModel_t *model, int lod )
{
    Assert( model );
    Assert( lod >= 0 );

    return model->lodInfo[lod].numSurfaces;
}

/* XModelName  0x00430260 */
const char *XModelName( const XModel_t *model )
{
    Assert( model );

    return model->name;
}

/* XModelNumLods  0x004304c0 */
int XModelNumLods( const XModel_t *model )
{
    return model->numLods;
}

/* XModelLodRampType  0x004304b0 */
int XModelLodRampType( const XModel_t *model )
{
    return model->lodRampType;
}

/* XModelNumBones  0x004304f0 */
int XModelNumBones( const XModel_t *model )
{
    return model->numBones;
}

/* Original: c:\trees\cod3\cod3src\src\xanim\xmodel_load_obj.cpp */

#include "cod4rad.h"
#include "xmodel.h"
#include "xmodel_load_obj.h"
#include "r_xsurface_load_obj.h"
#include "scr_stringlist.h"
#include "dobj.h"

#include <float.h>
#include <math.h>
#include <string.h>


#define CON_CHANNEL_MODEL       19

#define QUAT_SCALE              ( 1.0f / 32767.0f )

#define COLL_BOUNDS_EPSILON     0.001f

#define XMODEL_BOUNDS_TEMP_SIZE 0x60000

#define XMODEL_MAX_SAMPLE_POINTS    16
#define XMODEL_SAMPLE_ITERATIONS    100
#define XMODEL_SAMPLE_EPSILON       0.25f


static byte ReadByte( const byte **pos )
{
    byte value = **pos;

    *pos += 1;

    return value;
}

static short ReadShort( const byte **pos )
{
    short value = *( const short * )*pos;

    *pos += 2;

    return value;
}

static int ReadInt( const byte **pos )
{
    int value = *( const int * )*pos;

    *pos += 4;

    return value;
}

static float ReadFloat( const byte **pos )
{
    float value = *( const float * )*pos;

    *pos += 4;

    return value;
}

static void ReadString( const byte **pos, char *dest )
{
    strcpy( dest, ( const char * )*pos );

    *pos += strlen( ( const char * )*pos ) + 1;
}


/* 0x0042ad80 */
static XModelSurfs_t *XModelSurfsFindData( const char *name )
{
    return ( XModelSurfs_t * )Hunk_FindDataForFile( ASSET_TYPE_XMODELSURFS, name );
}

/* 0x0042ada0 */
static void XModelSurfsAddData( const char *name, XModelSurfs_t *modelSurfs,
                                XModelAllocFunc_t allocFunc )
{
    Hunk_AddDataForFile( ASSET_TYPE_XMODELSURFS, name, modelSurfs,
                         ( HunkAllocFunc_t )allocFunc );
}

/* 0x0042ad40 */
static XModelParts_t *XModelPartsFindData( const char *name )
{
    return ( XModelParts_t * )Hunk_FindDataForFile( ASSET_TYPE_XMODELPARTS, name );
}

/* 0x0042ad60 */
static void XModelPartsAddData( const char *name, XModelParts_t *modelParts,
                                XModelAllocFunc_t allocFunc )
{
    Hunk_AddDataForFile( ASSET_TYPE_XMODELPARTS, name, modelParts,
                         ( HunkAllocFunc_t )allocFunc );
}


/* XModelPartsQuat  0x0042c250 */
static void XModelPartsQuat( short *quat, const byte **pos )
{
    int iQ[4];
    int lengthSq;

    quat[0] = ReadShort( pos );
    quat[1] = ReadShort( pos );
    quat[2] = ReadShort( pos );

    iQ[0] = quat[0];
    iQ[1] = quat[1];
    iQ[2] = quat[2];

    lengthSq = 0x3fff0001 - iQ[2] * iQ[2] - iQ[1] * iQ[1] - iQ[0] * iQ[0];

    if ( lengthSq > 0 )
    {
        float value;

        value = ( float )sqrt( ( double )lengthSq );
        value = value + 0.5f;
        value = ( float )floor( value );

        iQ[3] = ( int )value;
    }
    else
    {
        iQ[3] = 0;
    }

    Assert( iQ[3] == ( short )iQ[3] );

    quat[3] = ( short )iQ[3];
}

/* XModelPartsBaseMats  0x0042c310 */
static void XModelPartsBaseMats( XModelParts_t *modelParts )
{
    const byte    *parentList;
    const short   *quats;
    const float   *trans;
    DObjAnimMat_t *mat;
    int            i;

    parentList = modelParts->parentList;
    quats      = modelParts->quats;
    trans      = modelParts->trans;
    mat        = modelParts->baseMat;

    for ( i = modelParts->numRootBones; i; i-- )
    {
        mat->quat[0] = 0.0f;
        mat->quat[1] = 0.0f;
        mat->quat[2] = 0.0f;
        mat->quat[3] = 1.0f;

        mat->trans[0] = 0.0f;
        mat->trans[1] = 0.0f;
        mat->trans[2] = 0.0f;

        mat->transWeight = 2.0f;

        mat++;
    }

    for ( i = modelParts->numBones - modelParts->numRootBones; i; i-- )
    {
        DObjAnimMat_t *parent;
        vec4_t         localQuat;
        float          axis[3][3];
        float          lengthSq;

        localQuat[0] = quats[0] * QUAT_SCALE;
        localQuat[1] = quats[1] * QUAT_SCALE;
        localQuat[2] = quats[2] * QUAT_SCALE;
        localQuat[3] = quats[3] * QUAT_SCALE;

        parent = mat - *parentList;

        QuatMultiply( localQuat, parent->quat, mat->quat );

        lengthSq = mat->quat[0] * mat->quat[0]
                 + mat->quat[1] * mat->quat[1]
                 + mat->quat[2] * mat->quat[2]
                 + mat->quat[3] * mat->quat[3];

        if ( lengthSq == 0.0f )
        {
            mat->quat[3] = 1.0f;
            mat->transWeight = 2.0f;
        }
        else
        {
            mat->transWeight = 2.0f / lengthSq;
        }

        DObjAnimMatToAxis( parent, axis );

        mat->trans[0] = trans[0] * axis[0][0] + trans[1] * axis[1][0]
                      + trans[2] * axis[2][0] + parent->trans[0];
        mat->trans[1] = trans[0] * axis[0][1] + trans[1] * axis[1][1]
                      + trans[2] * axis[2][1] + parent->trans[1];
        mat->trans[2] = trans[0] * axis[0][2] + trans[1] * axis[1][2]
                      + trans[2] * axis[2][2] + parent->trans[2];

        quats += 4;
        trans += 3;
        parentList++;
        mat++;
    }
}

/* XModelLoadParts  0x0042c4e0 */
static XModelParts_t *XModelLoadParts( const char *name, XModelAllocFunc_t allocFunc,
                                       XModel_t *model )
{
    char           filename[64];
    void          *buf;
    const byte    *pos;
    XModelParts_t *modelParts;
    unsigned short *boneNames;
    byte          *parentList;
    short         *quatList;
    float         *transList;
    int            len;
    int            version;
    int            numBones;
    int            numRootBones;
    int            totalBones;
    int            hasTrans;
    int            i;

    if ( Com_sprintf( filename, sizeof( filename ), "xmodelparts/%s", name ) < 0 )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: filename '%s' too long\n", filename );
        return NULL;
    }

    len = FS_ReadFile( filename, &buf );

    if ( len < 0 )
    {
        Assert( !buf );

        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodelparts '%s' not found\n", name );
        return NULL;
    }

    if ( !len )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodelparts '%s' has 0 length\n", name );
        FS_FreeFile( buf );
        return NULL;
    }

    Assert( buf );

    pos = ( const byte * )buf;

    version = ReadShort( &pos );

    if ( version != XMODEL_VERSION )
    {
        FS_FreeFile( buf );
        Com_Printf( CON_CHANNEL_MODEL,
                    "ERROR: xmodelparts '%s' out of date (version %d, expecting %d).\n",
                    name, version, XMODEL_VERSION );
        return NULL;
    }

    numBones     = ReadShort( &pos );
    numRootBones = ReadShort( &pos );

    totalBones = numBones + numRootBones;

    boneNames = ( unsigned short * )allocFunc( totalBones * 2 );
    model->memUsage += totalBones * 2;

    if ( totalBones >= XMODEL_MAX_BONES )
    {
        FS_FreeFile( buf );
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodelparts '%s' has too many bones\n", name );
        return NULL;
    }

    parentList = numBones ? ( byte * )allocFunc( numBones ) : NULL;
    model->memUsage += numBones;

    modelParts = ( XModelParts_t * )allocFunc( sizeof( XModelParts_t ) );
    model->memUsage += sizeof( XModelParts_t );

    modelParts->parentList = parentList;
    modelParts->boneNames  = boneNames;

    modelParts->baseMat = ( DObjAnimMat_t * )allocFunc( totalBones * 32 );
    model->memUsage += totalBones * 32;

    if ( numBones )
    {
        modelParts->quats = ( short * )allocFunc( numBones * 8 );
        model->memUsage += numBones * 8;

        modelParts->trans = ( float * )allocFunc( numBones * 16 );
        model->memUsage += numBones * 16;
    }
    else
    {
        modelParts->quats = NULL;
        modelParts->trans = NULL;
    }

    modelParts->partClassification = ( byte * )allocFunc( totalBones );
    model->memUsage += totalBones;

    modelParts->numBones = ( byte )totalBones;

    Assert( modelParts->numBones == totalBones );

    modelParts->numRootBones = ( byte )numRootBones;

    Assert( modelParts->numRootBones == numRootBones );

    quatList  = modelParts->quats;
    transList = modelParts->trans;

    for ( i = numRootBones; i < totalBones; i++ )
    {
        int index;

        index = ReadByte( &pos );

        Assert( index >= 0 );
        Assert( index < i );

        *parentList = ( byte )( i - index );

        Assert( i - index == *parentList );

        parentList++;

        transList[0] = ReadFloat( &pos );
        transList[1] = ReadFloat( &pos );
        transList[2] = ReadFloat( &pos );

        XModelPartsQuat( quatList, &pos );

        quatList  += 4;
        transList += 3;
    }

    for ( i = 0; i < totalBones; i++ )
    {
        int nameLen = I_strlen( ( const char * )pos ) + 1;

        modelParts->boneNames[i] = ( unsigned short )
            SL_GetStringOfSize( ( const char * )pos, 0, nameLen, 10 );

        pos += nameLen;
    }

    memcpy( modelParts->partClassification, pos, totalBones );
    pos += totalBones;

    hasTrans = ( *pos != 0 );

    FS_FreeFile( buf );

    XModelPartsBaseMats( modelParts );

    if ( !hasTrans )
        memset( modelParts->trans, 0, numBones * 16 );

    return modelParts;
}

/* XModelGetModelParts  0x0042cdc0 */
static XModelParts_t *XModelGetModelParts( XModel_t *model, const char *lodFileName,
                                           XModelAllocFunc_t allocFunc )
{
    XModelParts_t *modelParts;

    modelParts = XModelPartsFindData( lodFileName );

    if ( modelParts )
        return modelParts;

    modelParts = XModelLoadParts( lodFileName, allocFunc, model );

    if ( !modelParts )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: Cannot find xmodelparts '%s'.\n", lodFileName );
        return NULL;
    }

    XModelPartsAddData( lodFileName, modelParts, allocFunc );

    return modelParts;
}

/* XModelLoadCollision  0x0042ca80 */
static void XModelLoadCollision( XModel_t *model, XModelAllocFunc_t allocFunc,
                                 const char *name, const byte **pos )
{
    XModelCollSurf_t *collSurf;
    int i, j;

    Assert( !model->contents );

    model->numCollSurfs = ReadInt( pos );

    if ( !model->numCollSurfs )
    {
        Assert( !model->collSurfs );
        return;
    }

    model->collSurfs = ( XModelCollSurf_t * )
                       allocFunc( model->numCollSurfs * sizeof( XModelCollSurf_t ) );

    for ( i = 0; i < model->numCollSurfs; i++ )
    {
        collSurf = &model->collSurfs[i];

        collSurf->numCollTris = ReadInt( pos );

        Assert( collSurf->numCollTris );

        collSurf->collTris = ( XModelCollTri_t * )
                             allocFunc( collSurf->numCollTris * sizeof( XModelCollTri_t ) );

        for ( j = 0; j < collSurf->numCollTris; j++ )
        {
            XModelCollTri_t *tri = &collSurf->collTris[j];
            float plane[4];
            float svec[4];
            float tvec[4];
            float lengthSq;
            float length;

            plane[0] = ReadFloat( pos );
            plane[1] = ReadFloat( pos );
            plane[2] = ReadFloat( pos );
            plane[3] = ReadFloat( pos );

            lengthSq = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];
            length = ( float )sqrt( ( double )lengthSq );

            Assertx( I_fabs( length - 1.0f ) < 0.01f, "%s", name );

            svec[0] = ReadFloat( pos );
            svec[1] = ReadFloat( pos );
            svec[2] = ReadFloat( pos );
            svec[3] = ReadFloat( pos );

            tvec[0] = ReadFloat( pos );
            tvec[1] = ReadFloat( pos );
            tvec[2] = ReadFloat( pos );
            tvec[3] = ReadFloat( pos );

            tri->plane[0] = plane[0];
            tri->plane[1] = plane[1];
            tri->plane[2] = plane[2];
            tri->plane[3] = plane[3];

            tri->svec[0] = svec[0];
            tri->svec[1] = svec[1];
            tri->svec[2] = svec[2];
            tri->svec[3] = svec[3];

            tri->tvec[0] = tvec[0];
            tri->tvec[1] = tvec[1];
            tri->tvec[2] = tvec[2];
            tri->tvec[3] = tvec[3];
        }

        collSurf->mins[0] = ReadFloat( pos ) - COLL_BOUNDS_EPSILON;
        collSurf->mins[1] = ReadFloat( pos ) - COLL_BOUNDS_EPSILON;
        collSurf->mins[2] = ReadFloat( pos ) - COLL_BOUNDS_EPSILON;

        collSurf->maxs[0] = ReadFloat( pos ) + COLL_BOUNDS_EPSILON;
        collSurf->maxs[1] = ReadFloat( pos ) + COLL_BOUNDS_EPSILON;
        collSurf->maxs[2] = ReadFloat( pos ) + COLL_BOUNDS_EPSILON;

        collSurf->boneIdx  = ReadInt( pos );
        collSurf->contents = ReadInt( pos ) & 0xdffffffb;

        Assert( !collSurf->contents || ( collSurf->boneIdx >= 0 ) );

        collSurf->surfFlags = ReadInt( pos );

        model->contents |= collSurf->contents;
    }
}

/* XModelCopyModelParts  0x0042ce10 */
static void XModelCopyModelParts( const XModelParts_t *modelParts, XModel_t *model )
{
    model->numBones           = modelParts->numBones;
    model->numRootBones       = modelParts->numRootBones;
    model->boneNames          = modelParts->boneNames;
    model->parentList         = modelParts->parentList;
    model->quats              = modelParts->quats;
    model->trans              = modelParts->trans;
    model->partClassification = modelParts->partClassification;
    model->baseMat            = modelParts->baseMat;
}

/* XSurfaceLoadAll  0x0042ce60 */
static void XSurfaceLoadAll( XModelSurfs_t *modelSurfs, XModel_t *model, int surfCount,
                             const byte **pos, XModelAllocFunc_t allocFunc )
{
    XSurface_t *surfs;
    int i;

    surfs = modelSurfs->surfs;

    Assert( surfs );
    Assert( modelSurfs );
    Assert( surfCount > 0 );
    Assert( pos );
    Assert( *pos );

    for ( i = 0; i < surfCount; i++ )
    {
        XSurfaceLoad( model, pos, allocFunc, &surfs[i] );

        modelSurfs->partBits[0] |= surfs[i].partBits[0];
        modelSurfs->partBits[1] |= surfs[i].partBits[1];
        modelSurfs->partBits[2] |= surfs[i].partBits[2];
        modelSurfs->partBits[3] |= surfs[i].partBits[3];
    }
}

/* XModelLoadSurfs  0x0042cf60 */
static XModelSurfs_t *XModelLoadSurfs( XModel_t *model, XModelAllocFunc_t allocFunc,
                                       const char *name, const char *lodFileName,
                                       int numSurfs )
{
    char           filename[64];
    void          *buf;
    const byte    *pos;
    XModelSurfs_t *modelSurfs;
    int            len;
    int            version;
    int            surfCount;
    int            size;

    if ( Com_sprintf( filename, sizeof( filename ), "xmodelsurfs/%s", lodFileName ) < 0 )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: filename '%s' too long\n", filename );
        return NULL;
    }

    len = FS_ReadFile( filename, &buf );

    if ( len < 0 )
    {
        Assert( !buf );

        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodelsurf '%s' not found\n", lodFileName );
        return NULL;
    }

    if ( !len )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodelsurf '%s' has 0 length\n", lodFileName );
        FS_FreeFile( buf );
        return NULL;
    }

    Assert( buf );

    pos = ( const byte * )buf;

    version = ReadShort( &pos );

    if ( version != XMODEL_VERSION )
    {
        FS_FreeFile( buf );
        Com_Printf( CON_CHANNEL_MODEL,
                    "ERROR: xmodelsurfs '%s' out of date (version %d, expecting %d).\n",
                    lodFileName, version, XMODEL_VERSION );
        return NULL;
    }

    surfCount = ReadShort( &pos );

    if ( surfCount != ( short )numSurfs )
    {
        FS_FreeFile( buf );
        Com_Printf( CON_CHANNEL_MODEL,
                    "ERROR: File conflict (between non-iwd and iwd file) on xmodelsurfs "
                    "'%s' for xmodel '%s'.\nRun cleaniwds.bat to remove the stale iwd file.\n",
                    lodFileName, name );
        return NULL;
    }

    size = numSurfs * sizeof( XSurface_t ) + sizeof( XModelSurfs_t );

    modelSurfs = ( XModelSurfs_t * )allocFunc( size );
    model->memUsage += size;

    modelSurfs->surfs = ( XSurface_t * )( modelSurfs + 1 );

    XSurfaceLoadAll( modelSurfs, model, numSurfs, &pos, allocFunc );

    FS_FreeFile( buf );

    return modelSurfs;
}

/* XModelGetModelSurfs  0x0042d100 */
static qboolean XModelGetModelSurfs( XModel_t *model, XModelAllocFunc_t allocFunc,
                                     int numSurfs, const char *name,
                                     const char *lodFileName, XModelSurfs_t *out )
{
    XModelSurfs_t *modelSurfs;

    modelSurfs = XModelSurfsFindData( lodFileName );

    if ( modelSurfs )
    {
        *out = *modelSurfs;
        return qtrue;
    }

    modelSurfs = XModelLoadSurfs( model, allocFunc, name, lodFileName, numSurfs );

    if ( !modelSurfs )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: Cannot find 'xmodelsurfs '%s'.\n", lodFileName );
        return qfalse;
    }

    XModelSurfsAddData( lodFileName, modelSurfs, allocFunc );

    *out = *modelSurfs;

    return qtrue;
}

/* XModelGetTransformedBounds  0x0042d1a0 */
void XModelGetTransformedBounds( const XModel_t *model, const float axis[3][3],
                                 vec3_t mins, vec3_t maxs )
{
    XSurface_t *surfaces;
    vec3_t     *xyz;
    int         surfaceCount;
    int         vertCount;
    int         i, j;

    mins[0] = FLT_MAX;
    mins[1] = FLT_MAX;
    mins[2] = FLT_MAX;

    maxs[0] = -FLT_MAX;
    maxs[1] = -FLT_MAX;
    maxs[2] = -FLT_MAX;

    surfaceCount = XModelGetSurfaces( model, &surfaces, 0 );

    Assert( surfaces );

    xyz = ( vec3_t * )Hunk_AllocateTempMemory( XMODEL_BOUNDS_TEMP_SIZE );

    Assert( surfaceCount > 0 );

    for ( i = 0; i < surfaceCount; i++ )
    {
        vertCount = XSurfaceGetVertCount( &surfaces[i] );

        XSurfaceCopyVertexData( &surfaces[i], xyz, NULL, NULL );

        Assert( vertCount > 0 );

        for ( j = 0; j < vertCount; j++ )
        {
            float value;

            value = xyz[j][0] * axis[0][0] + xyz[j][1] * axis[1][0] + xyz[j][2] * axis[2][0];

            if ( value < mins[0] )
                mins[0] = value;

            if ( maxs[0] < value )
                maxs[0] = value;

            value = xyz[j][0] * axis[0][1] + xyz[j][1] * axis[1][1] + xyz[j][2] * axis[2][1];

            if ( value < mins[1] )
                mins[1] = value;

            if ( maxs[1] < value )
                maxs[1] = value;

            value = xyz[j][0] * axis[0][2] + xyz[j][1] * axis[1][2] + xyz[j][2] * axis[2][2];

            if ( value < mins[2] )
                mins[2] = value;

            if ( maxs[2] < value )
                maxs[2] = value;
        }
    }

    Hunk_FreeTempMemory( xyz );
}

/* ClusterPoints  0x0042d390 */
static unsigned int ClusterPoints( int numPoints, const vec3_t *points,
                                   vec3_t *dirs, unsigned int count,
                                   float *maxDistSq )
{
    vec3_t       sums[XMODEL_MAX_SAMPLE_POINTS];
    unsigned int counts[XMODEL_MAX_SAMPLE_POINTS];
    unsigned int usedCount;
    unsigned int i;
    int          j;

    AssertCmp( ( int )count, <=, XMODEL_MAX_SAMPLE_POINTS );

    memset( sums, 0, count * sizeof( vec3_t ) );
    memset( counts, 0, count * sizeof( unsigned int ) );

    for ( j = 0; j < numPoints; j++ )
    {
        float best;
        unsigned int bestIndex;

        best = Vec3DistanceSq( points[j], dirs[0] );
        bestIndex = 0;

        for ( i = 1; i < count; i++ )
        {
            float distSq = Vec3DistanceSq( points[j], dirs[i] );

            if ( distSq < best )
            {
                best = distSq;
                bestIndex = i;
            }
        }

        counts[bestIndex]++;

        sums[bestIndex][0] = points[j][0] + sums[bestIndex][0];
        sums[bestIndex][1] = points[j][1] + sums[bestIndex][1];
        sums[bestIndex][2] = points[j][2] + sums[bestIndex][2];
    }

    *maxDistSq = 0.0f;
    usedCount = 0;

    for ( i = 0; i < count; i++ )
    {
        float  memberCount;
        float  invCount;
        vec3_t average;
        float  distSq;

        memberCount = ( float )counts[i];

        if ( memberCount == 0.0f )
            continue;

        invCount = 1.0f / memberCount;

        average[0] = invCount * sums[i][0];
        average[1] = sums[i][1] * invCount;
        average[2] = invCount * sums[i][2];

        distSq = Vec3DistanceSq( average, dirs[i] );

        if ( distSq > *maxDistSq )
            *maxDistSq = distSq;

        dirs[usedCount][0] = average[0];
        dirs[usedCount][1] = average[1];
        dirs[usedCount][2] = average[2];

        usedCount++;
    }

    Assert( usedCount );

    return usedCount;
}

/* XModelGetSamplePoints  0x0042d5a0 */
unsigned int XModelGetSamplePoints( const XModel_t *model, vec3_t *points,
                                    unsigned int count )
{
    XSurface_t  *surfaces;
    vec3_t      *xyz;
    unsigned int surfCount;
    unsigned int totalVerts;
    unsigned int i;
    int          iteration;

    surfCount = XModelGetSurfaces( model, &surfaces, 0 );

    Assert( surfaces );
    Assert( surfCount > 0 );

    xyz = ( vec3_t * )Hunk_AllocateTempMemory( surfCount * XMODEL_BOUNDS_TEMP_SIZE );

    totalVerts = 0;

    for ( i = 0; i < surfCount; i++ )
    {
        int vertCount = XSurfaceGetVertCount( &surfaces[i] );

        XSurfaceCopyVertexData( &surfaces[i], &xyz[totalVerts], NULL, NULL );

        totalVerts += vertCount;
    }

    for ( i = 0; i < count; i++ )
        Vec3Copy( xyz[totalVerts * ( 2 * i + 1 ) / ( 2 * count )], points[i] );

    for ( iteration = 0; iteration < XMODEL_SAMPLE_ITERATIONS; iteration++ )
    {
        float maxDistSq;

        count = ClusterPoints( totalVerts, xyz, points, count, &maxDistSq );

        if ( XMODEL_SAMPLE_EPSILON > maxDistSq )
            break;
    }

    Hunk_FreeTempMemory( xyz );

    return count;
}

/* XModelLoadConfig  0x0042c910 */
static qboolean XModelLoadConfig( const char *name, XModelConfig_t *config,
                                  const byte **pos )
{
    int version;
    int i;

    version = ReadShort( pos );

    if ( version != XMODEL_VERSION )
    {
        Com_Printf( CON_CHANNEL_MODEL,
                    "ERROR: xmodel '%s' out of date (version %d, expecting %d).\n",
                    name, version, XMODEL_VERSION );
        return qfalse;
    }

    config->flags = ReadByte( pos );

    config->mins[0] = ReadFloat( pos );
    config->mins[1] = ReadFloat( pos );
    config->mins[2] = ReadFloat( pos );

    config->maxs[0] = ReadFloat( pos );
    config->maxs[1] = ReadFloat( pos );
    config->maxs[2] = ReadFloat( pos );

    ReadString( pos, config->name );

    for ( i = 0; i < MAX_XMODEL_LODS; i++ )
    {
        config->entries[i].lodDist = ReadFloat( pos );

        ReadString( pos, config->entries[i].filename );
    }

    config->collLod = ReadInt( pos );

    return qtrue;
}

/* XModel_Load  0x0042dc00 */
XModel_t *XModel_Load( const char *name, XModelAllocFunc_t allocFunc,
                       XModelAllocFunc_t collAllocFunc )
{
    XModelConfig_t   config;
    XModelSurfs_t    modelSurfs;
    XModelParts_t   *modelParts;
    XModelLodInfo_t *modelLodInfo;
    XBoneInfo_t     *boneInfo;
    XModel_t        *model;
    char             filename[64];
    char             materialName[256];
    float            axis[3][3];
    const byte      *pos;
    const byte      *lodPos;
    void            *buf;
    int              len;
    int              numsurfs;
    int              surfIndex;
    int              numBones;
    int              size;
    int              i, j;

    if ( Com_IsXModelName( name ) )
    {
        Com_Printf( CON_CHANNEL_MODEL,
                    "ERROR: Remove xmodel prefix from model name '%s'\n", name );
        return NULL;
    }

    if ( Com_sprintf( filename, sizeof( filename ), "xmodel/%s", name ) < 0 )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: filename '%s' too long\n", filename );
        return NULL;
    }

    len = FS_ReadFile( filename, &buf );

    if ( len < 0 )
    {
        Assert( !buf );

        if ( strstr( name, "shadow" ) )
            return NULL;

        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodel '%s' not found\n", name );
        return NULL;
    }

    if ( !len )
    {
        Com_Printf( CON_CHANNEL_MODEL, "ERROR: xmodel '%s' has 0 length\n", name );
        FS_FreeFile( buf );
        return NULL;
    }

    pos = ( const byte * )buf;

    if ( !XModelLoadConfig( name, &config, &pos ) )
    {
        FS_FreeFile( buf );
        return NULL;
    }

    model = ( XModel_t * )allocFunc( sizeof( XModel_t ) );
    model->memUsage = sizeof( XModel_t );

    XModelLoadCollision( model, collAllocFunc, name, &pos );

    model->numLods = 0;
    numsurfs = 0;

    lodPos = pos;

    modelLodInfo = model->lodInfo;

    for ( i = 0; i < MAX_XMODEL_LODS; i++ )
    {
        float dist;

        if ( config.entries[i].filename[0] )
        {
            Assert( i == model->numLods );

            model->numLods++;

            modelLodInfo->numSurfaces = ReadShort( &pos );
            numsurfs += modelLodInfo->numSurfaces;

            for ( j = modelLodInfo->numSurfaces; j; j-- )
                pos += strlen( ( const char * )pos ) + 1;
        }

        if ( config.entries[i].lodDist == 0.0f )
            dist = 1000000.0f;
        else
            dist = config.entries[i].lodDist;

        modelLodInfo->dist = dist;

        modelLodInfo++;
    }

    Assert( model->numLods );

    modelParts = XModelGetModelParts( model, config.entries[0].filename, allocFunc );

    if ( !modelParts )
    {
        FS_FreeFile( buf );
        return NULL;
    }

    XModelCopyModelParts( modelParts, model );

    numBones = model->numBones;
    size = numBones * sizeof( XBoneInfo_t );

    boneInfo = ( XBoneInfo_t * )allocFunc( size );
    model->memUsage += size;

    for ( i = 0; i < numBones; i++ )
    {
        float mid;
        float t0, t1, t2;

        boneInfo[i].mins[0] = ReadFloat( &pos );
        boneInfo[i].mins[1] = ReadFloat( &pos );
        boneInfo[i].mins[2] = ReadFloat( &pos );

        boneInfo[i].maxs[0] = ReadFloat( &pos );
        boneInfo[i].maxs[1] = ReadFloat( &pos );
        boneInfo[i].maxs[2] = ReadFloat( &pos );

        boneInfo[i].midPoint[0] = ( boneInfo[i].maxs[0] + boneInfo[i].mins[0] ) * 0.5f;
        boneInfo[i].midPoint[1] = ( boneInfo[i].mins[1] + boneInfo[i].maxs[1] ) * 0.5f;

        mid = ( boneInfo[i].maxs[2] + boneInfo[i].mins[2] ) * 0.5f;
        boneInfo[i].midPoint[2] = mid;

        t0 = boneInfo[i].maxs[0] - boneInfo[i].midPoint[0];
        t1 = boneInfo[i].maxs[1] - boneInfo[i].midPoint[1];
        t2 = boneInfo[i].maxs[2] - mid;

        boneInfo[i].radiusSquared = t0 * t0 + t1 * t1 + t2 * t2;
    }

    model->boneInfo = boneInfo;
    model->lodRampType = 0;

    pos = lodPos;

    Assert( config.entries[0].filename[0] );

    model->numSurfaces = ( byte )numsurfs;

    Assert( model->numSurfaces == numsurfs );

    model->surfs = ( XSurface_t * )allocFunc( numsurfs * sizeof( XSurface_t ) );
    model->materialNames = ( char ** )allocFunc( numsurfs * 4 );

    surfIndex = 0;
    modelLodInfo = model->lodInfo;

    for ( i = 0; i < MAX_XMODEL_LODS; i++ )
    {
        if ( config.entries[i].filename[0] )
        {
            pos += 2;

            if ( !XModelGetModelSurfs( model, allocFunc, modelLodInfo->numSurfaces,
                                       name, config.entries[i].filename, &modelSurfs ) )
            {
                FS_FreeFile( buf );
                return NULL;
            }

            modelLodInfo->partBits[0] = modelSurfs.partBits[0];
            modelLodInfo->partBits[1] = modelSurfs.partBits[1];
            modelLodInfo->partBits[2] = modelSurfs.partBits[2];
            modelLodInfo->partBits[3] = modelSurfs.partBits[3];

            modelLodInfo->surfIndex = ( unsigned short )surfIndex;

            Assert( modelLodInfo->surfIndex == surfIndex );

            for ( j = 0; j < modelLodInfo->numSurfaces; j++ )
            {
                const char *matName;

                matName = ( const char * )pos;
                pos += strlen( matName ) + 1;

                if ( !strcmp( matName, "$default" ) )
                    matName = "$default3d";

                I_strncpyz( materialName, matName, sizeof( materialName ) );
                Q_strlwr( materialName );

                model->materialNames[surfIndex] =
                    ( char * )allocFunc( I_strlen( materialName ) + 1 );

                strcpy( model->materialNames[surfIndex], materialName );

                if ( modelSurfs.surfs[j].deformed )
                    model->lodRampType = 1;

                memcpy( &model->surfs[surfIndex], &modelSurfs.surfs[j],
                        sizeof( XSurface_t ) );

                surfIndex++;
            }
        }

        modelLodInfo++;
    }

    Assert( surfIndex == numsurfs );

    axis[0][0] = 1.0f; axis[0][1] = 0.0f; axis[0][2] = 0.0f;
    axis[1][0] = 0.0f; axis[1][1] = 1.0f; axis[1][2] = 0.0f;
    axis[2][0] = 0.0f; axis[2][1] = 0.0f; axis[2][2] = 1.0f;

    XModelGetTransformedBounds( model, axis, model->mins, model->maxs );

    FS_FreeFile( buf );

    Assert( config.maxs[0] >= 0.0f );

    model->radius = config.maxs[0];

    model->collLod = ( short )config.collLod;

    Assert( model->collLod < model->numLods );

    model->flags = config.flags;
    model->physPreset = NULL;
    model->physCollmap = NULL;

    return model;
}

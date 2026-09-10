/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_xsurface_load_obj.cpp */

#include "cod4rad.h"
#include "r_xsurface.h"
#include "r_xsurface_load_obj.h"
#include "xmodel.h"

#include <string.h>


#define EQUAL_EPSILON           0.001f

#define MAX_RIGID_VERT_LISTS    129


static byte ReadByte( const byte **pos )
{
    byte value = **pos;

    *pos += 1;

    return value;
}

static unsigned short ReadShort( const byte **pos )
{
    unsigned short value = *( const unsigned short * )*pos;

    *pos += 2;

    return value;
}

static unsigned int ReadInt( const byte **pos )
{
    unsigned int value = *( const unsigned int * )*pos;

    *pos += 4;

    return value;
}

static float ReadFloat( const byte **pos )
{
    float value = *( const float * )*pos;

    *pos += 4;

    return value;
}

static void MarkBone( XSurface_t *surface, int boneIndex )
{
    surface->partBits[boneIndex >> 5] |= 0x80000000u >> ( boneIndex & 31 );
}


/* XSurfaceLoad  0x004273d0 */
void XSurfaceLoad( XModel_t *model, const byte **pos, XModelAllocFunc_t allocFunc,
                   XSurface_t *surface )
{
    XRigidVertList   rigidVertListArray[MAX_RIGID_VERT_LISTS];
    int              weightCount[4];
    XVertUnpacked_t *verts;
    XVertUnpacked_t *vertsBase;
    unsigned short  *vert0Out, *vert1Out, *vert2Out, *vert3Out;
    unsigned short  *verts0, *verts1, *verts2, *verts3;
    unsigned short  *vertsBlendOut;
    int    vertListCount;
    int    totalVerts;
    int    blendVertCount;
    int    allocCount;
    int    indexCount;
    int    vertIndex;
    int    triIndex;
    int    startTriIndex;
    int    boneIndex;
    int    numWeights;
    int    deformed;
    int    size;
    int    i, j;

    weightCount[0] = 0;
    weightCount[1] = 0;
    weightCount[2] = 0;
    weightCount[3] = 0;

    surface->tileMode = **pos;
    *pos += 3;

    surface->vertCount = ReadShort( pos );
    surface->triCount  = ReadShort( pos );

    Assertx( surface->triCount > 0, "%i", surface->triCount );

    vertListCount = 0;
    totalVerts = 0;

    for ( ;; )
    {
        Assert( vertListCount < MAX_RIGID_VERT_LISTS );

        rigidVertListArray[vertListCount].vertCount = ReadShort( pos );

        if ( !rigidVertListArray[vertListCount].vertCount )
            break;

        boneIndex = ReadShort( pos );

        totalVerts += rigidVertListArray[vertListCount].vertCount;
        rigidVertListArray[vertListCount].boneOffset = ( unsigned short )( boneIndex * 64 );

        vertListCount++;
    }

    deformed = ( totalVerts != surface->vertCount );
    surface->deformed = ( byte )deformed;

    if ( deformed )
        vertListCount = 0;

    if ( vertListCount == 1 )
    {
        MarkBone( surface, rigidVertListArray[0].boneOffset >> 6 );
        blendVertCount = 0;
    }
    else
    {
        blendVertCount = ( short )ReadShort( pos );
    }

    vertsBase = ( XVertUnpacked_t * )
                Hunk_AllocateTempMemory( ( surface->vertCount * 16 + blendVertCount ) * 4 );
    verts = vertsBase;

    for ( i = 0; i < surface->vertCount; i++ )
    {
        vec3_t check;
        unsigned short *blend;

        verts->normal[0] = ReadFloat( pos );
        verts->normal[1] = ReadFloat( pos );
        verts->normal[2] = ReadFloat( pos );

        verts->color = ReadInt( pos );

        verts->texCoord0 = ReadFloat( pos );
        verts->texCoord1 = ReadFloat( pos );

        verts->binormal[0] = ReadFloat( pos );
        verts->binormal[1] = ReadFloat( pos );
        verts->binormal[2] = ReadFloat( pos );

        verts->tangent[0] = ReadFloat( pos );
        verts->tangent[1] = ReadFloat( pos );
        verts->tangent[2] = ReadFloat( pos );

        check[0] = verts->tangent[0] * verts->normal[0]
                 + verts->tangent[1] * verts->normal[1]
                 + verts->tangent[2] * verts->normal[2];

        check[1] = verts->tangent[0] * verts->binormal[0]
                 + verts->tangent[1] * verts->binormal[1]
                 + verts->tangent[2] * verts->binormal[2];

        check[2] = verts->normal[0] * verts->binormal[0]
                 + verts->normal[1] * verts->binormal[1]
                 + verts->normal[2] * verts->binormal[2];

        Assert( VecNCompareEpsilon( check, vec3_origin, EQUAL_EPSILON * 2, 3 ) );

        if ( vertListCount == 1 )
        {
            Assert( !deformed );

            verts->numWeights = 0;
            verts->boneOffset = rigidVertListArray[0].boneOffset;

            verts->xyz[0] = ReadFloat( pos );
            verts->xyz[1] = ReadFloat( pos );
            verts->xyz[2] = ReadFloat( pos );

            verts++;
            continue;
        }

        numWeights = ReadByte( pos );
        verts->numWeights = ( byte )numWeights;

        Assertx( numWeights < 4, "%i", numWeights );

        weightCount[numWeights]++;

        boneIndex = ( short )ReadShort( pos );
        MarkBone( surface, boneIndex );
        verts->boneOffset = ( unsigned short )( boneIndex * 64 );

        Assert( boneIndex * 64 == verts->boneOffset );

        verts->xyz[0] = ReadFloat( pos );
        verts->xyz[1] = ReadFloat( pos );
        verts->xyz[2] = ReadFloat( pos );

        blend = ( unsigned short * )( verts + 1 );
        verts++;

        if ( numWeights )
        {
            Assert( deformed );

            for ( j = 0; j < numWeights; j++ )
            {
                boneIndex = ( short )ReadShort( pos );
                MarkBone( surface, boneIndex );

                blend[0] = ( unsigned short )( boneIndex * 64 );

                Assert( boneIndex * 64 == blend[0] );

                blend[1] = ReadShort( pos );
                blend += 2;
            }

            verts = ( XVertUnpacked_t * )blend;
        }
    }

    allocCount = ( surface->triCount + 1 ) & ~1;

    surface->triIndices = ( unsigned short * )allocFunc( ( surface->triCount + 1 ) * 6 );

    Assert( surface->triIndices );

    for ( indexCount = 0; indexCount < surface->triCount * 3; indexCount++ )
    {
        surface->triIndices[indexCount] = ReadShort( pos );

        Assert( surface->triIndices[indexCount] < surface->vertCount );
    }

    triIndex = 0;
    vertIndex = 0;

    for ( j = 0; j < vertListCount; j++ )
    {
        startTriIndex = triIndex;

        rigidVertListArray[j].triOffset = ( unsigned short )triIndex;

        Assert( rigidVertListArray[j].triOffset == startTriIndex );

        vertIndex += rigidVertListArray[j].vertCount;

        while ( triIndex < surface->triCount
                && surface->triIndices[triIndex * 3] < vertIndex )
            triIndex++;

        rigidVertListArray[j].triCount = ( unsigned short )( triIndex - startTriIndex );

        Assert( rigidVertListArray[j].triCount == triIndex - startTriIndex );
    }

    if ( allocCount != surface->triCount )
    {
        Assertx( allocCount == surface->triCount + 1, "%i", 1 );

        surface->triIndices[indexCount]     = surface->triIndices[indexCount - 1];
        surface->triIndices[indexCount + 1] = surface->triIndices[indexCount - 1];
        surface->triIndices[indexCount + 2] = surface->triIndices[indexCount - 1];

        surface->triCount++;
    }

    surface->vertListCount = vertListCount;

    surface->vertList = vertListCount
                      ? ( XRigidVertList * )allocFunc( vertListCount * 12 )
                      : NULL;

    memcpy( surface->vertList, rigidVertListArray, vertListCount * 12 );

    size = surface->vertCount * 32;

    surface->verts0 = ( GfxPackedVertex * )allocFunc( size );
    model->memUsage += size;

    XSurfaceSetupVertexes( vertsBase, surface->verts0, surface->verts0, surface->vertCount );

    if ( deformed )
    {
        Assert( XModelNumBones( model ) > 1 );

        verts0 = weightCount[0] ? ( unsigned short * )allocFunc( weightCount[0] *  2 ) : NULL;
        verts1 = weightCount[1] ? ( unsigned short * )allocFunc( weightCount[1] *  6 ) : NULL;
        verts2 = weightCount[2] ? ( unsigned short * )allocFunc( weightCount[2] * 10 ) : NULL;
        verts3 = weightCount[3] ? ( unsigned short * )allocFunc( weightCount[3] * 14 ) : NULL;

        for ( i = 0; i < 4; i++ )
        {
            surface->vertInfo.vertCount[i] = ( unsigned short )weightCount[i];

            Assert( surface->vertInfo.vertCount[i] == weightCount[i] );
        }

        vert0Out = verts0;
        vert1Out = verts1;
        vert2Out = verts2;
        vert3Out = verts3;

        verts = vertsBase;

        for ( i = 0; i < surface->vertCount; i++ )
        {
            unsigned short *out;
            unsigned short *blendOut;
            unsigned short *blend;

            switch ( verts->numWeights )
            {
            case 0:
                Assert( vert0Out );
                out = vert0Out;
                blendOut = NULL;
                vert0Out += 1;
                break;

            case 1:
                Assert( vert1Out );
                out = vert1Out;
                blendOut = vert1Out + 1;
                vert1Out += 3;
                break;

            case 2:
                Assert( vert2Out );
                out = vert2Out;
                blendOut = vert2Out + 1;
                vert2Out += 5;
                break;

            default:
                Assertx( verts->numWeights == 3, "%i", verts->numWeights );
                Assert( vert3Out );
                out = vert3Out;
                blendOut = vert3Out + 1;
                vert3Out += 7;
                break;
            }

            *out = verts->boneOffset;

            numWeights = verts->numWeights;

            blend = ( unsigned short * )( verts + 1 );
            verts++;

            if ( numWeights )
            {
                Assert( blendOut );

                for ( j = 0; j < numWeights; j++ )
                {
                    blendOut[0] = blend[0];
                    blendOut[1] = blend[1];

                    blendOut += 2;
                    blend += 2;
                }

                verts = ( XVertUnpacked_t * )blend;
            }
        }

        Assert( vert0Out == verts0 + surface->vertInfo.vertCount[0] );
        Assert( vert1Out == verts1 + surface->vertInfo.vertCount[1] * 3 );
        Assert( vert2Out == verts2 + surface->vertInfo.vertCount[2] * 5 );
        Assert( vert3Out == verts3 + surface->vertInfo.vertCount[3] * 7 );

        size = ( surface->vertInfo.vertCount[0]
               + surface->vertInfo.vertCount[1] * 3
               + surface->vertInfo.vertCount[2] * 5
               + surface->vertInfo.vertCount[3] * 7 ) * 2;

        vertsBlendOut = size ? ( unsigned short * )allocFunc( size ) : NULL;

        model->memUsage += size;
        surface->vertInfo.vertsBlend = vertsBlendOut;

        for ( i = 0; i < surface->vertInfo.vertCount[0]; i++ )
            *vertsBlendOut++ = verts0[i];

        for ( i = 0; i < surface->vertInfo.vertCount[1]; i++ )
        {
            vertsBlendOut[0] = verts1[i * 3 + 0];
            vertsBlendOut[1] = verts1[i * 3 + 1];
            vertsBlendOut[2] = verts1[i * 3 + 2];
            vertsBlendOut += 3;
        }

        for ( i = 0; i < surface->vertInfo.vertCount[2]; i++ )
        {
            vertsBlendOut[0] = verts2[i * 5 + 0];
            vertsBlendOut[1] = verts2[i * 5 + 1];
            vertsBlendOut[2] = verts2[i * 5 + 2];
            vertsBlendOut[3] = verts2[i * 5 + 3];
            vertsBlendOut[4] = verts2[i * 5 + 4];
            vertsBlendOut += 5;
        }

        for ( i = 0; i < surface->vertInfo.vertCount[3]; i++ )
        {
            vertsBlendOut[0] = verts3[i * 7 + 0];
            vertsBlendOut[1] = verts3[i * 7 + 1];
            vertsBlendOut[2] = verts3[i * 7 + 2];
            vertsBlendOut[3] = verts3[i * 7 + 3];
            vertsBlendOut[4] = verts3[i * 7 + 4];
            vertsBlendOut[5] = verts3[i * 7 + 5];
            vertsBlendOut[6] = verts3[i * 7 + 6];
            vertsBlendOut += 7;
        }

        Assert( ( byte * )vertsBlendOut - ( byte * )surface->vertInfo.vertsBlend == size );
    }

    Assert( surface->deformed == ( surface->vertListCount == 0 ) );

    for ( i = 0; i < surface->vertListCount; i++ )
        surface->vertList[i].collisionTree = NULL;

    Hunk_FreeTempMemory( vertsBase );
}

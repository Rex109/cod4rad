/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_xsurface.cpp */

#include "cod4rad.h"
#include "r_xsurface.h"
#include "com_pack.h"

#include <math.h>
#include <string.h>


/* XSurfaceGetVertCount  0x00454700 */
int XSurfaceGetVertCount( const XSurface_t *surface )
{
    Assert( surface );

    return surface->vertCount;
}

/* XSurfaceGetTriCount  0x00454730 */
int XSurfaceGetTriCount( const XSurface_t *surface )
{
    Assert( surface );

    return surface->triCount;
}

/* XSurfaceCopyTriIndices  0x00454760 */
void XSurfaceCopyTriIndices( const XSurface_t *surface, unsigned short *dstIndices,
                             unsigned short baseVertex )
{
    const unsigned int *src;
    unsigned int       *dst;
    unsigned int        offset;
    int                 count;

    Assert( ( reinterpret_cast< size_t >( surface->triIndices ) & 3 ) == 0 );
    Assert( ( reinterpret_cast< size_t >( dstIndices ) & 3 ) == 0 );
    Assert( ( surface->triCount & 1 ) == 0 );

    if ( !baseVertex )
    {
        memcpy( dstIndices, surface->triIndices, surface->triCount * 6 );
        return;
    }

    offset = ( ( unsigned int )baseVertex << 16 ) | baseVertex;

    src = ( const unsigned int * )surface->triIndices;
    dst = ( unsigned int * )dstIndices;

    for ( count = surface->triCount >> 1; count; count-- )
    {
        dst[0] = src[0] + offset;
        dst[1] = src[1] + offset;
        dst[2] = src[2] + offset;

        src += 3;
        dst += 3;
    }
}

/* XSurfaceCopyVertexData  0x00454840 */
void XSurfaceCopyVertexData( const XSurface_t *surface, vec3_t *xyzOut,
                             float *texCoordOut, vec3_t *normalOut )
{
    const GfxPackedVertex *vert;
    int                    count;

    vert = surface->verts0;
    Assert( vert );

    for ( count = surface->vertCount; count; count-- )
    {
        if ( normalOut )
        {
            Vec3UnpackUnitVec( vert->normal, *normalOut );
            normalOut++;
        }

        if ( texCoordOut )
        {
            Vec2UnpackTexCoords( vert->texCoord, texCoordOut );
            texCoordOut += 2;
        }

        ( *xyzOut )[0] = vert->xyz[0];
        ( *xyzOut )[1] = vert->xyz[1];
        ( *xyzOut )[2] = vert->xyz[2];

        xyzOut++;
        vert++;
    }
}

/* XSurfaceSetupVertexes  0x004548e0 */
void XSurfaceSetupVertexes( const XVertUnpacked_t *verts,
                            GfxPackedVertex *packedVerts0,
                            GfxPackedVertex *packedVerts1,
                            int vertCount )
{
    const XVertUnpacked_t *vert;
    float uSum, vSum;
    float uAvg, vAvg;
    float invCount;
    float rounded;
    qboolean uInRange, vInRange;
    int i;

    Assert( vertCount );

    uSum = 0.0f;
    vSum = 0.0f;
    uInRange = qtrue;
    vInRange = qtrue;

    for ( i = 0; i < vertCount; i++ )
    {
        uSum += verts->texCoord0;
        vSum += verts->texCoord1;

        if ( 0.0f > verts->texCoord0 || 1.0f < verts->texCoord0 )
            uInRange = qfalse;

        if ( 0.0f > verts->texCoord1 || 1.0f < verts->texCoord1 )
            vInRange = qfalse;
    }

    invCount = 1.0f / vertCount;

    uAvg = invCount * uSum;
    vAvg = invCount * vSum;

    if ( uInRange )
    {
        uAvg = 0.0f;
    }
    else
    {
        rounded = uAvg + 0.5f;
        uAvg = ( float )floor( rounded );
    }

    if ( vInRange )
    {
        vAvg = 0.0f;
    }
    else
    {
        rounded = vAvg + 0.5f;
        vAvg = ( float )floor( rounded );
    }

    vert = verts;

    for ( i = vertCount; i; i-- )
    {
        vec3_t cross;
        float  texCoord[2];
        float  dot;
        float  sign;

        packedVerts0->xyz[0] = vert->xyz[0];
        packedVerts0->xyz[1] = vert->xyz[1];
        packedVerts0->xyz[2] = vert->xyz[2];

        Vec3Cross( vert->normal, vert->tangent, cross );

        dot = vert->binormal[0] * cross[0]
            + vert->binormal[1] * cross[1]
            + vert->binormal[2] * cross[2];

        sign = ( dot < 0.0f ) ? -1.0f : 1.0f;

        packedVerts0->binormalSign = sign;

        packedVerts1->normal = Vec3PackUnitVec( vert->normal );
        packedVerts1->color  = vert->color;

        texCoord[0] = vert->texCoord0 - uAvg;
        texCoord[1] = vert->texCoord1 - vAvg;

        packedVerts1->texCoord = Vec2PackTexCoords( texCoord );
        packedVerts1->tangent  = Vec3PackUnitVec( vert->tangent );

        vert = ( const XVertUnpacked_t * )
               ( ( const byte * )vert + vert->numWeights * 4 + sizeof( XVertUnpacked_t ) );

        packedVerts0++;
        packedVerts1++;
    }
}

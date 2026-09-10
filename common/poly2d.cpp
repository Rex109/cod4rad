/* Original: ..\common\poly2d.cpp */

#include "../src/universal/q_shared.h"
#include "../src/universal/assertive.h"
#include "../src/universal/com_math.h"
#include "../src/universal/com_vector.h"
#include "polylib.h"
#include "poly2d.h"
#include "cmdlib.h"

static void SwapInt( int *a, int *b );                                    /* 0x0042b600 */

#define POLY2D_MIN_AREA_FRACTION    2.0000002e-06f

#define POLY2D_MAX_AREA_FRACTION    0.5

/* Poly2dAreaAndCentroid  0x0042ac30 */
float Poly2dAreaAndCentroid( const vec2_t *coords, int vertCount, vec2_t centroidOut )
{
    int    i;
    double doubleArea;
    double sumY;
    double sumX;

    i = vertCount - 1;

    doubleArea = coords[0][0] * ( coords[i][1] - coords[1][1] )
               + coords[i][0] * ( coords[i - 1][1] - coords[0][1] );

    sumX = coords[0][1] * ( coords[1][0] - coords[i][0] )
                        * ( coords[1][0] + coords[0][0] + coords[i][0] )
         + coords[i][1] * ( coords[0][0] - coords[i - 1][0] )
                        * ( coords[0][0] + coords[i][0] + coords[i - 1][0] );

    sumY = coords[0][0] * ( coords[i][1] - coords[1][1] )
                        * ( coords[1][1] + coords[0][1] + coords[i][1] )
         + coords[i][0] * ( coords[i - 1][1] - coords[0][1] )
                        * ( coords[0][1] + coords[i][1] + coords[i - 1][1] );

    for ( i = 1; i < vertCount - 1; i++ )
    {
        doubleArea = coords[i][0] * ( coords[i - 1][1] - coords[i + 1][1] ) + doubleArea;

        sumX = coords[i][1] * ( coords[i + 1][0] - coords[i - 1][0] )
                            * ( coords[i + 1][0] + coords[i][0] + coords[i - 1][0] ) + sumX;

        sumY = coords[i][0] * ( coords[i - 1][1] - coords[i + 1][1] )
                            * ( coords[i + 1][1] + coords[i][1] + coords[i - 1][1] ) + sumY;
    }

    centroidOut[0] = sumX / ( doubleArea * 3.0 );
    centroidOut[1] = sumY / ( doubleArea * 3.0 );

    return doubleArea;
}

/* Poly2dSplitByAxis  0x0042ae50 */
void Poly2dSplitByAxis( const vec2_t *coords, int vertCount, int axis, float dist,
                        vec2_t *coordsFront, int *frontCountOut,
                        vec2_t *coordsBack, int *backCountOut )
{
    int    i;
    int    sides[MAX_VERTS_PER_POLY];
    int    j;
    vec2_t mid;
    float  frontDist;
    float  frac;
    int    backCount;
    int    frontCount;
    float  backDist;

    Assert( coords );
    Assert( vertCount >= 3 && vertCount <= MAX_VERTS_PER_POLY );
    Assert( axis == 0 || axis == 1 );
    Assert( coordsFront );
    Assert( frontCountOut );
    Assert( coordsBack );
    Assert( backCountOut );

    Assert( coords != coordsFront );
    Assert( coords != coordsBack );
    Assert( coordsFront != coordsBack );

    *frontCountOut = 0;
    *backCountOut = 0;

    frontCount = 0;
    backCount = 0;

    frontDist = dist + 0.001f;
    backDist = dist - 0.001f;

    for ( j = 0; j < vertCount; j++ )
    {
        sides[j] = SIDE_ON;

        if ( coords[j][axis] > frontDist )
        {
            sides[j] = SIDE_FRONT;
            frontCount++;
        }
        else if ( coords[j][axis] < backDist )
        {
            sides[j] = SIDE_BACK;
            backCount++;
        }
    }

    if ( backCount == 0 )
    {
        memcpy( coordsFront, coords, vertCount * sizeof( vec2_t ) );
        *frontCountOut = vertCount;
    }
    else if ( frontCount == 0 )
    {
        memcpy( coordsBack, coords, vertCount * sizeof( vec2_t ) );
        *backCountOut = vertCount;
    }
    else
    {
        j = vertCount - 1;

        for ( i = 0; i < vertCount; j = i, i++ )
        {
            if ( sides[j] == SIDE_ON )
            {
                Vec2Copy( coords[j], coordsFront[*frontCountOut] );
                ( *frontCountOut )++;
                Vec2Copy( coords[j], coordsBack[*backCountOut] );
                ( *backCountOut )++;
            }
            else
            {
                if ( sides[j] == SIDE_FRONT )
                {
                    Vec2Copy( coords[j], coordsFront[*frontCountOut] );
                    ( *frontCountOut )++;
                }
                else if ( sides[j] == SIDE_BACK )
                {
                    Vec2Copy( coords[j], coordsBack[*backCountOut] );
                    ( *backCountOut )++;
                }

                if ( sides[i] != SIDE_ON && sides[i] != sides[j] )
                {
                    frac = ( coords[j][axis] - dist ) / ( coords[j][axis] - coords[i][axis] );

                    mid[1 - axis] = ( coords[i][1 - axis] - coords[j][1 - axis] ) * frac
                                  + coords[j][1 - axis];
                    mid[axis] = dist;

                    Vec2Copy( mid, coordsFront[*frontCountOut] );
                    ( *frontCountOut )++;
                    Vec2Copy( mid, coordsBack[*backCountOut] );
                    ( *backCountOut )++;
                }
            }
        }

        if ( *frontCountOut > MAX_VERTS_PER_POLY || *backCountOut > MAX_VERTS_PER_POLY )
            Com_Error( "MAX_VERTS_PER_POLY exceeded on 2d poly\n" );
    }
}

/* Poly2dSubdivideGrid  0x0042b350 */
void Poly2dSubdivideGrid( poly2dBuf_t *scratch, int vertCount, int countX, int countY,
                          float startX, float startY, float stepX, float stepY,
                          Poly2dGridCallback_t callback, void *userData )
{
    int    y;
    int    rowVertCount;
    float  area;
    float  curX;
    int    curVertCount;
    int    idxD;
    int    cellIndex;
    int    idxA;
    vec2_t center;
    float  maxArea;
    int    i;
    float  minArea;
    int    x;
    int    frontCount;
    float  curY;
    int    idxB;
    int    idxC;
    int    cellVertCount;

    minArea = stepX * POLY2D_MIN_AREA_FRACTION * stepY;
    maxArea = stepX * POLY2D_MAX_AREA_FRACTION * stepY;

    idxA = 0;
    curVertCount = vertCount;
    idxB = 1;
    idxC = 2;
    idxD = 3;
    cellIndex = 0;
    curY = startY;

    for ( y = 0; y < countY && curVertCount != 0; y++ )
    {
        if ( y == countY - 1 )
        {
            idxB = idxA;
            rowVertCount = curVertCount;
        }
        else
        {
            curY = curY + stepY;

            Poly2dSplitByAxis( scratch[idxA], curVertCount, 1, curY,
                               scratch[idxD], &frontCount,
                               scratch[idxB], &rowVertCount );

            SwapInt( &idxA, &idxD );
            curVertCount = frontCount;
        }

        curX = startX;

        for ( x = 0; x < countX && rowVertCount != 0; x++ )
        {
            if ( x == countX - 1 )
            {
                SwapInt( &idxB, &idxC );
                cellVertCount = rowVertCount;
            }
            else
            {
                curX = curX + stepX;

                Poly2dSplitByAxis( scratch[idxB], rowVertCount, 0, curX,
                                   scratch[idxD], &frontCount,
                                   scratch[idxC], &cellVertCount );

                SwapInt( &idxB, &idxD );
                rowVertCount = frontCount;
            }

            if ( cellVertCount >= 3 )
            {
                area = fabs( Poly2dAreaAndCentroid( scratch[idxC], cellVertCount, center ) );

                if ( minArea <= area )
                {
                    if ( area < maxArea )
                    {
                        Vec2Clear( center );

                        for ( i = 0; i < cellVertCount; i++ )
                            Vec2Add( center, scratch[idxC][i], center );

                        Vec2Scale( center, 1.0f / cellVertCount, center );
                    }

                    callback( area, center, scratch[idxC], cellVertCount, userData, cellIndex );
                    cellIndex++;
                }
            }
        }
    }
}

/* SwapInt  0x0042b600 */
static void SwapInt( int *a, int *b )
{
    int temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/* Original: ..\src\universal\com_convexhull.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "com_convexhull.h"


static void SwapIndices( int *array, int i, int j );                /* 0x00466d70 */
static void ConvexHull2DTranslate( vec2_t *points, unsigned int pointCount,
                                   const vec2_t offset );           /* 0x004675c0 */
static void ConvexHull2DFindExtremes( const vec2_t *points, int *pointOrder,
                                      unsigned int pointCount, int *hull );
                                                                    /* 0x00466c40 */
static int  InsertHullPoint( int pointIndex, unsigned int newIndex, int *hull,
                             unsigned int hullPointCount );         /* 0x004671f0 */
static int  ConvexHull2DRecurse( const vec2_t *points, int *pointOrder,
                                 unsigned int pointCount, int firstIndex,
                                 int secondIndex, int *hull, int hullPointCount );
                                                                    /* 0x00467260 */
static int  ConvexHull2DBuild( const vec2_t *points, int *pointOrder,
                               unsigned int pointCount, int *hull ); /* 0x00466db0 */


/* SwapIndices  0x00466d70 */
static void SwapIndices( int *array, int i, int j )
{
    int temp;

    temp     = array[i];
    array[i] = array[j];
    array[j] = temp;
}


/* ConvexHull2DTranslate  0x004675c0 */
static void ConvexHull2DTranslate( vec2_t *points, unsigned int pointCount, const vec2_t offset )
{
    unsigned int i;

    for ( i = 0; i < pointCount; i++ )
    {
        points[i][0] = points[i][0] + offset[0];
        points[i][1] = points[i][1] + offset[1];
    }
}


/* ConvexHull2DFindExtremes  0x00466c40 */
static void ConvexHull2DFindExtremes( const vec2_t *points, int *pointOrder,
                                      unsigned int pointCount, int *hull )
{
    unsigned int minIndex;
    unsigned int maxIndex;
    unsigned int i;

    minIndex = 0;
    maxIndex = 0;

    pointOrder[0] = 0;

    for ( i = 1; i < pointCount; i++ )
    {
        pointOrder[i] = i;

        if ( points[i][1] < points[maxIndex][1] )
        {
            if ( points[i][1] < points[minIndex][1] )
                minIndex = i;
        }
        else
        {
            maxIndex = i;
        }
    }

    SanityCheck( minIndex != maxIndex );

    hull[0] = minIndex;
    hull[1] = maxIndex;

    if ( maxIndex < minIndex )
    {
        SwapIndices( pointOrder, minIndex, pointCount - 1 );
        SwapIndices( pointOrder, maxIndex, pointCount - 2 );
    }
    else
    {
        SwapIndices( pointOrder, maxIndex, pointCount - 1 );
        SwapIndices( pointOrder, minIndex, pointCount - 2 );
    }
}


/* InsertHullPoint  0x004671f0 */
static int InsertHullPoint( int pointIndex, unsigned int newIndex, int *hull,
                            unsigned int hullPointCount )
{
    AssertRange( newIndex, 0, hullPointCount );

    memmove( &hull[newIndex + 1], &hull[newIndex], ( hullPointCount - newIndex ) * sizeof( hull[0] ) );
    hull[newIndex] = pointIndex;

    return hullPointCount + 1;
}


/* ConvexHull2DRecurse  0x00467260 */
static int ConvexHull2DRecurse( const vec2_t *points, int *pointOrder, unsigned int pointCount,
                                int firstIndex, int secondIndex, int *hull, int hullPointCount )
{
    vec2_t normal;
    float  dist;
    float  d;
    float  maxFront;
    int    frontIndex;
    int    botIndex;
    int    topIndex;

    Assert( pointCount > 0 );
    Assert( secondIndex == firstIndex + 1 ||
            ( firstIndex == hullPointCount - 1 && secondIndex == 0 ) );

    normal[0] = points[hull[firstIndex]][1] - points[hull[secondIndex]][1];
    normal[1] = points[hull[secondIndex]][0] - points[hull[firstIndex]][0];
    Vec2Normalize( normal );
    dist = Vec2Dot( normal, points[hull[firstIndex]] );

    topIndex   = pointCount - 1;
    maxFront   = 0.001f;
    frontIndex = -1;

    for ( botIndex = 0; botIndex <= topIndex; botIndex++ )
    {
        for ( ;; )
        {
            d = Vec2Dot( normal, points[pointOrder[botIndex]] ) - dist;
            if ( d < 0.0f )
                break;

            if ( maxFront < d )
            {
                maxFront   = d;
                frontIndex = botIndex;
            }

            botIndex++;
            if ( topIndex < botIndex )
                goto done;
        }

        SanityCheck( botIndex <= topIndex );

        for ( ;; )
        {
            d = Vec2Dot( normal, points[pointOrder[topIndex]] ) - dist;
            if ( 0.0f < d )
                break;

            topIndex--;
            if ( topIndex < botIndex )
                goto done;
        }

        if ( maxFront < d )
        {
            maxFront   = d;
            frontIndex = botIndex;
        }

        SanityCheck( botIndex < topIndex );

        SwapIndices( pointOrder, botIndex, topIndex );
        topIndex--;
    }

done:
    SanityCheck( topIndex == botIndex - 1 );
    SanityCheck( frontIndex <= topIndex );

    if ( frontIndex >= 0 )
    {
        SwapIndices( pointOrder, frontIndex, topIndex );
        hullPointCount = InsertHullPoint( pointOrder[topIndex], firstIndex + 1, hull, hullPointCount );

        if ( topIndex != 0 )
        {
            if ( secondIndex != 0 )
                secondIndex = firstIndex + 2;

            hullPointCount = ConvexHull2DRecurse( points, pointOrder, topIndex,
                                                  firstIndex + 1, secondIndex,
                                                  hull, hullPointCount );
            hullPointCount = ConvexHull2DRecurse( points, pointOrder, topIndex,
                                                  firstIndex, firstIndex + 1,
                                                  hull, hullPointCount );
        }
    }

    return hullPointCount;
}


/* ConvexHull2DBuild  0x00466db0 */
static int ConvexHull2DBuild( const vec2_t *points, int *pointOrder, unsigned int pointCount, int *hull )
{
    vec2_t normal;
    float  dist;
    float  d;
    float  maxFront;
    float  maxBack;
    int    frontIndex;
    int    backIndex;
    int    botIndex;
    int    topIndex;
    int    hullPointCount;

    Assert( pointCount >= 1 );

    normal[0] = points[hull[1]][1] - points[hull[0]][1];
    normal[1] = points[hull[0]][0] - points[hull[1]][0];
    Vec2Normalize( normal );
    dist = Vec2Dot( normal, points[hull[0]] );

    topIndex   = pointCount - 1;
    maxFront   =  0.001f;
    frontIndex = -1;
    maxBack    = -0.001f;
    backIndex  = -1;

    for ( botIndex = 0; botIndex <= topIndex; botIndex++ )
    {
        for ( ;; )
        {
            d = Vec2Dot( normal, points[pointOrder[botIndex]] ) - dist;
            if ( d < 0.0f )
            {
                if ( d < maxBack )
                {
                    maxBack   = d;
                    backIndex = botIndex;
                }
                break;
            }

            if ( maxFront < d )
            {
                maxFront   = d;
                frontIndex = botIndex;
            }

            botIndex++;
            if ( topIndex < botIndex )
                goto done;
        }

        SanityCheck( botIndex <= topIndex );

        for ( ;; )
        {
            d = Vec2Dot( normal, points[pointOrder[topIndex]] ) - dist;
            if ( 0.0f < d )
            {
                if ( maxFront < d )
                {
                    maxFront   = d;
                    frontIndex = botIndex;
                }
                break;
            }

            if ( d < maxBack )
            {
                maxBack   = d;
                backIndex = topIndex;
            }

            topIndex--;
            if ( topIndex < botIndex )
                goto done;
        }

        SanityCheck( botIndex < topIndex );

        SwapIndices( pointOrder, botIndex, topIndex );

        if ( backIndex == botIndex )
            backIndex = topIndex;

        topIndex--;
    }

done:
    SanityCheck( topIndex == botIndex - 1 );

    if ( frontIndex < 0 && backIndex < 0 )
        return 0;

    hullPointCount = 2;

    if ( frontIndex >= 0 )
    {
        SanityCheck( frontIndex <= topIndex );

        SwapIndices( pointOrder, frontIndex, topIndex );
        hullPointCount = InsertHullPoint( pointOrder[topIndex], 2, hull, hullPointCount );

        if ( topIndex > 0 )
        {
            hullPointCount = ConvexHull2DRecurse( points, pointOrder, topIndex,
                                                  2, 0, hull, hullPointCount );
            hullPointCount = ConvexHull2DRecurse( points, pointOrder, topIndex,
                                                  1, 2, hull, hullPointCount );
        }
    }

    if ( backIndex >= 0 )
    {
        SanityCheck( backIndex >= botIndex );

        SwapIndices( pointOrder, backIndex, botIndex );
        hullPointCount = InsertHullPoint( pointOrder[botIndex], 1, hull, hullPointCount );

        if ( pointCount != botIndex + 1 )
        {
            hullPointCount = ConvexHull2DRecurse( points, &pointOrder[botIndex + 1],
                                                  pointCount - ( botIndex + 1 ),
                                                  1, 2, hull, hullPointCount );
            hullPointCount = ConvexHull2DRecurse( points, &pointOrder[botIndex + 1],
                                                  pointCount - ( botIndex + 1 ),
                                                  0, 1, hull, hullPointCount );
        }
    }

    return hullPointCount;
}


/* ConvexHull2DPoints  0x00466ae0 */
unsigned int ConvexHull2DPoints( vec2_t *points, unsigned int pointCount, vec2_t *hull )
{
    int          pointOrder[MAX_CONVEX_HULL_POINTS];
    int          hullIndices[MAX_CONVEX_HULL_POINTS];
    vec2_t       offset;
    unsigned int hullPointCount;
    unsigned int i;

    AssertRange( pointCount, 3, ARRAY_COUNT( pointOrder ) );
    Assert( hull != points );
    Assert( hull >= points + pointCount || points >= hull + pointCount );

    Vec2Negate( points[0], offset );
    ConvexHull2DTranslate( points, pointCount, offset );

    ConvexHull2DFindExtremes( points, pointOrder, pointCount, hullIndices );
    hullPointCount = ConvexHull2DBuild( points, pointOrder, pointCount - 2, hullIndices );

    for ( i = 0; i < hullPointCount; i++ )
        Vec2Sub( points[hullIndices[i]], offset, hull[i] );

    return hullPointCount;
}


/* ConvexHull2D  0x00467620 */
int ConvexHull2D( vec2_t *points, int *pointOrder, unsigned int pointCount, int *hull )
{
    vec2_t offset;

    offset[0] = -ceilf( points[0][0] );
    offset[1] = -ceilf( points[0][1] );

    ConvexHull2DTranslate( points, pointCount, offset );

    ConvexHull2DFindExtremes( points, pointOrder, pointCount, hull );
    return ConvexHull2DBuild( points, pointOrder, pointCount - 2, hull );
}

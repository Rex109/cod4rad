/* Original: ..\common\brush_edges.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "com_shared.h"
#include "cmdlib.h"
#include "polylib.h"
#include "brush_edges.h"

/* FreeAdjacencyWinding  0x004054e0 */
void FreeAdjacencyWinding( adjacencyWinding_t *w )
{
    if ( w->numsides == FREED_WINDING_MARKER )
        Com_Error( "FreeWinding: freed a freed winding" );

    w->numsides = FREED_WINDING_MARKER;
    free( w );
}

/* CopyAdjacencyWinding  0x00405520 */
adjacencyWinding_t *CopyAdjacencyWinding( const adjacencyWinding_t *w )
{
    adjacencyWinding_t *copy;

    Assert( w );

    copy = AllocAdjacencyWinding( w->numsides );
    memcpy( copy, w, sizeof( adjacencyWinding_t ) + w->numsides * sizeof( int ) );
    return copy;
}

/* AllocAdjacencyWinding  0x00405590 */
adjacencyWinding_t *AllocAdjacencyWinding( int numsides )
{
    size_t              size;
    adjacencyWinding_t *w;

    size = sizeof( adjacencyWinding_t ) + numsides * sizeof( int );
    w = ( adjacencyWinding_t * )malloc( size );
    if ( !w )
        Com_Error( "out of memory: adjacencyWinding_t\n" );

    memset( w, 0, size );
    return w;
}

/* ReverseAdjacencyWinding  0x00405b60 */
void ReverseAdjacencyWinding( adjacencyWinding_t *w )
{
    int *first;
    int *last;
    int  tmp;

    last = ( int * )w + w->numsides;
    for ( first = w->sides; first < last; first++ )
    {
        tmp    = *first;
        *first = *last;
        *last  = tmp;
        last--;
    }
}

/* LargestTriangleArea  0x00405bc0 */
static float LargestTriangleArea( const vec3_t *pts, int ptCount, const vec3_t normal,
                                  int *outIdx0, int *outIdx1, int *outIdx2 )
{
    int    i, j, k;
    vec3_t edge0;
    vec3_t edge1;
    vec3_t cross;
    float  area;
    float  best;

    *outIdx0 = 0;
    *outIdx1 = 1;
    *outIdx2 = 2;
    best = 0.0f;

    for ( k = 2; k < ptCount; k++ )
    {
        for ( j = 1; j < k; j++ )
        {
            Vec3Sub( pts[k], pts[j], edge1 );
            for ( i = 0; i < j; i++ )
            {
                Vec3Sub( pts[i], pts[j], edge0 );
                Vec3Cross( edge0, edge1, cross );
                area = fabs( Vec3Dot( cross, normal ) );
                if ( best < area )
                {
                    *outIdx0 = i;
                    *outIdx1 = j;
                    *outIdx2 = k;
                    best = area;
                }
            }
        }
    }
    return best;
}

/* GetPtsOnPlane  0x00405d00 */
static int GetPtsOnPlane( int planeIndex, const brushPoint_t *pts, int ptCount,
                          const brushPoint_t **out, int maxOut )
{
    int outCount;
    int i;

    outCount = 0;
    for ( i = 0; i < ptCount; i++ )
    {
        if ( planeIndex == pts[i].planes[0]
          || planeIndex == pts[i].planes[1]
          || planeIndex == pts[i].planes[2] )
        {
            if ( outCount == maxOut )
                return 0;
            out[outCount] = &pts[i];
            outCount++;
        }
    }
    return outCount;
}

/* IsPtFormedByThisPlane  0x00405d90 */
bool IsPtFormedByThisPlane( int planeIndex, const brushPoint_t *pt )
{
    if ( pt->planes[0] == planeIndex )
        return true;
    if ( pt->planes[1] == planeIndex )
        return true;
    if ( pt->planes[2] == planeIndex )
        return true;
    return false;
}

/* SharedPlane  0x00405dd0 */
static bool SharedPlane( const brushPoint_t *pt0, const brushPoint_t *pt1,
                         int excludePlane, int *outPlane )
{
    int i, j;

    for ( i = 0; i < 3; i++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            if ( pt0->planes[i] == pt1->planes[j] && pt0->planes[i] != excludePlane )
            {
                *outPlane = pt0->planes[i];
                return true;
            }
        }
    }
    return false;
}

/* SecondPlane  0x00405e50 */
int SecondPlane( const brushPoint_t *pt, int planeIndex )
{
    int i;

    for ( i = 0; i < 3; i++ )
    {
        if ( pt->planes[i] != planeIndex )
            return pt->planes[i];
    }
    AssertMsg( "all planes identical" );
    return -1;
}

/* ThirdPlane  0x00405ec0 */
int ThirdPlane( const brushPoint_t *pt, int planeIndex0, int planeIndex1 )
{
    int i;

    for ( i = 0; i < 3; i++ )
    {
        if ( pt->planes[i] != planeIndex0 && pt->planes[i] != planeIndex1 )
            return pt->planes[i];
    }
    AssertMsg( "no third plane" );
    return -1;
}

/* FindPtUsingPlane  0x00405fa0 */
static const brushPoint_t **FindPtUsingPlane( int planeIndex, const brushPoint_t **first,
                                              const brushPoint_t **last )
{
    while ( first != last && !IsPtFormedByThisPlane( planeIndex, *first ) )
        first++;

    return first;
}

/* ExtractPtWithPlane  0x00405f40 */
static const brushPoint_t *ExtractPtWithPlane( int planeIndex, const brushPoint_t **first,
                                               const brushPoint_t **last )
{
    const brushPoint_t **found;
    const brushPoint_t  *pt;

    found = FindPtUsingPlane( planeIndex, first, last );
    if ( found == last )
        return NULL;

    pt = *found;
    memmove( found, found + 1, ( last - ( found + 1 ) ) * sizeof( *found ) );
    return pt;
}

/* PolyPerimeter  0x00405fe0 */
static float PolyPerimeter( const brushPoint_t * const *pts, int ptsCount )
{
    float total;
    int   i;

    Assert( ptsCount > 2 );

    total = Vec3Distance( pts[0]->xyz, pts[ptsCount - 1]->xyz );
    for ( i = 1; i < ptsCount; i++ )
        total = Vec3Distance( pts[i]->xyz, pts[i - 1]->xyz ) + total;

    return total;
}

/* PolyIsConvex  0x004061b0 */
static bool PolyIsConvex( const vec3_t *pts, unsigned int ptCount )
{
    unsigned int i;
    unsigned int prev;
    unsigned int prevPrev;
    vec3_t       edge1;
    vec3_t       edge2;
    vec3_t       normal;
    vec3_t       testNormal;
    float        len;

    Assert( ptCount > 2 );

    prev     = ptCount - 1;
    prevPrev = prev - 1;

    for ( i = 0; i < ptCount; prevPrev = prev, prev = i, i++ )
    {
        Vec3Sub( pts[prev], pts[prevPrev], edge1 );
        Vec3Sub( pts[i], pts[prev], edge2 );
        Assert( Vec3LengthSq( edge1 ) > 0 );
        Assert( Vec3LengthSq( edge2 ) > 0 );
        Vec3Cross( edge1, edge2, normal );
        len = Vec3Normalize( normal );
        if ( len >= CONVEX_CROSS_EPSILON )
            break;
        if ( Vec3Dot( edge1, edge2 ) < 0.0f )
            return false;
    }

    for ( prevPrev = prev, prev = i, i++; i < ptCount; prevPrev = prev, prev = i, i++ )
    {
        Vec3Sub( pts[prev], pts[prevPrev], edge1 );
        Vec3Sub( pts[i], pts[prev], edge2 );
        Assert( Vec3LengthSq( edge1 ) > 0 );
        Assert( Vec3LengthSq( edge2 ) > 0 );
        Vec3Cross( edge1, edge2, testNormal );
        len = Vec3Normalize( testNormal );
        if ( len >= CONVEX_CROSS_EPSILON )
        {
            if ( Vec3Dot( normal, testNormal ) < CONVEX_DOT_EPSILON )
                return false;
        }
        else
        {
            if ( Vec3Dot( edge1, edge2 ) < 0.0f )
                return false;
        }
    }
    return true;
}

/* CycleIsConvex  0x00406070 */
static bool CycleIsConvex( const brushPoint_t * const *pts, unsigned int ptCount )
{
    vec3_t       xyz[MAX_SIDE_PTS];
    unsigned int i;

    for ( i = 0; i < ptCount; i++ )
        Vec3Copy( pts[i]->xyz, xyz[i] );

    i = 1;
    while ( i < ptCount )
    {
        if ( Vec3DistanceSq( xyz[i], xyz[i - 1] ) < CYCLE_MERGE_DIST )
        {
            memmove( xyz[i], xyz[i + 1], ( ptCount - i - 1 ) * sizeof( vec3_t ) );
            ptCount--;
        }
        else
        {
            i++;
        }
    }

    if ( Vec3DistanceSq( xyz[0], xyz[ptCount - 1] ) < CYCLE_MERGE_DIST )
        ptCount--;

    if ( ptCount < 3 )
        return false;

    return PolyIsConvex( xyz, ptCount );
}

/* IsSecondCycleBetter  0x004064a0 */
static bool IsSecondCycleBetter( bool isConvex0, bool isConvex1,
                                 float perimeter0, float perimeter1,
                                 int ptCount0, int ptCount1 )
{
    if ( isConvex0 )
    {
        if ( !isConvex1 )
            return false;
    }
    else if ( isConvex1 )
    {
        return true;
    }

    if ( perimeter0 < perimeter1 - 1.0f )
        return true;
    if ( perimeter1 < perimeter0 - 1.0f )
        return false;

    return ptCount0 > ptCount1;
}

/* PlaneListContains  0x00406ab0 */
static bool PlaneListContains( const int *planeList, int planeCount, int planeIndex )
{
    int i;

    for ( i = 0; i < planeCount; i++ )
    {
        if ( planeList[i] == planeIndex )
            return true;
    }
    return false;
}

/* FindCycle  0x00406af0 */
typedef struct cycleNode_s
{
    const brushPoint_t *pt;         /* +0x00 */
    int                 plane;      /* +0x04 */
    int                 depth;      /* +0x08 */
    struct cycleNode_s *parent;     /* +0x0c */
} cycleNode_t;

static bool FindCycle( int basePlaneIndex, const brushPoint_t **pts, int ptCount,
                       const brushPoint_t *start, const brushPoint_t *end,
                       int connectingPlane,
                       const brushPoint_t **cycleOut, int *cycleCountOut )
{
    cycleNode_t          queue[MAX_SIDE_PTS];
    int                  queueHead;
    int                  queueTail;
    int                  goalPlane;
    int                  plane;
    int                  i;
    int                  cycleIndex;
    cycleNode_t         *node;
    const brushPoint_t **it;
    const brushPoint_t **last;

    Assert( IsPtFormedByThisPlane( connectingPlane, start ) );
    Assert( IsPtFormedByThisPlane( connectingPlane, end ) );

    queue[0].pt     = start;
    queue[0].plane  = ThirdPlane( start, basePlaneIndex, connectingPlane );
    queue[0].depth  = 1;
    queue[0].parent = NULL;

    queueTail = 0;
    queueHead = 1;
    goalPlane = ThirdPlane( end, basePlaneIndex, connectingPlane );

    while ( queueTail < queueHead )
    {
        last = pts + ptCount;
        for ( it = FindPtUsingPlane( queue[queueTail].plane, pts, last );
              it != last;
              it = FindPtUsingPlane( queue[queueTail].plane, it + 1, last ) )
        {
            plane = ThirdPlane( *it, basePlaneIndex, queue[queueTail].plane );
            if ( plane == connectingPlane )
                continue;

            for ( i = 0; i < queueHead; i++ )
            {
                if ( queue[i].plane == plane )
                    break;
            }
            if ( i < queueHead )
                continue;

            AssertIn( queueHead, ARRAY_COUNT( queue ) );
            queue[queueHead].pt     = *it;
            queue[queueHead].plane  = plane;
            queue[queueHead].depth  = queue[queueTail].depth + 1;
            queue[queueHead].parent = &queue[queueTail];
            queueHead++;

            if ( plane == goalPlane )
            {
                node = &queue[queueHead - 1];
                SanityCheck( node->plane == goalPlane );

                *cycleCountOut = node->depth + 1;
                cycleIndex = node->depth;
                for ( ; node; node = node->parent )
                {
                    cycleOut[cycleIndex] = node->pt;
                    cycleIndex--;
                }
                SanityCheck( cycleIndex == 0 );
                cycleOut[0] = end;
                return true;
            }
        }
        queueTail++;
    }

    *cycleCountOut = 0;
    return false;
}

/* CountPtsUsingPlane  0x00406f10 */
static int CountPtsUsingPlane( int planeIndex, const brushPoint_t **pts, int ptCount )
{
    const brushPoint_t **last;
    int count;

    Assert( pts );

    if ( !ptCount )
        return 0;

    last = pts + ptCount;
    count = 0;
    for ( pts = FindPtUsingPlane( planeIndex, pts, last );
          pts != last;
          pts = FindPtUsingPlane( planeIndex, pts + 1, last ) )
    {
        count++;
    }
    return count;
}

/* GetPtsUsingPlane  0x00406fb0 */
static int GetPtsUsingPlane( int planeIndex, const brushPoint_t **pts, int ptCount,
                             const brushPoint_t **result, int maxResults )
{
    const brushPoint_t **last;
    int occurances;

    Assert( pts );
    Assert( result );

    if ( !ptCount )
        return 0;

    last = pts + ptCount;
    occurances = 0;
    for ( pts = FindPtUsingPlane( planeIndex, pts, last );
          pts != last;
          pts = FindPtUsingPlane( planeIndex, pts + 1, last ) )
    {
        Assert( occurances < maxResults );
        result[occurances] = *pts;
        occurances++;
    }
    return occurances;
}

/* RemoveDegeneratePts  0x00406e40 */
static int RemoveDegeneratePts( const brushPoint_t **pts, int ptCount )
{
    int i;

    i = 0;
    while ( i < ptCount )
    {
        if ( CountPtsUsingPlane( pts[i]->planes[0], pts, ptCount ) < 2
          || CountPtsUsingPlane( pts[i]->planes[1], pts, ptCount ) < 2
          || CountPtsUsingPlane( pts[i]->planes[2], pts, ptCount ) < 2 )
        {
            memmove( &pts[i], &pts[i + 1], ( ptCount - i ) * sizeof( *pts ) - sizeof( *pts ) );
            ptCount--;
            i = 0;
        }
        else
        {
            i++;
        }
    }
    return i;
}

/* RemovePt  0x00407460 */
static int RemovePt( const brushPoint_t **pts, int ptCount, const brushPoint_t *pt )
{
    int i;

    for ( i = 0; i < ptCount && pts[i] != pt; i++ )
        ;

    if ( i != ptCount )
    {
        memmove( &pts[i], &pts[i + 1], ( ptCount - i ) * sizeof( *pts ) - sizeof( *pts ) );
        ptCount--;
        if ( ptCount > 2 )
            ptCount = RemoveDegeneratePts( pts, ptCount );
    }
    return ptCount;
}

/* ChooseWorstOfThreePts  0x004070b0 */
static int ChooseWorstOfThreePts( int basePlaneIndex, int connectingPlane,
                                  const brushPoint_t **pts, int ptCount,
                                  const brushPoint_t **planePts )
{
    const brushPoint_t *cycle0[MAX_SIDE_PTS];
    const brushPoint_t *cycle1[MAX_SIDE_PTS];
    const brushPoint_t *cycle2[MAX_SIDE_PTS];
    bool  isCycle[3];
    bool  isConvex[3];
    int   cycleCount[3];
    float perimeter[3];
    int   best;

    isCycle[0] = FindCycle( basePlaneIndex, pts, ptCount, planePts[0], planePts[1],
                            connectingPlane, cycle0, &cycleCount[0] );
    isCycle[1] = FindCycle( basePlaneIndex, pts, ptCount, planePts[0], planePts[2],
                            connectingPlane, cycle1, &cycleCount[1] );
    isCycle[2] = FindCycle( basePlaneIndex, pts, ptCount, planePts[1], planePts[2],
                            connectingPlane, cycle2, &cycleCount[2] );
    Assert( isCycle[0] && isCycle[1] && isCycle[2] );

    perimeter[0] = PolyPerimeter( cycle0, cycleCount[0] );
    perimeter[1] = PolyPerimeter( cycle1, cycleCount[1] );
    perimeter[2] = PolyPerimeter( cycle2, cycleCount[2] );

    isConvex[0] = CycleIsConvex( cycle0, cycleCount[0] );
    isConvex[1] = CycleIsConvex( cycle1, cycleCount[1] );
    isConvex[2] = CycleIsConvex( cycle2, cycleCount[2] );

    best = IsSecondCycleBetter( isConvex[0], isConvex[1], perimeter[0], perimeter[1],
                                cycleCount[0], cycleCount[1] ) ? 1 : 0;

    if ( IsSecondCycleBetter( isConvex[best], isConvex[2], perimeter[best], perimeter[2],
                              cycleCount[best], cycleCount[2] ) )
    {
        best = 2;
    }

    return 2 - best;
}

/* PartitionPtsByConnectivity  0x004072e0 */
static int PartitionPtsByConnectivity( int basePlaneIndex, int connectingPlane,
                                       const brushPoint_t **pts, int ptCount,
                                       const brushPoint_t **planePts, int planePtCount,
                                       int *partitions )
{
    const brushPoint_t *cycle[MAX_SIDE_PTS];
    int                 cycleCount;
    int                 partitionCount;
    int                 partitionIndex;
    int                 ptIndex;
    int                 i;
    const brushPoint_t *pt;

    partitionCount = 1;
    partitions[0] = 1;

    while ( partitions[partitionCount - 1] < planePtCount )
    {
        ptIndex = partitions[partitionCount - 1];

        for ( partitionIndex = 0; partitionIndex < partitionCount; partitionIndex++ )
        {
            if ( FindCycle( basePlaneIndex, pts, ptCount,
                            planePts[partitions[partitionIndex] - 1], planePts[ptIndex],
                            connectingPlane, cycle, &cycleCount ) )
            {
                break;
            }
        }

        if ( partitionIndex < partitionCount )
        {
            if ( partitionIndex < partitionCount - 1 )
            {
                pt = planePts[ptIndex];
                memmove( &planePts[partitions[partitionIndex] + 1],
                         &planePts[partitions[partitionIndex]],
                         ( ptIndex - partitions[partitionIndex] ) * sizeof( *planePts ) );
                planePts[partitions[partitionIndex]] = pt;
            }
            for ( i = partitionIndex; i < partitionCount; i++ )
                partitions[i]++;
        }

        if ( partitionIndex == partitionCount )
        {
            partitions[partitionCount] = ptIndex + 1;
            partitionCount++;
        }
    }
    return partitionCount;
}

/* RemoveExtraPts  0x00406510 */
static int RemoveExtraPts( int basePlaneIndex, const brushPoint_t **pts, int ptCount )
{
    int                 planeList[MAX_SIDE_PTS];
    const brushPoint_t *planePts[MAX_SIDE_PTS];
    int                 partitions[MAX_SIDE_PTS];
    const brushPoint_t *cycle0[MAX_SIDE_PTS];
    const brushPoint_t *cycle1[MAX_SIDE_PTS];
    int                 cycleCount[2];
    bool                isCycle[2];
    bool                isConvex[2];
    float               perimeter[2];
    int                 planeCount;
    int                 planePtCount;
    int                 partitionCount;
    int                 partitionSize;
    int                 base;
    int                 worst;
    int                 i, j, k;

    ptCount = RemoveDegeneratePts( pts, ptCount );
    if ( ptCount < 3 )
        return ptCount;

    planeCount = 0;
    for ( i = 0; i < ptCount; i++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            if ( basePlaneIndex != pts[i]->planes[j]
              && !PlaneListContains( planeList, planeCount, pts[i]->planes[j] ) )
            {
                AssertIn( planeCount, ARRAY_COUNT( planeList ) );
                planeList[planeCount] = pts[i]->planes[j];
                planeCount++;
            }
        }
    }
    SanityCheck( planeCount > 2 );

    for ( i = 0; i < planeCount; i++ )
    {
        planePtCount = GetPtsUsingPlane( planeList[i], pts, ptCount, planePts, MAX_SIDE_PTS );
        while ( planePtCount > 2 )
        {
            partitionCount = PartitionPtsByConnectivity( basePlaneIndex, planeList[i], pts, ptCount,
                                                         planePts, planePtCount, partitions );
            base = 0;
            k = 0;
            while ( k < partitionCount && k < 2 )
            {
                partitionSize = partitions[k] - base;
                Assert( partitionSize > 0 );

                if ( partitionSize == 1 )
                {
                    ptCount = RemovePt( pts, ptCount, planePts[base] );
                    break;
                }
                if ( partitionSize > 2 )
                {
                    worst = ChooseWorstOfThreePts( basePlaneIndex, planeList[i], pts, ptCount,
                                                   &planePts[base] );
                    ptCount = RemovePt( pts, ptCount, planePts[base + worst] );
                    break;
                }
                base = partitions[k];
                k++;
            }

            if ( k == 2 )
            {
                Assert( partitions[0] == 2 );
                Assert( partitions[1] == 4 );

                isCycle[0] = FindCycle( basePlaneIndex, pts, ptCount, planePts[0], planePts[1],
                                        planeList[i], cycle0, &cycleCount[0] );
                isCycle[1] = FindCycle( basePlaneIndex, pts, ptCount, planePts[2], planePts[3],
                                        planeList[i], cycle1, &cycleCount[1] );
                Assert( isCycle[0] && isCycle[1] );

                perimeter[0] = PolyPerimeter( cycle0, cycleCount[0] );
                perimeter[1] = PolyPerimeter( cycle1, cycleCount[1] );
                isConvex[0]  = CycleIsConvex( cycle0, cycleCount[0] );
                isConvex[1]  = CycleIsConvex( cycle1, cycleCount[1] );

                if ( IsSecondCycleBetter( isConvex[0], isConvex[1], perimeter[0], perimeter[1],
                                          cycleCount[0], cycleCount[1] ) )
                {
                    ptCount = RemovePt( pts, ptCount, planePts[0] );
                    ptCount = RemovePt( pts, ptCount, planePts[1] );
                }
                else
                {
                    ptCount = RemovePt( pts, ptCount, planePts[2] );
                    ptCount = RemovePt( pts, ptCount, planePts[3] );
                }
            }

            if ( ptCount < 3 )
                return ptCount;

            planePtCount = GetPtsUsingPlane( planeList[i], pts, ptCount, planePts, MAX_SIDE_PTS );
        }
    }
    return ptCount;
}

/* CountDistinctPts  0x004074f0 */
static int CountDistinctPts( const brushPoint_t **pts, int ptsCount )
{
    const brushPoint_t *distinct[MAX_SIDE_PTS];
    int distinctCount;
    int i, j;

    Assert( pts );
    Assert( ptsCount > 2 );

    distinctCount = 0;
    for ( i = 0; i < ptsCount; i++ )
    {
        j = 0;
        while ( j < distinctCount && !Vec3CompareEpsilon( pts[i]->xyz, distinct[j]->xyz, 0.01f ) )
            j++;

        if ( j == distinctCount )
        {
            distinct[distinctCount] = pts[i];
            distinctCount++;
        }
    }
    return distinctCount;
}

/* BuildAdjacencyWinding  0x004055e0 */
adjacencyWinding_t *BuildAdjacencyWinding( const vec3_t normal, int basePlaneIndex,
                                           const brushPoint_t *brushPts, int brushPtCount,
                                           adjacencyWinding_t *outWinding, int maxSides )
{
    const brushPoint_t *pts[MAX_SIDE_PTS];
    const brushPoint_t *cycle[2][MAX_SIDE_PTS];
    vec3_t              xyz[MAX_SIDE_PTS];
    vec4_t              plane;
    adjacencyWinding_t *winding;
    int                 cycleCount[2];
    bool                isConvex[2];
    float               perimeter[2];
    int                 ptCount;
    int                 cycleIndex;
    int                 planeIndex;
    int                 idx0, idx1, idx2;
    bool                rv;

    ptCount = GetPtsOnPlane( basePlaneIndex, brushPts, brushPtCount, pts, MAX_SIDE_PTS );
    if ( ptCount < 3 )
        return NULL;

    if ( CountDistinctPts( pts, ptCount ) < 3 )
        return NULL;

    ptCount = RemoveExtraPts( basePlaneIndex, pts, ptCount );
    if ( ptCount < 3 )
        return NULL;

    cycleIndex = 0;
    while ( ptCount )
    {
        ptCount--;
        cycle[cycleIndex][0] = pts[ptCount];
        cycleCount[cycleIndex] = 1;
        planeIndex = SecondPlane( cycle[cycleIndex][0], basePlaneIndex );

        while ( ptCount )
        {
            cycle[cycleIndex][cycleCount[cycleIndex]] =
                ExtractPtWithPlane( planeIndex, pts, &pts[ptCount] );
            if ( !cycle[cycleIndex][cycleCount[cycleIndex]] )
                break;
            planeIndex = ThirdPlane( cycle[cycleIndex][cycleCount[cycleIndex]],
                                     basePlaneIndex, planeIndex );
            ptCount--;
            cycleCount[cycleIndex]++;
        }

        SanityCheck( IsPtFormedByThisPlane( planeIndex, cycle[cycleIndex][0] ) );

        if ( cycleIndex < 1 )
        {
            cycleIndex = 1;
        }
        else
        {
            perimeter[0] = PolyPerimeter( cycle[0], cycleCount[0] );
            perimeter[1] = PolyPerimeter( cycle[1], cycleCount[1] );
            isConvex[0]  = CycleIsConvex( cycle[0], cycleCount[0] );
            isConvex[1]  = CycleIsConvex( cycle[1], cycleCount[1] );
            if ( IsSecondCycleBetter( isConvex[0], isConvex[1], perimeter[0], perimeter[1],
                                      cycleCount[0], cycleCount[1] ) )
            {
                memcpy( cycle[0], cycle[1], cycleCount[1] * sizeof( cycle[1][0] ) );
                cycleCount[0] = cycleCount[1];
            }
        }
    }
    SanityCheck( cycleCount[0] > 2 );

    winding = NULL;
    if ( outWinding )
    {
        winding = outWinding;
        if ( maxSides < cycleCount[0] )
        {
            Com_PrintError( CON_CHANNEL_ERROR, "Brush face has too many edges" );
            return NULL;
        }
    }
    else
    {
        winding = AllocAdjacencyWinding( cycleCount[0] );
    }
    Assert( winding );

    rv = SharedPlane( cycle[0][0], cycle[0][cycleCount[0] - 1], basePlaneIndex, &winding->sides[0] );
    SanityCheck( rv );

    Vec3Copy( cycle[0][0]->xyz, xyz[0] );
    for ( winding->numsides = 1; winding->numsides < cycleCount[0]; winding->numsides++ )
    {
        winding->sides[winding->numsides] = ThirdPlane( cycle[0][winding->numsides - 1],
                                                        basePlaneIndex,
                                                        winding->sides[winding->numsides - 1] );
        Vec3Copy( cycle[0][winding->numsides]->xyz, xyz[winding->numsides] );
    }
    SanityCheck( winding->sides[0] == ThirdPlane( cycle[0][cycleCount[0] - 1], basePlaneIndex,
                                                  winding->sides[winding->numsides - 1] ) );

    if ( LargestTriangleArea( xyz, winding->numsides, normal, &idx0, &idx1, &idx2 ) < MIN_TRIANGLE_AREA )
    {
        if ( !outWinding )
            FreeAdjacencyWinding( winding );
        return NULL;
    }

    PlaneFromPoints( plane, xyz[idx0], xyz[idx1], xyz[idx2] );
    if ( Vec3Dot( plane, normal ) < 0.0f )
        ReverseAdjacencyWinding( winding );

    return winding;
}

/* StringFromOffset  0x004053a0 / 0x004053c0 */

const char *StringFromOffset( const void *base, int offset )
{
    return (const char *)base + offset;
}

const char *StringFromOffset( const void *base )
{
    return StringFromOffset( base, *(const int *)base );
}

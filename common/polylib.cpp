/* Original: ..\common\polylib.cpp */

#include "../src/universal/q_shared.h"
#include "../src/universal/assertive.h"
#include "../src/universal/com_math.h"
#include "../src/universal/com_vector.h"
#include "polylib.h"
#include "cmdlib.h"


extern int  ConvexHull2D( vec2_t *points, int *work, unsigned int pointCount,
                          int *outIndices );                             /* 0x00467620 */


int c_activeWindings;                                                     /* 0x12ece908 */

/* AllocWinding  0x0042b6d0 */
winding_t *AllocWinding( unsigned int ptCount )
{
    winding_t *w;
    int        size;

    c_activeWindings++;

    size = ptCount * sizeof( vec3_t ) + sizeof( int );
    w = ( winding_t * )malloc( size );

    if ( !w )
        Com_Error( "out of memory: winding_t\n" );

    w->ptCount = 0;
    return w;
}

/* FreeWinding  0x0042b730 */
void FreeWinding( winding_t *w )
{
    c_activeWindings--;
    free( w );
}

/* AllocWindingList  0x0042b750 */
windingList_t *AllocWindingList( void )
{
    windingList_t *list;

    list = ( windingList_t * )malloc( sizeof( windingList_t ) );

    if ( !list )
        Com_Error( "Out of memory" );

    list->w    = NULL;
    list->next = NULL;

    return list;
}

/* FreeWindingList  0x0042b790 */
void FreeWindingList( windingList_t *list )
{
    free( list );
}

/* RemoveColinearPoints  0x0042b7b0 */
void RemoveColinearPoints( winding_t *w )
{
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int nump;
    vec3_t       v1;
    vec3_t       v2;
    vec3_t       pts[MAX_POINTS_ON_WINDING];

    nump = 0;

    for ( i = 0; i < w->ptCount; i++ )
    {
        j = ( i + 1 ) % w->ptCount;
        k = ( i + w->ptCount - 1 ) % w->ptCount;

        Vec3Sub( w->pts[j], w->pts[i], v1 );
        Vec3Sub( w->pts[i], w->pts[k], v2 );

        Vec3Normalize( v1 );
        Vec3Normalize( v2 );

        if ( Vec3Dot( v1, v2 ) < 0.999f )
        {
            Vec3Copy( w->pts[i], pts[nump] );
            nump++;
        }
    }

    if ( nump != w->ptCount )
    {
        w->ptCount = nump;
        memcpy( w->pts, pts, nump * sizeof( vec3_t ) );
    }
}

/* PlaneFromWindingTriangle  0x0042b920 */
int PlaneFromWindingTriangle( const winding_t *w, vec4_t plane )
{
    return PlaneFromPoints( plane, w->pts[0], w->pts[1], w->pts[2] );
}

/* WindingPlane  0x0042b950 */
float WindingPlane( const winding_t *w, vec4_t plane )
{
    unsigned int i;
    vec3_t       d1;
    vec3_t       d2;
    vec3_t       cross;
    float        area;

    Vec3Clear( plane );

    for ( i = 2; i < w->ptCount; i++ )
    {
        Vec3Sub( w->pts[i - 1], w->pts[0], d1 );
        Vec3Sub( w->pts[i], w->pts[0], d2 );
        Vec3Cross( d2, d1, cross );
        Vec3Add( cross, plane, plane );
    }

    area = Vec3Normalize( plane ) * 0.5;

    plane[3] = Vec3Dot( w->pts[0], plane );

    return area;
}

/* PlaneFromWinding  0x0042ba30 */
qboolean PlaneFromWinding( const winding_t *w, vec4_t plane )
{
    return WindingPlane( w, plane ) != 0.0;
}

/* WindingArea  0x0042ba70 */
float WindingArea( const winding_t *w )
{
    unsigned int i;
    vec3_t       d1;
    vec3_t       d2;
    vec3_t       cross;
    float        total;

    total = 0.0f;

    for ( i = 2; i < w->ptCount; i++ )
    {
        Vec3Sub( w->pts[i - 1], w->pts[0], d1 );
        Vec3Sub( w->pts[i], w->pts[0], d2 );
        Vec3Cross( d1, d2, cross );
        total = Vec3Length( cross ) * 0.5 + total;
    }

    return total;
}

/* WindingAreaWithKnownNormal  0x0042bb20 */
float WindingAreaWithKnownNormal( const winding_t *w, const vec3_t normal )
{
    unsigned int i;
    vec3_t       d1;
    vec3_t       d2;
    vec3_t       cross;
    float        total;

    total = 0.0f;

    for ( i = 2; i < w->ptCount; i++ )
    {
        Vec3Sub( w->pts[i - 1], w->pts[0], d1 );
        Vec3Sub( w->pts[i], w->pts[0], d2 );
        Vec3Cross( d2, d1, cross );
        total = Vec3Dot( cross, normal ) + total;
    }

    return total * 0.5;
}

/* WindingCornerIsConcave  0x0042bbd0 */
bool WindingCornerIsConcave( const winding_t *w, unsigned int i0, unsigned int i1,
                             unsigned int i2, const vec3_t normal )
{
    vec3_t v1;
    vec3_t v2;
    vec3_t cross;
    float  d;

    Vec3Sub( w->pts[i1], w->pts[i0], v1 );
    Vec3Sub( w->pts[i2], w->pts[i0], v2 );
    Vec3Cross( v2, v1, cross );

    d = Vec3Dot( cross, normal );

    return d < -0.001f;
}

/* WindingIsConvex  0x0042bc70 */
bool WindingIsConvex( const winding_t *w, const vec3_t normal )
{
    unsigned int i;

    if ( WindingCornerIsConcave( w, w->ptCount - 2, w->ptCount - 1, 0, normal ) )
        return false;

    if ( WindingCornerIsConcave( w, w->ptCount - 1, 0, 1, normal ) )
        return false;

    for ( i = 2; i < w->ptCount; i++ )
    {
        if ( WindingCornerIsConcave( w, i - 2, i - 1, i, normal ) )
            return false;
    }

    return true;
}

/* WindingBounds  0x0042bd20 */
void WindingBounds( const winding_t *w, vec3_t mins, vec3_t maxs )
{
    unsigned int i;

    Assert( w );
    Assert( w->ptCount > 0 );

    Vec3Copy( w->pts[0], mins );
    Vec3Copy( w->pts[0], maxs );

    for ( i = 1; i < w->ptCount; i++ )
        AddPointToBounds( w->pts[i], mins, maxs );
}

/* WindingCenter  0x0042bde0 */
void WindingCenter( const winding_t *w, vec3_t center )
{
    unsigned int i;

    Vec3Clear( center );

    for ( i = 0; i < w->ptCount; i++ )
        Vec3Add( w->pts[i], center, center );

    Vec3Scale( center, 1.0f / w->ptCount, center );
}

/* BaseWindingForPlane  0x0042be70 */
winding_t *BaseWindingForPlane( const vec3_t normal, float dist )
{
    int        i;
    int        x;
    float      max;
    float      v;
    vec3_t     org;
    vec3_t     vright;
    vec3_t     vup;
    winding_t *w;

    max = -262144.0f;
    x = -1;

    for ( i = 0; i < 3; i++ )
    {
        v = fabs( normal[i] );

        if ( v > max )
        {
            x = i;
            max = v;
        }
    }

    if ( x == -1 )
        Com_Error( "BaseWindingForPlane: no axis found" );

    Vec3Copy( vec3_origin, vup );

    switch ( x )
    {
    case 0:
    case 1:
        vup[2] = 1.0f;
        break;

    case 2:
        vup[0] = 1.0f;
        break;
    }

    v = Vec3Dot( vup, normal );
    Vec3Mad( vup, -v, normal, vup );
    Vec3Normalize( vup );

    Vec3Scale( normal, dist, org );

    Vec3Cross( vup, normal, vright );

    Vec3Scale( vup, MAX_WORLD_COORD, vup );
    Vec3Scale( vright, MAX_WORLD_COORD, vright );

    w = AllocWinding( 4 );

    Vec3Sub( org, vright, w->pts[0] );
    Vec3Add( w->pts[0], vup, w->pts[0] );

    Vec3Add( org, vright, w->pts[1] );
    Vec3Add( w->pts[1], vup, w->pts[1] );

    Vec3Add( org, vright, w->pts[2] );
    Vec3Sub( w->pts[2], vup, w->pts[2] );

    Vec3Sub( org, vright, w->pts[3] );
    Vec3Sub( w->pts[3], vup, w->pts[3] );

    w->ptCount = 4;

    return w;
}

/* WindingForConvexHull  0x0042c0a0 */
winding_t *WindingForConvexHull( const vec3_t normal, const vec3_t *xyz, unsigned int xyzCount )
{
    int          hull[MAX_HULL_POINTS + 1];
    int          axisV;
    int          axisU;
    unsigned int hullCount;
    int          work[MAX_HULL_POINTS];
    vec2_t       proj[MAX_HULL_POINTS];
    unsigned int i;
    int          i0;
    int          i1;
    int          i2;
    vec4_t       plane;
    winding_t   *w;
    winding_t   *reversed;

    Assertx( xyzCount <= ARRAY_COUNT( proj ), "(xyzCount) = %i", xyzCount );

    GetProjectionAxes( normal, &axisU, &axisV );

    for ( i = 0; i < xyzCount; i++ )
        Vec2Set( proj[i], xyz[i][axisU], xyz[i][axisV] );

    hullCount = ConvexHull2D( proj, work, xyzCount, hull );

    AssertCmp( hullCount, <=, xyzCount );

    w = AllocWinding( hullCount );
    w->ptCount = hullCount;

    for ( i = 0; i < hullCount; i++ )
        Vec3Copy( xyz[hull[i]], w->pts[i] );

    if ( WindingLargestTriangleArea( w->pts, w->ptCount, normal, &i0, &i1, &i2 ) < 0.001f )
    {
        FreeWinding( w );
        return NULL;
    }

    PlaneFromPoints( plane, w->pts[i0], w->pts[i1], w->pts[i2] );

    if ( Vec3Dot( plane, normal ) < 0.0 )
    {
        reversed = ReverseWinding( w );
        FreeWinding( w );
        w = reversed;
    }

    return w;
}

/* WindingForConvexHullList  0x0042c2d0 */
winding_t *WindingForConvexHullList( const vec3_t normal, const windingList_t *list )
{
    const windingList_t *node;
    unsigned int         xyzCount;
    vec3_t               xyz[MAX_HULL_POINTS];
    winding_t           *w;

    Assert( list );

    if ( !list->next )
        return CopyWinding( list->w );

    xyzCount = 0;

    for ( node = list; node != NULL; node = node->next )
    {
        if ( xyzCount + node->w->ptCount > MAX_HULL_POINTS )
        {
            Assert( xyzCount );

            w = WindingForConvexHull( normal, xyz, xyzCount );

            if ( w->ptCount + node->w->ptCount > MAX_HULL_POINTS )
                Com_Error( "convex hull has too many vertices" );

            memcpy( xyz, w->pts, w->ptCount * sizeof( vec3_t ) );
            xyzCount = w->ptCount;
            FreeWinding( w );
        }

        memcpy( xyz[xyzCount], node->w->pts, node->w->ptCount * sizeof( vec3_t ) );
        xyzCount = xyzCount + node->w->ptCount;
    }

    return WindingForConvexHull( normal, xyz, xyzCount );
}

/* BaseWindingForPlaneInBounds  0x0042c470 */
winding_t *BaseWindingForPlaneInBounds( const vec4_t plane, const vec3_t mins, const vec3_t maxs )
{
    vec4_t boxPlanes[6];
    vec3_t xyz[12];
    int    xyzCount;

    Vec4Set( boxPlanes[0], 1.0f, 0.0f, 0.0f, maxs[0] );
    Vec4Set( boxPlanes[1], 0.0f, 1.0f, 0.0f, maxs[1] );
    Vec4Set( boxPlanes[2], 0.0f, 0.0f, 1.0f, maxs[2] );
    Vec4Set( boxPlanes[3], -1.0f, 0.0f, 0.0f, -mins[0] );
    Vec4Set( boxPlanes[4], 0.0f, -1.0f, 0.0f, -mins[1] );
    Vec4Set( boxPlanes[5], 0.0f, 0.0f, -1.0f, -mins[2] );

    xyzCount = 0;
    xyzCount = PlaneBoxEdgeIntersections( plane, mins, maxs, boxPlanes, 0, 1, &xyz[xyzCount] ) + xyzCount;
    xyzCount = PlaneBoxEdgeIntersections( plane, mins, maxs, boxPlanes, 1, 2, &xyz[xyzCount] ) + xyzCount;
    xyzCount = PlaneBoxEdgeIntersections( plane, mins, maxs, boxPlanes, 2, 0, &xyz[xyzCount] ) + xyzCount;

    if ( xyzCount < 3 )
        return NULL;

    return WindingForConvexHull( plane, xyz, xyzCount );
}

/* PlaneBoxEdgeIntersections  0x0042c660 */
int PlaneBoxEdgeIntersections( const vec4_t plane, const vec3_t mins, const vec3_t maxs,
                               const vec4_t *boxPlanes, int axis0, int axis1, vec3_t *out )
{
    const vec4_t *planes[3];
    int           count;

    count = 0;

    planes[0] = ( const vec4_t * )plane;
    planes[1] = &boxPlanes[axis0];
    planes[2] = &boxPlanes[axis1];

    if ( PlaneIntersection3( planes, out[count] ) && PointInBounds( out[count], mins, maxs ) )
        count++;

    planes[2] = &boxPlanes[axis1 + 3];

    if ( PlaneIntersection3( planes, out[count] ) && PointInBounds( out[count], mins, maxs ) )
        count++;

    planes[1] = &boxPlanes[axis0 + 3];

    if ( PlaneIntersection3( planes, out[count] ) && PointInBounds( out[count], mins, maxs ) )
        count++;

    planes[2] = &boxPlanes[axis1];

    if ( PlaneIntersection3( planes, out[count] ) && PointInBounds( out[count], mins, maxs ) )
        count++;

    return count;
}

/* WindingForAxialRect  0x0042c7d0 */
winding_t *WindingForAxialRect( const vec3_t mins, const vec3_t maxs,
                                int axis0, int axis1, int axis2, float dist )
{
    winding_t *w;

    w = AllocWinding( 4 );
    w->ptCount = 4;

    w->pts[0][axis0] = mins[axis0];
    w->pts[0][axis1] = mins[axis1];
    w->pts[0][axis2] = dist;

    w->pts[1][axis0] = maxs[axis0];
    w->pts[1][axis1] = mins[axis1];
    w->pts[1][axis2] = dist;

    w->pts[2][axis0] = maxs[axis0];
    w->pts[2][axis1] = maxs[axis1];
    w->pts[2][axis2] = dist;

    w->pts[3][axis0] = mins[axis0];
    w->pts[3][axis1] = maxs[axis1];
    w->pts[3][axis2] = dist;

    return w;
}

/* CopyWinding  0x0042c8c0 */
winding_t *CopyWinding( const winding_t *w )
{
    winding_t *c;
    int        size;

    c = AllocWinding( w->ptCount );

    size = w->ptCount * sizeof( vec3_t ) + sizeof( int );
    memcpy( c, w, size );

    return c;
}

/* ReverseWinding  0x0042c900 */
winding_t *ReverseWinding( const winding_t *w )
{
    winding_t   *c;
    unsigned int i;

    c = AllocWinding( w->ptCount );

    for ( i = 0; i < w->ptCount; i++ )
        Vec3Copy( w->pts[w->ptCount - 1 - i], c->pts[i] );

    c->ptCount = w->ptCount;

    return c;
}

/* ClipWindingEpsilon_Internal  0x0042c980 */
int ClipWindingEpsilon_Internal( const winding_t *in, const vec3_t normal, float dist,
                                 float epsilon, winding_t **front, winding_t **back )
{
    double       dists[MAX_POINTS_ON_WINDING + WINDING_CLIP_SLACK];
    int          axialPlaneAxis;
    unsigned int i;
    const float *p1;
    winding_t   *b;
    int          exactAxialPlane;
    int          counts[3];
    const float *p2;
    int          sides[MAX_POINTS_ON_WINDING + WINDING_CLIP_SLACK];
    winding_t   *f;
    float        axialPlaneDist;
    unsigned int j;
    double       dot;
    vec3_t       mid;
    unsigned int maxPts;

    Assert( in );
    Assert( normal );
    Assert( Vec3LengthSq( normal ) > 0 );
    Assert( front );
    Assert( back );

    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 0;

    for ( i = 0; i < in->ptCount; i++ )
    {
        dot = Vec3Dot( in->pts[i], normal );
        dot -= dist;
        dists[i] = dot;

        if ( dot > epsilon )
            sides[i] = SIDE_FRONT;
        else if ( dot < -epsilon )
            sides[i] = SIDE_BACK;
        else
            sides[i] = SIDE_ON;

        counts[sides[i]]++;
    }

    sides[i] = sides[0];
    dists[i] = dists[0];

    if ( counts[SIDE_FRONT] == 0 )
        return SIDE_ON - ( counts[SIDE_BACK] != 0 );

    if ( counts[SIDE_BACK] == 0 )
        return SIDE_FRONT;

    exactAxialPlane = 1;
    axialPlaneAxis = -1;
    axialPlaneDist = 0.0f;

    for ( j = 0; j < 3; j++ )
    {
        if ( normal[j] == 1.0 )
        {
            axialPlaneAxis = j;
            axialPlaneDist = dist;
        }
        else if ( normal[j] == -1.0 )
        {
            axialPlaneAxis = j;
            axialPlaneDist = -dist;
        }
        else if ( normal[j] != 0.0 )
        {
            exactAxialPlane = 0;
            break;
        }
    }

    Assertx( !exactAxialPlane || ( axialPlaneAxis >= 0 && axialPlaneAxis < 3 ),
             "(axialPlaneAxis) = %i", axialPlaneAxis );

    maxPts = in->ptCount + WINDING_CLIP_SLACK;

    f = AllocWinding( maxPts );
    b = AllocWinding( maxPts );

    for ( i = 0; i < in->ptCount; i++ )
    {
        p1 = in->pts[i];

        if ( sides[i] == SIDE_ON )
        {
            Vec3Copy( p1, f->pts[f->ptCount] );
            f->ptCount++;
            Vec3Copy( p1, b->pts[b->ptCount] );
            b->ptCount++;
            continue;
        }

        if ( sides[i] == SIDE_FRONT )
        {
            Vec3Copy( p1, f->pts[f->ptCount] );
            f->ptCount++;
        }

        if ( sides[i] == SIDE_BACK )
        {
            Vec3Copy( p1, b->pts[b->ptCount] );
            b->ptCount++;
        }

        if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
            continue;

        p2 = in->pts[( i + 1 ) % in->ptCount];

        for ( j = 0; j < 3; j++ )
            mid[j] = ( p2[j] * dists[i] - p1[j] * dists[i + 1] ) / ( dists[i] - dists[i + 1] );

        if ( exactAxialPlane )
            mid[axialPlaneAxis] = axialPlaneDist;

        Vec3Copy( mid, f->pts[f->ptCount] );
        f->ptCount++;
        Vec3Copy( mid, b->pts[b->ptCount] );
        b->ptCount++;
    }

    AssertCmp( f->ptCount, <=, maxPts );
    AssertCmp( b->ptCount, <=, maxPts );
    Assertx( f->ptCount <= MAX_POINTS_ON_WINDING, "(f->ptCount) = %i", f->ptCount );
    Assertx( b->ptCount <= MAX_POINTS_ON_WINDING, "(b->ptCount) = %i", b->ptCount );

    *front = f;
    *back = b;

    return SIDE_CROSS;
}

/* ClipWindingEpsilon  0x0042d050 */
int ClipWindingEpsilon( const winding_t *in, const vec3_t normal, float dist, float epsilon,
                        winding_t **front, winding_t **back, qboolean keepOnPlane )
{
    int side;

    side = ClipWindingEpsilon_Internal( in, normal, dist, epsilon, front, back );

    if ( side == SIDE_BACK )
    {
        *front = NULL;
        *back = CopyWinding( in );
    }
    else if ( side != SIDE_CROSS )
    {
        *front = CopyWinding( in );

        if ( keepOnPlane && side == SIDE_ON )
            *back = CopyWinding( in );
        else
            *back = NULL;
    }

    return side;
}

/* ClipWindingByPlane  0x0042d0f0 */
void ClipWindingByPlane( winding_t **inout, const vec3_t normal, float dist, float epsilon )
{
    unsigned int j;
    float        dists[MAX_POINTS_ON_WINDING + WINDING_CLIP_SLACK];
    const float *p1;
    winding_t   *in;
    int          counts[3];
    const float *p2;
    int          sides[MAX_POINTS_ON_WINDING + WINDING_CLIP_SLACK];
    winding_t   *f;
    unsigned int i;
    float        dot;
    vec3_t       mid;
    unsigned int maxPts;

    in = *inout;

    counts[0] = counts[1] = counts[2] = 0;

    for ( i = 0; i < in->ptCount; i++ )
    {
        dot = Vec3Dot( in->pts[i], normal );
        dot -= dist;
        dists[i] = dot;

        if ( dot > epsilon )
            sides[i] = SIDE_FRONT;
        else if ( dot < -epsilon )
            sides[i] = SIDE_BACK;
        else
            sides[i] = SIDE_ON;

        counts[sides[i]]++;
    }

    sides[i] = sides[0];
    dists[i] = dists[0];

    if ( counts[SIDE_FRONT] == 0 )
    {
        FreeWinding( in );
        *inout = NULL;
        return;
    }

    if ( counts[SIDE_BACK] == 0 )
        return;

    maxPts = in->ptCount + WINDING_CLIP_SLACK;

    f = AllocWinding( maxPts );

    for ( i = 0; i < in->ptCount; i++ )
    {
        p1 = in->pts[i];

        if ( sides[i] == SIDE_ON )
        {
            Vec3Copy( p1, f->pts[f->ptCount] );
            f->ptCount++;
            continue;
        }

        if ( sides[i] == SIDE_FRONT )
        {
            Vec3Copy( p1, f->pts[f->ptCount] );
            f->ptCount++;
        }

        if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
            continue;

        p2 = in->pts[( i + 1 ) % in->ptCount];

        for ( j = 0; j < 3; j++ )
        {
            if ( normal[j] == 1.0 )
                mid[j] = dist;
            else if ( normal[j] == -1.0 )
                mid[j] = -dist;
            else
                mid[j] = ( dists[i] * p2[j] - dists[i + 1] * p1[j] ) / ( dists[i] - dists[i + 1] );
        }

        Vec3Copy( mid, f->pts[f->ptCount] );
        f->ptCount++;
    }

    AssertCmp( f->ptCount, <=, maxPts );

    if ( f->ptCount > MAX_POINTS_ON_WINDING )
        Com_Error( "ClipWinding: MAX_POINTS_ON_WINDING" );

    FreeWinding( in );
    *inout = f;
}

/* ClipWindingByBounds  0x0042d4e0 */
void ClipWindingByBounds( winding_t **inout, const vec3_t mins, const vec3_t maxs, float epsilon )
{
    vec3_t normal;

    Vec3Set( normal, 1.0f, 0.0f, 0.0f );
    ClipWindingByPlane( inout, normal, mins[0], epsilon );

    if ( !*inout )
        return;

    Vec3Set( normal, -1.0f, 0.0f, 0.0f );
    ClipWindingByPlane( inout, normal, -maxs[0], epsilon );

    if ( !*inout )
        return;

    Vec3Set( normal, 0.0f, 1.0f, 0.0f );
    ClipWindingByPlane( inout, normal, mins[1], epsilon );

    if ( !*inout )
        return;

    Vec3Set( normal, 0.0f, -1.0f, 0.0f );
    ClipWindingByPlane( inout, normal, -maxs[1], epsilon );

    if ( !*inout )
        return;

    Vec3Set( normal, 0.0f, 0.0f, 1.0f );
    ClipWindingByPlane( inout, normal, mins[2], epsilon );

    if ( !*inout )
        return;

    Vec3Set( normal, 0.0f, 0.0f, -1.0f );
    ClipWindingByPlane( inout, normal, -maxs[2], epsilon );
}

/* ChopWinding  0x0042d6b0 */
winding_t *ChopWinding( winding_t *in, const vec3_t normal, float dist )
{
    winding_t *f;
    winding_t *b;

    ClipWindingEpsilon( in, normal, dist, 0.1f, &f, &b, qfalse );
    FreeWinding( in );

    if ( b )
        FreeWinding( b );

    return f;
}

/* WindingMaxPlaneDist  0x0042d710 */
float WindingMaxPlaneDist( const winding_t *w, const vec3_t normal, float dist )
{
    float        maxDist;
    unsigned int i;
    float        d;

    maxDist = 0.0f;

    for ( i = 0; i < w->ptCount; i++ )
    {
        d = Vec3Dot( w->pts[i], normal ) - dist;

        if ( d < 0.0 )
            d = -d;

        if ( maxDist < d )
            maxDist = d;
    }

    return maxDist;
}

/* WindingPlaneDistExtent  0x0042d790 */
void WindingPlaneDistExtent( const winding_t *w, const vec3_t normal, float dist,
                             float *outMinDist, float *outMaxDist )
{
    unsigned int i;
    float        d;

    *outMinDist = FLT_MAX;
    *outMaxDist = -FLT_MAX;

    for ( i = 0; i < w->ptCount; i++ )
    {
        d = Vec3Dot( w->pts[i], normal ) - dist;

        if ( d < *outMinDist )
            *outMinDist = d;

        if ( *outMaxDist < d )
            *outMaxDist = d;
    }
}

/* WindingPlaneSide  0x0042d820 */
int WindingPlaneSide( const winding_t *w, const vec3_t normal, float dist )
{
    qboolean     front;
    qboolean     back;
    unsigned int i;
    float        d;

    front = qfalse;
    back = qfalse;

    for ( i = 0; i < w->ptCount; i++ )
    {
        d = Vec3Dot( w->pts[i], normal ) - dist;

        if ( d < -0.1 )
        {
            if ( front )
                return SIDE_CROSS;

            back = qtrue;
            continue;
        }

        if ( d > 0.1 )
        {
            if ( back )
                return SIDE_CROSS;

            front = qtrue;
            continue;
        }
    }

    if ( back )
        return SIDE_BACK;

    if ( front )
        return SIDE_FRONT;

    return SIDE_ON;
}

/* WindingIsOnPlane  0x0042d8f0 */
qboolean WindingIsOnPlane( const winding_t *w, const vec3_t normal, float dist, float epsilon )
{
    float        epsilonSq;
    unsigned int i;
    float        d;

    epsilonSq = epsilon * epsilon;

    for ( i = 0; i < w->ptCount; i++ )
    {
        d = Vec3Dot( normal, w->pts[i] ) - dist;

        if ( epsilonSq < d * d )
            return qfalse;
    }

    return qtrue;
}

/* SnapWindingToPlane  0x0042d960 */
void SnapWindingToPlane( winding_t *w, const vec3_t normal, float dist )
{
    unsigned int i;
    float        d;

    Assert( w );
    Assert( normal );

    for ( i = 0; i < w->ptCount; i++ )
    {
        d = Vec3Dot( normal, w->pts[i] ) - dist;
        Vec3Mad( w->pts[i], -d, normal, w->pts[i] );

        w->pts[i][0] = SnapToIntegralPowerOf2( w->pts[i][0], 8, 12 );
        w->pts[i][1] = SnapToIntegralPowerOf2( w->pts[i][1], 8, 12 );
        w->pts[i][2] = SnapToIntegralPowerOf2( w->pts[i][2], 8, 12 );
    }
}

/* AddWindingToConvexHull  0x0042dab0 */
void AddWindingToConvexHull( const winding_t *w, winding_t **hull, const vec3_t normal )
{
    int          outside[128];
    vec3_t       edgePlanes[128];
    vec3_t       newPts[128];
    vec3_t       hullPts[128];
    unsigned int j;
    unsigned int k;
    const float *p2;
    vec3_t       dir;
    float        d;
    qboolean     anyOutside;
    unsigned int hullCount;
    unsigned int i;
    const float *p;
    unsigned int numNew;

    if ( !*hull )
    {
        *hull = CopyWinding( w );
        return;
    }

    hullCount = ( *hull )->ptCount;
    memcpy( hullPts, ( *hull )->pts, hullCount * sizeof( vec3_t ) );

    for ( i = 0; i < w->ptCount; i++ )
    {
        p = w->pts[i];

        for ( j = 0; j < hullCount; j++ )
        {
            k = ( j + 1 ) % hullCount;
            Vec3Sub( hullPts[k], hullPts[j], dir );
            Vec3Normalize( dir );
            Vec3Cross( normal, dir, edgePlanes[j] );
        }

        anyOutside = qfalse;

        for ( j = 0; j < hullCount; j++ )
        {
            Vec3Sub( p, hullPts[j], dir );
            d = Vec3Dot( dir, edgePlanes[j] );

            if ( d >= 0.1 )
                anyOutside = qtrue;

            if ( d >= -0.1 )
                outside[j] = 1;
            else
                outside[j] = 0;
        }

        if ( !anyOutside )
            continue;

        for ( j = 0; j < hullCount; j++ )
        {
            if ( !outside[j % hullCount] && outside[( j + 1 ) % hullCount] )
                break;
        }

        if ( j == hullCount )
            continue;

        Vec3Copy( p, newPts[0] );
        numNew = 1;

        j = ( j + 1 ) % hullCount;

        for ( k = 0; k < hullCount; k++ )
        {
            if ( !outside[( j + k ) % hullCount] || !outside[( j + k + 1 ) % hullCount] )
            {
                p2 = hullPts[( j + k + 1 ) % hullCount];
                Vec3Copy( p2, newPts[numNew] );
                numNew++;
            }
        }

        hullCount = numNew;
        memcpy( hullPts, newPts, hullCount * sizeof( vec3_t ) );
    }

    FreeWinding( *hull );
    *hull = AllocWinding( hullCount );
    ( *hull )->ptCount = hullCount;
    memcpy( ( *hull )->pts, hullPts, hullCount * sizeof( vec3_t ) );
}

/* CheckWindingInPlane  0x0042dec0 */
void CheckWindingInPlane( const winding_t *w, const vec3_t normal, float dist )
{
    unsigned int i;
    float        error;

    if ( !w )
        return;

    for ( i = 0; i < w->ptCount; i++ )
    {
        error = Vec3Dot( w->pts[i], normal ) - dist;
        Assertx( I_fabs( error ) < I_fmax( 0.1f, 0.00001f * I_fabs( dist ) ),
                 "(error) = %g", error );
    }
}

/* PointOnLine  0x0042df90 */
qboolean PointOnLine( const vec3_t origin, const vec3_t point, const vec3_t end, float epsilon )
{
    vec3_t toPoint;
    vec3_t dir;
    float  d;
    vec3_t proj;
    vec3_t delta;

    Vec3Sub( point, origin, toPoint );
    Vec3Sub( end, origin, dir );

    d = Vec3Normalize( dir );

    if ( d == 0.0 )
        return qfalse;

    d = Vec3Dot( toPoint, dir );
    Vec3Scale( dir, d, proj );
    Vec3Sub( toPoint, proj, delta );

    d = Vec3Length( delta );

    if ( d < epsilon )
        return qtrue;

    return qfalse;
}

/* PointOnLine2D  0x0042e050 */
qboolean PointOnLine2D( const vec2_t origin, const vec2_t point, const vec2_t end, float epsilon )
{
    vec2_t toPoint;
    vec2_t dir;
    float  d;
    vec2_t proj;
    vec2_t delta;

    Vec2Sub( point, origin, toPoint );
    Vec2Sub( end, origin, dir );

    d = Vec2Normalize( dir );

    if ( d == 0.0 )
        return qfalse;

    d = Vec2Dot( toPoint, dir );
    Vec2Scale( dir, d, proj );
    Vec2Sub( toPoint, proj, delta );

    d = Vec2Length( delta );

    if ( d < epsilon )
        return qtrue;

    return qfalse;
}

/* WindingMinWidth  0x0042e110 */
float WindingMinWidth( const winding_t *w, const vec3_t normal )
{
    float        minDist;
    unsigned int i;
    unsigned int prev;
    unsigned int j;
    vec3_t       edge;
    vec3_t       edgeNormal;
    float        edgeLen;
    float        base;
    float        maxDist;
    float        dist;

    Assert( w );
    Assert( w->ptCount >= 3 );

    minDist = FLT_MAX;

    prev = w->ptCount - 1;

    for ( i = 0; i < w->ptCount; prev = i, i++ )
    {
        Vec3Sub( w->pts[i], w->pts[prev], edge );
        Vec3Cross( edge, normal, edgeNormal );
        edgeLen = Vec3Normalize( edgeNormal );

        base = Vec3Dot( edgeNormal, w->pts[prev] );

        j = ( i + 1 ) % w->ptCount;
        maxDist = -FLT_MAX;

        do
        {
            dist = Vec3Dot( w->pts[j], edgeNormal ) - base;

            Assertx( dist >= -0.1f || edgeLen < 1.0f, "(dist) = %g", dist );

            if ( dist < maxDist )
                break;

            maxDist = dist;
            j = ( j + 1 ) % w->ptCount;
        }
        while ( j != prev );

        if ( maxDist < minDist )
            minDist = maxDist;
    }

    return minDist;
}

/* CheckPointsAgainstPlane  0x0042e2e0 */
qboolean CheckPointsAgainstPlane( const vec3_t *points, int pointCount, const vec3_t normal,
                                  float dist, float epsilon )
{
    int   i;
    float d;

    for ( i = 0; i < pointCount; i++ )
    {
        d = Vec3Dot( points[i], normal ) - dist;

        if ( d < -epsilon )
            return qfalse;
    }

    return qtrue;
}

/* CheckPointsAgainstPlaneReversed  0x0042e340 */
qboolean CheckPointsAgainstPlaneReversed( const vec3_t *points, int pointCount,
                                          const vec3_t normal, float dist, float epsilon )
{
    int   i;
    float d;

    for ( i = 0; i < pointCount; i++ )
    {
        d = Vec3Dot( points[i], normal ) - dist;

        if ( epsilon < d )
            return qfalse;
    }

    return qtrue;
}

/* CheckWindingSeparation  0x0042e3a0 */
qboolean CheckWindingSeparation( const winding_t *w, const winding_t *testW,
                                 const vec3_t normal, float epsilon )
{
    float        minDist;
    float        maxDist;
    vec3_t       edge;
    float        maxAbs;
    unsigned int i;
    vec3_t       edgeNormal;
    float        base;
    float        smallEpsilon;

    smallEpsilon = epsilon * 0.01f;

    for ( i = 0; i < w->ptCount; i++ )
    {
        Vec3Sub( w->pts[( i + 1 ) % w->ptCount], w->pts[i], edge );
        Vec3Cross( normal, edge, edgeNormal );
        Vec3Normalize( edgeNormal );

        base = Vec3Dot( edgeNormal, w->pts[i] );

        WindingPlaneDistExtent( testW, edgeNormal, base, &minDist, &maxDist );

        if ( -smallEpsilon <= minDist )
            return qtrue;

        if ( -epsilon < minDist && minDist * -3.0 < maxDist )
        {
            maxAbs = WindingMaxPlaneDist( w, edgeNormal, base );

            if ( maxAbs * -0.25 < minDist )
                return qtrue;
        }
    }

    return qfalse;
}

/* CheckWindingSeparationBoth  0x0042e4f0 */
qboolean CheckWindingSeparationBoth( const winding_t *w0, const winding_t *w1,
                                     const vec3_t normal, float epsilon )
{
    return CheckWindingSeparation( w0, w1, normal, epsilon )
        || CheckWindingSeparation( w1, w0, normal, epsilon );
}

/* CheckWindingContainment  0x0042e550 */
qboolean CheckWindingContainment( const winding_t *outer, const winding_t *inner,
                                  const vec3_t normal, float epsilon )
{
    vec3_t       edge;
    unsigned int j;
    unsigned int i;
    vec3_t       edgeNormal;
    float        maxDist;

    for ( i = 0; i < outer->ptCount; i++ )
    {
        Vec3Sub( outer->pts[( i + 1 ) % outer->ptCount], outer->pts[i], edge );
        Vec3Cross( normal, edge, edgeNormal );
        Vec3Normalize( edgeNormal );

        maxDist = Vec3Dot( edgeNormal, outer->pts[i] ) + epsilon;

        for ( j = 0; j < inner->ptCount; j++ )
        {
            if ( maxDist < Vec3Dot( inner->pts[j], edgeNormal ) )
                return qfalse;
        }
    }

    return qtrue;
}

/* CheckWindingContainmentBoth  0x0042e640 */
qboolean CheckWindingContainmentBoth( const winding_t *w0, const winding_t *w1,
                                      const vec3_t normal, float epsilon )
{
    return CheckWindingContainment( w0, w1, normal, epsilon )
        && CheckWindingContainment( w1, w0, normal, epsilon );
}

/* WindingLargestTriangleArea  0x0042e6a0 */
float WindingLargestTriangleArea( const vec3_t *pts, int ptCount, const vec3_t normal,
                                  int *outIdx0, int *outIdx1, int *outIdx2 )
{
    int    j;
    float  best;
    vec3_t d1;
    int    k;
    vec3_t d2;
    vec3_t cross;
    float  area;
    int    i;

    *outIdx0 = 0;
    *outIdx1 = 1;
    *outIdx2 = 2;

    best = 0.0f;

    for ( k = 2; k < ptCount; k++ )
    {
        for ( j = 1; j < k; j++ )
        {
            Vec3Sub( pts[k], pts[j], d1 );

            for ( i = 0; i < j; i++ )
            {
                Vec3Sub( pts[i], pts[j], d2 );
                Vec3Cross( d2, d1, cross );

                area = fabs( Vec3Dot( cross, normal ) );

                if ( best < area )
                {
                    best = area;
                    *outIdx0 = i;
                    *outIdx1 = j;
                    *outIdx2 = k;
                }
            }
        }
    }

    return best;
}

/* AnyPointsInsideWinding  0x0042e7e0 */
qboolean AnyPointsInsideWinding( const winding_t *w, const vec3_t normal, float dist,
                                 const vec3_t *pts, unsigned int ptCount )
{
    unsigned int i;
    vec3_t       edge;
    char         culled[MAX_CULL_POINTS];
    unsigned int j;
    unsigned int prev;
    vec3_t       edgeNormal;
    float        base;
    unsigned int culledCount;

    Assertx( ptCount <= ARRAY_COUNT( culled ), "(ptCount) = %i", ptCount );

    memset( culled, 0, ptCount );
    culledCount = 0;

    prev = w->ptCount - 1;

    for ( i = 0; i < w->ptCount; prev = i, i++ )
    {
        Vec3Sub( w->pts[i], w->pts[prev], edge );

        if ( Vec3LengthSq( edge ) >= 0.1 )
        {
            Vec3Cross( normal, edge, edgeNormal );
            Vec3Normalize( edgeNormal );

            base = Vec3Dot( edgeNormal, w->pts[i] );

            for ( j = 0; j < ptCount; j++ )
            {
                if ( !culled[j] )
                {
                    if ( base + 0.1 < Vec3Dot( edgeNormal, pts[j] ) )
                    {
                        culled[j] = 1;
                        culledCount++;

                        if ( culledCount == ptCount )
                            return qfalse;
                    }
                }
            }
        }
    }

    return qtrue;
}

/* RemoveDuplicatePoints  0x0042e9a0 */
void RemoveDuplicatePoints( winding_t *w, float epsilon )
{
    unsigned int lastKeptPtIndex;
    float        epsilonSq;
    unsigned int i;

    Assert( w );
    Assert( w->ptCount >= 3 );

    epsilonSq = epsilon * epsilon;

    while ( w->ptCount != 0
            && Vec3DistanceSq( w->pts[0], w->pts[w->ptCount - 1] ) <= epsilonSq )
    {
        w->ptCount--;
    }

    if ( w->ptCount < 3 )
    {
        w->ptCount = 0;
    }
    else
    {
        lastKeptPtIndex = 0;

        for ( i = 1; i < w->ptCount; i++ )
        {
            if ( epsilonSq < Vec3DistanceSq( w->pts[i], w->pts[lastKeptPtIndex] ) )
            {
                lastKeptPtIndex++;
                Vec3Copy( w->pts[i], w->pts[lastKeptPtIndex] );
            }
        }

        SanityCheckCmp( w->ptCount, >=, lastKeptPtIndex + 1 );

        w->ptCount = lastKeptPtIndex + 1;
    }
}

/* WindingLightmapCoords  0x0042eb20 */
void WindingLightmapCoords( const winding_t *w, const vec4_t *lmapVecs, vec2_t *coordsOut )
{
    unsigned int i;

    for ( i = 0; i < w->ptCount; i++ )
    {
        coordsOut[i][0] = ( Vec3Dot( w->pts[i], lmapVecs[0] ) + lmapVecs[0][3] ) * 1024.0;
        coordsOut[i][1] = ( Vec3Dot( w->pts[i], lmapVecs[1] ) + lmapVecs[1][3] ) * 1024.0;
    }
}

/* LightmapVecsToAxes  0x0042ebb0 */
void LightmapVecsToAxes( const vec4_t plane, const vec4_t *lmapVecs,
                         vec3_t sVec, vec3_t tVec, vec3_t origin )
{
    double det;
    double nx;
    double ny;
    double nz;
    double nd;

    det = ( lmapVecs[0][1] * lmapVecs[1][2] - lmapVecs[0][2] * lmapVecs[1][1] ) * plane[0];
    det = ( lmapVecs[0][2] * lmapVecs[1][0] - lmapVecs[0][0] * lmapVecs[1][2] ) * plane[1] + det;
    det = ( lmapVecs[0][0] * lmapVecs[1][1] - lmapVecs[0][1] * lmapVecs[1][0] ) * plane[2] + det;

    if ( det == 0.0 )
        Com_Error( "singular lightmap coordinates" );

    nx = plane[0] / det;
    ny = plane[1] / det;
    nz = plane[2] / det;
    nd = plane[3] / det;

    sVec[0] = lmapVecs[1][1] * nz - lmapVecs[1][2] * ny;
    sVec[1] = lmapVecs[1][2] * nx - lmapVecs[1][0] * nz;
    sVec[2] = lmapVecs[1][0] * ny - lmapVecs[1][1] * nx;

    tVec[0] = lmapVecs[0][2] * ny - lmapVecs[0][1] * nz;
    tVec[1] = lmapVecs[0][0] * nz - lmapVecs[0][2] * nx;
    tVec[2] = lmapVecs[0][1] * nx - lmapVecs[0][0] * ny;

    origin[0] = ( lmapVecs[0][1] * lmapVecs[1][2] - lmapVecs[0][2] * lmapVecs[1][1] ) * nd;
    origin[1] = ( lmapVecs[0][2] * lmapVecs[1][0] - lmapVecs[0][0] * lmapVecs[1][2] ) * nd;
    origin[2] = ( lmapVecs[0][0] * lmapVecs[1][1] - lmapVecs[0][1] * lmapVecs[1][0] ) * nd;

    Vec3MadCombine( origin, -lmapVecs[0][3], sVec, -lmapVecs[1][3], tVec, origin );

    Vec3Scale( sVec, 0.0009765625f, sVec );
    Vec3Scale( tVec, 0.0009765625f, tVec );
}

/* WindingsCoincident  0x0042edd0 */
qboolean WindingsCoincident( const winding_t *w0, const vec3_t normal0, float dist0,
                             const winding_t *w1, const vec3_t normal1, float dist1 )
{
    unsigned int i;
    float        distDelta;

    if ( Vec3Dot( normal0, normal1 ) <= 0.999f )
        return qfalse;

    distDelta = dist0 - dist1;
    if ( fabs( distDelta ) > 0.001f )
        return qfalse;

    for ( i = 0; i < w0->ptCount; i++ )
    {
        if ( Vec3Dot( w0->pts[i], normal1 ) - dist1 > 0.001f )
            return qfalse;
    }

    for ( i = 0; i < w1->ptCount; i++ )
    {
        if ( Vec3Dot( w1->pts[i], normal0 ) - dist0 > 0.001f )
            return qfalse;
    }

    return qtrue;
}

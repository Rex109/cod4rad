/* Original: ..\common\polylib.cpp */

#ifndef POLYLIB_H
#define POLYLIB_H

#include "q_shared.h"
#include "../src/universal/q_shared.h"

typedef struct
{
    unsigned int ptCount;   /* +0x00 */
    vec3_t       pts[4];    /* +0x04 */
} winding_t;

#define MAX_POINTS_ON_WINDING   1024

#define WINDING_CLIP_SLACK      4

#define MAX_HULL_POINTS         64

#define MAX_CULL_POINTS         16384

#ifndef SIDE_FRONT
#define SIDE_FRONT   0
#define SIDE_BACK    1
#define SIDE_ON      2
#define SIDE_CROSS   3
#endif

typedef struct windingList_s
{
    winding_t            *w;      /* +0x00 */
    struct windingList_s *next;   /* +0x04 */
} windingList_t;

extern int c_activeWindings;      /* 0x12ece908 */

winding_t     *AllocWinding( unsigned int ptCount );                    /* 0x0042b6d0 */
void           FreeWinding( winding_t *w );                             /* 0x0042b730 */
windingList_t *AllocWindingList( void );                                /* 0x0042b750 */
void           FreeWindingList( windingList_t *list );                  /* 0x0042b790 */
winding_t     *CopyWinding( const winding_t *w );                       /* 0x0042c8c0 */
winding_t     *ReverseWinding( const winding_t *w );                    /* 0x0042c900 */

winding_t *BaseWindingForPlane( const vec3_t normal, float dist );      /* 0x0042be70 */
winding_t *WindingForConvexHull( const vec3_t normal, const vec3_t *xyz,
                                 unsigned int xyzCount );               /* 0x0042c0a0 */
winding_t *WindingForConvexHullList( const vec3_t normal,
                                     const windingList_t *list );       /* 0x0042c2d0 */
winding_t *BaseWindingForPlaneInBounds( const vec4_t plane, const vec3_t mins,
                                        const vec3_t maxs );            /* 0x0042c470 */
int        PlaneBoxEdgeIntersections( const vec4_t plane, const vec3_t mins,
                                      const vec3_t maxs, const vec4_t *boxPlanes,
                                      int axis0, int axis1, vec3_t *out ); /* 0x0042c660 */
winding_t *WindingForAxialRect( const vec3_t mins, const vec3_t maxs,
                                int axis0, int axis1, int axis2, float dist ); /* 0x0042c7d0 */

int  ClipWindingEpsilon_Internal( const winding_t *in, const vec3_t normal, float dist,
                                  float epsilon, winding_t **front,
                                  winding_t **back );                   /* 0x0042c980 */
int  ClipWindingEpsilon( const winding_t *in, const vec3_t normal, float dist,
                         float epsilon, winding_t **front, winding_t **back,
                         qboolean keepOnPlane );                        /* 0x0042d050 */
void ClipWindingByPlane( winding_t **inout, const vec3_t normal, float dist,
                         float epsilon );                               /* 0x0042d0f0 */
void ClipWindingByBounds( winding_t **inout, const vec3_t mins, const vec3_t maxs,
                          float epsilon );                              /* 0x0042d4e0 */
winding_t *ChopWinding( winding_t *in, const vec3_t normal, float dist ); /* 0x0042d6b0 */
void AddWindingToConvexHull( const winding_t *w, winding_t **hull,
                             const vec3_t normal );                     /* 0x0042dab0 */

float WindingPlane( const winding_t *w, vec4_t plane );                 /* 0x0042b950 */
int   PlaneFromWindingTriangle( const winding_t *w, vec4_t plane );     /* 0x0042b920 */
qboolean PlaneFromWinding( const winding_t *w, vec4_t plane );          /* 0x0042ba30 */
float WindingArea( const winding_t *w );                                /* 0x0042ba70 */
float WindingAreaWithKnownNormal( const winding_t *w, const vec3_t normal ); /* 0x0042bb20 */
bool  WindingCornerIsConcave( const winding_t *w, unsigned int i0, unsigned int i1,
                              unsigned int i2, const vec3_t normal );   /* 0x0042bbd0 */
bool  WindingIsConvex( const winding_t *w, const vec3_t normal );       /* 0x0042bc70 */
void  WindingBounds( const winding_t *w, vec3_t mins, vec3_t maxs );    /* 0x0042bd20 */
void  WindingCenter( const winding_t *w, vec3_t center );               /* 0x0042bde0 */
float WindingMaxPlaneDist( const winding_t *w, const vec3_t normal, float dist ); /* 0x0042d710 */
void  WindingPlaneDistExtent( const winding_t *w, const vec3_t normal, float dist,
                              float *outMinDist, float *outMaxDist );   /* 0x0042d790 */
int   WindingPlaneSide( const winding_t *w, const vec3_t normal, float dist ); /* 0x0042d820 */
qboolean WindingIsOnPlane( const winding_t *w, const vec3_t normal, float dist,
                           float epsilon );                             /* 0x0042d8f0 */
float WindingMinWidth( const winding_t *w, const vec3_t normal );       /* 0x0042e110 */
float WindingLargestTriangleArea( const vec3_t *pts, int ptCount, const vec3_t normal,
                                  int *outIdx0, int *outIdx1, int *outIdx2 ); /* 0x0042e6a0 */

void RemoveColinearPoints( winding_t *w );                              /* 0x0042b7b0 */
void RemoveDuplicatePoints( winding_t *w, float epsilon );              /* 0x0042e9a0 */
void SnapWindingToPlane( winding_t *w, const vec3_t normal, float dist ); /* 0x0042d960 */
void CheckWindingInPlane( const winding_t *w, const vec3_t normal, float dist ); /* 0x0042dec0 */

qboolean PointOnLine( const vec3_t origin, const vec3_t point, const vec3_t end,
                      float epsilon );                                  /* 0x0042df90 */
qboolean PointOnLine2D( const vec2_t origin, const vec2_t point, const vec2_t end,
                        float epsilon );                                /* 0x0042e050 */
qboolean CheckPointsAgainstPlane( const vec3_t *points, int pointCount,
                                  const vec3_t normal, float dist, float epsilon ); /* 0x0042e2e0 */
qboolean CheckPointsAgainstPlaneReversed( const vec3_t *points, int pointCount,
                                          const vec3_t normal, float dist,
                                          float epsilon );              /* 0x0042e340 */
qboolean CheckWindingSeparation( const winding_t *w, const winding_t *testW,
                                 const vec3_t normal, float epsilon );  /* 0x0042e3a0 */
qboolean CheckWindingSeparationBoth( const winding_t *w0, const winding_t *w1,
                                     const vec3_t normal, float epsilon ); /* 0x0042e4f0 */
qboolean CheckWindingContainment( const winding_t *outer, const winding_t *inner,
                                  const vec3_t normal, float epsilon ); /* 0x0042e550 */
qboolean CheckWindingContainmentBoth( const winding_t *w0, const winding_t *w1,
                                      const vec3_t normal, float epsilon ); /* 0x0042e640 */
qboolean AnyPointsInsideWinding( const winding_t *w, const vec3_t normal, float dist,
                                 const vec3_t *pts, unsigned int ptCount ); /* 0x0042e7e0 */

void WindingLightmapCoords( const winding_t *w, const vec4_t *lmapVecs,
                            vec2_t *coordsOut );                        /* 0x0042eb20 */
void LightmapVecsToAxes( const vec4_t plane, const vec4_t *lmapVecs,
                         vec3_t sVec, vec3_t tVec, vec3_t origin );     /* 0x0042ebb0 */

qboolean WindingsCoincident( const winding_t *w0, const vec3_t normal0, float dist0,
                             const winding_t *w1, const vec3_t normal1, float dist1 );
                                                                        /* 0x0042edd0 */

#endif

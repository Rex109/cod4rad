/* Original: ..\common\brush_edges.cpp */

#ifndef BRUSH_EDGES_H
#define BRUSH_EDGES_H

#include "q_shared.h"

#pragma warning( disable : 4200 )

typedef struct brushPoint_s
{
    vec3_t xyz;         /* +0x00 */
    int    planes[3];   /* +0x0c */
} brushPoint_t;         /* sizeof == 0x18 */

typedef struct
{
    int numsides;   /* +0x00 */
    int sides[];    /* +0x04 */
} adjacencyWinding_t;

#define MAX_SIDE_PTS    1024

#define FREED_WINDING_MARKER    ((int)0xdeaddead)

#define CYCLE_MERGE_DIST        1.0f

#define CONVEX_CROSS_EPSILON    0.01

#define CONVEX_DOT_EPSILON      0.5f

#define MIN_TRIANGLE_AREA       0.001f

void                FreeAdjacencyWinding( adjacencyWinding_t *w );          /* 0x004054e0 */
adjacencyWinding_t *CopyAdjacencyWinding( const adjacencyWinding_t *w );    /* 0x00405520 */
adjacencyWinding_t *AllocAdjacencyWinding( int numsides );                  /* 0x00405590 */
void                ReverseAdjacencyWinding( adjacencyWinding_t *w );       /* 0x00405b60 */

adjacencyWinding_t *BuildAdjacencyWinding( const vec3_t normal, int basePlaneIndex,
                                           const brushPoint_t *brushPts,
                                           int brushPtCount,
                                           adjacencyWinding_t *outWinding,
                                           int maxSides );                  /* 0x004055e0 */

bool  IsPtFormedByThisPlane( int planeIndex, const brushPoint_t *pt );          /* 0x00405d90 */
int   SecondPlane( const brushPoint_t *pt, int planeIndex );                    /* 0x00405e50 */
int   ThirdPlane( const brushPoint_t *pt, int planeIndex0, int planeIndex1 );   /* 0x00405ec0 */

const char *StringFromOffset( const void *base );
const char *StringFromOffset( const void *base, int offset );

#endif

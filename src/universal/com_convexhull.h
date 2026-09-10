/* Original: ..\src\universal\com_convexhull.cpp */

#ifndef COM_CONVEXHULL_H
#define COM_CONVEXHULL_H

#include "q_shared.h"

#define MAX_CONVEX_HULL_POINTS  64

#define CONVEX_HULL_EPSILON     0.001f

int ConvexHull2D( vec2_t *points, int *pointOrder, unsigned int pointCount,
                  int *hull );                                     /* 0x00467620 */

unsigned int ConvexHull2DPoints( vec2_t *points, unsigned int pointCount,
                                 vec2_t *hull );                   /* 0x00466ae0 */

#endif

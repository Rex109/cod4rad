/* Original: ..\common\poly2d.cpp */

#ifndef POLY2D_H
#define POLY2D_H

#include "../src/universal/q_shared.h"

#define MAX_VERTS_PER_POLY      10

#define POLY2D_CLIP_SLACK       4

typedef vec2_t poly2dBuf_t[MAX_VERTS_PER_POLY + POLY2D_CLIP_SLACK];

typedef void ( *Poly2dGridCallback_t )( float area, const vec2_t center, const vec2_t *coords,
                                        int vertCount, void *userData, int cellIndex );

float Poly2dAreaAndCentroid( const vec2_t *coords, int vertCount, vec2_t centroidOut ); /* 0x0042ac30 */

void  Poly2dSplitByAxis( const vec2_t *coords, int vertCount, int axis, float dist,
                         vec2_t *coordsFront, int *frontCountOut,
                         vec2_t *coordsBack, int *backCountOut );            /* 0x0042ae50 */

void  Poly2dSubdivideGrid( poly2dBuf_t *scratch, int vertCount, int countX, int countY,
                           float startX, float startY, float stepX, float stepY,
                           Poly2dGridCallback_t callback, void *userData );  /* 0x0042b350 */

#endif

/* Emissive brushes: glowing faces treated as area lights */

#ifndef EMISSIVE_H
#define EMISSIVE_H

#include "q_shared.h"
#include "com_vector.h"


#define EMISSIVE_DEFAULT_RADIUS     512.0f
#define EMISSIVE_DEFAULT_SAMPLES    16
#define EMISSIVE_MAX_SAMPLES        256


/* Registers one emissive brush.  Call Emissive_BeginBrush, then Emissive_AddTriangle
   for every face triangle (world space, normal pointing out of the brush), then
   Emissive_EndBrush.  color is the linear colour a face shows, radius is where its
   light fades to nothing. */
void Emissive_BeginBrush( const vec3_t color, float radius, int sampleCount );
void Emissive_AddTriangle( const vec3_t v0, const vec3_t v1, const vec3_t v2,
                           const vec3_t outwardNormal );
void Emissive_EndBrush( void );

bool Emissive_Active( void );

/* The light every emissive brush casts onto a point, as one colour arriving from one
   direction (toward the light).  normal is the surface normal at the point, or NULL
   when there is none (the light grid).  outWeight is the average cosine at the
   surface, 1 without a normal.  Returns false when no light reaches the point.
   seed makes the random sampling repeatable. */
bool Emissive_Gather( const vec3_t pos, const vec3_t normal, unsigned seed,
                      vec3_t outColor, vec3_t outDir, float *outWeight );

#endif

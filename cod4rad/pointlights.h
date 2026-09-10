/* Original: c:\trees\cod3\cod3src\cod2rad\pointlights.cpp */

#ifndef POINTLIGHTS_H
#define POINTLIGHTS_H

#include "q_shared.h"
#include "com_vector.h"


#define POINTLIGHT_MAX  2048

#define POINTLIGHT_COINCIDENT_DIST  0.001f

#define POINTLIGHT_TRACE_OFFSET     0.125


#define LIGHT_INFLUENCE_PRIMARY     1
#define LIGHT_INFLUENCE_DIRECTIONAL 2
#define LIGHT_INFLUENCE_COINCIDENT  3


typedef struct
{
    const char *name;       /* +0x00 */
    int     count;          /* +0x04 */
    int     height;         /* +0x08 */
    vec3_t *colors;         /* +0x0c */
} PointLightDef_t;

#define POINTLIGHT_MAX_DEFS     64


typedef struct
{
    int              primaryLightIndex;  /* +0x00 */
    vec3_t           origin;        /* +0x04 */
    float            radius;        /* +0x10 */
    float            falloffScale;  /* +0x14 */
    vec3_t           color;         /* +0x18 */
    PointLightDef_t *def;           /* +0x24 */
    byte             isSpot;        /* +0x28 */
    byte             pad29[0x03];   /* +0x29 */
    vec3_t           spotDir;       /* +0x2c */
    float            cosCutoff;     /* +0x38 */
    float            coneScale;     /* +0x3c */
    float            coneBias;      /* +0x40 */
    int              exponent;      /* +0x44 */
} PointLight_t;                     /* sizeof == 0x48 */


typedef struct
{
    int lightCount;         /* 0x130a81f4 */
} pointLightGlob_t;


extern pointLightGlob_t pointLightGlob;     /* 0x130a81f4 */

extern int             pointLightDefCount;                      /* 0x130a7df0 */
extern PointLightDef_t pointLightDefs[POINTLIGHT_MAX_DEFS];     /* 0x130a7df4 */
extern PointLight_t     pointLights[];      /* 0x130a81f8 */


void PointLight_AddOmni( int primaryLightIndex, const vec3_t origin, float radius,
                         const vec3_t color, const char *defName ); /* 0x0041b200 */
void PointLight_AddSpot( int primaryLightIndex, const vec3_t origin, float radius,
                         const vec3_t color, const char *defName, const vec3_t dir,
                         float cosHalfFovOuter, float cosHalfFovInner,
                         int exponent );                            /* 0x0041b230 */

int PointLight_Count( void );               /* 0x0041b2a0 */

float PointLight_Visibility( int lightIndex, const vec3_t pos,
                             const vec3_t normal );     /* 0x0041b2b0 */

float PointLight_TypeVisibility( int lightType, const vec3_t pos,
                                 const vec3_t normal );         /* 0x0041b990 */

int PointLight_Sample( int lightType, int lightIndex, const vec3_t pos,
                       const vec3_t axis0, const vec3_t axis1,
                       const vec3_t normal, vec3_t outDir, vec3_t outColor,
                       float *outWeight );              /* 0x0041b580 */

#endif

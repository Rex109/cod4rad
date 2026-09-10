/* Original: c:\trees\cod3\cod3src\cod2rad\modelcollision.cpp */

#ifndef MODELCOLLISION_H
#define MODELCOLLISION_H

#include "q_shared.h"
#include "com_vector.h"
#include "aabbtree.h"
#include "geometry.h"

struct XModel_s;
struct ModelAabbNode_s;


#define MODEL_CELL_NODE_COUNT   0x400

#define MODEL_CELL_EPSILON      0.001f

#define MODEL_MAX_Z_START       -131072.0f


#define MODEL_SURF_NO_CAST      0x00040000
#define MODEL_CONTENTS_CAST     0x00002003

#define MODEL_MAX_SURFACES      64
#define MODEL_MAX_MESH_VERTS    32768
#define MODEL_MAX_MESH_INDICES  32640


typedef struct ModelCollTri_s
{
    vec3_t                xyz;      /* +0x00 */
    vec3_t                edge0;    /* +0x0c */
    vec3_t                edge1;    /* +0x18 */
    void                 *mskMtl;   /* +0x24 */
    vec2_t                st[3];    /* +0x28 */
} ModelCollTri_t;                   /* sizeof == 0x40 */

typedef struct RadModelDef_s
{
    const struct XModel_s *xmodel;      /* +0x00 */
    struct ModelAabbNode_s *tree;       /* +0x04 */
    struct RadModelDef_s  *next;        /* +0x08 */
} RadModelDef_t;                        /* sizeof == 0x0c */

typedef struct RadModel_s
{
    RadModelDef_t     *def;             /* +0x00 */
    byte               pad04[0x10];     /* +0x04 */
    vec3_t             origin;          /* +0x14 */
    float              axis[3][3];      /* +0x20 */
    vec3_t             mins;            /* +0x44 */
    vec3_t             maxs;            /* +0x50 */
    byte               scripted;        /* +0x5c */
    byte               pad5d[0x03];     /* +0x5d */
    struct RadModel_s *next;            /* +0x60 */
} RadModel_t;                           /* sizeof == 0x64 */

typedef struct
{
    int         axis;       /* +0x00 */
    float       dist;       /* +0x04 */
    int         itemCount;  /* +0x08 */
    RadModel_t *models;     /* +0x0c */
} ModelCellNode_t;          /* sizeof == 0x10 */


#define MODEL_AABB_MAX_NODES        0x2000
#define MODEL_AABB_ITEM_STRIDE      64
#define MODEL_AABB_MIN_PARTITION    0x10
#define MODEL_AABB_MIN_LEAF_ITEMS   0x20


typedef struct ModelAabbNode_s
{
    vec3_t mins;        /* +0x00 */
    vec3_t maxs;        /* +0x0c */
    int    itemCount;   /* +0x18 */
    void  *items;       /* +0x1c */
    int    childCount;  /* +0x20 */
    void  *children;    /* +0x24 */
} ModelAabbNode_t;      /* sizeof == 0x28 */


extern float           modelMaxZ;                               /* 0x130a3df4 */
extern RadModel_t     *radModels;                               /* 0x130a3df8 */
extern ModelCellNode_t modelCells[MODEL_CELL_NODE_COUNT];       /* 0x130a3dfc */


void ModelCollision_AddStaticModel( struct XModel_s *xmodel, const vec3_t scale,
                                    const orientation_t *orient,
                                    qboolean scripted );        /* 0x0041a990 */

extern RadModelDef_t *radModelDefs;                             /* 0x130a3df0 */

void ModelCollision_BuildCellTree( const vec3_t mins, const vec3_t maxs ); /* 0x0041a250 */

ModelAabbNode_t *ModelCollision_BuildAabbTree( int triCount, void *tris,
                                               vec3_t *triMins,
                                               vec3_t *triMaxs ); /* 0x0041a2a0 */

void ModelCollision_TraceModel( RadModel_t *model, GeoTrace_t *trace ); /* 0x0041ac10 */

void ModelCollision_TraceNode_r( int nodeIndex, const vec3_t start,
                                 const vec3_t end, GeoTrace_t *trace ); /* 0x0041ad30 */

void Model_TraceLine( GeoTrace_t *trace );                      /* 0x0041af10 */

#endif

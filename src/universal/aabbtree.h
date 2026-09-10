/* Original: ..\src\universal\aabbtree.cpp */

#ifndef AABBTREE_H
#define AABBTREE_H

#include "q_shared.h"

typedef struct
{
    int firstItem;      /* +0x00 */
    int itemCount;      /* +0x04 */
    int firstChild;     /* +0x08 */
    int childCount;     /* +0x0c */
} AabbTreeNode_t;

typedef struct
{
    void           *itemData;           /* +0x00 */
    int             itemCount;          /* +0x04 */
    int             itemStride;         /* +0x08 */
    int             reorderBounds;      /* +0x0c */
    vec3_t         *itemMins;           /* +0x10 */
    vec3_t         *itemMaxs;           /* +0x14 */
    AabbTreeNode_t *nodes;              /* +0x18 */
    int             maxNodes;           /* +0x1c */
    int             minPartitionSize;   /* +0x20 */
    int             minLeafItems;       /* +0x24 */
} AabbTreeBuilder_t;

#define AABB_STACK_ITEMS  4096

extern int    aabbTreeCount;        /* 0x2b920aa8 */
extern float *aabbSortMins;         /* 0x2b920ab0 */
extern float *aabbSortMaxs;         /* 0x2b920aac */
extern float *aabbSortEqual;        /* 0x2b920ab4 */

int AabbBuildTree( AabbTreeBuilder_t *builder );                    /* 0x00465730 */

#endif

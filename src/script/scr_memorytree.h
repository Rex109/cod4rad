/* Original: c:\trees\cod3\cod3src\src\script\scr_memorytree.cpp */

#ifndef SCR_MEMORYTREE_H
#define SCR_MEMORYTREE_H

#include "q_shared.h"


#define MT_NODE_SIZE        12
#define MEMORY_NODE_BITS    16
#define MEMORY_NODE_COUNT   0x10000
#define MT_SIZE             ( MEMORY_NODE_COUNT * MT_NODE_SIZE )


#define MEM_EMPTY               0
#define MEM_THREAD              1
#define MEM_VECTOR              2
#define MEM_NOTETRACK           3
#define MEM_ANIM_TREE           4
#define MEM_SMALL_ANIM_TREE     5
#define MEM_EXTERNAL            6
#define MEM_TEMP                7
#define MEM_SURFACE             8
#define MEM_ANIM_PART           9
#define MEM_MODEL_PART          10
#define MEM_MODEL_PART_MAP      11
#define MEM_DUPLICATE_PARTS     12
#define MEM_MODEL_LIST          13
#define MEM_SCRIPT_PARSE        14
#define MEM_SCRIPT_STRING       15
#define MEM_CLASS               16
#define MEM_TAG_INFO            17
#define MEM_ANIMSCRIPTED        18
#define MEM_CONFIG_STRING       19
#define MEM_TYPE_COUNT          20


typedef struct
{
    unsigned short prev;                /* +0x00 */
    unsigned short next;                /* +0x02 */
    byte           unused[8];           /* +0x04 */
} MemoryNode;


typedef struct
{
    char *mt_buffer;                                /* 0x131ad600 */
} scrMemTreePub_t;

typedef struct
{
    MemoryNode    *nodes;
    unsigned short freeHead[MEMORY_NODE_BITS + 1];  /* 0x1318d580 */
    int            totalAlloc;                      /* 0x1318d5a4 */
    int            totalAllocBuckets;               /* 0x1318d5a8 */
} scrMemTreeGlob_t;

typedef struct
{
    byte mt_usage[MEMORY_NODE_COUNT];               /* 0x1318d600 */
    byte mt_usage_size[MEMORY_NODE_COUNT];          /* 0x1319d600 */
} scrMemTreeDebugGlob_t;


extern scrMemTreePub_t       scrMemTreePub;
extern scrMemTreeGlob_t      scrMemTreeGlob;
extern scrMemTreeDebugGlob_t scrMemTreeDebugGlob;


void  MT_Init( void );                                  /* 0x00428950 */
void *MT_Alloc( int numBytes, int type );               /* 0x00428e90 */
void  MT_Free( void *p, int numBytes );                 /* 0x00428ec0 */

unsigned short MT_AllocIndex( int numBytes, int type ); /* 0x00428ac0 */
void           MT_FreeIndex( int nodeNum, int numBytes );/* 0x00428cd0 */

#endif

/* Original: c:\trees\cod3\cod3src\src\script\scr_memorytree.cpp */

#include "cod4rad.h"
#include "scr_memorytree.h"

#include <string.h>


scrMemTreePub_t       scrMemTreePub;
scrMemTreeGlob_t      scrMemTreeGlob;
scrMemTreeDebugGlob_t scrMemTreeDebugGlob;

static MemoryNode s_memoryNodes[MEMORY_NODE_COUNT];     /* 0x130cd280 */

static byte s_bitLowIndex[256];         /* 0x1318d280 */
static byte s_bitCount[256];            /* 0x1318d380 */
static byte s_bitLength[256];           /* 0x1318d480 */


/* MT_InitBits  0x00428340 */
static void MT_InitBits( void )
{
    int num;
    int count;
    int value;
    int shift;

    for ( num = 0; num < 256; num++ )
    {
        count = 0;
        for ( value = num; value; value >>= 1 )
        {
            if ( value & 1 )
                count++;
        }
        s_bitCount[num] = ( byte )count;

        shift = 8;
        if ( num )
        {
            do
            {
                shift--;
            }
            while ( num & ( ( 1 << shift ) - 1 ) );
        }
        s_bitLowIndex[num] = ( byte )shift;

        count = 0;
        for ( value = num; value; value >>= 1 )
            count++;
        s_bitLength[num] = ( byte )count;
    }
}

/* MT_GetScore  0x004283b0 */
static int MT_GetScore( int num )
{
    int inverse;
    int hi;
    int lo;
    int shift;

    Assert( num );

    inverse = MEMORY_NODE_COUNT - num;
    Assert( inverse );

    hi = ( inverse >> 8 ) & 0xff;
    lo = inverse & 0xff;

    inverse -= s_bitCount[hi];
    inverse -= s_bitCount[lo];

    shift = s_bitLowIndex[lo];
    if ( !lo )
        shift += s_bitLowIndex[hi];

    return ( 1 << shift ) + inverse;
}

/* MT_AddNode  0x00428430 */
static void MT_AddNode( int size, int nodeNum )
{
    unsigned short *parentNode;
    int             node;
    int             score;
    int             mid;
    int             half;

    Assert( size >= 0 && size <= MEMORY_NODE_BITS );

    parentNode = &scrMemTreeGlob.freeHead[size];
    node       = *parentNode;
    mid        = 0;
    half       = MEMORY_NODE_COUNT;

    if ( node )
    {
        score = MT_GetScore( nodeNum );

        for ( ;; )
        {
            int newScore;

            Assert( nodeNum != node );

            newScore = MT_GetScore( node );
            Assert( newScore != score );

            if ( newScore < score )
            {
                for ( ;; )
                {
                    Assert( node == *parentNode );
                    Assert( node != nodeNum );

                    *parentNode = ( unsigned short )nodeNum;
                    scrMemTreeGlob.nodes[nodeNum] = scrMemTreeGlob.nodes[node];

                    if ( !node )
                        return;

                    half >>= 1;
                    Assert( node != mid );

                    if ( node < mid )
                    {
                        mid -= half;
                        parentNode = &scrMemTreeGlob.nodes[nodeNum].prev;
                    }
                    else
                    {
                        mid += half;
                        parentNode = &scrMemTreeGlob.nodes[nodeNum].next;
                    }

                    nodeNum = node;
                    node    = *parentNode;
                }
            }

            half >>= 1;
            Assert( nodeNum != mid );

            if ( nodeNum < mid )
            {
                mid -= half;
                parentNode = &scrMemTreeGlob.nodes[node].prev;
            }
            else
            {
                mid += half;
                parentNode = &scrMemTreeGlob.nodes[node].next;
            }

            node = *parentNode;
            if ( !node )
                break;
        }
    }

    *parentNode = ( unsigned short )nodeNum;

    scrMemTreeGlob.nodes[nodeNum].prev = 0;
    scrMemTreeGlob.nodes[nodeNum].next = 0;
}

static void MT_FillHole( unsigned short *hole, int node )
{
    MemoryNode saved;

    saved = scrMemTreeGlob.nodes[node];

    for ( ;; )
    {
        MemoryNode moved;
        int        child;

        if ( !saved.prev )
        {
            *hole = saved.next;
            if ( !saved.next )
                return;

            child = saved.next;
            hole  = &scrMemTreeGlob.nodes[child].next;
        }
        else if ( !saved.next )
        {
            child = saved.prev;
            *hole = ( unsigned short )child;
            hole  = &scrMemTreeGlob.nodes[child].prev;
        }
        else
        {
            int prevScore;
            int nextScore;

            prevScore = MT_GetScore( saved.prev );
            nextScore = MT_GetScore( saved.next );
            Assert( prevScore != nextScore );

            if ( prevScore < nextScore )
            {
                child = saved.next;
                *hole = ( unsigned short )child;
                hole  = &scrMemTreeGlob.nodes[child].next;
            }
            else
            {
                child = saved.prev;
                *hole = ( unsigned short )child;
                hole  = &scrMemTreeGlob.nodes[child].prev;
            }
        }

        Assert( child );

        moved                       = scrMemTreeGlob.nodes[child];
        scrMemTreeGlob.nodes[child] = saved;
        saved                       = moved;
    }
}

/* MT_RemoveNode  0x00428640 */
static bool MT_RemoveNode( int size, int nodeNum )
{
    unsigned short *parentNode;
    int             node;
    int             mid;
    int             half;

    Assert( size >= 0 && size <= MEMORY_NODE_BITS );

    parentNode = &scrMemTreeGlob.freeHead[size];
    node       = *parentNode;
    mid        = 0;
    half       = MEMORY_NODE_COUNT;

    while ( node )
    {
        if ( node == nodeNum )
        {
            MT_FillHole( parentNode, node );
            return true;
        }

        if ( nodeNum == mid )
            return false;

        half >>= 1;

        if ( nodeNum < mid )
        {
            mid -= half;
            parentNode = &scrMemTreeGlob.nodes[node].prev;
        }
        else
        {
            mid += half;
            parentNode = &scrMemTreeGlob.nodes[node].next;
        }

        node = *parentNode;
    }

    return false;
}

/* MT_RemoveHeadNode  0x004287f0 */
static void MT_RemoveHeadNode( int size )
{
    Assert( size >= 0 && size <= MEMORY_NODE_BITS );

    MT_FillHole( &scrMemTreeGlob.freeHead[size], scrMemTreeGlob.freeHead[size] );
}

/* MT_Init  0x00428950 */
void MT_Init( void )
{
    int i;

    Assert( Sys_IsMainThread() );

    scrMemTreePub.mt_buffer = ( char * )s_memoryNodes;
    scrMemTreeGlob.nodes    = s_memoryNodes;

    MT_InitBits();

    memset( scrMemTreeGlob.freeHead, 0, sizeof( scrMemTreeGlob.freeHead ) );

    scrMemTreeGlob.nodes[0].prev = 0;
    scrMemTreeGlob.nodes[0].next = 0;

    for ( i = 0; i < MEMORY_NODE_BITS; i++ )
        MT_AddNode( i, 1 << i );

    scrMemTreeGlob.totalAlloc        = 0;
    scrMemTreeGlob.totalAllocBuckets = 0;

    memset( scrMemTreeDebugGlob.mt_usage,      0, MEMORY_NODE_COUNT );
    memset( scrMemTreeDebugGlob.mt_usage_size, 0, MEMORY_NODE_COUNT );
}

/* MT_GetSize  0x00428a40 */
static int MT_GetSize( int numBytes )
{
    int num;

    Assert( numBytes > 0 );

    if ( numBytes >= MEMORY_NODE_COUNT )
    {
        Com_Error( 1, "%s: failed allocation of %d bytes for script usage",
                   "MT_GetSize: max allocation exceeded", numBytes );
        return 0;
    }

    num = ( numBytes + MT_NODE_SIZE - 1 ) / MT_NODE_SIZE - 1;

    if ( num <= 0xff )
        return s_bitLength[num];

    return s_bitLength[num >> 8] + 8;
}

/* MT_AllocIndex  0x00428ac0 */
unsigned short MT_AllocIndex( int numBytes, int type )
{
    int size;
    int bucket;
    int nodeNum;

    size = MT_GetSize( numBytes );
    Assert( size >= 0 && size <= MEMORY_NODE_BITS );
    Assert( Sys_IsMainThread() );

    nodeNum = 0;
    for ( bucket = size; bucket <= MEMORY_NODE_BITS; bucket++ )
    {
        nodeNum = scrMemTreeGlob.freeHead[bucket];
        if ( nodeNum )
            break;
    }

    if ( bucket > MEMORY_NODE_BITS )
    {
        Com_Error( 1, "%s: failed allocation of %d bytes for script usage",
                   "MT_AllocIndex", numBytes );
        return 0;
    }

    MT_RemoveHeadNode( bucket );

    while ( bucket != size )
    {
        bucket--;
        MT_AddNode( bucket, nodeNum + ( 1 << bucket ) );
    }

    Assert( nodeNum >= 0 && nodeNum < MEMORY_NODE_COUNT );

    scrMemTreeGlob.totalAlloc++;
    scrMemTreeGlob.totalAllocBuckets += 1 << size;

    Assert( type );
    Assert( !scrMemTreeDebugGlob.mt_usage[nodeNum] );
    Assert( !scrMemTreeDebugGlob.mt_usage_size[nodeNum] );

    scrMemTreeDebugGlob.mt_usage[nodeNum]      = ( byte )type;
    scrMemTreeDebugGlob.mt_usage_size[nodeNum] = ( byte )size;

    return ( unsigned short )nodeNum;
}

/* MT_FreeIndex  0x00428cd0 */
void MT_FreeIndex( int nodeNum, int numBytes )
{
    int size;
    int lowBit;

    size = MT_GetSize( numBytes );
    Assert( size >= 0 && size <= MEMORY_NODE_BITS );
    Assert( nodeNum > 0 && nodeNum < MEMORY_NODE_COUNT );
    Assert( Sys_IsMainThread() );

    scrMemTreeGlob.totalAlloc--;
    scrMemTreeGlob.totalAllocBuckets -= 1 << size;

    Assert( scrMemTreeDebugGlob.mt_usage[nodeNum] );
    Assert( scrMemTreeDebugGlob.mt_usage_size[nodeNum] == size );

    scrMemTreeDebugGlob.mt_usage[nodeNum]      = 0;
    scrMemTreeDebugGlob.mt_usage_size[nodeNum] = 0;

    for ( ;; )
    {
        Assert( size <= MEMORY_NODE_BITS );

        lowBit = 1 << size;
        Assert( nodeNum == ( nodeNum & ~( lowBit - 1 ) ) );

        if ( size == MEMORY_NODE_BITS )
            break;

        if ( !MT_RemoveNode( size, lowBit ^ nodeNum ) )
            break;

        nodeNum &= ~lowBit;
        size++;
    }

    MT_AddNode( size, nodeNum );
}

/* MT_Alloc  0x00428e90 */
void *MT_Alloc( int numBytes, int type )
{
    return &scrMemTreeGlob.nodes[MT_AllocIndex( numBytes, type )];
}

/* MT_Free  0x00428ec0 */
void MT_Free( void *p, int numBytes )
{
    int nodeNum;

    nodeNum = ( int )( ( MemoryNode * )p - scrMemTreeGlob.nodes );
    Assert( nodeNum >= 0 && nodeNum < MEMORY_NODE_COUNT );

    MT_FreeIndex( nodeNum, numBytes );
}

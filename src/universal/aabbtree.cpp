/* Original: ..\src\universal\aabbtree.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_math.h"
#include "com_vector.h"
#include "aabbtree.h"
#include "cmdlib.h"
#include "qsort_vc8.h"


extern int RoundFloatToIntHalf( float value );                       /* 0x0044c300 */


int    aabbTreeCount;                                                /* 0x2b920aa8 */

float *aabbSortMaxs;                                               /* 0x2b920aac */
float *aabbSortMins;                                               /* 0x2b920ab0 */
float *aabbSortEqual;                                              /* 0x2b920ab4 */


static int   CompareFunction( const void *a, const void *b );        /* 0x004668b0 */
static float AabbVolumeIncrease( const vec3_t itemMins, const vec3_t itemMaxs,
                                 const vec3_t mins, const vec3_t maxs );
                                                                     /* 0x00466910 */
static AabbTreeNode_t *AabbAllocNode( AabbTreeBuilder_t *builder );  /* 0x00466a90 */
static qboolean AabbFindBestSplitPlane( const vec3_t *itemMins, const vec3_t *itemMaxs,
                                        const int *indices, int count,
                                        int *outBestAxis, float *outSplitPos );
                                                                     /* 0x004662c0 */
static qboolean AabbPartitionItems( int itemCount, AabbTreeBuilder_t *builder,
                                    int *indices, int *outFrontCount,
                                    int *outMidStart );              /* 0x00465d00 */
static void AabbBuildSubtree( AabbTreeNode_t *parent, AabbTreeBuilder_t *builder,
                              int *indices, int offset, int count ); /* 0x004669a0 */
static int  AabbBuildTree_r( AabbTreeNode_t *tree, AabbTreeBuilder_t *builder,
                             int *indices );                         /* 0x00465b70 */


/* CompareFunction  0x004668b0 */
static int CompareFunction( const void *a, const void *b )
{
    float diff;

    diff = *(const float *)a - *(const float *)b;

    if ( diff < 0.0f )
        return -1;

    return diff > 0.0f;
}


/* AabbVolumeIncrease  0x00466910 */
static float AabbVolumeIncrease( const vec3_t itemMins, const vec3_t itemMaxs,
                                 const vec3_t mins, const vec3_t maxs )
{
    vec3_t newMins;
    vec3_t newMaxs;
    float  newVolume;
    float  oldVolume;
    float  increase;

    Vec3Copy( mins, newMins );
    Vec3Copy( maxs, newMaxs );
    AddBoundsToBounds( itemMins, itemMaxs, newMins, newMaxs );

    newVolume = ( newMaxs[0] - newMins[0] ) * ( newMaxs[1] - newMins[1] ) * ( newMaxs[2] - newMins[2] );
    oldVolume = ( maxs[0] - mins[0] ) * ( maxs[1] - mins[1] ) * ( maxs[2] - mins[2] );

    increase = newVolume - oldVolume;
    return increase;
}


/* AabbAllocNode  0x00466a90 */
static AabbTreeNode_t *AabbAllocNode( AabbTreeBuilder_t *builder )
{
    int nodeIndex;

    if ( aabbTreeCount == builder->maxNodes )
        Com_Error( "More than %i AABB nodes needed\n", builder->maxNodes );

    nodeIndex = aabbTreeCount;
    aabbTreeCount = aabbTreeCount + 1;

    return &builder->nodes[nodeIndex];
}


/* AabbFindBestSplitPlane  0x004662c0 */
static qboolean AabbFindBestSplitPlane( const vec3_t *itemMins, const vec3_t *itemMaxs,
                                        const int *indices, int count,
                                        int *outBestAxis, float *outSplitPos )
{
    vec3_t       overallMins;
    vec3_t       overallMaxs;
    int          axisScale[3];
    unsigned int longestAxis;
    int          axis;
    int          i;
    int          sortCount;
    int          equalCount;
    float        minVal;
    float        maxVal;
    int          minsIdx;
    int          maxsIdx;
    int          equalIdx;
    int          minsStep;
    int          equalStep;
    float        currentSplitPos;
    float        nextSplitPos;
    int          sideFrontCount;
    int          sideBackCount;
    int          sideSplitCount;
    int          sideOnCount;
    int          bestScore;
    int          score;

    ClearBounds( overallMins, overallMaxs );
    for ( i = 0; i < count; i++ )
        AddBoundsToBounds( itemMins[indices[i]], itemMaxs[indices[i]], overallMins, overallMaxs );

    longestAxis = ( overallMaxs[1] - overallMins[1] ) < ( overallMaxs[0] - overallMins[0] );
    if ( ( overallMaxs[2] - overallMins[2] ) < ( overallMaxs[longestAxis] - overallMins[longestAxis] ) )
        longestAxis = 2;

    for ( i = 0; i < 3; i++ )
    {
        axisScale[i] = RoundFloatToIntHalf(
            ( ( overallMaxs[i] - overallMins[i] ) + 1.0f ) * 10.0f /
            ( ( overallMaxs[longestAxis] - overallMins[longestAxis] ) + 1.0f ) );
    }

    bestScore = INT_MIN;

    for ( axis = 0; axis < 3; axis++ )
    {
        sortCount  = 0;
        equalCount = 0;

        for ( i = 0; i < count; i++ )
        {
            minVal = itemMins[indices[i]][axis];
            maxVal = itemMaxs[indices[i]][axis];

            if ( minVal != maxVal )
            {
                aabbSortMins[sortCount] = itemMins[indices[i]][axis];
                aabbSortMaxs[sortCount] = itemMaxs[indices[i]][axis];
                sortCount++;
            }
            else
            {
                aabbSortEqual[equalCount] = itemMins[indices[i]][axis];
                equalCount++;
            }
        }

        qsort_vc8( aabbSortMins,  sortCount,  sizeof( aabbSortMins[0] ),  CompareFunction );
        qsort_vc8( aabbSortMaxs,  sortCount,  sizeof( aabbSortMaxs[0] ),  CompareFunction );
        qsort_vc8( aabbSortEqual, equalCount, sizeof( aabbSortEqual[0] ), CompareFunction );

        sideFrontCount = 0;
        sideBackCount  = count;
        sideSplitCount = 0;
        sideOnCount    = 0;
        minsIdx        = 0;
        maxsIdx        = 0;
        equalIdx       = 0;
        minsStep       = 0;
        equalStep      = 0;

        nextSplitPos = I_fmin( aabbSortMins[0], aabbSortEqual[0] );

        while ( nextSplitPos < FLT_MAX )
        {
            currentSplitPos = nextSplitPos;
            nextSplitPos    = FLT_MAX;

            sideSplitCount = sideSplitCount + minsStep;
            sideBackCount  = sideBackCount  - minsStep;
            minsStep       = 0;

            for ( ; minsIdx < sortCount && aabbSortMins[minsIdx] == currentSplitPos; minsIdx++ )
                minsStep++;

            if ( minsIdx < sortCount && aabbSortMins[minsIdx] < FLT_MAX )
                nextSplitPos = aabbSortMins[minsIdx];

            while ( maxsIdx < sortCount && aabbSortMaxs[maxsIdx] == currentSplitPos )
            {
                sideFrontCount = sideFrontCount + 1;
                sideSplitCount = sideSplitCount - 1;
                maxsIdx++;
            }

            if ( maxsIdx < sortCount && aabbSortMaxs[maxsIdx] < nextSplitPos )
                nextSplitPos = aabbSortMaxs[maxsIdx];

            sideFrontCount = sideFrontCount + equalStep;
            sideOnCount    = sideOnCount    - equalStep;
            equalStep      = 0;

            for ( ; equalIdx < equalCount && aabbSortEqual[equalIdx] == currentSplitPos; equalIdx++ )
                equalStep++;

            sideOnCount   = sideOnCount   + equalStep;
            sideBackCount = sideBackCount - equalStep;

            if ( equalIdx < equalCount && aabbSortEqual[equalIdx] < nextSplitPos )
                nextSplitPos = aabbSortEqual[equalIdx];

            Assert( sideFrontCount + sideBackCount + sideSplitCount + sideOnCount == count );
            Assert( sideFrontCount >= 0 );
            Assert( sideBackCount  >= 0 );
            Assert( sideSplitCount >= 0 );
            Assert( sideOnCount    >= 0 );

            if ( sideFrontCount > 1 && sideBackCount > 1 )
            {
                score = count - abs( sideFrontCount - sideBackCount ) - sideOnCount
                      + sideSplitCount * -4 + axisScale[axis];

                if ( sideOnCount == 0 && sideSplitCount == 0 && minsStep == 0 )
                    score = RoundFloatToInt( nextSplitPos - currentSplitPos ) + score;

                if ( bestScore < score )
                {
                    bestScore    = score;
                    *outBestAxis = axis;

                    if ( sideOnCount == 0 && sideSplitCount == 0 && minsStep == 0 )
                        *outSplitPos = ( currentSplitPos + nextSplitPos ) * 0.5f;
                    else
                        *outSplitPos = currentSplitPos;
                }
            }
        }
    }

    return bestScore != INT_MIN;
}


/* AabbPartitionItems  0x00465d00 */
static qboolean AabbPartitionItems( int itemCount, AabbTreeBuilder_t *builder, int *indices,
                                    int *outFrontCount, int *outMidStart )
{
    const vec3_t *minsBuf;
    const vec3_t *maxsBuf;
    int           splitAxis;
    float         splitPos;
    vec3_t        frontMins, frontMaxs;
    vec3_t        backMins,  backMaxs;
    int           frontIdx;
    int           backIdx;
    int           midIdx;
    int           temp;

    minsBuf = builder->itemMins;
    maxsBuf = builder->itemMaxs;

    if ( !AabbFindBestSplitPlane( minsBuf, maxsBuf, indices, itemCount, &splitAxis, &splitPos ) )
        return 0;

    ClearBounds( frontMins, frontMaxs );
    ClearBounds( backMins,  backMaxs  );

    frontIdx = 0;
    backIdx  = itemCount - 1;

scan_pass:
    if ( frontIdx > backIdx )
        goto scan_done;

    while ( frontIdx <= backIdx
            && maxsBuf[indices[frontIdx]][splitAxis] <= splitPos
            && minsBuf[indices[frontIdx]][splitAxis] <  splitPos )
    {
        AddBoundsToBounds( minsBuf[indices[frontIdx]], maxsBuf[indices[frontIdx]], frontMins, frontMaxs );
        frontIdx++;
    }

    while ( frontIdx <= backIdx
            && minsBuf[indices[backIdx]][splitAxis] >= splitPos
            && maxsBuf[indices[backIdx]][splitAxis] >  splitPos )
    {
        AddBoundsToBounds( minsBuf[indices[backIdx]], maxsBuf[indices[backIdx]], backMins, backMaxs );
        backIdx--;
    }

    if ( frontIdx > backIdx )
        goto scan_done;

    if ( ( minsBuf[indices[frontIdx]][splitAxis] >= splitPos && maxsBuf[indices[frontIdx]][splitAxis] > splitPos )
      || ( maxsBuf[indices[backIdx]][splitAxis]  <= splitPos && minsBuf[indices[backIdx]][splitAxis]  < splitPos ) )
    {
        temp              = indices[frontIdx];
        indices[frontIdx] = indices[backIdx];
        indices[backIdx]  = temp;
        goto scan_pass;
    }

    for ( midIdx = frontIdx; midIdx < backIdx; midIdx++ )
    {
        if ( minsBuf[indices[midIdx]][splitAxis] >= splitPos
          && maxsBuf[indices[midIdx]][splitAxis] >  splitPos )
        {
            temp             = indices[midIdx];
            indices[midIdx]  = indices[backIdx];
            indices[backIdx] = temp;
            break;
        }

        if ( maxsBuf[indices[midIdx]][splitAxis] <= splitPos
          && minsBuf[indices[midIdx]][splitAxis] <  splitPos )
        {
            temp              = indices[midIdx];
            indices[midIdx]   = indices[frontIdx];
            indices[frontIdx] = temp;
            break;
        }
    }

    if ( midIdx == backIdx )
        goto scan_done;
    goto scan_pass;

scan_done:
    if ( frontIdx > backIdx )
        goto finish;

    if ( frontIdx >= builder->minPartitionSize
      && backIdx - frontIdx + 1 >= builder->minPartitionSize
      && itemCount - backIdx - 1 >= builder->minPartitionSize )
        goto finish;

vol_pass:
    if ( frontIdx > backIdx )
        goto finish;

    while ( frontIdx <= backIdx
            && AabbVolumeIncrease( minsBuf[indices[frontIdx]], maxsBuf[indices[frontIdx]], frontMins, frontMaxs )
            <= AabbVolumeIncrease( minsBuf[indices[frontIdx]], maxsBuf[indices[frontIdx]], backMins,  backMaxs  ) )
    {
        AddBoundsToBounds( minsBuf[indices[frontIdx]], maxsBuf[indices[frontIdx]], frontMins, frontMaxs );
        frontIdx++;
    }

    while ( frontIdx <= backIdx
            && AabbVolumeIncrease( minsBuf[indices[backIdx]], maxsBuf[indices[backIdx]], backMins,  backMaxs  )
            <= AabbVolumeIncrease( minsBuf[indices[backIdx]], maxsBuf[indices[backIdx]], frontMins, frontMaxs ) )
    {
        AddBoundsToBounds( minsBuf[indices[backIdx]], maxsBuf[indices[backIdx]], backMins, backMaxs );
        backIdx--;
    }

    if ( frontIdx < backIdx )
    {
        temp              = indices[frontIdx];
        indices[frontIdx] = indices[backIdx];
        indices[backIdx]  = temp;
        frontIdx++;
        backIdx--;
    }
    else if ( frontIdx == backIdx )
    {
        if ( frontIdx + frontIdx < itemCount )
            frontIdx++;
        else
            backIdx--;
    }
    goto vol_pass;

finish:
    if ( frontIdx == 0 || frontIdx == itemCount )
        return 0;

    *outFrontCount = frontIdx;
    *outMidStart   = backIdx + 1;
    return 1;
}


/* AabbBuildSubtree  0x004669a0 */
static void AabbBuildSubtree( AabbTreeNode_t *parent, AabbTreeBuilder_t *builder,
                              int *indices, int offset, int count )
{
    AabbTreeNode_t *node;
    int             frontCount;
    int             midStart;

    if ( builder->minLeafItems < count
      && AabbPartitionItems( count, builder, indices + offset, &frontCount, &midStart ) )
    {
        node = AabbAllocNode( builder );
        node->firstItem = parent->firstItem + offset;
        node->itemCount = frontCount;

        if ( frontCount < midStart )
        {
            node = AabbAllocNode( builder );
            node->firstItem = parent->firstItem + offset + frontCount;
            node->itemCount = midStart - frontCount;
        }

        node = AabbAllocNode( builder );
        node->firstItem = parent->firstItem + offset + midStart;
        node->itemCount = count - midStart;
        return;
    }

    node = AabbAllocNode( builder );
    node->firstItem = parent->firstItem + offset;
    node->itemCount = count;
}


/* AabbBuildTree_r  0x00465b70 */
static int AabbBuildTree_r( AabbTreeNode_t *tree, AabbTreeBuilder_t *builder, int *indices )
{
    AabbTreeNode_t *children;
    int             frontCount;
    int             midStart;
    int             i;

    Assert( tree->itemCount );

    tree->firstChild = aabbTreeCount;
    tree->childCount = 0;

    if ( builder->minLeafItems < tree->itemCount
      && AabbPartitionItems( tree->itemCount, builder, indices, &frontCount, &midStart ) )
    {
        children = &builder->nodes[aabbTreeCount];

        SanityCheck( tree->firstChild == aabbTreeCount );

        AabbBuildSubtree( tree, builder, indices, 0, frontCount );
        if ( frontCount < midStart )
            AabbBuildSubtree( tree, builder, indices, frontCount, midStart - frontCount );
        AabbBuildSubtree( tree, builder, indices, midStart, tree->itemCount - midStart );

        tree->childCount = aabbTreeCount - tree->firstChild;

        for ( i = 0; i < tree->childCount; i++ )
            AabbBuildTree_r( &children[i], builder,
                             indices + children[i].firstItem - tree->firstItem );
    }

    return tree->childCount;
}


/* AabbBuildTree  0x00465730 */
int AabbBuildTree( AabbTreeBuilder_t *builder )
{
    int    indexBufLocal[AABB_STACK_ITEMS];
    float  sortMinsLocal[AABB_STACK_ITEMS];
    float  sortMaxsLocal[AABB_STACK_ITEMS];
    float  sortEqualLocal[AABB_STACK_ITEMS];
    int   *indexBuf;
    char  *itemDataCopy;
    vec3_t *boundsCopy;
    int    i;

    if ( builder->itemCount <= ARRAY_COUNT( indexBufLocal ) )
    {
        indexBuf        = indexBufLocal;
        aabbSortMins  = sortMinsLocal;
        aabbSortMaxs  = sortMaxsLocal;
        aabbSortEqual = sortEqualLocal;
    }
    else
    {
        indexBuf        = new int[builder->itemCount];
        aabbSortMins  = new float[builder->itemCount];
        aabbSortMaxs  = new float[builder->itemCount];
        aabbSortEqual = new float[builder->itemCount];
    }

    for ( i = 0; i < builder->itemCount; i++ )
        indexBuf[i] = i;

    builder->nodes[0].firstItem = 0;
    builder->nodes[0].itemCount = builder->itemCount;
    aabbTreeCount = 1;

    AabbBuildTree_r( builder->nodes, builder, indexBuf );

    itemDataCopy = new char[builder->itemCount * builder->itemStride];
    memcpy( itemDataCopy, builder->itemData, builder->itemCount * builder->itemStride );

    for ( i = 0; i < builder->itemCount; i++ )
    {
        memcpy( (char *)builder->itemData + i * builder->itemStride,
                itemDataCopy + indexBuf[i] * builder->itemStride,
                builder->itemStride );
    }
    delete[] itemDataCopy;

    if ( builder->reorderBounds )
    {
        boundsCopy = new vec3_t[builder->itemCount];

        memcpy( boundsCopy, builder->itemMins, builder->itemCount * sizeof( vec3_t ) );
        for ( i = 0; i < builder->itemCount; i++ )
            Vec3Copy( boundsCopy[indexBuf[i]], builder->itemMins[i] );

        memcpy( boundsCopy, builder->itemMaxs, builder->itemCount * sizeof( vec3_t ) );
        for ( i = 0; i < builder->itemCount; i++ )
            Vec3Copy( boundsCopy[indexBuf[i]], builder->itemMaxs[i] );

        delete[] boundsCopy;
    }

    if ( indexBuf != indexBufLocal )
    {
        delete[] indexBuf;
        delete[] aabbSortMins;
        delete[] aabbSortMaxs;
        delete[] aabbSortEqual;
    }

    return aabbTreeCount;
}

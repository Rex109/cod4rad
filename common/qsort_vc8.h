#ifndef QSORT_VC8_H
#define QSORT_VC8_H


#include <stddef.h>

#define QSORT_VC8_CUTOFF 8
#define QSORT_VC8_STKSIZ ( 8 * sizeof( void * ) - 2 )

typedef int ( __cdecl *qsortVc8Compare_t )( const void *, const void * );

static void QsortVc8Swap( char *a, char *b, size_t width )
{
    char tmp;

    if ( a != b )
    {
        while ( width-- )
        {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}

static void QsortVc8ShortSort( char *lo, char *hi, size_t width, qsortVc8Compare_t comp )
{
    char *p;
    char *max;

    while ( hi > lo )
    {
        max = lo;
        for ( p = lo + width; p <= hi; p += width )
        {
            if ( comp( p, max ) > 0 )
                max = p;
        }
        QsortVc8Swap( max, hi, width );
        hi -= width;
    }
}

static void qsort_vc8( void *base, size_t num, size_t width, qsortVc8Compare_t comp )
{
    char  *lo;
    char  *hi;
    char  *mid;
    char  *loguy;
    char  *higuy;
    size_t size;
    char  *lostk[ 8 * sizeof( void * ) - 2 ];
    char  *histk[ 8 * sizeof( void * ) - 2 ];
    int    stkptr;

    if ( num < 2 )
        return;

    stkptr = 0;
    lo = ( char * )base;
    hi = ( char * )base + width * ( num - 1 );

recurse:
    size = ( size_t )( hi - lo ) / width + 1;

    if ( size <= QSORT_VC8_CUTOFF )
    {
        QsortVc8ShortSort( lo, hi, width, comp );
    }
    else
    {
        mid = lo + ( size / 2 ) * width;

        if ( comp( lo,  mid ) > 0 ) QsortVc8Swap( lo,  mid, width );
        if ( comp( lo,  hi  ) > 0 ) QsortVc8Swap( lo,  hi,  width );
        if ( comp( mid, hi  ) > 0 ) QsortVc8Swap( mid, hi,  width );

        loguy = lo;
        higuy = hi;

        for ( ;; )
        {
            if ( mid > loguy )
            {
                do { loguy += width; } while ( loguy < mid && comp( loguy, mid ) <= 0 );
            }
            if ( mid <= loguy )
            {
                do { loguy += width; } while ( loguy <= hi && comp( loguy, mid ) <= 0 );
            }

            do { higuy -= width; } while ( higuy > mid && comp( higuy, mid ) > 0 );

            if ( higuy < loguy )
                break;

            QsortVc8Swap( loguy, higuy, width );

            if ( mid == higuy )
                mid = loguy;
        }

        higuy += width;

        if ( mid < higuy )
        {
            do { higuy -= width; } while ( higuy > mid && comp( higuy, mid ) == 0 );
        }
        if ( mid >= higuy )
        {
            do { higuy -= width; } while ( higuy > lo && comp( higuy, mid ) == 0 );
        }

        if ( higuy - lo >= hi - loguy )
        {
            if ( lo < higuy )
            {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy;
                ++stkptr;
            }
            if ( loguy < hi )
            {
                lo = loguy;
                goto recurse;
            }
        }
        else
        {
            if ( loguy < hi )
            {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;
            }
            if ( lo < higuy )
            {
                hi = higuy;
                goto recurse;
            }
        }
    }

    --stkptr;
    if ( stkptr >= 0 )
    {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;
    }
}

#endif

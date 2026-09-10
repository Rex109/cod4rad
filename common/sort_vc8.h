
#ifndef SORT_VC8_H
#define SORT_VC8_H

#include <stddef.h>

#define VC8_ISORT_MAX   32

namespace vc8_detail
{

    template< class Ty >
    inline void IterSwap( Ty *a, Ty *b )
    {
        Ty tmp = *a;
        *a = *b;
        *b = tmp;
    }

    template< class Ty >
    inline void RotateLastToFront( Ty *first, Ty *mid )
    {
        Ty  val;
        Ty *p;

        if ( first == mid )
            return;

        val = *mid;
        for ( p = mid; p != first; p-- )
            *p = *( p - 1 );
        *first = val;
    }

    template< class Ty, class Pr >
    void Med3( Ty *first, Ty *mid, Ty *last, Pr pred )
    {
        if ( pred( *mid, *first ) )
            IterSwap( mid, first );
        if ( pred( *last, *mid ) )
            IterSwap( last, mid );
        if ( pred( *mid, *first ) )
            IterSwap( mid, first );
    }

    template< class Ty, class Pr >
    void Median( Ty *first, Ty *mid, Ty *last, Pr pred )
    {
        ptrdiff_t step;

        if ( 40 < last - first )
        {
            step = ( last - first + 1 ) / 8;

            Med3( first, first + step, first + 2 * step, pred );
            Med3( mid - step, mid, mid + step, pred );
            Med3( last - 2 * step, last - step, last, pred );
            Med3( first + step, mid, last - step, pred );
        }
        else
        {
            Med3( first, mid, last, pred );
        }
    }

    template< class Ty, class Pr >
    void UnguardedPartition( Ty *first, Ty *last, Pr pred, Ty **outPfirst, Ty **outPlast )
    {
        Ty *mid;
        Ty *pfirst;
        Ty *plast;
        Ty *gfirst;
        Ty *glast;

        mid = first + ( last - first ) / 2;
        Median( first, mid, last - 1, pred );

        pfirst = mid;
        plast  = pfirst + 1;

        while ( first < pfirst
                && !pred( *( pfirst - 1 ), *pfirst )
                && !pred( *pfirst, *( pfirst - 1 ) ) )
        {
            pfirst--;
        }

        while ( plast < last
                && !pred( *plast, *pfirst )
                && !pred( *pfirst, *plast ) )
        {
            plast++;
        }

        gfirst = plast;
        glast  = pfirst;

        for ( ;; )
        {
            for ( ; gfirst < last; gfirst++ )
            {
                if ( pred( *pfirst, *gfirst ) )
                {
                }
                else if ( pred( *gfirst, *pfirst ) )
                {
                    break;
                }
                else
                {
                    IterSwap( plast, gfirst );
                    plast++;
                }
            }

            for ( ; first < glast; glast-- )
            {
                if ( pred( *( glast - 1 ), *pfirst ) )
                {
                }
                else if ( pred( *pfirst, *( glast - 1 ) ) )
                {
                    break;
                }
                else
                {
                    pfirst--;
                    IterSwap( pfirst, glast - 1 );
                }
            }

            if ( glast == first && gfirst == last )
            {
                *outPfirst = pfirst;
                *outPlast  = plast;
                return;
            }

            if ( glast == first )
            {
                if ( plast != gfirst )
                    IterSwap( pfirst, plast );
                plast++;
                IterSwap( pfirst, gfirst );
                pfirst++;
                gfirst++;
            }
            else if ( gfirst == last )
            {
                glast--;
                pfirst--;
                if ( glast != pfirst )
                    IterSwap( glast, pfirst );
                plast--;
                IterSwap( pfirst, plast );
            }
            else
            {
                glast--;
                IterSwap( gfirst, glast );
                gfirst++;
            }
        }
    }

    template< class Ty, class Pr >
    void InsertionSort( Ty *first, Ty *last, Pr pred )
    {
        Ty *next;
        Ty *next1;
        Ty *first1;

        if ( first == last )
            return;

        for ( next = first; ++next != last; )
        {
            next1 = next;

            if ( pred( *next, *first ) )
            {
                RotateLastToFront( first, next );
            }
            else
            {
                first1 = next1;
                while ( pred( *next, *( --first1 ) ) )
                    next1 = first1;

                RotateLastToFront( next1, next );
            }
        }
    }

    template< class Ty, class Pr >
    void PushHeap( Ty *first, ptrdiff_t hole, ptrdiff_t top, Ty val, Pr pred )
    {
        ptrdiff_t idx;

        for ( idx = ( hole - 1 ) / 2;
              top < hole && pred( *( first + idx ), val );
              idx = ( hole - 1 ) / 2 )
        {
            *( first + hole ) = *( first + idx );
            hole = idx;
        }

        *( first + hole ) = val;
    }

    template< class Ty, class Pr >
    void AdjustHeap( Ty *first, ptrdiff_t hole, ptrdiff_t bottom, Ty val, Pr pred )
    {
        ptrdiff_t top;
        ptrdiff_t idx;

        top = hole;
        idx = 2 * hole + 2;

        for ( ; idx < bottom; idx = 2 * idx + 2 )
        {
            if ( pred( *( first + idx ), *( first + ( idx - 1 ) ) ) )
                idx--;
            *( first + hole ) = *( first + idx );
            hole = idx;
        }

        if ( idx == bottom )
        {
            *( first + hole ) = *( first + ( bottom - 1 ) );
            hole = bottom - 1;
        }

        PushHeap( first, hole, top, val, pred );
    }

    template< class Ty, class Pr >
    void MakeHeap( Ty *first, Ty *last, Pr pred )
    {
        ptrdiff_t bottom;
        ptrdiff_t hole;
        Ty        val;

        bottom = last - first;

        for ( hole = bottom / 2; 0 < hole; )
        {
            hole--;
            val = *( first + hole );
            AdjustHeap( first, hole, bottom, val, pred );
        }
    }

    template< class Ty, class Pr >
    void PopHeap( Ty *first, Ty *last, Pr pred )
    {
        Ty *dest;
        Ty  val;

        if ( last - first <= 1 )
            return;

        dest = last - 1;
        val  = *dest;
        *dest = *first;
        AdjustHeap( first, (ptrdiff_t)0, (ptrdiff_t)( dest - first ), val, pred );
    }

    template< class Ty, class Pr >
    void SortHeap( Ty *first, Ty *last, Pr pred )
    {
        for ( ; 1 < last - first; last-- )
            PopHeap( first, last, pred );
    }

    template< class Ty, class Pr >
    void Sort( Ty *first, Ty *last, ptrdiff_t ideal, Pr pred )
    {
        ptrdiff_t count;
        Ty       *pfirst;
        Ty       *plast;

        for ( ;; )
        {
            count = last - first;
            if ( !( VC8_ISORT_MAX < count && 0 < ideal ) )
                break;

            UnguardedPartition( first, last, pred, &pfirst, &plast );

            ideal /= 2;
            ideal += ideal / 2;

            if ( pfirst - first < last - plast )
            {
                Sort( first, pfirst, ideal, pred );
                first = plast;
            }
            else
            {
                Sort( plast, last, ideal, pred );
                last = pfirst;
            }
        }

        if ( VC8_ISORT_MAX < count )
        {
            MakeHeap( first, last, pred );
            SortHeap( first, last, pred );
        }
        else if ( 1 < count )
        {
            InsertionSort( first, last, pred );
        }
    }

}

template< class Ty, class Pr >
inline void Sort_VC8( Ty *first, Ty *last, Pr pred )
{
    vc8_detail::Sort( first, last, (ptrdiff_t)( last - first ), pred );
}

#endif

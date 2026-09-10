/* Original: c:\trees\cod3\cod3src\common\threads.cpp */

#include "cod4rad.h"
#include "threads.h"
#include "progress.h"

#include <windows.h>
#include <string.h>


int              threadCount;                       /* 0x130cc218 */
volatile long    lockSlots[THREAD_LOCK_SLOTS];      /* 0x130cc21c */
ThreadWorkFunc_t threadWorkFunc;                    /* 0x130cd210 */
volatile long    threadWorkNext;                    /* 0x130cd214 */
int              threadWorkCount;                   /* 0x130cd218 */


/* RunSingleThread  0x0041fe60 */
static void RunSingleThread( ThreadWorkFunc_t func )
{
    int i;

    for ( i = 0; i < threadWorkCount; i++ )
    {
        func( i, 0 );
        AddProgress( 1 );
    }
}

/* ThreadProc  0x0041fd80 */
static DWORD WINAPI ThreadProc( LPVOID param )
{
    int threadIndex = ( int )( size_t )param;
    int workIndex;

    workIndex = InterlockedExchangeAdd( &threadWorkNext, 1 );

    while ( workIndex < threadWorkCount )
    {
        threadWorkFunc( workIndex, threadIndex );

        if ( !threadIndex )
            SetProgress( workIndex + 1, threadWorkCount );

        workIndex = InterlockedExchangeAdd( &threadWorkNext, 1 );
    }

    return 0;
}

/* RunThreads  0x0041fde0 */
static void RunThreads( ThreadWorkFunc_t func )
{
    HANDLE handles[THREAD_COUNT_MAX];
    DWORD  threadIds[THREAD_COUNT_MAX];
    int    i;

    memset( ( void * )lockSlots, 0, sizeof( lockSlots ) );

    threadWorkNext = 0;
    threadWorkFunc = func;

    for ( i = 0; i < threadCount; i++ )
        handles[i] = CreateThread( NULL, 0, ThreadProc, ( LPVOID )( size_t )i, 0,
                                   &threadIds[i] );

    WaitForMultipleObjects( threadCount, handles, TRUE, INFINITE );
}

/* RunThreadsOn  0x0041fe90 */
void RunThreadsOn( int workCount, ThreadWorkFunc_t func, int threads )
{
    threadWorkCount = workCount;

    SetProgress( 0, workCount );

    threadCount = threads;

    if ( threads == 1 )
    {
        RunSingleThread( func );
        return;
    }

    RunThreads( func );
}

/* Lock  0x0041fef0 */
void Lock( const void *object )
{
    volatile long *slot;

    if ( threadCount == 1 )
        return;

    slot = &lockSlots[( unsigned )( size_t )object % THREAD_LOCK_SLOTS];

    if ( InterlockedCompareExchange( slot, 1, 0 ) )
    {
        do
        {
            Sleep( 0 );
        }
        while ( InterlockedCompareExchange( slot, 1, 0 ) );
    }
}

/* Unlock  0x0041ff60 */
void Unlock( const void *object )
{
    if ( threadCount == 1 )
        return;

    lockSlots[( unsigned )( size_t )object % THREAD_LOCK_SLOTS] = 0;
}

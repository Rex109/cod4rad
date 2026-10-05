/* Original: c:\trees\cod3\cod3src\common\threads.cpp */

#ifndef THREADS_H
#define THREADS_H

#include "q_shared.h"


#define THREAD_COUNT_MAX    16

#define THREAD_LOCK_SLOTS   1021


typedef void ( *ThreadWorkFunc_t )( int workIndex, int threadIndex );


extern int              threadCount;      /* 0x130cc218 */
extern volatile long    lockSlots[THREAD_LOCK_SLOTS];     /* 0x130cc21c */
extern ThreadWorkFunc_t threadWorkFunc;   /* 0x130cd210 */
extern volatile long    threadWorkNext;   /* 0x130cd214 */
extern int              threadWorkCount;  /* 0x130cd218 */


void RunThreadsOn( int workCount, ThreadWorkFunc_t func, int threads );  /* 0x0041fe90 */

void Lock( const void *object );        /* 0x0041fef0 */
void Unlock( const void *object );      /* 0x0041ff60 */

#endif

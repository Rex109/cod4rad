/* Original: c:\trees\cod3\cod3src\cod2rad\progress.cpp */

#include "cod4rad.h"
#include "progress.h"
#include "cmdline.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>


#define PROGRESS_FIRST_DELAY    2000
#define PROGRESS_REPEAT_DELAY   500


static int progressStartTime;       /* 0x130cc1fc */
static int progressCompleted;       /* 0x130cc200 */
static int progressTotal;           /* 0x130cc204 */
static int progressNextTime;        /* 0x130cc208 */
static int progressLastTenths;      /* 0x130cc20c */


/* VerbosePrint  0x0041f8f0 */
void VerbosePrint( const char *fmt, ... )
{
    va_list argptr;

    if ( !options.verbose )
        return;

    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );

    fflush( stdout );
}

/* Print  0x0041f920 */
void Print( const char *fmt, ... )
{
    va_list argptr;

    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );

    fflush( stdout );
}

/* Warning  0x0041f950 */
void Warning( int level, const char *fmt, ... )
{
    va_list argptr;

    if ( level > options.warningLevel )
        return;

    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );

    fflush( stdout );
}

/* ErrorV  0x0041f980 */
void ErrorV( const char *fmt, va_list argptr )
{
    printf( "\n" );
    vprintf( fmt, argptr );

    if ( fmt[strlen( fmt ) - 1] != '\n' )
        printf( "\n" );

    fflush( stdout );
    exit( -1 );
}

/* Error  0x0041f9e0 */
void Error( const char *fmt, ... )
{
    va_list argptr;

    va_start( argptr, fmt );
    ErrorV( fmt, argptr );
}

/* PrintDuration  0x0041fa00 */
void PrintDuration( int milliseconds )
{
    int totalSeconds;
    int hours;
    int minutes;
    int seconds;

    totalSeconds = ( milliseconds + 500 ) / 1000;

    hours   = totalSeconds / 3600;
    seconds = totalSeconds - hours * 3600;
    minutes = seconds / 60;
    seconds = seconds - minutes * 60;

    if ( hours )
        Print( "%i:%02i:%02i", hours, minutes, seconds );
    else if ( minutes )
        Print( "%i:%02i", minutes, seconds );
    else
        Print( "%i seconds", seconds );
}

/* UpdateProgress  0x0041faa0 */
static void UpdateProgress( void )
{
    float percent;
    int   tenths;
    int   elapsed;

    if ( options.quiet )
        return;

    if ( !progressTotal )
        return;

    percent = ( float )( progressCompleted * 100.0 / progressTotal );

    tenths = RoundFloatToInt( ( float )( percent * 10.0 ) );
    if ( tenths == progressLastTenths )
        return;

    progressLastTenths = tenths;

    elapsed = ( int )timeGetTime() - progressStartTime;
    if ( elapsed < progressNextTime )
        return;

    progressNextTime = elapsed + PROGRESS_REPEAT_DELAY;

    Print( "%i.%i%% complete", tenths / 10, tenths % 10 );
    Print( ", " );
    PrintDuration( elapsed );
    Print( " done, " );
    PrintDuration( ( int )( elapsed / percent * ( 100.0 - percent ) ) );
    Print( " remaining               " );
    Print( "\r" );
}

/* StartProgress  0x0041fbb0 */
void StartProgress( const char *name )
{
    progressCompleted  = 0;
    progressTotal      = 1;
    progressStartTime  = ( int )timeGetTime();
    progressNextTime   = PROGRESS_FIRST_DELAY;
    progressLastTenths = 0;

    Print( "----------------------------------------\n%s\n", name );

    UpdateProgress();
}

/* SetProgress  0x0041fc00 */
void SetProgress( int completed, int total )
{
    progressCompleted = completed;
    progressTotal     = total;

    UpdateProgress();
}

/* AddProgress  0x0041fc20 */
void AddProgress( int count )
{
    if ( !progressTotal )
        return;

    progressCompleted += count;

    UpdateProgress();
}

/* EndProgress  0x0041fc40 */
void EndProgress( void )
{
    int elapsed;

    elapsed = ( int )timeGetTime() - progressStartTime;

    Print( "Finished in " );
    PrintDuration( elapsed );
    Print( ".                                                 \n" );

    progressCompleted = 0;
    progressTotal     = 0;
}

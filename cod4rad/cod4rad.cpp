/* Original: c:\trees\cod3\cod3src\cod2rad\cod2rad.cpp */

#include "cod4rad.h"
#include "cmdline.h"
#include "progress.h"
#include "mapio.h"
#include "compile.h"
#include "crashlog.h"

#include "scr_stringlist.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <new>
#include <windows.h>
#include <mmsystem.h>


/* Com_Error  0x00406300 */
void Com_Error( int level, const char *fmt, ... )
{
    char    buf[0x10000];
    va_list argptr;

    va_start( argptr, fmt );
    _vsnprintf( buf, sizeof( buf ), fmt, argptr );
    va_end( argptr );

    Error( "%s", buf );
}

/* main  0x004063a0 */
int main( int argc, const char **argv )
{
    int startTime;
    int elapsed;
    int hours;
    int minutes;
    int seconds;

    CrashLog_Install();

    startTime = ( int )timeGetTime();

    try
    {
        Com_SetErrorHandler( ErrorV );

        if ( !ParseCommandLine( argc, argv ) )
            return 1;

        SL_Init();

        if ( !LoadMapFile( options.mapName ) )
            return 1;

        RunLightCompile( options.threadCount );

        WriteMapFile( options.mapName );

        elapsed = ( ( int )timeGetTime() - startTime + 500 ) / 1000;

        hours   = elapsed / 3600;
        minutes = ( elapsed - hours * 3600 ) / 60;
        seconds = elapsed - ( elapsed / 60 ) * 60;

        printf( "\nEntire light compile finished in " );

        if ( hours )
            printf( "%i hours ", hours );

        if ( minutes )
            printf( "%i minutes ", minutes );

        printf( "%i seconds\n", seconds );

        return 0;
    }
    catch ( std::bad_alloc & )
    {
        Error( "Unhandled out of memory error" );
    }

    return 1;
}

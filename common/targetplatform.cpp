/* Original: ..\common\targetplatform.cpp */

#include "cod4rad.h"


#define SRCFILE "..\\common\\targetplatform.cpp"

extern void AssertFailed(const char *file, int line, int fatal, const char *fmt, ...);  /* 0x00402180 */

#ifndef Assert
#define Assert(exp) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s", #exp); } while (0)
#endif

#ifndef Assertx
#define Assertx(exp, fmt, ...) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s\n\t" fmt, #exp, __VA_ARGS__); } while (0)
#endif


const char *g_platformNames[PLATFORM_COUNT] =       /* 0x00529620 */
{
    NULL,
    "pc",
    "xenon",
    "ps3"
};

platform_t  targetPlatform;                         /* 0x14899cc4 */

/* SetTargetPlatformByName  0x0043b2b0 */
qboolean SetTargetPlatformByName( const char *name )
{
    int i;

    Assert( name );

    for ( i = 0; i < PLATFORM_COUNT; i++ )
    {
        if ( g_platformNames[i] && !_stricmp( name, g_platformNames[i] ) )
        {
            if ( targetPlatform && targetPlatform != i )
            {
                Com_Error( "Error: Target platform already set as %s, trying to set again as %s\n",
                           g_platformNames[targetPlatform],
                           g_platformNames[i] );
                return qfalse;
            }

            targetPlatform = (platform_t)i;
            return qtrue;
        }
    }

    return qfalse;
}

/* PrintValidPlatforms  0x0043b380 */
void PrintValidPlatforms( void )
{
    int i;

    for ( i = 0; i < PLATFORM_COUNT; i++ )
    {
        if ( g_platformNames[i] )
            printf( "  %s\n", g_platformNames[i] );
    }
}

/* ValidatePlatformSet  0x0043b3d0 */
qboolean ValidatePlatformSet( void )
{
    if ( targetPlatform )
        return qtrue;

    printf( "No platform specified.  '-platform' must be set using one of the following:\n" );
    PrintValidPlatforms();
    return qfalse;
}

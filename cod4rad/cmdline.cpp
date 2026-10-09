/* Original: c:\trees\cod3\cod3src\cod2rad\cmdline.cpp */

#include "cod4rad.h"
#include "cmdline.h"
#include "progress.h"
#include "gputransport.h"
#include "lightgrid.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


RadOptions_t options;                       /* 0x11622e48 */


typedef int ( *OptionHandler_t )( int argc, const char **argv );

typedef struct
{
    const char     *name;           /* +0x00 */
    const char     *description;    /* +0x04 */
    OptionHandler_t handler;        /* +0x08 */
} RadOption_t;


/* GetDefaultThreadCount  0x00405700 */
static int GetDefaultThreadCount( void )
{
    SYSTEM_INFO systemInfo;

    GetSystemInfo( &systemInfo );

    if ( ( int )systemInfo.dwNumberOfProcessors < RAD_THREAD_COUNT_MIN )
        return RAD_THREAD_COUNT_MIN;

    if ( ( int )systemInfo.dwNumberOfProcessors > RAD_THREAD_COUNT_DEFAULT_MAX )
        return RAD_THREAD_COUNT_DEFAULT_MAX;

    return ( int )systemInfo.dwNumberOfProcessors;
}

static bool threadCountSet;

/* The radiosity trace is the part the GPU takes over, which leaves the CPU side
   (lightmap lookups and bounce bookkeeping) as the bottleneck.  Use every core
   for it unless the user chose a thread count. */
static void ApplyGpuThreadDefault( void )
{
    SYSTEM_INFO systemInfo;
    int         count;

    if ( !gpuTransportRequested || threadCountSet )
        return;

    GetSystemInfo( &systemInfo );

    count = ( int )systemInfo.dwNumberOfProcessors;

    if ( count > RAD_THREAD_COUNT_MAX )
        count = RAD_THREAD_COUNT_MAX;

    if ( count > options.threadCount )
        options.threadCount = count;
}

/* SetDefaultOptions  0x00405730 */
static void SetDefaultOptions( void )
{
    options.threadCount             = GetDefaultThreadCount();
    options.modelShadow             = 1;
    options.extraQuality            = 0;
    options.relight                 = 1;
    options.relightSave             = 1;
    options.supersampleAlphaCount   = 4;
    options.supersampleCount        = 2;
    options.jitter                  = 0.75f;
    options.radiosityTraceCount     = 32;
    options.skyTraceCount           = 64;
    options.traceFilterWidth        = TRACE_FILTER_POINT;
    options.basisDirCount           = 32;
    options.lightGridColorLimit     = 0xffff;
    options.lightGridColorTolerance = 3.5f;

    options.sunDirection[0] = 0.4418349862098694f;
    options.sunDirection[1] = 0.5680699944496155f;
    options.sunDirection[2] = 0.6943129897117615f;

    Vec3Set( options.sunColor,          0.7f, 0.7f, 0.7f );
    Vec3Set( options.sunDiffuseColor,   0.7f, 0.7f, 0.7f );
    Vec3Set( options.sunRadiosityColor, 0.3f, 0.3f, 0.3f );

    options.maxBounceCount   = 32;
    options.radiosityScale   = 1.0f;
    options.gamma            = 2.2f;
    options.contrastGain     = 0.3f;

    options.radiosityScaleSet = 0;
    options.contrastGainSet   = 0;
    options.verbose           = 0;
    options.quiet             = 0;
    options.warningLevel      = RAD_WARNING_LEVEL_MAX;

    strcpy( options.baseGame, "main" );
}

/* SetFastOptions  0x00405890 */
static void SetFastOptions( void )
{
    options.modelShadow         = 0;
    options.extraQuality        = 0;
    options.supersampleCount    = 1;
    options.radiosityTraceCount = 16;
    options.skyTraceCount       = 32;
    options.basisDirCount       = 16;
}

/* SetExtraOptions  0x004058c0 */
static void SetExtraOptions( void )
{
    options.modelShadow         = 1;
    options.extraQuality        = 1;
    options.supersampleCount    = 4;
    options.radiosityTraceCount = 48;
    options.skyTraceCount       = 96;
    options.basisDirCount       = 64;
}

/* GetIntOption  0x00405900 */
static int GetIntOption( int argc, const char **argv, int *value, int min, int max )
{
    Assert( argc >= 1 );
    Assert( min < max );

    if ( argc < 2 )
        Error( "%s: missing argument\n", argv[0] );

    *value = atoi( argv[1] );

    if ( *value < min )
    {
        Warning( 1, "%s: clamping to %i\n", argv[0], min );
        *value = min;
    }
    else if ( *value > max )
    {
        Warning( 1, "%s: clamping to %i\n", argv[0], max );
        *value = max;
    }

    return 2;
}

/* GetFloatOption  0x004059b0 */
static int GetFloatOption( int argc, const char **argv, float *value, float min, float max )
{
    Assert( argc >= 1 );
    Assert( min < max );

    if ( argc < 2 )
        Error( "%s: missing argument\n", argv[0] );

    *value = ( float )atof( argv[1] );

    if ( *value < min )
    {
        Warning( 1, "%s: clamping to %g\n", argv[0], min );
        *value = min;
    }
    else if ( *value > max )
    {
        Warning( 1, "%s: clamping to %g\n", argv[0], max );
        *value = max;
    }

    return 2;
}


/* OptVerbose  0x00405ab0 */
static int OptVerbose( int argc, const char **argv )
{
    options.verbose = 1;
    return 1;
}

/* OptQuiet  0x00405ac0 */
static int OptQuiet( int argc, const char **argv )
{
    options.quiet = 1;
    return 1;
}

/* OptWarn  0x00405ad0 */
static int OptWarn( int argc, const char **argv )
{
    return GetIntOption( argc, argv, &options.warningLevel,
                         RAD_WARNING_LEVEL_MIN, RAD_WARNING_LEVEL_MAX );
}

/* OptPlatform  0x00405b00 */
static int OptPlatform( int argc, const char **argv )
{
    SetTargetPlatformByName( argv[1] );
    Assert( targetPlatform != PLATFORM_VOID );
    return 2;
}

/* OptFast  0x00405b40 */
static int OptFast( int argc, const char **argv )
{
    SetFastOptions();
    return 1;
}

/* OptExtra  0x00405b80 */
static int OptExtra( int argc, const char **argv )
{
    SetExtraOptions();
    return 1;
}

/* OptModelShadow  0x00405bc0 */
static int OptModelShadow( int argc, const char **argv )
{
    options.modelShadow = 1;
    return 1;
}

/* OptNoModelShadow  0x00405bd0 */
static int OptNoModelShadow( int argc, const char **argv )
{
    options.modelShadow = 0;
    return 1;
}

/* OptGridMaxDistance: the filter is off unless this is given.  The distance is optional:
   "-GridMaxDistance" alone uses 2048 units, "-GridMaxDistance N" uses N. */
static int OptGridMaxDistance( int argc, const char **argv )
{
    char *end;
    float value;

    /* The last argument is always the map name, so a number has to come before it */
    if ( argc >= 3 )
    {
        value = ( float )strtod( argv[1], &end );

        if ( end != argv[1] && *end == '\0' )
        {
            if ( value < 0.0f )
                value = 0.0f;

            lightGridMaxDistance = value;
            return 2;
        }
    }

    lightGridMaxDistance = LIGHTGRID_DEFAULT_MAX_DISTANCE;
    return 1;
}

/* OptGpu */
static int OptGpu( int argc, const char **argv )
{
    gpuTransportRequested = true;
    return 1;
}

/* OptTraces  0x00405be0 */
static int OptTraces( int argc, const char **argv )
{
    int used;

    used = GetIntOption( argc, argv, &options.radiosityTraceCount,
                         RAD_TRACE_COUNT_MIN, RAD_TRACE_COUNT_MAX );

    options.skyTraceCount = options.radiosityTraceCount * 2;

    return used;
}

/* OptJitter  0x00405c20 */
static int OptJitter( int argc, const char **argv )
{
    return GetFloatOption( argc, argv, &options.jitter,
                           RAD_JITTER_MIN, RAD_JITTER_MAX );
}

/* OptSuperSampleAlpha  0x00405c50 */
static int OptSuperSampleAlpha( int argc, const char **argv )
{
    return GetIntOption( argc, argv, &options.supersampleAlphaCount,
                         RAD_SUPERSAMPLE_ALPHA_MIN, RAD_SUPERSAMPLE_ALPHA_MAX );
}

/* OptSuperSample  0x00405c80 */
static int OptSuperSample( int argc, const char **argv )
{
    return GetIntOption( argc, argv, &options.supersampleCount,
                         RAD_SUPERSAMPLE_MIN, RAD_SUPERSAMPLE_MAX );
}

/* OptTraceFilterLinear  0x00405cb0 */
static int OptTraceFilterLinear( int argc, const char **argv )
{
    options.traceFilterWidth = TRACE_FILTER_LINEAR;
    return 1;
}

/* OptTraceFilterPoint  0x00405cc0 */
static int OptTraceFilterPoint( int argc, const char **argv )
{
    options.traceFilterWidth = TRACE_FILTER_POINT;
    return 1;
}

/* OptMaxBounces  0x00405cd0 */
static int OptMaxBounces( int argc, const char **argv )
{
    return GetIntOption( argc, argv, &options.maxBounceCount,
                         RAD_BOUNCE_COUNT_MIN, RAD_BOUNCE_COUNT_MAX );
}

/* OptRadiosityScale  0x00405d00 */
static int OptRadiosityScale( int argc, const char **argv )
{
    options.radiosityScaleSet = 1;

    return GetFloatOption( argc, argv, &options.radiosityScale,
                           RAD_RADIOSITY_SCALE_MIN, RAD_RADIOSITY_SCALE_MAX );
}

/* OptGamma  0x00405d40 */
static int OptGamma( int argc, const char **argv )
{
    return GetFloatOption( argc, argv, &options.gamma,
                           RAD_GAMMA_MIN, RAD_GAMMA_MAX );
}

/* OptContrastGain  0x00405d80 */
static int OptContrastGain( int argc, const char **argv )
{
    options.contrastGainSet = 1;

    return GetFloatOption( argc, argv, &options.contrastGain,
                           RAD_CONTRAST_GAIN_MIN, RAD_CONTRAST_GAIN_MAX );
}

/* OptNoRelight  0x00405dc0 */
static int OptNoRelight( int argc, const char **argv )
{
    options.relight     = 0;
    options.relightSave = 0;
    return 1;
}

/* OptBasisDirCount  0x00405de0 */
static int OptBasisDirCount( int argc, const char **argv )
{
    return GetIntOption( argc, argv, &options.basisDirCount,
                         RAD_BASIS_DIR_COUNT_MIN, RAD_BASIS_DIR_COUNT_MAX );
}

/* OptThreads  0x00405e10 */
static int OptThreads( int argc, const char **argv )
{
    threadCountSet = true;

    return GetIntOption( argc, argv, &options.threadCount,
                         RAD_THREAD_COUNT_MIN, RAD_THREAD_COUNT_MAX );
}

/* DumpBool  0x00405e40 */
static void DumpBool( const char *name, bool value )
{
    Print( "%-30s %s\n", name, value ? "enabled" : "disabled" );
}

/* DumpInt  0x00405e70 */
static void DumpInt( const char *name, int value )
{
    Print( "%-30s %i\n", name, value );
}

/* DumpFloat  0x00405e80 */
static void DumpFloat( const char *name, float value )
{
    Print( "%-30s %g\n", name, value );
}

/* OptDumpOptions  0x00405ea0 */
static int OptDumpOptions( int argc, const char **argv )
{
    DumpInt(   "number of threads:",              options.threadCount );
    DumpFloat( "radiosity scale:",                options.radiosityScale );
    DumpFloat( "contrast gain:",                  options.contrastGain );
    DumpFloat( "gamma:",                          options.gamma );
    DumpInt(   "max bounces:",                    options.maxBounceCount );
    DumpInt(   "traces:",                         options.radiosityTraceCount );
    DumpFloat( "jitter:",                         options.jitter );
    DumpInt(   "alpha super sampling:",           options.supersampleAlphaCount );
    DumpInt(   "lightmap super sampling:",        options.supersampleCount );
    DumpBool(  "linear filter radiosity traces:", options.traceFilterWidth == TRACE_FILTER_LINEAR );
    DumpBool(  "model shadows:",                  options.modelShadow != 0 );
    DumpInt(   "basis direction count:",          options.basisDirCount );
    DumpBool(  "verbose messages:",               options.verbose != 0 );
    DumpBool(  "gpu radiosity traces:",           gpuTransportRequested );

    return 1;
}


static const RadOption_t radOptions[] =                     /* 0x004742f8 */
{
    { "-Verbose",           "Turns on verbose prints",                                  OptVerbose           },
    { "-Quiet",             "Turns off progress counters (can be used with -Verbose)",  OptQuiet             },
    { "-Warn",              "Sets the warning level",                                   OptWarn              },
    { "-Platform",          "Target platform (pc, etc)",                                OptPlatform          },
    { "-Fast",              "Use fast presets for several options",                     OptFast              },
    { "-Extra",             "Use high-quality presets for several options",             OptExtra             },
    { "-ModelShadow",       "Allows model surfaces to cast shadows",                    OptModelShadow       },
    { "-NoModelShadow",     "Prevents model surfaces from casting shadows",             OptNoModelShadow     },
    { "-Traces",            "Number of traces to do from each sample point",            OptTraces            },
    { "-Jitter",            "Breaks up aliasing from trace pattern (0 none, 1 max)",    OptJitter            },
    { "-SuperSampleAlpha",  "Does N lookups to antialias each alpha mask test",         OptSuperSampleAlpha  },
    { "-SuperSample",       "Turns each sample into NxN samples instead",               OptSuperSample       },
    { "-MaxBounces",        "Stops radiosity after N bounces if it hasn't settled",     OptMaxBounces        },
    { "-RadiosityScale",    "Scales intensity of all bounced light; 1 is no change",    OptRadiosityScale    },
    { "-TraceFilterLinear", "Linearly filter lightmap pixels hit by radiosity traces",  OptTraceFilterLinear },
    { "-TraceFilterPoint",  "Point sample lightmap pixels hit by  radiosity traces",    OptTraceFilterPoint  },
    { "-Gamma",             "Gamma value assumed to be implicitly stored in textures",  OptGamma             },
    { "-ContrastGain",      "Increase lighting contrast (0 no change, 1 max)",          OptContrastGain      },
    { "-NoRelight",         "Disable optimization of using results from last compile",  OptNoRelight         },
    { "-BasisDirCount",     "Sample directions used to approximate lightmap pixel",     OptBasisDirCount     },
    { "-Threads",           "Allows using more or fewer threads than processors",       OptThreads           },
    { "-GridMaxDistance",   "Drops light grid points farther than N units from geometry or lights (off unless given; N defaults to 2048)", OptGridMaxDistance },
    { "-Gpu",              "Trace radiosity on the GPU (Direct3D 11); results may differ slightly", OptGpu },
    { "-DumpOptions",       "Displays current settings of most parameters",             OptDumpOptions       },
};


/* PrintUsage  0x00406010 */
static void PrintUsage( void )
{
    int i;

    Print( "USAGE: cod2rad [args] mapname, where args is 0 or more of the following.\n" );
    Print( "Options ignore capitalization; it is only present in the list for clarity.\n" );

    for ( i = 0; i < ARRAY_COUNT( radOptions ); i++ )
        Print( "%-20s %s\n", radOptions[i].name, radOptions[i].description );
}

/* ParseOptions  0x00406060 */
static bool ParseOptions( int argc, int lastArgCount, const char **argv )
{
    int i;
    int used;

    while ( argc > lastArgCount )
    {
        for ( i = 0; i < ARRAY_COUNT( radOptions ); i++ )
        {
            if ( _stricmp( argv[0], radOptions[i].name ) )
                continue;

            used = radOptions[i].handler( argc, argv );
            if ( used <= 0 )
            {
                PrintUsage();
                return false;
            }

            argc -= used;
            argv += used;
            break;
        }

        if ( i == ARRAY_COUNT( radOptions ) )
        {
            PrintUsage();
            Print( "\n" );
            Error( "Unknown argument '%s'\n", argv[0] );
        }
    }

    if ( argc != lastArgCount )
    {
        PrintUsage();
        return false;
    }

    return true;
}

static char *FindLastPathSeparator( char *path )
{
    char *slash;
    char *backslash;

    slash     = strrchr( path, '/' );
    backslash = strrchr( path, '\\' );

    return slash > backslash ? slash : backslash;
}

/* ParseCommandLine  0x00406100 */
bool ParseCommandLine( int argc, const char **argv )
{
    char  exeDir[MAX_OS_PATH];
    char *separator;
    char *extension;

    SetDefaultOptions();

    strcpy( exeDir, argv[0] );
    separator = FindLastPathSeparator( exeDir );
    if ( separator )
        *separator = '\0';

    Assert( argc >= 1 );

    argc--;
    argv++;

    if ( !argc )
    {
        PrintUsage();
        return false;
    }

    SetBSPFileExtensions( "d3d" );

    strcpy( options.mapName, argv[argc - 1] );

    separator = FindLastPathSeparator( options.mapName );
    extension = strrchr( options.mapName, '.' );
    if ( extension && ( !separator || extension > separator ) )
        *extension = '\0';

    strcat( options.mapName, GetBSPFileExtension() );

    if ( !ParseOptions( argc, 1, argv ) )
        return false;

    ApplyGpuThreadDefault();

    if ( !ValidatePlatformSet() )
        return false;

    if ( options.mapName[0] == '-' || options.mapName[0] == '?' )
        return false;

    Assert( targetPlatform != PLATFORM_VOID );

    FS_Startup( options.mapName );
    Com_InitFileSystem( g_installDir, "", "" );

    return true;
}

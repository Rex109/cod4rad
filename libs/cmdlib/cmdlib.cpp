/* Original: ..\libs\cmdlib\cmdlib.cpp */

#include "cod4rad.h"

#include <errno.h>
#include <mmsystem.h>

extern int I_strlen( const char *string );          /* 0x0040a290 */


#define SRCFILE "..\\libs\\cmdlib\\cmdlib.cpp"

extern void AssertFailed(const char *file, int line, int fatal, const char *fmt, ...);  /* 0x00402180 */

#ifndef Assert
#define Assert(exp) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s", #exp); } while (0)
#endif

#ifndef Assertx
#define Assertx(exp, fmt, ...) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s\n\t" fmt, #exp, __VA_ARGS__); } while (0)
#endif


char              g_installDir[MAX_OS_PATH];        /* 0x11ba5138 */

ComErrorHandler_t g_errorHandler;                   /* 0x11ba5538 */
ComErrorHandler_t g_printHandler;                   /* 0x11ba553c */
ComPrintHandler_t g_channelPrintHandler;            /* 0x11ba5540 */
ComPrintHandler_t g_channelWarningHandler;          /* 0x11ba5544 */

int               verbose;                          /* 0x11ba5548 */
static char       s_expandedArg[MAX_OS_PATH];       /* 0x11ba5550 */
static char       s_expandedPath[MAX_OS_PATH];      /* 0x11ba5950 */
static DWORD      s_timeBase;                       /* 0x11ba5d50 */

/* Com_Error  0x0040ddf0 */

void Com_Error( const char *fmt, ... )
{
    va_list argptr;

    if ( g_errorHandler )
    {
        va_start( argptr, fmt );
        g_errorHandler( fmt, argptr );
        va_end( argptr );
    }
}

/* Com_SetErrorHandler  0x0040ded0 */
void Com_SetErrorHandler( ComErrorHandler_t handler )
{
    g_errorHandler = handler;
}

/* Com_SetPrintHandler  0x0040dee0 */
void Com_SetPrintHandler( ComErrorHandler_t handler )
{
    g_printHandler = handler;
}

/* Com_SetChannelPrintHandler  0x0040def0 */
void Com_SetChannelPrintHandler( ComPrintHandler_t handler )
{
    g_channelPrintHandler = handler;
}

/* Com_SetChannelWarningHandler  0x0040df00 */
void Com_SetChannelWarningHandler( ComPrintHandler_t handler )
{
    g_channelWarningHandler = handler;
}

/* SafeMallocNoAlign  0x0040df10 */
void *SafeMallocNoAlign( int size )
{
    void *ptr;

    ptr = malloc( size + 1 );
    memset( ptr, 0, size );
    return ptr;
}

/* FS_FileLength  0x0040df40 */
int FS_FileLength( FILE *f )
{
    int pos;
    int end;

    pos = ftell( f );
    fseek( f, 0, SEEK_END );
    end = ftell( f );
    fseek( f, pos, SEEK_SET );
    return end;
}

/* SafeOpenWrite  0x0040df90 */
FILE *SafeOpenWrite( const char *filename )
{
    FILE *f;

    f = fopen( filename, "wb" );
    if ( !f )
        Com_Error( "Error opening %s: %s", filename, strerror( errno ) );
    return f;
}

/* SafeOpenRead  0x0040dfe0 */
FILE *SafeOpenRead( const char *filename )
{
    FILE *f;

    f = fopen( filename, "rb" );
    if ( !f )
        Com_Error( "Error opening %s: %s", filename, strerror( errno ) );
    return f;
}

/* SafeRead  0x0040e030 */
void SafeRead( FILE *f, void *buffer, int count )
{
    int bytesRead;

    Assertx( (count > 0), "(count) = %i", count );

    bytesRead = (int)fread( buffer, 1, count, f );
    if ( bytesRead != count )
        Com_Error( "File read failure - read %i of %i bytes", bytesRead, count );
}

/* SafeWrite  0x0040e0a0 */
void SafeWrite( FILE *f, const void *buffer, int count )
{
    int bytesWritten;

    Assertx( (count >= 0), "(count) = %i", count );

    bytesWritten = (int)fwrite( buffer, 1, count, f );
    if ( bytesWritten != count )
        Com_Error( "File write failure - wrote %i of %i bytes", bytesWritten, count );
}

/* LoadFile  0x0040e110 */
int LoadFile( const char *filename, void **bufferptr )
{
    int    length;
    FILE  *f;
    byte  *buffer;

    *bufferptr = NULL;

    if ( !filename || !strlen( filename ) )
        return -1;

    f = fopen( filename, "rb" );
    if ( !f )
        return -1;

    length = FS_FileLength( f );
    buffer = (byte *)SafeMalloc( length + 1 );
    buffer[length] = 0;
    if ( length )
        SafeRead( f, buffer, length );
    fclose( f );

    *bufferptr = buffer;
    return length;
}

/* SafeMalloc  0x0040e1c0 */
void *SafeMalloc( int size )
{
    int   rem;
    void *ptr;

    rem = size % MEM_PAGE_SIZE;
    if ( rem > 0 )
        size = MEM_PAGE_SIZE - rem + size;

    ptr = malloc( size );
    memset( ptr, 0, size );
    return ptr;
}

/* TryLoadFile  0x0040e220 */
int TryLoadFile( const char *filename, void **bufferptr )
{
    int   length;
    FILE *f;
    byte *buffer;

    f = fopen( filename, "rb" );
    if ( !f )
        return -1;

    length = FS_FileLength( f );
    buffer = (byte *)SafeMallocNoAlign( length + 1 );
    buffer[length] = 0;
    SafeRead( f, buffer, length );
    fclose( f );

    *bufferptr = buffer;
    return length;
}

/* SaveFile  0x0040e2a0 */
void SaveFile( const char *filename, const void *buffer, int count )
{
    FILE *f;

    f = SafeOpenWrite( filename );
    SafeWrite( f, buffer, count );
    fclose( f );
}

/* DefaultExtension  0x0040e2e0 */
void DefaultExtension( char *path, const char *extension )
{
    char *src;

    src = path + strlen( path ) - 1;

    while ( *src != '/' && src != path )
    {
        if ( *src == '.' )
            return;
        src--;
    }

    strcat( path, extension );
}

/* DefaultPath  0x0040e340 */
void DefaultPath( char *path, const char *basepath )
{
    char temp[128];

    if ( path[0] == '/' )
        return;

    strcpy( temp, path );
    strcpy( path, basepath );
    strcat( path, temp );
}

/* StripFilename  0x0040e3b0 */
void StripFilename( char *path )
{
    int length;

    length = I_strlen( path ) - 1;
    while ( length > 0 && path[length] != '/' )
        length--;
    path[length] = 0;
}

/* StripExtension  0x0040e400 */
void StripExtension( char *path )
{
    int length;

    length = I_strlen( path ) - 1;
    while ( length > 0 && path[length] != '.' )
    {
        length--;
        if ( path[length] == '/' )
            return;
    }
    if ( length )
        path[length] = 0;
}

/* ExtractFilePath  0x0040e460 */
void ExtractFilePath( const char *path, char *dest )
{
    const char *src;

    src = path + strlen( path ) - 1;

    while ( src != path && *( src - 1 ) != '/' )
        src--;

    memcpy( dest, path, (size_t)( src - path ) );
    dest[src - path] = 0;
}

/* ExtractFileName  0x0040e4d0 */
void ExtractFileName( const char *path, char *dest )
{
    const char *src;

    src = path + strlen( path ) - 1;

    while ( src != path && *( src - 1 ) != '/' && *( src - 1 ) != '\\' )
        src--;

    while ( *src )
    {
        *dest++ = *src++;
    }
    *dest = 0;
}

/* ExtractFileBase  0x0040e550 */
void ExtractFileBase( const char *path, char *dest )
{
    const char *src;

    src = path + strlen( path ) - 1;

    while ( src != path && *( src - 1 ) != '/' && *( src - 1 ) != '\\' )
        src--;

    while ( *src && *src != '.' )
    {
        *dest++ = *src++;
    }
    *dest = 0;
}

/* ExtractFileExtension  0x0040e5e0 */
void ExtractFileExtension( const char *path, char *dest )
{
    const char *src;

    src = path + strlen( path ) - 1;

    while ( src != path && *( src - 1 ) != '.' )
        src--;

    if ( src == path )
    {
        *dest = 0;
        return;
    }

    strcpy( dest, src );
}

/* ConvertDOSToUnixName  0x0040e640 */
void ConvertDOSToUnixName( char *dst, const char *src )
{
    while ( *src )
    {
        if ( *src == '\\' )
            *dst = '/';
        else
            *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
}

/* ConvertUnixToDOSName  0x0040e690 */
void ConvertUnixToDOSName( char *dst, const char *src )
{
    while ( *src )
    {
        if ( *src == '/' )
            *dst = '\\';
        else
            *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
}

/* copystring  0x0040e6e0 */
char *copystring( const char *s )
{
    char *b;

    if ( !s )
        return NULL;

    b = (char *)::operator new( strlen( s ) + 1 );
    return strcpy( b, s );
}

/* CopyString  0x0040e720 */
namespace {

char *CopyString( const char *s )
{
    char *b;

    if ( !s )
        return NULL;

    b = (char *)::operator new( strlen( s ) + 1 );
    return strcpy( b, s );
}

}

/* Com_DPrintf  0x0040e760 */
void Com_DPrintf( const char *fmt, ... )
{
    va_list argptr;

    if ( verbose )
    {
        va_start( argptr, fmt );
        vprintf( fmt, argptr );
    }
}

/* Com_Printf  0x0040e790 */
void Com_Printf( const char *fmt, ... )
{
    char    buf[MAXPRINTMSG];
    va_list argptr;

    va_start( argptr, fmt );
    vsprintf( buf, fmt, argptr );
    va_end( argptr );

    printf( buf );
    fflush( stdout );
}

/* FS_GetPreviousPathComponent  0x0040e810 */
char *FS_GetPreviousPathComponent( const char *path, char *pos )
{
    Assert( path );

    if ( !pos )
        return NULL;

    Assert( pos >= path );
    Assert( *pos == '\0' || pos == path || pos[-1] == '/' );

    if ( pos == path )
        return NULL;

    do
    {
        pos--;
    }
    while ( pos != path && pos[-1] != '/' );

    return pos;
}

/* FS_Startup  0x0040e8e0 */
void FS_Startup( const char *exePath )
{
    char        fullPath[MAX_OS_PATH];
    int         i;
    int         len;
    const char *dirName;
    char       *pos;

    len = (int)GetFullPathNameA( exePath, MAX_OS_PATH, fullPath, NULL );
    if ( len <= 0 || (unsigned)len >= MAX_OS_PATH )
        Com_Error( "couldn't get full path for '%s'\n", exePath );

    for ( i = 0; i < len; i++ )
    {
        if ( fullPath[i] == '\\' )
            fullPath[i] = '/';
    }

    dirName = "maps";
    len = I_strlen( dirName );

    pos = fullPath + strlen( fullPath );
    do
    {
        pos = FS_GetPreviousPathComponent( fullPath, pos );
        if ( !pos )
            Com_Error( "No '%s' in '%s'\n", dirName, fullPath );
    }
    while ( _strnicmp( pos, dirName, len ) != 0 ||
            ( pos[len] != '\0' && pos[len] != '/' ) );

    pos = FS_GetPreviousPathComponent( fullPath, pos );
    if ( !pos || pos == fullPath )
        Com_Error( "There should be two folders below '%s' in a proper install\n", dirName );

    len = (int)( pos - fullPath );
    memcpy( g_installDir, fullPath, len );
    g_installDir[len] = 0;

    pos = FS_GetPreviousPathComponent( g_installDir, g_installDir + len );
    if ( pos )
    {
        if ( !_stricmp( pos, "share/" ) )
        {
            strcpy( &g_installDir[len - 6], g_platformNames[targetPlatform] );
            strcat( g_installDir, "/" );
        }
    }

    Com_DPrintf( "install directory: %s\n", g_installDir );
}

/* ReplacePathComponent  0x0040eb70 */
void ReplacePathComponent( char *path, const char *oldName, const char *newName )
{
    int       tailLen;
    char      temp[MAX_OS_PATH];
    int       nameLen;
    char     *tail;
    char     *pos;
    unsigned  i;

    nameLen = I_strlen( oldName );

    for ( pos = path + strlen( path ) - 1; pos != path; pos-- )
    {
        if ( Q_stricmpn( pos, oldName, nameLen ) != 0 )
            continue;

        tailLen = 1;
        for ( tail = pos + nameLen; *tail && *tail != '/' && *tail != '\\'; tail++ )
            tailLen++;

        memset( temp, 0, sizeof( temp ) );
        strncpy( temp, path, (size_t)( pos - path ) );
        strcat( temp, newName );
        strcat( temp, tail );

        for ( i = 0; i < strlen( g_installDir ); i++ )
        {
            if ( temp[i] == '\\' )
                temp[i] = '/';
        }

        strcpy( path, temp );
    }
}

/* ExpandArg  0x0040ed30 */
char *ExpandArg( const char *path )
{
    if ( path[0] != '/' && path[0] != '\\' && path[1] != ':' )
    {
        Q_getwd( s_expandedArg );
        strcat( s_expandedArg, path );
    }
    else
    {
        strcpy( s_expandedArg, path );
    }
    return s_expandedArg;
}

/* ExpandPath  0x0040ed90 */
char *ExpandPath( const char *path )
{
    if ( !g_installDir[0] )
        Com_Error( "ExpandPath called without install dir set" );

    if ( path[0] == '/' || path[0] == '\\' || path[1] == ':' )
    {
        strcpy( s_expandedPath, path );
        return s_expandedPath;
    }

    sprintf( s_expandedPath, "%s%s", g_installDir, path );
    return s_expandedPath;
}

/* I_FloatTime  0x0040ee10 */
double I_FloatTime( void )
{
    DWORD elapsed;

    if ( !s_timeBase )
        s_timeBase = timeGetTime();

    elapsed = timeGetTime() - s_timeBase;
    return elapsed * 0.001;
}

/* Q_getwd  0x0040ee60 */
void Q_getwd( char *out )
{
    int i;

    i = 0;
    _getcwd( out, 256 );
    strcat( out, "\\" );

    while ( out[i] != 0 )
    {
        if ( out[i] == '\\' )
            out[i] = '/';
        i++;
    }
}

/* Com_InitFileSystem  0x00405690 */
void Com_InitFileSystem( const char *basePath, const char *game, const char *baseGame )
{
    Swap_Init();
    Dvar_Init();

    if ( basePath )
        Dvar_SetStringByName( "fs_basepath", basePath );

    if ( baseGame )
        Dvar_SetStringByName( "fs_basegame", baseGame );

    if ( game )
        Dvar_SetStringByName( "fs_game", game );

    FS_InitFilesystem();
}

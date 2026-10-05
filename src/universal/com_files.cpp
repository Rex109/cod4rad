/* Original: ..\src\universal\com_files.cpp */


#include "q_shared.h"
#include "com_shared.h"
#include "com_memory.h"
#include "assertive.h"
#include "dvar.h"
#include "com_files.h"

#include "../zlib/unzip.h"
#include "../qcommon/com_fileAccess.h"
#include "qsort_vc8.h"

static fileHandle_t FS_HandleOpenWrite( const char *qpath, const char *ospath, int thread );
static qboolean FS_IsBadPathSequence( const char *s );
static qboolean FS_IsExt( const char *filename );
static int FS_ReturnPath( const char *zname, char *zpath, int *depth );
static int FS_AddFileToList( HunkUser *user, const char *name, char **list, int nfiles );
static int iwdsort( const void *a, const void *b );
static bool FS_GameDirDomainFunc( Dvar_t *dvar, DvarValue_t newValue );
static void FS_GameDirDomainError( Dvar_t *dvar, DvarValue_t newValue );
static void FS_ConvertGameDirSeparators( void );
static qboolean FS_VerifyFileSysCheck( void );

extern int I_strlen( const char *string );                          /* 0x0040a290 */

#ifndef DVAR_H

#define DVAR_INIT   0x0010

typedef union DvarValue_u
{
    const char *string;
    int         integer;
    unsigned    unsignedInt;
    float       value;
    vec4_t      vector;
    byte        enabled;
} DvarValue;

typedef struct Dvar_s
{
    const char    *name;            /* +0x00 */
    const char    *description;     /* +0x04 */
    unsigned short flags;           /* +0x08 */
    byte           type;            /* +0x0a */
    bool           modified;        /* +0x0b */
    DvarValue      current;         /* +0x0c */
    DvarValue      latched;         /* +0x1c */
    DvarValue      reset;           /* +0x2c */
} Dvar_t;

typedef bool ( *DvarDomainFunc_t )( Dvar_t *dvar, DvarValue_t newValue );

extern Dvar_t *Dvar_RegisterBool( const char *dvarName, bool value,
                                  unsigned short flags, const char *description );  /* 0x004792b0 */
extern Dvar_t *Dvar_RegisterInt( const char *dvarName, int value, int min, int max,
                                 unsigned short flags, const char *description );   /* 0x0047a670 */
extern Dvar_t *Dvar_RegisterString( const char *dvarName, const char *value,
                                    unsigned short flags, const char *description );/* 0x0047a8f0 */
extern void    Dvar_SetString( Dvar_t *dvar, const char *value );                   /* 0x0047b750 */
extern void    Dvar_SetDomainFunc( Dvar_t *dvar, DvarDomainFunc_t domainFunc );     /* 0x0047bf30 */
extern void    Dvar_ClearModified( Dvar_t *dvar );                                  /* 0x00477830 */

#endif


Dvar_t          *fs_copyfiles;                          /* 0x2b920ab8 */
Dvar_t          *fs_cdpath;                             /* 0x2b920abc */
Dvar_t          *fs_basegame;                           /* 0x2b920ac0 */
Dvar_t          *fs_ignoreLocalized;                    /* 0x2b920ac4 */
char             fs_gamedir[MAX_OS_PATH_SHORT];         /* 0x2b920ac8 */
Dvar_t          *fs_gameDirVar;                         /* 0x2b920bc8 */

static int       fs_fileAccessed;                       /* 0x2b920bcc */

Dvar_t          *fs_debug;                              /* 0x2b920bd0 */
static char      g_savedFsBasepath[MAX_OS_PATH_SHORT];  /* 0x2b920bd8 */
Dvar_t          *fs_basepath;                           /* 0x2b920cd8 */
static int       fs_loadStack;                          /* 0x2b920cdc */
static char      g_savedFsGame[MAX_OS_PATH_SHORT];      /* 0x2b920ce0 */
Dvar_t          *fs_homepath;                           /* 0x2b920de0 */
fileHandleData_t fsh[MAX_FILE_HANDLES];                 /* 0x2b920de8 */
searchpath_t    *fs_searchpaths;                        /* 0x2b925604 */
int              fs_iwdFileCount;                       /* 0x2b925608 */

static char      g_langBuf[2][64];                      /* 0x2b925610 */
static int       g_langBufToggle;                       /* 0x2b925690 */

/* FS_Initialized  0x004676a0 */
qboolean FS_Initialized( void )
{
    return fs_searchpaths != NULL;
}

/* FS_CheckInitialized  0x004676c0 */
void FS_CheckInitialized( void )
{
    if ( !fs_searchpaths )
        Com_ErrorLevel( ERR_FATAL, "Filesystem call made without initialization\n" );
}

/* FS_LoadStack  0x004676e0 */
int FS_LoadStack( void )
{
    return fs_loadStack;
}

/* FS_UseSearchPath  0x004676f0 */
qboolean FS_UseSearchPath( const searchpath_t *search )
{
    if ( !search->localized || !fs_ignoreLocalized->current.enabled )
        return qtrue;

    return qfalse;
}

/* FS_LanguageHasAssets  0x00467720 */
qboolean FS_LanguageHasAssets( int iLanguage )
{
    searchpath_t *search;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->localized && search->language == iLanguage )
            return qtrue;
    }

    return qfalse;
}

/* FS_HashFileName  0x00467760 */
unsigned FS_HashFileName( const char *fname, int hashSize )
{
    int hash;
    int letter;
    int i;

    hash = 0;
    i = 0;
    while ( fname[i] != '\0' )
    {
        letter = tolower( fname[i] );
        if ( letter == '.' )
            break;
        if ( letter == '\\' )
            letter = '/';
        hash += ( i + 119 ) * letter;
        i++;
    }

    return ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( hashSize - 1 );
}

/* FS_HandleForFile  0x004677f0 */
fileHandle_t FS_HandleForFile( int thread )
{
    int first;
    int count;
    int i;

    if ( thread == FS_THREAD_MAIN )
    {
        Assert( Sys_IsMainThread() );
        first = 1;
        count = 48;
    }
    else if ( thread == FS_THREAD_STREAM )
    {
        first = 1 + 48;
        count = 13;
    }
    else if ( thread == FS_THREAD_BACKEND )
    {
        first = 1 + 48 + 13 + 1;
        count = 1;
    }
    else
    {
        AssertCmp( thread, ==, FS_THREAD_DATABASE );
        Assert( Sys_IsMainThread() );
        first = 1 + 48 + 13;
        count = 1;
    }

    for ( i = 0; i < count; i++ )
    {
        if ( !fsh[first + i].handleFiles.file.o )
            return first + i;
    }

    if ( thread == FS_THREAD_MAIN )
    {
        for ( i = 1; i < MAX_FILE_HANDLES; i++ )
            Com_Printf( CON_CHANNEL_FILES, "FILE %2i: '%s' 0x%x\n",
                        i, fsh[i].name, fsh[i].handleFiles.file.o );

        Com_ErrorLevel( ERR_DROP, "FS_HandleForFile: none free" );
    }

    Com_PrintWarning( CON_CHANNEL_FILES, "FILE %2i: '%s' 0x%x\n",
                      first, fsh[first].name, fsh[first].handleFiles.file.o );
    Com_PrintWarning( CON_CHANNEL_FILES, "FS_HandleForFile: none free (%d)\n", thread );

    return 0;
}

/* FS_FileForHandle  0x004679c0 */
FILE *FS_FileForHandle( fileHandle_t f )
{
    Assertx( f > 0 && f < MAX_FILE_HANDLES, "(f) = %i", f );
    Assert( !fsh[f].zipFile );
    Assert( fsh[f].handleFiles.file.o );

    return fsh[f].handleFiles.file.o;
}

/* FS_filelength  0x00467a70 */
int FS_filelength( fileHandle_t f )
{
    int length;

    Assert( f );
    FS_CheckInitialized();

    if ( !fsh[f].zipFile )
        length = FS_FileLength( FS_FileForHandle( f ) );
    else
        length = ( (unz_s *)fsh[f].handleFiles.file.z )->cur_file_info.uncompressed_size;

    return length;
}

/* FS_ReplaceSeparators  0x00467af0 */
void FS_ReplaceSeparators( char *path )
{
    char *src;
    char *dst;
    bool  lastCharWasSep;

    lastCharWasSep = false;
    dst = path;

    for ( src = path; *src != '\0'; src++ )
    {
        if ( *src == '/' || *src == '\\' )
        {
            if ( !lastCharWasSep )
            {
                lastCharWasSep = true;
                *dst = '\\';
                dst++;
            }
        }
        else
        {
            lastCharWasSep = false;
            *dst = *src;
            dst++;
        }
    }

    *dst = '\0';
}

/* FS_BuildOSPath  0x00467b80 */
void FS_BuildOSPath( const char *base, const char *gamedir, const char *qpath, char *ospath )
{
    FS_BuildOSPathFull( base, gamedir, qpath, ospath, 0 );
}

/* FS_BuildOSPathFull  0x00467ba0 */
void FS_BuildOSPathFull( const char *base, const char *gamedir, const char *qpath,
                         char *ospath, int okToFail )
{
    int baseLength;
    int gamedirLength;
    int qpathLength;

    Assert( base );
    Assert( qpath );
    Assert( ospath );

    if ( !gamedir || !gamedir[0] )
        gamedir = fs_gamedir;

    baseLength = I_strlen( base );
    gamedirLength = I_strlen( gamedir );
    qpathLength = I_strlen( qpath );

    if ( baseLength + gamedirLength + 2 + qpathLength > MAX_OS_PATH_SHORT - 1 )
    {
        if ( okToFail )
        {
            *ospath = '\0';
            return;
        }

        Com_ErrorLevel( ERR_FATAL, "FS_BuildOSPath: os path length exceeded\n" );
    }

    memcpy( ospath, base, baseLength );
    ospath[baseLength] = '/';
    memcpy( ospath + baseLength + 1, gamedir, gamedirLength );
    ospath[baseLength + 1 + gamedirLength] = '/';
    memcpy( ospath + baseLength + gamedirLength + 2, qpath, qpathLength + 1 );

    FS_ReplaceSeparators( ospath );
}

/* FS_CreatePath  0x00467d10 */
qboolean FS_CreatePath( char *OSPath )
{
    char *ofs;

    if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) )
    {
        Com_PrintWarning( CON_CHANNEL_FILES,
                          "WARNING: refusing to create relative path \"%s\"\n", OSPath );
        return qtrue;
    }

    for ( ofs = OSPath + 1; *ofs != '\0'; ofs++ )
    {
        if ( *ofs == '\\' )
        {
            *ofs = '\0';
            Sys_Mkdir( OSPath );
            *ofs = '\\';
        }
    }

    return qfalse;
}

/* FS_FileExists  0x00467db0 */
qboolean FS_FileExists( const char *file )
{
    char  testpath[MAX_OS_PATH_SHORT];
    FILE *f;

    FS_BuildOSPath( fs_homepath->current.string, fs_gamedir, file, testpath );

    f = fopen( testpath, "rb" );
    if ( f )
    {
        fclose( f );
        return qtrue;
    }

    return qfalse;
}

/* FS_CopyFile  0x00467e30 */
void FS_CopyFile( const char *fromOSPath, const char *toOSPath )
{
    FILE *f;
    int   len;
    byte *buf;

    f = fopen( fromOSPath, "rb" );
    if ( !f )
        return;

    len = FS_FileLength( f );
    buf = (byte *)malloc( len );

    if ( fread( buf, 1, len, f ) != (size_t)len )
        Com_ErrorLevel( ERR_FATAL, "Short read in FS_CopyFile()\n" );

    fclose( f );

    if ( FS_CreatePath( (char *)toOSPath ) )
    {
        free( buf );
        return;
    }

    f = fopen( toOSPath, "wb" );
    if ( !f )
    {
        free( buf );
        return;
    }

    if ( fwrite( buf, 1, len, f ) != (size_t)len )
        Com_ErrorLevel( ERR_FATAL, "Short write in FS_CopyFile()\n" );

    fclose( f );
    free( buf );
}

/* FS_RemoveOSPath  0x00467f40 */
void FS_RemoveOSPath( const char *osPath )
{
    remove( osPath );
}

/* FS_Rename  0x00467f60 */
void FS_Rename( const char *from, const char *fromGamedir,
                const char *to, const char *toGamedir )
{
    char fromOSPath[MAX_OS_PATH_SHORT];
    char toOSPath[MAX_OS_PATH_SHORT];

    FS_CheckInitialized();

    FS_BuildOSPath( fs_homepath->current.string, fromGamedir, from, fromOSPath );
    FS_BuildOSPath( fs_homepath->current.string, toGamedir, to, toOSPath );

    if ( fs_debug->current.integer )
        Com_Printf( CON_CHANNEL_FILES, "FS_Rename: %s --> %s\n", fromOSPath, toOSPath );

    if ( rename( fromOSPath, toOSPath ) )
    {
        FS_RemoveOSPath( toOSPath );

        if ( rename( fromOSPath, toOSPath ) )
        {
            FS_CopyFile( fromOSPath, toOSPath );
            FS_RemoveOSPath( fromOSPath );
        }
    }
}

/* FS_FCloseFile  0x00468060 */
void FS_FCloseFile( fileHandle_t f )
{
    FS_CheckInitialized();

    Assert( !fsh[f].streamed );

    if ( !fsh[f].zipFile )
    {
        if ( f )
            fclose( FS_FileForHandle( f ) );

        Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
    }
    else
    {
        unzCloseCurrentFile( fsh[f].handleFiles.file.z );

        if ( fsh[f].handleFiles.unique )
        {
            unzClose( fsh[f].handleFiles.file.z );
        }
        else
        {
            Assert( fsh[f].zipFile->hasOpenFile );
            fsh[f].zipFile->hasOpenFile = 0;
        }

        Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
    }
}

/* FSH_FCloseFile  0x004681b0 */
void FSH_FCloseFile( fileHandle_t f )
{
    FS_FCloseFile( f );
}

/* FS_FOpenFileWriteToDir  0x004681d0 */
fileHandle_t FS_FOpenFileWriteToDir( const char *qpath, const char *dir )
{
    return FS_FOpenFileWriteToDirForThread( qpath, dir, FS_THREAD_MAIN );
}

/* FS_FOpenFileWriteToDirForThread  0x004681f0 */
fileHandle_t FS_FOpenFileWriteToDirForThread( const char *qpath, const char *dir, int thread )
{
    char ospath[MAX_OS_PATH_SHORT];

    FS_CheckInitialized();

    FS_BuildOSPath( fs_homepath->current.string, dir, qpath, ospath );

    if ( fs_debug->current.integer )
        Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileWrite: %s\n", ospath );

    if ( FS_CreatePath( ospath ) )
        return 0;

    return FS_HandleOpenWrite( qpath, ospath, thread );
}

/* FS_HandleOpenWrite  0x00468290 */
static fileHandle_t FS_HandleOpenWrite( const char *qpath, const char *ospath, int thread )
{
    FILE        *file;
    fileHandle_t f;

    file = fopen( ospath, "wb" );
    if ( !file )
        return 0;

    f = FS_HandleForFile( thread );
    fsh[f].zipFile = NULL;
    fsh[f].handleFiles.file.o = file;
    I_strncpyz( fsh[f].name, qpath, sizeof( fsh[f].name ) );
    fsh[f].handleSync = 0;

    return f;
}

/* FS_FOpenFileWrite  0x00468330 */
fileHandle_t FS_FOpenFileWrite( const char *qpath )
{
    return FS_FOpenFileWriteToDirForThread( qpath, fs_gamedir, FS_THREAD_MAIN );
}

/* FS_FOpenFileWriteCurrentThread  0x00468350 */
fileHandle_t FS_FOpenFileWriteCurrentThread( const char *qpath )
{
    int thread;

    thread = FS_GetCurrentThread();
    if ( thread == FS_THREAD_INVALID )
    {
        Com_PrintError( CON_CHANNEL_ERROR, "FS_FOpenFileWriteCurrentThread for an unknown thread\n" );
        return 0;
    }

    return FS_FOpenFileWriteToDirForThread( qpath, fs_gamedir, thread );
}

/* FS_GetCurrentThread  0x00468390 */
int FS_GetCurrentThread( void )
{
    if ( !Sys_IsMainThread() )
        return FS_THREAD_INVALID;

    return FS_THREAD_MAIN;
}

/* FS_FOpenTextFileWrite  0x004683b0 */
fileHandle_t FS_FOpenTextFileWrite( const char *qpath )
{
    char         ospath[MAX_OS_PATH_SHORT];
    fileHandle_t f;
    FILE        *file;

    f = 0;
    FS_CheckInitialized();

    f = FS_HandleForFile( FS_THREAD_MAIN );
    fsh[f].zipFile = NULL;

    FS_BuildOSPath( fs_homepath->current.string, fs_gamedir, qpath, ospath );

    if ( fs_debug->current.integer )
        Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileWrite: %s\n", ospath );

    if ( !FS_CreatePath( ospath ) )
    {
        file = fopen( ospath, "w+t" );
        fsh[f].handleFiles.file.o = file;
        I_strncpyz( fsh[f].name, qpath, sizeof( fsh[f].name ) );
        fsh[f].handleSync = 0;

        if ( !fsh[f].handleFiles.file.o )
            f = 0;
    }

    return f;
}

/* FS_FOpenFileAppend  0x004684d0 */
fileHandle_t FS_FOpenFileAppend( const char *qpath )
{
    char         ospath[MAX_OS_PATH_SHORT];
    fileHandle_t f;
    FILE        *file;

    Assert( Sys_IsMainThread() || Sys_IsRenderThread() );

    f = 0;
    FS_CheckInitialized();

    f = FS_HandleForFile( Sys_IsMainThread() ? FS_THREAD_MAIN : FS_THREAD_BACKEND );
    fsh[f].zipFile = NULL;
    I_strncpyz( fsh[f].name, qpath, sizeof( fsh[f].name ) );

    FS_BuildOSPath( fs_homepath->current.string, fs_gamedir, qpath, ospath );

    if ( fs_debug->current.integer )
        Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileAppend: %s\n", ospath );

    if ( !FS_CreatePath( ospath ) )
    {
        file = fopen( ospath, "at" );
        fsh[f].handleFiles.file.o = file;
        fsh[f].handleSync = 0;

        if ( !fsh[f].handleFiles.file.o )
            f = 0;
    }

    return f;
}

/* FS_FilenameCompare  0x00468640 */
int FS_FilenameCompare( const char *s1, const char *s2 )
{
    int c1;
    int c2;

    do
    {
        c1 = *s1++;
        c2 = *s2++;

        if ( Q_islower( c1 ) )
            c1 -= ( 'a' - 'A' );
        if ( Q_islower( c2 ) )
            c2 -= ( 'a' - 'A' );

        if ( c1 == '\\' || c1 == ':' )
            c1 = '/';
        if ( c2 == '\\' || c2 == ':' )
            c2 = '/';

        if ( c1 != c2 )
            return -1;
    }
    while ( c1 );

    return 0;
}

/* FS_FileCompare  0x004686f0 */
qboolean FS_FileCompare( const char *path0, const char *path1 )
{
    FILE        *f0;
    FILE        *f1;
    unsigned int len0;
    unsigned int len1;
    char        *buf0;
    char        *buf1;
    char        *p0;
    char        *p1;
    unsigned int i;

    f0 = fopen( path0, "rb" );
    if ( !f0 )
        Com_ErrorLevel( ERR_FATAL, "FS_FileCompare: %s does not exist\n", path0 );

    f1 = fopen( path1, "rb" );
    if ( !f1 )
    {
        fclose( f0 );
        return qfalse;
    }

    len0 = FS_FileLength( f0 );
    len1 = FS_FileLength( f1 );

    if ( len0 != len1 )
    {
        fclose( f0 );
        fclose( f1 );
        return qfalse;
    }

    buf0 = (char *)Z_Malloc( len0 );
    if ( fread( buf0, 1, len0, f0 ) != len0 )
        Com_ErrorLevel( ERR_FATAL, "Short read in FS_FileCompare()\n" );
    fclose( f0 );

    buf1 = (char *)Z_Malloc( len1 );
    if ( fread( buf1, 1, len1, f1 ) != len1 )
        Com_ErrorLevel( ERR_FATAL, "Short read in FS_FileCompare()\n" );
    fclose( f1 );

    p0 = buf0;
    p1 = buf1;

    for ( i = 0; i < len0; i++ )
    {
        if ( *p0 != *p1 )
        {
            free( buf0 );
            free( buf1 );
            return qfalse;
        }
        p0++;
        p1++;
    }

    free( buf0 );
    free( buf1 );

    return qtrue;
}

/* FS_ShiftedStrStr  0x004688b0 */
char *FS_ShiftedStrStr( const char *string, const char *substring, int shift )
{
    char buf[MAX_OS_PATH_SHORT];
    int  i;

    for ( i = 0; substring[i]; i++ )
        buf[i] = substring[i] + shift;

    buf[i] = '\0';

    return strstr( (char *)string, buf );
}

/* FS_FOpenFileReadStream  0x00468930 */
int FS_FOpenFileReadStream( const char *filename, fileHandle_t *file )
{
    return FS_FOpenFileReadForThread( filename, file, FS_THREAD_STREAM );
}

/* FS_FOpenFileReadForThread  0x00468950 */
int FS_FOpenFileReadForThread( const char *filename, fileHandle_t *file, int thread )
{
    char          copypath[MAX_OS_PATH_SHORT];
    char          sanitizedName[MAX_OS_PATH_SHORT];
    char          netpath[MAX_OS_PATH_SHORT];
    searchpath_t *search;
    directory_t  *dir;
    iwd_t        *pak;
    fileInIwd_t  *pakFile;
    unz_s        *zfi;
    FILE         *temp;
    void         *tempRead;
    unsigned      hash;

    hash = 0;

    Assert( filename );
    FS_CheckInitialized();

    if ( !FS_SanitizePath( filename, sanitizedName, sizeof( sanitizedName ) ) )
    {
        if ( file )
            *file = 0;

        return -1;
    }

    if ( !file )
    {
        for ( search = fs_searchpaths; search; search = search->next )
        {
            if ( !FS_UseSearchPath( search ) )
                continue;

            if ( search->iwd )
                hash = FS_HashFileName( sanitizedName, search->iwd->hashSize );

            if ( search->iwd && search->iwd->hashTable[hash] )
            {
                pak = search->iwd;

                for ( pakFile = pak->hashTable[hash]; pakFile; pakFile = pakFile->next )
                {
                    if ( !FS_FilenameCompare( pakFile->name, sanitizedName ) )
                        return 1;
                }
            }
            else if ( search->dir )
            {
                dir = search->dir;

                FS_BuildOSPathFull( dir->path, dir->gamedir, sanitizedName, netpath, thread );

                temp = fopen( netpath, "rb" );
                if ( temp )
                {
                    fclose( temp );
                    return 1;
                }
            }
        }

        return -1;
    }

    *file = FS_HandleForFile( thread );
    if ( !*file )
        return -1;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( !FS_UseSearchPath( search ) )
            continue;

        pak = search->iwd;
        if ( pak )
            hash = FS_HashFileName( sanitizedName, pak->hashSize );

        if ( pak && pak->hashTable[hash] )
        {
            for ( pakFile = pak->hashTable[hash]; pakFile; pakFile = pakFile->next )
            {
                if ( FS_FilenameCompare( pakFile->name, sanitizedName ) )
                    continue;

                if ( !pak->referenced && !FS_IsExt( sanitizedName ) )
                    pak->referenced = 1;

                if ( CompareExchange( &pak->hasOpenFile, 1, 0 ) == 1 )
                {
                    fsh[*file].handleFiles.unique = 1;
                    fsh[*file].handleFiles.file.z = unzReOpen( pak->iwdFilename, pak->handle );

                    if ( !fsh[*file].handleFiles.file.z )
                    {
                        if ( thread )
                        {
                            FS_FCloseFile( *file );
                            *file = 0;
                            return -1;
                        }

                        Com_ErrorLevel( ERR_FATAL, "Couldn't reopen %s", pak->iwdFilename );
                    }
                }
                else
                {
                    fsh[*file].handleFiles.unique = 0;
                    fsh[*file].handleFiles.file.z = pak->handle;
                }

                I_strncpyz( fsh[*file].name, sanitizedName, sizeof( fsh[*file].name ) );
                fsh[*file].zipFile = pak;

                zfi = (unz_s *)fsh[*file].handleFiles.file.z;

                temp = zfi->file;
                tempRead = zfi->pfile_in_zip_read;

                unzSetCurrentFileInfoPosition( pak->handle, pakFile->pos );
                Com_Memcpy( zfi, pak->handle, sizeof( unz_s ) );

                zfi->file = temp;
                zfi->pfile_in_zip_read = tempRead;

                unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
                fsh[*file].zipFilePos = pakFile->pos;

                if ( fs_debug->current.integer && thread == FS_THREAD_MAIN )
                    Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileRead: %s (found in '%s')\n",
                                sanitizedName, pak->iwdFilename );

                return zfi->cur_file_info.uncompressed_size;
            }
        }
        else if ( search->dir )
        {
            dir = search->dir;

            FS_BuildOSPathFull( dir->path, dir->gamedir, sanitizedName, netpath, thread );

            fsh[*file].handleFiles.file.o = fopen( netpath, "rb" );
            if ( !fsh[*file].handleFiles.file.o )
                continue;

            I_strncpyz( fsh[*file].name, sanitizedName, sizeof( fsh[*file].name ) );
            fsh[*file].zipFile = NULL;

            if ( fs_debug->current.integer && thread == FS_THREAD_MAIN )
                Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileRead: %s (found in '%s/%s')\n",
                            sanitizedName, dir->path, dir->gamedir );

            if ( fs_copyfiles->current.enabled &&
                 !Q_stricmp( dir->path, fs_cdpath->current.string ) )
            {
                FS_BuildOSPathFull( fs_basepath->current.string, dir->gamedir,
                                    sanitizedName, copypath, thread );
                FS_CopyFile( netpath, copypath );
            }

            return FS_filelength( *file );
        }
    }

    if ( fs_debug->current.integer && thread == FS_THREAD_MAIN )
        Com_Printf( CON_CHANNEL_FILES, "Can't find %s\n", filename );

    *file = 0;

    return -1;
}

/* FS_SanitizePath  0x00469070 */
qboolean FS_SanitizePath( const char *filename, char *sanitizedName, int sanitizedNameSize )
{
    int srcIndex;
    int dstIndex;

    Assert( filename );
    Assert( sanitizedName );
    Assertx( sanitizedNameSize > 0, "(sanitizedNameSize) = %i", sanitizedNameSize );

    srcIndex = 0;
    while ( FS_IsSlash( filename[srcIndex] ) )
        srcIndex++;

    dstIndex = 0;

    for ( ;; )
    {
        if ( filename[srcIndex] == '\0' )
        {
            SanityCheckCmp( dstIndex, <=, srcIndex );
            sanitizedName[dstIndex] = '\0';
            return qtrue;
        }

        if ( FS_IsBadPathSequence( &filename[srcIndex] ) )
            return qfalse;

        if ( filename[srcIndex] != '.' ||
             ( filename[srcIndex + 1] != '\0' && !FS_IsSlash( filename[srcIndex + 1] ) ) )
        {
            if ( dstIndex + 1 >= sanitizedNameSize )
            {
                Assertx( dstIndex + 1 < sanitizedNameSize, "%s",
                         va( "%i + 1 > %i", dstIndex, sanitizedNameSize ) );
                return qfalse;
            }

            if ( !FS_IsSlash( filename[srcIndex] ) )
            {
                sanitizedName[dstIndex] = filename[srcIndex];
            }
            else
            {
                sanitizedName[dstIndex] = '/';
                while ( FS_IsSlash( filename[srcIndex + 1] ) )
                    srcIndex++;
            }

            dstIndex++;
        }

        srcIndex++;
    }
}

/* FS_IsBadPathSequence  0x00469280 */
static qboolean FS_IsBadPathSequence( const char *s )
{
    if ( s[0] == '.' && s[1] == '.' )
        return qtrue;

    if ( s[0] == ':' && s[1] == ':' )
        return qtrue;

    return qfalse;
}

/* FS_IsExt  0x004692c0 */
static qboolean FS_IsExt( const char *filename )
{
    const char *extensions[8];
    int         length;
    int         i;

    extensions[0] = ".hlsl";
    extensions[1] = ".txt";
    extensions[2] = ".cfg";
    extensions[3] = ".levelshots";
    extensions[4] = ".menu";
    extensions[5] = ".arena";
    extensions[6] = ".str";
    extensions[7] = "";

    length = I_strlen( filename );

    for ( i = 0; extensions[i][0] != '\0'; i++ )
    {
        if ( !Q_stricmp( filename + length - strlen( extensions[i] ), extensions[i] ) )
            return qtrue;
    }

    return qfalse;
}

/* FS_FOpenFileReadDatabase  0x00469370 */
int FS_FOpenFileReadDatabase( const char *filename, fileHandle_t *file )
{
    return FS_FOpenFileReadForThread( filename, file, FS_THREAD_DATABASE );
}

/* FS_FOpenFileRead  0x00469390 */
int FS_FOpenFileRead( const char *filename, fileHandle_t *file )
{
    fs_fileAccessed = 1;

    return FS_FOpenFileReadForThread( filename, file, FS_THREAD_MAIN );
}

/* FS_FOpenFileReadCurrentThread  0x004693c0 */
int FS_FOpenFileReadCurrentThread( const char *filename, fileHandle_t *file )
{
    int thread;

    thread = FS_GetCurrentThread();
    if ( thread == FS_THREAD_INVALID )
    {
        Com_PrintError( CON_CHANNEL_ERROR, "FS_FOpenFileReadCurrentThread for an unknown thread\n" );

        if ( file )
            *file = 0;

        return -1;
    }

    return FS_FOpenFileReadForThread( filename, file, thread );
}

/* FS_FileExistsInSearchPaths  0x00469410 */
qboolean FS_FileExistsInSearchPaths( const char *filename )
{
    fileHandle_t f;

    FS_FOpenFileRead( filename, &f );
    if ( f )
        FS_FCloseFile( f );

    return f != 0;
}

/* FS_FindGamedirForFile  0x00469450 */
const char *FS_FindGamedirForFile( const char *qpath )
{
    char          netpath[MAX_OS_PATH_SHORT];
    searchpath_t *search;
    directory_t  *dir;
    FILE         *f;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( !FS_UseSearchPath( search ) )
            continue;

        if ( !search->dir )
            continue;

        dir = search->dir;

        FS_BuildOSPath( dir->path, dir->gamedir, qpath, netpath );

        f = fopen( netpath, "rb" );
        if ( f )
        {
            fclose( f );
            return va( "%s/%s", dir->gamedir, qpath );
        }
    }

    return NULL;
}

/* FS_Remove  0x00469530 */
void FS_Remove( const char *qpath )
{
    char ospath[MAX_OS_PATH_SHORT];

    FS_CheckInitialized();

    Assert( qpath );

    if ( qpath[0] == '\0' )
        return;

    FS_BuildOSPath( fs_homepath->current.string, fs_gamedir, qpath, ospath );
    remove( ospath );
}

/* FS_RemoveFromDir  0x004695d0 */
void FS_RemoveFromDir( const char *qpath, const char *gamedir )
{
    char ospath[MAX_OS_PATH_SHORT];

    FS_CheckInitialized();

    Assert( qpath );

    if ( qpath[0] == '\0' )
        return;

    FS_BuildOSPath( fs_homepath->current.string, gamedir, qpath, ospath );
    remove( ospath );
}

/* FS_SetFileReadOnly  0x00469670 */
void FS_SetFileReadOnly( const char *filename, qboolean readOnly )
{
    char  ospath[MAX_OS_PATH_SHORT];
    DWORD attributes;
    DWORD newAttributes;

    FS_CheckInitialized();

    Assert( filename && filename[0] );

    FS_BuildOSPath( fs_homepath->current.string, fs_gamedir, filename, ospath );

    attributes = GetFileAttributesA( ospath );

    if ( readOnly )
        newAttributes = attributes | FILE_ATTRIBUTE_READONLY;
    else
        newAttributes = attributes & ~FILE_ATTRIBUTE_READONLY;

    if ( newAttributes != attributes )
        SetFileAttributesA( ospath, newAttributes );
}

/* FS_Read  0x00469750 */
int FS_Read( void *buffer, int len, fileHandle_t f )
{
    FILE  *h;
    byte  *buf;
    int    remaining;
    size_t block;
    bool   tries;

    FS_CheckInitialized();

    if ( !f )
        return 0;

    if ( fsh[f].zipFile )
        return unzReadCurrentFile( fsh[f].handleFiles.file.z, buffer, len );

    h = FS_FileForHandle( f );

    buf = (byte *)buffer;
    tries = false;

    for ( remaining = len; remaining; remaining -= block )
    {
        block = fread( buf, 1, remaining, h );

        if ( block == 0 )
        {
            if ( tries )
                return len - remaining;

            tries = true;
        }

        if ( block == (size_t)-1 )
        {
            if ( f > 1 + 48 && f < 1 + 48 + 13 )
                return -1;

            Com_ErrorLevel( ERR_FATAL, "FS_Read: -1 bytes read" );
        }

        buf += block;
    }

    return len;
}

/* FS_Write  0x00469850 */
int FS_Write( const void *buffer, int len, fileHandle_t f )
{
    FILE       *h;
    const byte *buf;
    int         remaining;
    size_t      block;
    bool        tries;

    FS_CheckInitialized();

    if ( !f )
        return 0;

    h = FS_FileForHandle( f );

    buf = (const byte *)buffer;
    tries = false;

    for ( remaining = len; remaining; remaining -= block )
    {
        block = fwrite( buf, 1, remaining, h );

        if ( block == 0 )
        {
            if ( tries )
                return 0;

            tries = true;
        }

        if ( block == (size_t)-1 )
            return 0;

        buf += block;
    }

    if ( fsh[f].handleSync )
        fflush( h );

    return len;
}

/* FS_WriteNoResult  0x00469910 */
void FS_WriteNoResult( const void *buffer, int len, fileHandle_t f )
{
    FS_Write( buffer, len, f );
}

/* FS_Printf  0x00469930 */
void FS_Printf( fileHandle_t f, const char *fmt, ... )
{
    va_list argptr;
    char    msg[MAXPRINTMSG];

    va_start( argptr, fmt );
    _vsnprintf( msg, sizeof( msg ), fmt, argptr );
    va_end( argptr );

    FS_Write( msg, I_strlen( msg ), f );
}

/* FS_Seek  0x004699b0 */
int FS_Seek( fileHandle_t f, int offset, int origin )
{
    int result;
    int currentPosition;

    FS_CheckInitialized();

    Assert( !fsh[f].streamed );

    if ( !fsh[f].zipFile )
        return FS_FileSeek( FS_FileForHandle( f ), offset, origin );

    if ( offset == 0 && origin == FS_SEEK_SET )
    {
        unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
        return unzOpenCurrentFile( fsh[f].handleFiles.file.z );
    }

    if ( offset == 0 && origin == FS_SEEK_CUR )
        return 0;

    currentPosition = unztell( fsh[f].handleFiles.file.z );

    if ( origin == FS_SEEK_CUR )
    {
        Assert( offset != 0 );

        if ( offset < 0 )
        {
            unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
            unzOpenCurrentFile( fsh[f].handleFiles.file.z );
            currentPosition += offset;
        }
        else
        {
            currentPosition = offset;
        }
    }
    else if ( origin == FS_SEEK_END )
    {
        if ( FS_filelength( f ) + offset < currentPosition )
        {
            unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
            unzOpenCurrentFile( fsh[f].handleFiles.file.z );
            currentPosition = FS_filelength( f ) + offset;
        }
        else
        {
            currentPosition = FS_filelength( f ) + offset - currentPosition;
        }
    }
    else if ( origin == FS_SEEK_SET )
    {
        if ( offset < currentPosition )
        {
            unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
            unzOpenCurrentFile( fsh[f].handleFiles.file.z );
            currentPosition = offset;
        }
        else
        {
            currentPosition = offset - currentPosition;
        }
    }
    else
    {
        AssertMsg( va( "Bad origin %i in FS_Seek", origin ) );
        return -1;
    }

    result = unzReadCurrentFile( fsh[f].handleFiles.file.z, NULL, currentPosition );

    return result ? 0 : -1;
}

/* FS_ReadFile  0x00469ca0 */
int FS_ReadFile( const char *qpath, void **buffer )
{
    fileHandle_t h;
    int          len;
    byte        *buf;

    FS_CheckInitialized();

    if ( !qpath || !qpath[0] )
        Com_ErrorLevel( ERR_FATAL, "FS_ReadFile with empty name\n" );

    len = FS_FOpenFileRead( qpath, &h );

    if ( !h )
    {
        if ( buffer )
            *buffer = NULL;

        return -1;
    }

    if ( !buffer )
    {
        FS_FCloseFile( h );
        return len;
    }

    fs_loadStack++;

    buf = (byte *)FS_AllocLoadBuffer( len + 1 );
    *buffer = buf;

    FS_Read( buf, len, h );
    buf[len] = '\0';

    FS_FCloseFile( h );

    return len;
}

/* FS_AllocLoadBuffer  0x00469d70 */
void *FS_AllocLoadBuffer( int size )
{
    return Hunk_AllocateTempMemory( size );
}

/* FS_ClearLoadStack  0x00469d90 */
void FS_ClearLoadStack( void )
{
    fs_loadStack = 0;
}

/* FS_FreeFile  0x00469da0 */
void FS_FreeFile( void *buffer )
{
    FS_CheckInitialized();

    Assert( buffer );

    fs_loadStack--;

    FS_FreeLoadBuffer( buffer );
}

/* FS_FreeLoadBuffer  0x00469df0 */
void FS_FreeLoadBuffer( void *buffer )
{
    Hunk_FreeTempMemory( buffer );
}

/* FS_WriteFileToDir  0x00469e10 */
qboolean FS_WriteFileToDir( const char *filename, const char *gamedir,
                            const void *buffer, int size )
{
    fileHandle_t f;

    FS_CheckInitialized();

    Assert( filename );
    Assert( buffer );

    f = FS_FOpenFileWriteToDir( filename, gamedir );
    if ( !f )
    {
        Com_Printf( CON_CHANNEL_FILES, "Failed to open %s\n", filename );
        return qfalse;
    }

    if ( FS_Write( buffer, size, f ) == size )
    {
        FS_FCloseFile( f );
        return qtrue;
    }

    FS_FCloseFile( f );
    FS_Remove( filename );

    return qfalse;
}

/* FS_WriteFile  0x00469ee0 */
qboolean FS_WriteFile( const char *filename, const void *buffer, int size )
{
    fileHandle_t f;

    FS_CheckInitialized();

    Assert( filename );
    Assert( buffer );

    f = FS_FOpenFileWrite( filename );
    if ( !f )
    {
        Com_Printf( CON_CHANNEL_FILES, "Failed to open %s\n", filename );
        return qfalse;
    }

    if ( FS_Write( buffer, size, f ) == size )
    {
        FS_FCloseFile( f );
        return qtrue;
    }

    FS_FCloseFile( f );
    FS_Remove( filename );

    return qfalse;
}

/* FS_SV_GetFilepath  0x00469fb0 */
int FS_SV_GetFilepath( const char *filename, char *ospath )
{
    char          sanitizedName[MAX_OS_PATH_SHORT];
    searchpath_t *search;
    directory_t  *dir;
    FILE         *f;

    Assert( filename );
    Assert( ospath );

    if ( !FS_SanitizePath( filename, sanitizedName, sizeof( sanitizedName ) ) )
        return -1;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( !FS_UseSearchPath( search ) )
            continue;

        if ( search->iwd )
            continue;

        dir = search->dir;

        FS_BuildOSPathFull( dir->path, dir->gamedir, sanitizedName, ospath, 0 );

        f = fopen( ospath, "rb" );
        if ( f )
        {
            fclose( f );
            return 0;
        }
    }

    return -1;
}

/* FS_FOpenFileOverWrite  0x0046a0e0 */
fileHandle_t FS_FOpenFileOverWrite( const char *qpath )
{
    char  ospath[MAX_OS_PATH_SHORT];
    DWORD attributes;
    DWORD newAttributes;

    FS_CheckInitialized();

    Assert( qpath );

    if ( FS_SV_GetFilepath( qpath, ospath ) < 0 )
    {
        Com_ErrorLevel( ERR_DROP,
                        "FS_FOpenFileOverWrite: Failed to open %s for writing.  "
                        "It either does not exist or is in a iwd file.", qpath );
        return 0;
    }

    if ( fs_debug->current.integer )
        Com_Printf( CON_CHANNEL_FILES, "FS_FOpenFileOverWrite: %s\n", ospath );

    attributes = GetFileAttributesA( ospath );
    newAttributes = attributes & ~FILE_ATTRIBUTE_READONLY;
    if ( newAttributes != attributes )
        SetFileAttributesA( ospath, newAttributes );

    return FS_HandleOpenWrite( qpath, ospath, FS_THREAD_MAIN );
}

/* FS_ListFilteredFiles  0x0046a1d0 */
char **FS_ListFilteredFiles( searchpath_t *searchpaths, const char *path,
                             const char *extension, const char *filter,
                             int flags, int *numfiles, int allocType )
{
    char          ospath[MAX_OS_PATH_SHORT];
    char          zpath[MAX_OS_PATH_SHORT];
    char          foundName[MAX_QPATH];
    char          sanitizedPath[MAX_OS_PATH_SHORT];
    char        **sysFiles;
    char        **list;
    HunkUser     *user;
    searchpath_t *search;
    directory_t  *dir;
    iwd_t        *pak;
    fileInIwd_t  *buildBuffer;
    char         *name;
    int           numSysFiles;
    int           pathDepth;
    int           depth;
    int           nameDirLength;
    int           extensionLength;
    int           pathLength;
    int           nameLength;
    int           prefixLength;
    int           nfiles;
    qboolean      dirsOnly;
    int           i;

    FS_CheckInitialized();

    if ( !path )
    {
        *numfiles = 0;
        return NULL;
    }

    if ( !extension )
        extension = "";

    if ( !FS_SanitizePath( path, sanitizedPath, sizeof( sanitizedPath ) ) )
    {
        *numfiles = 0;
        return NULL;
    }

    dirsOnly = ( Q_stricmp( extension, "/" ) == 0 );

    pathLength = I_strlen( sanitizedPath );
    if ( pathLength && ( sanitizedPath[pathLength - 1] == '\\' ||
                         sanitizedPath[pathLength - 1] == '/' ) )
        pathLength--;

    extensionLength = I_strlen( extension );
    nfiles = 0;

    FS_ReturnPath( sanitizedPath, zpath, &pathDepth );
    if ( sanitizedPath[0] != '\0' )
        pathDepth++;

    user = Hunk_UserCreate( 0x20000, "FS_ListFilteredFiles", 0, 0, 3 );
    list = (char **)Hunk_UserAlloc( user, sizeof( char * ) * MAX_FOUND_FILES + sizeof( HunkUser * ), 4 );
    *(HunkUser **)list = user;
    list++;

    for ( search = searchpaths; search; search = search->next )
    {
        if ( !FS_UseSearchPath( search ) )
            continue;

        if ( !search->iwd )
        {
            if ( !search->dir )
                continue;

            dir = search->dir;

            FS_BuildOSPath( dir->path, dir->gamedir, sanitizedPath, ospath );
            sysFiles = Sys_ListFiles( ospath, extension, filter, &numSysFiles, dirsOnly );

            for ( i = 0; i < numSysFiles; i++ )
                nfiles = FS_AddFileToList( user, sysFiles[i], list, nfiles );

            Sys_FreeFileList( sysFiles );
            continue;
        }

        pak = search->iwd;
        buildBuffer = pak->buildBuffer;

        for ( i = 0; i < pak->numfiles; i++ )
        {
            name = buildBuffer[i].name;

            if ( filter )
            {
                if ( Com_FilterPath( filter, name, 0 ) )
                    nfiles = FS_AddFileToList( user, name, list, nfiles );

                continue;
            }

            nameDirLength = FS_ReturnPath( name, zpath, &depth );

            if ( depth != pathDepth || nameDirLength < pathLength )
                continue;
            if ( pathLength >= 1 && name[pathLength] != '/' )
                continue;
            if ( Q_stricmpn( name, sanitizedPath, pathLength ) )
                continue;

            if ( !dirsOnly )
            {
                if ( extensionLength )
                {
                    nameLength = I_strlen( name );

                    if ( nameLength <= extensionLength ||
                         name[nameLength - extensionLength - 1] != '.' ||
                         Q_stricmp( name + nameLength - extensionLength, extension ) )
                        continue;
                }
            }
            else
            {
                SanityCheck( extensionLength == 1 );
                SanityCheck( extension[0] == '/' && extension[1] == '\0' );

                if ( name[strlen( name ) - 1] != '/' )
                    continue;
            }

            prefixLength = pathLength;
            if ( pathLength )
                prefixLength = pathLength + 1;

            if ( !dirsOnly )
            {
                nfiles = FS_AddFileToList( user, name + prefixLength, list, nfiles );
            }
            else
            {
                strcpy( foundName, name + prefixLength );
                foundName[strlen( foundName ) - 1] = '\0';
                nfiles = FS_AddFileToList( user, foundName, list, nfiles );
            }
        }
    }

    *numfiles = nfiles;

    if ( !nfiles )
    {
        Hunk_UserDestroy( user );
        return NULL;
    }

    list[nfiles] = NULL;

    return list;
}

/* FS_ReturnPath  0x0046a7b0 */
static int FS_ReturnPath( const char *zname, char *zpath, int *depth )
{
    int len;
    int at;
    int newdep;

    newdep = 0;
    zpath[0] = '\0';
    len = 0;

    for ( at = 0; zname[at] != '\0'; at++ )
    {
        if ( zname[at] == '/' || zname[at] == '\\' )
        {
            len = at;
            newdep++;
        }
    }

    strcpy( zpath, zname );
    zpath[len] = '\0';

    if ( len + 1 == at )
        newdep--;

    *depth = newdep;

    return len;
}

/* FS_AddFileToList  0x0046a850 */
static int FS_AddFileToList( HunkUser *user, const char *name, char **list, int nfiles )
{
    int i;

    if ( nfiles == MAX_FOUND_FILES - 1 )
        return nfiles;

    for ( i = 0; i < nfiles; i++ )
    {
        if ( !Q_stricmp( name, list[i] ) )
            return nfiles;
    }

    list[nfiles] = Hunk_CopyString( user, name );

    return nfiles + 1;
}

/* FS_ListFiles  0x0046a8d0 */
char **FS_ListFiles( const char *path, const char *extension, int flags,
                     int *numfiles, int allocType )
{
    return FS_ListFilteredFiles( fs_searchpaths, path, extension, NULL,
                                 flags, numfiles, allocType );
}

/* FS_ListFilesInLocation  0x0046a900 */
char **FS_ListFilesInLocation( const char *path, const char *extension, int flags,
                               int *numfiles, int gameDirFlags, int allocType )
{
    return FS_ListFilteredFilesInLocation( path, extension, NULL, flags,
                                           numfiles, gameDirFlags, allocType );
}

/* FS_ListFilteredFilesInLocation  0x0046a930 */
char **FS_ListFilteredFilesInLocation( const char *path, const char *extension,
                                       const char *filter, int flags, int *numfiles,
                                       int gameDirFlags, int allocType )
{
    HunkUser     *user;
    searchpath_t *head;
    searchpath_t *tail;
    searchpath_t *search;
    const char   *pathDir;
    char        **list;

    user = Hunk_UserCreate( 0x20000, "FS_ListFilteredFilesInLocation", 0, 0, 3 );

    head = NULL;
    tail = NULL;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->dir )
            pathDir = search->dir->gamedir;
        else if ( search->iwd )
            pathDir = search->iwd->iwdGamename;
        else
            pathDir = NULL;

        Assert( pathDir );

        if ( !FS_IsGameDirAllowed( pathDir, gameDirFlags ) )
            continue;

        if ( !head )
        {
            head = (searchpath_t *)Hunk_UserAlloc( user, sizeof( searchpath_t ), 4 );
            tail = head;
        }
        else
        {
            tail->next = (searchpath_t *)Hunk_UserAlloc( user, sizeof( searchpath_t ), 4 );
            tail = tail->next;
        }

        tail->next = NULL;
        tail->dir = search->dir;
        tail->language = search->language;
        tail->localized = search->localized;
        tail->iwd = search->iwd;
    }

    list = FS_ListFilteredFiles( head, path, extension, filter, flags, numfiles, allocType );

    Hunk_UserDestroy( user );

    return list;
}

/* FS_IsGameDirAllowed  0x0046aab0 */
qboolean FS_IsGameDirAllowed( const char *gamedir, unsigned flags )
{
    if ( flags == FS_ALLOW_ALL )
        return qtrue;

    if ( ( flags & FS_ALLOW_MAIN )       && !Q_strncmp( gamedir, "main", 4 ) )
        return qtrue;
    if ( ( flags & FS_ALLOW_DEV )        && !Q_strncmp( gamedir, "dev", 3 ) )
        return qtrue;
    if ( ( flags & FS_ALLOW_TEMP )       && !Q_strncmp( gamedir, "temp", 4 ) )
        return qtrue;
    if ( ( flags & FS_ALLOW_RAW )        && !Q_strncmp( gamedir, "raw", 3 ) )
        return qtrue;
    if ( ( flags & FS_ALLOW_RAW_SHARED ) && !Q_strncmp( gamedir, "raw_shared", 10 ) )
        return qtrue;
    if ( ( flags & FS_ALLOW_DEVRAW )     && !Q_strncmp( gamedir, "devraw", 6 ) )
        return qtrue;

    return qfalse;
}

/* FS_FreeFileList  0x0046aba0 */
void FS_FreeFileList( char **list, int allocType )
{
    if ( !list )
        return;

    Hunk_UserDestroy( ( (HunkUser **)list )[-1] );
}

/* FS_GetFileList  0x0046abd0 */
int FS_GetFileList( const char *path, const char *extension, int flags,
                    char *listbuf, int bufsize )
{
    char **list;
    int    nFiles;
    int    nTotal;
    int    nLen;
    int    i;

    *listbuf = '\0';
    nFiles = 0;
    nTotal = 0;

    list = FS_ListFiles( path, extension, flags, &nFiles, 3 );

    for ( i = 0; i < nFiles; i++ )
    {
        nLen = I_strlen( list[i] ) + 1;

        if ( nTotal + nLen + 1 >= bufsize )
        {
            nFiles = i;
            break;
        }

        strcpy( listbuf, list[i] );
        listbuf += nLen;
        nTotal += nLen;
    }

    FS_FreeFileList( list, 3 );

    return nFiles;
}

/* FS_ConvertPath  0x0046aca0 */
void FS_ConvertPath( char *s )
{
    for ( ; *s != '\0'; s++ )
    {
        if ( *s == '\\' || *s == ':' )
            *s = '/';
    }
}

/* FS_PathCmp  0x0046ace0 */
int FS_PathCmp( const char *s1, const char *s2 )
{
    int c1;
    int c2;

    do
    {
        c1 = *s1++;
        c2 = *s2++;

        if ( Q_islower( c1 ) )
            c1 -= ( 'a' - 'A' );
        if ( Q_islower( c2 ) )
            c2 -= ( 'a' - 'A' );

        if ( c1 == '\\' || c1 == ':' )
            c1 = '/';
        if ( c2 == '\\' || c2 == ':' )
            c2 = '/';

        if ( c1 < c2 )
            return -1;
        if ( c2 < c1 )
            return 1;
    }
    while ( c1 );

    return 0;
}

/* FS_SortFileList  0x0046ada0 */
void FS_SortFileList( char **filelist, int numfiles )
{
    char **sortedlist;
    int    numsortedfiles;
    int    i;
    int    j;
    int    k;

    sortedlist = (char **)Z_Malloc( ( numfiles + 1 ) * sizeof( char * ) );
    sortedlist[0] = NULL;
    numsortedfiles = 0;

    for ( i = 0; i < numfiles; i++ )
    {
        for ( j = 0; j < numsortedfiles; j++ )
        {
            if ( FS_PathCmp( filelist[i], sortedlist[j] ) < 0 )
                break;
        }

        for ( k = numsortedfiles; k > j; k-- )
            sortedlist[k] = sortedlist[k - 1];

        sortedlist[j] = filelist[i];
        numsortedfiles++;
    }

    Com_Memcpy( filelist, sortedlist, numfiles * sizeof( char * ) );
    Z_Free( sortedlist );
}

/* FS_DisplayPath  0x0046aea0 */
void FS_DisplayPath( qboolean bLanguageCull )
{
    searchpath_t *search;
    int           i;

    if ( fs_ignoreLocalized->current.enabled )
        Com_Printf( CON_CHANNEL_FILES, "    localized assets are being ignored\n" );

    Com_Printf( CON_CHANNEL_FILES, "Current search path:\n" );

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( bLanguageCull && !FS_UseSearchPath( search ) )
            continue;

        if ( search->iwd )
            Com_Printf( CON_CHANNEL_FILES, "%s (%i files)\n",
                        search->iwd->iwdFilename, search->iwd->numfiles );
        else
            Com_Printf( CON_CHANNEL_FILES, "%s/%s\n",
                        search->dir->path, search->dir->gamedir );
    }

    Com_Printf( CON_CHANNEL_FILES, "\nFile Handles:\n" );

    for ( i = 1; i < MAX_FILE_HANDLES; i++ )
    {
        if ( fsh[i].handleFiles.file.o )
            Com_Printf( CON_CHANNEL_FILES, "handle %i: %s\n", i, fsh[i].name );
    }
}

/* FS_Path_f  0x0046afc0 */
void FS_Path_f( void )
{
    FS_DisplayPath( qfalse );
}

/* FS_DisplayFullPath  0x0046afd0 */
void FS_DisplayFullPath( void )
{
    FS_DisplayPath( qfalse );
}

/* FS_Shutdown  0x0046afe0 */
void FS_Shutdown( qboolean closemfp )
{
    int i;

    for ( i = 1; i < MAX_FILE_HANDLES; i++ )
    {
        if ( fsh[i].fileSize )
            FS_FCloseFile( i );
    }

    FS_FreeSearchPaths( fs_searchpaths );
    fs_searchpaths = NULL;
}

/* FS_FreeSearchPaths  0x0046b040 */
void FS_FreeSearchPaths( searchpath_t *searchpaths )
{
    searchpath_t *next;

    while ( searchpaths )
    {
        next = searchpaths->next;

        if ( searchpaths->iwd )
        {
            unzClose( searchpaths->iwd->handle );
            Z_Free( searchpaths->iwd->buildBuffer );
            Z_Free( searchpaths->iwd );
        }

        if ( searchpaths->dir )
            Z_Free( searchpaths->dir );

        Z_Free( searchpaths );
        searchpaths = next;
    }
}

/* FS_StartupFull  0x0046b0d0 */
void FS_StartupFull( const char *gameName )
{
    Com_Printf( CON_CHANNEL_FILES, "----- FS_Startup -----\n" );

    FS_RegisterDvars();

    if ( fs_basepath->current.string[0] )
    {
        FS_AddGameDirectories( fs_basepath->current.string, "devraw_shared" );
        FS_AddGameDirectories( fs_basepath->current.string, "devraw" );
        FS_AddGameDirectories( fs_basepath->current.string, "raw_shared" );
        FS_AddGameDirectories( fs_basepath->current.string, "raw" );
    }

    if ( fs_homepath->current.string[0] )
    {
        if ( Q_stricmp( fs_basepath->current.string, fs_homepath->current.string ) )
        {
            FS_AddGameDirectories( fs_homepath->current.string, "devraw_shared" );
            FS_AddGameDirectories( fs_homepath->current.string, "devraw" );
            FS_AddGameDirectories( fs_homepath->current.string, "raw_shared" );
            FS_AddGameDirectories( fs_homepath->current.string, "raw" );
        }
    }

    if ( fs_cdpath->current.string[0] )
    {
        if ( Q_stricmp( fs_basepath->current.string, fs_cdpath->current.string ) )
        {
            FS_AddGameDirectories( fs_cdpath->current.string, "devraw_shared" );
            FS_AddGameDirectories( fs_cdpath->current.string, "devraw" );
            FS_AddGameDirectories( fs_cdpath->current.string, "raw_shared" );
            FS_AddGameDirectories( fs_cdpath->current.string, "raw" );
            FS_AddGameDirectories( fs_cdpath->current.string, gameName );
        }
    }

    if ( fs_basepath->current.string[0] )
    {
        FS_AddGameDirectories( fs_basepath->current.string, va( "%s_shared", gameName ) );
        FS_AddGameDirectories( fs_basepath->current.string, gameName );
    }

    if ( fs_basepath->current.string[0] )
    {
        if ( Q_stricmp( fs_homepath->current.string, fs_basepath->current.string ) )
        {
            FS_AddGameDirectories( fs_basepath->current.string, va( "%s_shared", gameName ) );
            FS_AddGameDirectories( fs_homepath->current.string, gameName );
        }
    }

    if ( fs_basegame->current.string[0] &&
         !Q_stricmp( gameName, "main" ) &&
         Q_stricmp( fs_basegame->current.string, gameName ) )
    {
        if ( fs_cdpath->current.string[0] )
            FS_AddGameDirectories( fs_cdpath->current.string, fs_basegame->current.string );

        if ( fs_basepath->current.string[0] )
            FS_AddGameDirectories( fs_basepath->current.string, fs_basegame->current.string );

        if ( fs_homepath->current.string[0] &&
             Q_stricmp( fs_homepath->current.string, fs_basepath->current.string ) )
            FS_AddGameDirectories( fs_homepath->current.string, fs_basegame->current.string );
    }

    if ( fs_gameDirVar->current.string[0] &&
         !Q_stricmp( gameName, "main" ) &&
         Q_stricmp( fs_gameDirVar->current.string, gameName ) )
    {
        if ( fs_cdpath->current.string[0] )
            FS_AddGameDirectories( fs_cdpath->current.string, fs_gameDirVar->current.string );

        if ( fs_basepath->current.string[0] )
            FS_AddGameDirectories( fs_basepath->current.string, fs_gameDirVar->current.string );

        if ( fs_homepath->current.string[0] &&
             Q_stricmp( fs_homepath->current.string, fs_basepath->current.string ) )
            FS_AddGameDirectories( fs_homepath->current.string, fs_gameDirVar->current.string );
    }

    FS_DisplayFullPath();

    Dvar_ClearModified( fs_gameDirVar );

    Com_Printf( CON_CHANNEL_FILES, "----------------------\n" );
    Com_Printf( CON_CHANNEL_FILES, "%d files in iwd files\n", fs_iwdFileCount );
}

/* FS_AddGameDirectories  0x0046b560 */
void FS_AddGameDirectories( const char *path, const char *dir )
{
    FS_AddGameDirectory( path, dir, 1, 0 );
    FS_AddGameDirectory( path, dir, 0, 0 );
}

/* FS_AddGameDirectory  0x0046b590 */
void FS_AddGameDirectory( const char *path, const char *dir, int bLanguageDirectory, int iLanguage )
{
    char          gamedir[MAX_QPATH];
    char          ospath[MAX_OS_PATH_SHORT];
    searchpath_t *search;
    const char   *languageName;
    const char   *localizedText;

    if ( bLanguageDirectory )
    {
        languageName = "english";
        Com_sprintf( gamedir, sizeof( gamedir ), "%s/%s", dir, languageName );
    }
    else
    {
        I_strncpyz( gamedir, dir, sizeof( gamedir ) );
    }

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( !search->dir )
            continue;
        if ( Q_stricmp( search->dir->path, path ) )
            continue;
        if ( Q_stricmp( search->dir->gamedir, gamedir ) )
            continue;

        if ( search->localized != bLanguageDirectory )
        {
            if ( search->localized )
                localizedText = "localized";
            else
                localizedText = "non-localized";

            Com_PrintWarning( CON_CHANNEL_FILES,
                              "WARNING: game folder %s/%s added as both localized & "
                              "non-localized. Using folder as %s\n",
                              path, gamedir, localizedText );
        }

        if ( search->localized && search->language != iLanguage )
            Com_PrintWarning( CON_CHANNEL_FILES,
                              "WARNING: game folder %s/%s re-added as localized folder "
                              "with different language\n", path, gamedir );

        return;
    }

    if ( !bLanguageDirectory )
    {
        I_strncpyz( fs_gamedir, gamedir, sizeof( fs_gamedir ) );
    }
    else
    {
        FS_BuildOSPath( path, gamedir, "", ospath );
        ospath[strlen( ospath ) - 1] = '\0';

        if ( !Sys_DirectoryHasContents( ospath ) )
            return;
    }

    search = (searchpath_t *)Z_Malloc( sizeof( searchpath_t ) );
    search->dir = (directory_t *)Z_Malloc( sizeof( directory_t ) );

    I_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
    I_strncpyz( search->dir->gamedir, gamedir, sizeof( search->dir->gamedir ) );

    Assert( bLanguageDirectory || ( !bLanguageDirectory && !iLanguage ) );

    search->localized = bLanguageDirectory;
    search->language = iLanguage;
    search->iwd = NULL;
    search->isPlayersDir = ( Q_stricmp( dir, "players" ) == 0 );

    FS_AddSearchPath( search );

    FS_AddIwdFilesForGameDirectory( path, gamedir );
}

/* FS_AddSearchPath  0x0046b820 */
void FS_AddSearchPath( searchpath_t *search )
{
    searchpath_t **prev;

    prev = &fs_searchpaths;

    if ( search->localized )
    {
        while ( *prev && !( *prev )->localized )
            prev = &( *prev )->next;
    }

    search->next = *prev;
    *prev = search;
}

/* FS_AddIwdFilesForGameDirectory  0x0046b870 */
void FS_AddIwdFilesForGameDirectory( const char *path, const char *dir )
{
    char          ospath[MAX_OS_PATH_SHORT];
    char         *sorted[MAX_IWD_FILES];
    char        **iwdfiles;
    char         *curName;
    const char   *language;
    iwd_t        *pak;
    searchpath_t *search;
    int           numfiles;
    int           localized;
    int           iLanguage;
    int           i;

    FS_BuildOSPath( path, dir, "", ospath );
    ospath[strlen( ospath ) - 1] = '\0';

    iwdfiles = Sys_ListFiles( ospath, "iwd", NULL, &numfiles, 0 );

    if ( (int)numfiles > MAX_IWD_FILES )
    {
        Com_PrintWarning( CON_CHANNEL_FILES,
                          "WARNING: Exceeded max number of iwd files in %s/%s (%1/%1)\n",
                          path, dir, numfiles, MAX_IWD_FILES );
        numfiles = MAX_IWD_FILES;
    }

    for ( i = 0; i < (int)numfiles; i++ )
    {
        sorted[i] = iwdfiles[i];

        if ( !Q_strncmp( sorted[i], IWD_LOCALIZED_PREFIX, IWD_LOCALIZED_PREFIX_LEN ) )
            memcpy( sorted[i], IWD_LOCALIZED_BLANK, IWD_LOCALIZED_PREFIX_LEN );
    }

    qsort_vc8( sorted, numfiles, sizeof( char * ), iwdsort );

    for ( i = 0; i < (int)numfiles; i++ )
    {
        curName = sorted[i];

        if ( !Q_strncmp( curName, IWD_LOCALIZED_BLANK, IWD_LOCALIZED_PREFIX_LEN ) )
        {
            memcpy( curName, IWD_LOCALIZED_PREFIX, IWD_LOCALIZED_PREFIX_LEN );
            localized = 1;

            language = IwdFileLanguage( curName );
            if ( !language[0] )
            {
                Com_PrintWarning( CON_CHANNEL_FILES,
                                  "WARNING: Localized assets iwd file %s/%s/%s has invalid name "
                                  "(no language specified). Proper naming convention is: "
                                  "localized_[language]_iwd#.iwd\n",
                                  path, dir, curName );
                continue;
            }

            if ( Q_stricmp( language, "english" ) )
                continue;
        }
        else
        {
            localized = 0;
        }

        iLanguage = 0;

        FS_BuildOSPath( path, dir, curName, ospath );

        pak = FS_LoadZipFile( ospath, curName );
        if ( !pak )
            continue;

        strcpy( pak->iwdGamename, dir );

        search = (searchpath_t *)Z_Malloc( sizeof( searchpath_t ) );
        search->iwd = pak;
        search->localized = localized;
        search->language = iLanguage;

        FS_AddSearchPath( search );
    }

    Sys_FreeFileList( iwdfiles );
}

/* FS_LoadZipFile  0x0046bbb0 */
iwd_t *FS_LoadZipFile( const char *zipfile, const char *basename )
{
    unz_file_info  file_info;
    unz_global_info gi;
    char           filename_inzip[MAX_OS_PATH_SHORT];
    unzFile        uf;
    iwd_t         *pak;
    fileInIwd_t   *buildBuffer;
    char          *namePtr;
    int           *fs_headerLongs;
    int            fs_numHeaderLongs;
    unsigned       numfiles;
    unsigned       hashSize;
    unsigned       hash;
    int            namePoolSize;
    unsigned       i;

    fs_numHeaderLongs = 0;

    uf = unzOpen( zipfile );
    if ( unzGetGlobalInfo( uf, &gi ) )
        return NULL;

    numfiles = gi.number_entry;
    fs_iwdFileCount += numfiles;

    namePoolSize = 0;
    unzGoToFirstFile( uf );
    for ( i = 0; i < numfiles; i++ )
    {
        if ( unzGetCurrentFileInfo( uf, &file_info, filename_inzip,
                                    sizeof( filename_inzip ), NULL, 0, NULL, 0 ) )
            break;

        namePoolSize += I_strlen( filename_inzip ) + 1;
        unzGoToNextFile( uf );
    }

    buildBuffer = (fileInIwd_t *)Z_Malloc( numfiles * sizeof( fileInIwd_t ) + namePoolSize );
    namePtr = (char *)( buildBuffer + numfiles );

    fs_headerLongs = (int *)Z_Malloc( numfiles * sizeof( int ) );

    for ( hashSize = 1; hashSize <= MAX_IWD_HASH_SIZE && hashSize <= numfiles; hashSize <<= 1 )
        ;

    pak = (iwd_t *)Z_Malloc( hashSize * sizeof( fileInIwd_t * ) + sizeof( iwd_t ) );
    pak->hashSize = hashSize;
    pak->hashTable = (fileInIwd_t **)( pak + 1 );

    for ( i = 0; i < (unsigned)pak->hashSize; i++ )
        pak->hashTable[i] = NULL;

    I_strncpyz( pak->iwdFilename, zipfile, sizeof( pak->iwdFilename ) );
    I_strncpyz( pak->iwdBasename, basename, sizeof( pak->iwdBasename ) );

    if ( strlen( pak->iwdBasename ) > 4 &&
         !Q_stricmp( pak->iwdBasename + strlen( pak->iwdBasename ) - 4, ".iwd" ) )
        pak->iwdBasename[strlen( pak->iwdBasename ) - 4] = '\0';

    pak->handle = uf;
    pak->numfiles = numfiles;
    pak->hasOpenFile = 0;

    unzGoToFirstFile( uf );
    for ( i = 0; i < numfiles; i++ )
    {
        if ( unzGetCurrentFileInfo( uf, &file_info, filename_inzip,
                                    sizeof( filename_inzip ), NULL, 0, NULL, 0 ) )
            break;

        if ( file_info.uncompressed_size )
            fs_headerLongs[fs_numHeaderLongs++] = LittleLong( file_info.crc );

        Q_strlwr( filename_inzip );
        hash = FS_HashFileName( filename_inzip, pak->hashSize );

        buildBuffer[i].name = namePtr;
        strcpy( buildBuffer[i].name, filename_inzip );
        namePtr += strlen( filename_inzip ) + 1;

        unzGetCurrentFileInfoPosition( uf, &buildBuffer[i].pos );

        buildBuffer[i].next = pak->hashTable[hash];
        pak->hashTable[hash] = &buildBuffer[i];

        unzGoToNextFile( uf );
    }

    Z_Free( fs_headerLongs );

    pak->buildBuffer = buildBuffer;

    return pak;
}

/* IwdFileLanguage  0x0046c040 */
const char *IwdFileLanguage( const char *iwdName )
{
    char *buf;
    int   i;

    g_langBufToggle ^= 1;
    buf = g_langBuf[g_langBufToggle];

    if ( strlen( iwdName ) < IWD_LOCALIZED_PREFIX_LEN )
    {
        buf[0] = '\0';
        return buf;
    }

    memset( buf, 0, sizeof( g_langBuf[0] ) );

    for ( i = IWD_LOCALIZED_PREFIX_LEN;
          i < (int)sizeof( g_langBuf[0] ) && iwdName[i] != '\0';
          i++ )
    {
        if ( !isalpha( iwdName[i] ) )
            break;

        buf[i - IWD_LOCALIZED_PREFIX_LEN] = iwdName[i];
    }

    return buf;
}

/* iwdsort  0x0046c100 */
static int iwdsort( const void *a, const void *b )
{
    const char *name1;
    const char *name2;
    const char *lang1;
    const char *lang2;

    name1 = *(const char **)a;
    name2 = *(const char **)b;

    if ( !Q_strncmp( name1, IWD_LOCALIZED_BLANK, IWD_LOCALIZED_PREFIX_LEN ) &&
         !Q_strncmp( name2, IWD_LOCALIZED_BLANK, IWD_LOCALIZED_PREFIX_LEN ) )
    {
        lang1 = IwdFileLanguage( name1 );
        lang2 = IwdFileLanguage( name2 );

        if ( !Q_stricmp( lang1, "english" ) )
        {
            if ( Q_stricmp( lang2, "english" ) )
                return -1;
        }
        else
        {
            if ( !Q_stricmp( lang2, "english" ) )
                return 1;
        }
    }

    return FS_PathCmp( name1, name2 );
}

/* FS_RegisterDvars  0x0046c1d0 */
void FS_RegisterDvars( void )
{
    const char *homePath;

    fs_debug = Dvar_RegisterInt( "fs_debug", 0, 0, 2, 0,
                                 "Enable file system debugging information" );
    fs_copyfiles = Dvar_RegisterBool( "fs_copyfiles", 0, DVAR_INIT,
                                      "Copy all used files to another location" );
    fs_cdpath = Dvar_RegisterString( "fs_cdpath", Sys_DefaultCDPath(), DVAR_INIT,
                                     "CD path" );
    fs_basepath = Dvar_RegisterString( "fs_basepath", Sys_Cwd(), 0x210,
                                       "Base game path" );
    fs_basegame = Dvar_RegisterString( "fs_basegame", "", DVAR_INIT,
                                       "Base game name" );
    fs_gameDirVar = Dvar_RegisterString( "fs_game", "", 0x1c,
                                         "Game data directory. Must be \"\" or a sub directory of 'mods/'." );

    Dvar_SetDomainFunc( fs_gameDirVar, FS_GameDirDomainFunc );
    FS_ConvertGameDirSeparators();

    fs_ignoreLocalized = Dvar_RegisterBool( "fs_ignoreLocalized", 0, 0xa0,
                                            "Ignore localized files" );

    homePath = Sys_DefaultHomePath();
    if ( !homePath || !homePath[0] )
        homePath = fs_basepath->reset.string;

    fs_homepath = Dvar_RegisterString( "fs_homepath", homePath, DVAR_INIT,
                                       "Game home path" );
}

/* FS_GameDirDomainFunc  0x0046c310 */
static bool FS_GameDirDomainFunc( Dvar_t *dvar, DvarValue_t newValue )
{
    Assert( dvar );

    if ( !newValue.string[0] )
        return true;

    if ( Q_stricmpn( newValue.string, "mods", 4 ) )
    {
        FS_GameDirDomainError( dvar, newValue );
        return false;
    }

    if ( (unsigned)I_strlen( newValue.string ) < 6 ||
         ( newValue.string[4] != '/' && newValue.string[4] != '\\' ) )
    {
        FS_GameDirDomainError( dvar, newValue );
        return false;
    }

    if ( strstr( newValue.string, ".." ) || strstr( newValue.string, "::" ) )
    {
        FS_GameDirDomainError( dvar, newValue );
        return false;
    }

    return true;
}

/* FS_GameDirDomainError  0x0046c450 */
static void FS_GameDirDomainError( Dvar_t *dvar, DvarValue_t newValue )
{
    Assert( dvar );
    Assert( newValue.string[0] );

    Com_ErrorLevel( ERR_DROP, "ERROR: Invalid server value '%s' for '%s'\n\n",
                    newValue.string, dvar->name );
}

/* FS_ConvertGameDirSeparators  0x0046c4d0 */
static void FS_ConvertGameDirSeparators( void )
{
    char gameDir[MAX_OS_PATH_SHORT + 4];
    int  length;
    int  i;
    bool changed;

    changed = false;

    Assert( fs_gameDirVar->flags & DVAR_INIT );

    length = I_strlen( fs_gameDirVar->current.string );
    I_strncpyz( gameDir, fs_gameDirVar->current.string, sizeof( gameDir ) );

    for ( i = 0; i < length; i++ )
    {
        if ( gameDir[i] == '\\' )
        {
            gameDir[i] = '/';
            changed = true;
        }
    }

    if ( changed )
        Dvar_SetString( fs_gameDirVar, gameDir );
}

/* FS_ClearIwdReferences  0x0046c5d0 */
void FS_ClearIwdReferences( void )
{
    searchpath_t *search;

    for ( search = fs_searchpaths; search; search = search->next )
    {
        if ( search->iwd )
            search->iwd->referenced = 0;
    }
}

/* FS_VerifyFileSysCheck  0x0046c610 */
static qboolean FS_VerifyFileSysCheck( void )
{
    return FS_ReadFile( "fileSysCheck.cfg", NULL ) > 0;
}

/* FS_InitFilesystem  0x0046c630 */
void FS_InitFilesystem( void )
{
    FS_StartupFull( "main" );

    if ( !FS_VerifyFileSysCheck() )
        Com_ErrorLevel( ERR_FATAL,
                        "Couldn't load %s.  Make sure Call of Duty is run from the correct folder.",
                        "fileSysCheck.cfg" );

    I_strncpyz( g_savedFsBasepath, fs_basepath->current.string, sizeof( g_savedFsBasepath ) );
    I_strncpyz( g_savedFsGame, fs_gameDirVar->current.string, sizeof( g_savedFsGame ) );
}

/* FS_Restart  0x0046c6a0 */
void FS_Restart( int localClientNum, int checksumFeed )
{
    FS_Shutdown( qfalse );
    FS_ClearIwdReferences();

    FS_StartupFull( "main" );

    if ( FS_ReadFile( "default.cfg", NULL ) <= 0 )
    {
        if ( g_savedFsBasepath[0] )
        {
            Dvar_SetString( fs_basepath, g_savedFsBasepath );
            Dvar_SetString( fs_gameDirVar, g_savedFsGame );
            g_savedFsBasepath[0] = '\0';
            g_savedFsGame[0] = '\0';

            FS_Restart( localClientNum, checksumFeed );
            Com_ErrorLevel( ERR_DROP, "Invalid game folder\n" );
        }

        Com_ErrorLevel( ERR_FATAL,
                        "Couldn't load %s.  Make sure Call of Duty is run from the correct folder.",
                        "default.cfg" );
    }

    I_strncpyz( g_savedFsBasepath, fs_basepath->current.string, sizeof( g_savedFsBasepath ) );
    I_strncpyz( g_savedFsGame, fs_gameDirVar->current.string, sizeof( g_savedFsGame ) );
}

/* FS_GameDirModified  0x0046c780 */
qboolean FS_GameDirModified( void )
{
    return fs_gameDirVar->modified;
}

/* FS_ConditionalRestart  0x0046c7a0 */
qboolean FS_ConditionalRestart( int localClientNum, int checksumFeed )
{
    if ( !FS_GameDirModified() )
        return qfalse;

    FS_Restart( localClientNum, checksumFeed );

    return qtrue;
}

/* FSH_FOpenFile  0x0046c7d0 */
int FSH_FOpenFile( const char *qpath, fileHandle_t *f, int mode )
{
    int r;
    int sync;

    r = 6969;
    sync = 0;

    switch ( mode )
    {
    case FS_READ:
        r = FS_FOpenFileRead( qpath, f );
        break;

    case FS_WRITE:
        *f = FS_FOpenFileWrite( qpath );
        r = 0;
        if ( !*f )
            r = -1;
        break;

    case FS_APPEND_SYNC:
        sync = 1;

    case FS_APPEND:
        *f = FS_FOpenFileAppend( qpath );
        r = 0;
        if ( !*f )
            r = -1;
        break;

    default:
        Com_ErrorLevel( ERR_FATAL, "FSH_FOpenFile: bad mode" );
        break;
    }

    if ( f )
    {
        if ( *f )
        {
            fsh[*f].fileSize = r;
            fsh[*f].streamed = 0;
        }

        fsh[*f].handleSync = sync;
    }

    return r;
}

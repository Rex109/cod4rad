/* Original: ..\src\universal\com_files.cpp */

#ifndef COM_FILES_H
#define COM_FILES_H

#include "q_shared.h"

typedef struct Dvar_s Dvar_t;

#define MAX_FILE_HANDLES        (1 + 48 + 13 + 1 + 1 + 1)

#define MAX_IWD_HASH_SIZE       1024

#define MAX_IWD_FILES           1024

#define IWD_LOCALIZED_PREFIX    "localized_"
#define IWD_LOCALIZED_BLANK     "          "
#define IWD_LOCALIZED_PREFIX_LEN 10

typedef int fileHandle_t;

#define FS_THREAD_MAIN          0
#define FS_THREAD_STREAM        1
#define FS_THREAD_DATABASE      2
#define FS_THREAD_BACKEND       3
#define FS_THREAD_INVALID       6

#define FS_READ                 0
#define FS_WRITE                1
#define FS_APPEND               2
#define FS_APPEND_SYNC          3

#define FS_SEEK_CUR             0
#define FS_SEEK_END             1
#define FS_SEEK_SET             2

typedef struct
{
    union
    {
        FILE *o;                    /* +0x00 */
        void *z;                    /* +0x00 */
    } file;
    int unique;                     /* +0x04 */
} fileHandleFiles_t;

typedef struct
{
    fileHandleFiles_t handleFiles;  /* +0x00 */
    int               handleSync;   /* +0x08 */
    int               fileSize;     /* +0x0c */
    int               zipFilePos;   /* +0x10 */
    struct iwd_s     *zipFile;      /* +0x14 */
    int               streamed;     /* +0x18 */
    char              name[MAX_OS_PATH_SHORT]; /* +0x1c */
} fileHandleData_t;                 /* sizeof == 0x11c */

typedef struct fileInIwd_s
{
    unsigned long       pos;        /* +0x00 */
    char               *name;       /* +0x04 */
    struct fileInIwd_s *next;       /* +0x08 */
} fileInIwd_t;                      /* sizeof == 0x0c */

typedef struct
{
    char path[MAX_OS_PATH_SHORT];       /* +0x000 */
    char gamedir[MAX_OS_PATH_SHORT];    /* +0x100 */
} directory_t;

typedef struct iwd_s
{
    char          iwdFilename[MAX_OS_PATH_SHORT];  /* +0x000 */
    char          iwdBasename[MAX_OS_PATH_SHORT];  /* +0x100 */
    char          iwdGamename[MAX_OS_PATH_SHORT];  /* +0x200 */
    void         *handle;           /* +0x300 */
    int           hasOpenFile;      /* +0x304 */
    int           numfiles;         /* +0x308 */
    byte          referenced;       /* +0x30c */
    int           hashSize;         /* +0x310 */
    fileInIwd_t **hashTable;        /* +0x314 */
    fileInIwd_t  *buildBuffer;      /* +0x318 */
} iwd_t;                            /* sizeof == 0x31c */

typedef struct searchpath_s
{
    struct searchpath_s *next;      /* +0x00 */
    iwd_t               *iwd;       /* +0x04 */
    directory_t         *dir;       /* +0x08 */
    int                  localized; /* +0x0c */
    int                  unknown10; /* +0x10 */
    int                  isPlayersDir; /* +0x14 */
    int                  language;  /* +0x18 */
} searchpath_t;                     /* sizeof == 0x1c */

#define FS_ALLOW_MAIN           0x01
#define FS_ALLOW_DEV            0x02
#define FS_ALLOW_TEMP           0x04
#define FS_ALLOW_RAW            0x08
#define FS_ALLOW_RAW_SHARED     0x10
#define FS_ALLOW_DEVRAW         0x20
#define FS_ALLOW_ALL            0x3f

extern searchpath_t    *fs_searchpaths;                     /* 0x2b925604 */
extern fileHandleData_t fsh[MAX_FILE_HANDLES];              /* 0x2b920de8 */
extern char             fs_gamedir[MAX_OS_PATH_SHORT];      /* 0x2b920ac8 */
extern int              fs_iwdFileCount;                    /* 0x2b925608 */

extern Dvar_t          *fs_debug;                           /* 0x2b920bd0 */
extern Dvar_t          *fs_copyfiles;                       /* 0x2b920ab8 */
extern Dvar_t          *fs_cdpath;                          /* 0x2b920abc */
extern Dvar_t          *fs_basepath;                        /* 0x2b920cd8 */
extern Dvar_t          *fs_basegame;                        /* 0x2b920ac0 */
extern Dvar_t          *fs_gameDirVar;                      /* 0x2b920bc8 */
extern Dvar_t          *fs_ignoreLocalized;                 /* 0x2b920ac4 */
extern Dvar_t          *fs_homepath;                        /* 0x2b920de0 */

qboolean  FS_Initialized( void );                                       /* 0x004676a0 */
void      FS_CheckInitialized( void );                                  /* 0x004676c0 */
int       FS_LoadStack( void );                                         /* 0x004676e0 */
qboolean  FS_UseSearchPath( const searchpath_t *search );               /* 0x004676f0 */
qboolean  FS_LanguageHasAssets( int iLanguage );                        /* 0x00467720 */
unsigned  FS_HashFileName( const char *fname, int hashSize );           /* 0x00467760 */

fileHandle_t FS_HandleForFile( int thread );                            /* 0x004677f0 */
FILE     *FS_FileForHandle( fileHandle_t f );                           /* 0x004679c0 */
int       FS_filelength( fileHandle_t f );                              /* 0x00467a70 */
int       FS_GetCurrentThread( void );                                  /* 0x00468390 */

void      FS_ReplaceSeparators( char *path );                           /* 0x00467af0 */
void      FS_BuildOSPath( const char *base, const char *gamedir,
                          const char *qpath, char *ospath );            /* 0x00467b80 */
void      FS_BuildOSPathFull( const char *base, const char *gamedir, const char *qpath,
                              char *ospath, int okToFail );             /* 0x00467ba0 */
qboolean  FS_CreatePath( char *OSPath );                                /* 0x00467d10 */
qboolean  FS_SanitizePath( const char *filename, char *sanitizedName,
                           int sanitizedNameSize );                     /* 0x00469070 */
void      FS_ConvertPath( char *s );                                    /* 0x0046aca0 */
int       FS_PathCmp( const char *s1, const char *s2 );                 /* 0x0046ace0 */
int       FS_FilenameCompare( const char *s1, const char *s2 );          /* 0x00468640 */

qboolean  FS_FileExists( const char *file );                            /* 0x00467db0 */
void      FS_CopyFile( const char *fromOSPath, const char *toOSPath );  /* 0x00467e30 */
void      FS_RemoveOSPath( const char *osPath );                        /* 0x00467f40 */
void      FS_Rename( const char *from, const char *fromGamedir,
                     const char *to, const char *toGamedir );           /* 0x00467f60 */
qboolean  FS_FileCompare( const char *path0, const char *path1 );       /* 0x004686f0 */
char     *FS_ShiftedStrStr( const char *string, const char *substring, int shift ); /* 0x004688b0 */

void      FS_FCloseFile( fileHandle_t f );                              /* 0x00468060 */
void      FSH_FCloseFile( fileHandle_t f );                             /* 0x004681b0 */
fileHandle_t FS_FOpenFileWriteToDir( const char *qpath, const char *dir );          /* 0x004681d0 */
fileHandle_t FS_FOpenFileWriteToDirForThread( const char *qpath, const char *dir,
                                              int thread );             /* 0x004681f0 */
fileHandle_t FS_FOpenFileWrite( const char *qpath );                    /* 0x00468330 */
fileHandle_t FS_FOpenFileWriteCurrentThread( const char *qpath );       /* 0x00468350 */
fileHandle_t FS_FOpenTextFileWrite( const char *qpath );                /* 0x004683b0 */
fileHandle_t FS_FOpenFileAppend( const char *qpath );                   /* 0x004684d0 */
fileHandle_t FS_FOpenFileOverWrite( const char *qpath );                /* 0x0046a0e0 */

int       FS_FOpenFileReadForThread( const char *filename, fileHandle_t *file, int thread ); /* 0x00468950 */
int       FS_FOpenFileReadStream( const char *filename, fileHandle_t *file );      /* 0x00468930 */
int       FS_FOpenFileReadDatabase( const char *filename, fileHandle_t *file );    /* 0x00469370 */
int       FS_FOpenFileRead( const char *filename, fileHandle_t *file );            /* 0x00469390 */
int       FS_FOpenFileReadCurrentThread( const char *filename, fileHandle_t *file );/* 0x004693c0 */
int       FSH_FOpenFile( const char *qpath, fileHandle_t *f, int mode );           /* 0x0046c7d0 */

qboolean  FS_FileExistsInSearchPaths( const char *filename );           /* 0x00469410 */
const char *FS_FindGamedirForFile( const char *qpath );                 /* 0x00469450 */
int       FS_SV_GetFilepath( const char *filename, char *ospath );      /* 0x00469fb0 */

int       FS_Read( void *buffer, int len, fileHandle_t f );             /* 0x00469750 */
int       FS_Write( const void *buffer, int len, fileHandle_t f );      /* 0x00469850 */
void      FS_WriteNoResult( const void *buffer, int len, fileHandle_t f ); /* 0x00469910 */
void      FS_Printf( fileHandle_t f, const char *fmt, ... );            /* 0x00469930 */
int       FS_Seek( fileHandle_t f, int offset, int origin );            /* 0x004699b0 */

int       FS_ReadFile( const char *qpath, void **buffer );              /* 0x00469ca0 */
void     *FS_AllocLoadBuffer( int size );                               /* 0x00469d70 */
void      FS_ClearLoadStack( void );                                    /* 0x00469d90 */
void      FS_FreeFile( void *buffer );                                  /* 0x00469da0 */
void      FS_FreeLoadBuffer( void *buffer );                            /* 0x00469df0 */
qboolean  FS_WriteFileToDir( const char *filename, const char *gamedir,
                             const void *buffer, int size );            /* 0x00469e10 */
qboolean  FS_WriteFile( const char *filename, const void *buffer, int size ); /* 0x00469ee0 */

void      FS_Remove( const char *qpath );                               /* 0x00469530 */
void      FS_RemoveFromDir( const char *qpath, const char *gamedir );   /* 0x004695d0 */
void      FS_SetFileReadOnly( const char *filename, qboolean readOnly ); /* 0x00469670 */

char    **FS_ListFilteredFiles( searchpath_t *searchpaths, const char *path,
                                const char *extension, const char *filter,
                                int flags, int *numfiles, int allocType );          /* 0x0046a1d0 */
char    **FS_ListFiles( const char *path, const char *extension,
                        int flags, int *numfiles, int allocType );                  /* 0x0046a8d0 */
char    **FS_ListFilesInLocation( const char *path, const char *extension, int flags,
                                  int *numfiles, int gameDirFlags, int allocType );  /* 0x0046a900 */
char    **FS_ListFilteredFilesInLocation( const char *path, const char *extension,
                                          const char *filter, int flags, int *numfiles,
                                          int gameDirFlags, int allocType );         /* 0x0046a930 */
void      FS_FreeFileList( char **list, int allocType );                             /* 0x0046aba0 */
int       FS_GetFileList( const char *path, const char *extension, int flags,
                          char *listbuf, int bufsize );                              /* 0x0046abd0 */
qboolean  FS_IsGameDirAllowed( const char *gamedir, unsigned flags );                /* 0x0046aab0 */
void      FS_SortFileList( char **list, int n );                                     /* 0x0046ada0 */

void      FS_DisplayPath( qboolean bLanguageCull );                     /* 0x0046aea0 */
void      FS_Path_f( void );                                            /* 0x0046afc0 */
void      FS_DisplayFullPath( void );                                   /* 0x0046afd0 */
void      FS_Shutdown( qboolean closemfp );                             /* 0x0046afe0 */
void      FS_FreeSearchPaths( searchpath_t *searchpaths );              /* 0x0046b040 */
void      FS_StartupFull( const char *gameName );                       /* 0x0046b0d0 */
void      FS_AddGameDirectories( const char *path, const char *dir );   /* 0x0046b560 */
void      FS_AddGameDirectory( const char *path, const char *dir,
                               int bLanguageDirectory, int iLanguage ); /* 0x0046b590 */
void      FS_AddSearchPath( searchpath_t *search );                     /* 0x0046b820 */
void      FS_AddIwdFilesForGameDirectory( const char *path, const char *dir ); /* 0x0046b870 */
iwd_t    *FS_LoadZipFile( const char *zipfile, const char *basename );  /* 0x0046bbb0 */
const char *IwdFileLanguage( const char *iwdName );                     /* 0x0046c040 */
void      FS_RegisterDvars( void );                                     /* 0x0046c1d0 */
void      FS_ClearIwdReferences( void );                                /* 0x0046c5d0 */
void      FS_InitFilesystem( void );                                    /* 0x0046c630 */
void      FS_Restart( int localClientNum, int checksumFeed );           /* 0x0046c6a0 */
qboolean  FS_GameDirModified( void );                                   /* 0x0046c780 */
qboolean  FS_ConditionalRestart( int localClientNum, int checksumFeed );/* 0x0046c7a0 */

#endif

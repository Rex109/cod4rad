/* Original: ..\libs\cmdlib\cmdlib.cpp */

#ifndef CMDLIB_H
#define CMDLIB_H

#include "q_shared.h"

#define MEM_PAGE_SIZE   4096


extern char  g_installDir[MAX_OS_PATH];             /* 0x11ba5138 */

typedef void (*ComErrorHandler_t)( const char *fmt, va_list argptr );
typedef void (*ComPrintHandler_t)( int channel, const char *fmt, va_list argptr );

extern ComErrorHandler_t g_errorHandler;            /* 0x11ba5538 */
extern ComErrorHandler_t g_printHandler;            /* 0x11ba553c */
extern ComPrintHandler_t g_channelPrintHandler;     /* 0x11ba5540 */
extern ComPrintHandler_t g_channelWarningHandler;   /* 0x11ba5544 */

extern int   verbose;                               /* 0x11ba5548 */

void   Com_SetErrorHandler( ComErrorHandler_t handler );
void   Com_SetPrintHandler( ComErrorHandler_t handler );
void   Com_SetChannelPrintHandler( ComPrintHandler_t handler );
void   Com_SetChannelWarningHandler( ComPrintHandler_t handler );

void  *SafeMallocNoAlign( int size );
void  *SafeMalloc( int size );

int    FS_FileLength( FILE *f );
FILE  *SafeOpenWrite( const char *filename );
FILE  *SafeOpenRead( const char *filename );
void   SafeRead( FILE *f, void *buffer, int count );
void   SafeWrite( FILE *f, const void *buffer, int count );
int    LoadFile( const char *filename, void **bufferptr );
int    TryLoadFile( const char *filename, void **bufferptr );
void   SaveFile( const char *filename, const void *buffer, int count );

void   DefaultExtension( char *path, const char *extension );
void   DefaultPath( char *path, const char *basepath );
void   StripFilename( char *path );
void   StripExtension( char *path );
void   ExtractFilePath( const char *path, char *dest );
void   ExtractFileName( const char *path, char *dest );
void   ExtractFileBase( const char *path, char *dest );
void   ExtractFileExtension( const char *path, char *dest );
void   ConvertDOSToUnixName( char *dst, const char *src );
void   ConvertUnixToDOSName( char *dst, const char *src );
char  *FS_GetPreviousPathComponent( const char *path, char *pos );
void   ReplacePathComponent( char *path, const char *oldName, const char *newName );
char  *ExpandArg( const char *path );
char  *ExpandPath( const char *path );
void   Q_getwd( char *out );

char  *copystring( const char *s );
void   Com_DPrintf( const char *fmt, ... );
void   Com_Printf( const char *fmt, ... );
void   FS_Startup( const char *exePath );
void   Com_InitFileSystem( const char *basePath, const char *game, const char *baseGame );
double I_FloatTime( void );

void Com_Error( const char *fmt, ... );                                 /* 0x0040ddf0 */

#endif

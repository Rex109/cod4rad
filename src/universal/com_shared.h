/* Original: ..\src\universal\com_shared.cpp */

#ifndef COM_SHARED_H
#define COM_SHARED_H

#include "q_shared.h"

#define CON_CHANNEL_ERROR       1
#define CON_CHANNEL_FILES       10
#define CON_CHANNEL_SYSTEM      16
#define CON_CHANNEL_PARSER      23

#define ERR_FATAL               0
#define ERR_DROP                1

void Com_ErrorLevel( int errorLevel, const char *fmt, ... );        /* 0x00408870 */
void Com_Printf( int channel, const char *fmt, ... );               /* 0x0040eec0 */
void Com_PrintError( int channel, const char *fmt, ... );           /* 0x0040ef10 */
void Com_PrintWarning( int channel, const char *fmt, ... );         /* 0x0040ef50 */

void     Com_Memcpy( void *dest, const void *src, int count );
void     Com_Memset4( void *dest, int fillValue, int count );
void     Com_Memset( void *dest, int fillByte, int byteCount );
qboolean Com_Memcmp( const void *a, const void *b, unsigned int count );

qboolean Com_Filter( const char *filter, const char *name, int casesensitive );
qboolean Com_FilterPath( const char *filter, const char *name, int casesensitive );

int      Com_HashKey( const char *string, int maxlen );

time_t   Com_Time( time_t *t );
struct tm *Com_LocalTime( const time_t *t );
int      Com_GetLocalTime( struct tm *localTime );

void Com_InitFilesystem( const char *basepath, const char *game, const char *basegame ); /* 0x0040ef90 */

#endif

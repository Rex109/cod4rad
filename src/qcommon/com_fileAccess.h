/* Original: ..\qcommon\com_fileAccess.h */

#ifndef COM_FILEACCESS_H
#define COM_FILEACCESS_H

#include "../universal/q_shared.h"
#include "../universal/assertive.h"
#include "../universal/com_files.h"
#include "../zlib/unzip.h"

/* FS_FTell  0x0046c8e0 */
long FS_FTell( fileHandle_t f )
{
    long pos;

    if ( fsh[f].zipFile )
        pos = unztell( fsh[f].handleFiles.file.z );
    else
        pos = ftell( FS_FileForHandle( f ) );

    return pos;
}

/* FS_Flush  0x0046c940 */
void FS_Flush( fileHandle_t f )
{
    fflush( FS_FileForHandle( f ) );
}

/* Com_GetBspFilename  0x0046c960 */
void Com_GetBspFilename( char *dest, int size, const char *mapname )
{
    Com_sprintf( dest, size, "maps/%s.d3dbsp", mapname );
}

/* FS_FileLength  0x0046c980 */
long FS_FileLength( FILE *h )
{
    long pos;
    long end;

    pos = ftell( h );
    fseek( h, 0, SEEK_END );
    end = ftell( h );
    fseek( h, pos, SEEK_SET );

    return end;
}

/* CompareExchange  0x0046c9d0 */
int CompareExchange( volatile int *dest, int exchange, int comparand )
{
    int old;

    old = *dest;
    if ( old == comparand )
        *dest = exchange;

    return old;
}

/* FS_IsSlash  0x0046ca00 */
qboolean FS_IsSlash( char c )
{
    if ( c == '/' || c == '\\' )
        return qtrue;

    return qfalse;
}

/* FS_FileSeek  0x0046ca30 */
int FS_FileSeek( FILE *h, long offset, int origin )
{
    int whence;

    if ( origin == FS_SEEK_CUR )
    {
        whence = SEEK_CUR;
    }
    else if ( origin == FS_SEEK_END )
    {
        whence = SEEK_END;
    }
    else if ( origin == FS_SEEK_SET )
    {
        whence = SEEK_SET;
    }
    else
    {
        AssertMsg( va( "Bad origin %i in FS_Seek", origin ) );
        return 0;
    }

    return fseek( h, offset, whence );
}

#endif

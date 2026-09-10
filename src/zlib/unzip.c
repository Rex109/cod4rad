/* Original: ..\src\zlib\unzip.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "unzip.h"

#ifndef local
#  define local static
#endif

/* unz_copyright  0x005100d8 */
const char unz_copyright[] =
   " unzip 0.15 Copyright 1998 Gilles Vollant ";

#define UNZ_BUFSIZE             (16384)

#define UNZ_MAXFILENAMEINZIP    (256)

#define ALLOC( size )   (malloc( size ))
#define TRYFREE( p )    {free( p );}

#define SIZECENTRALDIRITEM      (0x2e)
#define SIZEZIPLOCALHEADER      (0x1e)

#define BUFREADCOMMENT          (0x400)

#define CASESENSITIVITYDEFAULTVALUE (2)

#define ZLIB_FILEFUNC_SEEK_CUR  (0)
#define ZLIB_FILEFUNC_SEEK_END  (1)
#define ZLIB_FILEFUNC_SEEK_SET  (2)

typedef struct
{
    char         *read_buffer;              /* +0x00 */
    z_stream      stream;                   /* +0x04 */

    uLong         pos_in_zipfile;           /* +0x38 */
    uLong         stream_initialised;       /* +0x3c */

    uLong         offset_local_extrafield;  /* +0x40 */
    uInt          size_local_extrafield;    /* +0x44 */
    uLong         pos_local_extrafield;     /* +0x48 */

    uLong         rest_read_compressed;     /* +0x4c */
    uLong         rest_read_uncompressed;   /* +0x50 */
    FILE         *file;                     /* +0x54 */
    uLong         compression_method;       /* +0x58 */
    uLong         byte_before_the_zipfile;  /* +0x5c */
} file_in_zip_read_info_s;                  /* sizeof == 0x60 */

local int   strcmpcasenosensitive_internal( const char *fileName1, const char *fileName2 );
local int   unzlocal_getShort( FILE *fin, uLong *pX );
local int   unzlocal_getLong( FILE *fin, uLong *pX );
local uLong unzlocal_SearchCentralDir( FILE *fin );
local int   unzlocal_GetCurrentFileInfoInternal( unzFile file,
                                                 unz_file_info *pfile_info,
                                                 unz_file_info_internal *pfile_info_internal,
                                                 char *szFileName,
                                                 uLong fileNameBufferSize,
                                                 void *extraField,
                                                 uLong extraFieldBufferSize,
                                                 char *szComment,
                                                 uLong commentBufferSize );
local void  unzlocal_DosDateToTmuDate( uLong ulDosDate, tm_unz *ptm );
local int   unzlocal_CheckCurrentFileCoherencyHeader( unz_s *s,
                                                      uInt *piSizeVar,
                                                      uLong *poffset_local_extrafield,
                                                      uInt *psize_local_extrafield );
local long  fseek_file_func( FILE *file, long offset, int origin );
local long  unzlocal_GetFileSize( FILE *file );


/* unzStringFileNameCompare  0x0048be60 */
int unzStringFileNameCompare( const char *fileName1, const char *fileName2, int iCaseSensitivity )
{
    if ( iCaseSensitivity == 0 )
    {
        iCaseSensitivity = CASESENSITIVITYDEFAULTVALUE;
    }

    if ( iCaseSensitivity == 1 )
    {
        return strcmp( fileName1, fileName2 );
    }

    return strcmpcasenosensitive_internal( fileName1, fileName2 );
}


/* strcmpcasenosensitive_internal  0x0048bea0 */
local int strcmpcasenosensitive_internal( const char *fileName1, const char *fileName2 )
{
    for ( ;; )
    {
        char c1 = *( fileName1++ );
        char c2 = *( fileName2++ );

        if ( ( c1 >= 'a' ) && ( c1 <= 'z' ) )
        {
            c1 -= 0x20;
        }
        if ( ( c2 >= 'a' ) && ( c2 <= 'z' ) )
        {
            c2 -= 0x20;
        }
        if ( c1 == '\0' )
        {
            return ( ( c2 == '\0' ) ? 0 : -1 );
        }
        if ( c2 == '\0' )
        {
            return 1;
        }
        if ( c1 < c2 )
        {
            return -1;
        }
        if ( c1 > c2 )
        {
            return 1;
        }
    }
}


/* unzReOpen  0x0048bf50 */
unzFile unzReOpen( const char *path, unzFile file )
{
    unz_s *s;
    FILE  *fin;

    fin = fopen( path, "rb" );
    if ( fin == NULL )
    {
        return NULL;
    }

    s = (unz_s *)ALLOC( sizeof( unz_s ) );
    memcpy( s, file, sizeof( unz_s ) );

    s->file = fin;
    s->pfile_in_zip_read = NULL;

    return (unzFile)s;
}


/* unzOpen  0x0048bfc0 */
unzFile unzOpen( const char *path )
{
    unz_s  us;
    unz_s *s;
    uLong  central_pos, uL;
    FILE  *fin;

    uLong  number_disk;
    uLong  number_disk_with_CD;
    uLong  number_entry_CD;
    int    err = UNZ_OK;

    if ( unz_copyright[0] != ' ' )
    {
        return NULL;
    }

    fin = fopen( path, "rb" );
    if ( fin == NULL )
    {
        return NULL;
    }

    central_pos = unzlocal_SearchCentralDir( fin );
    if ( central_pos == 0 )
    {
        err = UNZ_ERRNO;
    }

    if ( fseek_file_func( fin, central_pos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( fin, &uL ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( fin, &number_disk ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( fin, &number_disk_with_CD ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( fin, &us.gi.number_entry ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( fin, &number_entry_CD ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( ( number_entry_CD != us.gi.number_entry ) ||
         ( number_disk_with_CD != 0 ) ||
         ( number_disk != 0 ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( unzlocal_getLong( fin, &us.size_central_dir ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( fin, &us.offset_central_dir ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( fin, &us.gi.size_comment ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( ( central_pos < us.offset_central_dir + us.size_central_dir ) &&
         ( err == UNZ_OK ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( err != UNZ_OK )
    {
        fclose( fin );
        return NULL;
    }

    us.file = fin;
    us.byte_before_the_zipfile = central_pos - ( us.offset_central_dir + us.size_central_dir );
    us.central_pos = central_pos;
    us.pfile_in_zip_read = NULL;

    s = (unz_s *)ALLOC( sizeof( unz_s ) );
    *s = us;
    unzGoToFirstFile( (unzFile)s );

    return (unzFile)s;
}


/* unzlocal_getShort  0x0048c1e0 */
local int unzlocal_getShort( FILE *fin, uLong *pX )
{
    short v;

    fread( &v, 1, sizeof( v ), fin );

    *pX = (uLong)v;
    return UNZ_OK;
}


/* unzlocal_getLong  0x0048c210 */
local int unzlocal_getLong( FILE *fin, uLong *pX )
{
    int v;

    fread( &v, 1, sizeof( v ), fin );

    *pX = (uLong)v;
    return UNZ_OK;
}


/* unzlocal_SearchCentralDir  0x0048c240 */
local uLong unzlocal_SearchCentralDir( FILE *fin )
{
    unsigned char *buf;
    uLong          uSizeFile;
    uLong          uBackRead;
    uLong          uMaxBack = 0xffff;
    uLong          uPosFound = 0;

    uSizeFile = unzlocal_GetFileSize( fin );

    if ( uMaxBack > uSizeFile )
    {
        uMaxBack = uSizeFile;
    }

    buf = (unsigned char *)ALLOC( BUFREADCOMMENT + 4 );
    if ( buf == NULL )
    {
        return 0;
    }

    uBackRead = 4;
    while ( uBackRead < uMaxBack )
    {
        uLong uReadSize, uReadPos;
        int   i;

        if ( uBackRead + BUFREADCOMMENT > uMaxBack )
        {
            uBackRead = uMaxBack;
        }
        else
        {
            uBackRead += BUFREADCOMMENT;
        }
        uReadPos = uSizeFile - uBackRead;

        uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
                        ( BUFREADCOMMENT + 4 ) : ( uSizeFile - uReadPos );

        if ( fseek_file_func( fin, uReadPos, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
        {
            break;
        }

        if ( fread( buf, 1, uReadSize, fin ) != uReadSize )
        {
            break;
        }

        for ( i = (int)uReadSize - 3; ( i-- ) > 0; )
        {
            if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) &&
                 ( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) )
            {
                uPosFound = uReadPos + i;
                break;
            }
        }

        if ( uPosFound != 0 )
        {
            break;
        }
    }

    TRYFREE( buf );
    return uPosFound;
}


/* unzClose  0x0048c3c0 */
int unzClose( unzFile file )
{
    unz_s *s;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    if ( s->pfile_in_zip_read != NULL )
    {
        unzCloseCurrentFile( file );
    }

    fclose( s->file );
    TRYFREE( s );
    return UNZ_OK;
}


/* unzGetGlobalInfo  0x0048c410 */
int unzGetGlobalInfo( unzFile file, unz_global_info *pglobal_info )
{
    unz_s *s;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    *pglobal_info = s->gi;
    return UNZ_OK;
}


/* unzGetCurrentFileInfo  0x0048c440 */
int unzGetCurrentFileInfo( unzFile file,
                           unz_file_info *pfile_info,
                           char *szFileName,
                           unsigned long fileNameBufferSize,
                           void *extraField,
                           unsigned long extraFieldBufferSize,
                           char *szComment,
                           unsigned long commentBufferSize )
{
    return unzlocal_GetCurrentFileInfoInternal( file, pfile_info, NULL,
                                                szFileName, fileNameBufferSize,
                                                extraField, extraFieldBufferSize,
                                                szComment, commentBufferSize );
}


/* unzlocal_GetCurrentFileInfoInternal  0x0048c470 */
local int unzlocal_GetCurrentFileInfoInternal( unzFile file,
                                               unz_file_info *pfile_info,
                                               unz_file_info_internal *pfile_info_internal,
                                               char *szFileName,
                                               uLong fileNameBufferSize,
                                               void *extraField,
                                               uLong extraFieldBufferSize,
                                               char *szComment,
                                               uLong commentBufferSize )
{
    unz_s                  *s;
    unz_file_info           file_info;
    unz_file_info_internal  file_info_internal;
    int                     err = UNZ_OK;
    uLong                   uMagic;
    long                    lSeek = 0;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    if ( fseek_file_func( s->file,
                          s->pos_in_central_dir + s->byte_before_the_zipfile,
                          ZLIB_FILEFUNC_SEEK_SET ) != 0 )
    {
        err = UNZ_ERRNO;
    }

    if ( err == UNZ_OK )
    {
        if ( unzlocal_getLong( s->file, &uMagic ) != UNZ_OK )
        {
            err = UNZ_ERRNO;
        }
        else if ( uMagic != 0x02014b50 )
        {
            err = UNZ_BADZIPFILE;
        }
    }

    if ( unzlocal_getShort( s->file, &file_info.version ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.version_needed ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.flag ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.compression_method ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &file_info.dosDate ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    unzlocal_DosDateToTmuDate( file_info.dosDate, &file_info.tmu_date );

    if ( unzlocal_getLong( s->file, &file_info.crc ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &file_info.compressed_size ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &file_info.uncompressed_size ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.size_filename ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.size_file_extra ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.size_file_comment ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.disk_num_start ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &file_info.internal_fa ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &file_info.external_fa ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &file_info_internal.offset_curfile ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    lSeek += file_info.size_filename;
    if ( ( err == UNZ_OK ) && ( szFileName != NULL ) )
    {
        uLong uSizeRead;

        if ( file_info.size_filename < fileNameBufferSize )
        {
            *( szFileName + file_info.size_filename ) = '\0';
            uSizeRead = file_info.size_filename;
        }
        else
        {
            uSizeRead = fileNameBufferSize;
        }

        if ( ( file_info.size_filename > 0 ) && ( fileNameBufferSize > 0 ) )
        {
            if ( fread( szFileName, 1, uSizeRead, s->file ) != uSizeRead )
            {
                err = UNZ_ERRNO;
            }
        }
        lSeek -= uSizeRead;
    }

    if ( ( err == UNZ_OK ) && ( extraField != NULL ) )
    {
        uLong uSizeRead;

        if ( file_info.size_file_extra < extraFieldBufferSize )
        {
            uSizeRead = file_info.size_file_extra;
        }
        else
        {
            uSizeRead = extraFieldBufferSize;
        }

        if ( lSeek != 0 )
        {
            if ( fseek_file_func( s->file, lSeek, ZLIB_FILEFUNC_SEEK_CUR ) == 0 )
            {
                lSeek = 0;
            }
            else
            {
                err = UNZ_ERRNO;
            }
        }

        if ( ( file_info.size_file_extra > 0 ) && ( extraFieldBufferSize > 0 ) )
        {
            if ( fread( extraField, 1, uSizeRead, s->file ) != uSizeRead )
            {
                err = UNZ_ERRNO;
            }
        }
        lSeek += file_info.size_file_extra - uSizeRead;
    }
    else
    {
        lSeek += file_info.size_file_extra;
    }

    if ( ( err == UNZ_OK ) && ( szComment != NULL ) )
    {
        uLong uSizeRead;

        if ( file_info.size_file_comment < commentBufferSize )
        {
            *( szComment + file_info.size_file_comment ) = '\0';
            uSizeRead = file_info.size_file_comment;
        }
        else
        {
            uSizeRead = commentBufferSize;
        }

        if ( lSeek != 0 )
        {
            if ( fseek_file_func( s->file, lSeek, ZLIB_FILEFUNC_SEEK_CUR ) == 0 )
            {
                lSeek = 0;
            }
            else
            {
                err = UNZ_ERRNO;
            }
        }

        if ( ( file_info.size_file_comment > 0 ) && ( commentBufferSize > 0 ) )
        {
            if ( fread( szComment, 1, uSizeRead, s->file ) != uSizeRead )
            {
                err = UNZ_ERRNO;
            }
        }
        lSeek += file_info.size_file_comment - uSizeRead;
    }
    else
    {
        lSeek += file_info.size_file_comment;
    }

    if ( ( err == UNZ_OK ) && ( pfile_info != NULL ) )
    {
        *pfile_info = file_info;
    }

    if ( ( err == UNZ_OK ) && ( pfile_info_internal != NULL ) )
    {
        *pfile_info_internal = file_info_internal;
    }

    return err;
}


/* unzlocal_DosDateToTmuDate  0x0048c8b0 */
local void unzlocal_DosDateToTmuDate( uLong ulDosDate, tm_unz *ptm )
{
    uLong uDate;

    uDate = (uLong)( ulDosDate >> 16 );

    ptm->tm_mday = (uLong)( uDate & 0x1f );
    ptm->tm_mon  = (uLong)( ( ( ( uDate ) & 0x1e0 ) / 0x20 ) - 1 );
    ptm->tm_year = (uLong)( ( ( uDate ) & 0x0fe00 ) / 0x0200 + 1980 );

    ptm->tm_hour = (uLong)( ( ulDosDate & 0xf800 ) / 0x800 );
    ptm->tm_min  = (uLong)( ( ulDosDate & 0x7e0 ) / 0x20 );
    ptm->tm_sec  = (uLong)( 2 * ( ulDosDate & 0x1f ) );
}


/* unzGoToFirstFile  0x0048c930 */
int unzGoToFirstFile( unzFile file )
{
    int    err;
    unz_s *s;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    s->pos_in_central_dir = s->offset_central_dir;
    s->num_file = 0;
    err = unzlocal_GetCurrentFileInfoInternal( file, &s->cur_file_info,
                                               &s->cur_file_info_internal,
                                               NULL, 0, NULL, 0, NULL, 0 );
    s->current_file_ok = ( err == UNZ_OK );
    return err;
}


/* unzGoToNextFile  0x0048c9b0 */
int unzGoToNextFile( unzFile file )
{
    unz_s *s;
    int    err;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    if ( !s->current_file_ok )
    {
        return UNZ_END_OF_LIST_OF_FILE;
    }
    if ( s->num_file + 1 == s->gi.number_entry )
    {
        return UNZ_END_OF_LIST_OF_FILE;
    }

    s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
                             s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment;
    s->num_file++;
    err = unzlocal_GetCurrentFileInfoInternal( file, &s->cur_file_info,
                                               &s->cur_file_info_internal,
                                               NULL, 0, NULL, 0, NULL, 0 );
    s->current_file_ok = ( err == UNZ_OK );
    return err;
}


/* unzGetCurrentFileInfoPosition  0x0048ca70 */
int unzGetCurrentFileInfoPosition( unzFile file, unsigned long *pos )
{
    unz_s *s;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    *pos = s->pos_in_central_dir;
    return UNZ_OK;
}


/* unzSetCurrentFileInfoPosition  0x0048caa0 */
int unzSetCurrentFileInfoPosition( unzFile file, unsigned long pos )
{
    unz_s *s;
    int    err;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    s->pos_in_central_dir = pos;
    err = unzlocal_GetCurrentFileInfoInternal( file, &s->cur_file_info,
                                               &s->cur_file_info_internal,
                                               NULL, 0, NULL, 0, NULL, 0 );
    s->current_file_ok = ( err == UNZ_OK );
    return UNZ_OK;
}


/* unzLocateFile  0x0048cb00 */
int unzLocateFile( unzFile file, const char *szFileName, int iCaseSensitivity )
{
    unz_s *s;
    int    err;

    uLong  num_fileSaved;
    uLong  pos_in_central_dirSaved;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }

    if ( strlen( szFileName ) >= UNZ_MAXFILENAMEINZIP )
    {
        return UNZ_PARAMERROR;
    }

    s = (unz_s *)file;
    if ( !s->current_file_ok )
    {
        return UNZ_END_OF_LIST_OF_FILE;
    }

    num_fileSaved = s->num_file;
    pos_in_central_dirSaved = s->pos_in_central_dir;

    err = unzGoToFirstFile( file );

    while ( err == UNZ_OK )
    {
        char szCurrentFileName[UNZ_MAXFILENAMEINZIP + 1];

        unzGetCurrentFileInfo( file, NULL,
                               szCurrentFileName, sizeof( szCurrentFileName ) - 1,
                               NULL, 0, NULL, 0 );
        if ( unzStringFileNameCompare( szCurrentFileName, szFileName, iCaseSensitivity ) == 0 )
        {
            return UNZ_OK;
        }
        err = unzGoToNextFile( file );
    }

    s->num_file = num_fileSaved;
    s->pos_in_central_dir = pos_in_central_dirSaved;
    return err;
}


/* unzOpenCurrentFile  0x0048cc00 */
int unzOpenCurrentFile( unzFile file )
{
    int    err = UNZ_OK;
    int    Store;
    uInt   iSizeVar;
    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;
    uLong  offset_local_extrafield;
    uInt   size_local_extrafield;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    if ( !s->current_file_ok )
    {
        return UNZ_PARAMERROR;
    }

    if ( s->pfile_in_zip_read != NULL )
    {
        unzCloseCurrentFile( file );
    }

    if ( unzlocal_CheckCurrentFileCoherencyHeader( s, &iSizeVar,
                                                   &offset_local_extrafield,
                                                   &size_local_extrafield ) != UNZ_OK )
    {
        return UNZ_BADZIPFILE;
    }

    pfile_in_zip_read_info = (file_in_zip_read_info_s *)ALLOC( sizeof( file_in_zip_read_info_s ) );
    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_INTERNALERROR;
    }

    pfile_in_zip_read_info->read_buffer = (char *)ALLOC( UNZ_BUFSIZE );
    pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
    pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
    pfile_in_zip_read_info->pos_local_extrafield = 0;

    if ( pfile_in_zip_read_info->read_buffer == NULL )
    {
        TRYFREE( pfile_in_zip_read_info );
        return UNZ_INTERNALERROR;
    }

    pfile_in_zip_read_info->stream_initialised = 0;

    if ( ( s->cur_file_info.compression_method != 0 ) &&
         ( s->cur_file_info.compression_method != Z_DEFLATED ) )
    {
        err = UNZ_BADZIPFILE;
    }
    Store = s->cur_file_info.compression_method == 0;

    pfile_in_zip_read_info->compression_method = s->cur_file_info.compression_method;
    pfile_in_zip_read_info->file = s->file;
    pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;

    pfile_in_zip_read_info->stream.total_out = 0;

    if ( !Store )
    {
        pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
        pfile_in_zip_read_info->stream.zfree = (free_func)0;
        pfile_in_zip_read_info->stream.opaque = (voidpf)0;

        err = inflateInit2( &pfile_in_zip_read_info->stream, -MAX_WBITS );
        if ( err == Z_OK )
        {
            pfile_in_zip_read_info->stream_initialised = 1;
        }
    }

    pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.compressed_size;
    pfile_in_zip_read_info->rest_read_uncompressed = s->cur_file_info.uncompressed_size;

    pfile_in_zip_read_info->pos_in_zipfile = s->cur_file_info_internal.offset_curfile +
                                             SIZEZIPLOCALHEADER + iSizeVar;

    pfile_in_zip_read_info->stream.avail_in = (uInt)0;

    s->pfile_in_zip_read = pfile_in_zip_read_info;

    return UNZ_OK;
}


/* unzlocal_CheckCurrentFileCoherencyHeader  0x0048cdd0 */
local int unzlocal_CheckCurrentFileCoherencyHeader( unz_s *s,
                                                    uInt *piSizeVar,
                                                    uLong *poffset_local_extrafield,
                                                    uInt *psize_local_extrafield )
{
    uLong uMagic, uData, uFlags;
    uLong size_filename;
    uLong size_extra_field;
    int   err = UNZ_OK;

    *piSizeVar = 0;
    *poffset_local_extrafield = 0;
    *psize_local_extrafield = 0;

    if ( fseek_file_func( s->file,
                          s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile,
                          ZLIB_FILEFUNC_SEEK_SET ) != 0 )
    {
        return UNZ_ERRNO;
    }

    if ( err == UNZ_OK )
    {
        if ( unzlocal_getLong( s->file, &uMagic ) != UNZ_OK )
        {
            err = UNZ_ERRNO;
        }
        else if ( uMagic != 0x04034b50 )
        {
            err = UNZ_BADZIPFILE;
        }
    }

    if ( unzlocal_getShort( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &uFlags ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getShort( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.compression_method ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( ( err == UNZ_OK ) && ( s->cur_file_info.compression_method != 0 ) &&
                              ( s->cur_file_info.compression_method != Z_DEFLATED ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( unzlocal_getLong( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }

    if ( unzlocal_getLong( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.crc ) &&
                                   ( ( uFlags & 8 ) == 0 ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( unzlocal_getLong( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.compressed_size ) &&
                                   ( ( uFlags & 8 ) == 0 ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( unzlocal_getLong( s->file, &uData ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    else if ( ( err == UNZ_OK ) && ( uData != s->cur_file_info.uncompressed_size ) &&
                                   ( ( uFlags & 8 ) == 0 ) )
    {
        err = UNZ_BADZIPFILE;
    }

    if ( unzlocal_getShort( s->file, &size_filename ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    else if ( ( err == UNZ_OK ) && ( size_filename != s->cur_file_info.size_filename ) )
    {
        err = UNZ_BADZIPFILE;
    }

    *piSizeVar += (uInt)size_filename;

    if ( unzlocal_getShort( s->file, &size_extra_field ) != UNZ_OK )
    {
        err = UNZ_ERRNO;
    }
    *poffset_local_extrafield = s->cur_file_info_internal.offset_curfile +
                                SIZEZIPLOCALHEADER + size_filename;
    *psize_local_extrafield = (uInt)size_extra_field;

    *piSizeVar += (uInt)size_extra_field;

    return err;
}


/* unzReadCurrentFile  0x0048d050 */
int unzReadCurrentFile( unzFile file, void *buf, unsigned len )
{
    int    err = UNZ_OK;
    uInt   iRead = 0;
    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    pfile_in_zip_read_info = (file_in_zip_read_info_s *)s->pfile_in_zip_read;

    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_PARAMERROR;
    }

    if ( ( pfile_in_zip_read_info->read_buffer == NULL ) )
    {
        return UNZ_END_OF_LIST_OF_FILE;
    }
    if ( len == 0 )
    {
        return 0;
    }

    pfile_in_zip_read_info->stream.next_out = (Bytef *)buf;

    pfile_in_zip_read_info->stream.avail_out = (uInt)len;

    if ( len > pfile_in_zip_read_info->rest_read_uncompressed )
    {
        pfile_in_zip_read_info->stream.avail_out =
            (uInt)pfile_in_zip_read_info->rest_read_uncompressed;
    }

    while ( pfile_in_zip_read_info->stream.avail_out > 0 )
    {
        if ( ( pfile_in_zip_read_info->stream.avail_in == 0 ) &&
             ( pfile_in_zip_read_info->rest_read_compressed > 0 ) )
        {
            uInt uReadThis = UNZ_BUFSIZE;

            if ( pfile_in_zip_read_info->rest_read_compressed < uReadThis )
            {
                uReadThis = (uInt)pfile_in_zip_read_info->rest_read_compressed;
            }
            if ( uReadThis == 0 )
            {
                return UNZ_EOF;
            }
            if ( fseek_file_func( pfile_in_zip_read_info->file,
                                  pfile_in_zip_read_info->pos_in_zipfile +
                                      pfile_in_zip_read_info->byte_before_the_zipfile,
                                  ZLIB_FILEFUNC_SEEK_SET ) != 0 )
            {
                return UNZ_ERRNO;
            }
            if ( fread( pfile_in_zip_read_info->read_buffer, 1, uReadThis,
                        pfile_in_zip_read_info->file ) != uReadThis )
            {
                return UNZ_ERRNO;
            }
            pfile_in_zip_read_info->pos_in_zipfile += uReadThis;

            pfile_in_zip_read_info->rest_read_compressed -= uReadThis;

            pfile_in_zip_read_info->stream.next_in = (Bytef *)pfile_in_zip_read_info->read_buffer;
            pfile_in_zip_read_info->stream.avail_in = (uInt)uReadThis;
        }

        if ( pfile_in_zip_read_info->compression_method == 0 )
        {
            uInt uDoCopy, i;

            if ( pfile_in_zip_read_info->stream.avail_out <
                 pfile_in_zip_read_info->stream.avail_in )
            {
                uDoCopy = pfile_in_zip_read_info->stream.avail_out;
            }
            else
            {
                uDoCopy = pfile_in_zip_read_info->stream.avail_in;
            }

            for ( i = 0; i < uDoCopy; i++ )
            {
                *( pfile_in_zip_read_info->stream.next_out + i ) =
                    *( pfile_in_zip_read_info->stream.next_in + i );
            }

            pfile_in_zip_read_info->rest_read_uncompressed -= uDoCopy;
            pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
            pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
            pfile_in_zip_read_info->stream.next_out += uDoCopy;
            pfile_in_zip_read_info->stream.next_in += uDoCopy;
            pfile_in_zip_read_info->stream.total_out += uDoCopy;
            iRead += uDoCopy;
        }
        else
        {
            uLong uTotalOutBefore, uTotalOutAfter;

            uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
            err = inflate( &pfile_in_zip_read_info->stream, Z_SYNC_FLUSH );
            uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;

            pfile_in_zip_read_info->rest_read_uncompressed -=
                ( uTotalOutAfter - uTotalOutBefore );

            iRead += (uInt)( uTotalOutAfter - uTotalOutBefore );

            if ( err == Z_STREAM_END )
            {
                return ( iRead == 0 ) ? UNZ_EOF : iRead;
            }
            if ( err != Z_OK )
            {
                break;
            }
        }
    }

    if ( err == Z_OK )
    {
        return iRead;
    }
    return err;
}


/* unztell  0x0048d310 */
long unztell( unzFile file )
{
    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    pfile_in_zip_read_info = (file_in_zip_read_info_s *)s->pfile_in_zip_read;

    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_PARAMERROR;
    }

    return (long)pfile_in_zip_read_info->stream.total_out;
}


/* unzeof  0x0048d350 */
int unzeof( unzFile file )
{
    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    pfile_in_zip_read_info = (file_in_zip_read_info_s *)s->pfile_in_zip_read;

    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_PARAMERROR;
    }

    if ( pfile_in_zip_read_info->rest_read_uncompressed == 0 )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/* unzGetLocalExtrafield  0x0048d3a0 */
int unzGetLocalExtrafield( unzFile file, void *buf, unsigned len )
{
    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;
    uInt   read_now;
    uLong  size_to_read;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    pfile_in_zip_read_info = (file_in_zip_read_info_s *)s->pfile_in_zip_read;

    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_PARAMERROR;
    }

    size_to_read = ( pfile_in_zip_read_info->size_local_extrafield -
                     pfile_in_zip_read_info->pos_local_extrafield );

    if ( buf == NULL )
    {
        return (int)size_to_read;
    }

    if ( len > size_to_read )
    {
        read_now = (uInt)size_to_read;
    }
    else
    {
        read_now = len;
    }

    if ( read_now == 0 )
    {
        return 0;
    }

    if ( fseek_file_func( pfile_in_zip_read_info->file,
                          pfile_in_zip_read_info->offset_local_extrafield +
                              pfile_in_zip_read_info->pos_local_extrafield,
                          ZLIB_FILEFUNC_SEEK_SET ) != 0 )
    {
        return UNZ_ERRNO;
    }

    if ( fread( buf, 1, size_to_read, pfile_in_zip_read_info->file ) != size_to_read )
    {
        return UNZ_ERRNO;
    }

    return (int)read_now;
}


/* unzCloseCurrentFile  0x0048d460 */
int unzCloseCurrentFile( unzFile file )
{
    int    err = UNZ_OK;

    unz_s *s;
    file_in_zip_read_info_s *pfile_in_zip_read_info;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;
    pfile_in_zip_read_info = (file_in_zip_read_info_s *)s->pfile_in_zip_read;

    if ( pfile_in_zip_read_info == NULL )
    {
        return UNZ_PARAMERROR;
    }

    TRYFREE( pfile_in_zip_read_info->read_buffer );
    pfile_in_zip_read_info->read_buffer = NULL;
    if ( pfile_in_zip_read_info->stream_initialised )
    {
        inflateEnd( &pfile_in_zip_read_info->stream );
    }

    pfile_in_zip_read_info->stream_initialised = 0;
    TRYFREE( pfile_in_zip_read_info );

    s->pfile_in_zip_read = NULL;

    return err;
}


/* unzGetGlobalComment  0x0048d4f0 */
int unzGetGlobalComment( unzFile file, char *szComment, unsigned long uSizeBuf )
{
    unz_s *s;
    uLong  uReadThis;

    if ( file == NULL )
    {
        return UNZ_PARAMERROR;
    }
    s = (unz_s *)file;

    uReadThis = uSizeBuf;
    if ( uReadThis > s->gi.size_comment )
    {
        uReadThis = s->gi.size_comment;
    }

    if ( fseek_file_func( s->file, s->central_pos + 22, ZLIB_FILEFUNC_SEEK_SET ) != 0 )
    {
        return UNZ_ERRNO;
    }

    if ( uReadThis > 0 )
    {
        *szComment = '\0';
        if ( fread( szComment, 1, uReadThis, s->file ) != uReadThis )
        {
            return UNZ_ERRNO;
        }
    }

    if ( ( szComment != NULL ) && ( uSizeBuf > s->gi.size_comment ) )
    {
        *( szComment + s->gi.size_comment ) = '\0';
    }
    return (int)uReadThis;
}


/* fseek_file_func  0x0048d5a0 */
local long fseek_file_func( FILE *file, long offset, int origin )
{
    int fseek_origin;

    switch ( origin )
    {
    case ZLIB_FILEFUNC_SEEK_CUR:
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END:
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET:
        fseek_origin = SEEK_SET;
        break;
    default:
        return 0;
    }

    return fseek( file, offset, fseek_origin );
}


/* unzlocal_GetFileSize  0x0048d600 */
local long unzlocal_GetFileSize( FILE *file )
{
    long pos;
    long size;

    pos = ftell( file );
    fseek( file, 0, SEEK_END );
    size = ftell( file );
    fseek( file, pos, SEEK_SET );

    return size;
}

/* Original: ..\src\zlib\unzip.c */

/* unzip.h -- IO for uncompress .zip files using zlib
   Version 0.15 beta, Mar 19th, 1998,

   Copyright (C) 1998 Gilles Vollant

   This unzip package allow extract file from .ZIP file, compatible with PKZip 2.04g
     WinZip, InfoZip tools and compatible.
   Encryption and multi volume ZipFile (span) are not supported.
   Old compressions used by old PKZip 1.x are not supported

   THIS IS AN ALPHA VERSION. AT THIS STAGE OF DEVELOPPEMENT, SOMES API OR STRUCTURE
   CAN CHANGE IN FUTURE VERSION !!
   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/

/* This is an altered version of minizip 0.15, not the original software. */

#ifndef UNZIP_H
#define UNZIP_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *unzFile;

#define UNZ_OK                      (0)
#define UNZ_END_OF_LIST_OF_FILE     (-100)
#define UNZ_ERRNO                   (-1)
#define UNZ_EOF                     (0)
#define UNZ_PARAMERROR              (-102)
#define UNZ_BADZIPFILE              (-103)
#define UNZ_INTERNALERROR           (-104)
#define UNZ_CRCERROR                (-105)

typedef struct tm_unz_s
{
    unsigned long tm_sec;
    unsigned long tm_min;
    unsigned long tm_hour;
    unsigned long tm_mday;
    unsigned long tm_mon;
    unsigned long tm_year;
} tm_unz;

typedef struct unz_global_info_s
{
    unsigned long number_entry;         /* +0x00 */
    unsigned long size_comment;         /* +0x04 */
} unz_global_info;

typedef struct unz_file_info_s
{
    unsigned long version;              /* +0x00 */
    unsigned long version_needed;       /* +0x04 */
    unsigned long flag;                 /* +0x08 */
    unsigned long compression_method;   /* +0x0c */
    unsigned long dosDate;              /* +0x10 */
    unsigned long crc;                  /* +0x14 */
    unsigned long compressed_size;      /* +0x18 */
    unsigned long uncompressed_size;    /* +0x1c */
    unsigned long size_filename;        /* +0x20 */
    unsigned long size_file_extra;      /* +0x24 */
    unsigned long size_file_comment;    /* +0x28 */
    unsigned long disk_num_start;       /* +0x2c */
    unsigned long internal_fa;          /* +0x30 */
    unsigned long external_fa;          /* +0x34 */
    tm_unz        tmu_date;             /* +0x38 */
} unz_file_info;                        /* sizeof == 0x50 */

typedef struct unz_file_info_internal_s
{
    unsigned long offset_curfile;       /* +0x00 */
} unz_file_info_internal;

typedef struct
{
    FILE                   *file;                       /* +0x00 */
    unz_global_info         gi;                         /* +0x04 */
    unsigned long           byte_before_the_zipfile;    /* +0x0c */
    unsigned long           num_file;                   /* +0x10 */
    unsigned long           pos_in_central_dir;         /* +0x14 */
    unsigned long           current_file_ok;            /* +0x18 */
    unsigned long           central_pos;                /* +0x1c */
    unsigned long           size_central_dir;           /* +0x20 */
    unsigned long           offset_central_dir;         /* +0x24 */
    unz_file_info           cur_file_info;              /* +0x28 */
    unz_file_info_internal  cur_file_info_internal;     /* +0x78 */
    void                   *pfile_in_zip_read;          /* +0x7c */
} unz_s;                                                /* sizeof == 0x80 */

unzFile unzOpen( const char *path );                                        /* 0x0048bfc0 */
unzFile unzReOpen( const char *path, unzFile file );                        /* 0x0048bf50 */
int     unzClose( unzFile file );                                           /* 0x0048c3c0 */
int     unzGetGlobalInfo( unzFile file, unz_global_info *pglobal_info );    /* 0x0048c410 */
int     unzGoToFirstFile( unzFile file );                                   /* 0x0048c930 */
int     unzGoToNextFile( unzFile file );                                    /* 0x0048c9b0 */
int     unzGetCurrentFileInfo( unzFile file, unz_file_info *pfile_info,
                               char *szFileName, unsigned long fileNameBufferSize,
                               void *extraField, unsigned long extraFieldBufferSize,
                               char *szComment, unsigned long commentBufferSize ); /* 0x0048c440 */
int     unzGetCurrentFileInfoPosition( unzFile file, unsigned long *pos );  /* 0x0048ca70 */
int     unzSetCurrentFileInfoPosition( unzFile file, unsigned long pos );   /* 0x0048caa0 */
int     unzOpenCurrentFile( unzFile file );                                 /* 0x0048cc00 */
int     unzCloseCurrentFile( unzFile file );                                /* 0x0048d460 */
int     unzReadCurrentFile( unzFile file, void *buf, unsigned len );        /* 0x0048d050 */
long    unztell( unzFile file );                                            /* 0x0048d310 */

int     unzStringFileNameCompare( const char *fileName1, const char *fileName2,
                                  int iCaseSensitivity );                   /* 0x0048be60 */
int     unzLocateFile( unzFile file, const char *szFileName,
                       int iCaseSensitivity );                              /* 0x0048cb00 */
int     unzeof( unzFile file );                                             /* 0x0048d350 */
int     unzGetLocalExtrafield( unzFile file, void *buf, unsigned len );     /* 0x0048d3a0 */
int     unzGetGlobalComment( unzFile file, char *szComment,
                             unsigned long uSizeBuf );                      /* 0x0048d4f0 */

/* unz_copyright  0x005100d8 */
extern const char unz_copyright[];

#ifdef __cplusplus
}
#endif

#endif

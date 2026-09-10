/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_imagedecode.cpp */

#ifndef R_IMAGEDECODE_H
#define R_IMAGEDECODE_H

#include "q_shared.h"


#define IMG_FORMAT_BITMAP_RGBA          1
#define IMG_FORMAT_BITMAP_RGB           2
#define IMG_FORMAT_BITMAP_LUMINANCE_ALPHA 3
#define IMG_FORMAT_BITMAP_LUMINANCE     4
#define IMG_FORMAT_BITMAP_ALPHA         5
#define IMG_FORMAT_WAVELET_RGBA         6
#define IMG_FORMAT_WAVELET_RGB          7
#define IMG_FORMAT_WAVELET_LUMINANCE_ALPHA 8
#define IMG_FORMAT_WAVELET_LUMINANCE    9
#define IMG_FORMAT_WAVELET_ALPHA        10
#define IMG_FORMAT_DXT1                 11
#define IMG_FORMAT_DXT3                 12
#define IMG_FORMAT_DXT5                 13

#define IMG_VERSION                     6

#define IMG_FLAG_NOMIPMAPS              0x02
#define IMG_FLAG_CUBEMAP                0x04

#define MAX_IMAGE_NAME                  0x44


typedef struct
{
    char           magic[3];        /* +0x00 */
    byte           version;         /* +0x03 */
    byte           format;          /* +0x04 */
    byte           flags;           /* +0x05 */
    short          dimensions[3];   /* +0x06 */
    int            fileSize[4];     /* +0x0c */
} ImageFile_t;                      /* sizeof == 0x1c */

typedef struct
{
    char   name[MAX_IMAGE_NAME];    /* +0x00 */
    byte   hasAlpha;                /* +0x44 */
    byte   pad45[3];                /* +0x45 */
    int    width;                   /* +0x48 */
    int    height;                  /* +0x4c */
    byte  *pixels;                  /* +0x50 */
} Image_t;


void Image_Register( const char *imageName, Image_t *image );   /* 0x004543d0 */

#endif

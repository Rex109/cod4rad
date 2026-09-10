/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_image_wavelet.cpp */

#ifndef R_IMAGE_WAVELET_H
#define R_IMAGE_WAVELET_H

#include "q_shared.h"


#define WAVELET_TABLE_SIZE  4096

#define WAVELET_ESCAPE      ( -32768 )

#define WAVELET_MAX_CHANNELS    4
#define WAVELET_MAX_FACES       6


typedef struct
{
    short          value;   /* +0x00 */
    unsigned short bits;    /* +0x02 */
} ImageWaveletEntry_t;      /* sizeof == 0x04 */


typedef struct
{
    unsigned short bits;        /* +0x00 */
    unsigned short bit;         /* +0x02 */
    const byte    *data;        /* +0x04 */
    int            width;       /* +0x08 */
    int            height;      /* +0x0c */
    int            channels;    /* +0x10 */
    int            bpp;         /* +0x14 */
    int            mipLevel;    /* +0x18 */
    byte           primed;      /* +0x1c */
} ImageWaveletDecode_t;         /* sizeof == 0x20 */


extern const ImageWaveletEntry_t waveletHigh[WAVELET_TABLE_SIZE];   /* 0x004911a8 */
extern const ImageWaveletEntry_t waveletMid[WAVELET_TABLE_SIZE];    /* 0x004951a8 */
extern const ImageWaveletEntry_t waveletLow[WAVELET_TABLE_SIZE];    /* 0x004991a8 */


void Image_DecodeWaveletMip( const byte *src, byte *dst,
                             ImageWaveletDecode_t *decode );        /* 0x00452a90 */

#endif

/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_image_wavelet.cpp */

#include "r_image_wavelet.h"
#include "assertive.h"


/* Image_ReadBits  0x00452900 */
static void Image_ReadBits( ImageWaveletDecode_t *decode, unsigned short bitCount )
{
    unsigned bits;
    unsigned bit;

    Assertx( bitCount > 0 && bitCount <= 16, "%s", "bitCount > 0 && bitCount <= 16" );
    Assertx( decode->bit < 8, "%s", "decode->bit < 8" );

    decode->bits = ( unsigned short )( decode->bits >> bitCount );

    bits = ( ( unsigned )decode->data[3] << 24 )
         | ( ( unsigned )decode->data[2] << 16 )
         | ( ( unsigned )decode->data[1] << 8 )
         |   ( unsigned )decode->data[0];

    bits >>= decode->bit;
    bits <<= 16 - bitCount;
    bits |= decode->bits;

    decode->bits = ( unsigned short )bits;

    bit = decode->bit + bitCount;

    decode->data += ( unsigned short )bit >> 3;
    decode->bit   = ( unsigned short )( bit & 7 );
}

/* Image_DecodeSymbol  0x004529a0 */
static int Image_DecodeSymbol( ImageWaveletDecode_t *decode,
                               const ImageWaveletEntry_t *table,
                               unsigned short bitCount, int bias )
{
    int mask = ( 1 << bitCount ) - 1;
    int index;
    int value;

    Assertx( mask >= bias * 2 - 1, "%s", "(1 << bitCount) - 1 >= bias * 2 - 1" );
    Assertx( ( 1 << ( bitCount - 1 ) ) - 1 < bias * 2 - 1,
             "%s", "(1 << (bitCount - 1)) - 1 < bias * 2 - 1" );

    index = decode->bits & ( WAVELET_TABLE_SIZE - 1 );

    Image_ReadBits( decode, table[index].bits );

    value = table[index].value;

    if ( value == WAVELET_ESCAPE )
    {
        value = ( decode->bits & mask ) - bias;

        Image_ReadBits( decode, bitCount );
    }

    return value;
}

/* Image_DecodeWaveletRow  0x00452a50 */
static void Image_DecodeWaveletRow( ImageWaveletDecode_t *decode, byte *dst, int count,
                                    const int *offsets )
{
    do
    {
        int channel;

        for ( channel = 0; channel < decode->channels; channel++ )
            dst[offsets[channel]] += ( byte )Image_DecodeSymbol( decode, waveletLow, 9, 0xff );

        dst += decode->bpp;
    }
    while ( --count );
}

static int Image_ReadSign( ImageWaveletDecode_t *decode )
{
    int sign = decode->bits & 1;

    Image_ReadBits( decode, 1 );

    return sign;
}

static void Image_ExpandBlock( byte *dst, int base, int a, int b, int c, int sign,
                               int bpp, int rowBytes )
{
    base += base;

    dst[0]                = ( byte )( ( ( base + a + b + c ) >> 1 ) + sign );
    dst[bpp]              = ( byte )( ( base + a - b - c ) >> 1 );
    dst[rowBytes]         = ( byte )( ( base + b - a - c ) >> 1 );
    dst[rowBytes + bpp]   = ( byte )( ( base + c - a - b ) >> 1 );
}

/* Image_DecodeWaveletMip  0x00452a90 */
void Image_DecodeWaveletMip( const byte *src, byte *dst, ImageWaveletDecode_t *decode )
{
    int offsets[WAVELET_MAX_CHANNELS];
    int width;
    int height;
    int rowBytes;
    int rowPairs;
    int bpp = decode->bpp;

    Assertx( decode->bpp >= 1 && decode->bpp <= 4,
             "decode->bpp not in [1, 4]\n\t%i not in [%i, %i]", decode->bpp, 1, 4 );
    Assertx( decode->bpp == decode->channels
             || ( decode->bpp == 4 && decode->channels == 3 ),
             "%s", "decode->bpp == decode->channels"
                   " || (decode->bpp == 4 && decode->channels == 3)" );
    Assertx( decode->mipLevel >= 0, "%s", "decode->mipLevel >= 0" );

    switch ( decode->bpp )
    {
    case 4:
    case 3:
        offsets[2] = 2;
        offsets[3] = 3;
    case 2:
        offsets[1] = 1;
    case 1:
        offsets[0] = 0;
        break;
    }

    width  = decode->width  >> decode->mipLevel;
    height = decode->height >> decode->mipLevel;

    if ( width <= 1 || height <= 1 )
    {
        int count;

        if ( width < 1 )
            width = 1;

        if ( height < 1 )
            height = 1;

        count = height * width;

        Assertx( count >= 1, "%s", "size >= 1" );

        do
        {
            int channel;

            for ( channel = 0; channel < decode->channels; channel++ )
            {
                dst[offsets[channel]] = *decode->data;
                decode->data++;
            }

            if ( decode->bpp != decode->channels )
                dst[offsets[3]] = 0xff;

            dst += decode->bpp;
        }
        while ( --count );

        return;
    }

    if ( !decode->primed )
    {
        decode->bits   = ( unsigned short )( decode->data[0] | ( decode->data[1] << 8 ) );
        decode->bit    = 0;
        decode->data  += 2;
        decode->primed = 1;
    }

    if ( Image_ReadSign( decode ) )
        Image_DecodeWaveletRow( decode, ( byte * )src, height * width / 4, offsets );

    rowBytes = width * bpp;
    rowPairs = ( height - 1 ) / 2 + 1;

    if ( height <= 0 )
        return;

    do
    {
        int colPairs;

        if ( width <= 0 )
        {
            dst += rowBytes;
            continue;
        }

        colPairs = ( width - 1 ) / 2 + 1;

        do
        {
            int a0 = 0;
            int b0 = 0;
            int c0 = 0;

            Assertx( dst + rowBytes + bpp <= src || dst > src,
                     "%s", "dst + stride + dstBpp <= src || dst > src" );

            if ( decode->channels != 1 )
            {
                int sign = Image_ReadSign( decode );

                a0 = Image_DecodeSymbol( decode, waveletHigh, 9, 0xff );
                b0 = Image_DecodeSymbol( decode, waveletHigh, 9, 0xff );
                c0 = Image_DecodeSymbol( decode, waveletHigh, 9, 0xff );

                Image_ExpandBlock( dst + offsets[0], src[offsets[0]], a0, b0, c0,
                                   sign, bpp, rowBytes );

                if ( decode->channels >= 3 )
                {
                    int channel;

                    for ( channel = 1; channel <= 2; channel++ )
                    {
                        int a;
                        int b;
                        int c;

                        sign = Image_ReadSign( decode );

                        a = Image_DecodeSymbol( decode, waveletMid, 10, 0x1fe ) + a0;
                        b = Image_DecodeSymbol( decode, waveletMid, 10, 0x1fe ) + b0;
                        c = Image_DecodeSymbol( decode, waveletMid, 10, 0x1fe ) + c0;

                        Image_ExpandBlock( dst + offsets[channel], src[offsets[channel]],
                                           a, b, c, sign, bpp, rowBytes );
                    }
                }
            }

            if ( decode->channels != 3 )
            {
                int last = decode->channels - 1;
                int sign = Image_ReadSign( decode );
                int a;
                int b;
                int c;

                a = Image_DecodeSymbol( decode, waveletLow, 9, 0xff );
                b = Image_DecodeSymbol( decode, waveletLow, 9, 0xff );
                c = Image_DecodeSymbol( decode, waveletLow, 9, 0xff );

                Image_ExpandBlock( dst + offsets[last], src[offsets[last]], a, b, c,
                                   sign, bpp, rowBytes );
            }
            else if ( decode->bpp != 3 )
            {
                byte *alpha = dst + offsets[3];

                alpha[0]              = 0xff;
                alpha[bpp]            = 0xff;
                alpha[rowBytes]       = 0xff;
                alpha[rowBytes + bpp] = 0xff;
            }

            src += bpp;
            dst += bpp * 2;
        }
        while ( --colPairs );

        dst += rowBytes;
    }
    while ( --rowPairs );
}

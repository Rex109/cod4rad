/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_imagedecode.cpp */

#include "cod4rad.h"
#include "r_imagedecode.h"
#include "r_image_wavelet.h"
#include "progress.h"

#include <string.h>


#define RGB5_SCALE      0.03125
#define RGB6_SCALE      0.015625
#define COLOR_SCALE     255.0
#define COLOR_HALF      127.5
#define COLOR_THIRD     85.0
#define COLOR_TWOTHIRD  170.0
#define ALPHA_SEVENTH   0.1428571492433548
#define ALPHA_FIFTH     0.20000000298023224


/* Image_ValidateHeader  0x0040dea0 */
static qboolean Image_ValidateHeader( const ImageFile_t *imageFile, const char *name )
{
    if ( imageFile->magic[0] != 'I' || imageFile->magic[1] != 'W' || imageFile->magic[2] != 'i' )
    {
        Com_PrintWarning( 8, "ERROR: image '%s' is not a PC IW image\n", name );
        return qfalse;
    }

    if ( imageFile->version != IMG_VERSION )
    {
        Com_PrintWarning( 8, "ERROR: image '%s' is version %i but should be version %i\n",
                          name, imageFile->version, IMG_VERSION );
        return qfalse;
    }

    return qtrue;
}

/* Image_GetMipCount  0x00453300 */
static int Image_GetMipCount( const ImageFile_t *imageFile )
{
    int width;
    int height;
    int count;

    if ( imageFile->flags & IMG_FLAG_NOMIPMAPS )
        return 1;

    Assert( imageFile->dimensions[2] == 1 );

    width  = imageFile->dimensions[0];
    height = imageFile->dimensions[1];
    count  = 1;

    while ( width > 1 || height > 1 )
    {
        width >>= 1;
        if ( width < 1 )
            width = 1;

        height >>= 1;
        if ( height < 1 )
            height = 1;

        count++;
    }

    return count;
}

/* Image_DecodeColorBlock  0x004535f0 */
static void Image_DecodeColorBlock( Image_t *image, const byte *src,
                                    int x, int y, qboolean fourColor )
{
    unsigned int color0;
    unsigned int color1;
    float        r0, g0, b0;
    float        r1, g1, b1;
    byte         palette[4][4];
    int          row;
    int          col;

    color0 = src[0] | ( src[1] << 8 );
    color1 = src[2] | ( src[3] << 8 );

    r0 = ( float )( ( int )( ( color0 >> 11 ) & 0x1f ) * RGB5_SCALE );
    g0 = ( float )( ( int )( ( color0 >>  5 ) & 0x3f ) * RGB6_SCALE );
    b0 = ( float )( ( int )( ( color0       ) & 0x1f ) * RGB5_SCALE );

    r1 = ( float )( ( int )( ( color1 >> 11 ) & 0x1f ) * RGB5_SCALE );
    g1 = ( float )( ( int )( ( color1 >>  5 ) & 0x3f ) * RGB6_SCALE );
    b1 = ( float )( ( int )( ( color1       ) & 0x1f ) * RGB5_SCALE );

    if ( !fourColor && color0 <= color1 )
    {
        palette[0][0] = ( byte )RoundFloatToInt( ( float )( r0 * COLOR_SCALE ) );
        palette[0][1] = ( byte )RoundFloatToInt( ( float )( g0 * COLOR_SCALE ) );
        palette[0][2] = ( byte )RoundFloatToInt( ( float )( b0 * COLOR_SCALE ) );
        palette[0][3] = 255;

        palette[1][0] = ( byte )RoundFloatToInt( ( float )( r1 * COLOR_SCALE ) );
        palette[1][1] = ( byte )RoundFloatToInt( ( float )( g1 * COLOR_SCALE ) );
        palette[1][2] = ( byte )RoundFloatToInt( ( float )( b1 * COLOR_SCALE ) );
        palette[1][3] = 255;

        palette[2][0] = ( byte )RoundFloatToInt( ( float )( r1 * COLOR_HALF + COLOR_HALF * r0 ) );
        palette[2][1] = ( byte )RoundFloatToInt( ( float )( g1 * COLOR_HALF + COLOR_HALF * g0 ) );
        palette[2][2] = ( byte )RoundFloatToInt( ( float )( b1 * COLOR_HALF + COLOR_HALF * b0 ) );
        palette[2][3] = 255;

        palette[3][0] = 0;
        palette[3][1] = 0;
        palette[3][2] = 0;
        palette[3][3] = 0;
    }
    else
    {
        palette[0][0] = ( byte )RoundFloatToInt( ( float )( r0 * COLOR_SCALE ) );
        palette[0][1] = ( byte )RoundFloatToInt( ( float )( g0 * COLOR_SCALE ) );
        palette[0][2] = ( byte )RoundFloatToInt( ( float )( b0 * COLOR_SCALE ) );
        palette[0][3] = 255;

        palette[1][0] = ( byte )RoundFloatToInt( ( float )( r1 * COLOR_SCALE ) );
        palette[1][1] = ( byte )RoundFloatToInt( ( float )( g1 * COLOR_SCALE ) );
        palette[1][2] = ( byte )RoundFloatToInt( ( float )( b1 * COLOR_SCALE ) );
        palette[1][3] = 255;

        palette[2][0] = ( byte )RoundFloatToInt( ( float )( r1 * COLOR_THIRD + r0 * COLOR_TWOTHIRD ) );
        palette[2][1] = ( byte )RoundFloatToInt( ( float )( g1 * COLOR_THIRD + g0 * COLOR_TWOTHIRD ) );
        palette[2][2] = ( byte )RoundFloatToInt( ( float )( b1 * COLOR_THIRD + b0 * COLOR_TWOTHIRD ) );
        palette[2][3] = 255;

        palette[3][0] = ( byte )RoundFloatToInt( ( float )( r0 * COLOR_THIRD + r1 * COLOR_TWOTHIRD ) );
        palette[3][1] = ( byte )RoundFloatToInt( ( float )( g0 * COLOR_THIRD + g1 * COLOR_TWOTHIRD ) );
        palette[3][2] = ( byte )RoundFloatToInt( ( float )( b0 * COLOR_THIRD + b1 * COLOR_TWOTHIRD ) );
        palette[3][3] = 255;
    }

    for ( row = 0; row < 4; row++ )
    {
        for ( col = 0; col < 4; col++ )
        {
            byte *dst = image->pixels + ( ( y + row ) * image->width + x + col ) * 4;

            memcpy( dst, palette[( src[4 + row] >> ( col * 2 ) ) & 3], 4 );
        }
    }
}

/* Image_DecodeDxt1Block  0x00453c20 */
static void Image_DecodeDxt1Block( const byte *src, Image_t *image, int x, int y )
{
    Image_DecodeColorBlock( image, src, x, y, qfalse );
}

/* Image_DecodeDxt3Block  0x00453c40 */
static void Image_DecodeDxt3Block( const byte *src, Image_t *image, int x, int y )
{
    int row;
    int col;

    Image_DecodeColorBlock( image, src + 8, x, y, qtrue );

    for ( row = 0; row < 4; row++ )
    {
        for ( col = 0; col < 4; col++ )
        {
            byte *dst = image->pixels + ( ( y + row ) * image->width + x + col ) * 4;
            int   nibble;

            nibble = ( src[row * 2 + ( col >> 1 )] >> ( ( col & 1 ) * 4 ) ) & 0xf;

            dst[3] = ( byte )( nibble * 17 );
        }
    }
}

/* Image_DecodeDxt5Block  0x00453e40 */
static void Image_DecodeDxt5Block( const byte *src, Image_t *image, int x, int y )
{
    byte         alpha[8];
    unsigned int bits;
    int          half;
    int          index;
    int          row;
    int          col;

    Image_DecodeColorBlock( image, src + 8, x, y, qtrue );

    alpha[0] = src[0];
    alpha[1] = src[1];

    if ( src[0] > src[1] )
    {
        alpha[2] = ( byte )RoundFloatToInt( ( float )( ( 6 * src[0] + 1 * src[1] ) * ALPHA_SEVENTH ) );
        alpha[3] = ( byte )RoundFloatToInt( ( float )( ( 5 * src[0] + 2 * src[1] ) * ALPHA_SEVENTH ) );
        alpha[4] = ( byte )RoundFloatToInt( ( float )( ( 4 * src[0] + 3 * src[1] ) * ALPHA_SEVENTH ) );
        alpha[5] = ( byte )RoundFloatToInt( ( float )( ( 3 * src[0] + 4 * src[1] ) * ALPHA_SEVENTH ) );
        alpha[6] = ( byte )RoundFloatToInt( ( float )( ( 2 * src[0] + 5 * src[1] ) * ALPHA_SEVENTH ) );
        alpha[7] = ( byte )RoundFloatToInt( ( float )( ( 1 * src[0] + 6 * src[1] ) * ALPHA_SEVENTH ) );
    }
    else
    {
        alpha[2] = ( byte )RoundFloatToInt( ( float )( ( 4 * src[0] + 1 * src[1] ) * ALPHA_FIFTH ) );
        alpha[3] = ( byte )RoundFloatToInt( ( float )( ( 3 * src[0] + 2 * src[1] ) * ALPHA_FIFTH ) );
        alpha[4] = ( byte )RoundFloatToInt( ( float )( ( 2 * src[0] + 3 * src[1] ) * ALPHA_FIFTH ) );
        alpha[5] = ( byte )RoundFloatToInt( ( float )( ( 1 * src[0] + 4 * src[1] ) * ALPHA_FIFTH ) );
        alpha[6] = 0;
        alpha[7] = 255;
    }

    for ( half = 0; half < 2; half++ )
    {
        bits = src[2 + half * 3] | ( src[3 + half * 3] << 8 ) | ( src[4 + half * 3] << 16 );

        for ( index = 0; index < 8; index++ )
        {
            row = ( half * 8 + index ) >> 2;
            col = ( half * 8 + index ) & 3;

            image->pixels[( ( y + row ) * image->width + x + col ) * 4 + 3] =
                alpha[( bits >> ( index * 3 ) ) & 7];
        }
    }
}

/* Image_DecodeDxtBlocks  0x004541e0 */
static void Image_DecodeDxtBlocks( const ImageFile_t *imageFile, const byte *src, Image_t *image )
{
    void ( *decodeBlock )( const byte *, Image_t *, int, int );
    int bytesPerBlock;
    int x;
    int y;

    switch ( imageFile->format )
    {
    case IMG_FORMAT_DXT1:
        bytesPerBlock = 8;
        decodeBlock   = Image_DecodeDxt1Block;
        break;

    case IMG_FORMAT_DXT3:
        bytesPerBlock = 16;
        decodeBlock   = Image_DecodeDxt3Block;
        break;

    case IMG_FORMAT_DXT5:
        bytesPerBlock = 16;
        decodeBlock   = Image_DecodeDxt5Block;
        break;

    default:
        Com_Error( 1, "unhandled case" );
        return;
    }

    for ( y = 0; y < image->height; y += 4 )
    {
        for ( x = 0; x < image->width; x += 4 )
        {
            decodeBlock( src, image, x, y );
            src += bytesPerBlock;
        }
    }
}

/* Image_LoadDxt  0x004542a0 */
static void Image_LoadDxt( Image_t *image, const ImageFile_t *imageFile,
                           const byte *data, int bytesPerBlock )
{
    int faceCount;
    int mip;

    Assert( image );
    Assert( imageFile );
    Assert( bytesPerBlock == ( imageFile->format == IMG_FORMAT_DXT1 ? 8 : 16 ) );

    faceCount = ( imageFile->flags & IMG_FLAG_CUBEMAP ) ? 6 : 1;

    for ( mip = Image_GetMipCount( imageFile ) - 1; mip >= 0; mip-- )
    {
        int width  = imageFile->dimensions[0] >> mip;
        int height = imageFile->dimensions[1] >> mip;
        int bytes;
        int face;

        if ( width <= 1 )
            width = 1;

        if ( height <= 1 )
            height = 1;

        bytes = ( ( height + 3 ) >> 2 ) * ( ( width + 3 ) >> 2 ) * bytesPerBlock;

        for ( face = 0; face < faceCount; face++ )
        {
            if ( !face && !mip )
                Image_DecodeDxtBlocks( imageFile, data, image );

            data += bytes;
        }
    }
}

/* Image_DecodeBitmap  0x00453190 */
static void Image_DecodeBitmap( const byte *src, const ImageFile_t *imageFile,
                                Image_t *image )
{
    byte *dst   = image->pixels;
    int   count = imageFile->dimensions[0] * imageFile->dimensions[1];

    switch ( imageFile->format )
    {
    case IMG_FORMAT_BITMAP_RGBA:
    case IMG_FORMAT_WAVELET_RGBA:
        for ( ; count; count-- )
        {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];

            src += 4;
            dst += 4;
        }
        break;

    case IMG_FORMAT_BITMAP_RGB:
    case IMG_FORMAT_WAVELET_RGB:
        for ( ; count; count-- )
        {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = 255;

            src += 3;
            dst += 4;
        }
        break;

    case IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
    case IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
        for ( ; count; count-- )
        {
            dst[0] = src[0];
            dst[1] = src[0];
            dst[2] = src[0];
            dst[3] = src[1];

            src += 2;
            dst += 4;
        }
        break;

    case IMG_FORMAT_BITMAP_LUMINANCE:
    case IMG_FORMAT_WAVELET_LUMINANCE:
        for ( ; count; count-- )
        {
            dst[0] = src[0];
            dst[1] = src[0];
            dst[2] = src[0];
            dst[3] = 255;

            src += 1;
            dst += 4;
        }
        break;

    case IMG_FORMAT_BITMAP_ALPHA:
    case IMG_FORMAT_WAVELET_ALPHA:
        for ( ; count; count-- )
        {
            dst[0] = 0;
            dst[1] = 0;
            dst[2] = 0;
            dst[3] = src[0];

            src += 1;
            dst += 4;
        }
        break;

    default:
        Assert( 0 );
        break;
    }
}

/* Image_LoadBitmap  0x00453500 */
static void Image_LoadBitmap( Image_t *image, const ImageFile_t *imageFile,
                              const byte *data, int bytesPerPixel )
{
    int faceCount;
    int mip;

    Assert( image );
    Assert( imageFile );

    faceCount = ( imageFile->flags & IMG_FLAG_CUBEMAP ) ? 6 : 1;

    for ( mip = Image_GetMipCount( imageFile ) - 1; mip >= 0; mip-- )
    {
        int width  = imageFile->dimensions[0] >> mip;
        int height = imageFile->dimensions[1] >> mip;
        int face;

        if ( width <= 1 )
            width = 1;

        if ( height <= 1 )
            height = 1;

        for ( face = 0; face < faceCount; face++ )
        {
            if ( !face && !mip )
                Image_DecodeBitmap( data, imageFile, image );

            data += width * height * bytesPerPixel;
        }
    }
}

/* Image_LoadWavelet  0x00453370 */
static void Image_LoadWavelet( Image_t *image, const ImageFile_t *imageFile,
                               const byte *data, int bytesPerPixel )
{
    ImageWaveletDecode_t decode;
    byte *faceData[WAVELET_MAX_FACES];
    byte *facePrev[WAVELET_MAX_FACES];
    int   faceCount;
    int   faceBytes;
    int   mip;
    int   face;

    Assert( image );
    Assert( imageFile );

    decode.bits     = 0;
    decode.bit      = 0;
    decode.data     = data;
    decode.width    = imageFile->dimensions[0];
    decode.height   = imageFile->dimensions[1];
    decode.channels = bytesPerPixel;
    decode.bpp      = bytesPerPixel;
    decode.primed   = 0;

    mip = Image_GetMipCount( imageFile ) - 1;

    faceCount = ( imageFile->flags & IMG_FLAG_CUBEMAP ) ? 6 : 1;
    faceBytes = imageFile->dimensions[0] * imageFile->dimensions[1] * bytesPerPixel;

    for ( face = 0; face < faceCount; face++ )
        facePrev[face] = NULL;

    for ( face = 0; face < faceCount; face++ )
        faceData[face] = ( byte * )Z_Malloc( faceBytes );

    for ( ; mip >= 0; mip-- )
    {
        int width  = decode.width  >> mip;
        int height = decode.height >> mip;
        int size;

        if ( width <= 1 )
            width = 1;

        if ( height <= 1 )
            height = 1;

        size = width * height * bytesPerPixel;

        decode.mipLevel = mip;

        for ( face = 0; face < faceCount; face++ )
        {
            byte *dst = faceData[face] + faceBytes - size;

            Image_DecodeWaveletMip( facePrev[face], dst, &decode );

            facePrev[face] = dst;

            if ( !face && !mip )
                Image_DecodeBitmap( dst, imageFile, image );
        }
    }

    for ( face = faceCount - 1; face >= 0; face-- )
        Z_Free( faceData[face] );
}

/* Image_Register  0x004543d0 */
void Image_Register( const char *imageName, Image_t *image )
{
    char        path[MAX_IMAGE_NAME];
    ImageFile_t *imageFile;
    const byte *data;
    int         len;

    Assert( imageName );

    Com_sprintf( path, sizeof( path ), "%s%s%s", "images/", imageName, ".iwi" );

    len = FS_ReadFile( path, ( void ** )&imageFile );
    if ( len < 0 )
        Com_Error( 1, "image '%s' is missing", path );

    if ( !Image_ValidateHeader( imageFile, path ) )
        Com_Error( 1, "image '%s' is not valid", path );

    strcpy( image->name, imageName );

    image->width  = imageFile->dimensions[0];
    image->height = imageFile->dimensions[1];
    image->pixels = ( byte * )Z_Malloc( image->width * image->height * 4 );

    data = ( const byte * )imageFile + 0x1c;

    switch ( imageFile->format )
    {
    case IMG_FORMAT_BITMAP_RGBA:
        image->hasAlpha = 1;
        Image_LoadBitmap( image, imageFile, data, 4 );
        break;

    case IMG_FORMAT_BITMAP_RGB:
        image->hasAlpha = 0;
        Image_LoadBitmap( image, imageFile, data, 3 );
        break;

    case IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
        image->hasAlpha = 1;
        Image_LoadBitmap( image, imageFile, data, 2 );
        break;

    case IMG_FORMAT_BITMAP_LUMINANCE:
        image->hasAlpha = 0;
        Image_LoadBitmap( image, imageFile, data, 1 );
        break;

    case IMG_FORMAT_BITMAP_ALPHA:
        image->hasAlpha = 1;
        Image_LoadBitmap( image, imageFile, data, 1 );
        break;

    case IMG_FORMAT_WAVELET_RGBA:
        image->hasAlpha = 1;
        Image_LoadWavelet( image, imageFile, data, 4 );
        break;

    case IMG_FORMAT_WAVELET_RGB:
        image->hasAlpha = 0;
        Image_LoadWavelet( image, imageFile, data, 3 );
        break;

    case IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
        image->hasAlpha = 1;
        Image_LoadWavelet( image, imageFile, data, 2 );
        break;

    case IMG_FORMAT_WAVELET_LUMINANCE:
        image->hasAlpha = 0;
        Image_LoadWavelet( image, imageFile, data, 1 );
        break;

    case IMG_FORMAT_WAVELET_ALPHA:
        image->hasAlpha = 1;
        Image_LoadWavelet( image, imageFile, data, 1 );
        break;

    case IMG_FORMAT_DXT1:
        image->hasAlpha = 0;
        Image_LoadDxt( image, imageFile, data, 8 );
        break;

    case IMG_FORMAT_DXT3:
    case IMG_FORMAT_DXT5:
        image->hasAlpha = 1;
        Image_LoadDxt( image, imageFile, data, 16 );
        break;

    default:
        Com_Error( 1, "image '%s': format %i is not reconstructed yet",
                   path, imageFile->format );
        break;
    }

    FS_FreeFile( imageFile );
}

/* Original: c:\trees\cod3\cod3src\src\universal\com_crc32.cpp */

#include "com_crc32.h"


#define CRC32_POLYNOMIAL    0xedb88320


/* Com_BlockChecksum32  0x00458300 */
unsigned Com_BlockChecksum32( const void *buffer, int length, unsigned crc )
{
    const unsigned char *p   = ( const unsigned char * )buffer;
    const unsigned char *end = p + length;

    crc = ~crc;

    while ( p != end )
    {
        int bit;

        crc ^= *p++;

        for ( bit = 0; bit < 8; bit++ )
            crc = ( crc >> 1 ) ^ ( ( crc & 1 ) * CRC32_POLYNOMIAL );
    }

    return ~crc;
}

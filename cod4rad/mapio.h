/* Original: c:\trees\cod3\cod3src\cod2rad\mapio.cpp */

#ifndef MAPIO_H
#define MAPIO_H

#include "q_shared.h"
#include "r_material.h"
#include "r_imagedecode.h"

byte *BuildAlphaMask( const Material_t *material, const Image_t *image ); /* 0x00418890 */
byte *BuildColorMask( const Image_t *image );                              /* 0x00418960 */

bool LoadMapFile( const char *mapName );        /* 0x00418810 */
void WriteMapFile( const char *mapName );       /* 0x00418880 */

#endif

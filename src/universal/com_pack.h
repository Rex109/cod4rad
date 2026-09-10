/* Original: c:\trees\cod3\cod3src\src\universal\com_pack.cpp */

#ifndef COM_PACK_H
#define COM_PACK_H

#include "q_shared.h"
#include "com_vector.h"


void         Vec3UnpackUnitVec( unsigned int packed, vec3_t out );  /* 0x00441b60 */

unsigned int Vec3PackUnitVec( const vec3_t v );                     /* 0x00441ad0 */

unsigned int Vec2PackTexCoords( const float *uv );                  /* 0x00442150 */
void         Vec2UnpackTexCoords( unsigned int packed, float *out );/* 0x004421f0 */

#endif

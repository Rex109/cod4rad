/* Original: ..\common\targetplatform.cpp */

#ifndef TARGETPLATFORM_H
#define TARGETPLATFORM_H

#include "q_shared.h"

typedef enum
{
    PLATFORM_VOID  = 0,
    PLATFORM_PC    = 1,
    PLATFORM_XENON = 2,
    PLATFORM_PS3   = 3,

    PLATFORM_COUNT = 4
} platform_t;

extern const char *g_platformNames[PLATFORM_COUNT];     /* 0x00529620 */

extern platform_t  targetPlatform;                      /* 0x14899cc4 */

qboolean SetTargetPlatformByName( const char *name );
void     PrintValidPlatforms( void );
qboolean ValidatePlatformSet( void );

#endif

/* Original: c:\trees\cod3\cod3src\src\gfx_d3d\r_light_load_obj.cpp */

#include "cod4rad.h"
#include "r_light_load_obj.h"

#include <string.h>


/* Light_Parse  0x00454610 */
static const char *Light_Parse( const char *buf, Image_t *image )
{
    const char *name;
    int         len;

    name = buf + 1;
    len = I_strlen( name );

    if ( len )
        Image_Register( name, image );
    else
        memset( image, 0, sizeof( Image_t ) );

    return name + len + 1;
}

/* Light_TryRegister  0x00454660 */
qboolean Light_TryRegister( const char *name, Image_t *image )
{
    void *buf;
    int   len;

    Assert( name );

    len = FS_ReadFile( va( "lights/%s", name ), &buf );

    if ( len < 0 )
        return qfalse;

    if ( !len )
    {
        FS_FreeFile( buf );
        return qfalse;
    }

    Light_Parse( ( const char * )buf, image );

    FS_FreeFile( buf );

    return qtrue;
}

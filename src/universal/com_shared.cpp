/* Original: ..\src\universal\com_shared.cpp */

#include "com_shared.h"
#include "assertive.h"
#include "dvar.h"
#include "com_files.h"
#include "cmdlib.h"


extern const char *Q_stristr(const char *s0, const char *substr);                          /* 0x0047f5f0 */

extern void  Com_Prefetch(const void *s, unsigned int bytes);                              /* 0x004765e0 */


/* Com_Memcpy  0x004762c0 */
void Com_Memcpy( void *dest, const void *src, int count )
{
    byte *d;
    const byte *s;
    int i;

    Assert( src || !count );
    Assert( dest || !count );

    Com_Prefetch( src, count );

    if ( count )
    {
        d = (byte *)dest;
        s = (const byte *)src;

        if ( count >= 32 )
        {
            for ( i = 0; i < (count & ~31); i += 32 )
            {
                ((int *)(d + i))[0] = ((const int *)(s + i))[0];
                ((int *)(d + i))[1] = ((const int *)(s + i))[1];
                ((int *)(d + i))[2] = ((const int *)(s + i))[2];
                ((int *)(d + i))[3] = ((const int *)(s + i))[3];
                ((int *)(d + i))[4] = ((const int *)(s + i))[4];
                ((int *)(d + i))[5] = ((const int *)(s + i))[5];
                ((int *)(d + i))[6] = ((const int *)(s + i))[6];
                ((int *)(d + i))[7] = ((const int *)(s + i))[7];
            }

            s += count & ~31;
            d += count & ~31;
            count &= 31;

            if ( count == 0 )
                return;
        }

        if ( count >= 16 )
        {
            ((int *)d)[0] = ((const int *)s)[0];
            ((int *)d)[1] = ((const int *)s)[1];
            ((int *)d)[2] = ((const int *)s)[2];
            ((int *)d)[3] = ((const int *)s)[3];
            count -= 16;
            s += 16;
            d += 16;
        }

        if ( count >= 8 )
        {
            ((int *)d)[0] = ((const int *)s)[0];
            ((int *)d)[1] = ((const int *)s)[1];
            count -= 8;
            s += 8;
            d += 8;
        }

        if ( count >= 4 )
        {
            ((int *)d)[0] = ((const int *)s)[0];
            count -= 4;
            s += 4;
            d += 4;
        }

        if ( count >= 2 )
        {
            *(short *)d = *(const short *)s;

            if ( count >= 3 )
                d[2] = s[2];
        }
        else if ( count >= 1 )
        {
            d[0] = s[0];
        }
    }
}

/* Com_Memset4  0x00476230 */
void Com_Memset4( void *dest, int fillValue, int count )
{
    int *p;
    int i;

    for ( i = 0; i < (count & ~7); i += 8 )
    {
        ((int *)dest)[i + 0] = fillValue;
        ((int *)dest)[i + 1] = fillValue;
        ((int *)dest)[i + 2] = fillValue;
        ((int *)dest)[i + 3] = fillValue;
        ((int *)dest)[i + 4] = fillValue;
        ((int *)dest)[i + 5] = fillValue;
        ((int *)dest)[i + 6] = fillValue;
        ((int *)dest)[i + 7] = fillValue;
    }

    if ( count & 7 )
    {
        p = (int *)dest + (count & ~7);
        count &= 7;

        if ( count >= 4 )
        {
            p[0] = fillValue;
            p[1] = fillValue;
            p[2] = fillValue;
            p[3] = fillValue;
            p += 4;
            count -= 4;
        }

        if ( count >= 2 )
        {
            p[0] = fillValue;
            p[1] = fillValue;
            p += 2;
            count -= 2;
        }

        if ( count >= 1 )
            p[0] = fillValue;
    }
}

/* Com_Memset  0x00476420 */
void Com_Memset( void *dest, int fillByte, int byteCount )
{
    unsigned short packedWord;
    int packedValue;
    byte *p;
    int rem;

    if ( byteCount < 8 )
    {
        packedWord = (unsigned short)((fillByte << 8) | fillByte);
        packedValue = (packedWord << 16) | packedWord;

        p = (byte *)dest;

        if ( byteCount >= 4 )
        {
            *(int *)p = packedValue;
            p += 4;
            byteCount -= 4;
        }

        if ( byteCount >= 2 )
        {
            *(short *)p = (short)packedValue;
            p += 2;
            byteCount -= 2;
        }

        if ( byteCount )
            *p = (char)packedValue;
    }
    else
    {
        fillByte = (fillByte << 8) | fillByte;

        Com_Memset4( dest, (fillByte << 16) | fillByte, byteCount / 4 );

        rem = byteCount & 3;

        if ( rem )
        {
            p = (byte *)dest + (byteCount & ~3);

            if ( rem < 2 )
            {
                if ( rem )
                    p[0] = (byte)fillByte;
            }
            else
            {
                *(short *)p = (short)fillByte;

                if ( rem != 2 )
                    p[2] = (byte)fillByte;
            }
        }
    }
}

/* Com_Memcmp  0x004764e0 */
qboolean Com_Memcmp( const void *a, const void *b, unsigned int count )
{
    unsigned int i;

    if ( count >= 16 )
    {
        for ( i = 0; i < (count >> 4); i += 4 )
        {
            if ( ((const int *)a)[i + 0] != ((const int *)b)[i + 0] ||
                 ((const int *)a)[i + 1] != ((const int *)b)[i + 1] ||
                 ((const int *)a)[i + 2] != ((const int *)b)[i + 2] ||
                 ((const int *)a)[i + 3] != ((const int *)b)[i + 3] )
            {
                return qfalse;
            }
        }
    }

    if ( count & 15 )
    {
        for ( i = count & ~15; i < count; i++ )
        {
            if ( ((const char *)a)[i] != ((const char *)b)[i] )
                return qfalse;
        }
    }

    return qtrue;
}


/* Com_Filter  0x00475c40 */
qboolean Com_Filter( const char *filter, const char *name, int casesensitive )
{
    char buf[MAX_TOKEN_CHARS];
    const char *ptr;
    int i, found;

    while ( *filter )
    {
        if ( *filter == '*' )
        {
            filter++;

            for ( i = 0; *filter; i++ )
            {
                if ( *filter == '*' || *filter == '?' )
                    break;

                buf[i] = *filter;
                filter++;
            }

            buf[i] = '\0';

            if ( strlen( buf ) )
            {
                if ( casesensitive )
                    ptr = Q_stristr( name, buf );
                else
                    ptr = strstr( name, buf );

                if ( !ptr )
                    return qfalse;

                name = ptr + strlen( buf );
            }
        }
        else if ( *filter == '?' )
        {
            filter++;
            name++;
        }
        else if ( *filter == '[' && *(filter + 1) == '[' )
        {
            filter++;
        }
        else if ( *filter == '[' )
        {
            filter++;
            found = qfalse;

            while ( *filter && !found )
            {
                if ( *filter == ']' && *(filter + 1) != ']' )
                    break;

                if ( *(filter + 1) == '-' && *(filter + 2) &&
                     ( *(filter + 2) != ']' || *(filter + 3) == ']' ) )
                {
                    if ( casesensitive )
                    {
                        if ( *filter <= *name && *name <= *(filter + 2) )
                            found = qtrue;
                    }
                    else
                    {
                        if ( toupper( *name ) >= toupper( *filter ) &&
                             toupper( *name ) <= toupper( *(filter + 2) ) )
                            found = qtrue;
                    }

                    filter += 3;
                }
                else
                {
                    if ( casesensitive )
                    {
                        if ( *filter == *name )
                            found = qtrue;
                    }
                    else
                    {
                        if ( toupper( *filter ) == toupper( *name ) )
                            found = qtrue;
                    }

                    filter++;
                }
            }

            if ( !found )
                return qfalse;

            while ( *filter )
            {
                if ( *filter == ']' && *(filter + 1) != ']' )
                    break;

                filter++;
            }

            filter++;
            name++;
        }
        else
        {
            if ( casesensitive )
            {
                if ( *filter != *name )
                    return qfalse;
            }
            else
            {
                if ( toupper( *filter ) != toupper( *name ) )
                    return qfalse;
            }

            filter++;
            name++;
        }
    }

    return qtrue;
}

/* Com_FilterPath  0x00475fc0 */
qboolean Com_FilterPath( const char *filter, const char *name, int casesensitive )
{
    int i;
    char new_filter[MAX_QPATH];
    char new_name[MAX_QPATH];

    for ( i = 0; i < MAX_QPATH - 1 && filter[i]; i++ )
    {
        if ( filter[i] == '\\' || filter[i] == ':' )
            new_filter[i] = '/';
        else
            new_filter[i] = filter[i];
    }
    new_filter[i] = '\0';

    for ( i = 0; i < MAX_QPATH - 1 && name[i]; i++ )
    {
        if ( name[i] == '\\' || name[i] == ':' )
            new_name[i] = '/';
        else
            new_name[i] = name[i];
    }
    new_name[i] = '\0';

    return Com_Filter( new_filter, new_name, casesensitive );
}


/* Com_HashKey  0x004760d0 */
int Com_HashKey( const char *string, int maxlen )
{
    int hash, i;

    hash = 0;

    for ( i = 0; i < maxlen && string[i] != '\0'; i++ )
        hash += string[i] * (i + 119);

    hash = hash ^ (hash >> 10) ^ (hash >> 20);

    return hash;
}


/* Com_GetLocalTime  0x00476140 */
int Com_GetLocalTime( struct tm *localTime )
{
    time_t t;
    struct tm *lt;

    t = Com_Time( NULL );

    if ( !localTime )
        return (int)t;

    lt = Com_LocalTime( &t );

    if ( lt )
        *localTime = *lt;

    return (int)t;
}

/* Com_LocalTime  0x004761f0 */
struct tm *Com_LocalTime( const time_t *t )
{
    return localtime( t );
}

/* Com_Time  0x00476210 */
time_t Com_Time( time_t *t )
{
    return time( t );
}


void Com_Printf( int channel, const char *fmt, ... )
{
    va_list argptr;
    (void)channel;
    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );
    fflush( stdout );
}

void Com_PrintError( int channel, const char *fmt, ... )
{
    va_list argptr;
    (void)channel;
    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );
    fflush( stdout );
}

void Com_PrintWarning( int channel, const char *fmt, ... )
{
    va_list argptr;
    (void)channel;
    va_start( argptr, fmt );
    vprintf( fmt, argptr );
    va_end( argptr );
    fflush( stdout );
}


/* Com_ErrorLevel  0x00408870 */

void Com_ErrorLevel( int errorLevel, const char *fmt, ... )
{
    char    msg[32772];
    va_list argptr;

    (void)errorLevel;
    va_start( argptr, fmt );
    vsprintf( msg, fmt, argptr );
    va_end( argptr );

    ::Com_Printf( msg, 0 );
}


/* Com_InitFilesystem  0x0040ef90 */

void Com_InitFilesystem( const char *basepath, const char *game, const char *basegame )
{
    Swap_Init();
    Dvar_Init();

    if ( basepath )
        Dvar_SetStringByName( "fs_basepath", basepath );
    if ( basegame )
        Dvar_SetStringByName( "fs_basegame", basegame );
    if ( game )
        Dvar_SetStringByName( "fs_game", game );

    FS_InitFilesystem();
}

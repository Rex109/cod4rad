/* Original: c:\trees\cod3\cod3src\src\script\scr_stringlist.cpp */

#include "cod4rad.h"
#include "scr_stringlist.h"
#include "scr_memorytree.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>


scrStringGlob_t scrStringGlob;


/* SL_ConvertToRefString  0x00428f50 */
static RefString *SL_ConvertToRefString( unsigned int stringValue )
{
    Assert( stringValue );
    Assert( stringValue * MT_NODE_SIZE < MT_SIZE );

    return ( RefString * )( scrMemTreePub.mt_buffer + stringValue * MT_NODE_SIZE );
}

/* SL_ConvertFromString  0x00428fc0 */
static RefString *SL_ConvertFromString( const char *str )
{
    Assert( str >= scrMemTreePub.mt_buffer && str < scrMemTreePub.mt_buffer + MT_SIZE );

    return ( RefString * )( str - 4 );
}

/* SL_ConvertToString  0x00429000 */
const char *SL_ConvertToString( unsigned int stringValue )
{
    if ( !stringValue )
        return NULL;

    return SL_ConvertToRefString( stringValue )->str;
}

/* SL_ConvertToStringSafe  0x00429020 */
const char *SL_ConvertToStringSafe( unsigned int stringValue )
{
    if ( !stringValue )
        return "(NULL)";

    return SL_ConvertToRefString( stringValue )->str;
}

/* SL_GetRefStringLen  0x004290b0 */
static int SL_GetRefStringLen( const RefString *refStr )
{
    int len;

    len = ( refStr->len - 1 ) & 0xff;

    while ( refStr->str[len] )
        len += 0x100;

    return len;
}

/* SL_ConvertToStringDebug  0x00429040 */
const char *SL_ConvertToStringDebug( unsigned int stringValue )
{
    const RefString *refStr;
    int              len;
    int              i;

    if ( !stringValue )
        return "<NULL>";

    refStr = SL_ConvertToRefString( stringValue );
    len    = ( refStr->len - 1 ) & 0xff;

    if ( refStr->str[len] )
        return "<BINARY>";

    for ( i = 0; i < len; i++ )
    {
        if ( !isprint( ( byte )refStr->str[i] ) )
            return "<BINARY>";
    }

    return refStr->str;
}

/* SL_GetStringLen  0x004290d0 */
int SL_GetStringLen( unsigned int stringValue )
{
    Assert( stringValue );

    return SL_GetRefStringLen( SL_ConvertToRefString( stringValue ) );
}

/* SL_ConvertFromRefString  0x00429130 */
static unsigned int SL_ConvertFromRefString( const RefString *refStr )
{
    return ( unsigned int )( ( ( const char * )refStr - scrMemTreePub.mt_buffer ) / MT_NODE_SIZE );
}

/* SL_GetStringValue  0x00429150 */
unsigned int SL_GetStringValue( const char *str )
{
    Assert( str );
    Assert( str >= scrMemTreePub.mt_buffer && str < scrMemTreePub.mt_buffer + MT_SIZE );

    return ( unsigned int )( ( str - scrMemTreePub.mt_buffer - 4 ) / MT_NODE_SIZE );
}

/* SL_GetHash  0x004291d0 */
static unsigned int SL_GetHash( int len, const char *str )
{
    unsigned int hash;
    int          i;

    if ( len >= 256 )
        return ( ( unsigned int )len >> 2 ) % HASH_MAX + 1;

    hash = 0;
    for ( i = 0; i < len; i++ )
        hash = hash * 31 + ( signed char )str[i];

    return hash % HASH_MAX + 1;
}

/* SL_Init  0x00429220 */
void SL_Init( void )
{
    unsigned int hash;
    unsigned int prev;

    Assert( !scrStringGlob.inited );

    MT_Init();

    Assert( Sys_IsMainThread() );

    scrStringGlob.hashTable[0].status_next = 0;

    prev = 0;
    for ( hash = 1; hash < HASH_COUNT; hash++ )
    {
        Assert( !( hash & HASH_STAT_MASK ) );

        scrStringGlob.hashTable[hash].status_next  = 0;
        scrStringGlob.hashTable[prev].status_next |= hash;
        scrStringGlob.hashTable[hash].prev         = prev;

        prev = hash;
    }

    Assert( !( scrStringGlob.hashTable[prev].status_next & HASH_NEXT_MASK ) );

    scrStringGlob.hashTable[0].prev = prev;

    scrStringGlob.inited = qtrue;
}

/* SL_Shutdown  0x00429310 */
void SL_Shutdown( void )
{
    if ( !scrStringGlob.inited )
        return;

    scrStringGlob.inited = qfalse;
}

/* SL_FindStringOfSize  0x00429330 */
unsigned int SL_FindStringOfSize( const char *str, int len )
{
    HashEntry   *entry;
    HashEntry   *cur;
    RefString   *refStr;
    unsigned int hash;
    unsigned int curHash;
    unsigned int prevHash;
    unsigned int value;
    byte         lenByte;

    Assert( str );

    hash    = SL_GetHash( len, str );
    entry   = &scrStringGlob.hashTable[hash];
    lenByte = ( byte )len;

    Assert( Sys_IsMainThread() );

    if ( ( entry->status_next & HASH_STAT_MASK ) != HASH_STAT_HEAD )
        return 0;

    refStr = SL_ConvertToRefString( entry->prev );
    if ( refStr->len == lenByte && !memcmp( refStr->str, str, len ) )
    {
        Assert( entry->status_next & HASH_STAT_MASK );
        value = entry->prev;
        Assert( refStr->str == SL_ConvertToString( value ) );
        return value;
    }

    prevHash = hash;
    curHash  = entry->status_next & HASH_NEXT_MASK;
    cur      = &scrStringGlob.hashTable[curHash];

    while ( cur != entry )
    {
        Assert( ( cur->status_next & HASH_STAT_MASK ) == HASH_STAT_MOVABLE );

        refStr = SL_ConvertToRefString( cur->prev );
        if ( refStr->len == lenByte && !memcmp( refStr->str, str, len ) )
        {
            unsigned int found;

            scrStringGlob.hashTable[prevHash].status_next =
                ( scrStringGlob.hashTable[prevHash].status_next & HASH_STAT_MASK )
                | ( cur->status_next & HASH_NEXT_MASK );

            cur->status_next = ( cur->status_next & HASH_STAT_MASK )
                             | ( entry->status_next & HASH_NEXT_MASK );

            entry->status_next = ( entry->status_next & HASH_STAT_MASK ) | curHash;

            found      = cur->prev;
            cur->prev  = entry->prev;
            entry->prev = found;

            Assert( entry->status_next & HASH_STAT_MASK );
            Assert( cur->status_next & HASH_STAT_MASK );
            Assert( refStr->str == SL_ConvertToString( found ) );

            return found;
        }

        prevHash = curHash;
        curHash  = cur->status_next & HASH_NEXT_MASK;
        cur      = &scrStringGlob.hashTable[curHash];
    }

    return 0;
}

/* SL_FindString  0x004296b0 */
unsigned int SL_FindString( const char *str )
{
    return SL_FindStringOfSize( str, ( int )strlen( str ) + 1 );
}

/* SL_FindLowercaseString  0x004296e0 */
unsigned int SL_FindLowercaseString( const char *str )
{
    char buf[MAX_STRING_CHARS_SL + 4];
    int  len;
    int  i;

    len = ( int )strlen( str ) + 1;
    if ( len > MAX_STRING_CHARS_SL )
        return 0;

    for ( i = 0; i < len; i++ )
        buf[i] = ( char )tolower( str[i] );

    return SL_FindStringOfSize( buf, len );
}

/* SL_IsStringLowercase  0x00429770 */
qboolean SL_IsStringLowercase( unsigned int stringValue )
{
    const char *str;

    Assert( stringValue );

    str = SL_ConvertToRefString( stringValue )->str;

    while ( *str )
    {
        if ( *str != ( char )tolower( *str ) )
            return qfalse;
        str++;
    }

    return qtrue;
}

/* SL_AddUser  0x004297e0 */
static void SL_AddUser( RefString *refStr, int user )
{
    if ( refStr->user & user )
        return;

    *( unsigned int * )refStr = ( ( ( unsigned int )user << 16 ) | *( unsigned int * )refStr ) + 1;
}

/* SL_AddRefToStringOfUser  0x00429800 */
void SL_AddRefToStringOfUser( unsigned int stringValue, int user )
{
    SL_AddUser( SL_ConvertToRefString( stringValue ), user );
}

/* SL_GetStringUser  0x00429830 */
int SL_GetStringUser( unsigned int stringValue )
{
    return SL_ConvertToRefString( stringValue )->user;
}

/* SL_GetStringOfSize  0x00429850 */
unsigned int SL_GetStringOfSize( const char *str, int user, int len, int type )
{
    HashEntry   *entry;
    HashEntry   *cur;
    RefString   *refStr;
    unsigned int hash;
    unsigned int curHash;
    unsigned int prevHash;
    unsigned int value;
    unsigned int freeHash;
    byte         lenByte;

    Assert( str );

    hash    = SL_GetHash( len, str );
    entry   = &scrStringGlob.hashTable[hash];
    lenByte = ( byte )len;

    Assert( Sys_IsMainThread() );

    if ( ( entry->status_next & HASH_STAT_MASK ) == HASH_STAT_HEAD )
    {
        refStr = SL_ConvertToRefString( entry->prev );

        if ( refStr->len == lenByte && !memcmp( refStr->str, str, len ) )
        {
            SL_AddUser( refStr, user );

            Assert( entry->status_next & HASH_STAT_MASK );
            value = entry->prev;
            Assert( refStr->str == SL_ConvertToString( value ) );
            return value;
        }

        prevHash = hash;
        curHash  = entry->status_next & HASH_NEXT_MASK;
        cur      = &scrStringGlob.hashTable[curHash];

        while ( cur != entry )
        {
            Assert( ( cur->status_next & HASH_STAT_MASK ) == HASH_STAT_MOVABLE );

            refStr = SL_ConvertToRefString( cur->prev );
            if ( refStr->len == lenByte && !memcmp( refStr->str, str, len ) )
            {
                unsigned int found;

                scrStringGlob.hashTable[prevHash].status_next =
                    ( scrStringGlob.hashTable[prevHash].status_next & HASH_STAT_MASK )
                    | ( cur->status_next & HASH_NEXT_MASK );

                cur->status_next = ( cur->status_next & HASH_STAT_MASK )
                                 | ( entry->status_next & HASH_NEXT_MASK );

                entry->status_next = ( entry->status_next & HASH_STAT_MASK ) | curHash;

                found       = cur->prev;
                cur->prev   = entry->prev;
                entry->prev = found;

                SL_AddUser( refStr, user );

                Assert( cur->status_next & HASH_STAT_MASK );
                Assert( entry->status_next & HASH_STAT_MASK );
                Assert( refStr->str == SL_ConvertToString( found ) );

                return found;
            }

            prevHash = curHash;
            curHash  = cur->status_next & HASH_NEXT_MASK;
            cur      = &scrStringGlob.hashTable[curHash];
        }

        freeHash = scrStringGlob.hashTable[0].status_next;
        if ( !freeHash )
            Com_Error( 1, "Out of hash table space for strings" );

        value = MT_AllocIndex( len + 4, type );

        Assert( ( scrStringGlob.hashTable[freeHash].status_next & HASH_STAT_MASK ) == HASH_STAT_FREE );

        scrStringGlob.hashTable[0].status_next =
            scrStringGlob.hashTable[freeHash].status_next & HASH_NEXT_MASK;
        scrStringGlob.hashTable[scrStringGlob.hashTable[0].status_next].prev = 0;

        scrStringGlob.hashTable[freeHash].status_next =
            ( entry->status_next & HASH_NEXT_MASK ) | HASH_STAT_MOVABLE;
        scrStringGlob.hashTable[freeHash].prev = entry->prev;

        entry->status_next = ( entry->status_next & HASH_STAT_MASK ) | freeHash;
    }
    else if ( ( entry->status_next & HASH_STAT_MASK ) == HASH_STAT_FREE )
    {
        unsigned int nextFree;
        unsigned int prevFree;

        value = MT_AllocIndex( len + 4, type );

        nextFree = entry->status_next & HASH_NEXT_MASK;
        prevFree = entry->prev;

        scrStringGlob.hashTable[prevFree].status_next =
            ( scrStringGlob.hashTable[prevFree].status_next & HASH_STAT_MASK ) | nextFree;
        scrStringGlob.hashTable[nextFree].prev = prevFree;

        Assert( !( hash & HASH_STAT_MASK ) );
        entry->status_next = hash | HASH_STAT_HEAD;
    }
    else
    {
        unsigned int foreignNext;
        unsigned int walk;
        unsigned int prev;

        Assert( ( entry->status_next & HASH_STAT_MASK ) == HASH_STAT_MOVABLE );

        foreignNext = entry->status_next & HASH_NEXT_MASK;

        prev = foreignNext;
        walk = scrStringGlob.hashTable[prev].status_next & HASH_NEXT_MASK;
        while ( walk != hash )
        {
            prev = walk;
            walk = scrStringGlob.hashTable[prev].status_next & HASH_NEXT_MASK;
        }
        Assert( prev );

        freeHash = scrStringGlob.hashTable[0].status_next;
        if ( !freeHash )
            Com_Error( 1, "Out of hash table space for strings" );

        value = MT_AllocIndex( len + 4, type );

        Assert( ( scrStringGlob.hashTable[freeHash].status_next & HASH_STAT_MASK ) == HASH_STAT_FREE );

        scrStringGlob.hashTable[0].status_next =
            scrStringGlob.hashTable[freeHash].status_next & HASH_NEXT_MASK;
        scrStringGlob.hashTable[scrStringGlob.hashTable[0].status_next].prev = 0;

        scrStringGlob.hashTable[prev].status_next =
            ( scrStringGlob.hashTable[prev].status_next & HASH_STAT_MASK ) | freeHash;

        scrStringGlob.hashTable[freeHash].status_next = foreignNext | HASH_STAT_MOVABLE;
        scrStringGlob.hashTable[freeHash].prev        = entry->prev;

        Assert( !( hash & HASH_STAT_MASK ) );
        entry->status_next = hash | HASH_STAT_HEAD;
    }

    Assert( value );
    entry->prev = value;

    refStr = SL_ConvertToRefString( value );
    memcpy( refStr->str, str, len );

    Assert( user == ( byte )user );
    refStr->user     = ( byte )user;
    refStr->refCount = 1;
    refStr->len      = lenByte;

    Assert( entry->status_next & HASH_STAT_MASK );
    Assert( refStr->str == SL_ConvertToString( value ) );

    return value;
}

/* SL_GetStringOfType  0x00429f00 */
unsigned int SL_GetStringOfType( const char *str, int user, int type )
{
    return SL_GetStringOfSize( str, user, ( int )strlen( str ) + 1, type );
}

/* SL_GetString  0x00429f40 */
unsigned int SL_GetString( const char *str, int user )
{
    return SL_GetStringOfSize( str, user, ( int )strlen( str ) + 1, MEM_EXTERNAL );
}

/* SL_GetLowercaseStringOfSize  0x00429f80 */
static unsigned int SL_GetLowercaseStringOfSize( const char *str, int user, int len, int type )
{
    char buf[MAX_STRING_CHARS_SL + 4];
    int  i;

    if ( ( unsigned int )len > MAX_STRING_CHARS_SL )
    {
        Com_Error( 1, "max string length exceeded: \"%s\"", str );
        return 0;
    }

    for ( i = 0; i < len; i++ )
        buf[i] = ( char )tolower( str[i] );

    return SL_GetStringOfSize( buf, user, len, type );
}

/* SL_GetLowercaseStringOfType  0x0042a010 */
unsigned int SL_GetLowercaseStringOfType( const char *str, int user, int type )
{
    return SL_GetLowercaseStringOfSize( str, user, ( int )strlen( str ) + 1, type );
}

/* SL_GetLowercaseString  0x0042a050 */
unsigned int SL_GetLowercaseString( const char *str, int user )
{
    return SL_GetLowercaseStringOfSize( str, user, ( int )strlen( str ) + 1, MEM_EXTERNAL );
}

/* SL_RemoveRefToStringOfUser  0x0042a090 */
void SL_RemoveRefToStringOfUser( unsigned int stringValue, int user )
{
    RefString *refStr;

    refStr = SL_ConvertToRefString( stringValue );

    if ( refStr->user & user )
    {
        Assertx( refStr->refCount > 1, "%s", SL_ConvertToStringDebug( stringValue ) );
        *( unsigned int * )refStr -= 1;
        return;
    }

    *( unsigned int * )refStr |= ( unsigned int )user << 16;
}

/* SL_AddRefToString  0x0042a0f0 */
void SL_AddRefToString( unsigned int stringValue )
{
    *( unsigned int * )SL_ConvertToRefString( stringValue ) += 1;
}

/* SL_FreeString  0x0042a110 */
static void SL_FreeString( unsigned int stringValue, RefString *refStr, int len )
{
    HashEntry   *entry;
    HashEntry   *cur;
    unsigned int hash;
    unsigned int curHash;
    unsigned int prevHash;

    hash  = SL_GetHash( len, refStr->str );
    entry = &scrStringGlob.hashTable[hash];

    Assert( Sys_IsMainThread() );
    Assert( !refStr->refCount );
    Assertx( !refStr->user, "%s", SL_ConvertToStringDebug( stringValue ) );

    MT_FreeIndex( stringValue, len + 4 );

    Assert( ( entry->status_next & HASH_STAT_MASK ) == HASH_STAT_HEAD );

    curHash = entry->status_next & HASH_NEXT_MASK;
    cur     = &scrStringGlob.hashTable[curHash];

    if ( entry->prev == stringValue )
    {
        if ( cur != entry )
        {
            entry->status_next = ( cur->status_next & HASH_NEXT_MASK ) | HASH_STAT_HEAD;
            entry->prev        = cur->prev;
            scrStringGlob.lastHead = entry;
        }
        else
        {
            curHash = hash;
            cur     = entry;
        }
    }
    else
    {
        prevHash = hash;

        for ( ;; )
        {
            Assert( cur != entry );
            Assert( ( cur->status_next & HASH_STAT_MASK ) == HASH_STAT_MOVABLE );

            if ( cur->prev == stringValue )
                break;

            prevHash = curHash;
            curHash  = cur->status_next & HASH_NEXT_MASK;
            cur      = &scrStringGlob.hashTable[curHash];
        }

        scrStringGlob.hashTable[prevHash].status_next =
            ( scrStringGlob.hashTable[prevHash].status_next & HASH_STAT_MASK )
            | ( cur->status_next & HASH_NEXT_MASK );
    }

    Assert( cur->status_next & HASH_STAT_MASK );
    Assert( !( scrStringGlob.hashTable[0].status_next & HASH_STAT_MASK ) );

    cur->status_next = scrStringGlob.hashTable[0].status_next;
    cur->prev        = 0;

    scrStringGlob.hashTable[scrStringGlob.hashTable[0].status_next].prev = curHash;
    scrStringGlob.hashTable[0].status_next = curHash;
}

/* SL_RemoveRefToStringOfSize  0x0042a320 */
void SL_RemoveRefToStringOfSize( unsigned int stringValue, int len )
{
    RefString *refStr;

    refStr = SL_ConvertToRefString( stringValue );

    *( unsigned int * )refStr -= 1;

    if ( !( *( unsigned int * )refStr & 0xffff ) )
        SL_FreeString( stringValue, refStr, len );
}

/* SL_GetSystemString  0x0042a350 */
unsigned int SL_GetSystemString( const char *str, int sys )
{
    Assert( sys == SCR_SYS_GAME );

    return SL_GetStringOfSize( str, SCR_SYS_GAME, ( int )strlen( str ) + 1, MEM_EXTERNAL );
}

/* SL_GetStringForFloat  0x0042a3a0 */
unsigned int SL_GetStringForFloat( float value )
{
    char buf[128];

    sprintf( buf, "%g", value );

    return SL_GetStringOfSize( buf, 0, ( int )strlen( buf ) + 1, MEM_SCRIPT_STRING );
}

/* SL_GetStringForInt  0x0042a400 */
unsigned int SL_GetStringForInt( int value )
{
    char buf[128];

    sprintf( buf, "%i", value );

    return SL_GetStringOfSize( buf, 0, ( int )strlen( buf ) + 1, MEM_SCRIPT_STRING );
}

/* SL_GetStringForVector  0x0042a450 */
unsigned int SL_GetStringForVector( const float *value )
{
    char buf[128];

    sprintf( buf, "(%g, %g, %g)", value[0], value[1], value[2] );

    return SL_GetStringOfSize( buf, 0, ( int )strlen( buf ) + 1, MEM_SCRIPT_STRING );
}

/* SL_TransferUser  0x0042a4c0 */
void SL_TransferUser( int from, int to )
{
    unsigned int hash;
    RefString   *refStr;

    Assert( from );
    Assert( to );
    Assert( Sys_IsMainThread() );

    for ( hash = 1; hash < HASH_COUNT; hash++ )
    {
        if ( !( scrStringGlob.hashTable[hash].status_next & HASH_STAT_MASK ) )
            continue;

        refStr = SL_ConvertToRefString( scrStringGlob.hashTable[hash].prev );
        if ( !( refStr->user & from ) )
            continue;

        refStr->user = ( byte )( ( refStr->user & ~from ) | to );
    }
}

/* SL_ConvertFilename  0x0042a580 */
static void SL_ConvertFilename( int maxLen, const char *src, char *dst )
{
    int c;

    Assert( maxLen );

    for ( ;; )
    {
        do
        {
            c = ( signed char )*src++;
        }
        while ( c == '\\' || c == '/' );

        if ( c < ' ' )
            break;

        for ( ;; )
        {
            *dst++ = ( char )tolower( c );

            if ( !--maxLen )
                Com_Error( 1, "Filename '%s' exceeds maximum length of %d", src, maxLen );

            if ( c == '/' )
                break;

            c = ( signed char )*src++;
            if ( c == '\\' )
            {
                c = '/';
                continue;
            }
            if ( c < ' ' )
                goto done;
        }
    }

done:
    *dst = '\0';
}

/* SL_GetCanonicalFilename  0x0042a620 */
unsigned int SL_GetCanonicalFilename( const char *filename )
{
    char buf[MAX_OS_PATH];

    SL_ConvertFilename( MAX_OS_PATH, filename, buf );

    return SL_GetStringOfSize( buf, 0, ( int )strlen( buf ) + 1, MEM_TEMP );
}

/* SL_RemoveRefToString  0x0042a670 */
void SL_RemoveRefToString( unsigned int stringValue )
{
    RefString *refStr;
    int        len;

    len = SL_GetRefStringLen( SL_ConvertToRefString( stringValue ) );

    refStr = SL_ConvertToRefString( stringValue );

    *( unsigned int * )refStr -= 1;

    if ( !( *( unsigned int * )refStr & 0xffff ) )
        SL_FreeString( stringValue, refStr, len + 1 );
}

/* SL_SetStringValue  0x0042a6e0 */
void SL_SetStringValue( unsigned short *ref, unsigned int stringValue )
{
    if ( stringValue )
        SL_AddRefToString( stringValue );

    if ( *ref )
        SL_RemoveRefToString( *ref );

    *ref = ( unsigned short )stringValue;
}

/* SL_SetString  0x0042a720 */
void SL_SetString( unsigned short *ref, const char *str )
{
    if ( *ref )
        SL_RemoveRefToString( *ref );

    *ref = ( unsigned short )SL_GetStringOfSize( str, 0, ( int )strlen( str ) + 1, MEM_EXTERNAL );
}

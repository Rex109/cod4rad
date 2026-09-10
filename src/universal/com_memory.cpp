/* Original: ..\src\universal\com_memory.cpp */

#include "com_memory.h"
#include "assertive.h"


extern void   Com_ErrorLevel(int errorLevel, const char *fmt, ...);                       /* 0x00408870 */
extern void   Com_Printf(int channel, const char *fmt, ...);                              /* 0x0040eec0 */
extern void   Com_PrintError(int channel, const char *fmt, ...);                          /* 0x0040ef10 */

extern int    I_strlen(const char *string);                                               /* 0x0040a290 */
extern int    Q_stricmp(const char *s0, const char *s1);                                  /* 0x0047f6d0 */
extern char  *va(const char *format, ...);                                                /* 0x0047fbd0 */
extern qboolean Sys_IsMainThread(void);                                                   /* 0x00481fc0 */
extern qboolean Sys_IsRenderThread(void);                                                 /* 0x00481fb0 */

extern void   Com_Memset(void *dest, int fillByte, int byteCount);                        /* 0x00476420 */

extern int    FS_LoadStack(void);                                                         /* 0x004676e0 */
extern unsigned FS_HashFileName(const char *fname, int hashSize);                         /* 0x00467760 */

#define ERR_FATAL              0
#define ERR_DROP               1

#define CON_CHANNEL_SYSTEM     16


static HunkUser        *g_debugUser;                                /* 0x11ba6fdc */
static int              s_hunkTotal;                                /* 0x11ba6fe0 */
static hunkFileEntry_t *com_hunkData;                               /* 0x11ba6fe4 */
static hunkUsed_t       hunk_high;                                  /* 0x11ba6fe8 */
static hunkUsed_t       hunk_low;                                   /* 0x11ba6ff0 */
static hunkFileEntry_t *com_fileDataHashTable[FILE_DATA_HASH_SIZE]; /* 0x11ba6ff8 */
static psize_int        s_hunkData;                                 /* 0x11ba7ffc */
static void            *s_origHunkData;                             /* 0x11ba8000 */


/* Z_VirtualReserve  0x004128b0 */
void *Z_VirtualReserve( int size )
{
    void *buf;

    Assertx( (size > 0), "(size) = %i", size );

    buf = VirtualAlloc( NULL, size, MEM_RESERVE, PAGE_READWRITE );

    Assert( buf );

    return buf;
}

/* Z_TryVirtualCommit  0x004129e0 */
bool Z_TryVirtualCommit( void *buf, int size )
{
    Assertx( (size >= 0), "(size) = %i", size );

    return VirtualAlloc( buf, size, MEM_COMMIT, PAGE_READWRITE ) != NULL;
}

/* Z_VirtualDecommit  0x00412930 */
void Z_VirtualDecommit( void *buf, int size )
{
    Assertx( (size >= 0), "(size) = %i", size );

    VirtualFree( buf, size, MEM_DECOMMIT );
}

/* Z_VirtualFree  0x00412980 */
void Z_VirtualFree( void *buf )
{
    VirtualFree( buf, 0, MEM_RELEASE );
}

/* Z_VirtualCommit  0x00412a40 */
void Z_VirtualCommit( void *buf, int size )
{
    if ( !Z_TryVirtualCommit( buf, size ) )
    {
        AssertMsg( va( "Z_VirtualCommit: failed to allocate %d bytes", size ) );
        Com_Printf( CON_CHANNEL_SYSTEM, "Z_VirtualCommit: failed to allocate %d bytes\n", size );
        Com_ErrorLevel( ERR_FATAL, "Z_VirtualCommit: EXE_ERR_OUT_OF_MEMORY" );
    }
}

/* Z_TryVirtualAlloc  0x004129a0 */
void *Z_TryVirtualAlloc( int size )
{
    void *buf;

    buf = Z_VirtualReserve( size );

    if ( !Z_TryVirtualCommit( buf, size ) )
    {
        Z_VirtualFree( buf );
        buf = NULL;
    }

    return buf;
}

/* Z_Free  0x00412ab0 */
void Z_Free( void *ptr )
{
    free( ptr );
}

/* Z_TryMallocInternal  0x00412b10 */
void *Z_TryMallocInternal( int size )
{
    return malloc( size );
}

/* Z_TryMalloc  0x00412ad0 */
void *Z_TryMalloc( int size )
{
    void *buf;

    buf = Z_TryMallocInternal( size );

    if ( buf )
        Com_Memset( buf, 0, size );

    return buf;
}

/* Z_MallocFailed  0x00412b60 */
void Z_MallocFailed( int size )
{
    AssertMsg( va( "Z_Malloc: failed to allocate %d bytes", size ) );
    Com_PrintError( CON_CHANNEL_SYSTEM, "Z_Malloc: failed to allocate %d bytes\n", size );
    Com_ErrorLevel( ERR_FATAL, "Z_Malloc: EXE_ERR_OUT_OF_MEMORY" );
}

/* Z_Malloc  0x00412b30 */
void *Z_Malloc( int size )
{
    void *buf;

    buf = Z_TryMalloc( size );

    if ( !buf )
        Z_MallocFailed( size );

    return buf;
}

/* Z_MallocGarbage  0x00412bc0 */
void *Z_MallocGarbage( int size )
{
    void *buf;

    buf = Z_TryMallocInternal( size );

    if ( !buf )
        Z_MallocFailed( size );

    return buf;
}

/* Z_VirtualAlloc  0x00412bf0 */
void *Z_VirtualAlloc( int size )
{
    void *buf;

    buf = Z_TryVirtualAlloc( size );

    if ( !buf )
    {
        AssertMsg( va( "Z_VirtualAlloc: failed to allocate %d bytes", size ) );
        Com_Printf( CON_CHANNEL_SYSTEM, "Z_VirtualAlloc: failed to allocate %d bytes\n", size );
        Com_ErrorLevel( ERR_FATAL, "Z_VirtualAlloc: EXE_ERR_OUT_OF_MEMORY" );
    }

    return buf;
}

/* CopyString  0x00412c70 */
char *CopyString( const char *in )
{
    char *out;

    Assert( in );

    out = (char *)Z_Malloc( I_strlen( in ) + 1 );
    strcpy( out, in );

    return out;
}

/* SetString  0x00412cd0 */
void SetString( char **str, const char *in )
{
    int len;

    Assert( str );
    Assert( in );

    len = I_strlen( in );

    if ( *str )
        Z_Free( *str );

    *str = (char *)Z_Malloc( len + 1 );
    strcpy( *str, in );
}

/* FreeString  0x00412d80 */
void FreeString( char *str )
{
    Assert( str );

    Z_Free( str );
}


/* Hunk_InitMemory  0x00412dc0 */
void Hunk_InitMemory( void )
{
    Assert( Sys_IsMainThread() );
    Assert( !s_hunkData );

    if ( FS_LoadStack() )
        Com_ErrorLevel( ERR_FATAL, "Hunk initialization failed. File system load stack not zero" );

    if ( Hunk_UseLargeHunk() )
        s_hunkTotal = HUNK_SIZE_LARGE;

    s_hunkData = (psize_int)VirtualAlloc( NULL, s_hunkTotal, MEM_COMMIT, PAGE_READWRITE );

    if ( !s_hunkData )
        Com_ErrorLevel( ERR_FATAL, va( "EXE_ERR_HUNK_ALLOC_FAILED%i", s_hunkTotal / (1024 * 1024) ) );

    s_origHunkData = (void *)s_hunkData;

    Hunk_Clear();
}

/* Hunk_Shutdown  0x00412eb0 */
void Hunk_Shutdown( void )
{
    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    memset( com_fileDataHashTable, 0, sizeof( com_fileDataHashTable ) );

    Assert( s_origHunkData );

    VirtualFree( s_origHunkData, 0, MEM_RELEASE );

    s_origHunkData = NULL;
    s_hunkData = 0;
    s_hunkTotal = 0;

    memset( &hunk_low, 0, sizeof( hunk_low ) );
    memset( &hunk_high, 0, sizeof( hunk_high ) );
}


/* Hunk_FindDataForFileInternal  0x00412fd0 */
void *Hunk_FindDataForFileInternal( int type, const char *name, int hash )
{
    hunkFileEntry_t *fileData;

    for ( fileData = com_fileDataHashTable[hash]; ; fileData = fileData->next )
    {
        if ( !fileData )
            return NULL;

        if ( fileData->type == type && !Q_stricmp( fileData->name, name ) )
            break;
    }

    return fileData->data;
}

/* Hunk_FindDataForFile  0x00412fa0 */
void *Hunk_FindDataForFile( int type, const char *name )
{
    int hash;

    hash = FS_HashFileName( name, FILE_DATA_HASH_SIZE );

    return Hunk_FindDataForFileInternal( type, name, hash );
}

/* Hunk_IsInHunk  0x00413030 */
qboolean Hunk_IsInHunk( const void *ptr )
{
    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    if ( ptr < (const void *)s_hunkData )
        return qfalse;

    if ( ptr >= (const void *)(s_hunkData + s_hunkTotal) )
        return qfalse;

    return qtrue;
}

/* Hunk_AddDataForFile  0x004130c0 */
char *Hunk_AddDataForFile( int type, const char *name, void *data, HunkAllocFunc_t allocFunc )
{
    hunkFileEntry_t *fileData;
    int hash;

    Assert( Sys_IsMainThread() );

    hash = FS_HashFileName( name, FILE_DATA_HASH_SIZE );

    Assert( !Hunk_FindDataForFileInternal( type, name, hash ) );

    fileData = (hunkFileEntry_t *)allocFunc( I_strlen( name ) + 10 );
    fileData->data = data;
    fileData->type = (byte)type;

    Assert( type == fileData->type );

    strcpy( fileData->name, name );

    fileData->next = com_fileDataHashTable[hash];
    com_fileDataHashTable[hash] = fileData;

    return fileData->name;
}

/* Hunk_AddData  0x004131d0 */
void Hunk_AddData( int type, void *data, HunkAllocFunc_t allocFunc )
{
    hunkFileEntry_t *fileData;

    Assert( Sys_IsMainThread() );

    fileData = (hunkFileEntry_t *)allocFunc( 9 );
    fileData->data = data;
    fileData->type = (byte)type;

    Assert( type == fileData->type );

    fileData->next = com_hunkData;
    com_hunkData = fileData;
}

/* Hunk_OverrideDataForFile  0x00413260 */
void Hunk_OverrideDataForFile( int type, const char *name, void *data )
{
    hunkFileEntry_t *fileData;
    int hash;

    Assert( Sys_IsMainThread() );

    hash = FS_HashFileName( name, FILE_DATA_HASH_SIZE );

    for ( fileData = com_fileDataHashTable[hash]; ; fileData = fileData->next )
    {
        if ( !fileData )
        {
            AssertMsg( "Hunk_OverrideDataForFile: could not find data" );
            return;
        }

        if ( fileData->type == type && !Q_stricmp( fileData->name, name ) )
            break;
    }

    fileData->data = data;
}

/* Hunk_PurgeFreeListRange  0x00413640 */
void Hunk_PurgeFreeListRange( hunkFileEntry_t **listHead, unsigned int lowAddr, unsigned int highAddr )
{
    hunkFileEntry_t *fileData;

    Assert( Sys_IsMainThread() );

    while ( *listHead )
    {
        fileData = *listHead;

        if ( (unsigned int)fileData < lowAddr || highAddr <= (unsigned int)fileData )
            listHead = &fileData->next;
        else
            *listHead = fileData->next;
    }
}

/* Hunk_ClearTempMemory  0x00413590 */
void Hunk_ClearTempMemory( void )
{
    unsigned int lowAddr, highAddr;
    unsigned int i;

    Assert( Sys_IsMainThread() );

    lowAddr  = s_hunkData + hunk_low.permanent;
    highAddr = (s_hunkData + s_hunkTotal) - hunk_high.permanent;

    for ( i = 0; i < FILE_DATA_HASH_SIZE; i++ )
        Hunk_PurgeFreeListRange( &com_fileDataHashTable[i], lowAddr, highAddr );

    Hunk_PurgeFreeListRange( &com_hunkData, lowAddr, highAddr );
}


/* Hunk_EnumerateFileData  0x00413490 */
void Hunk_EnumerateFileData( hunkFileEntry_t *fileData, int type,
                             HunkAssetFunc_t callback, void *userData )
{
    for ( ; fileData; fileData = fileData->next )
    {
        if ( fileData->type == type && (char)fileData->type == 5 )
            callback( fileData->data, userData );
    }
}

/* Hunk_EnumerateAssetsInternal  0x00413420 */
void Hunk_EnumerateAssetsInternal( int assetType, HunkAssetFunc_t callback,
                                   void *userData, bool includeTemp )
{
    hunkFileEntry_t *fileData;
    unsigned int i;
    int type;

    switch ( assetType )
    {
    case 3:
        type = 5;
        break;

    default:
        return;
    }

    for ( i = 0; i < FILE_DATA_HASH_SIZE; i++ )
    {
        fileData = com_fileDataHashTable[i];
        Hunk_EnumerateFileData( fileData, type, callback, userData );
    }
}

/* Hunk_EnumerateAssets  0x00413400 */
void Hunk_EnumerateAssets( int assetType, HunkAssetFunc_t callback,
                           void *userData, bool includeTemp )
{
    Hunk_EnumerateAssetsInternal( assetType, callback, userData, includeTemp );
}

/* Hunk_AddAssetToList  0x00413380 */
void Hunk_AddAssetToList( void *data, assetList_t *assetList )
{
    Assert( data );
    Assert( assetList->assetCount < assetList->maxCount );

    assetList->assets[assetList->assetCount] = data;
    assetList->assetCount++;
}

/* Hunk_GetAssetListInternal  0x00413340 */
int Hunk_GetAssetListInternal( int assetType, void **assets, int maxCount )
{
    assetList_t assetList;

    assetList.assets = assets;
    assetList.assetCount = 0;
    assetList.maxCount = maxCount;

    Hunk_EnumerateAssets( assetType, (HunkAssetFunc_t)Hunk_AddAssetToList, &assetList, false );

    return assetList.assetCount;
}

/* Hunk_GetAssetList  0x00413320 */
int Hunk_GetAssetList( int assetType, void **assets, int maxCount )
{
    return Hunk_GetAssetListInternal( assetType, assets, maxCount );
}


/* Hunk_HighMark  0x00413500 */
int Hunk_HighMark( void )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryHigh();

    return hunk_high.permanent;
}

/* Hunk_ClearHighToMark  0x00413540 */
void Hunk_ClearHighToMark( int mark )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryHigh();

    hunk_high.temp = mark;
    hunk_high.permanent = mark;

    Hunk_ClearTempMemory();
}

/* Hunk_LowMark  0x004136c0 */
int Hunk_LowMark( void )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryLow();

    return hunk_low.permanent;
}

/* Hunk_ClearLowToMark  0x00413700 */
void Hunk_ClearLowToMark( int mark )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryLow();

    hunk_low.temp = mark;
    hunk_low.permanent = mark;

    Hunk_ClearTempMemory();
}

/* Hunk_Clear  0x00413750 */
void Hunk_Clear( void )
{
    Assert( Sys_IsMainThread() );

    hunk_low.permanent = 0;
    hunk_low.temp = 0;
    hunk_high.permanent = 0;
    hunk_high.temp = 0;

    Hunk_ClearTempMemory();
}

/* Hunk_MemoryUsed  0x004137b0 */
int Hunk_MemoryUsed( void )
{
    Assert( Sys_IsMainThread() || Sys_IsRenderThread() );

    return hunk_low.permanent + hunk_high.permanent;
}


/* Hunk_Alloc  0x00413800 */
void *Hunk_Alloc( int size )
{
    Assert( Sys_IsMainThread() );

    return Hunk_AllocAlign( size, HUNK_DEFAULT_ALIGNMENT );
}

/* Hunk_AllocAlign  0x00413840 */
void *Hunk_AllocAlign( int size, int alignment )
{
    void *buf;

    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    Assert( !(alignment & (alignment - 1)) );
    Assert( alignment <= HUNK_MAX_ALIGNEMT );

    alignment--;

    Hunk_CheckTempMemoryHigh();

    hunk_high.permanent += size;
    hunk_high.permanent = (hunk_high.permanent + alignment) & ~alignment;
    hunk_high.temp = hunk_high.permanent;

    if ( hunk_low.temp + hunk_high.temp > s_hunkTotal )
        Com_ErrorLevel( ERR_DROP, "Hunk_AllocAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                        size,
                        s_hunkTotal / (1024 * 1024),
                        hunk_low.temp / (1024 * 1024),
                        hunk_high.temp / (1024 * 1024) );

    buf = (void *)((s_hunkData + s_hunkTotal) - hunk_high.permanent);

    Assert( !(((psize_int)buf) & alignment) );

    memset( buf, 0, size );

    return buf;
}

/* Hunk_AllocateTempMemoryHigh  0x004139e0 */
void *Hunk_AllocateTempMemoryHigh( int size )
{
    void *buf;

    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    hunk_high.temp += size;
    hunk_high.temp = (hunk_high.temp + 15) & ~15;

    if ( hunk_low.temp + hunk_high.temp > s_hunkTotal )
        Com_ErrorLevel( ERR_DROP, "Hunk_AllocateTempMemoryHigh: failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                        size,
                        s_hunkTotal / (1024 * 1024),
                        hunk_low.temp / (1024 * 1024),
                        hunk_high.temp / (1024 * 1024) );

    buf = (void *)((s_hunkData + s_hunkTotal) - hunk_high.temp);

    Assert( !(((psize_int)buf) & 15) );

    return buf;
}

/* Hunk_ClearTempMemoryHigh  0x00413b00 */
void Hunk_ClearTempMemoryHigh( void )
{
    Assert( Sys_IsMainThread() );

    hunk_high.temp = hunk_high.permanent;
}

/* Hunk_AllocLow  0x00413b40 */
void *Hunk_AllocLow( int size )
{
    Assert( Sys_IsMainThread() );

    return Hunk_AllocLowAlign( size, HUNK_DEFAULT_ALIGNMENT );
}

/* Hunk_AllocLowAlign  0x00413b80 */
void *Hunk_AllocLowAlign( int size, int alignment )
{
    void *buf;

    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    Assert( !(alignment & (alignment - 1)) );
    Assert( alignment <= HUNK_MAX_ALIGNEMT );

    alignment--;

    Hunk_CheckTempMemoryLow();

    hunk_low.permanent = (hunk_low.permanent + alignment) & ~alignment;

    buf = (void *)(s_hunkData + hunk_low.permanent);

    Assert( !(((psize_int)buf) & alignment) );

    hunk_low.permanent += size;
    hunk_low.temp = hunk_low.permanent;

    if ( hunk_low.temp + hunk_high.temp > s_hunkTotal )
        Com_ErrorLevel( ERR_DROP, "Hunk_AllocLowAlign failed on %i bytes (total %i MB, low %i MB, high %i MB)",
                        size,
                        s_hunkTotal / (1024 * 1024),
                        hunk_low.temp / (1024 * 1024),
                        hunk_high.temp / (1024 * 1024) );

    memset( buf, 0, size );

    return buf;
}


/* Hunk_AllocateTempMemory  0x00413d20 */
void *Hunk_AllocateTempMemory( int size )
{
    hunkHeader_t *hdr;
    void *buf;
    int oldTemp;

    Assert( Sys_IsMainThread() );

    if ( !s_hunkData )
        return Z_Malloc( size );

    Assert( s_hunkData );

    size += sizeof( hunkHeader_t );

    oldTemp = hunk_low.temp;
    hunk_low.temp = (hunk_low.temp + 15) & ~15;

    buf = (void *)(s_hunkData + hunk_low.temp);

    hunk_low.temp += size;

    if ( hunk_low.temp + hunk_high.temp > s_hunkTotal )
        Com_ErrorLevel( ERR_DROP, "Hunk_AllocateTempMemory: failed on %i bytes (total %i MB, low %i MB, high %i MB), needs %i more hunk bytes",
                        size,
                        s_hunkTotal / (1024 * 1024),
                        hunk_low.temp / (1024 * 1024),
                        hunk_high.temp / (1024 * 1024),
                        hunk_low.temp + hunk_high.temp - s_hunkTotal );

    hdr = (hunkHeader_t *)buf;
    buf = hdr + 1;

    Assert( !(((psize_int)buf) & 15) );

    hdr->sentinel = HUNK_SENTINEL_ALLOC;
    hdr->size = hunk_low.temp - oldTemp;

    return buf;
}

/* Hunk_FreeTempMemory  0x00413ea0 */
void Hunk_FreeTempMemory( void *buf )
{
    hunkHeader_t *hdr;

    Assert( Sys_IsMainThread() );

    if ( !s_hunkData )
    {
        Z_Free( buf );
        return;
    }

    Assert( s_hunkData );
    Assert( buf );

    hdr = (hunkHeader_t *)buf - 1;

    if ( hdr->sentinel != HUNK_SENTINEL_ALLOC )
        Com_ErrorLevel( ERR_FATAL, "Hunk_FreeTempMemory: bad magic" );

    hdr->sentinel = HUNK_SENTINEL_FREE;

    Assert( hdr == (void *)( s_hunkData + ((hunk_low.temp - hdr->size + 15) & ~15) ) );

    hunk_low.temp -= hdr->size;
}

/* Hunk_ClearTempMemoryLow  0x00413fc0 */
void Hunk_ClearTempMemoryLow( void )
{
    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );

    hunk_low.temp = hunk_low.permanent;
}

/* Hunk_CheckTempMemoryLow  0x00414030 */
void Hunk_CheckTempMemoryLow( void )
{
    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );
    Assert( hunk_low.temp == hunk_low.permanent );
}

/* Hunk_CheckTempMemoryHigh  0x004140c0 */
void Hunk_CheckTempMemoryHigh( void )
{
    Assert( Sys_IsMainThread() );
    Assert( s_hunkData );
    Assert( hunk_high.temp == hunk_high.permanent );
}

/* Hunk_KeepTempMemoryLow  0x00414150 */
int Hunk_KeepTempMemoryLow( void )
{
    int mark;

    Assert( Sys_IsMainThread() );

    mark = hunk_low.permanent;
    hunk_low.permanent = hunk_low.temp;

    return mark;
}

/* Hunk_ReleaseTempMemoryLow  0x004141a0 */
void Hunk_ReleaseTempMemoryLow( int mark )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryLow();

    hunk_low.permanent = mark;
}

/* Hunk_KeepTempMemoryHigh  0x004141e0 */
int Hunk_KeepTempMemoryHigh( void )
{
    int mark;

    Assert( Sys_IsMainThread() );

    mark = hunk_high.permanent;
    hunk_high.permanent = hunk_high.temp;

    return mark;
}

/* Hunk_ReleaseTempMemoryHigh  0x00414230 */
void Hunk_ReleaseTempMemoryHigh( int mark )
{
    Assert( Sys_IsMainThread() );

    Hunk_CheckTempMemoryHigh();

    hunk_high.permanent = mark;
}


/* Hunk_InitDebugMemory  0x00414270 */
void Hunk_InitDebugMemory( void )
{
    Assert( Sys_IsMainThread() );
    Assert( !g_debugUser );

    g_debugUser = Hunk_UserCreate( HUNK_DEBUG_SIZE, "Hunk_InitDebugMemory", false, false, 0 );
}

/* Hunk_ShutdownDebugMemory  0x004142f0 */
void Hunk_ShutdownDebugMemory( void )
{
    Assert( Sys_IsMainThread() );
    Assert( g_debugUser );

    Hunk_UserDestroy( g_debugUser );
    g_debugUser = NULL;
}

/* Hunk_ResetDebugMemory  0x00414370 */
void Hunk_ResetDebugMemory( void )
{
    Assert( Sys_IsMainThread() );
    Assert( g_debugUser );

    Hunk_UserReset( g_debugUser );
}

/* Hunk_AllocDebugMemory  0x004143e0 */
void *Hunk_AllocDebugMemory( int size )
{
    Assert( Sys_IsMainThread() );
    Assert( g_debugUser );

    return Hunk_UserAlloc( g_debugUser, size, 4 );
}

/* Hunk_CheckDebugMemory  0x00414450 */
void Hunk_CheckDebugMemory( void )
{
    Assert( Sys_IsMainThread() );
    Assert( g_debugUser );
}


/* Hunk_UserCreate  0x004144b0 */
HunkUser *Hunk_UserCreate( int maxSize, const char *name, bool fixed,
                           bool debugMemory, int type )
{
    HunkUser *user;

    Assertx( (!(maxSize % (4*1024))), "(maxSize) = %i", maxSize );

    user = (HunkUser *)Z_VirtualReserve( maxSize );

    Z_VirtualCommit( user, offsetof( HunkUser, buf ) );

    user->end = (psize_int)user + maxSize;
    user->pos = (psize_int)user + offsetof( HunkUser, buf );

    Assertx( (!(user->pos & 31)), "(user->pos) = %i", user->pos );

    user->maxSize = maxSize;
    user->current = user;
    user->fixed = fixed;
    user->name = name;
    user->debugMemory = debugMemory;
    user->type = type;

    Assert( !user->next );

    return user;
}

/* Hunk_UserAlloc  0x004145c0 */
void *Hunk_UserAlloc( HunkUser *user, int size, int alignment )
{
    HunkUser *newUser;
    HunkUser *current;
    psize_int buf;
    psize_int pos;
    psize_int end;

    Assert( user );
    Assertx( static_cast< uint >( size ) <= user->maxSize - offsetof( HunkUser, buf ),
             "%s", va( "size: %d, maxSize: %d", size, user->maxSize - offsetof( HunkUser, buf ) ) );
    Assert( !(alignment & (alignment - 1)) );
    Assert( alignment <= HUNK_MAX_ALIGNEMT );

    alignment--;

    current = user->current;

    for ( ;; )
    {
        pos = current->pos;
        buf = (pos + alignment) & ~alignment;
        end = buf + size;

        if ( end <= current->end )
        {
            current->pos = end;
            break;
        }

        if ( user->fixed )
            Com_ErrorLevel( ERR_FATAL, "Hunk_UserAlloc: out of memory" );

        newUser = Hunk_UserCreate( user->maxSize, user->name, false, user->debugMemory, user->type );

        user->current = newUser;
        current->next = newUser;
        current = newUser;
    }

    pos = (pos + 0xfff) & ~0xfff;

    if ( pos != ((current->pos + 0xfff) & ~0xfff) )
    {
        Assert( current->pos - pos > 0 );

        Z_VirtualCommit( (void *)pos, current->pos - pos );
    }

    return (void *)buf;
}

/* Hunk_UserAlloc  0x004147b0 */
void *Hunk_UserAlloc( HunkUser *user, int size )
{
    return Hunk_UserAlloc( user, size, 1 );
}

/* Hunk_UserAllocUnaligned  0x004147d0 */
void *Hunk_UserAllocUnaligned( HunkUser *user, int size )
{
    return Hunk_UserAlloc( user, size, 1 );
}

/* Hunk_UserSetPos  0x004147f0 */
void Hunk_UserSetPos( HunkUser *user, byte *pos )
{
    Assert( user->fixed );
    Assert( pos >= user->buf );
    Assert( (psize_int)pos <= user->pos );

    user->pos = (psize_int)pos;
}

/* Hunk_UserReset  0x00414880 */
void Hunk_UserReset( HunkUser *user )
{
    psize_int pos;

    if ( user->next )
    {
        Hunk_UserDestroy( user->next );
        user->current = user;
        user->next = NULL;
    }

    pos = ((psize_int)user + offsetof( HunkUser, buf ) + 0xfff) & ~0xfff;

    if ( pos != ((user->pos + 0xfff) & ~0xfff) )
    {
        Assert( user->pos - pos > 0 );

        Z_VirtualDecommit( (void *)pos, user->pos - pos );
    }

    user->pos = (psize_int)user + offsetof( HunkUser, buf );

    memset( user->buf, 0, 4096 - offsetof( HunkUser, buf ) );
}

/* Hunk_UserDestroy  0x00414940 */
void Hunk_UserDestroy( HunkUser *user )
{
    HunkUser *current;
    HunkUser *next;

    for ( current = user->next; current; current = next )
    {
        next = current->next;
        Z_VirtualFree( current );
    }

    Z_VirtualFree( user );
}

/* Hunk_CopyString  0x00414990 */
char *Hunk_CopyString( HunkUser *user, const char *str )
{
    char *buf;

    buf = (char *)Hunk_UserAlloc( user, I_strlen( str ) + 1, 1 );
    strcpy( buf, str );

    return buf;
}

/* Hunk_UseLargeHunk  0x004149d0 */
bool Hunk_UseLargeHunk( void )
{
    return true;
}


#ifndef COD4MAP_COM_MATH_OWNS_COMDATS

#include "com_math.h"

#define ANGLE2RAD   ( (double)0.017453292f )

/* AnglesToAxis  0x00412690 */
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
    float angle;
    float sr, sp, sy, cr, cp, cy;

    angle = (float)( angles[1] * ANGLE2RAD );
    SinCos( angle, &sy, &cy );

    angle = (float)( angles[0] * ANGLE2RAD );
    SinCos( angle, &sp, &cp );

    axis[0][0] = cp * cy;
    axis[0][1] = cp * sy;
    axis[0][2] = -sp;

    angle = (float)( angles[2] * ANGLE2RAD );
    SinCos( angle, &sr, &cr );

    axis[1][0] = sr * sp * cy + -sy * cr;
    axis[1][1] = sr * sp * sy + cr * cy;
    axis[1][2] = sr * cp;
    axis[2][0] = cr * sp * cy + -sr * -sy;
    axis[2][1] = cr * sp * sy + -sr * cy;
    axis[2][2] = cr * cp;
}

/* Vec4Normalize  0x004127b0 */
float Vec4Normalize( vec4_t v )
{
    float length;
    float ilength;

    length = I_sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3] );

    if ( length != 0.0f )
    {
        ilength = 1.0f / length;

        v[0] = v[0] * ilength;
        v[1] = v[1] * ilength;
        v[2] = v[2] * ilength;
        v[3] = v[3] * ilength;
    }

    return length;
}

/* AnglesToQuat  0x00412860 */
void AnglesToQuat( const vec3_t angles, vec4_t quat )
{
    vec3_t axis[3];

    AnglesToAxis( angles, axis );
    AxisToQuat( axis, quat );
}

#endif

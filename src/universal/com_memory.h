/* Original: ..\src\universal\com_memory.cpp */

#ifndef COM_MEMORY_H
#define COM_MEMORY_H

#include "q_shared.h"

#ifndef PSIZE_INT_DEFINED
#define PSIZE_INT_DEFINED
#ifdef _WIN64
typedef __int64      psize_int;     /* holds a pointer, as in the 32-bit build */
#else
typedef int          psize_int;
#endif
typedef unsigned int uint;
#endif


#define HUNK_MAX_ALIGNEMT       4096

#define HUNK_DEFAULT_ALIGNMENT  32

#define FILE_DATA_HASH_SIZE     1024

#define HUNK_SIZE_LARGE         0x0C000000

#define HUNK_DEBUG_SIZE         0x01000000

#define HUNK_SENTINEL_ALLOC     ((int)0x89537892)
#define HUNK_SENTINEL_FREE      ((int)0x89537893)

typedef struct
{
    int permanent;      /* +0x00 */
    int temp;           /* +0x04 */
} hunkUsed_t;

typedef struct
{
    int sentinel;       /* +0x00 */
    int size;           /* +0x04 */
    int pad[2];         /* +0x08 */
} hunkHeader_t;

typedef struct hunkFileEntry_s
{
    void                   *data;   /* +0x00 */
    struct hunkFileEntry_s *next;   /* +0x04 */
    byte                    type;   /* +0x08 */
    char                    name[1];/* +0x09 */
} hunkFileEntry_t;

typedef struct
{
    int    assetCount;  /* +0x00 */
    int    maxCount;    /* +0x04 */
    void **assets;      /* +0x08 */
} assetList_t;

typedef struct HunkUser_s
{
    struct HunkUser_s *current;     /* +0x00 */
    struct HunkUser_s *next;        /* +0x04 */
    unsigned int       maxSize;     /* +0x08 */
    psize_int          end;         /* +0x0c */
    psize_int          pos;         /* +0x10 */
    const char        *name;        /* +0x14 */
    bool               fixed;       /* +0x18 */
    bool               debugMemory; /* +0x19 */
    int                type;        /* +0x1c */
#ifdef _WIN64
    /* The header is bigger with 8 byte pointers; keep the data 32 byte aligned as in the 32-bit build */
    __declspec( align( 32 ) ) byte buf[1];
#else
    byte               buf[1];      /* +0x20 */
#endif
} HunkUser;

void  *Z_VirtualReserve( int size );
bool   Z_TryVirtualCommit( void *buf, int size );
void   Z_VirtualDecommit( void *buf, int size );
void   Z_VirtualFree( void *buf );
void   Z_VirtualCommit( void *buf, int size );
void  *Z_TryVirtualAlloc( int size );
void  *Z_VirtualAlloc( int size );

void   Z_Free( void *ptr );
void  *Z_TryMallocInternal( int size );
void  *Z_TryMalloc( int size );
void   Z_MallocFailed( int size );
void  *Z_Malloc( int size );
void  *Z_MallocGarbage( int size );

char  *CopyString( const char *in );
void   SetString( char **str, const char *in );
void   FreeString( char *str );

void   Hunk_InitMemory( void );
void   Hunk_Shutdown( void );
void   Hunk_Clear( void );
qboolean Hunk_IsInHunk( const void *ptr );
int    Hunk_MemoryUsed( void );

typedef void *(*HunkAllocFunc_t)( int size );

void  *Hunk_FindDataForFileInternal( int type, const char *name, int hash );
void  *Hunk_FindDataForFile( int type, const char *name );
char  *Hunk_AddDataForFile( int type, const char *name, void *data, HunkAllocFunc_t allocFunc );
void   Hunk_AddData( int type, void *data, HunkAllocFunc_t allocFunc );
void   Hunk_OverrideDataForFile( int type, const char *name, void *data );
void   Hunk_PurgeFreeListRange( hunkFileEntry_t **listHead, size_t lowAddr, size_t highAddr );
void   Hunk_ClearTempMemory( void );

typedef void (*HunkAssetFunc_t)( void *data, void *userData );

void   Hunk_EnumerateFileData( hunkFileEntry_t *fileData, int type,
                               HunkAssetFunc_t callback, void *userData );
void   Hunk_EnumerateAssetsInternal( int assetType, HunkAssetFunc_t callback,
                                     void *userData, bool includeTemp );
void   Hunk_EnumerateAssets( int assetType, HunkAssetFunc_t callback,
                             void *userData, bool includeTemp );
void   Hunk_AddAssetToList( void *data, assetList_t *assetList );
int    Hunk_GetAssetListInternal( int assetType, void **assets, int maxCount );
int    Hunk_GetAssetList( int assetType, void **assets, int maxCount );

int    Hunk_HighMark( void );
void   Hunk_ClearHighToMark( int mark );
int    Hunk_LowMark( void );
void   Hunk_ClearLowToMark( int mark );

void  *Hunk_Alloc( int size );
void  *Hunk_AllocAlign( int size, int alignment );
void  *Hunk_AllocLow( int size );
void  *Hunk_AllocLowAlign( int size, int alignment );

void  *Hunk_AllocateTempMemoryHigh( int size );
void  *Hunk_AllocateTempMemory( int size );
void   Hunk_FreeTempMemory( void *buf );
void   Hunk_ClearTempMemoryHigh( void );
void   Hunk_ClearTempMemoryLow( void );
void   Hunk_CheckTempMemoryLow( void );
void   Hunk_CheckTempMemoryHigh( void );
int    Hunk_KeepTempMemoryLow( void );
void   Hunk_ReleaseTempMemoryLow( int mark );
int    Hunk_KeepTempMemoryHigh( void );
void   Hunk_ReleaseTempMemoryHigh( int mark );

void   Hunk_InitDebugMemory( void );
void   Hunk_ShutdownDebugMemory( void );
void   Hunk_ResetDebugMemory( void );
void  *Hunk_AllocDebugMemory( int size );
void   Hunk_CheckDebugMemory( void );

HunkUser *Hunk_UserCreate( int maxSize, const char *name, bool fixed,
                           bool debugMemory, int type );
void  *Hunk_UserAlloc( HunkUser *user, int size, int alignment );
void  *Hunk_UserAlloc( HunkUser *user, int size );
void  *Hunk_UserAllocUnaligned( HunkUser *user, int size );
void   Hunk_UserSetPos( HunkUser *user, byte *pos );
void   Hunk_UserReset( HunkUser *user );
void   Hunk_UserDestroy( HunkUser *user );
char  *Hunk_CopyString( HunkUser *user, const char *str );

bool   Hunk_UseLargeHunk( void );

#endif

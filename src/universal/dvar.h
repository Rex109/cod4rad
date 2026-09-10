/* Original: ..\src\universal\dvar.cpp */

#ifndef DVAR_H
#define DVAR_H

#include "q_shared.h"

#define MAX_DVARS                4096      /* 0x1000 */
#define DVAR_HASH_SIZE           256

#define DVAR_HASH_POSITION_SEED  119

#define DVAR_INVALID_ENUM_INDEX  (-1337)

enum dvarType_e
{
    DVAR_TYPE_BOOL    = 0,
    DVAR_TYPE_FLOAT   = 1,
    DVAR_TYPE_FLOAT_2 = 2,
    DVAR_TYPE_FLOAT_3 = 3,
    DVAR_TYPE_FLOAT_4 = 4,
    DVAR_TYPE_INT     = 5,
    DVAR_TYPE_ENUM    = 6,
    DVAR_TYPE_STRING  = 7,
    DVAR_TYPE_COLOR   = 8,

    DVAR_TYPE_COUNT   = 9
};

typedef unsigned char dvarType_t;

#define DVAR_ARCHIVE            (1 << 0)    /* 0x0001 */
#define DVAR_USERINFO           (1 << 1)    /* 0x0002 */
#define DVAR_SERVERINFO         (1 << 2)    /* 0x0004 */
#define DVAR_SYSTEMINFO         (1 << 3)    /* 0x0008 */
#define DVAR_INIT               (1 << 4)    /* 0x0010 */
#define DVAR_LATCH              (1 << 5)    /* 0x0020 */
#define DVAR_ROM                (1 << 6)    /* 0x0040 */
#define DVAR_CHEAT              (1 << 7)    /* 0x0080 */
#define DVAR_TEMP               (1 << 8)    /* 0x0100 */
#define DVAR_CHANGEABLE_RESET   (1 << 9)    /* 0x0200 */
#define DVAR_NORESTART          (1 << 10)   /* 0x0400 */
#define DVAR_DEVGUILATCH        (1 << 11)   /* 0x0800 */
#define DVAR_SAVED              (1 << 12)   /* 0x1000 */
#define DVAR_RESERVED13         (1 << 13)   /* 0x2000 */
#define DVAR_EXTERNAL           (1 << 14)   /* 0x4000 */
#define DVAR_AUTOEXEC           (1 << 15)   /* 0x8000 */

typedef enum DvarSetSource_e
{
    DVAR_SOURCE_INTERNAL = 0,
    DVAR_SOURCE_EXTERNAL = 1,
    DVAR_SOURCE_SCRIPT   = 2,
    DVAR_SOURCE_DEVGUI   = 3
} DvarSetSource_t;

typedef union DvarValue_u
{
    bool           enabled;
    int            integer;
    unsigned int   unsignedInt;
    float          value;
    float          vector[4];
    const char    *string;
    byte           color[4];
} DvarValue_t;

typedef union DvarLimits_u
{
    struct
    {
        int          stringCount;    /* +0x00 */
        const char **strings;        /* +0x04 */
    } enumeration;

    struct
    {
        int          min;            /* +0x00 */
        int          max;            /* +0x04 */
    } integer;

    struct
    {
        float        min;            /* +0x00 */
        float        max;            /* +0x04 */
    } value;

    struct
    {
        float        min;            /* +0x00 */
        float        max;            /* +0x04 */
    } vector;
} DvarLimits_t;

typedef struct Dvar_s
{
    const char     *name;        /* +0x00 */
    const char     *description; /* +0x04 */
    unsigned short  flags;       /* +0x08 */
    dvarType_t      type;        /* +0x0a */
    bool            modified;    /* +0x0b */
    DvarValue_t     current;     /* +0x0c */
    DvarValue_t     latched;     /* +0x1c */
    DvarValue_t     reset;       /* +0x2c */
    DvarLimits_t    domain;      /* +0x3c */
    bool          (*domainFunc)( struct Dvar_s *dvar, DvarValue_t value );  /* +0x44 */
    struct Dvar_s  *hashNext;    /* +0x48 */
} Dvar_t;

typedef bool (*DvarDomainFunc_t)( Dvar_t *dvar, DvarValue_t value );

typedef void (*DvarForEachFunc_t)( Dvar_t *dvar, void *userData );
typedef void (*DvarForEachNameFunc_t)( const char *dvarName );

extern int     dvar_modifiedFlags;      /* 0x2b9256c0 */
extern Dvar_t *dvar_cheats;             /* 0x2b9296d0 */

void     Dvar_Init( void );
void     Dvar_Shutdown( void );
bool     Dvar_IsSystemActive( void );
void     Dvar_SetLoadingAutoExec( bool value );

int      Dvar_GenerateHashValue( const char *name );
Dvar_t  *Dvar_FindVar( const char *dvarName );
Dvar_t  *Dvar_FindMalleableVar( const char *dvarName );
Dvar_t  *Dvar_GetDvarByIndex( unsigned int index );
bool     Dvar_IsValidName( const char *dvarName );
void     Dvar_ForEach( DvarForEachFunc_t callback, void *userData );
void     Dvar_ForEachName( DvarForEachNameFunc_t callback );

const char *Dvar_EnumToString( const Dvar_t *dvar );
const char *Dvar_IndexStringToEnumString( const Dvar_t *dvar, const char *indexString );
char       *Dvar_ValueToString( const Dvar_t *dvar, DvarValue_t value );
char       *Dvar_DisplayableValue( const Dvar_t *dvar );
char       *Dvar_DisplayableResetValue( const Dvar_t *dvar );
char       *Dvar_DisplayableLatchedValue( const Dvar_t *dvar );

bool        Dvar_StringToBool( const char *string );
int         Dvar_StringToInt( const char *string );
float       Dvar_StringToFloat( const char *string );
void        Dvar_StringToVec2( const char *string, vec2_t value );
void        Dvar_StringToVec3( const char *string, vec3_t value );
void        Dvar_StringToVec4( const char *string, vec4_t value );
int         Dvar_StringToEnum( const DvarLimits_t *domain, const char *string );
void        Dvar_StringToColor( const char *string, byte color[4] );
DvarValue_t Dvar_StringToValue( dvarType_t type, DvarLimits_t domain, const char *string );

bool        Dvar_ValuesEqual( dvarType_t type, DvarValue_t val0, DvarValue_t val1 );
bool        Dvar_ValueInDomain( dvarType_t type, DvarValue_t value, DvarLimits_t domain );
DvarValue_t Dvar_ClampValueToDomain( dvarType_t type, DvarValue_t value,
                                     DvarValue_t resetValue, DvarLimits_t domain );
char       *Dvar_DomainToString( dvarType_t type, DvarLimits_t domain,
                                 char *outBuffer, int outBufferLen );
char       *Dvar_DomainToStringWithLineCount( dvarType_t type, DvarLimits_t domain,
                                              char *outBuffer, int outBufferLen,
                                              int *outLineCount );
void        Dvar_PrintDomain( dvarType_t type, DvarLimits_t domain );
void        Dvar_SetEnumDomain( Dvar_t *dvar, const char **stringTable );
void        Dvar_SetDomainFunc( Dvar_t *dvar, DvarDomainFunc_t domainFunc );

bool        Dvar_GetBool( const char *dvarName );
int         Dvar_GetInt( const char *dvarName );
float       Dvar_GetFloat( const char *dvarName );
void        Dvar_GetVec3( const char *dvarName, vec3_t value );
const char *Dvar_GetString( const char *dvarName );
const char *Dvar_GetVariantString( const char *dvarName );
void        Dvar_GetUnpackedColor( const Dvar_t *dvar, vec4_t expandedColor );
void        Dvar_GetUnpackedColorByName( const char *dvarName, vec4_t expandedColor );

Dvar_t *Dvar_RegisterVariant( const char *dvarName, dvarType_t type, unsigned short flags,
                              DvarValue_t value, DvarLimits_t domain, const char *description );
Dvar_t *Dvar_RegisterBool  ( const char *dvarName, bool value,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterInt   ( const char *dvarName, int value, int min, int max,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterFloat ( const char *dvarName, float value, float min, float max,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterVec2  ( const char *dvarName, float x, float y, float min, float max,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterVec3  ( const char *dvarName, float x, float y, float z,
                             float min, float max,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterVec4  ( const char *dvarName, float x, float y, float z, float w,
                             float min, float max,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterString( const char *dvarName, const char *value,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterEnum  ( const char *dvarName, const char **valueList, int defaultIndex,
                             unsigned short flags, const char *description );
Dvar_t *Dvar_RegisterColor ( const char *dvarName, float r, float g, float b, float a,
                             unsigned short flags, const char *description );

void Dvar_SetVariant  ( Dvar_t *dvar, DvarValue_t value, DvarSetSource_t source );

void Dvar_SetBoolFromSource  ( Dvar_t *dvar, bool value, DvarSetSource_t source );
void Dvar_SetIntFromSource   ( Dvar_t *dvar, int value, DvarSetSource_t source );
void Dvar_SetFloatFromSource ( Dvar_t *dvar, float value, DvarSetSource_t source );
void Dvar_SetVec2FromSource  ( Dvar_t *dvar, float x, float y, DvarSetSource_t source );
void Dvar_SetVec3FromSource  ( Dvar_t *dvar, float x, float y, float z, DvarSetSource_t source );
void Dvar_SetVec4FromSource  ( Dvar_t *dvar, float x, float y, float z, float w,
                               DvarSetSource_t source );
void Dvar_SetStringFromSource( Dvar_t *dvar, const char *string, DvarSetSource_t source );
void Dvar_SetColorFromSource ( Dvar_t *dvar, float r, float g, float b, float a,
                               DvarSetSource_t source );
void Dvar_SetFromStringFromSource( Dvar_t *dvar, const char *string, DvarSetSource_t source );

void Dvar_SetBool  ( Dvar_t *dvar, bool value );
void Dvar_SetInt   ( Dvar_t *dvar, int value );
void Dvar_SetFloat ( Dvar_t *dvar, float value );
void Dvar_SetVec2  ( Dvar_t *dvar, float x, float y );
void Dvar_SetVec3  ( Dvar_t *dvar, float x, float y, float z );
void Dvar_SetVec4  ( Dvar_t *dvar, float x, float y, float z, float w );
void Dvar_SetString( Dvar_t *dvar, const char *string );
void Dvar_SetColor ( Dvar_t *dvar, float r, float g, float b, float a );
void Dvar_SetFromString( Dvar_t *dvar, const char *string );

void Dvar_SetBoolByName  ( const char *dvarName, bool value );
void Dvar_SetIntByName   ( const char *dvarName, int value );
void Dvar_SetFloatByName ( const char *dvarName, float value );
void Dvar_SetVec2ByName  ( const char *dvarName, float x, float y );
void Dvar_SetVec3ByName  ( const char *dvarName, float x, float y, float z );
void Dvar_SetVec4ByName  ( const char *dvarName, float x, float y, float z, float w );
void Dvar_SetStringByName( const char *dvarName, const char *string );
void Dvar_SetColorByName ( const char *dvarName, float r, float g, float b, float a );
Dvar_t *Dvar_SetFromStringByNameFromSource( const char *dvarName, const char *string,
                                            DvarSetSource_t source );
void Dvar_SetFromStringByName( const char *dvarName, const char *string );
void Dvar_SetFromStringByNameExternal( const char *dvarName, const char *string );

void Dvar_ClearModified( Dvar_t *dvar );
void Dvar_SetModified( Dvar_t *dvar );
void Dvar_SetLatchedValue( Dvar_t *dvar, DvarValue_t value );
void Dvar_ApplyLatchedValue( Dvar_t *dvar );
void Dvar_ClearLatchedValue( Dvar_t *dvar );
bool Dvar_HasLatchedValue( const Dvar_t *dvar );
bool Dvar_AnyLatchedValues( void );

void Dvar_SetResetVariant( Dvar_t *dvar, DvarValue_t value );
void Dvar_Reset( Dvar_t *dvar, DvarSetSource_t source );
void Dvar_ResetFlaggedDvars( unsigned short flags, DvarSetSource_t source );
void Dvar_ResetCheatDvars( void );
void Dvar_AddFlags( Dvar_t *dvar, unsigned int flags );

int  Com_SaveDvarsToBuffer( const char **dvarNames, int dvarCount, char *buffer, int bufferSize );
int  Com_LoadDvarsFromBuffer( const char *buffer, int bufferSize );

#endif

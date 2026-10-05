/* Original: ..\src\universal\dvar.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_shared.h"
#include "com_memory.h"
#include "com_math.h"
#include "com_vector.h"
#include "dvar.h"

#include <algorithm>
#include "sort_vc8.h"

#ifndef BYTE4_HELPERS_DEFINED
#define BYTE4_HELPERS_DEFINED

inline void Byte4Copy( const byte from[4], byte to[4] )
{
    *(unsigned int *)to = *(const unsigned int *)from;
}

/* 0x0047c2c0 */
inline bool Byte4Compare( const byte c0[4], const byte c1[4] )
{
    return *(const unsigned int *)c0 == *(const unsigned int *)c1;
}

#endif

/* Vec4Clear  0x0047c2e0 */
#ifndef VEC4CLEAR_DEFINED
#define VEC4CLEAR_DEFINED
inline void Vec4Clear( vec4_t v )
{
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
}
#endif


int      dvar_modifiedFlags;

/* 0x2b9256c4 */
static int      s_dvarCount;


static bool     s_isLoadingAutoExec;

/* 0x2b9256cd */
static bool     s_isDvarSystemActive;

static Dvar_t  *s_sortedDvars[MAX_DVARS];

Dvar_t  *dvar_cheats;


/* 0x2b9296d8 */
static Dvar_t  *s_dvarHashTable[DVAR_HASH_SIZE];

static Dvar_t   s_dvarPool[MAX_DVARS];

static bool     s_areDvarsSorted;


/* Dvar_IsSystemActive  0x00476770 */
bool Dvar_IsSystemActive( void )
{
    return s_isDvarSystemActive;
}

/* Dvar_SetLoadingAutoExec  0x00476760 */
void Dvar_SetLoadingAutoExec( bool value )
{
    s_isLoadingAutoExec = value;
}

/* Dvar_IsValidName  0x00476780 */
bool Dvar_IsValidName( const char *dvarName )
{
    int  i;
    char c;

    if ( !dvarName )
        return false;

    for ( i = 0; dvarName[i] != '\0'; i++ )
    {
        c = dvarName[i];
        if ( !isalnum( c ) && c != '_' )
            return false;
    }

    return true;
}

/* Dvar_GetDvarByIndex  0x00476720 */
Dvar_t *Dvar_GetDvarByIndex( unsigned int index )
{
    AssertIn( index, s_dvarCount );
    return &s_dvarPool[index];
}

/* Dvar_SortCompare  0x004766a0 */
static bool Dvar_SortCompare( const Dvar_t *dvar0, const Dvar_t *dvar1 )
{
    return Q_stricmp( dvar0->name, dvar1->name ) < 0;
}

/* Dvar_SortDvars  0x00476670 */
static void Dvar_SortDvars( void )
{
    Sort_VC8( s_sortedDvars, s_sortedDvars + s_dvarCount, Dvar_SortCompare );
    s_areDvarsSorted = true;
}

/* Dvar_ForEach  0x00476620 */
void Dvar_ForEach( DvarForEachFunc_t callback, void *userData )
{
    int i;

    if ( !s_areDvarsSorted )
        Dvar_SortDvars();

    for ( i = 0; i < s_dvarCount; i++ )
        callback( s_sortedDvars[i], userData );
}

/* Dvar_ForEachName  0x004766d0 */
void Dvar_ForEachName( DvarForEachNameFunc_t callback )
{
    int i;

    if ( !s_areDvarsSorted )
        Dvar_SortDvars();

    for ( i = 0; i < s_dvarCount; i++ )
        callback( s_sortedDvars[i]->name );
}

/* Dvar_GenerateHashValue  0x004777b0 */
int Dvar_GenerateHashValue( const char *fname )
{
    int i;
    int letter;
    int hash;

    if ( !fname )
        Com_ErrorLevel( ERR_DROP, "null name in generateHashValue" );

    hash = 0;
    for ( i = 0; fname[i] != '\0'; i++ )
    {
        letter = tolower( fname[i] );
        hash += ( i + DVAR_HASH_POSITION_SEED ) * letter;
    }

    hash &= ( DVAR_HASH_SIZE - 1 );
    return hash;
}

/* Dvar_FindVar  0x00477750 */
Dvar_t *Dvar_FindVar( const char *dvarName )
{
    int     hash;
    Dvar_t *dvar;

    hash = Dvar_GenerateHashValue( dvarName );

    for ( dvar = s_dvarHashTable[hash]; dvar; dvar = dvar->hashNext )
    {
        if ( !Q_stricmp( dvarName, dvar->name ) )
            return dvar;
    }

    return NULL;
}

/* Dvar_FindMalleableVar  0x00477730 */
Dvar_t *Dvar_FindMalleableVar( const char *dvarName )
{
    return Dvar_FindVar( dvarName );
}

/* Dvar_CopyName  0x0047a2e0 */
static const char *Dvar_CopyName( const char *name )
{
    return CopyString( name );
}

static void Dvar_FreeName( const char *name )
{
    FreeString( (char *)name );
}

/* Dvar_CopyStringValue  0x004788b0 */
static void Dvar_CopyStringValue( const char *string, DvarValue_t *outValue )
{
    Assert( string );
    outValue->string = CopyString( string );
}

static void Dvar_MakeStringValue( const char *string, DvarValue_t *outValue )
{
    Assert( string );
    outValue->string = string;
}

/* Dvar_ShouldFreeCurrentString  0x00478510 */
static bool Dvar_ShouldFreeCurrentString( const Dvar_t *dvar )
{
    if ( !dvar->current.string
      || dvar->current.string == dvar->latched.string
      || dvar->current.string == dvar->reset.string )
        return false;

    return true;
}

static bool Dvar_ShouldFreeLatchedString( const Dvar_t *dvar )
{
    if ( !dvar->latched.string
      || dvar->latched.string == dvar->current.string
      || dvar->latched.string == dvar->reset.string )
        return false;

    return true;
}

static bool Dvar_ShouldFreeResetString( const Dvar_t *dvar )
{
    if ( !dvar->reset.string
      || dvar->reset.string == dvar->current.string
      || dvar->reset.string == dvar->latched.string )
        return false;

    return true;
}

/* Dvar_FreeString  0x004785d0 */
static void Dvar_FreeString( const char **string )
{
    FreeString( (char *)*string );
    *string = NULL;
}

/* Dvar_MakeCurrentStringValue  0x00478fc0 */
static void Dvar_MakeCurrentStringValue( const Dvar_t *dvar, DvarValue_t *outValue, const char *string )
{
    Assert( string );

    if ( dvar->latched.string
      && ( string == dvar->latched.string || !strcmp( string, dvar->latched.string ) ) )
        Dvar_MakeStringValue( dvar->latched.string, outValue );
    else if ( dvar->reset.string
      && ( string == dvar->reset.string || !strcmp( string, dvar->reset.string ) ) )
        Dvar_MakeStringValue( dvar->reset.string, outValue );
    else
        Dvar_CopyStringValue( string, outValue );
}

static void Dvar_MakeLatchedStringValue( const Dvar_t *dvar, DvarValue_t *outValue, const char *string )
{
    Assert( string );

    if ( dvar->current.string
      && ( string == dvar->current.string || !strcmp( string, dvar->current.string ) ) )
        Dvar_MakeStringValue( dvar->current.string, outValue );
    else if ( dvar->reset.string
      && ( string == dvar->reset.string || !strcmp( string, dvar->reset.string ) ) )
        Dvar_MakeStringValue( dvar->reset.string, outValue );
    else
        Dvar_CopyStringValue( string, outValue );
}

static void Dvar_MakeResetStringValue( const Dvar_t *dvar, DvarValue_t *outValue, const char *string )
{
    Assert( string );

    if ( dvar->current.string
      && ( string == dvar->current.string || !strcmp( string, dvar->current.string ) ) )
        Dvar_MakeStringValue( dvar->current.string, outValue );
    else if ( dvar->latched.string
      && ( string == dvar->latched.string || !strcmp( string, dvar->latched.string ) ) )
        Dvar_MakeStringValue( dvar->latched.string, outValue );
    else
        Dvar_CopyStringValue( string, outValue );
}

/* Dvar_EnumToString  0x004767e0 */
const char *Dvar_EnumToString( const Dvar_t *dvar )
{
    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_ENUM, "(dvar->name) = %s", dvar->name );
    Assertx( dvar->domain.enumeration.strings, "(dvar->name) = %s", dvar->name );
    Assertx( dvar->current.integer >= 0 && dvar->current.integer < dvar->domain.enumeration.stringCount
             || dvar->current.integer == 0,
             "(dvar->current.integer) = %i", dvar->current.integer );

    if ( !dvar->domain.enumeration.stringCount )
        return "";

    return dvar->domain.enumeration.strings[dvar->current.integer];
}

/* Dvar_IndexStringToEnumString  0x00476900 */
const char *Dvar_IndexStringToEnumString( const Dvar_t *dvar, const char *indexString )
{
    int i;
    int indexStringLen;
    int index;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_ENUM, "(dvar->name) = %s", dvar->name );
    Assertx( dvar->domain.enumeration.strings, "(dvar->name) = %s", dvar->name );
    Assertx( indexString, "(dvar->name) = %s", dvar->name );

    if ( !dvar->domain.enumeration.stringCount )
        return "";

    indexStringLen = I_strlen( indexString );
    for ( i = 0; i < indexStringLen; i++ )
    {
        if ( !isdigit( indexString[i] ) )
            return "";
    }

    index = atoi( indexString );
    if ( index < 0 || index >= dvar->domain.enumeration.stringCount )
        return "";

    return dvar->domain.enumeration.strings[index];
}

/* Dvar_ValueToString  0x00476ad0 */
char *Dvar_ValueToString( const Dvar_t *dvar, DvarValue_t value )
{
    char *string;

    switch ( dvar->type )
    {
    case DVAR_TYPE_BOOL:
        if ( value.enabled )
            string = "1";
        else
            string = "0";
        break;

    case DVAR_TYPE_INT:
        string = va( "%i", value.integer );
        break;

    case DVAR_TYPE_FLOAT:
        string = va( "%g", value.value );
        break;

    case DVAR_TYPE_FLOAT_2:
        string = va( "%g %g", value.vector[0], value.vector[1] );
        break;

    case DVAR_TYPE_FLOAT_3:
        string = va( "%g %g %g", value.vector[0], value.vector[1], value.vector[2] );
        break;

    case DVAR_TYPE_FLOAT_4:
        string = va( "%g %g %g %g",
                     value.vector[0], value.vector[1], value.vector[2], value.vector[3] );
        break;

    case DVAR_TYPE_ENUM:
        Assertx( value.integer >= 0 && value.integer < dvar->domain.enumeration.stringCount
                 || value.integer == 0,
                 "(value.integer) = %i", value.integer );
        if ( !dvar->domain.enumeration.stringCount )
            string = "";
        else
            string = (char *)dvar->domain.enumeration.strings[value.integer];
        break;

    case DVAR_TYPE_STRING:
        Assertx( value.string, "(dvar->name) = %s", dvar->name );
        string = va( "%s", value.string );
        break;

    case DVAR_TYPE_COLOR:
        string = va( "%g %g %g %g",
                     value.color[0] * ( 1.0f / 255.0f ),
                     value.color[1] * ( 1.0f / 255.0f ),
                     value.color[2] * ( 1.0f / 255.0f ),
                     value.color[3] * ( 1.0f / 255.0f ) );
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", dvar->type ) );
        string = "";
        break;
    }

    return string;
}

/* Dvar_StringToBool  0x00477db0 */
bool Dvar_StringToBool( const char *string )
{
    Assert( string );
    return atoi( string ) != 0;
}

int Dvar_StringToInt( const char *string )
{
    Assert( string );
    return atoi( string );
}

float Dvar_StringToFloat( const char *string )
{
    Assert( string );
    return (float)atof( string );
}

/* Dvar_StringToVec2  0x00479bd0 */
void Dvar_StringToVec2( const char *string, vec2_t value )
{
    Assert( string );

    Vec2Clear( value );
    sscanf( string, "%g %g", &value[0], &value[1] );
}

void Dvar_StringToVec3( const char *string, vec3_t value )
{
    Assert( string );

    Vec3Clear( value );
    if ( *string == '(' )
        sscanf( string, "( %g, %g, %g )", &value[0], &value[1], &value[2] );
    else
        sscanf( string, "%g %g %g", &value[0], &value[1], &value[2] );
}

void Dvar_StringToVec4( const char *string, vec4_t value )
{
    Assert( string );

    Vec4Clear( value );
    sscanf( string, "%g %g %g %g", &value[0], &value[1], &value[2], &value[3] );
}

/* Dvar_StringToEnum  0x00479d30 */
int Dvar_StringToEnum( const DvarLimits_t *domain, const char *string )
{
    int         index;
    int         stringLen;
    const char *s;

    Assert( domain );
    Assert( string );

    for ( index = 0; index < domain->enumeration.stringCount; index++ )
    {
        if ( !Q_stricmp( string, domain->enumeration.strings[index] ) )
            return index;
    }

    index = 0;
    for ( s = string; *s != '\0'; s++ )
    {
        if ( *s < '0' || *s > '9' )
            return DVAR_INVALID_ENUM_INDEX;

        index = index * 10 + ( *s - '0' );
    }

    if ( index >= 0 && index < domain->enumeration.stringCount )
        return index;

    stringLen = I_strlen( string );
    for ( index = 0; index < domain->enumeration.stringCount; index++ )
    {
        if ( !Q_stricmpn( string, domain->enumeration.strings[index], stringLen ) )
            return index;
    }

    return DVAR_INVALID_ENUM_INDEX;
}

/* Dvar_StringToColor  0x00478230 */
void Dvar_StringToColor( const char *string, byte color[4] )
{
    vec4_t value;

    Vec4Clear( value );
    sscanf( string, "%g %g %g %g", &value[0], &value[1], &value[2], &value[3] );

    color[0] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, value[0] ) ) * 255.0 );
    color[1] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, value[1] ) ) * 255.0 );
    color[2] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, value[2] ) ) * 255.0 );
    color[3] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, value[3] ) ) * 255.0 );
}

/* Dvar_StringToValue  0x00479a50 */
DvarValue_t Dvar_StringToValue( dvarType_t type, DvarLimits_t domain, const char *string )
{
    DvarValue_t value;

    Assert( string );

    switch ( type )
    {
    case DVAR_TYPE_BOOL:
        value.enabled = Dvar_StringToBool( string );
        break;

    case DVAR_TYPE_FLOAT:
        value.value = Dvar_StringToFloat( string );
        break;

    case DVAR_TYPE_FLOAT_2:
        Dvar_StringToVec2( string, value.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        Dvar_StringToVec3( string, value.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        Dvar_StringToVec4( string, value.vector );
        break;

    case DVAR_TYPE_INT:
        value.integer = Dvar_StringToInt( string );
        break;

    case DVAR_TYPE_ENUM:
        value.integer = Dvar_StringToEnum( &domain, string );
        break;

    case DVAR_TYPE_STRING:
        value.string = string;
        break;

    case DVAR_TYPE_COLOR:
        Dvar_StringToColor( string, value.color );
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", type ) );
        value.integer = 0;
        break;
    }

    return value;
}

/* Dvar_DisplayableValue  0x00476a70 */
char *Dvar_DisplayableValue( const Dvar_t *dvar )
{
    Assert( dvar );
    return Dvar_ValueToString( dvar, dvar->current );
}

char *Dvar_DisplayableResetValue( const Dvar_t *dvar )
{
    Assert( dvar );
    return Dvar_ValueToString( dvar, dvar->reset );
}

char *Dvar_DisplayableLatchedValue( const Dvar_t *dvar )
{
    Assert( dvar );
    return Dvar_ValueToString( dvar, dvar->latched );
}

/* Dvar_ClampVectorToDomain  0x00477cb0 */
static void Dvar_ClampVectorToDomain( float *value, int components, float min, float max )
{
    int i;

    for ( i = 0; i < components; i++ )
    {
        if ( value[i] < min )
            value[i] = min;
        else if ( value[i] > max )
            value[i] = max;
    }
}

/* Dvar_ClampValueToDomain  0x00477ad0 */
DvarValue_t Dvar_ClampValueToDomain( dvarType_t type, DvarValue_t value,
                                     DvarValue_t resetValue, DvarLimits_t domain )
{
    switch ( type )
    {
    case DVAR_TYPE_BOOL:
        value.enabled = ( value.enabled != 0 );
        break;

    case DVAR_TYPE_FLOAT:
        if ( value.value < domain.value.min )
            value.value = domain.value.min;
        else if ( value.value > domain.value.max )
            value.value = domain.value.max;
        break;

    case DVAR_TYPE_FLOAT_2:
        Dvar_ClampVectorToDomain( value.vector, 2, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_FLOAT_3:
        Dvar_ClampVectorToDomain( value.vector, 3, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_FLOAT_4:
        Dvar_ClampVectorToDomain( value.vector, 4, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_INT:
        Assert( domain.integer.min <= domain.integer.max );
        if ( value.integer < domain.integer.min )
            value.integer = domain.integer.min;
        else if ( value.integer > domain.integer.max )
            value.integer = domain.integer.max;
        break;

    case DVAR_TYPE_ENUM:
        if ( value.integer < 0 || value.integer >= domain.enumeration.stringCount )
        {
            value = resetValue;
            Assertx( value.integer >= 0 && value.integer < domain.enumeration.stringCount
                     || value.integer == 0,
                     "(value.integer) = %i", value.integer );
        }
        break;

    case DVAR_TYPE_STRING:
        break;

    case DVAR_TYPE_COLOR:
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", type ) );
        break;
    }

    return value;
}

/* Dvar_VectorInDomain  0x00476fd0 */
static bool Dvar_VectorInDomain( const float *value, int components, float min, float max )
{
    int i;

    for ( i = 0; i < components; i++ )
    {
        if ( value[i] < min )
            return false;

        if ( value[i] > max )
            return false;
    }

    return true;
}

/* Dvar_ValueInDomain  0x00476e00 */
bool Dvar_ValueInDomain( dvarType_t type, DvarValue_t value, DvarLimits_t domain )
{
    bool inDomain;

    switch ( type )
    {
    case DVAR_TYPE_BOOL:
        Assert( value.enabled == true || value.enabled == false );
        inDomain = true;
        break;

    case DVAR_TYPE_FLOAT:
        if ( value.value < domain.value.min )
            inDomain = false;
        else if ( value.value > domain.value.max )
            inDomain = false;
        else
            inDomain = true;
        break;

    case DVAR_TYPE_FLOAT_2:
        inDomain = Dvar_VectorInDomain( value.vector, 2, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_FLOAT_3:
        inDomain = Dvar_VectorInDomain( value.vector, 3, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_FLOAT_4:
        inDomain = Dvar_VectorInDomain( value.vector, 4, domain.vector.min, domain.vector.max );
        break;

    case DVAR_TYPE_INT:
        Assert( domain.integer.min <= domain.integer.max );
        if ( value.integer < domain.integer.min )
            inDomain = false;
        else if ( value.integer > domain.integer.max )
            inDomain = false;
        else
            inDomain = true;
        break;

    case DVAR_TYPE_ENUM:
        if ( ( value.integer < 0 || value.integer >= domain.enumeration.stringCount )
          && value.integer != 0 )
            inDomain = false;
        else
            inDomain = true;
        break;

    case DVAR_TYPE_STRING:
        inDomain = true;
        break;

    case DVAR_TYPE_COLOR:
        inDomain = true;
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", type ) );
        inDomain = false;
        break;
    }

    return inDomain;
}

/* Dvar_VectorDomainToString  0x004773b0 */
static void Dvar_VectorDomainToString( int components, DvarLimits_t domain,
                                       char *outBuffer, size_t outBufferLen )
{
    if ( domain.vector.min != -FLT_MAX )
    {
        if ( domain.vector.max != FLT_MAX )
            _snprintf( outBuffer, outBufferLen,
                       "Domain is any %iD vector with components from %g to %g",
                       components, domain.vector.min, domain.vector.max );
        else
            _snprintf( outBuffer, outBufferLen,
                       "Domain is any %iD vector with components %g or bigger",
                       components, domain.vector.min );
    }
    else if ( domain.vector.max != FLT_MAX )
    {
        _snprintf( outBuffer, outBufferLen,
                   "Domain is any %iD vector with components %g or smaller",
                   components, domain.vector.max );
    }
    else
    {
        _snprintf( outBuffer, outBufferLen, "Domain is any %iD vector", components );
    }
}

/* Dvar_DomainToString_Internal  0x00477060 */
static char *Dvar_DomainToString_Internal( dvarType_t type, DvarLimits_t domain,
                                           char *outBuffer, size_t outBufferLen,
                                           int *outLineCount )
{
    char *bufferEnd;
    char *pos;
    int   charsWritten;
    int   i;

    Assert( outBufferLen > 0 );

    bufferEnd = outBuffer + outBufferLen;

    if ( outLineCount )
        *outLineCount = 0;

    switch ( type )
    {
    case DVAR_TYPE_BOOL:
        _snprintf( outBuffer, outBufferLen, "Domain is 0 or 1" );
        break;

    case DVAR_TYPE_INT:
        if ( domain.integer.min != INT_MIN )
        {
            if ( domain.integer.max != INT_MAX )
                _snprintf( outBuffer, outBufferLen, "Domain is any integer from %i to %i",
                           domain.integer.min, domain.integer.max );
            else
                _snprintf( outBuffer, outBufferLen, "Domain is any integer %i or bigger",
                           domain.integer.min );
        }
        else if ( domain.integer.max != INT_MAX )
        {
            _snprintf( outBuffer, outBufferLen, "Domain is any integer %i or smaller",
                       domain.integer.max );
        }
        else
        {
            _snprintf( outBuffer, outBufferLen, "Domain is any integer" );
        }
        break;

    case DVAR_TYPE_FLOAT:
        if ( domain.value.min != -FLT_MAX )
        {
            if ( domain.value.max != FLT_MAX )
                _snprintf( outBuffer, outBufferLen, "Domain is any number from %g to %g",
                           domain.value.min, domain.value.max );
            else
                _snprintf( outBuffer, outBufferLen, "Domain is any number %g or bigger",
                           domain.value.min );
        }
        else if ( domain.value.max != FLT_MAX )
        {
            _snprintf( outBuffer, outBufferLen, "Domain is any number %g or smaller",
                       domain.value.max );
        }
        else
        {
            _snprintf( outBuffer, outBufferLen, "Domain is any number" );
        }
        break;

    case DVAR_TYPE_FLOAT_2:
        Dvar_VectorDomainToString( 2, domain, outBuffer, outBufferLen );
        break;

    case DVAR_TYPE_FLOAT_3:
        Dvar_VectorDomainToString( 3, domain, outBuffer, outBufferLen );
        break;

    case DVAR_TYPE_FLOAT_4:
        Dvar_VectorDomainToString( 4, domain, outBuffer, outBufferLen );
        break;

    case DVAR_TYPE_STRING:
        _snprintf( outBuffer, outBufferLen, "Domain is any text" );
        break;

    case DVAR_TYPE_ENUM:
        charsWritten = _snprintf( outBuffer, bufferEnd - outBuffer, "Domain is one of the following:" );
        if ( charsWritten < 0 )
            break;

        pos = outBuffer + charsWritten;
        for ( i = 0; i < domain.enumeration.stringCount; i++ )
        {
            charsWritten = _snprintf( pos, bufferEnd - pos, "\n  %2i: %s",
                                      i, domain.enumeration.strings[i] );
            if ( charsWritten < 0 )
                break;

            if ( outLineCount )
                ( *outLineCount )++;

            pos += charsWritten;
        }
        break;

    case DVAR_TYPE_COLOR:
        _snprintf( outBuffer, outBufferLen, "Domain is any 4-component color, in RGBA format" );
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", type ) );
        *outBuffer = '\0';
        break;
    }

    bufferEnd[-1] = '\0';
    return outBuffer;
}

/* Dvar_DomainToString  0x00477030 */
char *Dvar_DomainToString( dvarType_t type, DvarLimits_t domain,
                           char *outBuffer, int outBufferLen )
{
    return Dvar_DomainToString_Internal( type, domain, outBuffer, outBufferLen, NULL );
}

char *Dvar_DomainToStringWithLineCount( dvarType_t type, DvarLimits_t domain,
                                        char *outBuffer, int outBufferLen, int *outLineCount )
{
    Assert( outLineCount );
    return Dvar_DomainToString_Internal( type, domain, outBuffer, outBufferLen, outLineCount );
}

/* Dvar_PrintDomain  0x004774d0 */
void Dvar_PrintDomain( dvarType_t type, DvarLimits_t domain )
{
    char domainString[1024];

    Com_Printf( CON_CHANNEL_SYSTEM, "  %s\n",
                Dvar_DomainToString( type, domain, domainString, sizeof( domainString ) ) );
}

/* Dvar_ValuesEqual  0x00477590 */
bool Dvar_ValuesEqual( dvarType_t type, DvarValue_t val0, DvarValue_t val1 )
{
    bool equal;

    switch ( type )
    {
    case DVAR_TYPE_BOOL:
        equal = ( val0.enabled == val1.enabled );
        break;

    case DVAR_TYPE_FLOAT:
        equal = ( val0.value == val1.value );
        break;

    case DVAR_TYPE_FLOAT_2:
        equal = Vec2Compare( val0.vector, val1.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        equal = Vec3Compare( val0.vector, val1.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        equal = Vec4Compare( val0.vector, val1.vector );
        break;

    case DVAR_TYPE_INT:
        equal = ( val0.integer == val1.integer );
        break;

    case DVAR_TYPE_ENUM:
        equal = ( val0.integer == val1.integer );
        break;

    case DVAR_TYPE_STRING:
        Assert( val0.string );
        Assert( val1.string );
        equal = ( strcmp( val0.string, val1.string ) == 0 );
        break;

    case DVAR_TYPE_COLOR:
        equal = Byte4Compare( val0.color, val1.color );
        break;

    default:
        AssertMsg( va( "unhandled dvar type '%i'", type ) );
        equal = false;
        break;
    }

    return equal;
}

/* Dvar_HasLatchedValue  0x00477530 */
bool Dvar_HasLatchedValue( const Dvar_t *dvar )
{
    return !Dvar_ValuesEqual( dvar->type, dvar->current, dvar->latched );
}

/* Dvar_SetLatchedValue  0x00479080 */
void Dvar_SetLatchedValue( Dvar_t *dvar, DvarValue_t value )
{
    DvarValue_t newValue;
    DvarValue_t oldValue;
    bool        freeOld;

    switch ( dvar->type )
    {
    case DVAR_TYPE_FLOAT_2:
        Vec2Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        Vec3Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        Vec4Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_STRING:
        if ( dvar->latched.string != value.string )
        {
            freeOld = Dvar_ShouldFreeLatchedString( dvar );
            if ( freeOld )
                oldValue.string = dvar->latched.string;

            Dvar_MakeLatchedStringValue( dvar, &newValue, value.string );
            dvar->latched.string = newValue.string;

            if ( freeOld )
                Dvar_FreeString( &oldValue.string );
        }
        break;

    default:
        dvar->latched = value;
        break;
    }
}

/* Dvar_ClearLatchedValue  0x00479260 */
void Dvar_ClearLatchedValue( Dvar_t *dvar )
{
    if ( Dvar_HasLatchedValue( dvar ) )
        Dvar_SetLatchedValue( dvar, dvar->current );
}

/* Dvar_ApplyLatchedValue  0x00478930 */
void Dvar_ApplyLatchedValue( Dvar_t *dvar )
{
    Dvar_SetVariant( dvar, dvar->latched, DVAR_SOURCE_INTERNAL );
}

/* Dvar_SetVariant  0x00478970 */
void Dvar_SetVariant( Dvar_t *dvar, DvarValue_t value, DvarSetSource_t source )
{
    DvarValue_t newValue;
    DvarValue_t oldValue;
    bool        freeOld;
    bool        freeLatched;

    Assert( dvar );
    Assert( dvar->name );

    if ( !Dvar_ValueInDomain( dvar->type, value, dvar->domain ) )
    {
        Com_Printf( CON_CHANNEL_SYSTEM, "'%s' is not a valid value for dvar '%s'\n",
                    Dvar_ValueToString( dvar, value ), dvar->name );
        Dvar_PrintDomain( dvar->type, dvar->domain );

        if ( dvar->type == DVAR_TYPE_ENUM )
        {
            Assertx( Dvar_ValueInDomain( dvar->type, dvar->reset, dvar->domain ),
                     "(dvar->name) = %s", dvar->name );
            Dvar_SetVariant( dvar, dvar->reset, source );
        }
        return;
    }

    if ( dvar->domainFunc && !dvar->domainFunc( dvar, value ) )
    {
        Com_Printf( CON_CHANNEL_SYSTEM, "'%s' is not a valid value for dvar '%s'\n\n",
                    Dvar_ValueToString( dvar, value ), dvar->name );
        return;
    }

    if ( source == DVAR_SOURCE_EXTERNAL || source == DVAR_SOURCE_SCRIPT )
    {
        if ( dvar->flags & DVAR_ROM )
        {
            Com_Printf( CON_CHANNEL_SYSTEM, "%s is read only.\n", dvar->name );
            return;
        }

        if ( dvar->flags & DVAR_INIT )
        {
            Com_Printf( CON_CHANNEL_SYSTEM, "%s is write protected.\n", dvar->name );
            return;
        }

        if ( source == DVAR_SOURCE_EXTERNAL && ( dvar->flags & DVAR_CHEAT )
          && !dvar_cheats->current.enabled )
        {
            Com_Printf( CON_CHANNEL_SYSTEM, "%s is cheat protected.\n", dvar->name );
            return;
        }

        if ( dvar->flags & DVAR_LATCH )
        {
            Dvar_SetLatchedValue( dvar, value );
            if ( !Dvar_ValuesEqual( dvar->type, dvar->latched, dvar->current ) )
                Com_Printf( CON_CHANNEL_SYSTEM, "%s will be changed upon restarting.\n", dvar->name );
            return;
        }
    }
    else if ( source == DVAR_SOURCE_DEVGUI && ( dvar->flags & DVAR_DEVGUILATCH ) )
    {
        Dvar_SetLatchedValue( dvar, value );
        return;
    }

    if ( !Dvar_ValuesEqual( dvar->type, dvar->current, value ) )
    {
        dvar_modifiedFlags |= dvar->flags;

        switch ( dvar->type )
        {
        case DVAR_TYPE_FLOAT_2:
            Vec2Copy( value.vector, dvar->current.vector );
            Vec2Copy( value.vector, dvar->latched.vector );
            break;

        case DVAR_TYPE_FLOAT_3:
            Vec3Copy( value.vector, dvar->current.vector );
            Vec3Copy( value.vector, dvar->latched.vector );
            break;

        case DVAR_TYPE_FLOAT_4:
            Vec4Copy( value.vector, dvar->current.vector );
            Vec4Copy( value.vector, dvar->latched.vector );
            break;

        case DVAR_TYPE_STRING:
            Assert( dvar->name );
            Assertx( value.string != dvar->current.string
                     || value.string == dvar->latched.string
                     || value.string == dvar->reset.string,
                     "(dvar->name) = %s", dvar->name );

            freeOld = Dvar_ShouldFreeCurrentString( dvar );
            if ( freeOld )
                oldValue.string = dvar->current.string;

            Dvar_MakeCurrentStringValue( dvar, &newValue, value.string );
            dvar->current.string = newValue.string;

            freeLatched = Dvar_ShouldFreeLatchedString( dvar );
            if ( freeLatched )
                Dvar_FreeString( &dvar->latched.string );
            dvar->latched.string = NULL;
            Dvar_MakeStringValue( dvar->current.string, &dvar->latched );

            if ( freeOld )
                Dvar_FreeString( &oldValue.string );
            break;

        default:
            dvar->current = value;
            dvar->latched = value;
            break;
        }

        dvar->modified = true;
    }
    else
    {
        Dvar_SetLatchedValue( dvar, dvar->current );
    }
}

/* Dvar_ClearModified  0x00477830 */
void Dvar_ClearModified( Dvar_t *dvar )
{
    Assert( dvar );
    dvar->modified = false;
}

void Dvar_SetModified( Dvar_t *dvar )
{
    Assert( dvar );
    dvar->modified = true;
}

/* Dvar_SetEnumDomain  0x004778b0 */
void Dvar_SetEnumDomain( Dvar_t *dvar, const char **stringTable )
{
    int stringCount;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( stringTable, "(dvar->name) = %s", dvar->name );
    Assertx( dvar->type == DVAR_TYPE_ENUM, "%s",
             va( "dvar %s type %i", dvar->name, dvar->type ) );

    for ( stringCount = 0; stringTable[stringCount]; stringCount++ )
        ;

    Assertx( dvar->reset.integer >= 0
             && ( dvar->reset.integer < stringCount || dvar->reset.integer == 0 ), "%s",
             va( "name %i reset %i count %i", dvar->name, dvar->reset.integer, stringCount ) );

    dvar->domain.enumeration.stringCount = stringCount;
    dvar->domain.enumeration.strings = stringTable;

    dvar->current = Dvar_ClampValueToDomain( dvar->type, dvar->current, dvar->reset, dvar->domain );
    dvar->latched = dvar->current;
}

/* Dvar_GetBool  0x00477d20 */
bool Dvar_GetBool( const char *dvarName )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        return false;

    Assertx( dvar->type == DVAR_TYPE_BOOL
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->type) = %i", dvar->type );

    if ( dvar->type == DVAR_TYPE_BOOL )
        return dvar->current.enabled;

    return Dvar_StringToBool( dvar->current.string );
}

int Dvar_GetInt( const char *dvarName )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        return 0;

    Assertx( dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->type) = %i", dvar->type );

    if ( dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM )
        return dvar->current.integer;

    return Dvar_StringToInt( dvar->current.string );
}

float Dvar_GetFloat( const char *dvarName )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        return 0.0f;

    Assertx( dvar->type == DVAR_TYPE_FLOAT
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->type) = %i", dvar->type );

    if ( dvar->type == DVAR_TYPE_FLOAT )
        return dvar->current.value;

    return Dvar_StringToFloat( dvar->current.string );
}

void Dvar_GetVec3( const char *dvarName, vec3_t value )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
    {
        Vec3Copy( vec3_origin, value );
        return;
    }

    Assert( dvar->type == DVAR_TYPE_FLOAT_3 );
    Vec3Copy( dvar->current.vector, value );
}

const char *Dvar_GetString( const char *dvarName )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        return "";

    Assertx( dvar->type == DVAR_TYPE_STRING || dvar->type == DVAR_TYPE_ENUM,
             "(dvar->type) = %i", dvar->type );

    if ( dvar->type == DVAR_TYPE_ENUM )
        return Dvar_EnumToString( dvar );

    return dvar->current.string;
}

const char *Dvar_GetVariantString( const char *dvarName )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        return "";

    return Dvar_ValueToString( dvar, dvar->current );
}

/* Dvar_GetUnpackedColor  0x00478120 */
void Dvar_GetUnpackedColor( const Dvar_t *dvar, vec4_t expandedColor )
{
    byte color[4];

    Assert( dvar );
    Assertx( dvar->type == DVAR_TYPE_COLOR
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->type) = %i", dvar->type );

    if ( dvar->type == DVAR_TYPE_COLOR )
        Byte4Copy( dvar->current.color, color );
    else
        Dvar_StringToColor( dvar->current.string, color );

    expandedColor[0] = color[0] * ( 1.0f / 255.0f );
    expandedColor[1] = color[1] * ( 1.0f / 255.0f );
    expandedColor[2] = color[2] * ( 1.0f / 255.0f );
    expandedColor[3] = color[3] * ( 1.0f / 255.0f );
}

static const vec4_t s_dvarColorWhite = { 1.0f, 1.0f, 1.0f, 1.0f };

void Dvar_GetUnpackedColorByName( const char *dvarName, vec4_t expandedColor )
{
    const Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
    {
        Vec4Copy( s_dvarColorWhite, expandedColor );
        return;
    }

    Dvar_GetUnpackedColor( dvar, expandedColor );
}

/* Dvar_Shutdown  0x004783c0 */
void Dvar_Shutdown( void )
{
    int     i;
    Dvar_t *dvar;

    for ( i = 0; i < s_dvarCount; i++ )
    {
        dvar = &s_dvarPool[i];

        if ( dvar->type == DVAR_TYPE_STRING )
        {
            if ( Dvar_ShouldFreeCurrentString( dvar ) )
                Dvar_FreeString( &dvar->current.string );
            dvar->current.string = NULL;

            if ( Dvar_ShouldFreeResetString( dvar ) )
                Dvar_FreeString( &dvar->reset.string );
            dvar->reset.string = NULL;

            if ( Dvar_ShouldFreeLatchedString( dvar ) )
                Dvar_FreeString( &dvar->latched.string );
            dvar->latched.string = NULL;
        }

        if ( dvar->flags & DVAR_EXTERNAL )
            Dvar_FreeName( dvar->name );
    }

    s_dvarCount = 0;
    dvar_cheats = NULL;
    dvar_modifiedFlags = 0;
    s_isDvarSystemActive = false;

    memset( s_dvarHashTable, 0, sizeof( s_dvarHashTable ) );
}

/* Dvar_MakeExternal  0x0047a190 */
static void Dvar_MakeExternal( Dvar_t *dvar )
{
    DvarValue_t newValue;

    Assert( dvar );

    if ( !( dvar->flags & DVAR_EXTERNAL ) )
    {
        dvar->flags |= DVAR_EXTERNAL;
        dvar->name = Dvar_CopyName( dvar->name );
    }

    if ( dvar->type != DVAR_TYPE_STRING )
    {
        Dvar_CopyStringValue( Dvar_DisplayableLatchedValue( dvar ), &dvar->current );

        if ( Dvar_ShouldFreeLatchedString( dvar ) )
            Dvar_FreeString( &dvar->latched.string );
        dvar->latched.string = NULL;
        Dvar_MakeStringValue( dvar->current.string, &dvar->latched );

        if ( Dvar_ShouldFreeResetString( dvar ) )
            Dvar_FreeString( &dvar->reset.string );
        dvar->reset.string = NULL;
        Dvar_MakeResetStringValue( dvar, &newValue, Dvar_DisplayableResetValue( dvar ) );
        dvar->reset.string = newValue.string;

        dvar->type = DVAR_TYPE_STRING;
    }
}

/* Dvar_SetResetValue  0x004786b0 */
static void Dvar_SetResetValue( Dvar_t *dvar, DvarValue_t value )
{
    DvarValue_t newValue;
    DvarValue_t oldValue;
    bool        freeOld;

    Assert( dvar );

    switch ( dvar->type )
    {
    case DVAR_TYPE_FLOAT_2:
        Vec2Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        Vec3Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        Vec4Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_STRING:
        if ( dvar->reset.string != value.string )
        {
            freeOld = Dvar_ShouldFreeResetString( dvar );
            if ( freeOld )
                oldValue.string = dvar->reset.string;

            Dvar_MakeResetStringValue( dvar, &newValue, value.string );
            dvar->reset.string = newValue.string;

            if ( freeOld )
                Dvar_FreeString( &oldValue.string );
        }
        break;

    default:
        dvar->reset = value;
        break;
    }
}

void Dvar_SetResetVariant( Dvar_t *dvar, DvarValue_t value )
{
    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->flags & ( 1 << 9 ), "(dvar->name) = %s", dvar->name );

    Dvar_SetResetValue( dvar, value );
}

/* Dvar_AssignCurrentAndLatchedValues  0x00479e90 */
static void Dvar_AssignCurrentAndLatchedValues( Dvar_t *dvar, DvarValue_t value )
{
    DvarValue_t newValue;
    DvarValue_t oldValue;
    bool        freeOld;

    Assert( dvar );

    switch ( dvar->type )
    {
    case DVAR_TYPE_FLOAT_2:
        Vec2Copy( value.vector, dvar->current.vector );
        Vec2Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        Vec3Copy( value.vector, dvar->current.vector );
        Vec3Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        Vec4Copy( value.vector, dvar->current.vector );
        Vec4Copy( value.vector, dvar->latched.vector );
        break;

    case DVAR_TYPE_STRING:
        if ( value.string != dvar->current.string )
        {
            freeOld = Dvar_ShouldFreeCurrentString( dvar );
            if ( freeOld )
                oldValue.string = dvar->current.string;

            Dvar_MakeCurrentStringValue( dvar, &newValue, value.string );
            dvar->current.string = newValue.string;

            if ( Dvar_ShouldFreeLatchedString( dvar ) )
                Dvar_FreeString( &dvar->latched.string );
            dvar->latched.string = NULL;
            Dvar_MakeStringValue( dvar->current.string, &dvar->latched );

            if ( freeOld )
                Dvar_FreeString( &oldValue.string );
        }
        break;

    default:
        dvar->current = value;
        dvar->latched = value;
        break;
    }
}

/* Dvar_ReRegisterVariant  0x00479780 */
static void Dvar_ReRegisterVariant( Dvar_t *dvar, const char *dvarName, dvarType_t type,
                                    unsigned short flags, DvarValue_t resetValue,
                                    DvarLimits_t domain )
{
    DvarValue_t newValue;
    bool        freeNew;

    Assertx( dvar->type == DVAR_TYPE_STRING, "(dvar->type) = %i", dvar->type );

    dvar->type = type;
    dvar->domain = domain;

    if ( !( flags & DVAR_ROM )
      && ( !( flags & DVAR_CHEAT ) || !dvar_cheats || dvar_cheats->current.enabled ) )
    {
        newValue = Dvar_StringToValue( dvar->type, dvar->domain, dvar->current.string );
        newValue = Dvar_ClampValueToDomain( type, newValue, resetValue, domain );
    }
    else
    {
        newValue = resetValue;
    }

    freeNew = ( dvar->type == DVAR_TYPE_STRING && newValue.string != NULL );
    if ( freeNew )
        newValue.string = CopyString( newValue.string );

    if ( dvar->type != DVAR_TYPE_STRING )
    {
        if ( Dvar_ShouldFreeCurrentString( dvar ) )
            Dvar_FreeString( &dvar->current.string );
    }
    dvar->current.string = NULL;

    if ( Dvar_ShouldFreeLatchedString( dvar ) )
        Dvar_FreeString( &dvar->latched.string );
    dvar->latched.string = NULL;

    if ( Dvar_ShouldFreeResetString( dvar ) )
        Dvar_FreeString( &dvar->reset.string );
    dvar->reset.string = NULL;

    Dvar_SetResetValue( dvar, resetValue );
    Dvar_AssignCurrentAndLatchedValues( dvar, newValue );

    dvar_modifiedFlags |= flags;

    if ( freeNew )
        FreeString( (char *)newValue.string );
}

/* Dvar_GetSavedResetValue  0x0047a300 */
static DvarValue_t Dvar_GetSavedResetValue( const Dvar_t *dvar, DvarValue_t resetValue,
                                            dvarType_t type, unsigned short flags,
                                            DvarLimits_t domain )
{
    DvarValue_t value;
    bool        canKeepValue;

    canKeepValue = ( ( dvar->flags & ( DVAR_INIT | DVAR_ROM | DVAR_CHEAT ) ) == 0 );

    if ( ( dvar->flags & DVAR_AUTOEXEC ) && ( flags & DVAR_SAVED ) && canKeepValue )
        value = Dvar_StringToValue( type, domain, dvar->reset.string );
    else
        value = resetValue;

    return value;
}

/* Dvar_ReinterpretExternalDvar  0x0047a070 */
static void Dvar_ReinterpretExternalDvar( Dvar_t *dvar, const char *dvarName, dvarType_t type,
                                          unsigned short flags, DvarValue_t resetValue,
                                          DvarLimits_t domain )
{
    DvarValue_t value;

    if ( ( dvar->flags & DVAR_EXTERNAL ) && !( flags & DVAR_EXTERNAL ) )
    {
        value = Dvar_GetSavedResetValue( dvar, resetValue, type, flags, domain );

        Dvar_MakeExternal( dvar );
        Dvar_FreeName( dvar->name );

        dvar->name = dvarName;
        dvar->flags &= ~DVAR_EXTERNAL;

        Dvar_ReRegisterVariant( dvar, dvarName, type, flags, value, domain );
    }
}

/* Dvar_ReRegister  0x00479410 */
static void Dvar_ReRegister( Dvar_t *dvar, const char *dvarName, dvarType_t type,
                             unsigned short flags, DvarValue_t resetValue,
                             DvarLimits_t domain, const char *description )
{
    Assert( dvar );
    Assert( dvarName );
    Assertx( dvar->type == type || ( dvar->flags & DVAR_EXTERNAL ), "%s",
             va( "%s: %i != %i", dvarName, dvar->type, type ) );

    if ( ( dvar->flags ^ flags ) & DVAR_EXTERNAL )
        Dvar_ReinterpretExternalDvar( dvar, dvarName, type, flags, resetValue, domain );

    if ( ( dvar->flags & DVAR_EXTERNAL ) && dvar->type != type )
    {
        Assertx( dvar->type == DVAR_TYPE_STRING, "%s",
                 va( "dvar %s, type %i", dvar->name, dvar->type ) );
        Dvar_ReRegisterVariant( dvar, dvarName, type, flags, resetValue, domain );
    }

    Assertx( dvar->type == type, "(dvarName) = %s", dvarName );

    if ( !( dvar->flags & ( DVAR_CHANGEABLE_RESET | DVAR_SAVED | DVAR_AUTOEXEC ) )
      && !Dvar_ValuesEqual( type, dvar->reset, resetValue ) )
    {
        Assertx( ( dvar->flags & ( DVAR_CHANGEABLE_RESET | DVAR_SAVED | DVAR_AUTOEXEC ) )
                 || Dvar_ValuesEqual( type, dvar->reset, resetValue ), "%s",
                 va( "dvar %s, %s != %s", dvarName,
                     Dvar_DisplayableResetValue( dvar ),
                     Dvar_ValueToString( dvar, resetValue ) ) );
    }

    dvar->flags |= flags;

    if ( description )
        dvar->description = description;

    if ( ( dvar->flags & DVAR_CHEAT ) && dvar_cheats && !dvar_cheats->current.enabled )
    {
        Dvar_SetVariant( dvar, dvar->reset, DVAR_SOURCE_INTERNAL );
        Dvar_SetLatchedValue( dvar, dvar->reset );
    }

    if ( dvar->flags & DVAR_LATCH )
        Dvar_ApplyLatchedValue( dvar );
}

/* Dvar_CreateNew  0x0047a3e0 */
static Dvar_t *Dvar_CreateNew( const char *dvarName, dvarType_t type, unsigned short flags,
                               DvarValue_t value, DvarLimits_t domain, const char *description )
{
    Dvar_t *dvar;
    int     hash;

    if ( s_dvarCount >= MAX_DVARS )
        Com_ErrorLevel( ERR_FATAL, "Can't create dvar '%s': %i dvars already exist",
                        dvarName, MAX_DVARS );

    dvar = &s_dvarPool[s_dvarCount];
    s_sortedDvars[s_dvarCount] = dvar;
    s_areDvarsSorted = false;
    s_dvarCount++;

    dvar->type = type;

    if ( flags & DVAR_EXTERNAL )
        dvar->name = Dvar_CopyName( dvarName );
    else
        dvar->name = dvarName;

    switch ( type )
    {
    case DVAR_TYPE_FLOAT_2:
        Vec2Copy( value.vector, dvar->current.vector );
        Vec2Copy( value.vector, dvar->latched.vector );
        Vec2Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_FLOAT_3:
        Vec3Copy( value.vector, dvar->current.vector );
        Vec3Copy( value.vector, dvar->latched.vector );
        Vec3Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_FLOAT_4:
        Vec4Copy( value.vector, dvar->current.vector );
        Vec4Copy( value.vector, dvar->latched.vector );
        Vec4Copy( value.vector, dvar->reset.vector );
        break;

    case DVAR_TYPE_STRING:
        Dvar_CopyStringValue( value.string, &dvar->current );
        Dvar_MakeStringValue( dvar->current.string, &dvar->latched );
        Dvar_MakeStringValue( dvar->current.string, &dvar->reset );
        break;

    default:
        dvar->current = value;
        dvar->latched = value;
        dvar->reset = value;
        break;
    }

    dvar->domain = domain;
    dvar->modified = false;
    dvar->domainFunc = NULL;
    dvar->flags = flags;
    dvar->description = description;

    hash = Dvar_GenerateHashValue( dvarName );
    dvar->hashNext = s_dvarHashTable[hash];
    s_dvarHashTable[hash] = dvar;

    return dvar;
}

/* Dvar_RegisterVariant  0x00479320 */
Dvar_t *Dvar_RegisterVariant( const char *dvarName, dvarType_t type, unsigned short flags,
                              DvarValue_t value, DvarLimits_t domain, const char *description )
{
    Dvar_t *dvar;

    Assertx( ( flags & ( 1 << 14 ) ) || CanKeepStringPointer( dvarName ),
             "(dvarName) = %s", dvarName );

    dvar = Dvar_FindVar( dvarName );
    if ( dvar )
        Dvar_ReRegister( dvar, dvarName, type, flags, value, domain, description );
    else
        dvar = Dvar_CreateNew( dvarName, type, flags, value, domain, description );

    return dvar;
}


/* 0x004792b0 */
Dvar_t *Dvar_RegisterBool( const char *dvarName, bool value,
                           unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    dvarValue.enabled = value;
    memset( &dvarDomain, 0, sizeof( dvarDomain ) );

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_BOOL, flags, dvarValue, dvarDomain, description );
}

/* 0x0047a670 */
Dvar_t *Dvar_RegisterInt( const char *dvarName, int value, int min, int max,
                          unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    dvarValue.integer = value;
    dvarDomain.integer.min = min;
    dvarDomain.integer.max = max;

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_INT, flags, dvarValue, dvarDomain, description );
}

/* 0x0047a6e0 */
Dvar_t *Dvar_RegisterFloat( const char *dvarName, float value, float min, float max,
                            unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    dvarValue.value = value;
    dvarDomain.value.min = min;
    dvarDomain.value.max = max;

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_FLOAT, flags, dvarValue, dvarDomain, description );
}

/* 0x0047a750 */
Dvar_t *Dvar_RegisterVec2( const char *dvarName, float x, float y, float min, float max,
                           unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    Vec2Set( dvarValue.vector, x, y );
    dvarDomain.vector.min = min;
    dvarDomain.vector.max = max;

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_FLOAT_2, flags, dvarValue, dvarDomain, description );
}

/* 0x0047a7d0 */
Dvar_t *Dvar_RegisterVec3( const char *dvarName, float x, float y, float z,
                           float min, float max,
                           unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    Vec3Set( dvarValue.vector, x, y, z );
    dvarDomain.vector.min = min;
    dvarDomain.vector.max = max;

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_FLOAT_3, flags, dvarValue, dvarDomain, description );
}

/* 0x0047a860 */
Dvar_t *Dvar_RegisterVec4( const char *dvarName, float x, float y, float z, float w,
                           float min, float max,
                           unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    Vec4Set( dvarValue.vector, x, y, z, w );
    dvarDomain.vector.min = min;
    dvarDomain.vector.max = max;

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_FLOAT_4, flags, dvarValue, dvarDomain, description );
}

/* Dvar_RegisterString  0x0047a8f0 */
Dvar_t *Dvar_RegisterString( const char *dvarName, const char *value,
                             unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    Assert( dvarName );
    Assert( value );
    Assertx( ( flags & ( 1 << 14 ) ) || CanKeepStringPointer( value ),
             "(dvarName) = %s", dvarName );

    dvarValue.string = value;
    memset( &dvarDomain, 0, sizeof( dvarDomain ) );

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_STRING, flags, dvarValue, dvarDomain, description );
}

/* Dvar_RegisterEnum  0x0047a9f0 */
Dvar_t *Dvar_RegisterEnum( const char *dvarName, const char **valueList, int defaultIndex,
                           unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;
    int          stringCount;

    Assert( dvarName );
    Assert( valueList );

    dvarValue.integer = defaultIndex;
    dvarDomain.enumeration.strings = valueList;

    for ( stringCount = 0; valueList[stringCount]; stringCount++ )
        ;
    dvarDomain.enumeration.stringCount = stringCount;

    Assertx( defaultIndex >= 0 && defaultIndex < dvarDomain.enumeration.stringCount
             || defaultIndex == 0,
             "(dvarName) = %s", dvarName );

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_ENUM, flags, dvarValue, dvarDomain, description );
}

/* Dvar_RegisterColor  0x0047ab00 */
Dvar_t *Dvar_RegisterColor( const char *dvarName, float r, float g, float b, float a,
                            unsigned short flags, const char *description )
{
    DvarValue_t  dvarValue;
    DvarLimits_t dvarDomain;

    dvarValue.color[0] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, r ) ) * 255.0 + 0.001f );
    dvarValue.color[1] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, g ) ) * 255.0 + 0.001f );
    dvarValue.color[2] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, b ) ) * 255.0 + 0.001f );
    dvarValue.color[3] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, a ) ) * 255.0 + 0.001f );
    memset( &dvarDomain, 0, sizeof( dvarDomain ) );

    return Dvar_RegisterVariant( dvarName, DVAR_TYPE_COLOR, flags, dvarValue, dvarDomain, description );
}


void Dvar_SetBoolFromSource( Dvar_t *dvar, bool value, DvarSetSource_t source )
{
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_BOOL
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type != DVAR_TYPE_BOOL )
    {
        if ( value )
            value = (bool)"1";
        else
            value = (bool)"0";
    }

    newValue.enabled = value;
    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetIntFromSource( Dvar_t *dvar, int value, DvarSetSource_t source )
{
    char        string[32];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM )
    {
        newValue.integer = value;
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%i", value );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetFloatFromSource( Dvar_t *dvar, float value, DvarSetSource_t source )
{
    char        string[32];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_FLOAT
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_FLOAT )
    {
        newValue.value = value;
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%g", value );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetVec2FromSource( Dvar_t *dvar, float x, float y, DvarSetSource_t source )
{
    char        string[64];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_FLOAT_4
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_FLOAT_2 )
    {
        Vec2Set( newValue.vector, x, y );
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%g %g", x, y );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetVec3FromSource( Dvar_t *dvar, float x, float y, float z, DvarSetSource_t source )
{
    char        string[96];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_FLOAT_3
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_FLOAT_3 )
    {
        Vec3Set( newValue.vector, x, y, z );
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%g %g %g", x, y, z );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetVec4FromSource( Dvar_t *dvar, float x, float y, float z, float w,
                             DvarSetSource_t source )
{
    char        string[128];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_FLOAT_4
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_FLOAT_4 )
    {
        Vec4Set( newValue.vector, x, y, z, w );
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%g %g %g %g", x, y, z, w );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetStringFromSource( Dvar_t *dvar, const char *string, DvarSetSource_t source )
{
    char        stringCopy[1024];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_STRING || dvar->type == DVAR_TYPE_ENUM,
             "(dvar->name) = %s", dvar->name );
    Assert( string );

    if ( dvar->type == DVAR_TYPE_STRING )
    {
        I_strncpyz( stringCopy, string, sizeof( stringCopy ) );
        newValue.string = stringCopy;
    }
    else
    {
        newValue.integer = Dvar_StringToEnum( &dvar->domain, string );
        Assertx( newValue.integer != DVAR_INVALID_ENUM_INDEX, "%s",
                 va( "%s doesn't include %s", dvar->name, string ) );
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetColorFromSource( Dvar_t *dvar, float r, float g, float b, float a,
                              DvarSetSource_t source )
{
    char        string[128];
    DvarValue_t newValue;

    Assert( dvar );
    Assert( dvar->name );
    Assertx( dvar->type == DVAR_TYPE_COLOR
             || ( dvar->type == DVAR_TYPE_STRING && ( dvar->flags & ( 1 << 14 ) ) ),
             "(dvar->name) = %s", dvar->name );

    if ( dvar->type == DVAR_TYPE_COLOR )
    {
        newValue.color[0] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, r ) ) * 255.0 );
        newValue.color[1] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, g ) ) * 255.0 );
        newValue.color[2] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, b ) ) * 255.0 );
        newValue.color[3] = (byte)RoundFloatToInt( I_fmax( 0.0f, I_fmin( 1.0f, a ) ) * 255.0 );
    }
    else
    {
        Com_sprintf( string, sizeof( string ), "%g %g %g %g", r, g, b, a );
        newValue.string = string;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

/* Dvar_SetFromStringFromSource  0x0047b950 */
void Dvar_SetFromStringFromSource( Dvar_t *dvar, const char *string, DvarSetSource_t source )
{
    char        stringCopy[1024];
    DvarValue_t newValue;

    I_strncpyz( stringCopy, string, sizeof( stringCopy ) );
    newValue = Dvar_StringToValue( dvar->type, dvar->domain, stringCopy );

    if ( dvar->type == DVAR_TYPE_ENUM && newValue.integer == DVAR_INVALID_ENUM_INDEX )
    {
        Com_Printf( CON_CHANNEL_SYSTEM, "'%s' is not a valid value for dvar '%s'\n",
                    stringCopy, dvar->name );
        Dvar_PrintDomain( dvar->type, dvar->domain );
        newValue = dvar->reset;
    }

    Dvar_SetVariant( dvar, newValue, source );
}

void Dvar_SetBool( Dvar_t *dvar, bool value )
{
    Dvar_SetBoolFromSource( dvar, value, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetInt( Dvar_t *dvar, int value )
{
    Dvar_SetIntFromSource( dvar, value, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetFloat( Dvar_t *dvar, float value )
{
    Dvar_SetFloatFromSource( dvar, value, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetVec2( Dvar_t *dvar, float x, float y )
{
    Dvar_SetVec2FromSource( dvar, x, y, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetVec3( Dvar_t *dvar, float x, float y, float z )
{
    Dvar_SetVec3FromSource( dvar, x, y, z, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetVec4( Dvar_t *dvar, float x, float y, float z, float w )
{
    Dvar_SetVec4FromSource( dvar, x, y, z, w, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetString( Dvar_t *dvar, const char *string )
{
    Dvar_SetStringFromSource( dvar, string, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetColor( Dvar_t *dvar, float r, float g, float b, float a )
{
    Dvar_SetColorFromSource( dvar, r, g, b, a, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetFromString( Dvar_t *dvar, const char *string )
{
    Dvar_SetFromStringFromSource( dvar, string, DVAR_SOURCE_INTERNAL );
}

void Dvar_SetBoolByName( const char *dvarName, bool value )
{
    Dvar_t     *dvar;
    const char *string;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
    {
        if ( value )
            string = "1";
        else
            string = "0";

        Dvar_RegisterString( dvarName, string, DVAR_EXTERNAL, "External Dvar" );
    }
    else
    {
        Dvar_SetBool( dvar, value );
    }
}

void Dvar_SetIntByName( const char *dvarName, int value )
{
    Dvar_t *dvar;
    char    string[32];

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
    {
        Com_sprintf( string, sizeof( string ), "%i", value );
        Dvar_RegisterString( dvarName, string, DVAR_EXTERNAL, "External Dvar" );
    }
    else
    {
        Dvar_SetInt( dvar, value );
    }
}

void Dvar_SetFloatByName( const char *dvarName, float value )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, va( "%g", value ), DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetFloat( dvar, value );
}

void Dvar_SetVec2ByName( const char *dvarName, float x, float y )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, va( "%g %g", x, y ), DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetVec2( dvar, x, y );
}

void Dvar_SetVec3ByName( const char *dvarName, float x, float y, float z )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, va( "%g %g %g", x, y, z ), DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetVec3( dvar, x, y, z );
}

void Dvar_SetVec4ByName( const char *dvarName, float x, float y, float z, float w )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, va( "%g %g %g %g", x, y, z, w ),
                             DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetVec4( dvar, x, y, z, w );
}

void Dvar_SetStringByName( const char *dvarName, const char *string )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, string, DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetString( dvar, string );
}

void Dvar_SetColorByName( const char *dvarName, float r, float g, float b, float a )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        Dvar_RegisterString( dvarName, va( "%g %g %g %g", r, g, b, a ),
                             DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetColor( dvar, r, g, b, a );
}

Dvar_t *Dvar_SetFromStringByNameFromSource( const char *dvarName, const char *string,
                                            DvarSetSource_t source )
{
    Dvar_t *dvar;

    dvar = Dvar_FindMalleableVar( dvarName );
    if ( !dvar )
        dvar = Dvar_RegisterString( dvarName, string, DVAR_EXTERNAL, "External Dvar" );
    else
        Dvar_SetFromStringFromSource( dvar, string, source );

    return dvar;
}

/* 0x0047bea0 */
void Dvar_SetFromStringByName( const char *dvarName, const char *string )
{
    Dvar_SetFromStringByNameFromSource( dvarName, string, DVAR_SOURCE_INTERNAL );
}

/* Dvar_SetFromStringByNameExternal  0x0047bec0 */
void Dvar_SetFromStringByNameExternal( const char *dvarName, const char *string )
{
    Dvar_t *dvar;

    dvar = Dvar_SetFromStringByNameFromSource( dvarName, string, DVAR_SOURCE_EXTERNAL );

    if ( dvar && s_isLoadingAutoExec )
    {
        Dvar_AddFlags( dvar, DVAR_AUTOEXEC );
        Dvar_SetResetValue( dvar, dvar->current );
    }
}

/* Dvar_SetDomainFunc  0x0047bf30 */
void Dvar_SetDomainFunc( Dvar_t *dvar, DvarDomainFunc_t domainFunc )
{
    Assert( dvar );

    dvar->domainFunc = domainFunc;

    if ( domainFunc && !dvar->domainFunc( dvar, dvar->current ) )
    {
        Com_Printf( CON_CHANNEL_SYSTEM, "'%s' is not a valid value for dvar '%s'\n\n",
                    Dvar_ValueToString( dvar, dvar->current ), dvar->name );
        Dvar_Reset( dvar, DVAR_SOURCE_INTERNAL );
    }
}

/* Dvar_AddFlags  0x0047c000 */
void Dvar_AddFlags( Dvar_t *dvar, unsigned int flags )
{
    Assert( dvar );
    Assertx( ( flags & ( ( 1 << 7 ) | ( 1 << 4 ) | ( 1 << 6 ) | ( 1 << 14 ) | ( 1 << 5 ) ) ) == 0,
             "(flags) = %i", flags );

    dvar->flags |= (unsigned short)flags;
}

/* Dvar_Reset  0x0047c070 */
void Dvar_Reset( Dvar_t *dvar, DvarSetSource_t source )
{
    Assert( dvar );
    Dvar_SetVariant( dvar, dvar->reset, source );
}

/* Dvar_ResetCheatDvars  0x0047c0d0 */
void Dvar_ResetCheatDvars( void )
{
    int i;

    for ( i = 0; i < s_dvarCount; i++ )
    {
        if ( s_dvarPool[i].flags & DVAR_CHEAT )
            Dvar_SetVariant( &s_dvarPool[i], s_dvarPool[i].reset, DVAR_SOURCE_INTERNAL );
    }
}

/* Dvar_ResetFlaggedDvars  0x0047c1d0 */
void Dvar_ResetFlaggedDvars( unsigned short flags, DvarSetSource_t source )
{
    int i;

    for ( i = 0; i < s_dvarCount; i++ )
    {
        if ( s_dvarPool[i].flags & flags )
            Dvar_Reset( &s_dvarPool[i], source );
    }
}

/* Dvar_AnyLatchedValues  0x0047c180 */
bool Dvar_AnyLatchedValues( void )
{
    int i;

    for ( i = 0; i < s_dvarCount; i++ )
    {
        if ( Dvar_HasLatchedValue( &s_dvarPool[i] ) )
            return true;
    }

    return false;
}

/* Dvar_Init  0x0047c150 */
void Dvar_Init( void )
{
    s_isDvarSystemActive = true;
    dvar_cheats = Dvar_RegisterBool( "sv_cheats", 1, DVAR_INIT | DVAR_SYSTEMINFO, "External Dvar" );
}

/* Com_SaveDvarsToBuffer  0x0047c230 */
int Com_SaveDvarsToBuffer( const char **dvarNames, int dvarCount, char *buffer, int bufferSize )
{
    Com_ErrorLevel( ERR_FATAL, "Com_SaveDvarsToBuffer not handled" );
    return 1;
}

int Com_LoadDvarsFromBuffer( const char *buffer, int bufferSize )
{
    Com_ErrorLevel( ERR_FATAL, "Com_LoadDvarsFromBuffer not handled" );
    return 0;
}

/* Com_Prefetch  0x004765e0 */

void Com_Prefetch( const void *s, unsigned int bytes )
{
    unsigned int lines;

    (void)s;
    if ( bytes > 0x1000 )
        bytes = 0x1000;

    for ( lines = ( bytes + 0x1f ) >> 5; lines != 0; lines-- )
        ;
}

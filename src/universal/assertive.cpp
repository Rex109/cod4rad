/* Original: ..\src\universal\assertive.cpp */

#include "q_shared.h"
#include "assertive.h"

extern void Com_Printf( int channel, const char *fmt, ... );        /* 0x0040eec0 */

#define CON_CHANNEL_SYSTEM      16


#define ASSERT_MAX_FRAMES       32

#define ASSERT_NAME_SIZE        64

#define ASSERT_MESSAGE_SIZE     1024

#define ASSERT_LINE_BUFSIZE     256

#define ASSERT_PATH_SIZE        2048

#define X86_CALL_INSTR_SIZE     5

#define PE_PAGE_SIZE            0x1000

#define ASSERT_MIN_FRAME_ADDR   0x400
#define ASSERT_FRAME_RANGE      0x1400000

typedef struct
{
    unsigned int addr;                              /* +0x000 */
    char         moduleName[ASSERT_NAME_SIZE];      /* +0x004 */
    char         funcName[ASSERT_NAME_SIZE];        /* +0x044 */
    char         funcFile[ASSERT_NAME_SIZE];        /* +0x084 */
    unsigned int funcAddr;                          /* +0x0c4 */
    char         lineFile[ASSERT_NAME_SIZE];        /* +0x0c8 */
    unsigned int lineAddr;                          /* +0x108 */
    int          lineNum;                           /* +0x10c */
} AssertStackFrame_t;

typedef struct StackFrame_s
{
    struct StackFrame_s *prev;          /* +0x00 */
    unsigned int         returnAddr;    /* +0x04 */
} StackFrame_t;


static char  s_lineBuf[ASSERT_LINE_BUFSIZE];                /* 0x00531380 */
static int   s_lineLen;                                     /* 0x00531484 */
static char  s_moduleFileName[MAX_PATH];                    /* 0x00531488 */
static int   s_linePos;                                     /* 0x0053158c */

static AssertStackFrame_t s_frames[ASSERT_MAX_FRAMES];      /* 0x00531590 */
static char  s_assertText[MAXPRINTMSG];                     /* 0x00533790 */
static int   s_frameCount;                                  /* 0x00534790 */

int          g_assertsDisabled;                             /* 0x00534794 */

static char  s_exitOnAssert;                                /* 0x00534798 */
static int   s_assertActive;                                /* 0x0053479c */
static AssertHandler_t s_assertHandler;                     /* 0x005347a0 */
static int   s_assertType;                                  /* 0x005347a4 */
static char  s_assertMessage[ASSERT_MESSAGE_SIZE];          /* 0x005347a8 */

static int   Assertive_WalkStack( char *buffer, int skipFrames, int type );
static int   Assertive_FormatStackTrace( char *buffer );
static void  Assertive_LoadMapFiles( const char *dir );
static char  Assertive_ParseMapFile( FILE *file, unsigned int moduleBase, const char *moduleName );
static void  MapFileParseError( const char *message );
static char  Assertive_ReadLine( FILE *file );
static char  Assertive_SkipLines( int count, FILE *file, int unusedFlag );
static HMODULE Assertive_GetModuleHandle( const char *moduleName );
static void  Assertive_CopyToClipboard( void );
static char  AssertShowDialog( int type, int recursive );
static int   AssertMessage( const char *expr, const char *file, int line, int type,
                            int skipFrames, char *buffer );
static void  Assertive_UpdateExitOnAssert( void );
static char  Assertive_ExitOnAssert( void );


/* Assertive_ResetDisplay  0x00401000 */
void Assertive_ResetDisplay( void )
{
    HWND           desktop;
    HDC            hdc;
    WORD           ramp[3][256];
    unsigned short i;

    ChangeDisplaySettingsA( NULL, 0 );

    desktop = GetDesktopWindow();
    hdc = GetDC( desktop );

    for ( i = 0; i < 256; i++ )
    {
        ramp[0][i] = i * 0x101;
        ramp[1][i] = i * 0x101;
        ramp[2][i] = i * 0x101;
    }

    SetDeviceGammaRamp( hdc, ramp );
    ReleaseDC( desktop, hdc );
}


/* Assertive_WalkStack  0x004010d0 */
static int Assertive_WalkStack( char *buffer, int skipFrames, int type )
{
    int           delta;
    int           frameIndex;
    unsigned int  returnAddr;
    StackFrame_t *frame;
    StackFrame_t *framePtr;
    char          reachedMain;

    memset( s_frames, 0, sizeof( s_frames ) );
    s_frameCount = 0;

    __asm { mov framePtr, ebp }

    reachedMain = 0;

    for ( frameIndex = 0; frameIndex < skipFrames + ASSERT_MAX_FRAMES; frameIndex++ )
    {
        frame = framePtr;
        if ( ( unsigned int )frame <= ASSERT_MIN_FRAME_ADDR )
            break;

        framePtr   = frame->prev;
        returnAddr = frame->returnAddr;

        if ( frameIndex >= skipFrames )
        {
            s_frames[s_frameCount].addr = returnAddr - X86_CALL_INSTR_SIZE;
            s_frameCount++;

            if ( framePtr == NULL )
                break;

            delta = ( int )Assertive_WalkStack - ( int )framePtr;
            if ( delta < -ASSERT_FRAME_RANGE || delta > ASSERT_FRAME_RANGE )
                break;
        }
    }

    return Assertive_FormatStackTrace( buffer );
}


/* Assertive_FormatStackTrace  0x004011a0 */
static int Assertive_FormatStackTrace( char *buffer )
{
    char               *out;
    AssertStackFrame_t *frame;
    int                 frameIndex;

    Assertive_LoadMapFiles( "" );

    out = buffer;
    for ( frameIndex = 0; frameIndex < s_frameCount; frameIndex++ )
    {
        frame = &s_frames[frameIndex];
        if ( frame->moduleName[0] == 0 )
            continue;

        out += sprintf( out, "%s:    ", frame->moduleName );

        if ( frame->lineFile[0] != 0 )
        {
            out += sprintf( out, "%s        ...%s, line %i",
                            frame->funcName, frame->lineFile, frame->lineNum );
        }
        else if ( frame->funcName[0] != 0 )
        {
            out += sprintf( out, "%s        ...%s, address %x",
                            frame->funcName, frame->funcFile, frame->addr );
        }
        else
        {
            out += sprintf( out, "%s, address %x", frame->funcName, frame->addr );
        }

        out += sprintf( out, "\n" );
    }

    return ( int )( out - buffer );
}


/* Assertive_LoadMapFiles  0x004012e0 */
static void Assertive_LoadMapFiles( const char *dir )
{
    HANDLE           findHandle;
    FILE            *file;
    HMODULE          module;
    char             mapName[ASSERT_PATH_SIZE];
    CHAR             searchPattern[ASSERT_PATH_SIZE];
    WIN32_FIND_DATAA findFileData;

    if ( dir[0] != 0 )
        sprintf( searchPattern, "%s\\*.map", dir );
    else
        strcpy( searchPattern, "*.map" );

    findHandle = FindFirstFileA( searchPattern, &findFileData );
    if ( findHandle == INVALID_HANDLE_VALUE )
        return;

    do
    {
        module = Assertive_GetModuleHandle( findFileData.cFileName );
        if ( module == NULL )
            continue;

        file = fopen( findFileData.cFileName, "rb" );
        if ( file != NULL )
        {
            strcpy( mapName, findFileData.cFileName );
            mapName[strlen( mapName ) - 4] = 0;

            Assertive_ParseMapFile( file, ( unsigned int )module, mapName );
            fclose( file );
        }
    }
    while ( FindNextFileA( findHandle, &findFileData ) );

    FindClose( findHandle );
}


/* Assertive_ParseMapFile  0x00401430 */
static char Assertive_ParseMapFile( FILE *file, unsigned int moduleBase, const char *moduleName )
{
    AssertStackFrame_t *frame;
    const char         *sourceFile;
    char               *lastSpace;
    char               *openParen;
    char               *closeParen;
    char               *symbol;
    const char         *baseName;
    char               *at;
    unsigned int        moduleEnd;
    unsigned int        endAddr;
    unsigned int        matchAddr;
    unsigned int        symAddr;
    int                 preferredLoadAddr;
    int                 segIndex;
    int                 segOffset;
    int                 frameIndex;
    int                 fieldCount;
    int                 entry;
    int                 lineNums[4];
    int                 lineSegs[4];
    int                 lineOffsets[4];
    char                symName[MAX_OS_PATH];
    char                fileName[MAX_OS_PATH];

    do
    {
        if ( !Assertive_ReadLine( file ) )
            return 0;
    }
    while ( sscanf( s_lineBuf, " Preferred load address is %x\r\n", &preferredLoadAddr ) != 1 );

    if ( !Assertive_SkipLines( 2, file, 1 ) )
        return 0;

    moduleEnd = 0;
    for ( ;; )
    {
        if ( !Assertive_ReadLine( file ) )
            return 0;
        if ( s_lineBuf[0] == 0 )
            break;

        if ( sscanf( s_lineBuf, "%x:%x %xH %s %s",
                     &segIndex, &segOffset, &symAddr, symName, fileName ) != 5 )
        {
            MapFileParseError( "Unknown line format in the segments section" );
            return 0;
        }

        if ( segIndex == 1 )
        {
            endAddr = symAddr + segOffset + moduleBase + PE_PAGE_SIZE;
            if ( moduleEnd < endAddr )
                moduleEnd = endAddr;
        }
    }

    for ( frameIndex = 0; frameIndex < s_frameCount; frameIndex++ )
    {
        frame = &s_frames[frameIndex];
        if ( frame->addr >= moduleBase && frame->addr < moduleEnd )
            I_strncpyz( frame->moduleName, moduleName, ASSERT_NAME_SIZE );
    }

    do
    {
        if ( !Assertive_ReadLine( file ) )
            return 0;
    }
    while ( !strstr( s_lineBuf, "Publics by Value" ) );

    if ( !Assertive_SkipLines( 1, file, 1 ) )
        return 0;

    for ( ;; )
    {
        if ( !Assertive_ReadLine( file ) )
            return 0;
        if ( s_lineBuf[0] == 0 )
            break;

        if ( sscanf( s_lineBuf, "%x:%x %s %x", &segIndex, &segOffset, symName, &symAddr ) != 4 )
        {
            MapFileParseError( "Unknown line format in the public symbols section" );
            return 0;
        }

        lastSpace = strrchr( s_lineBuf, ' ' );
        if ( lastSpace == NULL || sscanf( lastSpace + 1, "%s", fileName ) != 1 )
        {
            MapFileParseError( "Couldn't parse file name in the public symbols section" );
            return 0;
        }

        matchAddr = symAddr;
        for ( frameIndex = 0; frameIndex < s_frameCount; frameIndex++ )
        {
            frame = &s_frames[frameIndex];
            if ( frame->addr < moduleBase || frame->addr >= moduleEnd )
                continue;
            if ( matchAddr > frame->addr )
                continue;
            if ( frame->funcName[0] != 0 && frame->funcAddr >= matchAddr )
                continue;

            frame->funcAddr = matchAddr;

            symbol = symName;
            if ( symbol[0] == '_' || symbol[0] == '?' )
                symbol++;
            I_strncpyz( frame->funcName, symbol, ASSERT_NAME_SIZE );

            at = strchr( frame->funcName, '@' );
            if ( at != NULL )
                *at = 0;

            baseName = strrchr( fileName, '\\' );
            if ( baseName != NULL )
                baseName++;
            else
                baseName = fileName;
            I_strncpyz( frame->funcFile, baseName, ASSERT_NAME_SIZE );
        }
    }

    if ( !Assertive_SkipLines( 2, file, 1 ) )
        return 0;
    if ( !Assertive_ReadLine( file ) )
        return 0;

    if ( !strcmp( s_lineBuf, " Static symbols\r" ) )
    {
        if ( !Assertive_SkipLines( 1, file, 1 ) )
            return 0;

        while ( Assertive_ReadLine( file ) && s_lineBuf[0] != 0 )
        {
            if ( sscanf( s_lineBuf, "%x:%x %s %x", &segIndex, &segOffset, symName, &symAddr ) != 4 )
            {
                MapFileParseError( "Unknown line format in the static symbols section" );
                return 0;
            }

            lastSpace = strrchr( s_lineBuf, ' ' );
            if ( lastSpace == NULL || sscanf( lastSpace + 1, "%s", fileName ) != 1 )
            {
                MapFileParseError( "Couldn't parse file name in the static symbols section" );
                return 0;
            }

            matchAddr = symAddr;
            for ( frameIndex = 0; frameIndex < s_frameCount; frameIndex++ )
            {
                frame = &s_frames[frameIndex];
                if ( frame->addr < moduleBase || frame->addr >= moduleEnd )
                    continue;
                if ( matchAddr > frame->addr )
                    continue;
                if ( frame->funcName[0] != 0 && frame->funcAddr >= matchAddr )
                    continue;

                frame->funcAddr = matchAddr;

                symbol = symName;
                if ( symbol[0] == '_' || symbol[0] == '?' )
                    symbol++;
                I_strncpyz( frame->funcName, symbol, ASSERT_NAME_SIZE );

                at = strchr( frame->funcName, '@' );
                if ( at != NULL )
                    *at = 0;

                baseName = strrchr( fileName, '\\' );
                if ( baseName != NULL )
                    baseName++;
                else
                    baseName = fileName;
                I_strncpyz( frame->funcFile, baseName, ASSERT_NAME_SIZE );
            }
        }
    }

    for ( ;; )
    {
        if ( !Assertive_ReadLine( file ) )
            return 1;

        if ( strncmp( s_lineBuf, "Line numbers for ", 17 ) )
        {
            MapFileParseError( "Expected line number section" );
            return 0;
        }

        openParen = strchr( s_lineBuf, '(' );
        if ( openParen == NULL )
        {
            MapFileParseError( "Couldn't find '(' for the name of the source file in line number section" );
            return 0;
        }

        closeParen = strchr( openParen, ')' );
        if ( closeParen == NULL )
        {
            MapFileParseError( "Couldn't find ')' for the name of the source file in line number section" );
            return 0;
        }

        strncpy( fileName, openParen + 1, closeParen - openParen - 1 );
        fileName[closeParen - openParen - 1] = 0;
        sourceFile = fileName;

        if ( !Assertive_SkipLines( 1, file, 1 ) )
            return 0;

        while ( Assertive_ReadLine( file ) && s_lineBuf[0] != 0 )
        {
            fieldCount = sscanf( s_lineBuf, "%i %x:%x %i %x:%x %i %x:%x %i %x:%x\r\n",
                                 &lineNums[0], &lineSegs[0], &lineOffsets[0],
                                 &lineNums[1], &lineSegs[1], &lineOffsets[1],
                                 &lineNums[2], &lineSegs[2], &lineOffsets[2],
                                 &lineNums[3], &lineSegs[3], &lineOffsets[3] );

            if ( fieldCount % 3 != 0 || fieldCount / 3 <= 0 )
            {
                MapFileParseError( "unknown line format in the line number section" );
                return 0;
            }

            for ( entry = 0; entry * 3 < fieldCount; entry++ )
            {
                matchAddr = lineOffsets[entry] + moduleBase + PE_PAGE_SIZE;

                for ( frameIndex = 0; frameIndex < s_frameCount; frameIndex++ )
                {
                    frame = &s_frames[frameIndex];
                    if ( frame->addr < moduleBase || frame->addr >= moduleEnd )
                        continue;
                    if ( matchAddr > frame->addr )
                        continue;
                    if ( frame->lineFile[0] != 0 && frame->lineAddr >= matchAddr )
                        continue;

                    frame->lineAddr = matchAddr;
                    frame->lineNum  = lineNums[entry];

                    baseName = strrchr( sourceFile, '\\' );
                    if ( baseName != NULL )
                        baseName++;
                    else
                        baseName = sourceFile;
                    I_strncpyz( frame->lineFile, baseName, ASSERT_NAME_SIZE );
                }
            }
        }
    }
}


/* MapFileParseError  0x00401ed0 */
static void MapFileParseError( const char *message )
{
    MessageBoxA( GetActiveWindow(), message, ".map parse error", MB_ICONERROR );
}


/* Assertive_ReadLine  0x00401ef0 */
static char Assertive_ReadLine( FILE *file )
{
    int  i;
    char overflowed;

restart:
    overflowed = 0;

    s_lineLen -= s_linePos;
    memmove( s_lineBuf, &s_lineBuf[s_linePos], s_lineLen );
    s_linePos = 0;

    for ( ;; )
    {
        s_lineLen += fread( &s_lineBuf[s_lineLen], 1, sizeof( s_lineBuf ) - s_lineLen - 1, file );
        s_lineBuf[s_lineLen] = 0;
        if ( s_lineLen == 0 )
            return 0;

        for ( i = 0; i < s_lineLen; i++ )
        {
            if ( s_lineBuf[i] == '\n' )
            {
                s_lineBuf[i] = 0;
                if ( s_lineBuf[i + 1] == '\r' )
                    s_linePos = i + 2;
                else
                    s_linePos = i + 1;

                if ( !overflowed )
                    return 1;
                goto restart;
            }
        }

        overflowed = 1;
        s_lineLen = 0;
    }
}


/* Assertive_SkipLines  0x00402010 */
static char Assertive_SkipLines( int count, FILE *file, int unusedFlag )
{
    int i;

    for ( i = 0; i < count; i++ )
    {
        if ( !Assertive_ReadLine( file ) )
            return 0;
    }

    return 1;
}


/* Assertive_GetModuleHandle  0x00402050 */
static HMODULE Assertive_GetModuleHandle( const char *moduleName )
{
    HMODULE      module;
    unsigned int len;
    int          pos;
    CHAR         name[MAX_PATH];

    len = strlen( moduleName );

    for ( pos = len - 1; pos >= 0; pos-- )
    {
        if ( moduleName[pos] == '.' || moduleName[pos] == '/' || moduleName[pos] == '\\' )
            break;
    }
    if ( pos >= 0 && moduleName[pos] == '.' )
        len = pos;

    memcpy( name, moduleName, len );

    strcpy( &name[len], ".exe" );
    module = GetModuleHandleA( name );
    if ( module == NULL )
    {
        strcpy( &name[len], ".dll" );
        module = GetModuleHandleA( name );
    }

    return module;
}


/* Assertive_Init  0x00402160 */
void Assertive_Init( void )
{
}


/* Assertive_SetHandler  0x00402170 */
void Assertive_SetHandler( AssertHandler_t handler )
{
    s_assertHandler = handler;
}


/* AssertFailed  0x00402180 */
void AssertFailed( const char *file, int line, int type, const char *fmt, ... )
{
    va_list argptr;
    char    doBreak;

    va_start( argptr, fmt );
    _vsnprintf( s_assertMessage, sizeof( s_assertMessage ), fmt, argptr );
    s_assertMessage[sizeof( s_assertMessage ) - 1] = 0;
    va_end( argptr );

    if ( s_assertActive )
    {
        Assertive_CopyToClipboard();
        AssertShowDialog( s_assertType, 1 );

        AssertMessage( s_assertMessage, file, line, type, 1, s_assertText );

        if ( s_assertActive == 1 )
        {
            s_assertActive = 2;
            Com_Printf( CON_CHANNEL_SYSTEM,
                        "ASSERTBEGIN - ( Recursive assert )---------------------------------------------\n" );
            Com_Printf( CON_CHANNEL_SYSTEM, s_assertText );
            Com_Printf( CON_CHANNEL_SYSTEM,
                        "ASSERTEND - ( Recursive assert ) ----------------------------------------------\n\n" );
        }

        exit( -1 );
    }

    s_assertType   = type;
    s_assertActive = 1;

    AssertMessage( s_assertMessage, file, line, type, 1, s_assertText );

    Com_Printf( CON_CHANNEL_SYSTEM,
                "ASSERTBEGIN -------------------------------------------------------------------\n" );
    Com_Printf( CON_CHANNEL_SYSTEM, "%s", s_assertText );
    Com_Printf( CON_CHANNEL_SYSTEM,
                "ASSERTEND ---------------------------------------------------------------------\n" );

    if ( Assertive_ExitOnAssert() )
        ExitProcess( -1 );

    Assertive_CopyToClipboard();

    doBreak = AssertShowDialog( type, 0 );
    s_assertActive = 0;
    if ( doBreak )
        DebugBreak();
}


/* Assertive_CopyToClipboard  0x004022f0 */
static void Assertive_CopyToClipboard( void )
{
    HGLOBAL hMem;
    char   *locked;

    if ( !OpenClipboard( GetDesktopWindow() ) )
        return;

    EmptyClipboard();

    hMem = GlobalAlloc( GMEM_MOVEABLE, strlen( s_assertText ) + 1 );
    if ( hMem != NULL )
    {
        locked = ( char * )GlobalLock( hMem );
        if ( locked != NULL )
        {
            strcpy( locked, s_assertText );
            GlobalUnlock( hMem );
            SetClipboardData( CF_TEXT, hMem );
        }
    }

    CloseClipboard();
}


/* AssertShowDialog  0x00402380 */
static char AssertShowDialog( int type, int recursive )
{
    const char *caption;
    int         answer;

    if ( s_assertHandler != NULL )
        s_assertHandler( s_assertText );

    if ( type == ASSERT_TYPE_ASSERT )
        caption = "ASSERTION FAILURE... (this text is on the clipboard)";
    else if ( type == ASSERT_TYPE_SANITY )
        caption = "SANITY CHECK FAILURE... (this text is on the clipboard)";
    else
        caption = "INTERNAL ERROR";

    answer = MessageBoxA( GetActiveWindow(), s_assertText, caption,
                          MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_DEFBUTTON2 |
                          MB_TASKMODAL | MB_SETFOREGROUND );

    if ( answer == IDABORT )
    {
        if ( recursive == 1 )
            return 1;
        ExitProcess( -1 );
    }
    else if ( answer == IDIGNORE )
    {
        return 0;
    }

    return 1;
}


/* AssertMessage  0x00402420 */
static int AssertMessage( const char *expr, const char *file, int line, int type,
                          int skipFrames, char *buffer )
{
    char *out;
    char  unknown[12];

    strcpy( unknown, "<unknown>" );
    if ( file == NULL )
        file = unknown;
    if ( expr == NULL )
        expr = unknown;

    if ( !GetModuleFileNameA( NULL, s_moduleFileName, MAX_PATH ) )
        strcpy( s_moduleFileName, "<unknown application>" );

    out = buffer;
    out += sprintf( out, "Expression:\n    %s\n\nModule:    %s\nFile:    %s\nLine:    %d\n\n",
                    expr, s_moduleFileName, file, line );

    return Assertive_WalkStack( out, skipFrames + 1, type );
}


/* Assertive_UpdateExitOnAssert  0x004024e0 */
static void Assertive_UpdateExitOnAssert( void )
{
    s_exitOnAssert = 0;
}


/* Assertive_ExitOnAssert  0x004024f0 */
static char Assertive_ExitOnAssert( void )
{
    Assertive_UpdateExitOnAssert();
    return s_exitOnAssert;
}


/* Assertive_OutOfMemory  0x00402500 */
void Assertive_OutOfMemory( const char *file, int line )
{
    Com_Printf( CON_CHANNEL_SYSTEM, "Out of memory: filename '%s', line %d\n", file, line );
    exit( -1 );
}

/* Original: ..\src\universal\q_parse.cpp */

#include "q_shared.h"
#include "assertive.h"
#include "com_shared.h"
#include "q_parse.h"

extern int I_strlen( const char *string );                          /* 0x0040a290 */

/* punctuation  0x00529e50 */
const char *punctuation[] =
{
    "+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
    "&&", "||", "<=", ">=", "==", "!=", NULL
};

/* g_parse  0x00529e90 */
ParseThreadInfo_t g_parse =
{
    {
        {
            { 0 },
            1,
            false,
            true,
            false,
            false,
            true,
            "",
            "",
            1,
            0,
            ""
        }
    }
};

/* Com_BeginParseSession  0x0047d280 */
void Com_BeginParseSession( const char *sessionName )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    int i;

    parse = Com_GetParseThreadInfo();
    if ( parse->parseInfoNum == MAX_PARSE_INFO - 1 )
    {
        Com_Printf( CON_CHANNEL_PARSER, "Already parsing:\n" );
        for ( i = 0; i < parse->parseInfoNum; i++ )
            Com_Printf( CON_CHANNEL_PARSER, "%i. %s\n", i, parse->parseInfo[i].parseFile );
        Com_ErrorLevel( ERR_FATAL, "Com_BeginParseSession: session overflow trying to parse %s\n", sessionName );
    }
    parse->parseInfoNum++;
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Com_InitParseInfo( parseInfo );
    parseInfo->parseFile = sessionName;
}

/* Com_GetParseThreadInfo  0x0047d350 */
ParseThreadInfo_t *Com_GetParseThreadInfo( void )
{
    return &g_parse;
}

/* Com_InitParseInfo  0x0047d360 */
void Com_InitParseInfo( ParseInfo_t *parseInfo )
{
    parseInfo->lines = 1;
    parseInfo->ungetToken = false;
    parseInfo->spaceDelimited = true;
    parseInfo->keepStringQuotes = false;
    parseInfo->csv = false;
    parseInfo->negativeNumbers = false;
    parseInfo->errorPrefix = "";
    parseInfo->warningPrefix = "";
    parseInfo->backup_lines = 0;
    parseInfo->backup_text = 0;
}

/* Com_EndParseSession  0x0047d3e0 */
void Com_EndParseSession( void )
{
    ParseThreadInfo_t *parse;

    parse = Com_GetParseThreadInfo();
    if ( !parse->parseInfoNum )
        Com_ErrorLevel( ERR_FATAL, "Com_EndParseSession: session underflow" );
    parse->parseInfoNum--;
}

/* Com_ResetParseSessions  0x0047d420 */
void Com_ResetParseSessions( void )
{
    ParseThreadInfo_t *parse;

    parse = Com_GetParseThreadInfo();
    parse->parseInfoNum = 0;
}

/* Com_SetSpaceDelimited  0x0047d440 */
void Com_SetSpaceDelimited( int enabled )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    parseInfo->spaceDelimited = enabled != 0;
}

/* Com_SetKeepStringQuotes  0x0047d480 */
void Com_SetKeepStringQuotes( int enabled )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    parseInfo->keepStringQuotes = enabled != 0;
}

/* Com_SetCSV  0x0047d4c0 */
void Com_SetCSV( int enabled )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    parseInfo->csv = enabled != 0;
}

/* Com_SetParseNegativeNumbers  0x0047d500 */
void Com_SetParseNegativeNumbers( int enabled )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    parseInfo->negativeNumbers = enabled != 0;
}

/* Com_GetCurrentParseLine  0x0047d540 */
int Com_GetCurrentParseLine( void )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    return parseInfo->lines;
}

/* Com_SetErrorPrefix  0x0047d570 */
void Com_SetErrorPrefix( const char *prefix )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Assert( prefix );
    Assertx( ( parse->parseInfoNum > 0 ), "(parse->parseInfoNum) = %i",
             parse->parseInfoNum );
    parseInfo->errorPrefix = prefix;
}

/* Com_SetWarningPrefix  0x0047d600 */
void Com_SetWarningPrefix( const char *prefix )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Assert( prefix );
    Assertx( ( parse->parseInfoNum > 0 ), "(parse->parseInfoNum) = %i",
             parse->parseInfoNum );
    parseInfo->warningPrefix = prefix;
}

/* Com_ScriptError  0x0047d690 */
void Com_ScriptError( const char *format, ... )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    char buffer[MAXPRINTMSG];
    va_list argptr;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    va_start( argptr, format );
    _vsnprintf( buffer, sizeof( buffer ), format, argptr );
    va_end( argptr );
    if ( parse->parseInfoNum )
        Com_ErrorLevel( ERR_DROP, "%sFile %s, line %i: %s",
                        parseInfo->errorPrefix, parseInfo->parseFile, parseInfo->lines, buffer );
    else
        Com_ErrorLevel( ERR_DROP, "%s", buffer );
}

/* Com_ScriptWarning  0x0047d760 */
void Com_ScriptWarning( const char *format, ... )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    char buffer[MAXPRINTMSG];
    va_list argptr;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    va_start( argptr, format );
    _vsnprintf( buffer, sizeof( buffer ), format, argptr );
    va_end( argptr );
    if ( parse->parseInfoNum )
        Com_PrintError( CON_CHANNEL_PARSER, "%sFile %s, line %i: %s",
                        parseInfo->warningPrefix, parseInfo->parseFile, parseInfo->lines, buffer );
    else
        Com_PrintError( CON_CHANNEL_PARSER, "%s", buffer );
}

/* Com_UngetToken  0x0047d830 */
void Com_UngetToken( void )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    if ( parseInfo->ungetToken )
        Com_ScriptError( "UngetToken called twice" );
    parseInfo->ungetToken = true;
    parse->tokenPos = parse->prevTokenPos;
}

/* Com_SaveParserState  0x0047d890 */
void Com_SaveParserState( const char **text, ComParseMark_t *mark )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Assert( text );
    Assert( mark );
    mark->lines = parseInfo->lines;
    mark->text = *text;
    mark->ungetToken = parseInfo->ungetToken;
    mark->backup_lines = parseInfo->backup_lines;
    mark->backup_text = parseInfo->backup_text;
}

/* Com_RestoreParserState  0x0047d950 */
void Com_RestoreParserState( const char **text, ComParseMark_t *mark )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Assert( text );
    Assert( mark );
    parseInfo->lines = mark->lines;
    *text = mark->text;
    parseInfo->ungetToken = mark->ungetToken != 0;
    parseInfo->backup_lines = mark->backup_lines;
    parseInfo->backup_text = mark->backup_text;
}

/* COM_Compress  0x0047da10 */
int COM_Compress( char *data_p )
{
    char *in;
    char *out;
    char c;
    int whitespace = 0;
    int count = 0;

    in = out = data_p;
    if ( in )
    {
        while ( ( c = *in ) != 0 )
        {
            if ( c == '\r' || c == '\n' )
            {
                *out++ = c;
                count++;
                whitespace = 0;
                in++;
            }
            else if ( c == '/' && in[1] == '/' )
            {
                while ( *in && *in != '\n' )
                    in++;
                whitespace = 0;
            }
            else if ( c == '/' && in[1] == '*' )
            {
                while ( *in && ( *in != '*' || in[1] != '/' ) )
                {
                    if ( *in == '\n' )
                    {
                        *out++ = '\n';
                        count++;
                    }
                    in++;
                }
                if ( *in )
                    in += 2;
                whitespace = 0;
            }
            else
            {
                if ( whitespace )
                    *out++ = ' ';
                *out++ = c;
                count++;
                in++;
                whitespace = 0;
            }
        }
    }
    *out = 0;
    return count;
}

/* Com_GetCurrentTokenPos  0x0047dbb0 */
const char *Com_GetCurrentTokenPos( void )
{
    ParseThreadInfo_t *parse;

    parse = Com_GetParseThreadInfo();
    return parse->tokenPos;
}

/* COM_Parse  0x0047dbd0 */
char *COM_Parse( const char **data_p )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    if ( parseInfo->ungetToken )
    {
        parseInfo->ungetToken = false;
        *data_p = parseInfo->backup_text;
        parseInfo->lines = parseInfo->backup_lines;
    }
    return ParseToken( data_p, 1 );
}

/* ParseToken  0x0047dc40 */
char *ParseToken( const char **data_p, int crossLine )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    const char *data;
    const char **punc;
    char c;
    int len;
    int puncLen;
    int i;
    int hasNewLines;

    c = 0;
    hasNewLines = 0;
    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    Assert( data_p );
    data = *data_p;
    len = 0;
    parseInfo->token[0] = 0;

    if ( !data )
    {
        *data_p = 0;
        return parseInfo->token;
    }

    parseInfo->backup_lines = parseInfo->lines;
    parseInfo->backup_text = *data_p;

    if ( parseInfo->csv )
        return Com_ParseCSV( data_p, crossLine );

    while ( 1 )
    {
        data = Com_SkipWhitespace( data, &hasNewLines );
        if ( !data )
        {
            *data_p = 0;
            return parseInfo->token;
        }
        if ( hasNewLines && !crossLine )
            return parseInfo->token;
        c = *data;
        if ( c == '/' && data[1] == '/' )
        {
            while ( *data && *data != '\n' )
                data++;
        }
        else if ( c == '/' && data[1] == '*' )
        {
            while ( *data && ( *data != '*' || data[1] != '/' ) )
            {
                if ( *data == '\n' )
                    parseInfo->lines++;
                data++;
            }
            if ( *data )
                data += 2;
        }
        else
        {
            break;
        }
    }

    parse->prevTokenPos = parse->tokenPos;
    parse->tokenPos = data;

    if ( c == '"' )
    {
        if ( parseInfo->keepStringQuotes )
            parseInfo->token[len++] = '"';
        data++;
        while ( 1 )
        {
            c = *data++;
            if ( c == '\\' && ( *data == '"' || *data == '\\' ) )
            {
                c = *data++;
            }
            else
            {
                if ( c == '"' || c == 0 )
                {
                    if ( parseInfo->keepStringQuotes )
                        parseInfo->token[len++] = '"';
                    parseInfo->token[len] = 0;
                    *data_p = data;
                    return parseInfo->token;
                }
                if ( *data == '\n' )
                    parseInfo->lines++;
            }
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = c;
        }
    }

    if ( parseInfo->spaceDelimited )
    {
        do
        {
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = c;
            data++;
            c = *data;
        }
        while ( c > ' ' );
        if ( len == MAX_TOKEN_CHARS )
            len = 0;
        parseInfo->token[len] = 0;
        *data_p = data;
        return parseInfo->token;
    }

    if ( ( c >= '0' && c <= '9' )
      || ( parseInfo->negativeNumbers && c == '-' && data[1] >= '0' && data[1] <= '9' )
      || ( c == '.' && data[1] >= '0' && data[1] <= '9' ) )
    {
        do
        {
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = c;
            data++;
            c = *data;
        }
        while ( ( c >= '0' && c <= '9' ) || c == '.' );

        if ( c == 'e' || c == 'E' )
        {
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = c;
            data++;
            c = *data;
            if ( c == '-' || c == '+' )
            {
                if ( len < MAX_TOKEN_CHARS - 1 )
                    parseInfo->token[len++] = c;
                data++;
                c = *data;
            }
            do
            {
                if ( len < MAX_TOKEN_CHARS - 1 )
                    parseInfo->token[len++] = c;
                data++;
                c = *data;
            }
            while ( c >= '0' && c <= '9' );
        }
        if ( len == MAX_TOKEN_CHARS )
            len = 0;
        parseInfo->token[len] = 0;
        *data_p = data;
        return parseInfo->token;
    }

    if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' )
      || c == '_' || c == '/' || c == '\\' )
    {
        do
        {
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = c;
            data++;
            c = *data;
        }
        while ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' )
             || c == '_' || ( c >= '0' && c <= '9' ) );
        if ( len == MAX_TOKEN_CHARS )
            len = 0;
        parseInfo->token[len] = 0;
        *data_p = data;
        return parseInfo->token;
    }

    for ( punc = punctuation; *punc; punc++ )
    {
        puncLen = I_strlen( *punc );
        for ( i = 0; i < puncLen; i++ )
        {
            if ( data[i] != ( *punc )[i] )
                break;
        }
        if ( i == puncLen )
        {
            memcpy( parseInfo->token, *punc, puncLen );
            parseInfo->token[puncLen] = 0;
            data += puncLen;
            *data_p = data;
            return parseInfo->token;
        }
    }

    parseInfo->token[0] = *data;
    parseInfo->token[1] = 0;
    data++;
    *data_p = data;
    return parseInfo->token;
}

/* Com_SkipWhitespace  0x0047e2f0 */
const char *Com_SkipWhitespace( const char *data, int *hasNewLines )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    int c;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    while ( ( c = *data ) <= ' ' )
    {
        if ( !c )
            return 0;
        if ( c == '\n' )
        {
            parseInfo->lines++;
            *hasNewLines = 1;
        }
        data++;
    }
    return data;
}

/* Com_ParseCSV  0x0047e370 */
char *Com_ParseCSV( const char **data_p, int crossLine )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;
    const char *data;
    const char *out;
    unsigned int len;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    data = *data_p;
    len = 0;
    parseInfo->token[0] = 0;

    if ( crossLine )
    {
        while ( *data == '\r' || *data == '\n' )
            data++;
    }
    else if ( *data == '\r' || *data == '\n' )
    {
        return parseInfo->token;
    }

    parse->prevTokenPos = parse->tokenPos;
    parse->tokenPos = data;

    while ( *data && *data != ',' && *data != '\n' )
    {
        if ( *data == '\r' )
        {
            data++;
        }
        else if ( *data == '"' )
        {
            data++;
            while ( 1 )
            {
                if ( *data == '"' )
                {
                    if ( data[1] == '"' )
                    {
                        if ( len < MAX_TOKEN_CHARS - 1 )
                            parseInfo->token[len++] = '"';
                        data += 2;
                    }
                    else
                    {
                        data++;
                        break;
                    }
                }
                else
                {
                    if ( len < MAX_TOKEN_CHARS - 1 )
                        parseInfo->token[len++] = *data;
                    data++;
                }
            }
        }
        else
        {
            if ( len < MAX_TOKEN_CHARS - 1 )
                parseInfo->token[len++] = *data;
            data++;
        }
    }

    if ( *data && *data != '\n' )
        data++;

    if ( !*data && !len )
        out = 0;
    else
        out = data;
    *data_p = out;
    parseInfo->token[len] = 0;
    return parseInfo->token;
}

/* COM_ParseExt  0x0047e560 */
char *COM_ParseExt( const char **data_p )
{
    ParseThreadInfo_t *parse;
    ParseInfo_t *parseInfo;

    parse = Com_GetParseThreadInfo();
    parseInfo = &parse->parseInfo[parse->parseInfoNum];
    if ( parseInfo->ungetToken )
    {
        parseInfo->ungetToken = false;
        if ( !parseInfo->spaceDelimited )
            return parseInfo->token;
        *data_p = parseInfo->backup_text;
        parseInfo->lines = parseInfo->backup_lines;
    }
    return ParseToken( data_p, 0 );
}

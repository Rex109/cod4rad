
#ifndef Q_PARSE_H
#define Q_PARSE_H

#include "q_shared.h"

#define MAX_PARSE_INFO  16

typedef struct
{
    char        token[MAX_TOKEN_CHARS];  /* +0x000 */
    int         lines;                   /* +0x400 */
    bool        ungetToken;              /* +0x404 */
    bool        spaceDelimited;          /* +0x405 */
    bool        keepStringQuotes;        /* +0x406 */
    bool        csv;                     /* +0x407 */
    bool        negativeNumbers;         /* +0x408 */
    const char *errorPrefix;             /* +0x40c */
    const char *warningPrefix;           /* +0x410 */
    int         backup_lines;            /* +0x414 */
    const char *backup_text;             /* +0x418 */
    const char *parseFile;               /* +0x41c */
} ParseInfo_t;

typedef struct
{
    ParseInfo_t parseInfo[MAX_PARSE_INFO]; /* +0x0000 */
    int         parseInfoNum;              /* +0x4200 */
    const char *tokenPos;                  /* +0x4204 */
    const char *prevTokenPos;              /* +0x4208 */
    char        line[MAX_TOKEN_CHARS];     /* +0x420c */
} ParseThreadInfo_t;

typedef struct
{
    int         lines;         /* +0x00 */
    const char *text;          /* +0x04 */
    int         ungetToken;    /* +0x08 */
    int         backup_lines;  /* +0x0c */
    const char *backup_text;   /* +0x10 */
} ComParseMark_t;

extern ParseThreadInfo_t g_parse;              /* 0x00529e90 */
extern const char       *punctuation[];        /* 0x00529e50 */

ParseThreadInfo_t *Com_GetParseThreadInfo( void );                 /* 0x0047d350 */
void   Com_InitParseInfo( ParseInfo_t *parseInfo );                /* 0x0047d360 */
void   Com_BeginParseSession( const char *sessionName );           /* 0x0047d280 */
void   Com_EndParseSession( void );                                /* 0x0047d3e0 */
void   Com_ResetParseSessions( void );                             /* 0x0047d420 */

void   Com_SetSpaceDelimited( int enabled );                       /* 0x0047d440 */
void   Com_SetKeepStringQuotes( int enabled );                     /* 0x0047d480 */
void   Com_SetCSV( int enabled );                                  /* 0x0047d4c0 */
void   Com_SetParseNegativeNumbers( int enabled );                 /* 0x0047d500 */
void   Com_SetErrorPrefix( const char *prefix );                   /* 0x0047d570 */
void   Com_SetWarningPrefix( const char *prefix );                 /* 0x0047d600 */

int    Com_GetCurrentParseLine( void );                            /* 0x0047d540 */
const char *Com_GetCurrentTokenPos( void );                        /* 0x0047dbb0 */
void   Com_ScriptError( const char *format, ... );                 /* 0x0047d690 */
void   Com_ScriptWarning( const char *format, ... );               /* 0x0047d760 */

void   Com_UngetToken( void );                                     /* 0x0047d830 */
void   Com_SaveParserState( const char **text, ComParseMark_t *mark );     /* 0x0047d890 */
void   Com_RestoreParserState( const char **text, ComParseMark_t *mark );  /* 0x0047d950 */

int    COM_Compress( char *data_p );                               /* 0x0047da10 */
const char *Com_SkipWhitespace( const char *data, int *hasNewLines );      /* 0x0047e2f0 */
char  *Com_ParseCSV( const char **data_p, int crossLine );         /* 0x0047e370 */
char  *ParseToken( const char **data_p, int crossLine );           /* 0x0047dc40 */
char  *COM_Parse( const char **data_p );                           /* 0x0047dbd0 */
char  *COM_ParseExt( const char **data_p );                        /* 0x0047e560 */

#endif

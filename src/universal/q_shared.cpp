/* Original: ..\src\universal\q_shared.cpp */

#include "q_shared.h"
#include "q_parse.h"
#include "com_memory.h"


extern void   AssertFailed(const char *file, int line, int fatal, const char *fmt, ...);   /* 0x00402180 */

extern void   Com_ErrorLevel(int errorLevel, const char *fmt, ...);                        /* 0x00408870 */
extern void   Com_Printf(int channel, const char *fmt, ...);                               /* 0x0040eec0 */
extern qboolean Com_FilterPath(const char *filter, const char *name, int casesensitive);   /* 0x00475fc0 */

extern float  Vec3Dot(const vec3_t v0, const vec3_t v1);                                   /* 0x004052d0 */
extern int    Float_GetBits(const float *value);                                           /* 0x004758d0 */

extern int    I_strlen(const char *string);                                                /* 0x0040a290 */


extern int    FS_Read(void *buffer, int len, int f);                                        /* 0x00469750 */
extern int    FS_Seek(int f, int offset, int origin);                                       /* 0x004699b0 */


#define SRCFILE "..\\src\\universal\\q_shared.cpp"

#ifndef Assert
#define Assert(exp) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s", #exp); } while (0)
#endif

#ifndef Assertx
#define Assertx(exp, fmt, ...) \
    do { if (!(exp)) AssertFailed(SRCFILE, __LINE__, 0, "%s\n\t" fmt, #exp, __VA_ARGS__); } while (0)
#endif

#define ERR_FATAL              0
#define ERR_DROP               1

#define CON_CHANNEL_SYSTEM     16

#define PARSEINFO_SESSION_SIZE      0x420
#define PARSEINFO_CURRENT_INDEX     0x4200
#define PARSEINFO_LINES             0x400
#define PARSEINFO_RESTOFLINE        0x420c


static short            (*_BigShort)(short);            /* 0x2b9817f4 */
static short            (*_LittleShort)(short);         /* 0x2b9817f0 */
static int              (*_BigLong)(int);               /* 0x2b9894f8 */
static int              (*_LittleLong)(int);            /* 0x2b979ae4 */
static unsigned __int64 (*_LittleLong64)(unsigned __int64);  /* 0x2b9817e8 */
static float            (*_LittleFloat)(float);         /* 0x2b979ae0 */
static int              (*_LittleFloatBits)(float);     /* 0x2b9817ec */

static char  g_vaBigBuf[2][BIG_INFO_STRING];            /* 0x2b975ae0 */
static int   g_vaBufferToggle;                          /* 0x2b989514 */

static char  g_vaBuffer[VA_SCRATCH_SIZE];               /* 0x2b9817f8 */
static char  g_vaTerminator;                            /* 0x2b9894f7 */
static char  g_vaCircularBuf[VA_RING_SIZE];             /* 0x2b979ae8 */
static int   g_vaBufferIdx;                             /* 0x2b989510 */

static char  sys_cwd[MAX_OS_PATH_SHORT];                /* 0x2b989520 */
static char  sys_basepath[MAX_OS_PATH_SHORT];           /* 0x2b989620 */

static void *sys_values[8];                             /* 0x2b989720 */

static int   s_assertDisable_ParseConfigStringToStruct; /* 0x00534794 */

static float FloatIdentity(float f);


/* COM_MatchToken  0x0047e5e0 */
int COM_MatchToken(const char **data_p, const char *match, int warnOnly)
{
    const char *token;

    token = COM_Parse(data_p);
    if (!strcmp(token, match))
        return 1;

    if (warnOnly)
        Com_ScriptWarning("MatchToken: got '%s', expected '%s'\n", token, match);
    else
        Com_ScriptError("MatchToken: got '%s', expected '%s'\n", token, match);
    return 0;
}

/* SkipBracedSection  0x0047e650 */
int SkipBracedSection(const char **data_p, int depth, int maxDepth)
{
    const char *token;
    int         level;
    int         hitMax;

    hitMax = 0;
    level = depth;
    do
    {
        token = COM_Parse(data_p);
        if (token[1] == '\0')
        {
            if (token[0] == '{')
            {
                if (level == maxDepth)
                    hitMax = 1;
                else
                    level++;
            }
            else if (token[0] == '}')
            {
                level--;
            }
        }
    }
    while (level != 0 && *data_p != NULL);

    return hitMax;
}

/* SkipRestOfLine  0x0047e6d0 */
void SkipRestOfLine(const char **data_p)
{
    byte       *parseInfo;
    const char *p;
    char        c;

    parseInfo = (byte *)Com_GetParseThreadInfo();
    parseInfo += *(int *)(parseInfo + PARSEINFO_CURRENT_INDEX) * PARSEINFO_SESSION_SIZE;

    p = *data_p;
    if (!p)
        return;

    while (1)
    {
        c = *p;
        if (c == '\0')
        {
            *data_p = p;
            return;
        }
        p++;
        if (c == '\n')
            break;
    }
    ++*(int *)(parseInfo + PARSEINFO_LINES);
    *data_p = p;
}

/* COM_CountRestOfLineTokens  0x0047e750 */
int COM_CountRestOfLineTokens(const char **data_p)
{
    ComParseMark_t mark;
    const char *token;
    int         count;

    Com_SaveParserState(data_p, &mark);
    count = 0;
    while (1)
    {
        token = COM_ParseExt(data_p);
        if (!*token)
            break;
        count++;
    }
    Com_RestoreParserState(data_p, &mark);
    return count;
}

/* COM_ParseRestOfLine  0x0047e7b0 */
char *COM_ParseRestOfLine(const char **data_p)
{
    char *line;
    char *token;

    line = (char *)Com_GetParseThreadInfo() + PARSEINFO_RESTOFLINE;
    line[0] = '\0';
    while (1)
    {
        token = COM_ParseExt(data_p);
        if (!token[0])
            break;
        if (line[0])
            I_strncat(line, MAX_TOKEN_CHARS, " ");
        I_strncat(line, MAX_TOKEN_CHARS, token);
    }
    return line;
}

/* COM_ParseFloatCrossLine  0x0047e830 */
float COM_ParseFloatCrossLine(const char **data_p)
{
    const char *token;

    token = COM_Parse(data_p);
    return (float)atof(token);
}

/* COM_ParseFloat  0x0047e860 */
float COM_ParseFloat(const char **data_p)
{
    const char *token;

    token = COM_ParseExt(data_p);
    return (float)atof(token);
}

/* COM_ParseInt  0x0047e890 */
int COM_ParseInt(const char **data_p)
{
    const char *token;

    token = COM_Parse(data_p);
    return atoi(token);
}

/* COM_ParseIntExt  0x0047e8c0 */
int COM_ParseIntExt(const char **data_p)
{
    const char *token;

    token = COM_ParseExt(data_p);
    return atoi(token);
}

/* COM_Parse1DMatrix  0x0047e8f0 */
void COM_Parse1DMatrix(const char **data_p, int x, float *m)
{
    const char *token;
    int         i;

    COM_MatchToken(data_p, "(", 0);
    for (i = 0; i < x; i++)
    {
        token = COM_Parse(data_p);
        m[i] = (float)atof(token);
    }
    COM_MatchToken(data_p, ")", 0);
}

/* COM_Parse2DMatrix  0x0047e960 */
void COM_Parse2DMatrix(const char **data_p, int y, int x, float *m)
{
    int i;

    COM_MatchToken(data_p, "(", 0);
    for (i = 0; i < y; i++)
        COM_Parse1DMatrix(data_p, x, m + i * x);
    COM_MatchToken(data_p, ")", 0);
}

/* COM_Parse3DMatrix  0x0047e9d0 */
void COM_Parse3DMatrix(const char **data_p, int z, int y, int x, float *m)
{
    int i;

    COM_MatchToken(data_p, "(", 0);
    for (i = 0; i < z; i++)
        COM_Parse2DMatrix(data_p, y, x, m + i * y * x);
    COM_MatchToken(data_p, ")", 0);
}

/* I_ColorIndex  0x0047ea40 */
int I_ColorIndex(char c)
{
    byte index;

    index = (byte)(c - '0');
    if (index >= 10)
        return 7;
    return index;
}


/* COM_SkipPath  0x0047ea70 */
char *COM_SkipPath(const char *pathname)
{
    const char *last;

    last = pathname;
    for (; *pathname; pathname++)
    {
        if (*pathname == '/' || *pathname == '\\')
            last = pathname + 1;
    }
    return (char *)last;
}

/* Com_BuildFilepath  0x0047eac0 */
void Com_BuildFilepath(const char *folder, const char *name, const char *extension,
                       char *path, int maxCharCount)
{
    int folderLen;
    int nameLen;
    int extensionLen;

    Assert(folder);
    Assert(name);
    Assert(extension);
    Assert(path);
    Assert(maxCharCount > 0);

    folderLen = I_strlen(folder);
    nameLen = I_strlen(name);
    extensionLen = I_strlen(extension);

    if (folderLen + nameLen + extensionLen >= maxCharCount)
    {
        Com_ErrorLevel(ERR_DROP, "filepath '%s%s%s' is longer than %i characters",
                       folder, name, extension, maxCharCount - 1);
    }

    memcpy(path, folder, folderLen);
    memcpy(path + folderLen, name, nameLen);
    memcpy(path + folderLen + nameLen, extension, extensionLen + 1);
}

/* COM_GetExtension  0x0047ec40 */
char *COM_GetExtension(const char *filename)
{
    const char *lastDot;

    Assert(filename);

    lastDot = NULL;
    for (; *filename; filename++)
    {
        if (*filename == '.')
            lastDot = filename;
        else if (*filename == '/' || *filename == '\\')
            lastDot = NULL;
    }
    if (!lastDot)
        lastDot = filename;
    return (char *)lastDot;
}

/* COM_StripExtension  0x0047ecd0 */
void COM_StripExtension(const char *in, char *out)
{
    const char *end;

    end = COM_GetExtension(in);
    for (; in != end; in++)
    {
        *out = *in;
        out++;
    }
    *out = '\0';
}

/* COM_StripFilename  0x0047ed20 */
void COM_StripFilename(const char *in, char *out)
{
    const char *name;

    name = COM_SkipPath(in);
    memcpy(out, in, name - in);
    out[name - in] = '\0';
}

/* COM_DefaultExtension  0x0047ed60 */
void COM_DefaultExtension(char *path, int maxSize, const char *extension)
{
    char        oldPath[MAX_QPATH];
    const char *p;

    p = path + (strlen(path) - 1);
    while (*p != '/' && p != path)
    {
        if (*p == '.')
            return;
        p--;
    }

    I_strncpyz(oldPath, path, sizeof(oldPath));
    Com_sprintf(path, maxSize, "%s%s", oldPath, extension);
}


/* BigShort  0x0047edf0 */
short BigShort(short l)
{
    return _BigShort(l);
}

/* BigLong  0x0047ee10 */
int BigLong(int l)
{
    return _BigLong(l);
}

/* LittleLong64  0x0047ee30 */
unsigned __int64 LittleLong64(unsigned __int64 l)
{
    return _LittleLong64(l);
}

/* LittleShort  0x0047ee50 */
short LittleShort(short l)
{
    return _LittleShort(l);
}

/* LittleLong  0x0047ee70 */
int LittleLong(int l)
{
    return _LittleLong(l);
}

/* LittleFloat  0x0047ee90 */
float LittleFloat(float f)
{
    return _LittleFloat(f);
}

/* LittleFloatBits  0x0047eeb0 */
int LittleFloatBits(float f)
{
    return _LittleFloatBits(f);
}

/* BigFloat  0x0047eed0 */
float BigFloat(float f)
{
    return _LittleFloat(f);
}

/* BigFloatBits  0x0047eef0 */
int BigFloatBits(float f)
{
    return _LittleFloatBits(f);
}

/* ShortSwap  0x0047ef10 */
short ShortSwap(short l)
{
    byte b0, b1;

    b0 = (byte)(l & 0xff);
    b1 = (byte)((l >> 8) & 0xff);
    return (short)((b0 << 8) + b1);
}

/* ShortNoSwap  0x0047ef50 */
short ShortNoSwap(short l)
{
    return l;
}

/* LongSwap  0x0047ef60 */
int LongSwap(int l)
{
    byte b0, b1, b2, b3;

    b0 = (byte)(l & 0xff);
    b1 = (byte)((l >> 8) & 0xff);
    b2 = (byte)((l >> 16) & 0xff);
    b3 = (byte)((l >> 24) & 0xff);
    return (b0 << 24) + (b1 << 16) + (b2 << 8) + b3;
}

/* LongNoSwap  0x0047efc0 */
int LongNoSwap(int l)
{
    return l;
}

/* Long64Swap  0x0047efd0 */
unsigned __int64 Long64Swap(unsigned __int64 ll)
{
    byte b0, b1, b2, b3, b4, b5, b6, b7;

    b0 = (byte)(ll & 0xff);
    b1 = (byte)((ll >> 8) & 0xff);
    b2 = (byte)((ll >> 16) & 0xff);
    b3 = (byte)((ll >> 24) & 0xff);
    b4 = (byte)((ll >> 32) & 0xff);
    b5 = (byte)((ll >> 40) & 0xff);
    b6 = (byte)((ll >> 48) & 0xff);
    b7 = (byte)((ll >> 56) & 0xff);

    return ((unsigned __int64)b0 << 56)
         + ((unsigned __int64)b1 << 48)
         + ((unsigned __int64)b2 << 40)
         + ((unsigned __int64)b3 << 32)
         + ((unsigned __int64)b4 << 24)
         + ((unsigned __int64)b5 << 16)
         + ((unsigned __int64)b6 << 8)
         + (unsigned __int64)b7;
}

/* Long64NoSwap  0x0047f120 */
unsigned __int64 Long64NoSwap(unsigned __int64 ll)
{
    return ll;
}

/* FloatSwap  0x0047f130 */
float FloatSwap(float f)
{
    union { float f; int i; byte b[4]; } in, out;

    in.i = *(int *)&f;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];
    return out.f;
}

/* FloatNoSwap  0x0047f160 */
float FloatNoSwap(float f)
{
    return FloatIdentity(f);
}

/* FloatSwapBits  0x0047f180 */
int FloatSwapBits(float f)
{
    union { float f; int i; byte b[4]; } in, out;

    in.f = f;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];
    return out.i;
}

/* FloatNoSwapBits  0x0047f1b0 */
int FloatNoSwapBits(float f)
{
    return Float_GetBits(&f);
}

/* Swap_InitLittleEndian  0x0047f1d0 */
static void Swap_InitLittleEndian(void)
{
    _BigShort = ShortSwap;
    _LittleShort = ShortNoSwap;
    _BigLong = LongSwap;
    _LittleLong = LongNoSwap;
    _LittleLong64 = Long64NoSwap;
    _LittleFloat = FloatNoSwap;
    _LittleFloatBits = FloatNoSwapBits;
}

/* Swap_InitBigEndian  0x0047f220 */
static void Swap_InitBigEndian(void)
{
    _BigShort = ShortNoSwap;
    _LittleShort = ShortSwap;
    _BigLong = LongNoSwap;
    _LittleLong = LongSwap;
    _LittleLong64 = Long64Swap;
    _LittleFloat = FloatSwap;
    _LittleFloatBits = FloatSwapBits;
}

/* Swap_Init  0x0047f270 */
void Swap_Init(void)
{
    byte swaptest[2] = { 1, 0 };

    if (*(short *)swaptest == 1)
        Swap_InitLittleEndian();
    else
        Swap_InitBigEndian();
}


/* Q_isprint  0x0047f2a0 */
qboolean Q_isprint(int c)
{
    if (c < 0x20 || c > 0x7e)
        return qfalse;
    return qtrue;
}

/* Q_islower  0x0047f2d0 */
qboolean Q_islower(int c)
{
    if (c < 'a' || c > 'z')
        return qfalse;
    return qtrue;
}

/* Q_isupper  0x0047f300 */
qboolean Q_isupper(int c)
{
    if (c < 'A' || c > 'Z')
        return qfalse;
    return qtrue;
}

/* Q_isalpha  0x0047f330 */
qboolean Q_isalpha(int c)
{
    if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
        return qfalse;
    return qtrue;
}

/* Q_isnumeric  0x0047f370 */
qboolean Q_isnumeric(int c)
{
    if (c < '0' || c > '9')
        return qfalse;
    return qtrue;
}

/* Q_isalphanumeric  0x0047f3a0 */
qboolean Q_isalphanumeric(int c)
{
    if (!Q_isalpha(c) && !Q_isnumeric(c))
        return qfalse;
    return qtrue;
}

/* Q_isforfilename  0x0047f3f0 */
qboolean Q_isforfilename(int c)
{
    if (!Q_isalphanumeric(c) && c != '_' && c != '-')
        return qfalse;
    return qtrue;
}


/* I_strncpyz  0x0047f430 */
void I_strncpyz(char *dest, const char *src, int destsize)
{
    Assert(src);
    Assert(dest);
    Assertx((destsize >= 1), "(destsize) = %i", destsize);

    strncpy(dest, src, destsize - 1);
    dest[destsize - 1] = '\0';
}

/* Q_stricmpn  0x0047f4d0 */
int Q_stricmpn(const char *s0, const char *s1, int n)
{
    int c0;
    int c1;

    while (1)
    {
        c0 = *(const byte *)s0;
        c1 = *(const byte *)s1;
        s0++;
        s1++;

        if (!n--)
            return 0;

        if (c0 != c1)
        {
            if (Q_isupper(c0))
                c0 += 'a' - 'A';
            if (Q_isupper(c1))
                c1 += 'a' - 'A';
            if (c0 != c1)
                return 2 * (c0 >= c1) - 1;
        }

        if (!c0)
            return 0;
    }
}

/* Q_strncmp  0x0047f580 */
int Q_strncmp(const char *s0, const char *s1, int n)
{
    char c0;
    char c1;

    while (1)
    {
        c0 = *s0;
        s0++;
        c1 = *s1;
        s1++;

        if (!n--)
            return 0;
        if (c0 != c1)
            break;
        if (!c0)
            return 0;
    }
    return 2 * (c0 >= c1) - 1;
}

/* Q_stristr  0x0047f5f0 */
const char *Q_stristr(const char *s0, const char *substr)
{
    int i;
    int j;

    Assert(s0);
    Assert(substr);

    i = 0;
    while (1)
    {
        if (s0[i] == '\0')
            return NULL;

        j = -1;
        do
        {
            j++;
            if (substr[j] == '\0')
                return s0 + i;
        }
        while (tolower(s0[i + j]) == tolower(substr[j]));

        i++;
    }
}

/* Q_stricmp  0x0047f6d0 */
int Q_stricmp(const char *s0, const char *s1)
{
    Assert(s0);
    Assert(s1);
    return Q_stricmpn(s0, s1, 0x7fffffff);
}

/* Q_strcmp  0x0047f740 */
int Q_strcmp(const char *s0, const char *s1)
{
    Assert(s0);
    Assert(s1);
    return Q_strncmp(s0, s1, 0x7fffffff);
}

/* I_stricmpWild  0x0047f7b0 */
int I_stricmpWild(const char *wild, const char *s)
{
    char wc;
    char sc;
    int  d0;
    int  d1;

    Assert(wild);
    Assert(s);

    while (1)
    {
        wc = *wild;
        wild++;

        if (wc == '*')
        {
            if (*wild == '\0')
                return 0;
            if (*s != '\0' && !I_stricmpWild(wild - 1, s + 1))
                return 0;
        }
        else
        {
            sc = *s;
            s++;
            if (wc != sc && wc != '?')
            {
                d0 = tolower(wc);
                d1 = tolower(sc);
                if (d0 != d1)
                    return 2 * (d0 - d1 >= 0) - 1;
            }
        }

        if (wc == '\0')
            return 0;
    }
}

/* Q_strlwr  0x0047f8d0 */
char *Q_strlwr(char *s)
{
    char *p;

    for (p = s; *p; p++)
    {
        if (Q_isupper(*p))
            *p += 'a' - 'A';
    }
    return s;
}

/* Q_strupr  0x0047f920 */
char *Q_strupr(char *s)
{
    char *p;

    for (p = s; *p; p++)
    {
        if (Q_islower(*p))
            *p -= 'a' - 'A';
    }
    return s;
}

/* Q_isidentifierchar  0x0047f970 */
qboolean Q_isidentifierchar(int c)
{
    if (c >= 'a' && c <= 'z')
        return qtrue;
    if (c >= 'A' && c <= 'Z')
        return qtrue;
    if (c >= '0' && c <= '9')
        return qtrue;
    if (c == '_')
        return qtrue;
    return qfalse;
}

/* I_strncat  0x0047f9c0 */
void I_strncat(char *dest, int size, const char *src)
{
    int len;

    Assert(size != sizeof( char * ));

    len = I_strlen(dest);
    if (len >= size)
        Com_ErrorLevel(ERR_FATAL, "I_strncat: already overflowed");

    I_strncpyz(dest + len, src, size - len);
}

/* Q_PrintStrlen  0x0047fa30 */
int Q_PrintStrlen(const char *string)
{
    const char *p;
    int         len;

    p = string;
    len = 0;
    while (*p)
    {
        if (p && *p == '^' && p[1] != '\0' && p[1] != '^' && p[1] >= '0' && p[1] <= '9')
        {
            p += 2;
        }
        else
        {
            len++;
            p++;
        }
    }
    return len;
}

/* Com_sprintf  0x0047fac0 */
int Com_sprintf(char *dest, size_t size, const char *fmt, ...)
{
    int     len;
    va_list argptr;

    va_start(argptr, fmt);
    len = _vsnprintf(dest, size, fmt, argptr);
    va_end(argptr);
    dest[size - 1] = '\0';
    return len;
}

/* Com_sprintfAppend  0x0047fb00 */
int Com_sprintfAppend(char *dest, int size, int *offset, const char *fmt, ...)
{
    char   *at;
    int     space;
    int     len;
    va_list argptr;

    if (*offset >= size - 1)
        return -1;

    at = dest + *offset;
    space = size - *offset;

    va_start(argptr, fmt);
    len = _vsnprintf(at, space, fmt, argptr);
    va_end(argptr);
    at[space - 1] = '\0';

    if (len == space || len == -1)
        *offset = size - 1;
    else
        *offset += len;

    return len;
}

/* CanKeepStringPointer  0x0047fb90 */
int CanKeepStringPointer(const void *ptr)
{
    char stackLocal[4];

    if (ptr >= (const void *)stackLocal &&
        ptr < (const void *)(stackLocal + STACK_CHECK_SIZE))
        return 0;

    if (ptr >= (const void *)g_vaCircularBuf &&
        ptr < (const void *)(g_vaCircularBuf + VA_RING_SIZE))
        return 0;

    return 1;
}

#ifndef Q_LEGACY_THREE_DIGIT_EXPONENT
#define Q_LEGACY_THREE_DIGIT_EXPONENT 1
#endif

#if Q_LEGACY_THREE_DIGIT_EXPONENT
static int Q_PadExponentsToThreeDigits(char *buf, int len, int capacity)
{
    int i;

    for (i = 1; i < len; i++)
    {
        if (buf[i] != 'e' && buf[i] != 'E')
            continue;
        if (buf[i - 1] < '0' || buf[i - 1] > '9')
            continue;
        if (i + 3 >= len)
            continue;
        if (buf[i + 1] != '+' && buf[i + 1] != '-')
            continue;
        if (buf[i + 2] < '0' || buf[i + 2] > '9')
            continue;
        if (buf[i + 3] < '0' || buf[i + 3] > '9')
            continue;
        if (i + 4 < len && buf[i + 4] >= '0' && buf[i + 4] <= '9')
            continue;
        if (len + 2 > capacity)
            return len;

        memmove(buf + i + 3, buf + i + 2, (size_t)(len - (i + 2) + 1));
        buf[i + 2] = '0';
        len++;
        i += 4;
    }

    return len;
}
#endif


/* va  0x0047fbd0 */
char *va(const char *format, ...)
{
    int     len;
    char   *buf;
    va_list argptr;

    va_start(argptr, format);
    len = _vsnprintf(g_vaBuffer, VA_RING_SIZE, format, argptr);
    va_end(argptr);
    g_vaTerminator = '\0';

#if Q_LEGACY_THREE_DIGIT_EXPONENT
    if (len >= 0 && len < VA_SCRATCH_SIZE)
        len = Q_PadExponentsToThreeDigits(g_vaBuffer, len, VA_SCRATCH_SIZE);
#endif

    if (len < 0 || len >= VA_RING_SIZE)
        Com_ErrorLevel(ERR_DROP, "Attempted to overrun string in call to va()\n");

    if (len + g_vaBufferIdx >= VA_SCRATCH_SIZE)
        g_vaBufferIdx = 0;

    buf = &g_vaCircularBuf[g_vaBufferIdx];
    memcpy(buf, g_vaBuffer, len + 1);
    g_vaBufferIdx += len + 1;
    return buf;
}


/* Info_ValueForKey  0x0047fc80 */
const char *Info_ValueForKey(const char *s, const char *key)
{
    char  pkey[BIG_INFO_STRING];
    char *o;
    char *value;

    if (!s || !key)
        return "";

    g_vaBufferToggle ^= 1;

    if (*s == '\\')
        s++;

    while (1)
    {
        o = pkey;
        while (*s != '\\')
        {
            if (!*s)
                return "";
            *o = *s;
            o++;
            s++;
            if (o - pkey >= BIG_INFO_STRING)
                Com_ErrorLevel(ERR_DROP, "Info_ValueForKey: oversize key %d", o - pkey);
        }
        *o = '\0';
        s++;

        value = g_vaBigBuf[g_vaBufferToggle];
        o = value;
        while (*s != '\\' && *s != '\0')
        {
            *o = *s;
            o++;
            s++;
            if (o - value >= BIG_INFO_STRING)
                Com_ErrorLevel(ERR_DROP, "Info_ValueForKey: oversize key %d", o - value);
        }
        *o = '\0';

        if (!Q_stricmp(key, pkey))
            return value;
        if (!*s)
            return "";
        s++;
    }
}

/* Info_NextPair  0x0047fe50 */
void Info_NextPair(const char **head, char *key, char *value)
{
    const char *s;
    char       *o;

    s = *head;
    if (*s == '\\')
        s++;

    key[0] = '\0';
    value[0] = '\0';

    o = key;
    while (1)
    {
        if (*s == '\\')
        {
            *o = '\0';
            o = value;
            while (1)
            {
                s++;
                if (*s == '\\' || *s == '\0')
                    break;
                *o = *s;
                o++;
            }
            *o = '\0';
            *head = s;
            return;
        }
        if (*s == '\0')
            break;
        *o = *s;
        o++;
        s++;
    }
    *o = '\0';
    *head = s;
}

/* Info_RemoveKey  0x0047ff30 */
void Info_RemoveKey(char *s, const char *key)
{
    char  pkey[MAX_INFO_STRING];
    char  value[MAX_INFO_STRING];
    char *o;
    char *start;

    if (strlen(s) >= MAX_INFO_STRING)
        Com_ErrorLevel(ERR_DROP, "Info_RemoveKey: oversize infostring");

    if (strchr(key, '\\'))
        return;

    do
    {
        start = s;
        if (*s == '\\')
            s++;

        o = pkey;
        while (*s != '\\')
        {
            if (*s == '\0')
                return;
            *o = *s;
            o++;
            s++;
        }
        *o = '\0';

        o = value;
        while (1)
        {
            s++;
            if (*s == '\\' || *s == '\0')
                break;
            if (*s == '\0')
                return;
            *o = *s;
            o++;
        }
        *o = '\0';

        if (!strcmp(key, pkey))
        {
            strcpy(start, s);
            return;
        }
    }
    while (*s != '\0');
}

/* Info_RemoveKey_Big  0x004800a0 */
void Info_RemoveKey_Big(char *s, const char *key)
{
    char  pkey[BIG_INFO_STRING];
    char  value[BIG_INFO_STRING];
    char *o;
    char *start;

    if (strlen(s) >= BIG_INFO_STRING)
        Com_ErrorLevel(ERR_DROP, "Info_RemoveKey_Big: oversize infostring");

    if (strchr(key, '\\'))
        return;

    do
    {
        start = s;
        if (*s == '\\')
            s++;

        o = pkey;
        while (*s != '\\')
        {
            if (*s == '\0')
                return;
            *o = *s;
            o++;
            s++;
        }
        *o = '\0';

        o = value;
        while (1)
        {
            s++;
            if (*s == '\\' || *s == '\0')
                break;
            if (*s == '\0')
                return;
            *o = *s;
            o++;
        }
        *o = '\0';

        if (!strcmp(key, pkey))
        {
            strcpy(start, s);
            return;
        }
    }
    while (*s != '\0');
}

/* Info_Validate  0x00480220 */
qboolean Info_Validate(const char *s)
{
    if (strchr(s, '"'))
        return qfalse;
    if (strchr(s, ';'))
        return qfalse;
    return qtrue;
}

/* Info_SetValueForKey  0x00480260 */
void Info_SetValueForKey(char *s, const char *key, const char *value)
{
    char cleanValue[MAX_INFO_STRING];
    char newi[MAX_INFO_STRING];
    int  len;
    int  i;
    int  j;
    char c;

    Assert(value);

    if (strlen(s) >= MAX_INFO_STRING)
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Info_SetValueForKey: oversize infostring");
        return;
    }

    j = 0;
    for (i = 0; i < MAX_INFO_STRING - 1; i++)
    {
        c = value[i];
        if (c == '\0')
            break;
        if (c != '\\' && c != ';' && c != '"')
        {
            Assert(j < MAX_INFO_STRING);
            cleanValue[j] = c;
            j++;
        }
    }
    Assert(j < MAX_INFO_STRING);
    cleanValue[j] = '\0';

    if (strchr(key, '\\'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }
    if (strchr(key, ';'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }
    if (strchr(key, '"'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }

    Info_RemoveKey(s, key);
    if (cleanValue[0] == '\0')
        return;

    len = Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, cleanValue);
    if (len < 1)
    {
        Com_Printf(CON_CHANNEL_SYSTEM,
                   "Info buffer length exceeded, not including key/value pair in response\n");
        return;
    }

    if (strlen(newi) + strlen(s) <= MAX_INFO_STRING)
        strcat(s, newi);
    else
        Com_Printf(CON_CHANNEL_SYSTEM,
                   "Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n",
                   key, value, s);
}

/* Info_SetValueForKey_Big  0x00480510 */
void Info_SetValueForKey_Big(char *s, const char *key, const char *value)
{
    char cleanValue[BIG_INFO_STRING];
    char newi[BIG_INFO_STRING];
    int  len;
    int  i;
    int  j;
    char c;

    Assert(value);

    if (strlen(s) >= BIG_INFO_STRING)
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Info_SetValueForKey: oversize infostring");
        return;
    }

    j = 0;
    for (i = 0; i < BIG_INFO_STRING - 1; i++)
    {
        c = value[i];
        if (c == '\0')
            break;
        if (c != '\\' && c != ';' && c != '"')
        {
            Assert(j < BIG_INFO_STRING);
            cleanValue[j] = c;
            j++;
        }
    }
    Assert(j < BIG_INFO_STRING);
    cleanValue[j] = '\0';

    if (strchr(key, '\\'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }
    if (strchr(key, ';'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }
    if (strchr(key, '"'))
    {
        Com_Printf(CON_CHANNEL_SYSTEM, "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'", key, value);
        return;
    }

    Info_RemoveKey_Big(s, key);
    if (cleanValue[0] == '\0')
        return;

    len = Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, cleanValue);
    if (len < 1)
    {
        Com_Printf(CON_CHANNEL_SYSTEM,
                   "Info buffer length exceeded, not including key/value pair in response\n");
        return;
    }

    if (strlen(newi) + strlen(s) <= MAX_INFO_STRING)
        strcat(s, newi);
    else
        Com_Printf(CON_CHANNEL_SYSTEM,
                   "Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n",
                   key, value, s);
}


/* ParseConfigStringToStructInternal  0x004807f0 */
static bool ParseConfigStringToStructInternal(byte *pStruct, const cspField_t *pFieldList, int numFields,
                                              const char *pConfigString, int numSpecialFieldTypes,
                                              bool (*parseSpecialFieldType)(byte *, const char *, int),
                                              void (*parseDefaultFieldType)(byte *, const char *))
{
    const cspField_t *field;
    const char       *value;
    int               i;

    i = 0;
    field = pFieldList;
    while (1)
    {
        if (i >= numFields)
            return i == numFields;

        value = Info_ValueForKey(pConfigString, field->szName);
        if (*value)
        {
            if (field->iFieldType < 8)
            {
                switch (field->iFieldType)
                {
                case CS_FIELD_CALLBACK:
                    parseDefaultFieldType(pStruct + field->iOffset, value);
                    break;

                case CS_FIELD_STRING_1024:
                    I_strncpyz((char *)(pStruct + field->iOffset), value, 1024);
                    break;

                case CS_FIELD_STRING_64:
                    I_strncpyz((char *)(pStruct + field->iOffset), value, 64);
                    break;

                case CS_FIELD_STRING_256:
                    I_strncpyz((char *)(pStruct + field->iOffset), value, 256);
                    break;

                case CS_FIELD_INT:
                    *(int *)(pStruct + field->iOffset) = atoi(value);
                    break;

                case CS_FIELD_BOOL:
                    *(unsigned int *)(pStruct + field->iOffset) = (atoi(value) != 0);
                    break;

                case CS_FIELD_FLOAT:
                    *(float *)(pStruct + field->iOffset) = (float)atof(value);
                    break;

                case CS_FIELD_FLOAT_TO_INT:
                    *(int *)(pStruct + field->iOffset) = (int)(float)atof(value);
                    break;

                default:
                    if (field->iFieldType < 0)
                    {
                        if (!s_assertDisable_ParseConfigStringToStruct)
                            AssertFailed(SRCFILE, __LINE__, 0,
                                         va("Negative field type %i given to ParseConfigStringToStruct\n",
                                            field->iFieldType));
                    }
                    else if (!s_assertDisable_ParseConfigStringToStruct)
                    {
                        AssertFailed(SRCFILE, __LINE__, 0,
                                     "ParseConfigStringToStruct is out of sync with the csParseFieldType_t enum list\n");
                    }
                    break;
                }
            }
            else if (numSpecialFieldTypes < 1 || field->iFieldType >= numSpecialFieldTypes)
            {
                if (!s_assertDisable_ParseConfigStringToStruct)
                    AssertFailed(SRCFILE, __LINE__, 0, va("Bad field type %i\n", field->iFieldType));
                Com_ErrorLevel(ERR_DROP, "Bad field type %i\n", field->iFieldType);
            }
            else
            {
                Assert(parseSpecialFieldType != NULL);
                if (!parseSpecialFieldType(pStruct, value, field->iFieldType))
                    return false;
            }
        }

        i++;
        field++;
    }
}

/* ParseConfigStringToStruct  0x004807c0 */
bool ParseConfigStringToStruct(byte *pStruct, const cspField_t *pFieldList, int numFields,
                               const char *pConfigString, int numSpecialFieldTypes,
                               bool (*parseSpecialFieldType)(byte *, const char *, int),
                               void (*parseDefaultFieldType)(byte *, const char *))
{
    return ParseConfigStringToStructInternal(pStruct, pFieldList, numFields, pConfigString,
                                             numSpecialFieldTypes, parseSpecialFieldType,
                                             parseDefaultFieldType);
}


/* TransformPoint  0x00480ac0 */
void TransformPoint(const orientation_t *orient, const vec3_t pos, vec3_t out)
{
    Assert(pos != out);

    out[0] = orient->origin[0] + pos[0] * orient->axis[0][0] + pos[1] * orient->axis[1][0] + pos[2] * orient->axis[2][0];
    out[1] = orient->origin[1] + pos[0] * orient->axis[0][1] + pos[1] * orient->axis[1][1] + pos[2] * orient->axis[2][1];
    out[2] = orient->origin[2] + pos[0] * orient->axis[0][2] + pos[1] * orient->axis[1][2] + pos[2] * orient->axis[2][2];
}

/* TransformDir  0x00480b90 */
void TransformDir(const orientation_t *orient, const vec3_t dir, vec3_t out)
{
    Assert(dir != out);

    out[0] = dir[0] * orient->axis[0][0] + dir[1] * orient->axis[1][0] + dir[2] * orient->axis[2][0];
    out[1] = dir[0] * orient->axis[0][1] + dir[1] * orient->axis[1][1] + dir[2] * orient->axis[2][1];
    out[2] = dir[0] * orient->axis[0][2] + dir[1] * orient->axis[1][2] + dir[2] * orient->axis[2][2];
}

/* TransformPoint4  0x00480c50 */
void TransformPoint4(const orientation_t *orient, const vec4_t pos, vec4_t out)
{
    Assert(pos != out);

    out[0] = pos[3] * orient->origin[0] + pos[0] * orient->axis[0][0] + pos[1] * orient->axis[1][0] + pos[2] * orient->axis[2][0];
    out[1] = pos[3] * orient->origin[1] + pos[0] * orient->axis[0][1] + pos[1] * orient->axis[1][1] + pos[2] * orient->axis[2][1];
    out[2] = pos[3] * orient->origin[2] + pos[0] * orient->axis[0][2] + pos[1] * orient->axis[1][2] + pos[2] * orient->axis[2][2];
    out[3] = pos[3];
}

/* TransformPlane  0x00480d40 */
void TransformPlane(const orientation_t *orient, const vec4_t plane, vec4_t out)
{
    Assert(plane != out);

    TransformDir(orient, plane, out);
    out[3] = Vec3Dot(orient->origin, out) + plane[3];
}

/* InverseTransformPoint  0x00480da0 */
void InverseTransformPoint(const orientation_t *orient, const vec3_t pos, vec3_t out)
{
    float delta0;
    float delta1;
    float delta2;

    Assert(pos != out);

    delta0 = pos[0] - orient->origin[0];
    delta1 = pos[1] - orient->origin[1];
    delta2 = pos[2] - orient->origin[2];

    out[0] = delta0 * orient->axis[0][0] + delta1 * orient->axis[0][1] + delta2 * orient->axis[0][2];
    out[1] = delta0 * orient->axis[1][0] + delta1 * orient->axis[1][1] + delta2 * orient->axis[1][2];
    out[2] = delta0 * orient->axis[2][0] + delta1 * orient->axis[2][1] + delta2 * orient->axis[2][2];
}

/* InverseTransformDir  0x00480e70 */
void InverseTransformDir(const orientation_t *orient, const vec3_t dir, vec3_t out)
{
    Assert(dir != out);

    out[0] = dir[0] * orient->axis[0][0] + dir[1] * orient->axis[0][1] + dir[2] * orient->axis[0][2];
    out[1] = dir[0] * orient->axis[1][0] + dir[1] * orient->axis[1][1] + dir[2] * orient->axis[1][2];
    out[2] = dir[0] * orient->axis[2][0] + dir[1] * orient->axis[2][1] + dir[2] * orient->axis[2][2];
}

/* InverseTransformPoint4  0x00480f30 */
void InverseTransformPoint4(const orientation_t *orient, const vec4_t pos, vec4_t out)
{
    float delta0;
    float delta1;
    float delta2;

    Assert(pos != out);

    delta0 = pos[0] - orient->origin[0] * pos[3];
    delta1 = pos[1] - orient->origin[1] * pos[3];
    delta2 = pos[2] - orient->origin[2] * pos[3];

    out[0] = delta0 * orient->axis[0][0] + delta1 * orient->axis[0][1] + delta2 * orient->axis[0][2];
    out[1] = delta0 * orient->axis[1][0] + delta1 * orient->axis[1][1] + delta2 * orient->axis[1][2];
    out[2] = delta0 * orient->axis[2][0] + delta1 * orient->axis[2][1] + delta2 * orient->axis[2][2];
    out[3] = pos[3];
}

/* TransformOrientation  0x00481020 */
void TransformOrientation(const orientation_t *orFirst, const orientation_t *orSecond, orientation_t *out)
{
    Assert(out != orFirst);
    Assert(out != orSecond);

    TransformDir(orSecond, orFirst->axis[0], out->axis[0]);
    TransformDir(orSecond, orFirst->axis[1], out->axis[1]);
    TransformDir(orSecond, orFirst->axis[2], out->axis[2]);
    TransformPoint(orSecond, orFirst->origin, out->origin);
}

/* InverseTransformPlane  0x004810e0 */
void InverseTransformPlane(const orientation_t *orient, const vec4_t plane, vec4_t out)
{
    Assert(plane != out);

    InverseTransformDir(orient, plane, out);
    out[3] = plane[3] - Vec3Dot(orient->origin, plane);
}

/* TransformPointScaled  0x00481140 */
void TransformPointScaled(const orientation_t *orient, float scale, const vec3_t pos, vec3_t out)
{
    Assert(pos != out);

    out[0] = (pos[0] * orient->axis[0][0] + pos[1] * orient->axis[1][0] + pos[2] * orient->axis[2][0]) * scale + orient->origin[0];
    out[1] = (pos[0] * orient->axis[0][1] + pos[1] * orient->axis[1][1] + pos[2] * orient->axis[2][1]) * scale + orient->origin[1];
    out[2] = (pos[0] * orient->axis[0][2] + pos[1] * orient->axis[1][2] + pos[2] * orient->axis[2][2]) * scale + orient->origin[2];
}

/* TransformPlaneScaled  0x00481210 */
void TransformPlaneScaled(const orientation_t *orient, float scale, const vec4_t plane, vec4_t out)
{
    Assert(plane != out);

    TransformDir(orient, plane, out);
    out[3] = plane[3] * scale + Vec3Dot(orient->origin, out);
}

/* InverseTransformPointScaled  0x00481280 */
void InverseTransformPointScaled(const orientation_t *orient, float scale, const vec3_t pos, vec3_t out)
{
    float delta0;
    float delta1;
    float delta2;
    float oneOverScale;

    Assert(pos != out);
    Assert(scale);

    delta0 = pos[0] - orient->origin[0];
    delta1 = pos[1] - orient->origin[1];
    delta2 = pos[2] - orient->origin[2];
    oneOverScale = 1.0f / scale;

    out[0] = (delta0 * orient->axis[0][0] + delta1 * orient->axis[0][1] + delta2 * orient->axis[0][2]) * oneOverScale;
    out[1] = (delta0 * orient->axis[1][0] + delta1 * orient->axis[1][1] + delta2 * orient->axis[1][2]) * oneOverScale;
    out[2] = (delta0 * orient->axis[2][0] + delta1 * orient->axis[2][1] + delta2 * orient->axis[2][2]) * oneOverScale;
}

/* InverseTransformPlaneScaled  0x00481390 */
void InverseTransformPlaneScaled(const orientation_t *orient, float scale, const vec4_t plane, vec4_t out)
{
    Assert(plane != out);

    InverseTransformDir(orient, plane, out);
    out[3] = (plane[3] - Vec3Dot(orient->origin, plane)) / scale;
}


/* Com_IsXModelPath  0x004813f0 */
qboolean Com_IsXModelPath(const char *path)
{
    if (!Q_stricmpn(path, "xmodel", 6) && (path[6] == '/' || path[6] == '\\'))
        return qtrue;
    return qfalse;
}

/* FloatIdentity  0x00481440 */
static float FloatIdentity(float f)
{
    return f;
}

/* Sys_Mkdir  0x00481450 */
int Sys_Mkdir(const char *path)
{
    return _mkdir(path);
}

/* Sys_RemoveDir  0x00481470 */
qboolean Sys_RemoveDir(const char *path)
{
    char               search[MAX_OS_PATH_SHORT];
    struct _finddata_t findinfo;
    intptr_t           findhandle;
    const char        *fmt;
    int                len;
    qboolean           hasTrailingSlash;
    qboolean           failed;
    int                result;

    len = I_strlen(path);
    if (path[len - 1] == '\\' || path[len - 1] == '/')
        hasTrailingSlash = qtrue;
    else
        hasTrailingSlash = qfalse;

    Com_sprintf(search, sizeof(search), hasTrailingSlash ? "%s*" : "%s\\*", path);

    findhandle = _findfirst(search, &findinfo);
    if (findhandle != -1)
    {
        failed = qfalse;
        do
        {
            if (findinfo.name[0] != '.' ||
                (findinfo.name[1] != '\0' && (findinfo.name[1] != '.' || findinfo.name[2] != '\0')))
            {
                if (hasTrailingSlash)
                    fmt = "%s%s";
                else
                    fmt = "%s\\%s";
                Com_sprintf(search, sizeof(search), fmt, path, findinfo.name);

                if (findinfo.attrib & _A_SUBDIR)
                {
                    result = Sys_RemoveDir(search);
                    failed = (result == 0);
                }
                else
                {
                    result = remove(search);
                    failed = (result == -1);
                }
            }
        }
        while (!failed && _findnext(findhandle, &findinfo) != -1);

        _findclose(findhandle);
        if (failed)
            return qfalse;
    }

    return _rmdir(path) != -1;
}

/* Sys_Cwd  0x00481670 */
char *Sys_Cwd(void)
{
    _getcwd(sys_cwd, sizeof(sys_cwd) - 1);
    sys_cwd[sizeof(sys_cwd) - 1] = '\0';
    return sys_cwd;
}

/* Sys_DefaultCDPath  0x004816a0 */
const char *Sys_DefaultCDPath(void)
{
    return "";
}

/* Sys_DefaultHomePath  0x004816b0 */
const char *Sys_DefaultHomePath(void)
{
    return NULL;
}

/* Sys_DefaultBasePath  0x004816c0 */
char *Sys_DefaultBasePath(void)
{
    HMODULE hModule;
    DWORD   len;

    if (sys_basepath[0] == '\0')
    {
        if (IsDebuggerPresent())
        {
            I_strncpyz(sys_basepath, Sys_Cwd(), sizeof(sys_basepath));
        }
        else
        {
            hModule = GetModuleHandleA(NULL);
            len = GetModuleFileNameA(hModule, sys_basepath, sizeof(sys_basepath));
            if (len == MAX_OS_PATH_SHORT)
                len = MAX_OS_PATH_SHORT - 1;

            while (len != 0 && sys_basepath[len] != '\\' && sys_basepath[len] != '/' && sys_basepath[len] != ':')
                len--;

            sys_basepath[len] = '\0';
        }
    }
    return sys_basepath;
}

/* Sys_ListFiles  0x00481780 */
char **Sys_ListFiles(const char *directory, const char *extension, const char *filter,
                     int *numfiles, qboolean wantsubs)
{
    char              *list[MAX_FOUND_FILES];
    char               search[MAX_OS_PATH_SHORT];
    struct _finddata_t findinfo;
    intptr_t           findhandle;
    HunkUser          *hunkUser;
    char             **listCopy;
    int                nfiles;
    int                subdirFlag;
    int                i;

    if (filter)
    {
        hunkUser = Hunk_UserCreate(0x20000, "Sys_ListFiles", 0, 0, 3);
        nfiles = 0;
        Sys_ListFilteredFiles(hunkUser, directory, "", filter, list, &nfiles);
        list[nfiles] = NULL;
        *numfiles = nfiles;
        if (!nfiles)
        {
            Hunk_UserDestroy(hunkUser);
            return NULL;
        }
        listCopy = (char **)Hunk_UserAlloc(hunkUser, nfiles * 4 + 8, 4);
        *(void **)listCopy = hunkUser;
        listCopy++;
        for (i = 0; i < nfiles; i++)
            listCopy[i] = list[i];
        listCopy[i] = NULL;
        return listCopy;
    }

    if (!extension)
        extension = "";

    if (extension[0] == '/' && extension[1] == '\0')
    {
        extension = "";
        subdirFlag = 0;
    }
    else
    {
        subdirFlag = _A_SUBDIR;
    }

    if (*extension)
        Com_sprintf(search, sizeof(search), "%s\\*.%s", directory, extension);
    else
        Com_sprintf(search, sizeof(search), "%s\\*", directory);

    nfiles = 0;
    findhandle = _findfirst(search, &findinfo);
    if (findhandle == -1)
    {
        *numfiles = 0;
        return NULL;
    }

    hunkUser = Hunk_UserCreate(0x20000, "Sys_ListFiles", 0, 0, 3);
    do
    {
        if ((!wantsubs && (findinfo.attrib & _A_SUBDIR) != subdirFlag) ||
            (wantsubs && (findinfo.attrib & _A_SUBDIR) != 0))
        {
            if (!(findinfo.attrib & _A_SUBDIR) ||
                (Q_stricmp(findinfo.name, ".") &&
                 Q_stricmp(findinfo.name, "..") &&
                 Q_stricmp(findinfo.name, "CVS")))
            {
                if (!*extension || Sys_FilterExtension(findinfo.name, extension))
                {
                    list[nfiles] = Hunk_CopyString(hunkUser, findinfo.name);
                    nfiles++;
                    if (nfiles == MAX_FOUND_FILES - 1)
                        break;
                }
            }
        }
    }
    while (_findnext(findhandle, &findinfo) != -1);

    list[nfiles] = NULL;
    _findclose(findhandle);
    *numfiles = nfiles;

    if (!nfiles)
    {
        Hunk_UserDestroy(hunkUser);
        return NULL;
    }

    listCopy = (char **)Hunk_UserAlloc(hunkUser, nfiles * 4 + 8, 4);
    *(void **)listCopy = hunkUser;
    listCopy++;
    for (i = 0; i < nfiles; i++)
        listCopy[i] = list[i];
    listCopy[i] = NULL;
    return listCopy;
}

/* Sys_ListFilteredFiles  0x00481ba0 */
void Sys_ListFilteredFiles(HunkUser *hunkUser, const char *basedir, const char *subdirs,
                           const char *filter, char **list, int *numfiles)
{
    char               filename[MAX_OS_PATH_SHORT];
    struct _finddata_t findinfo;
    intptr_t           findhandle;
    char               search[MAX_OS_PATH_SHORT];

    if (*numfiles >= MAX_FOUND_FILES - 1)
        return;

    if (strlen(subdirs))
        Com_sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
    else
        Com_sprintf(search, sizeof(search), "%s\\*", basedir);

    findhandle = _findfirst(search, &findinfo);
    if (findhandle == -1)
        return;

    do
    {
        if (!(findinfo.attrib & _A_SUBDIR) ||
            (Q_stricmp(findinfo.name, ".") &&
             Q_stricmp(findinfo.name, "..") &&
             Q_stricmp(findinfo.name, "CVS")))
        {
            if (*numfiles >= MAX_FOUND_FILES - 1)
                break;

            if (subdirs)
                Com_sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
            else
                Com_sprintf(filename, sizeof(filename), "%s", findinfo.name);

            if (Com_FilterPath(filter, filename, 0))
            {
                list[*numfiles] = Hunk_CopyString(hunkUser, filename);
                ++*numfiles;
            }
        }
    }
    while (_findnext(findhandle, &findinfo) != -1);

    _findclose(findhandle);
}

/* Sys_FilterExtension  0x00481d80 */
qboolean Sys_FilterExtension(const char *filename, const char *extension)
{
    char pattern[MAX_OS_PATH_SHORT];

    Com_sprintf(pattern, sizeof(pattern), "*.%s", extension);
    return I_stricmpWild(pattern, filename) == 0;
}

/* Sys_FreeFileList  0x00481de0 */
void Sys_FreeFileList(char **list)
{
    if (list)
        Hunk_UserDestroy((HunkUser *)((void **)list)[-1]);
}

/* Sys_DirectoryHasContents  0x00481e10 */
qboolean Sys_DirectoryHasContents(const char *dir)
{
    char               search[MAX_OS_PATH_SHORT];
    struct _finddata_t findinfo;
    intptr_t           findhandle;

    Com_sprintf(search, sizeof(search), "%s\\*", dir);

    findhandle = _findfirst(search, &findinfo);
    if (findhandle == -1)
        return qfalse;

    do
    {
        if (!(findinfo.attrib & _A_SUBDIR) ||
            (Q_stricmp(findinfo.name, ".") &&
             Q_stricmp(findinfo.name, "..") &&
             Q_stricmp(findinfo.name, "CVS")))
        {
            _findclose(findhandle);
            return qtrue;
        }
    }
    while (_findnext(findhandle, &findinfo) != -1);

    _findclose(findhandle);
    return qfalse;
}


void Sys_Stub481f20(void) { }   /* 0x00481f20 */
void Sys_Stub481f30(void) { }   /* 0x00481f30 */
void Sys_Stub481f40(void) { }   /* 0x00481f40 */
void Sys_Stub481f50(void) { }   /* 0x00481f50 */

/* FS_FRead  0x00481f60 */
int FS_FRead(void *buffer, int elemSize, int count, int f)
{
    return FS_Read(buffer, elemSize * count, f);
}

/* FS_FSeek  0x00481f80 */
int FS_FSeek(int f, int offset, int origin)
{
    return FS_Seek(f, offset, origin);
}

void Sys_Stub481fa0(void) { }   /* 0x00481fa0 */

/* Sys_IsRenderThread  0x00481fb0 */
qboolean Sys_IsRenderThread(void)
{
    return qfalse;
}

/* Sys_IsMainThread  0x00481fc0 */
qboolean Sys_IsMainThread(void)
{
    return qtrue;
}

/* Sys_SetValue  0x00481fd0 */
void Sys_SetValue(int valueIndex, void *value)
{
    sys_values[valueIndex] = value;
}

/* Sys_GetValue  0x00481ff0 */
void *Sys_GetValue(int valueIndex)
{
    return sys_values[valueIndex];
}

/* Com_IsXModelName  0x0044b180 */
qboolean Com_IsXModelName( const char *name )
{
    if ( Q_stricmpn( name, "xmodel", 6 ) )
        return qfalse;

    if ( name[6] != '/' && name[6] != '\\' )
        return qfalse;

    return qtrue;
}

/* MatrixTransformVector43  0x0044aa00 */
void MatrixTransformVector43( const float m[4][3], const vec3_t pos, vec3_t out )
{
    Assert( pos != out );

    out[0] = m[1][0] * pos[0] + m[0][0] + m[2][0] * pos[1] + m[3][0] * pos[2];
    out[1] = m[1][1] * pos[0] + m[0][1] + m[2][1] * pos[1] + m[3][1] * pos[2];
    out[2] = m[1][2] * pos[0] + m[0][2] + m[2][2] * pos[1] + m[3][2] * pos[2];
}

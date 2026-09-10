/* Original: cod3src\src\universal\q_shared.h */

#ifndef Q_SHARED_H
#define Q_SHARED_H

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <float.h>
#include <time.h>
#include <limits.h>

typedef float          vec_t;
typedef vec_t          vec2_t[2];
typedef vec_t          vec3_t[3];
typedef vec_t          vec4_t[4];
typedef unsigned char  byte;
typedef int            qboolean;

#define qtrue  1
#define qfalse 0

typedef char q_static_assert_int4[sizeof(int)   == 4 ? 1 : -1];
typedef char q_static_assert_flt4[sizeof(float) == 4 ? 1 : -1];

#define MAX_QPATH          64
#define MAX_OS_PATH        1024
#define MAX_STRING_CHARS   32768
#define MAXPRINTMSG        4096
#define MAX_TOKEN_CHARS    1024

#define MAX_OS_PATH_SHORT  256

#define MAX_FOUND_FILES    8192

#define MAX_INFO_STRING    1024      /* 0x0400 */
#define BIG_INFO_STRING    8192      /* 0x2000 */

#define VA_SCRATCH_SIZE    31999
#define VA_RING_SIZE       0x7D00
#define VA_BIG_BUF_SIZE    0x4000

#define STACK_CHECK_SIZE   0x2000

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

typedef struct
{
    vec3_t origin;    /* +0x00 */
    vec3_t axis[3];   /* +0x0c */
} orientation_t;

int      COM_MatchToken(const char **data_p, const char *match, int warnOnly);
int      SkipBracedSection(const char **data_p, int depth, int maxDepth);
void     SkipRestOfLine(const char **data_p);
int      COM_CountRestOfLineTokens(const char **data_p);
char    *COM_ParseRestOfLine(const char **data_p);
float    COM_ParseFloatCrossLine(const char **data_p);
float    COM_ParseFloat(const char **data_p);
int      COM_ParseInt(const char **data_p);
int      COM_ParseIntExt(const char **data_p);
void     COM_Parse1DMatrix(const char **data_p, int x, float *m);
void     COM_Parse2DMatrix(const char **data_p, int y, int x, float *m);
void     COM_Parse3DMatrix(const char **data_p, int z, int y, int x, float *m);
int      I_ColorIndex(char c);

char    *COM_SkipPath(const char *pathname);
void     Com_BuildFilepath(const char *folder, const char *name, const char *extension,
                           char *path, int maxCharCount);
char    *COM_GetExtension(const char *filename);
void     COM_StripExtension(const char *in, char *out);
void     COM_StripFilename(const char *in, char *out);
void     COM_DefaultExtension(char *path, int maxSize, const char *extension);
qboolean Com_IsXModelPath(const char *path);

short    ShortSwap(short l);
short    ShortNoSwap(short l);
int      LongSwap(int l);
int      LongNoSwap(int l);
unsigned __int64 Long64Swap(unsigned __int64 ll);
unsigned __int64 Long64NoSwap(unsigned __int64 ll);
float    FloatSwap(float f);
float    FloatNoSwap(float f);
int      FloatSwapBits(float f);
int      FloatNoSwapBits(float f);

void     Swap_Init(void);

short    BigShort(short l);
short    LittleShort(short l);
int      BigLong(int l);
int      LittleLong(int l);
unsigned __int64 LittleLong64(unsigned __int64 l);
float    LittleFloat(float f);
float    BigFloat(float f);
int      LittleFloatBits(float f);
int      BigFloatBits(float f);

qboolean Q_isprint(int c);
qboolean Q_islower(int c);
qboolean Q_isupper(int c);
qboolean Q_isalpha(int c);
qboolean Q_isnumeric(int c);
qboolean Q_isalphanumeric(int c);
qboolean Q_isforfilename(int c);
qboolean Q_isidentifierchar(int c);

void     I_strncpyz(char *dest, const char *src, int destsize);

qboolean Com_IsXModelName( const char *name );                       /* 0x0044b180 */

void     MatrixTransformVector43( const float m[4][3], const vec3_t pos,
                                  vec3_t out );                      /* 0x0044aa00 */
void     I_strncat(char *dest, int size, const char *src);
int      Q_stricmpn(const char *s0, const char *s1, int n);
int      Q_strncmp(const char *s0, const char *s1, int n);
int      Q_stricmp(const char *s0, const char *s1);
int      Q_strcmp(const char *s0, const char *s1);
const char *Q_stristr(const char *s0, const char *substr);
int      I_stricmpWild(const char *wild, const char *s);
char    *Q_strlwr(char *s);
char    *Q_strupr(char *s);
int      Q_PrintStrlen(const char *string);
int      Com_sprintf(char *dest, size_t size, const char *fmt, ...);
int      Com_sprintfAppend(char *dest, int size, int *offset, const char *fmt, ...);
int      CanKeepStringPointer(const void *ptr);
char    *va(const char *format, ...);

const char *Info_ValueForKey(const char *s, const char *key);
void     Info_NextPair(const char **head, char *key, char *value);
void     Info_RemoveKey(char *s, const char *key);
void     Info_RemoveKey_Big(char *s, const char *key);
qboolean Info_Validate(const char *s);
void     Info_SetValueForKey(char *s, const char *key, const char *value);
void     Info_SetValueForKey_Big(char *s, const char *key, const char *value);

typedef struct
{
    const char *szName;      /* +0x00 */
    int         iOffset;     /* +0x04 */
    int         iFieldType;  /* +0x08 */
} cspField_t;

#define CS_FIELD_CALLBACK      0
#define CS_FIELD_STRING_1024   1
#define CS_FIELD_STRING_64     2
#define CS_FIELD_STRING_256    3
#define CS_FIELD_INT           4
#define CS_FIELD_BOOL          5
#define CS_FIELD_FLOAT         6
#define CS_FIELD_FLOAT_TO_INT  7

bool     ParseConfigStringToStruct(byte *pStruct, const cspField_t *pFieldList, int numFields,
                                   const char *pConfigString, int numSpecialFieldTypes,
                                   bool (*parseSpecialFieldType)(byte *, const char *, int),
                                   void (*parseDefaultFieldType)(byte *, const char *));

void     TransformPoint(const orientation_t *orient, const vec3_t pos, vec3_t out);
void     TransformDir(const orientation_t *orient, const vec3_t dir, vec3_t out);
void     TransformPoint4(const orientation_t *orient, const vec4_t pos, vec4_t out);
void     TransformPlane(const orientation_t *orient, const vec4_t plane, vec4_t out);
void     InverseTransformPoint(const orientation_t *orient, const vec3_t pos, vec3_t out);
void     InverseTransformDir(const orientation_t *orient, const vec3_t dir, vec3_t out);
void     InverseTransformPoint4(const orientation_t *orient, const vec4_t pos, vec4_t out);
void     TransformOrientation(const orientation_t *orFirst, const orientation_t *orSecond,
                              orientation_t *out);
void     InverseTransformPlane(const orientation_t *orient, const vec4_t plane, vec4_t out);
void     TransformPointScaled(const orientation_t *orient, float scale, const vec3_t pos, vec3_t out);
void     TransformPlaneScaled(const orientation_t *orient, float scale, const vec4_t plane, vec4_t out);
void     InverseTransformPointScaled(const orientation_t *orient, float scale, const vec3_t pos, vec3_t out);
void     InverseTransformPlaneScaled(const orientation_t *orient, float scale, const vec4_t plane, vec4_t out);

int      Sys_Mkdir(const char *path);
qboolean Sys_RemoveDir(const char *path);
char    *Sys_Cwd(void);
const char *Sys_DefaultCDPath(void);
const char *Sys_DefaultHomePath(void);
char    *Sys_DefaultBasePath(void);
typedef struct HunkUser_s HunkUser;

char   **Sys_ListFiles(const char *directory, const char *extension, const char *filter,
                       int *numfiles, qboolean wantsubs);
void     Sys_ListFilteredFiles(HunkUser *hunkUser, const char *basedir, const char *subdirs,
                               const char *filter, char **list, int *numfiles);
qboolean Sys_FilterExtension(const char *filename, const char *extension);
void     Sys_FreeFileList(char **list);
qboolean Sys_DirectoryHasContents(const char *dir);

qboolean Sys_IsMainThread(void);
qboolean Sys_IsRenderThread(void);
void     Sys_SetValue(int valueIndex, void *value);
void    *Sys_GetValue(int valueIndex);

inline int I_strlen( const char *string ) { return (int)strlen( string ); }

#endif

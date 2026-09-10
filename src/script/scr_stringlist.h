/* Original: c:\trees\cod3\cod3src\src\script\scr_stringlist.cpp */

#ifndef SCR_STRINGLIST_H
#define SCR_STRINGLIST_H

#include "q_shared.h"


#define HASH_MAX            19999
#define HASH_COUNT          ( HASH_MAX + 1 )

#define HASH_NEXT_MASK      0x0000ffff
#define HASH_STAT_MASK      0x00030000

#define HASH_STAT_FREE      0x00000000
#define HASH_STAT_MOVABLE   0x00010000
#define HASH_STAT_HEAD      0x00020000

#define MAX_STRING_CHARS_SL 0x2000

#define SCR_SYS_GAME        1


typedef struct
{
    unsigned short refCount;        /* +0x00 */
    byte           user;            /* +0x02 */
    byte           len;             /* +0x03 */
    char           str[1];          /* +0x04 */
} RefString;

typedef struct
{
    unsigned int status_next;       /* +0x00 */
    unsigned int prev;              /* +0x04 */
} HashEntry;

typedef struct
{
    HashEntry  hashTable[HASH_COUNT];   /* 0x131ad680 */
    qboolean   inited;                  /* 0x131d4780 */
    HashEntry *lastHead;                /* 0x131d4784 */
} scrStringGlob_t;


extern scrStringGlob_t scrStringGlob;


void SL_Init( void );                                            /* 0x00429220 */
void SL_Shutdown( void );                                        /* 0x00429310 */

const char *SL_ConvertToString( unsigned int stringValue );      /* 0x00429000 */
const char *SL_ConvertToStringSafe( unsigned int stringValue );  /* 0x00429020 */
const char *SL_ConvertToStringDebug( unsigned int stringValue ); /* 0x00429040 */

int          SL_GetStringLen( unsigned int stringValue );        /* 0x004290d0 */
unsigned int SL_GetStringValue( const char *str );               /* 0x00429150 */

unsigned int SL_FindStringOfSize( const char *str, int len );    /* 0x00429330 */
unsigned int SL_FindString( const char *str );                   /* 0x004296b0 */
unsigned int SL_FindLowercaseString( const char *str );          /* 0x004296e0 */

qboolean     SL_IsStringLowercase( unsigned int stringValue );   /* 0x00429770 */

unsigned int SL_GetStringOfSize( const char *str, int user,
                                 int len, int type );            /* 0x00429850 */
unsigned int SL_GetStringOfType( const char *str, int user,
                                 int type );                     /* 0x00429f00 */
unsigned int SL_GetString( const char *str, int user );          /* 0x00429f40 */
unsigned int SL_GetLowercaseStringOfType( const char *str, int user,
                                          int type );            /* 0x0042a010 */
unsigned int SL_GetLowercaseString( const char *str, int user );  /* 0x0042a050 */
unsigned int SL_GetSystemString( const char *str, int sys );      /* 0x0042a350 */
unsigned int SL_GetCanonicalFilename( const char *filename );     /* 0x0042a620 */

unsigned int SL_GetStringForFloat( float value );                 /* 0x0042a3a0 */
unsigned int SL_GetStringForInt( int value );                     /* 0x0042a400 */
unsigned int SL_GetStringForVector( const float *value );         /* 0x0042a450 */

void SL_AddRefToString( unsigned int stringValue );               /* 0x0042a0f0 */
void SL_AddRefToStringOfUser( unsigned int stringValue, int user );/* 0x00429800 */
void SL_RemoveRefToString( unsigned int stringValue );            /* 0x0042a670 */
void SL_RemoveRefToStringOfSize( unsigned int stringValue, int len ); /* 0x0042a320 */
void SL_RemoveRefToStringOfUser( unsigned int stringValue, int user );/* 0x0042a090 */

int  SL_GetStringUser( unsigned int stringValue );                /* 0x00429830 */
void SL_TransferUser( int from, int to );                         /* 0x0042a4c0 */

void SL_SetStringValue( unsigned short *ref, unsigned int stringValue ); /* 0x0042a6e0 */
void SL_SetString( unsigned short *ref, const char *str );        /* 0x0042a720 */

#endif

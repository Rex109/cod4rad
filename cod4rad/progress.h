/* Original: c:\trees\cod3\cod3src\cod2rad\progress.cpp */

#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdarg.h>


void Print( const char *fmt, ... );                     /* 0x0041f920 */
void VerbosePrint( const char *fmt, ... );              /* 0x0041f8f0 */
void Warning( int level, const char *fmt, ... );        /* 0x0041f950 */

void ErrorV( const char *fmt, va_list argptr );         /* 0x0041f980 */
void Error( const char *fmt, ... );                     /* 0x0041f9e0 */

void PrintDuration( int milliseconds );                 /* 0x0041fa00 */

void StartProgress( const char *name );                 /* 0x0041fbb0 */
void SetProgress( int completed, int total );           /* 0x0041fc00 */
void AddProgress( int count );                          /* 0x0041fc20 */
void EndProgress( void );                               /* 0x0041fc40 */

#endif

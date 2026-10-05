/* Prints where an unhandled exception happened, with function names and line numbers
   when the .pdb sits next to the exe.  Output goes to the console and to
   cod4rad_crash.txt in the current directory. */

#include "crashlog.h"

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>

#pragma comment( lib, "dbghelp.lib" )


static void CrashPrint( FILE *file, const char *fmt, ... )
{
    char    buf[1024];
    va_list args;

    va_start( args, fmt );
    _vsnprintf( buf, sizeof( buf ) - 1, fmt, args );
    va_end( args );

    buf[sizeof( buf ) - 1] = 0;

    fputs( buf, stderr );

    if ( file )
        fputs( buf, file );
}

static void CrashPrintAddress( FILE *file, HANDLE process, DWORD64 address )
{
    char                symbolBuffer[sizeof( SYMBOL_INFO ) + 256];
    SYMBOL_INFO        *symbol = ( SYMBOL_INFO * )symbolBuffer;
    DWORD64             displacement = 0;
    IMAGEHLP_LINE64     line;
    DWORD               lineDisplacement = 0;
    IMAGEHLP_MODULE64   module;

    memset( symbolBuffer, 0, sizeof( symbolBuffer ) );
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
    symbol->MaxNameLen   = 255;

    memset( &module, 0, sizeof( module ) );
    module.SizeOfStruct = sizeof( module );

    memset( &line, 0, sizeof( line ) );
    line.SizeOfStruct = sizeof( line );

    CrashPrint( file, "  0x%016llx", ( unsigned long long )address );

    if ( SymGetModuleInfo64( process, address, &module ) )
        CrashPrint( file, "  %s+0x%llx", module.ModuleName,
                    ( unsigned long long )( address - module.BaseOfImage ) );

    if ( SymFromAddr( process, address, &displacement, symbol ) )
        CrashPrint( file, "  %s+0x%llx", symbol->Name, ( unsigned long long )displacement );

    if ( SymGetLineFromAddr64( process, address, &lineDisplacement, &line ) )
        CrashPrint( file, "  (%s:%lu)", line.FileName, line.LineNumber );

    CrashPrint( file, "\n" );
}

static LONG WINAPI CrashFilter( EXCEPTION_POINTERS *info )
{
    static volatile LONG entered;
    FILE      *file;
    HANDLE     process = GetCurrentProcess();
    HANDLE     thread  = GetCurrentThread();
    CONTEXT    context = *info->ContextRecord;
    STACKFRAME64 frame;
    DWORD      machine;
    int        i;

    if ( InterlockedExchange( &entered, 1 ) )
        return EXCEPTION_CONTINUE_SEARCH;

    file = fopen( "cod4rad_crash.txt", "w" );

    SymSetOptions( SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
    SymInitialize( process, NULL, TRUE );

    CrashPrint( file, "\ncod4rad crashed: exception 0x%08lx at\n",
                ( unsigned long )info->ExceptionRecord->ExceptionCode );
    CrashPrintAddress( file, process, ( DWORD64 )( size_t )info->ExceptionRecord->ExceptionAddress );

    if ( ( info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
           || info->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR )
         && info->ExceptionRecord->NumberParameters >= 2 )
    {
        CrashPrint( file, "  %s address 0x%016llx\n",
                    info->ExceptionRecord->ExceptionInformation[0] == 0 ? "reading"
                    : info->ExceptionRecord->ExceptionInformation[0] == 1 ? "writing" : "executing",
                    ( unsigned long long )info->ExceptionRecord->ExceptionInformation[1] );
    }

    memset( &frame, 0, sizeof( frame ) );

#ifdef _WIN64
    machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#else
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset    = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#endif
    frame.AddrPC.Mode    = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    CrashPrint( file, "call stack:\n" );

    for ( i = 0; i < 32; i++ )
    {
        if ( !StackWalk64( machine, process, thread, &frame, &context, NULL,
                           SymFunctionTableAccess64, SymGetModuleBase64, NULL ) )
            break;

        if ( !frame.AddrPC.Offset )
            break;

        CrashPrintAddress( file, process, frame.AddrPC.Offset );
    }

    if ( file )
    {
        fclose( file );
        fputs( "(also written to cod4rad_crash.txt)\n", stderr );
    }

    fflush( stderr );

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashLog_Install( void )
{
    SetErrorMode( SEM_FAILCRITICALERRORS );
    SetUnhandledExceptionFilter( CrashFilter );
}

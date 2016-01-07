
#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>

#include <shellapi.h>
#include <Shlwapi.h>
#pragma comment( lib, "Shlwapi.lib" )

#include <DbgHelp.h>
#pragma comment( lib, "dbghelp.lib" )

#include <process.h>
#include <atlbase.h>
#include <initguid.h>

//////////////////////////////////////////////////////////////////////////

inline BOOL WSL2_IsRunIn64()
{
	BOOL b64 = FALSE ;
	IsWow64Process( GetCurrentProcess(), &b64 ) ;
	return b64 ;
}

//////////////////////////////////////////////////////////////////////////

#define MAKE_SOFTWARE_EXCEPTION( Severity, Facility, Exception )	\
		( (DWORD)(\
		/* Severity code	*/ ( Severity		)		|\
		/* MS(0) or Cust(1)	*/ ( 1 << 29		)		|\
		/* Reserved(0)		*/ ( 0 << 28		)		|\
		/* Facility code	*/ ( Facility << 16 )		|\
		/* Exception code	*/ ( Exception << 0 )		)	)

//////////////////////////////////////////////////////////////////////////

#define	BEGIN_THREAD( psa, cbStackSize, pfnStartAddr, pvParam, dwCreateFlags, pdwThreadId )	\
		( (HANDLE)_beginthreadex(\
		(void*)								psa,			\
		(unsigned int)						cbStackSize,	\
		(unsigned int (__stdcall *)(void *))pfnStartAddr,	\
		(void*)					pvParam,					\
		(unsigned int)				dwCreateFlags,			\
		(unsigned int*)				pdwThreadId	)	)		

//////////////////////////////////////////////////////////////////////////

inline LONG WINAPI WSL2_TopLevelExceptionFilter( _EXCEPTION_POINTERS* ExceptionInfo )
{
	wchar_t szDumpFile[ MAX_PATH ] = { 0 } ;
	GetModuleFileNameW( NULL, szDumpFile, MAX_PATH ) ;
	wcscat_s( szDumpFile, L".dmp" ) ;

	HANDLE hDump = CreateFileW( szDumpFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ;
	if ( hDump == INVALID_HANDLE_VALUE )
	{
		return EXCEPTION_EXECUTE_HANDLER ;
	}

	MINIDUMP_EXCEPTION_INFORMATION stExceptionParam ;
	stExceptionParam.ThreadId = GetCurrentThreadId() ;
	stExceptionParam.ExceptionPointers = ExceptionInfo ;
	stExceptionParam.ClientPointers = FALSE ;
	MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hDump, MiniDumpNormal, &stExceptionParam, NULL, NULL ) ;

	CloseHandle( hDump ) ;
	return EXCEPTION_EXECUTE_HANDLER ;
}

inline int WSL2_InitDump()
{
	SetUnhandledExceptionFilter( WSL2_TopLevelExceptionFilter ) ;
	return 0 ;
}

//////////////////////////////////////////////////////////////////////////

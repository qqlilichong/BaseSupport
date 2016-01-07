
#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>

#include <Shlwapi.h>
#pragma comment( lib, "Shlwapi.lib" )

#include <process.h>

//////////////////////////////////////////////////////////////////////////

inline bool WSL_IsRunIn64()
{
	BOOL b64 = FALSE ;
	IsWow64Process( GetCurrentProcess(), &b64 ) ;
	return b64 == TRUE ;
}

//////////////////////////////////////////////////////////////////////////

#define MAKESOFTWAREEXCEPTION( Severity, Facility, Exception )	\
		( (DWORD)(\
		/* Severity code	*/ ( Severity		)		|\
		/* MS(0) or Cust(1)	*/ ( 1 << 29		)		|\
		/* Reserved(0)		*/ ( 0 << 28		)		|\
		/* Facility code	*/ ( Facility << 16 )		|\
		/* Exception code	*/ ( Exception << 0 )		)	)

//////////////////////////////////////////////////////////////////////////

#define	BEGINTHREAD( psa, cbStackSize, pfnStartAddr, pvParam, dwCreateFlags, pdwThreadId )	\
		( (HANDLE)_beginthreadex(\
		(void*)								psa,			\
		(unsigned)							cbStackSize,	\
		(unsigned int (__stdcall *)(void *))pfnStartAddr,	\
		(void*)					pvParam,					\
		(unsigned)				dwCreateFlags,				\
		(unsigned*)				pdwThreadId					)	)

//////////////////////////////////////////////////////////////////////////

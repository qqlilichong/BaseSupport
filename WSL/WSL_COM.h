
#pragma once

#include <atlbase.h>
#include <ShObjIdl.h>
#include "WSL_String.h"

//////////////////////////////////////////////////////////////////////////

class CwComBase
{
public:
	CwComBase()
	{
		CoInitialize( 0 ) ;
	}

	~CwComBase()
	{
		CoUninitialize() ;
	}
};

//////////////////////////////////////////////////////////////////////////

inline wstring WSL_GUID2WSTR( _In_ GUID g )
{
	wstring s ;
	wchar_t* pszGUID = NULL ;
	if ( StringFromCLSID( g, &pszGUID ) == 0 )
	{
		s = pszGUID ;
		CoTaskMemFree( pszGUID ) ;
		pszGUID = NULL ;
	}
	
	return s ;
}

//////////////////////////////////////////////////////////////////////////

inline GUID WSL_WSTR2GUID( _In_ const wchar_t* s )
{
	GUID g = GUID_NULL ;
	CLSIDFromString( s, &g ) ;
	return g ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WSL_CreateGuidStr()
{
	GUID guid = GUID_NULL ;
	CoCreateGuid( &guid ) ;
	return WSL_GUID2WSTR( guid ) ;
}

//////////////////////////////////////////////////////////////////////////

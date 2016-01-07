
#pragma once

#include "Chilkat/CkInclude.h"

//////////////////////////////////////////////////////////////////////////

class CvHttpDownload
{
public:
	CvHttpDownload()
	{
		if ( !CkUnlock( m_http, L"30277129240" ) )
		{
			wprintf( L"CvHttpDownload CkUnlock failed." ) ;
		}
	}

	void SetAuth( const wchar_t* pszUser, const wchar_t* pszPassW )
	{
		m_http.put_Login( pszUser ) ;
		m_http.put_Password( pszPassW ) ;
		m_http.put_SaveCookies( true ) ;
		m_http.put_CookieDir( L"memory" ) ;
	}

	bool QuickGetDatabytes( const wchar_t* pszURL )
	{
		return m_http.QuickGet( pszURL, m_outData ) ;
	}

	const byte* get_Data() { return m_outData.getBytes() ; }
	unsigned long get_Size() { return m_outData.getSize() ; }

private:
	CkHttpW		m_http ;
	CkByteData  m_outData ;
};

//////////////////////////////////////////////////////////////////////////

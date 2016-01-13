
#pragma once

//////////////////////////////////////////////////////////////////////////

#define W2MAKEFOURCC( ch0, ch1, ch2, ch3 ) \
					((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
					((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

inline wstring WSL2_GetComputerName()
{
	wchar_t szName[ MAX_COMPUTERNAME_LENGTH + 1 ] = { 0 } ;
	DWORD dwCount = _countof( szName ) ;
	GetComputerNameW( szName, &dwCount ) ;
	return szName ;
}

inline wstring WSL2_GUID2String( GUID guid )
{
	wchar_t result[ 128 ] = { 0 } ;
	StringFromGUID2( guid, result, _countof( result ) ) ;
	return result ;
}

inline void WSL2_MyExit()
{
	TerminateProcess( GetCurrentProcess(), 0 ) ;
}

//////////////////////////////////////////////////////////////////////////

template< class T >
inline T* WSL2_String_New( _In_ DWORD dwMaxLen )
{
	T* str = new T[ dwMaxLen ] ;
	if ( str != nullptr )
	{
		SecureZeroMemory( str, sizeof( T ) * dwMaxLen ) ;
	}

	return str ;
}

template< class T >
inline void WSL2_String_Release( _In_ T** str )
{
	T*& ptr = *str ;
	if ( ptr != nullptr )
	{
		delete []ptr ;
		ptr = nullptr ;
	}
}

Cw2AutoHandle( Cw2WString, wchar_t*, nullptr, WSL2_String_Release< wchar_t > ) ;
Cw2AutoHandle( Cw2String, char*, nullptr, WSL2_String_Release< char > ) ;

//////////////////////////////////////////////////////////////////////////

inline wstring WSL2_String2W( _In_ const char* str )
{
	const int nNeedLen = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 ) ;
	if ( nNeedLen > 0 )
	{
		Cw2WString wstr = WSL2_String_New< wchar_t >( nNeedLen ) ;
		if ( !wstr.InvalidHandle() )
		{
			if ( MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, nNeedLen ) != 0 )
			{
				return (wchar_t*)wstr ;
			}
		}
	}
	
	return L"" ;
}

inline string WSL2_String2A( _In_ const wchar_t* wstr )
{
	const int nNeedSize = WideCharToMultiByte( CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL ) ;
	if ( nNeedSize > 0 )
	{
		Cw2String str = WSL2_String_New< char >( nNeedSize ) ;
		if ( !str.InvalidHandle() )
		{
			if ( WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, nNeedSize, NULL, NULL ) != 0 )
			{
				return (char*)str ;
			}
		}
	}
	
	return "" ;
}

inline shared_ptr<char> WSL2_String2UTF8( _In_ const wchar_t* wstr )
{
	shared_ptr<char> ret ;

	const int nNeedSize = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL ) ;
	if ( nNeedSize > 0 )
	{
		ret.reset( new char[ nNeedSize ] ) ;
		if ( ret )
		{
			WideCharToMultiByte( CP_UTF8, 0, wstr, -1, ret.get(), nNeedSize, NULL, NULL ) ;
		}
	}

	return ret ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WSL2_String_FormatWCore( _In_ const wchar_t* format, va_list args )
{
	const int nNeedLen = _vscwprintf_p ( format, args ) + 1 ;
	Cw2WString wstr = WSL2_String_New< wchar_t >( nNeedLen ) ;
	if ( !wstr.InvalidHandle() )
	{
		_vswprintf_p( wstr, nNeedLen, format, args ) ;
	}

	return (wchar_t*)wstr ;
}

inline wstring WSL2_String_FormatW( _In_ const wchar_t* format, ... )
{
	va_list args ;
	va_start( args, format ) ;
	return WSL2_String_FormatWCore( format, args ) ;
}

inline string WSL2_String_FormatA( _In_ const char* format, ... )
{
	va_list args ;
	va_start( args, format ) ;
	
	const int nNeedLen = _vscprintf_p ( format, args ) + 1 ;
	Cw2String str = WSL2_String_New< char >( nNeedLen ) ;
	if ( !str.InvalidHandle() )
	{
		_vsprintf_p( str, nNeedLen, format, args ) ;
	}
	
	return (char*)str ;
}

inline void WSL2_String_Replace( _In_ string& str, _In_ const char src, _In_ const char dst )
{
	for ( auto& it : str )
	{
		if ( it == src )
		{
			it = dst ;
		}
	}
}

inline void WSL2_String_Replace( _In_ wstring& str, _In_ const wchar_t src, _In_ const wchar_t dst )
{
	for ( auto& it : str )
	{
		if ( it == src )
		{
			it = dst ;
		}
	}
}

template< typename S >
inline void WSL2_String_Lower( _In_ S& str, _In_opt_ bool lower = true )
{
	std::transform( str.begin(), str.end(), str.begin(), lower ? tolower : toupper ) ;
}

inline int WSL2_String2Int( _In_ string& str )
{
	int nRet = 0 ;
	istringstream stream( str ) ;
	stream >> nRet ;
	return nRet ;
}

inline int WSL2_String2Int( _In_ wstring& str )
{
	int nRet = 0 ;
	wistringstream stream( str ) ;
	stream >> nRet ;
	return nRet ;
}

template< typename S, typename C >
inline int WSL2_String2Vector( _In_ S str, _Out_ vector< S >& vec, _In_ const C* pszFlag )
{
	S flag = pszFlag ;
	S::size_type BOF = S::npos ;
	while ( ( BOF = str.find( flag ) ) != S::npos )
	{
		vec.push_back( str.substr( 0, BOF ) ) ;
		str = str.substr( BOF + flag.length() ) ;
	}
	
	if ( str.length() )
	{
		vec.push_back( str ) ;
	}
	
	return 0 ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WSL2_ErrMsg( DWORD dwErr = 0 )
{
	if ( dwErr == 0 )
	{
		dwErr = GetLastError() ;
	}
	
	// Format error message.
	Cw2Local fmErr ;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER 
		| FORMAT_MESSAGE_FROM_SYSTEM 
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		(LPTSTR)&fmErr, 0, NULL ) ;
	
	return WSL2_String_FormatW( L"ErrorCode = %u, %s", dwErr, fmErr ) ;
}

inline void WSL2_DbgMsg( _In_ FILE* os, _In_ const char* pszDBG, _In_ const wchar_t* format, ... )
{
	wstring part1 = WSL2_String2W( pszDBG ) ;
	
	SYSTEMTIME st = { 0 } ;
	GetLocalTime( &st ) ;

	va_list args ;
	va_start( args, format ) ;
	wstring part2 = WSL2_String_FormatWCore( format, args ) ;

	wstring part3 = WSL2_String_FormatW( L"%04d-%02d-%02d %02d:%02d:%02d:%03d",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds ) ;
	
	wstring outpart = WSL2_String_FormatW( L"[%s][%s]:%s\n", part3.c_str(), part1.c_str(), part2.c_str() ) ;
	OutputDebugStringW( outpart.c_str() ) ;
	fwprintf_s( os, outpart.c_str() ) ;
}

#define __W2DBG_ADD2(x)		#x
#define __W2DBG_ADD(x)		__W2DBG_ADD2(x)
#define __W2DBG_MSG()		(__FILE__"("__W2DBG_ADD(__LINE__)")")
#define W2DBG( ... )		WSL2_DbgMsg(stderr,__W2DBG_MSG(),__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

inline wstring WSL2_SubWString( _In_ const wchar_t* wstr,
							   _In_ const wchar_t* flag1, _In_ const wchar_t* flag2 = L"",
							   _In_ int lf1 = 1, _In_ int lf2 = 1 )
{
	wstring text = wstr ;
	wstring str_flag1 = flag1 ;
	wstring str_flag2 = flag2 ;
	
	wstring::size_type pos1 = ( lf1 ? text.find( flag1 ) : text.rfind( flag1 ) ) ;
	if ( pos1 == wstring::npos )
	{
		return L"" ;
	}

	if ( str_flag2.length() == 0 )
	{
		return text.substr( pos1 + str_flag1.length() ) ;
	}

	wstring::size_type pos2 = ( lf2 ? text.find( flag2 ) : text.rfind( flag2 ) ) ;
	if ( pos2 == wstring::npos )
	{
		return L"" ;
	}

	if ( pos2 <= pos1 )
	{
		return L"" ;
	}
	
	return text.substr( pos1 + str_flag1.length(), pos2 - pos1 - str_flag1.length() ) ;
}

inline wstring WSL2_FileName_Path( _In_ const wchar_t* filename )
{
	return WSL2_SubWString( filename, L"", L"\\", 1, 0 ) ;
}

inline wstring WSL2_FileName_File( _In_ const wchar_t* filename )
{
	return WSL2_SubWString( filename, L"\\", L".", 0, 0 ) ;
}

inline wstring WSL2_FileName_Ext( _In_ const wchar_t* filename )
{
	return WSL2_SubWString( filename, L".", L"", 0, 0 ) ;
}

inline wstring WSL2_FileName_PathName( _In_ const wchar_t* filename )
{
	return WSL2_SubWString( filename, L"", L".", 1, 0 ) ;
}

inline wstring WSL2_FileName_FileName( _In_ const wchar_t* filename )
{
	return WSL2_SubWString( filename, L"\\", L"", 0 ) ;
}

inline wstring WSL2_GetModuleFileName( _In_opt_ HMODULE hMod = NULL )
{
	wchar_t szFile[ MAX_PATH ] = { 0 } ;
	GetModuleFileNameW( hMod, szFile, _countof( szFile ) ) ;
	return szFile ;
}

inline int WSL2_DeletePath( const wchar_t* path )
{
	wchar_t file[ MAX_PATH ] = { 0 } ;
	wsprintfW( file, path ) ;
	
	SHFILEOPSTRUCTW op = { 0 } ;
	op.fFlags = FOF_NO_UI ;
	op.pFrom = file ;
	op.wFunc = FO_DELETE ;
	return SHFileOperationW( &op ) ;
}

//////////////////////////////////////////////////////////////////////////

inline BOOL WSL2_ListFindedFiles( _Out_ vector< wstring >& vecResults,
								 _In_ const wchar_t* pszFiles, _In_opt_ const wchar_t* pszDir = L"" )
{
	wstring strDir = pszDir ;
	if ( strDir.length() == 0 )
	{
		strDir = WSL2_FileName_Path( WSL2_GetModuleFileName().c_str() ) ;
	}
	
	strDir += L"\\" ;
	wstring strFind = strDir + pszFiles ;
	WIN32_FIND_DATAW ffd = { 0 } ;
	HANDLE hFind = FindFirstFileW( strFind.c_str(), &ffd ) ;
	if ( hFind == INVALID_HANDLE_VALUE )
	{
		return FALSE ;
	}
	
	do { vecResults.push_back( strDir + ffd.cFileName ) ; } while ( FindNextFileW( hFind, &ffd ) != 0 ) ;
	
	const DWORD dwErr = GetLastError() ;
	FindClose( hFind ) ;
	return dwErr == ERROR_NO_MORE_FILES ;
}

//////////////////////////////////////////////////////////////////////////

class Cw2Ini
{
public:
	Cw2Ini()
	{
		m_strProfile = WSL2_GetModuleFileName() ;
		m_strProfile = WSL2_FileName_PathName( m_strProfile.c_str() ) ;
		m_strProfile += L".ini" ;
	}
	
	void InitProfile( _In_ const wchar_t* pszProfile )
	{
		m_strProfile = pszProfile ;
	}

	void InitAddProfile( _In_ const wchar_t* pszAddProfile )
	{
		m_strProfile = WSL2_GetModuleFileName() ;
		m_strProfile = WSL2_FileName_Path( m_strProfile.c_str() ) ;
		m_strProfile += L"\\" ;
		m_strProfile += pszAddProfile ;
	}
	
	wstring GetStr( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_opt_ const wchar_t* pszDefault = NULL )
	{
		wchar_t szRet[ 2048 ] = { 0 } ;
		GetPrivateProfileStringW( pszSession, pszKey, pszDefault, szRet, _countof( szRet ), m_strProfile.c_str() ) ;
		return szRet ;
	}
	
	wstring GetStr2( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_opt_ const wchar_t* pszDefault = NULL )
	{
		wchar_t szRet[ 2048 ] = { 0 } ;
		GetPrivateProfileStringW( pszSession, pszKey, pszDefault, szRet, _countof( szRet ), m_strProfile.c_str() ) ;
		SetStr( pszSession, pszKey, szRet ) ;
		return szRet ;
	}
	
	BOOL SetStr( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_ const wchar_t* pszValue )
	{
		return WritePrivateProfileStringW( pszSession, pszKey, pszValue, m_strProfile.c_str() ) ;
	}
	
	int GetInt( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_ int nDefault )
	{
		return GetPrivateProfileIntW( pszSession, pszKey, nDefault, m_strProfile.c_str() ) ;
	}
	
	int GetInt2( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_ int nDefault )
	{
		nDefault = GetPrivateProfileIntW( pszSession, pszKey, nDefault, m_strProfile.c_str() ) ;
		SetInt( pszSession, pszKey, nDefault ) ;
		return nDefault ;
	}
	
	BOOL SetInt( _In_ const wchar_t* pszSession, _In_ const wchar_t* pszKey, _In_ int nValue )
	{
		wstring wstr = WSL2_String_FormatW( L"%d", nValue ) ;
		return SetStr( pszSession, pszKey, wstr.c_str() ) ;
	}
	
	BOOL EnumSectionNames( vector< wstring >& vecNames )
	{
		const DWORD dwFileSize = GetCompressedFileSizeW( m_strProfile.c_str(), NULL ) ;
		if ( dwFileSize == INVALID_FILE_SIZE )
		{
			return FALSE ;
		}
		
		Cw2WString wstr = WSL2_String_New< wchar_t >( dwFileSize ) ;
		GetPrivateProfileSectionNamesW( wstr, dwFileSize, m_strProfile.c_str() ) ;

		wchar_t* pBuffer = wstr ;
		int nIndex = 0 ;
		while ( pBuffer[ nIndex ] != 0 )
		{
			vecNames.push_back( pBuffer + nIndex ) ;
			nIndex += wcslen( pBuffer + nIndex ) + 1 ;
		}
		
		return TRUE ;
	}
	
	const wchar_t* GetProfile()
	{
		return m_strProfile.c_str() ;
	}

private:
	wstring	m_strProfile ;
};

//////////////////////////////////////////////////////////////////////////

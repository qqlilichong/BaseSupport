
#pragma once

#include "WSL_IO.h"
#include "WSL_Thread.h"
#include "WSL_Sync.h"

//////////////////////////////////////////////////////////////////////////

inline wstring WSL_GetComputerName()
{
	wchar_t szName[ MAX_COMPUTERNAME_LENGTH + 1 ] = { 0 } ;
	DWORD dwCount = MAX_COMPUTERNAME_LENGTH + 1 ;
	GetComputerName( szName, &dwCount ) ;
	return szName ;
}

//////////////////////////////////////////////////////////////////////////

class CwTickCount
{
public:
	CwTickCount()
	{
		QueryPerformanceFrequency( &m_liPerfFreq ) ;
		TickStart() ;
	}

	inline void TickStart()
	{
		QueryPerformanceCounter( &m_liPerfStart ) ;
	}

	inline double TickNow()
	{
		LARGE_INTEGER liPerNow ;
		QueryPerformanceCounter( &liPerNow ) ;
		return ( ( liPerNow.QuadPart - m_liPerfStart.QuadPart ) / (double)m_liPerfFreq.QuadPart ) ;
	}

private:
	LARGE_INTEGER m_liPerfFreq	;
	LARGE_INTEGER m_liPerfStart	;
};

//////////////////////////////////////////////////////////////////////////

// bool timeval_subtract( timeval time_start, timeval time_now, double& time_duration )
// {
// 	if ( time_start.tv_sec > time_now.tv_sec )
// 	{
// 		return false ;
// 	}
// 
// 	if ( time_start.tv_sec == time_now.tv_sec )
// 	{
// 		if ( time_start.tv_usec > time_now.tv_usec )
// 		{
// 			return false ;
// 		}
// 	}
// 
// 	time_now.tv_sec = time_now.tv_sec - time_start.tv_sec ;
// 	time_now.tv_usec = time_now.tv_usec - time_start.tv_usec ;
// 	if ( time_now.tv_usec < 0 )
// 	{
// 		time_now.tv_sec-- ;
// 		time_now.tv_usec += 1000000 ;
// 	}
// 
// 	time_duration = (double)time_now.tv_sec ;
// 	time_duration += ( (double)time_now.tv_usec / 1000000 ) ;
// 	return true ;
// }

inline double timeval2double( timeval& timestamp )
{
	double ret = 0.0f ;
	ret = (double)timestamp.tv_sec ;
	ret += ( (double)timestamp.tv_usec / 1000000 ) ;
	return ret ;
}

inline bool timedur( double time_start, double time_now, double& time_duration )
{
	time_duration = time_now - time_start ;
	return ( ( time_duration < 0.0f ) ? false : true ) ;
}

//////////////////////////////////////////////////////////////////////////

class CwVa_List : public CwLocalAllocer< _TCHAR* >
{
public:
	CwVa_List() {}
	CwVa_List( _In_ const _TCHAR*& format ) { FmtCore( format ) ; }
	inline int Fmt( _In_ const _TCHAR* format, ... ) { return FmtCore( format ) ; }

protected:
	inline int FmtCore( _In_ const _TCHAR*& format )
	{
		int nErr = -1 ;
		va_list args ;
		va_start( args, format ) ;
		
		const int nNeedLen = _vsctprintf_p( format, args ) + 1 ;
		if ( Alloc( TCLEN2SIZE( nNeedLen ) ) )
		{
			nErr = _vstprintf( *this, nNeedLen, format, args ) ;
		}
		
		va_end( args ) ;
		return nErr ;
	}
};

//////////////////////////////////////////////////////////////////////////

inline wstring WSL_String2W( _In_ const char* str )
{
	const int nNeedLen = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 ) ;
	if ( nNeedLen > 0 )
	{
		CwLocalAllocer< wchar_t* > wstr ;
		if ( wstr.Alloc( nNeedLen * sizeof( wchar_t ) ) )
		{
			if ( MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, nNeedLen ) != 0 )
			{
				return (wchar_t*)wstr ;
			}
		}
	}

	return L"" ;
}

inline string WSL_String2A( _In_ const wchar_t* str )
{
	const int nNeedSize = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL ) ;
	if ( nNeedSize > 0 )
	{
		CwLocalAllocer< char* > astr ;
		if ( astr.Alloc( nNeedSize ) )
		{
			if ( WideCharToMultiByte( CP_ACP, 0, str, -1, astr, nNeedSize, NULL, NULL ) != 0 )
			{
				return (char*)astr ;
			}
		}
	}
	
	return "" ;
}

//////////////////////////////////////////////////////////////////////////

#define __WDBG_ADD2( x )	#x
#define __WDBG_ADD( x )		__WDBG_ADD2( x )
#define __WDBG_MSG()		( __FILE__ "(" __WDBG_ADD(__LINE__) "): " )

inline wstring WSL_ErrMsg( DWORD dwErr = 0 )
{
	if ( dwErr == 0 )
	{
		dwErr = GetLastError() ;
	}
	
	// Format error message.
	CwLocal fmErr ;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER 
		| FORMAT_MESSAGE_FROM_SYSTEM 
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		(LPTSTR)&fmErr, 0, NULL ) ;
	
	CwVa_List msg ;
	msg.Fmt( TEXT( "%u : %s" ), dwErr, fmErr ) ;
	return (wchar_t*)msg ;
}

inline void WSL_DbgMsg( const char* pszDBG, _In_ const wchar_t* format, ... )
{
	wstring strDBG = WSL_String2W( pszDBG ) ;
	CwVa_List func( format ) ;
	
	CwVa_List msg ;
	msg.Fmt( L"< %s%s >\n", strDBG.c_str(), func ) ;
	
	OutputDebugString( msg ) ;
	fwprintf_s( stderr, msg ) ;
}

#define WDBG( ... )		WSL_DbgMsg( __WDBG_MSG(), __VA_ARGS__ )

//////////////////////////////////////////////////////////////////////////

template< DWORD CF, BOOL INH >
bool WSL_CreateProc( _In_opt_ LPSTARTUPINFOW lpStartupInfo,
					_In_opt_ LPPROCESS_INFORMATION lpProcessInformation,
					_Inout_opt_ LPDWORD lpExitCode,
					_In_ LPCTSTR format, ... )
{
	PROCESS_INFORMATION pi = { 0 } ;
	STARTUPINFO si = { 0 } ;
	si.cb = sizeof( si ) ;
	lpStartupInfo = ( lpStartupInfo == NULL ? &si : lpStartupInfo ) ;
	lpProcessInformation = ( lpProcessInformation == NULL ? &pi : lpProcessInformation ) ;
	
	CwVa_List szCMD( format ) ;

#ifdef _DEBUG
	
	OutputDebugString( szCMD ) ;
	OutputDebugString( TEXT( "\n" ) ) ;

#endif // _DEBUG
	
	if ( !CreateProcess( NULL, szCMD, NULL, NULL, INH, CF, NULL, NULL, lpStartupInfo, lpProcessInformation ) )
	{
		return false ;
	}
	
	CloseHandle( lpProcessInformation->hThread ) ;
	lpProcessInformation->hThread = NULL ;
	if ( lpExitCode != NULL )
	{
		WaitForSingleObject( lpProcessInformation->hProcess, *lpExitCode ) ;
		GetExitCodeProcess( lpProcessInformation->hProcess, lpExitCode ) ;
	}
	
	CloseHandle( lpProcessInformation->hProcess ) ;
	lpProcessInformation->hProcess= NULL ;
	return true ;
}

template< DWORD CF, BOOL INH >
BOOL WSL_CreateEXE( _In_opt_ LPPROCESS_INFORMATION lpProcessInformation, _In_ LPCTSTR format, ... )
{
	STARTUPINFO si = { 0 } ;
	si.cb = sizeof( si ) ;
	
	CwVa_List szCMD( format ) ;

#ifdef _DEBUG

	OutputDebugString( szCMD ) ;
	OutputDebugString( TEXT( "\n" ) ) ;

#endif // _DEBUG
	
	return CreateProcess( NULL, szCMD, NULL, NULL, INH, CF, NULL, NULL, &si, lpProcessInformation ) ;
}

//////////////////////////////////////////////////////////////////////////

inline bool WSL_ProcStdIO( _In_ LPTSTR pszCMD,
						  _Out_opt_ LPDWORD lpExitCode = NULL,
						  _In_opt_ HANDLE hStdOutput = NULL,
						  _In_opt_ HANDLE hStdInput = NULL,
						  _In_opt_ HANDLE hStdError = NULL )
{
	PROCESS_INFORMATION pi = { 0 } ;
	STARTUPINFO si = { 0 } ;
	si.cb = sizeof( si ) ;
	si.hStdOutput = hStdOutput ;
	si.hStdInput = hStdInput ;
	si.hStdError = hStdError ;
	si.dwFlags |= STARTF_USESTDHANDLES ;
	DWORD dwExitCode = INFINITE ;
	if ( !WSL_CreateProc< CREATE_NO_WINDOW, TRUE >( &si, &pi, &dwExitCode, pszCMD ) )
	{
		return false ;
	}

	if ( lpExitCode != NULL )
	{
		*lpExitCode = dwExitCode ;
	}
	
	return true ;
}

inline bool WSL_ReadProcStdoutAndStderr( _Out_ wstring& strOutput, _Out_opt_ LPDWORD lpExitCode, _In_ LPCTSTR format, ... )
{
	CwPipe pipeStdoutAndStderr ;
	
	CwVa_List szCMD( format ) ;
	if ( !WSL_ProcStdIO( szCMD, lpExitCode, pipeStdoutAndStderr, NULL, pipeStdoutAndStderr ) )
	{
		return false ;
	}
	
	pipeStdoutAndStderr.FreeWrite() ;
	if ( !pipeStdoutAndStderr.Read() )
	{
		return false ;
	}
	
	strOutput = WSL_String2W( pipeStdoutAndStderr ) ;
	return true ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WSL_ModuleFileName( _In_opt_ HMODULE hMod = NULL )
{
	wchar_t szFile[ MAX_PATH ] = { 0 } ;
	GetModuleFileName( NULL, szFile, _countof( szFile ) ) ;
	return szFile ;
}

inline int WSL_DeletePath( const wchar_t* path )
{
	wchar_t file[ MAX_PATH ] = { 0 } ;
	wsprintf( file, path ) ;

	SHFILEOPSTRUCT op = { 0 } ;
	op.fFlags = FOF_NO_UI ;
	op.pFrom = file ;
	op.wFunc = FO_DELETE ;
	return SHFileOperation( &op ) ;
}

//////////////////////////////////////////////////////////////////////////

inline bool WSL_ListTheFiles( _Out_ vector< wstring >& vecResults,_In_ const wchar_t* pszTheFiles, _In_opt_ const wchar_t* pszDir = NULL )
{
	wstring strDir ;
	if ( pszDir == NULL )
	{
		strDir = WSL_SstrPathFilePath( WSL_ModuleFileName() ) ;
	}

	else
	{
		strDir = pszDir ;
	}

	wstring strFind = strDir + pszTheFiles ;
	WIN32_FIND_DATA ffd = { 0 } ;
	HANDLE hFind = FindFirstFile( strFind.c_str(), &ffd ) ;
	if ( hFind == INVALID_HANDLE_VALUE )
	{
		return false ;
	}

	do 
	{
		vecResults.push_back( strDir + ffd.cFileName ) ;

	} while ( FindNextFile( hFind, &ffd ) != 0 ) ;

	const DWORD dwErr = GetLastError() ;
	FindClose( hFind ) ;
	return dwErr == ERROR_NO_MORE_FILES ;
}

//////////////////////////////////////////////////////////////////////////

class CwProfileIni
{
public:
	CwProfileIni()
	{
		m_strProfile = WSL_ModuleFileName() ;
		m_strProfile = WSL_SstrPathFilePathName( m_strProfile ) ;
		m_strProfile += L".ini" ;
	}

	void InitProfile( _In_ const wchar_t* pszProfile )
	{
		m_strProfile = pszProfile ;
	}

	void InitAddProfile( _In_ const wchar_t* pszAddProfile )
	{
		m_strProfile = WSL_ModuleFileName() ;
		m_strProfile = WSL_SstrPathFilePath( m_strProfile ) ;
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
		CwVa_List vData ;
		vData.Fmt( L"%d", nValue ) ;
		return SetStr( pszSession, pszKey, vData ) ;
	}

	BOOL EnumSectionNames( vector< wstring >& vNames )
	{
		const DWORD dwFileSize = GetCompressedFileSize( m_strProfile.c_str(), NULL ) ;
		if ( dwFileSize == INVALID_FILE_SIZE )
		{
			return FALSE ;
		}

		CwLocalAllocer< wchar_t* > buffer ;
		if ( !buffer.Alloc( sizeof( wchar_t ) * dwFileSize ) )
		{
			return FALSE ;
		}

		GetPrivateProfileSectionNames( buffer, dwFileSize, m_strProfile.c_str() ) ;
		
		wchar_t* pBuffer = buffer ;
		int nIndex = 0 ;
		while ( pBuffer[ nIndex ] != 0 )
		{
			vNames.push_back( pBuffer + nIndex ) ;
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

__interface IContainerForVPP
{
	virtual void OnCleanupIOCPCache( _In_ ULONG_PTR vpp_id, _In_ DWORD dwNumberOfBytesTransferred, _In_ ULONG_PTR dwCompletionKey, _In_opt_ LPVOID lpOverlapped ) = 0 ;
	virtual void OnVPPSession( _In_ ULONG_PTR vpp_id, _In_ DWORD dwNumberOfBytesTransferred, _In_ ULONG_PTR dwCompletionKey, _In_opt_ LPVOID lpOverlapped ) = 0 ;
};

class CwVPP : public IThreadEngineRoutine, public CwUserSync_CS
{
public:
	CwVPP( _In_ IContainerForVPP* pContainer ) : m_engine_vpp( this ), m_pContainer( pContainer )
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		m_iocp_vpp.OpenIOCP() ;
		m_engine_vpp.EngineStart() ;
	}

	~CwVPP()
	{
		QuitVPP() ;
	}

	inline void QuitVPP()
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;

		// Send exit code.
		m_iocp_vpp.PostStatus( 0 ) ;

		// Wait vpp engine exit.
		m_engine_vpp.EngineStop() ;

		// Clean up cache.
		CleanupIOCPCache() ;

		// Free IOCP.
		m_iocp_vpp.CloseIOCP() ;
	}

	inline BOOL Push_VPP( _In_ DWORD dwNumberOfBytesTransferred, _In_ ULONG_PTR dwCompletionKey, _In_opt_ LPVOID lpOverlapped )
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		return m_iocp_vpp.PostStatus( dwNumberOfBytesTransferred, dwCompletionKey, lpOverlapped ) ;
	}

	inline bool operator == ( _In_ ULONG_PTR vpp_id )
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		return (ULONG_PTR)this == vpp_id ;
	}

private: // IThreadEngineRoutine
	virtual int OnThreadEngineRoutine()
	{
		DWORD dwNumberOfBytes = 0 ;
		ULONG_PTR ptrCompletionKey = 0 ;
		LPOVERLAPPED pOverlapped = NULL ;
		if ( m_iocp_vpp.GetStatus( &dwNumberOfBytes, &ptrCompletionKey, &pOverlapped ) )
		{
			if ( dwNumberOfBytes == 0 && ptrCompletionKey == 0 && pOverlapped == NULL ) // exit code.
			{
				return -1 ;
			}
			
			m_pContainer->OnVPPSession( (ULONG_PTR)this, dwNumberOfBytes, ptrCompletionKey, pOverlapped ) ;
		}

		return 0 ;
	}

private:
	inline void CleanupIOCPCache()
	{
		DWORD dwNumberOfBytes = 0 ;
		ULONG_PTR ptrCompletionKey = 0 ;
		LPOVERLAPPED pOverlapped = NULL ;
		while ( m_iocp_vpp.GetStatus( &dwNumberOfBytes, &ptrCompletionKey, &pOverlapped, 0 ) )
		{
			if ( dwNumberOfBytes == 0 && ptrCompletionKey == 0 && pOverlapped == NULL ) // exit code.
			{
				continue ;
			}
			
			m_pContainer->OnCleanupIOCPCache( (ULONG_PTR)this, dwNumberOfBytes, ptrCompletionKey, pOverlapped ) ;
		}
	}

private:
	IContainerForVPP*		m_pContainer	;
	CwIOCP					m_iocp_vpp		;
	CwThreadEngine			m_engine_vpp	;
};

//////////////////////////////////////////////////////////////////////////

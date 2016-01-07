
#pragma once

#include "WSL_Mem.h"
#include "WSL_String.h"
#include "WSL_Sync.h"

//////////////////////////////////////////////////////////////////////////
// Pipe
inline BOOL WSL_CrPipe( _Out_ PHANDLE hReadPipe, _Out_ PHANDLE hWritePipe )
{
	SECURITY_ATTRIBUTES sa = { 0 } ;
	sa.nLength = sizeof( sa ) ;
	sa.bInheritHandle = TRUE ;
	sa.lpSecurityDescriptor = NULL ;
	return CreatePipe( hReadPipe, hWritePipe, &sa, 0 ) ;
}

class CwPipe
{
public:
	CwPipe()
	{
		WSL_CrPipe( hReadPipe, hWritePipe ) ;
		SetHandleInformation( hReadPipe, HANDLE_FLAG_INHERIT, 0 ) ;
	}

public:
	bool Read()
	{
		DWORD dwReadySize = GetFileSize( hReadPipe, NULL ) ;
		if ( dwReadySize == INVALID_FILE_SIZE )
		{
			return false ;
		}

		if ( dwReadySize == 0 )
		{
			return false ;
		}

		if ( !m_pipeMsg.Alloc( dwReadySize + 1 ) )
		{
			return false ;
		}

		if ( !ReadFile( hReadPipe, m_pipeMsg, dwReadySize, &dwReadySize, NULL ) )
		{
			return false ;
		}

		return true ;
	}
	
	operator HANDLE () { return hWritePipe ; }
	operator char*  () { return m_pipeMsg ; }
	inline void FreeWrite() { hWritePipe.FreeHandle() ; }
	inline void FreeRead()  { hReadPipe.FreeHandle() ; }

protected:
	CwHandle hReadPipe ;
	CwHandle hWritePipe ;

protected:
	CwLocalAllocer< char* > m_pipeMsg ;
};

//////////////////////////////////////////////////////////////////////////
// File mapping

// class CwMapView
CWSLHANDLE( CwMapView, LPVOID, NULL, UnmapViewOfFile )

inline HANDLE WSL_CreateVMFileMap( _In_opt_ LPCWSTR pszFileMapName = NULL, _In_opt_ DWORD dwVMSize = 0 )
{
	if ( dwVMSize == 0 )
	{
		dwVMSize = (DWORD)WSL_VM_RegionSize() ;
	}
	
	return CreateFileMappingW( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dwVMSize, pszFileMapName ) ;
}

inline bool WSL_FileMapAddString( _In_ LPCWSTR pszFileMapName, _In_ LPCWSTR pszString )
{
	CwHandle hFileMap = OpenFileMappingW( FILE_MAP_WRITE, FALSE, pszFileMapName ) ;
	if ( hFileMap.InvalidHandle() )
	{
		return false ;
	}

	CwMapView mvWrite = MapViewOfFile( hFileMap, FILE_MAP_WRITE, 0, 0, 0 ) ;
	if ( mvWrite.InvalidHandle() )
	{
		return false ;
	}
	
	const SIZE_T nVMSize = WSL_VM_RegionSize( mvWrite ) ;
	return wcscat_s( (wchar_t*)(LPVOID)mvWrite, nVMSize / sizeof( WCHAR ), pszString ) == 0 ;
}

inline bool WSL_FileMapReadString( _In_ HANDLE hFileMap, _Out_ wstring& strOut )
{
	CwMapView mvRead = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 ) ;
	if ( mvRead.InvalidHandle() )
	{
		return false ;
	}

	strOut = (wchar_t*)(LPVOID)mvRead ;
	return true ;
}

inline bool WSL_FileMapWrite( _In_ HANDLE hFileMap, _In_ LPVOID pvData, _In_ SIZE_T nDataSize )
{
	CwMapView mvWrite = MapViewOfFile( hFileMap, FILE_MAP_WRITE, 0, 0, 0 ) ;
	if ( mvWrite.InvalidHandle() )
	{
		return false ;
	}
	
	return memcpy_s( mvWrite, WSL_VM_RegionSize( mvWrite ), pvData, nDataSize ) == 0 ;
}

inline bool WSL_FileMapRead( _In_ HANDLE hFileMap, _In_ LPVOID pvOut, _In_ SIZE_T nReadSize )
{
	CwMapView mvRead = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 ) ;
	if ( mvRead.InvalidHandle() )
	{
		return false ;
	}

	if ( WSL_VM_RegionSize( mvRead ) < nReadSize )
	{
		return false ;
	}
	
	return memcpy_s( pvOut, nReadSize, mvRead, nReadSize ) == 0 ;
}

//////////////////////////////////////////////////////////////////////////

// class CwIO
CWSLHANDLE( CwIO, HANDLE, INVALID_HANDLE_VALUE, CloseHandle )
	
inline DWORD IO_SyncReadFile( _In_ HANDLE hIO, _In_ LPVOID pBuffer, _In_ DWORD dwReadBytes )
{
	return ReadFile( hIO, pBuffer, dwReadBytes, &dwReadBytes, NULL ) ? dwReadBytes : 0 ;
}

inline DWORD IO_SyncWriteFile( _In_ HANDLE hIO, _In_ LPCVOID pBuffer, _In_ DWORD dwWriteBytes )
{
	return WriteFile( hIO, pBuffer, dwWriteBytes, &dwWriteBytes, NULL ) ? dwWriteBytes : 0 ;
}

inline LARGE_INTEGER IO_SyncFileLength( _In_ HANDLE hIO )
{
	LARGE_INTEGER li = { 0 } ;
	GetFileSizeEx( hIO, &li ) ;
	return li ;
}

//////////////////////////////////////////////////////////////////////////
// IO completion port
class CwIOCP : public CwUserSync_CS
{
public:
	CwIOCP()
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		m_iocp = NULL ;
	}

	~CwIOCP()
	{
		CloseIOCP() ;
	}

public:
	inline BOOL OpenIOCP( _In_ ULONG_PTR CompletionKey = 0, _In_ DWORD NumberOfConcurrentThreads = 0 )
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		if ( m_iocp != NULL )
		{
			return FALSE ;
		}
		
		m_iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, CompletionKey, NumberOfConcurrentThreads ) ;
		return m_iocp != NULL ;
	}

	inline BOOL CloseIOCP()
	{
		CwStackLock< CwUserSync_CS > lock( this ) ;
		if ( m_iocp == NULL )
		{
			return FALSE ;
		}
		
		CloseHandle( m_iocp ) ;
		m_iocp = NULL ;
		return TRUE ;
	}
	
	inline BOOL PostStatus( _In_ DWORD dwNumberOfBytesTransferred, _In_ ULONG_PTR dwCompletionKey = 0, _In_opt_ LPVOID lpOverlapped = NULL )
	{
		return PostQueuedCompletionStatus( m_iocp, dwNumberOfBytesTransferred, dwCompletionKey, (LPOVERLAPPED)lpOverlapped ) ;
	}
	
	inline BOOL GetStatus( _Out_ LPDWORD lpNumberOfBytes, _Out_ PULONG_PTR lpCompletionKey, _Out_ LPVOID lpOverlapped, _In_ DWORD dwMilliseconds = INFINITE )
	{
		return GetQueuedCompletionStatus( m_iocp, lpNumberOfBytes, lpCompletionKey, (LPOVERLAPPED*)lpOverlapped, dwMilliseconds ) ;
	}

protected:
	HANDLE		m_iocp ;
};

class CwNamedIOCP : public CwIOCP
{
public:
	CwNamedIOCP()
	{
		m_bProxyIOCP = FALSE ;
	}

	~CwNamedIOCP()
	{
		CleanupAll() ;
	}

public:
	BOOL OpenNamedIOCP( _In_ LPCWSTR pszFileMapName )
	{
		if ( !m_vmfm.InvalidHandle() )
		{
			return FALSE ;
		}

		try
		{
			m_vmfm = WSL_CreateVMFileMap( pszFileMapName ) ;
			if ( m_vmfm.InvalidHandle() )
			{
				throw -1 ;
			}

			// Open Exist IOCP
			if ( GetLastError() != ERROR_ALREADY_EXISTS )
			{
				throw -2 ;
			}

			m_bProxyIOCP = TRUE ;
			HANDLE hExistIOCP = NULL ;
			if ( !WSL_FileMapRead( m_vmfm, &hExistIOCP, sizeof( HANDLE ) ) )
			{
				throw -3 ;
			}

			m_iocp = hExistIOCP ;
		}

		catch ( int )
		{
			CleanupAll() ;
			return FALSE ;
		}

		return TRUE ;
	}

	BOOL CreateNamedIOCP( _In_ LPCWSTR pszFileMapName )
	{
		if ( !m_vmfm.InvalidHandle() )
		{
			return FALSE ;
		}

		try
		{
			m_vmfm = WSL_CreateVMFileMap( pszFileMapName ) ;
			if ( m_vmfm.InvalidHandle() )
			{
				throw -1 ;
			}

			// Create New IOCP
			if ( GetLastError() == ERROR_ALREADY_EXISTS )
			{
				throw -2 ;
			}

			if ( !CwIOCP::OpenIOCP() )
			{
				throw -3 ;
			}

			if ( !WSL_FileMapWrite( m_vmfm, &m_iocp, sizeof( HANDLE ) ) )
			{
				throw -4 ;
			}
		}

		catch ( int )
		{
			CleanupAll() ;
			return FALSE ;
		}

		return TRUE ;
	}

	void CleanupAll()
	{
		if ( m_bProxyIOCP )
		{
			m_iocp = NULL ;
		}

		m_bProxyIOCP = FALSE ;
		CwIOCP::CloseIOCP() ;
		m_vmfm.FreeHandle() ;
	}

protected:
	BOOL		m_bProxyIOCP ;
	CwHandle	m_vmfm ;
};

//////////////////////////////////////////////////////////////////////////

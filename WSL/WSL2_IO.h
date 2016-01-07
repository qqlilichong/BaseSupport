
#pragma once

//////////////////////////////////////////////////////////////////////////

Cw2AutoHandle( Cw2MapView, LPVOID, NULL, UnmapViewOfFile )

inline DWORD WSL2_IO_SyncReadFile( _In_ HANDLE hFile, _In_ LPVOID ptr, _In_ DWORD size )
{
	return ReadFile( hFile, ptr, size, &size, NULL ) ? size : 0 ;
}

inline DWORD WSL2_IO_SyncWriteFile( _In_ HANDLE hFile, _In_ LPVOID ptr, _In_ DWORD size )
{
	return WriteFile( hFile, ptr, size, &size, NULL ) ? size : 0 ;
}

//////////////////////////////////////////////////////////////////////////

class Cw2IOCP
{
public:
	Cw2IOCP()
	{
		InterlockedExchange64( &m_iocp_length, -1 ) ;
	}

	~Cw2IOCP()
	{
		CloseIOCP() ;
	}

public:
	inline BOOL OpenIOCP( _In_ DWORD NumberOfConcurrentThreads = 0 )
	{
		if ( m_iocp_length != -1 )
		{
			return FALSE ;
		}
		
		if ( !m_iocp.InvalidHandle() )
		{
			return FALSE ;
		}
		
		m_iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, NumberOfConcurrentThreads ) ;
		if ( m_iocp.InvalidHandle() )
		{
			return FALSE ;
		}
		
		InterlockedExchange64( &m_iocp_length, 0 ) ;
		return TRUE ;
	}
	
	inline BOOL PostStatus( _In_ DWORD dwNumberOfBytesTransferred, _In_ ULONG_PTR dwCompletionKey, _In_opt_ LPVOID lpOverlapped )
	{
		if ( PostQueuedCompletionStatus( m_iocp, dwNumberOfBytesTransferred, dwCompletionKey, (LPOVERLAPPED)lpOverlapped ) )
		{
			InterlockedIncrement64( &m_iocp_length ) ;
			return TRUE ;
		}
		
		return FALSE ;
	}
	
	inline BOOL GetStatus( _Out_ LPDWORD lpNumberOfBytesTransferred, _Out_ PULONG_PTR lpCompletionKey, _Out_ LPVOID* lpOverlapped, _In_ DWORD dwMilliseconds = INFINITE )
	{
		if ( GetQueuedCompletionStatus( m_iocp, lpNumberOfBytesTransferred, lpCompletionKey, (LPOVERLAPPED*)lpOverlapped, dwMilliseconds ) )
		{
			InterlockedDecrement64( &m_iocp_length ) ;
			return TRUE ;
		}

		return FALSE ;
	}
	
	inline LONGLONG GetLength()
	{
		return m_iocp_length ;
	}

	inline void CloseIOCP()
	{
		m_iocp.FreeHandle() ;
	}

private:
	LONGLONG volatile	m_iocp_length	;
	Cw2Handle			m_iocp			;
};

//////////////////////////////////////////////////////////////////////////

__interface IW2_IOCP_STATUS
{
	virtual int OnGetStatus( LONGLONG iocp_length, ULONG_PTR iocp_id,
		DWORD NumberOfBytesTransferred,
		ULONG_PTR CompletionKey,
		LPVOID lpOverlapped ) = 0 ;
	
	virtual int OnCleanStatus( ULONG_PTR iocp_id,
		DWORD NumberOfBytesTransferred,
		ULONG_PTR CompletionKey,
		LPVOID lpOverlapped ) = 0 ;
};

inline BOOL WSL2_IOCP_PostExitStatus( _In_ Cw2IOCP& iocp )
{
	return iocp.PostStatus( 0, 0, nullptr ) ;
}

inline BOOL WSL2_IOCP_IsExitStatus( DWORD NumberOfBytesTransferred, ULONG_PTR CompletionKey, LPVOID lpOverlapped )
{
	return ( NumberOfBytesTransferred == 0 && CompletionKey == 0 && lpOverlapped == nullptr ) ;
}

inline int WSL2_IOCP_RoutineStatus( _In_ Cw2IOCP& iocp, _In_ IW2_IOCP_STATUS* pRoutine,
								   _In_ DWORD dwMilliseconds = INFINITE, _In_ BOOL bClean = FALSE )
{
	DWORD NumberOfBytesTransferred = 0 ;
	ULONG_PTR CompletionKey = 0 ;
	LPVOID lpOverlapped = nullptr ;
	if ( !iocp.GetStatus( &NumberOfBytesTransferred, &CompletionKey, &lpOverlapped, dwMilliseconds ) )
	{
		return -1 ;
	}
	
	const ULONG_PTR iocp_id = (ULONG_PTR)&iocp ;
	if ( bClean )
	{
		return pRoutine->OnCleanStatus( iocp_id, NumberOfBytesTransferred, CompletionKey, lpOverlapped ) ;
	}
	
	return pRoutine->OnGetStatus( iocp.GetLength(), iocp_id, NumberOfBytesTransferred, CompletionKey, lpOverlapped ) ;
}

//////////////////////////////////////////////////////////////////////////

class Cw2VPP : public IThreadEngine2Routine
{
public:
	Cw2VPP( _In_ IW2_IOCP_STATUS* pSink ) : m_pSink( pSink ), m_thread( this )
	{

	}
	
	~Cw2VPP()
	{
		ExitVPP() ;
	}

public:
	inline BOOL PostVPP( DWORD NumberOfBytesTransferred, ULONG_PTR CompletionKey, LPVOID lpOverlapped )
	{
		return m_iocp.PostStatus( NumberOfBytesTransferred, CompletionKey, lpOverlapped ) ;
	}
	
	inline BOOL ExitVPP()
	{
		if ( m_pSink == nullptr )
		{
			return FALSE ;
		}
		
		if ( !WSL2_IOCP_PostExitStatus( m_iocp ) )
		{
			return FALSE ;
		}
		
		m_thread.EngineStop() ;
		while ( WSL2_IOCP_RoutineStatus( m_iocp, m_pSink, 0, TRUE ) == 0 )
		{

		}
		
		m_iocp.CloseIOCP() ;
		m_pSink = nullptr ;
		return TRUE ;
	}
	
	inline BOOL EnterVPP()
	{
		if ( m_pSink == nullptr )
		{
			return FALSE ;
		}
		
		if ( !m_iocp.OpenIOCP() )
		{
			return FALSE ;
		}
		
		if ( m_thread.EngineStart() != W2ERR_OK )
		{
			return FALSE ;
		}

		return TRUE ;
	}

private: // IThreadEngine2Routine
	virtual int OnEngineRoutine( DWORD tid )
	{
		return WSL2_IOCP_RoutineStatus( m_iocp, m_pSink ) ;
	}

private:
	IW2_IOCP_STATUS*	m_pSink		;

private:
	Cw2IOCP				m_iocp		;
	Cw2ThreadEngine2	m_thread	;
};

//////////////////////////////////////////////////////////////////////////

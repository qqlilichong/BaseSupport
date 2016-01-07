
#pragma once

//////////////////////////////////////////////////////////////////////////

__interface IThreadEngine2Routine
{
	virtual int OnEngineRoutine( DWORD tid ) = 0 ;
};

class Cw2ThreadEngine2
{
public:
	Cw2ThreadEngine2( _In_ IThreadEngine2Routine* pRoutine ) : m_pRoutine( pRoutine )
	{
		m_tid = 0 ;
		m_hExit = WSL2_CreateEvent( NULL, TRUE ) ;
	}

	~Cw2ThreadEngine2()
	{
		EngineStop() ;
	}

public:
	inline int EngineStart()
	{
		if ( !m_hThread.InvalidHandle() )
		{
			return W2ERR_NOTSUPPORT ;
		}
		
		if ( m_hExit.InvalidHandle() )
		{
			return W2ERR_NOTREADY ;
		}

		m_hThread = BEGIN_THREAD( NULL, 0, EngineRoutine, this, 0, &m_tid ) ;
		if ( m_hThread.InvalidHandle() )
		{
			return W2ERR_NOMEMORY ;
		}

		return W2ERR_OK ;
	}

	inline int EngineStop()
	{
		if ( !m_hThread.InvalidHandle() )
		{
			SetEvent( m_hExit ) ;
			WaitForSingleObject( m_hThread, INFINITE ) ;
		}
		
		m_hExit.FreeHandle() ;
		m_hThread.FreeHandle() ;
		return W2ERR_OK ;
	}

	inline BOOL EngineNeedExit()
	{
		return WaitForSingleObject( m_hExit, 0 ) != WAIT_TIMEOUT ;
	}

	inline BOOL operator == ( DWORD tid )
	{
		return m_tid == tid ;
	}

	inline DWORD GetTID()
	{
		return m_tid ;
	}

private:
	static DWORD CALLBACK EngineRoutine( LPVOID ptr )
	{
		return ( (Cw2ThreadEngine2*)ptr )->OnEngineRoutine() ;
	}

	DWORD OnEngineRoutine()
	{
		if ( m_pRoutine == nullptr )
		{
			return W2ERR_OK ;
		}

		while ( EngineNeedExit() == FALSE )
		{
			if ( m_pRoutine->OnEngineRoutine( m_tid ) != 0 )
			{
				break ;
			}
		}

		return W2ERR_OK ;
	}

private:
	IThreadEngine2Routine*	m_pRoutine	;
	DWORD					m_tid		;
	Cw2Handle				m_hExit		;
	Cw2Handle				m_hThread	;
};

//////////////////////////////////////////////////////////////////////////

#define W2THREADPOOL_MAX	64

class Cw2ThreadPool : public IThreadEngine2Routine
{
public:
	Cw2ThreadPool( _In_ IThreadEngine2Routine* pRoutine ) : m_pRoutine( pRoutine )
	{
		for ( auto& it : m_pool ) { it = nullptr ; }
	}

	~Cw2ThreadPool()
	{
		CloseThreadPool() ;
	}

public:
	inline int OpenThreadPool( _In_ DWORD pool_size )
	{
		if ( m_pRoutine == nullptr || m_pool[ 0 ] != nullptr || pool_size == 0 || pool_size > W2THREADPOOL_MAX )
		{
			return W2ERR_NOTSUPPORT ;
		}

		for ( DWORD i = 0 ; i < pool_size ; i++ )
		{
			Cw2ThreadEngine2* ptr = new Cw2ThreadEngine2( this ) ;
			if ( ptr == nullptr )
			{
				CloseThreadPool() ;
				return W2ERR_NOMEMORY ;
			}
			
			m_pool[ i ] = ptr ;
		}

		for ( DWORD i = 0 ; i < pool_size ; i++ )
		{
			m_pool[ i ]->EngineStart() ;
		}
		
		return W2ERR_OK ;
	}

	inline int CloseThreadPool()
	{
		for ( auto& it : m_pool )
		{
			if ( it != nullptr )
			{
				delete it ;
				it = nullptr ;
			}
		}
		
		return W2ERR_OK ;
	}

private: // IThreadEngine2Routine
	virtual int OnEngineRoutine( DWORD tid )
	{
		return m_pRoutine->OnEngineRoutine( tid ) ;
	}

private:
	IThreadEngine2Routine*	m_pRoutine					;
	Cw2ThreadEngine2*		m_pool[ W2THREADPOOL_MAX ]	;
};

//////////////////////////////////////////////////////////////////////////

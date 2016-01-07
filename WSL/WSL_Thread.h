
#pragma once

#include "WSL_Mem.h"

//////////////////////////////////////////////////////////////////////////

template< typename T >
class CwQuickThread
{
public:
	typedef DWORD ( WINAPI *PQT_ROUTINE)( T* ) ;
	struct SFQT_PARAT
	{
		PQT_ROUTINE		pfnStartAddr ;
		T				data ;
	};

private:
	static DWORD WINAPI SessionForQuickThread( SFQT_PARAT* pObj )
	{
		CwObjAllocer< SFQT_PARAT > para = pObj ;
		return para->pfnStartAddr( &para->data ) ;
	}

public:
	static bool Create( T* pSrc, PQT_ROUTINE pfnStartAddr )
	{
		CwObjAllocer< SFQT_PARAT > para ;
		if ( para.Alloc() )
		{
			para->pfnStartAddr = pfnStartAddr ;
			para->data = *pSrc ;
			CloseHandle( BEGINTHREAD( NULL, 0, SessionForQuickThread, para.DetachHandle(), 0, NULL ) ) ;
			return true ;
		}
		
		return false ;
	}
};

//////////////////////////////////////////////////////////////////////////

__interface IThreadEngineRoutine
{
	virtual int OnThreadEngineRoutine() = 0 ;
};

class CwThreadEngine
{
public:
	CwThreadEngine( _In_ IThreadEngineRoutine* pRoutine )
	{
		m_pRoutine = pRoutine ;
		InterlockedExchange( &m_nThreadSync, 0 ) ;
	}

	~CwThreadEngine()
	{
		EngineStop() ;
	}

public:
	BOOL EngineStart()
	{
		if ( m_pRoutine == NULL )
		{
			return FALSE ;
		}

		if ( m_hThreadEngine.InvalidHandle() && m_nThreadSync == 0 )
		{
			m_hThreadEngine = BEGINTHREAD( NULL, 0, ThreadEngineEnter, this, 0, NULL ) ;
			if ( !m_hThreadEngine.InvalidHandle() )
			{
				InterlockedExchange( &m_nThreadSync, 1 ) ;
				return TRUE ;
			}
		}

		return FALSE ;
	}

	void EngineStop()
	{
		if ( !m_hThreadEngine.InvalidHandle() && m_nThreadSync == 1 )
		{
			InterlockedExchange( &m_nThreadSync, 2 ) ;
			WaitForSingleObject( m_hThreadEngine, INFINITE ) ;
			m_hThreadEngine.FreeHandle() ;
		}
	}

	BOOL EngineNeedExit()
	{
		return m_nThreadSync == 2 ;
	}

private:
	static DWORD CALLBACK ThreadEngineEnter( LPVOID pvObj )
	{
		CwThreadEngine* pEngine = (CwThreadEngine*)pvObj ;
		if ( pEngine == NULL )
		{
			return WOERR_PARAMFAILED ;
		}

		while ( pEngine->EngineNeedExit() == FALSE )
		{
			if ( pEngine->m_pRoutine->OnThreadEngineRoutine() != 0 )
			{
				break ;
			}
		}
		
		return 0 ;
	}

private:
	IThreadEngineRoutine*	m_pRoutine		;
	long					m_nThreadSync	;
	CwHandle				m_hThreadEngine	;
};

//////////////////////////////////////////////////////////////////////////

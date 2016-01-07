
#pragma once

#include "WSL_Mem.h"

//////////////////////////////////////////////////////////////////////////

class CwUserSync_CS
{
public:
	CwUserSync_CS()
	{
		InitializeCriticalSection( &cs ) ;
	}

	~CwUserSync_CS()
	{
		DeleteCriticalSection( &cs ) ;
	}

	inline void Lock()
	{
		EnterCriticalSection( &cs ) ;
	}

	inline void Unlock()
	{
		LeaveCriticalSection( &cs ) ;
	}

	inline BOOL Try()
	{
		return TryEnterCriticalSection( &cs ) ;
	}

private:
	CRITICAL_SECTION	cs ;
};

//////////////////////////////////////////////////////////////////////////

#ifndef MUST_SUPPORT_XP

class CwUserSync_SRW
{
public:
	CwUserSync_SRW()
	{
		InitializeSRWLock( &srw ) ;
	}

	inline void Lock()
	{
		AcquireSRWLockExclusive( &srw ) ;
	}

	inline void Unlock()
	{
		ReleaseSRWLockExclusive( &srw ) ;
	}

	inline void LockEx()
	{
		AcquireSRWLockShared( &srw ) ;
	}

	inline void UnlockEx()
	{
		ReleaseSRWLockShared( &srw ) ;
	}

private:
	SRWLOCK	srw ;
};

#endif

//////////////////////////////////////////////////////////////////////////

template< typename T >
class CwStackLock
{
public:
	CwStackLock( T* pLock ) : pObj( pLock )
	{
		if ( pObj != NULL )
		{
			pObj->Lock() ;
		}
	}

	~CwStackLock()
	{
		if ( pObj != NULL )
		{
			pObj->Unlock() ;
		}
	}

	T* operator -> ()
	{
		return pObj ;
	}

private:
	T*	pObj ;
};

template< typename T >
class CwStackLockEx
{
public:
	CwStackLockEx( T* pLock ) : pObj( pLock )
	{
		if ( pObj != NULL )
		{
			pObj->LockEx() ;
		}
	}

	~CwStackLockEx()
	{
		if ( pObj != NULL )
		{
			pObj->UnlockEx() ;
		}
	}

	T* operator -> ()
	{
		return pObj ;
	}

private:
	T*	pObj ;
};

//////////////////////////////////////////////////////////////////////////

template< typename T >
class CwEventAuto
{
public:
	CwEventAuto()
	{
		m_hEventAuto = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
	}

	inline BOOL EventTime()
	{
		return SetEvent( m_hEventAuto ) ;
	}

	inline void WaitEvent()
	{
		WaitForSingleObject( m_hEventAuto, INFINITE ) ;
	}

	T& operator = ( T v )
	{
		m_tUserData = v ;
		return m_tUserData ;
	}

	operator T ()
	{
		return m_tUserData ;
	}

	operator T* ()
	{
		return &m_tUserData ;
	}

protected:
	CwHandle	m_hEventAuto ;
	T			m_tUserData ;
};

//////////////////////////////////////////////////////////////////////////

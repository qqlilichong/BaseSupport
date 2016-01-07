
#pragma once

//////////////////////////////////////////////////////////////////////////

inline SYSTEMTIME WSL2_FT2ST( ULONGLONG ft )
{
	ULARGE_INTEGER uli = { 0 } ;
	uli.QuadPart = ft ;
	
	FILETIME ff = { 0 } ;
	ff.dwHighDateTime = uli.HighPart ;
	ff.dwLowDateTime = uli.LowPart ;
	
	SYSTEMTIME st = { 0 } ;
	FileTimeToSystemTime( &ff, &st ) ;
	return st ;
}

inline ULONGLONG WSL2_ST2FT( SYSTEMTIME st )
{
	FILETIME ft = { 0 } ;
	SystemTimeToFileTime( &st, &ft ) ;
	
	ULARGE_INTEGER uli = { 0 } ;
	uli.HighPart = ft.dwHighDateTime ;
	uli.LowPart = ft.dwLowDateTime ;
	return uli.QuadPart ;
}

inline ULONGLONG WSL2_FT2LFT( ULONGLONG ft )
{
	ULARGE_INTEGER uli = { 0 } ;
	uli.QuadPart = ft ;
	
	FILETIME ff = { 0 } ;
	ff.dwHighDateTime = uli.HighPart ;
	ff.dwLowDateTime = uli.LowPart ;
	FileTimeToLocalFileTime( &ff, &ff ) ;
	
	uli.LowPart = ff.dwLowDateTime ;
	uli.HighPart = ff.dwHighDateTime ;
	return uli.QuadPart ;
}

//////////////////////////////////////////////////////////////////////////

class Cw2TickCount
{
public:
	Cw2TickCount()
	{
		QueryPerformanceFrequency( &m_liPerfFreq ) ;
		TickStart() ;
	}

	inline BOOL TickStart()
	{
		return QueryPerformanceCounter( &m_liPerfStart ) ;
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

class Cw2US_CS
{
public:
	Cw2US_CS()
	{
		InitializeCriticalSection( &m_cs ) ;
	}

	~Cw2US_CS()
	{
		DeleteCriticalSection( &m_cs ) ;
	}

	inline void Lock()
	{
		EnterCriticalSection( &m_cs ) ;
	}

	inline void Unlock()
	{
		LeaveCriticalSection( &m_cs ) ;
	}

	inline BOOL Try()
	{
		return TryEnterCriticalSection( &m_cs ) ;
	}

private:
	CRITICAL_SECTION	m_cs ;
};

//////////////////////////////////////////////////////////////////////////

#ifndef MUST_SUPPORT_XP

class Cw2US_SRW
{
public:
	Cw2US_SRW()
	{
		InitializeSRWLock( &m_srw ) ;
	}

	inline void Lock()
	{
		LockWrite() ;
	}

	inline void Unlock()
	{
		UnlockWrite() ;
	}

	inline void LockRead()
	{
		AcquireSRWLockShared( &m_srw ) ;
	}

	inline void UnlockRead()
	{
		ReleaseSRWLockShared( &m_srw ) ;
	}

	inline void LockWrite()
	{
		AcquireSRWLockExclusive( &m_srw ) ;
	}

	inline void UnlockWrite()
	{
		ReleaseSRWLockExclusive( &m_srw ) ;
	}

private:
	SRWLOCK		m_srw ;
};

#endif

//////////////////////////////////////////////////////////////////////////

template< class T >
class Cw2AutoLock
{
public:
	Cw2AutoLock( _In_ T* ptr ) : m_ptr( ptr )
	{
		if ( m_ptr != nullptr )
		{
			m_ptr->Lock() ;
		}
	}

	~Cw2AutoLock()
	{
		if ( m_ptr != nullptr )
		{
			m_ptr->Unlock() ;
			m_ptr = nullptr ;
		}
	}

	T* operator -> ()
	{
		return m_ptr ;
	}

private:
	T*		m_ptr ;
};

//////////////////////////////////////////////////////////////////////////

inline HANDLE WSL2_CreateEvent( _In_opt_ LPCTSTR lpName = NULL, _In_ BOOL bManualReset = FALSE, _In_ BOOL bInitialState = FALSE )
{
	return CreateEvent( NULL, bManualReset, bInitialState, lpName ) ;
}

//////////////////////////////////////////////////////////////////////////


#pragma once

#include "WSL_Win.h"
#include "WSL_Safe.h"

//////////////////////////////////////////////////////////////////////////
// Handler

// class CwHandle
CWSLHANDLE( CwHandle, HANDLE, NULL, CloseHandle )

// class CwLibrary
CWSLHANDLE( CwLibrary, HMODULE, NULL, FreeLibrary )

// class CwLocal
CWSLHANDLE( CwLocal, HLOCAL, NULL, LocalFree )

// class CwAligned
CWSLHANDLE( CwAligned, void*, NULL, _aligned_free )

//////////////////////////////////////////////////////////////////////////
// Allocer

// class CwObjAllocer
template< typename T >
class CwObjAllocer
{
public:
	CwObjAllocer() { m_pObj = NULL ; }
	CwObjAllocer( T* pObj ) { m_pObj = pObj ; }
	~CwObjAllocer() { FreeHandle() ; }
	
	inline void FreeHandle() { if( !InvalidHandle() ) { delete m_pObj ; m_pObj = NULL ; } }
	inline bool InvalidHandle() { return m_pObj == NULL ; }
	inline T* DetachHandle() { T* ret = m_pObj ; m_pObj = NULL ; return ret ; }
	operator T* () { return m_pObj ; }
	operator T** () { return &m_pObj ; }
	T* operator = ( T* pObj ) { FreeHandle() ; return m_pObj = pObj ; }
	T* operator -> () { return *this ; }
	inline bool Alloc() { FreeHandle() ; m_pObj = new T ; return InvalidHandle() == false ; }

protected:
	T*	m_pObj ;
};

// class CwLocalAllocer
CWSLALLOCER_BEGIN( CwLocal, LocalAlloc, UINT uFlags = LMEM_ZEROINIT )
CWSLALLOCER_ALLOCPARAM( uFlags, uBytes )
CWSLALLOCER_SIZE( return InvalidHandle() ? 0 : LocalSize( *this ) )
CWSLALLOCER_END()

// class CwAlignedAllocer
CWSLALLOCER_BEGIN( CwAligned, _aligned_malloc, size_t _Alignment )
CWSLALLOCER_ALLOCPARAM( uBytes, _Alignment )
CWSLALLOCER_SIZE( return InvalidHandle() ? 0 : _aligned_msize( *this, 0, 0 ) )
CWSLALLOCER_END()

//////////////////////////////////////////////////////////////////////////
// Interface for all framework objects in WSL.
__interface IWObjBase
{
	virtual void FreeObj() = 0 ;
	virtual int Dispatch( int nDisID, void* pDis, int nDisSize ) = 0 ;
};

#define WOBJ_CLASSNAME_ONLY( x )	public : static const wchar_t* GetClassName() { return x ; }
#define WOBJ_CLASSNAME( x )	private : void FreeObj() { delete this ; } WOBJ_CLASSNAME_ONLY( x )
#define WOBJ_DEFDISPATCH()	int Dispatch( int nDisID, void* pDis, int nDisSize ) { return -1 ; }

inline void Factory_FreeObj( _In_ IWObjBase* pObj )
{
	if ( pObj != NULL )
	{
		pObj->FreeObj() ;
	}
}

#define WOERR_OK				0L	// no error.
#define WOERR_UNKNOWN			-1L // unknown error.
#define WOERR_NOTSUPPORT		-2L // not support.
#define WOERR_NOMEMORY			-3L // no enough memory.
#define WOERR_NOTREADY			-4L // no enough parameters to run.
#define WOERR_PARAMFAILED		-5L // invalid parameters.

//////////////////////////////////////////////////////////////////////////
// Virtual Memory
inline SIZE_T WSL_VM_RegionSize( _In_opt_ LPCVOID lpAddress = NULL )
{
	MEMORY_BASIC_INFORMATION mbi = { 0 } ;
	VirtualQuery( lpAddress, &mbi, sizeof( mbi ) ) ;
	return mbi.RegionSize ;
}

//////////////////////////////////////////////////////////////////////////

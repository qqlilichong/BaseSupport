
#pragma once

#include <ddraw.h>
#pragma comment( lib, "ddraw.lib" )
#pragma comment( lib, "dxguid.lib" )

//////////////////////////////////////////////////////////////////////////
// DirectDraw Support
class CwDirectDraw
{
public:
	CwDirectDraw()
	{
		m_pDD7 = NULL ;
	}

	~CwDirectDraw()
	{
		UnInitDD() ;
	}

	bool InitExclusive( _In_ HWND hTopWnd )
	{
		if ( DD_Ready() )
		{
			return false ;
		}

		try
		{
			HMONITOR hMonitor = MonitorFromWindow( hTopWnd, MONITOR_DEFAULTTONEAREST ) ;
			if ( hMonitor == NULL )
			{
				throw -1 ;
			}

			if ( DirectDrawEnumerateExA( DDEnumCallbackEx, &hMonitor, DDENUM_ATTACHEDSECONDARYDEVICES ) != DD_OK )
			{
				throw -2 ;
			}

			m_pDD7 = (LPDIRECTDRAW7)hMonitor ;
			if ( !DD_Ready() )
			{
				throw -3 ;
			}

			if ( m_pDD7->SetCooperativeLevel( hTopWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) != DD_OK )
			{
				throw -4 ;
			}
		}

		catch ( int nError )
		{
			UnInitDD() ;
			return nError == 0 ;
		}

		return true ;
	}

	bool InitNormal( _In_ HMONITOR hMonitor )
	{
		if ( DD_Ready() )
		{
			return false ;
		}

		try
		{
			HMONITOR hOut = hMonitor ;
			if ( DirectDrawEnumerateExA( DDEnumCallbackEx, &hOut, DDENUM_ATTACHEDSECONDARYDEVICES ) != DD_OK )
			{
				throw -1 ;
			}

			if ( hOut == hMonitor ) // try to use default display.
			{
				DirectDrawCreateEx( NULL, (LPVOID*)&m_pDD7, IID_IDirectDraw7, NULL ) ;
			}

			else
			{
				m_pDD7 = (LPDIRECTDRAW7)hOut ;
			}

			if ( !DD_Ready() )
			{
				throw -3 ;
			}

			if ( m_pDD7->SetCooperativeLevel( NULL, DDSCL_NORMAL ) != DD_OK )
			{
				throw -4 ;
			}
		}

		catch ( int )
		{
			UnInitDD() ;
			return false ;
		}
		
		return true ;
	}

	bool InitNormal( _In_ HWND hWnd )
	{
		return InitNormal( MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST ) ) ;
	}

	void UnInitDD()
	{
		if ( m_pDD7 != NULL )
		{
			m_pDD7->Release() ;
			m_pDD7 = NULL ;
		}
	}
	
	inline operator LPDIRECTDRAW7 () { return m_pDD7 ; }
	inline bool Restore()
	{
		if ( !DD_Ready() )
		{
			return false ;
		}
		
		return m_pDD7->RestoreAllSurfaces() == DD_OK ;
	}

private:
	static BOOL WINAPI DDEnumCallbackEx( _In_ GUID FAR * lpGUID,
		_In_ LPSTR lpDriverDescription,
		_In_ LPSTR lpDriverName,
		_In_ LPVOID lpContext,
		_In_ HMONITOR hm )
	{
		HMONITOR* pInOut = (HMONITOR*)lpContext ;
		if ( *pInOut == hm )
		{
			LPDIRECTDRAW7 pDD7 = NULL ;
			DirectDrawCreateEx( lpGUID, (LPVOID*)&pDD7, IID_IDirectDraw7, NULL ) ;
			*pInOut = (HMONITOR)pDD7 ;
			return FALSE ;
		}

		return TRUE ;
	}

	inline bool DD_Ready() { return m_pDD7 != NULL ; }

private:
	LPDIRECTDRAW7			m_pDD7 ;
};

//////////////////////////////////////////////////////////////////////////
// DirectDraw Surface Base
class CwDirectSurface
{
public:
	CwDirectSurface()
	{
		m_pSurface = NULL ;
	}

	~CwDirectSurface()
	{
		FreeDirectSurface() ;
	}

public:
	inline bool Lock( _In_ DDSURFACEDESC2& desc, _In_ DWORD dwFlags = DDLOCK_WAIT | DDLOCK_WRITEONLY )
	{
		if ( !DS_Ready() )
		{
			return false ;
		}

		desc.dwSize = sizeof( desc ) ;
		return m_pSurface->Lock( NULL, &desc, dwFlags, NULL ) == DD_OK ;
	}

	inline bool Unlock()
	{
		if ( !DS_Ready() )
		{
			return false ;
		}

		return m_pSurface->Unlock( NULL ) == DD_OK ;
	}
	
	inline bool InitDirectSurface( _In_ const LPDIRECTDRAW7 pDD7, _In_ DDSURFACEDESC2& desc )
	{
		if ( pDD7 == NULL )
		{
			return false ;
		}
		
		if ( DS_Ready() )
		{
			return false ;
		}
		
		return pDD7->CreateSurface( &desc, &m_pSurface, NULL ) == DD_OK ;
	}

	inline void FreeDirectSurface()
	{
		if ( DS_Ready() )
		{
			m_pSurface->Release() ;
			m_pSurface = NULL ;
		}
	}

	inline DDSURFACEDESC2 GetSurfaceDesc()
	{
		DDSURFACEDESC2 desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		if ( DS_Ready() )
		{
			m_pSurface->GetSurfaceDesc( &desc ) ;
		}
		return desc ;
	}
	
	inline operator LPDIRECTDRAWSURFACE7 () { return m_pSurface ; }
	inline bool DS_Ready() { return m_pSurface != NULL ; }

protected:
	LPDIRECTDRAWSURFACE7	m_pSurface ;
};

//////////////////////////////////////////////////////////////////////////
// Primary Surface
class CwPrimarySurface : public CwDirectSurface
{
public:
	CwPrimarySurface()
	{
		m_pBackSurface = NULL ;
		m_pClipper = NULL ;
	}

	~CwPrimarySurface()
	{
		if ( m_pClipper != NULL )
		{
			m_pClipper->Release() ;
			m_pClipper = NULL ;
		}

		if ( m_pBackSurface != NULL )
		{
			m_pBackSurface->Release() ;
			m_pBackSurface = NULL ;
		}
	}

	bool InitComplexSurface( _In_ const LPDIRECTDRAW7 pDD7, _In_ DWORD dwBackBufferCount = 1 )
	{
		DDSURFACEDESC2 desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT ;
		desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY ;
		desc.dwBackBufferCount = dwBackBufferCount ;
		if ( !InitDirectSurface( pDD7, desc ) )
		{
			return false ;
		}

		DDSCAPS2 caps = { 0 } ;
		caps.dwCaps = DDSCAPS_BACKBUFFER ;
		return m_pSurface->GetAttachedSurface( &caps, &m_pBackSurface ) == DD_OK ;
	}

	bool InitNormalSurface( _In_ const LPDIRECTDRAW7 pDD7 )
	{
		DDSURFACEDESC2 desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		desc.dwFlags = DDSD_CAPS ;
		desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY ;
		return InitDirectSurface( pDD7, desc ) ;
	}

	bool BindClipper( _In_ const LPDIRECTDRAW7 pDD7, _In_ HWND hClipperWnd )
	{
		if ( pDD7 == NULL )
		{
			return false ;
		}

		if ( !DS_Ready() )
		{
			return false ;
		}

		if ( m_pBackSurface != NULL )
		{
			return false ;
		}

		if ( m_pClipper == NULL )
		{
			if ( pDD7->CreateClipper( 0, &m_pClipper, NULL ) != DD_OK )
			{
				return false ;
			}
		}

		m_pClipper->SetHWnd( 0, hClipperWnd ) ;
		m_pSurface->SetClipper( m_pClipper ) ;
		return true ;
	}

	bool Blt( _In_ LPDIRECTDRAWSURFACE7 pSurface,
		_In_ LPRECT pSrcRect = NULL, _In_ LPRECT pDstRect = NULL,
		_In_ DWORD dwFlags = DDBLT_WAIT, _In_ LPDDBLTFX lpDDBltFx = NULL )
	{
		if ( !DS_Ready() )
		{
			return false ;
		}

		if ( m_pBackSurface != NULL )
		{
			if ( m_pBackSurface->Blt( pDstRect, pSurface, pSrcRect, dwFlags, lpDDBltFx ) != DD_OK )
			{
				return false ;
			}

			return m_pSurface->Flip( NULL, DDFLIP_WAIT ) == DD_OK ;
		}

		return m_pSurface->Blt( pDstRect, pSurface, pSrcRect, dwFlags, lpDDBltFx ) == DD_OK ;
	}

private:
	LPDIRECTDRAWSURFACE7	m_pBackSurface ;
	LPDIRECTDRAWCLIPPER		m_pClipper ;
};

//////////////////////////////////////////////////////////////////////////

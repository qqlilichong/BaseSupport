
#pragma once

#include "WSL_General.h"
#include "WSL_Sync.h"
#include "WSL_DDraw.h"

//////////////////////////////////////////////////////////////////////////

class CwRect : public RECT
{
public:
	CwRect() { Empty() ; }
	~CwRect() { Empty() ; }
	
	inline void Empty() { SetRectEmpty( this ) ; }
	inline BOOL IsEmpty() { return IsRectEmpty( this ) ; }
	
	inline LONG get_X() { return left ; }
	inline void put_X( _In_ LONG x ) { left = x ; }

	inline LONG get_Y() { return top ; }
	inline void put_Y( _In_ LONG y ) { top = y ; }

	inline LONG get_Width() { return right - left ; }
	inline void put_Width( _In_ LONG w ) { right = w + left ; }

	inline LONG get_Height() { return bottom - top ; }
	inline void put_Height( _In_ LONG h ) { bottom = h + top ; }

	inline BOOL Offset( _In_ int nX, _In_ int nY )
	{
		return OffsetRect( this, nX, nY ) ;
	}

	inline BOOL DrawRectangle( _In_ HDC dc )
	{
		return Rectangle( dc, left, top, right, bottom ) ;
	}

	inline void operator = ( RECT rc )
	{
		SetRect( this, rc.left, rc.top, rc.right, rc.bottom ) ;
	}
};

//////////////////////////////////////////////////////////////////////////

__interface IMediaRender ;
__interface IMediaSurface : public IWObjBase
{
	enum MSRS_TYPE
	{
		MSRS_TYPE_NONE = 0,
		MSRS_TYPE_JPEG = MAKEFOURCC( 'J', 'P', 'E', 'G' ),
		MSRS_TYPE_PNGX = MAKEFOURCC( 'P', 'N', 'G', 'X' ),
		MSRS_TYPE_RGBX = MAKEFOURCC( 'R', 'G', 'B', 'X' ),
		MSRS_TYPE_YV12 = MAKEFOURCC( 'Y', 'V', '1', '2' ),
		MSRS_TYPE_NV12 = MAKEFOURCC( 'N', 'V', '1', '2' ),
	};
	
	virtual int InitSurface( _In_ int nWidth, _In_ int nHeight, _In_ int nDepth, _In_ MSRS_TYPE dwType, _In_ IMediaRender* pRender ) = 0 ;
	virtual SIZE_T LockSurface( _Out_ LPVOID* pSurfaceBuf, _In_ DWORD dwReserve ) = 0 ;
	virtual void UnlockSurface() = 0 ;
	virtual BITMAPINFOHEADER GetSurfaceContext() = 0 ;
};
CWSLHANDLE( CMediaSurface, IMediaSurface*, NULL, Factory_FreeObj )

class CMediaAutoLockSurface
{
public:
	CMediaAutoLockSurface( _In_ IMediaSurface* pSurface, _In_ DWORD dwReserve = 0 )
	{
		m_sizeLock = 0 ;
		m_pBits = NULL ;
		m_pSurface = pSurface ;

		if ( m_pSurface != NULL )
		{
			m_sizeLock = m_pSurface->LockSurface( &m_pBits, dwReserve ) ;
		}
	}

	~CMediaAutoLockSurface()
	{
		if ( m_pSurface != NULL && m_pBits != NULL )
		{
			m_pSurface->UnlockSurface() ;
		}

		m_sizeLock = 0 ;
		m_pBits = NULL ;
		m_pSurface = NULL ;
	}
	
	inline BOOL Ready() { return m_sizeLock > 0 ; }
	inline operator LPVOID () { return m_pBits ; }
	inline operator SIZE_T () { return m_sizeLock ; }

private:
	SIZE_T			m_sizeLock ;
	LPVOID			m_pBits ;
	IMediaSurface*	m_pSurface ;
};

//////////////////////////////////////////////////////////////////////////

__interface IMediaRender : public IWObjBase
{
	virtual BOOL InitMediaRender( _In_opt_ HWND hWnd, _In_opt_ HMONITOR hMonitor ) = 0 ;
	virtual int RenderSurface( _In_ IMediaSurface* pSurface, _In_opt_ LPRECT pRegionDst, _In_opt_ LPRECT pRegionSrc, _In_opt_ DWORD dwFlags ) = 0 ;
	virtual IMediaSurface* AllocCompatibleSurface() = 0 ;
};
CWSLHANDLE( CMediaRender, IMediaRender*, NULL, Factory_FreeObj )

//////////////////////////////////////////////////////////////////////////

class CwSurfaceLocalMemory : public IMediaSurface, public CwUserSync_SRW
{
public:
	CwSurfaceLocalMemory()
	{
		SecureZeroMemory( &m_context, sizeof( m_context ) ) ;
		m_context.biSize = sizeof( m_context ) ;
		m_context.biPlanes = 1 ;
	}

	// IWObjBase
	WOBJ_CLASSNAME( L"Surface_LocalMemory" )
	WOBJ_DEFDISPATCH()

public: // IMediaSurface
	virtual int InitSurface( _In_ int nWidth, _In_ int nHeight, _In_ int nDepth, _In_ MSRS_TYPE dwType, _In_ IMediaRender* )
	{
		CwStackLock< CwUserSync_SRW > lock( this ) ;

		switch ( dwType )
		{
		case MSRS_TYPE_RGBX :
			{
				const size_t nSize = nWidth * nHeight * nDepth ;
				if ( m_buf.Alloc( nSize ) )
				{
					m_context.biWidth = nWidth ;
					m_context.biHeight = nHeight ;
					m_context.biBitCount = nDepth * 8 ;
					m_context.biCompression = dwType ;
					m_context.biSizeImage = nSize ;
					break ;
				}
				
				return WOERR_NOMEMORY ;
			}

		case MSRS_TYPE_JPEG :
		case MSRS_TYPE_PNGX :
			{
				const size_t nSize = nDepth ;
				if ( m_buf.Alloc( nSize ) )
				{
					m_context.biWidth = nWidth ;
					m_context.biHeight = nHeight ;
					m_context.biCompression = dwType ;
					m_context.biSizeImage = nSize ;
					break ;
				}

				return WOERR_NOMEMORY ;
			}

		default:
			{
				return WOERR_NOTSUPPORT ;
			}
		}
		
		return WOERR_OK ;
	}

	virtual SIZE_T LockSurface( _Out_ LPVOID* pSurfaceBuf, _In_ DWORD dwReserve )
	{
		if ( pSurfaceBuf != NULL )
		{
			// CwUserSync_SRW Lock
			Lock() ;

			*pSurfaceBuf = m_buf ;
			return m_buf.Size() ;
		}

		return 0 ;
	}

	virtual void UnlockSurface()
	{
		// CwUserSync_SRW Unlock
		Unlock() ;
	}

	virtual BITMAPINFOHEADER GetSurfaceContext()
	{
		return m_context ;
	}

protected:
	CwLocalAllocer< void* >		m_buf ;
	BITMAPINFOHEADER			m_context ;
};

//////////////////////////////////////////////////////////////////////////
// DIB Bits To Device
class CwRenderDIBToDevice : public IMediaRender
{
public:
	CwRenderDIBToDevice()
	{
		m_dc = NULL ;
	}

	~CwRenderDIBToDevice()
	{
		if ( m_dc != NULL )
		{
			ReleaseDC( NULL, m_dc ) ;
			m_dc = NULL ;
		}
	}

	// IWObjBase
	WOBJ_CLASSNAME( L"Render_DIBToDevice" )
	WOBJ_DEFDISPATCH()

public: // IMediaRender
	virtual BOOL InitMediaRender( _In_opt_ HWND hWnd, _In_opt_ HMONITOR hMonitor )
	{
		if ( m_dc != NULL )
		{
			return FALSE ;
		}

		if ( IsWindow( hWnd ) )
		{
			m_dc = GetDC( hWnd ) ;
		}

		return m_dc != NULL ;
	}
	
	virtual int RenderSurface( _In_ IMediaSurface* pSurface, _In_ RECT rcRegionDst, _In_opt_ LPRECT pRegionSrc, _In_opt_ DWORD dwFlags )
	{
		// Version 1.0.
		// Support RGB24 or RGB32.
		if ( m_dc == NULL )
		{
			return WOERR_NOTREADY ;
		}

		if ( pSurface == NULL )
		{
			return WOERR_PARAMFAILED ;
		}

		// Check surface compatibility.
		BITMAPINFO bmi = { 0 } ;
		bmi.bmiHeader = pSurface->GetSurfaceContext() ;
		switch ( bmi.bmiHeader.biCompression )
		{
		case IMediaSurface::MSRS_TYPE_RGBX :
			{
				bmi.bmiHeader.biCompression = BI_RGB ;
				break ;
			}
		
		default : return WOERR_NOTSUPPORT ;
		}
		
		// Check render region.
		CwRect rcDest ;
		rcDest = rcRegionDst ;
		const int nRenderHeight = min( rcDest.get_Height(), bmi.bmiHeader.biHeight ) ;

		// Check flags.
		if ( dwFlags )
		{
			bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight ;
		}

		// RGB Render
		BOOL bRGBRenderOK = FALSE ;
		{
			CMediaAutoLockSurface als( pSurface ) ;
			if ( als.Ready() )
			{
				bRGBRenderOK = SetDIBitsToDevice( m_dc, rcDest.get_X(), rcDest.get_Y(),
					rcDest.get_Width(), rcDest.get_Height(), 0, 0, 0, nRenderHeight, als, &bmi, DIB_RGB_COLORS ) > 0 ;
			}
		}
		
		return bRGBRenderOK ? WOERR_OK : WOERR_UNKNOWN ;
	}

	virtual IMediaSurface* AllocCompatibleSurface()
	{
		return new CwSurfaceLocalMemory ;
	}

protected:
	HDC	m_dc ;
};

//////////////////////////////////////////////////////////////////////////
// DirectDraw Support
class CwSurfaceDirectDraw : public IMediaSurface, public CwUserSync_SRW
{
public:
	CwSurfaceDirectDraw()
	{
		SecureZeroMemory( &m_context, sizeof( m_context ) ) ;
		m_context.biSize = sizeof( m_context ) ;
		m_context.biPlanes = 1 ;
	}

public:
	// IWObjBase
	WOBJ_CLASSNAME( L"Surface_DirectSurface" )
	int Dispatch( int nDisID, void* pDis, int nDisSize )
	{
		switch ( nDisID )
		{
		case 0 :
			if ( pDis != NULL && nDisSize == sizeof( LPDIRECTDRAWSURFACE7 ) )
			{
				LPDIRECTDRAWSURFACE7* pRet = (LPDIRECTDRAWSURFACE7*)pDis ;
				*pRet = m_ds ;
				return 0 ;
			}
			break ;

		case 1 :
			Lock() ;
			return 0 ;

		case 2 :
			Unlock() ;
			return 0 ;

		case 0x100 :
			m_lock.Lock() ;
			return 0 ;

		case 0x101 :
			m_lock.Unlock() ;
			return 0 ;
		}
		
		return -1 ;
	}

public: // IMediaSurface
	virtual int InitSurface( _In_ int nWidth, _In_ int nHeight, _In_ int nDepth, _In_ MSRS_TYPE dwType, _In_ IMediaRender* pRender )
	{
		CwStackLock< CwUserSync_SRW > lock( this ) ;
		if ( pRender == NULL )
		{
			return WOERR_PARAMFAILED ;
		}
		
		if ( m_ds.DS_Ready() )
		{
			if ( IsSameSurface( nWidth, nHeight, dwType ) )
			{
				return WOERR_OK ;
			}
			
			m_ds.FreeDirectSurface() ;
		}
		
		switch ( dwType )
		{
		case MSRS_TYPE_NV12 :
		case MSRS_TYPE_YV12 :
			{
				LPDIRECTDRAW7 pDD7 = NULL ;
				if ( pRender->Dispatch( 0, (void*)&pDD7, sizeof( LPDIRECTDRAW7* ) ) != 0 )
				{
					return WOERR_NOTSUPPORT ;
				}
				
				if ( pDD7 == NULL )
				{
					return WOERR_NOTSUPPORT ;
				}
				
				if ( !InitYUVSurface( pDD7, nWidth, nHeight, dwType ) )
				{
					return WOERR_NOMEMORY ;
				}

				UpdateSurfaceContext() ;
				break ;
			}

		case MSRS_TYPE_RGBX :
			{
				LPDIRECTDRAW7 pDD7 = NULL ;
				if ( pRender->Dispatch( 0, (void*)&pDD7, sizeof( LPDIRECTDRAW7* ) ) != 0 )
				{
					return WOERR_NOTSUPPORT ;
				}

				if ( pDD7 == NULL )
				{
					return WOERR_NOTSUPPORT ;
				}

				if ( !InitSurface( pDD7, nWidth, nHeight ) )
				{
					return WOERR_NOMEMORY ;
				}

				UpdateSurfaceContext2() ;
				break ;
			}

		default:
			{
				return WOERR_NOTSUPPORT ;
			}
		}
		
		return WOERR_OK ;
	}
	
	virtual SIZE_T LockSurface( _Out_ LPVOID* pSurfaceBuf, _In_ DWORD dwReserve )
	{
		if ( pSurfaceBuf != NULL )
		{
			// CwUserSync_SRW Lock
			Lock() ;
			
			DDSURFACEDESC2 desc = { 0 } ;
			if ( m_ds.Lock( desc ) )
			{
				*pSurfaceBuf = desc.lpSurface ;
				return m_context.biSizeImage ;
			}
			
			// CwUserSync_SRW Unlock
			Unlock() ;
		}
		
		return 0 ;
	}

	virtual void UnlockSurface()
	{
		m_ds.Unlock() ;
		
		// CwUserSync_SRW Unlock
		Unlock() ;
	}

	virtual BITMAPINFOHEADER GetSurfaceContext()
	{
		return m_context ;
	}

private:
	inline bool InitYUVSurface( _In_ const LPDIRECTDRAW7 pDD7, _In_ DWORD dwWidth, _In_ DWORD dwHeight, _In_ DWORD dwFourCC )
	{
		DDPIXELFORMAT pix = { 0 } ;
		pix.dwSize = sizeof( pix ) ;
		pix.dwFlags = DDPF_FOURCC ;
		pix.dwFourCC = dwFourCC ;
		
		DDSURFACEDESC2 desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT ;
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY ;
		desc.dwWidth = dwWidth ;
		desc.dwHeight = dwHeight ;
		desc.ddpfPixelFormat = pix ;
		return m_ds.InitDirectSurface( pDD7, desc ) ;
	}

	inline bool InitSurface( _In_ const LPDIRECTDRAW7 pDD7, _In_ DWORD dwWidth, _In_ DWORD dwHeight )
	{
		DDSURFACEDESC2 desc = { 0 } ;
		desc.dwSize = sizeof( desc ) ;
		desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT ;
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY ;
		desc.dwWidth = dwWidth ;
		desc.dwHeight = dwHeight ;
		return m_ds.InitDirectSurface( pDD7, desc ) ;
	}
	
	inline void UpdateSurfaceContext()
	{
		DDSURFACEDESC2 desc = m_ds.GetSurfaceDesc() ;
		m_context.biWidth = desc.dwWidth ;
		m_context.biHeight = desc.dwHeight ;
		m_context.biCompression = desc.ddpfPixelFormat.dwFourCC ;
		m_context.biSizeImage = ( desc.lPitch * desc.dwHeight * 3 / 2 ) ;
		m_context.biClrUsed = desc.lPitch ;
	}

	inline void UpdateSurfaceContext2()
	{
		DDSURFACEDESC2 desc = m_ds.GetSurfaceDesc() ;
		m_context.biWidth = desc.dwWidth ;
		m_context.biHeight = desc.dwHeight ;
		m_context.biCompression = MSRS_TYPE_RGBX ;
		m_context.biSizeImage = ( desc.lPitch * desc.dwHeight * 4 ) ;
		m_context.biClrUsed = desc.lPitch ;
	}
	
	inline BOOL IsSameSurface( _In_ int nWidth, _In_ int nHeight, _In_ MSRS_TYPE dwType )
	{
		if ( m_context.biWidth != nWidth )
		{
			return FALSE ;
		}

		if ( m_context.biHeight != nHeight )
		{
			return FALSE ;
		}

		return m_context.biCompression == dwType ;
	}

protected:
	CwUserSync_CS		m_lock		;
	BITMAPINFOHEADER	m_context	;
	CwDirectSurface		m_ds		;
};

class CwRenderDirectDraw : public IMediaRender
{
public:
	// IWObjBase
	WOBJ_CLASSNAME( L"Render_DirectDraw" )
	int Dispatch( int nDisID, void* pDis, int nDisSize )
	{
		if ( nDisID == 0 && pDis != NULL && nDisSize == sizeof( LPDIRECTDRAW7 ) )
		{
			LPDIRECTDRAW7* pRet = (LPDIRECTDRAW7*)pDis ;
			*pRet = m_ddraw ;
			return 0 ;
		}
		
		return -1 ;
	}

public: // IMediaRender
	virtual BOOL InitMediaRender( _In_opt_ HWND hWnd, _In_opt_ HMONITOR hMonitor )
	{
		if ( !m_ddraw.InitNormal( hMonitor ) )
		{
			return FALSE ;
		}

		if ( !m_dsPrimary.InitNormalSurface( m_ddraw ) )
		{
			return FALSE ;
		}
		
		return m_dsPrimary.BindClipper( m_ddraw, hWnd ) ;
	}
	
	virtual int RenderSurface( _In_ IMediaSurface* pSurface, _In_opt_ LPRECT pRegionDst, _In_opt_ LPRECT pRegionSrc, _In_opt_ DWORD dwFlags )
	{
		// Version 1.0.
		// Support YV12 NV12 RGB
		if ( pSurface == NULL )
		{
			return WOERR_PARAMFAILED ;
		}
		
		LPDIRECTDRAWSURFACE7 pDS = NULL ;
		if ( pSurface->Dispatch( 0, (void*)&pDS, sizeof( LPDIRECTDRAWSURFACE7* ) ) != 0 )
		{
			return WOERR_NOTSUPPORT ;
		}
		
		if ( pDS == NULL )
		{
			return WOERR_NOTSUPPORT ;
		}
		
		if ( pSurface->Dispatch( 1, 0, 0 ) == 0 )
		{
			m_dsPrimary.Blt( pDS, pRegionSrc, pRegionDst ) ;
			
			pSurface->Dispatch( 2, 0, 0 ) ;
		}
		
		return WOERR_OK ;
	}
	
	virtual IMediaSurface* AllocCompatibleSurface()
	{
		return new CwSurfaceDirectDraw ;
	}

private:
	CwDirectDraw		m_ddraw ;
	CwPrimarySurface	m_dsPrimary ;
};

//////////////////////////////////////////////////////////////////////////

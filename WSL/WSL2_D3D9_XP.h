
#pragma once

#include <d3d9.h>
#pragma comment( lib, "d3d9.lib" )

//////////////////////////////////////////////////////////////////////////

class Cw2D3D9
{
public:
	Cw2D3D9()
	{
		m_d3d9 = Direct3DCreate9( D3D_SDK_VERSION ) ;
	}
	
	operator IDirect3D9* ()
	{
		return m_d3d9 ;
	}

private:
	CComQIPtr< IDirect3D9 >	m_d3d9 ;
};

//////////////////////////////////////////////////////////////////////////

class Cw2D3D9Device
{
public:
	inline HRESULT CreateDevice( _In_ IDirect3D9* pD3D9,
		_In_ HWND hWnd,
		_Out_opt_ D3DPRESENT_PARAMETERS* pp = nullptr,
		_In_ UINT BackBufferWidth = 0,
		_In_ UINT BackBufferHeight = 0,
		_In_ D3DSWAPEFFECT SwapEffect = D3DSWAPEFFECT_DISCARD,
		_In_ UINT PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE,
		_In_ DWORD Flags = 0 )
	{
		if ( pD3D9 == nullptr || m_d3d9_device != nullptr )
		{
			return S_FALSE ;
		}
		
		const UINT nAdapterCount = pD3D9->GetAdapterCount() ;
		UINT nAdapter = nAdapterCount ;
		for ( UINT i = 0 ; i < nAdapterCount ; i++ )
		{
			if ( pD3D9->GetAdapterMonitor( i ) == MonitorFromWindow( hWnd, MONITOR_DEFAULTTONULL ) )
			{
				nAdapter = i ;
				break ;
			}
		}

		if ( nAdapter == nAdapterCount )
		{
			return S_FALSE ;
		}
		
		D3DPRESENT_PARAMETERS d3dpp = { 0 } ;
		
		d3dpp.Windowed = TRUE ;
		d3dpp.BackBufferWidth = BackBufferWidth ;
		d3dpp.BackBufferHeight = BackBufferHeight ;
		d3dpp.BackBufferCount = 0 ;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN ;
		
		d3dpp.SwapEffect = SwapEffect ;
		d3dpp.PresentationInterval = PresentationInterval ;
		d3dpp.Flags = Flags ;
		
		if ( pD3D9->CreateDevice( nAdapter, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&d3dpp, &m_d3d9_device ) != S_OK )
		{
			return S_FALSE ;
		}
		
		if ( pp != nullptr )
		{
			*pp = d3dpp ;
		}
		
		return S_OK ;
	}
	
	inline HRESULT UpdateBackSurface( _In_ LPDIRECT3DSURFACE9 pSurface,
		_In_opt_ const LPRECT pDestRect = NULL, _In_opt_ const LPRECT pSrcRect = NULL,
		_In_ D3DTEXTUREFILTERTYPE Filter = D3DTEXF_LINEAR )
	{
		CComQIPtr< IDirect3DSurface9 > backsurface ;
		if ( m_d3d9_device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backsurface ) != S_OK )
		{
			return S_FALSE ;
		}
		
		return m_d3d9_device->StretchRect( pSurface, pSrcRect, backsurface, pDestRect, Filter ) ;
	}
	
	inline HRESULT CreateOffscreenSurface( _In_ UINT Width, _In_ UINT Height, _In_ D3DFORMAT Format, _Out_ IDirect3DSurface9** pp )
	{
		return m_d3d9_device->CreateOffscreenPlainSurface( Width, Height, Format, D3DPOOL_DEFAULT, pp, NULL ) ;
	}
	
	inline HRESULT Present( _In_opt_ const LPRECT pDestRect = NULL, _In_opt_ const LPRECT pSrcRect = NULL, _In_opt_ HWND hWnd = NULL )
	{
		return m_d3d9_device->Present( pSrcRect, pDestRect, hWnd, NULL ) ;
	}
	
	inline HRESULT PresentEx()
	{
		return m_d3d9_device->Present( NULL, NULL, NULL, NULL ) ;
	}
	
	inline operator IDirect3DDevice9* ()
	{
		return m_d3d9_device ;
	}

private:
	CComQIPtr< IDirect3DDevice9 >	m_d3d9_device ;
};

//////////////////////////////////////////////////////////////////////////

typedef BOOL ( CALLBACK *LPD3D9_EnumMonitor_Callback )( _In_ UINT nAdapter, _In_ HMONITOR hMonitor, _In_ LPVOID pv ) ;

inline BOOL WSL2_D3D9_EnumMonitor( _In_ IDirect3D9* pD3D9, LPD3D9_EnumMonitor_Callback pCallBack, _In_ LPVOID pv )
{
	if ( pD3D9 == nullptr || pCallBack == nullptr )
	{
		return FALSE ;
	}
	
	const UINT nAdapterCount = pD3D9->GetAdapterCount() ;
	UINT nAdapter = nAdapterCount ;
	for ( UINT i = 0 ; i < nAdapterCount ; i++ )
	{
		if ( !pCallBack( i, pD3D9->GetAdapterMonitor( i ), pv ) )
		{
			return FALSE ;
		}
	}

	return TRUE ;
}

//////////////////////////////////////////////////////////////////////////

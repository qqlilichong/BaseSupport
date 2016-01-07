
#pragma once

#include <windowsx.h>

#ifdef _WIN64
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.6000.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else // _WIN32
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include "WSL_COM.h"
#include "WSL_General.h"

//////////////////////////////////////////////////////////////////////////

#define	DLG_MSGHANDLE( hWnd, msg, func )	\
		case ( msg ) : return SetDlgMsgResult( hWnd, msg, \
		HANDLE_##msg( hWnd, wParam, lParam, func ) )

//////////////////////////////////////////////////////////////////////////

#define DLG_PROC_BEGIN( func )	\
		INT_PTR CALLBACK func( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) {\
		switch( uMsg ) {
#define DLG_PROC_MSGHANDLE( m, p )		case ( m ) : return SetDlgMsgResult( hWnd, m, HANDLE_##m( hWnd, wParam, lParam, p ) ) ;
#define DLG_PROC_MSGPRIVATE( m, p )		case ( m ) : { p( hWnd, uMsg, wParam, lParam ) ; break ; }
#define DLG_PROC_END()					default : return FALSE ; } return TRUE ; }

//////////////////////////////////////////////////////////////////////////

inline void DLG_SetIcon( HWND hWnd, int icon )
{
	SendMessage( hWnd, WM_SETICON, ICON_BIG,
		(LPARAM)LoadIcon( (HINSTANCE)GetWindowLongPtr( hWnd, GWLP_HINSTANCE ), MAKEINTRESOURCE( icon ) ) ) ;
	
	SendMessage( hWnd, WM_SETICON, ICON_SMALL,
		(LPARAM)LoadIcon( (HINSTANCE)GetWindowLongPtr( hWnd, GWLP_HINSTANCE ), MAKEINTRESOURCE( icon ) ) ) ;
}

//////////////////////////////////////////////////////////////////////////

inline void WND_Center( HWND hWnd )
{
	RECT rcWnd = { 0 } ;
	GetWindowRect( hWnd, &rcWnd ) ;
	
	RECT rcArea = { 0 } ;
	SystemParametersInfo( SPI_GETWORKAREA, NULL, &rcArea, NULL ) ;
	
	const int nX = ( ( rcArea.right - rcArea.left ) - ( rcWnd.right - rcWnd.left ) ) / 2 ;
	const int nY = ( ( rcArea.bottom - rcArea.top ) - ( rcWnd.bottom - rcWnd.top ) ) / 2 ;
	SetWindowPos( hWnd, NULL, nX, nY, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE ) ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WND_ClassName( _In_ HWND hWnd )
{
	wchar_t szClassName[ 128 ] = { 0 } ;
	GetClassName( hWnd, szClassName, _countof( szClassName ) ) ;
	return szClassName ;
}

//////////////////////////////////////////////////////////////////////////

inline wstring WND_GetText( _In_ HWND hWnd, _In_opt_ DWORD dwNeedLen = 1024 )
{
	CwLocalAllocer< wchar_t* > szText ;
	if ( szText.Alloc( dwNeedLen * sizeof( wchar_t ) ) )
	{
		GetWindowText( hWnd, szText, dwNeedLen ) ;
		return (wchar_t*)szText ;
	}

	return L"" ;
}

//////////////////////////////////////////////////////////////////////////

inline bool WND_SetText( _In_ HWND hWnd, _In_ const wchar_t* pszText = L"" )
{
	return SetWindowText( hWnd, pszText ) == TRUE ;
}

//////////////////////////////////////////////////////////////////////////
// Multi-thread Window
class CwThreadWindow
{
public:
	CwThreadWindow()
	{
		m_hWnd = NULL ;
	}

	~CwThreadWindow()
	{
		DestroyWin() ;
	}

public:
	bool CreateWin( _In_opt_ LPCTSTR lpClassName, _In_ DWORD dwStyle, _In_opt_ HWND hParent = NULL )
	{
		if ( IsWindow( m_hWnd ) )
		{
			return false ;
		}

		CwEventAuto< HWND > autoEvent ;
		autoEvent = NULL ;

		ME_PARAM mp ;
		mp.pHandler = this ;
		mp.cs.lpszClass = lpClassName ;
		mp.cs.style = dwStyle ;
		mp.cs.hwndParent = hParent ;
		mp.pEvent = &autoEvent ;
		if ( !CwQuickThread< ME_PARAM >::Create( &mp, MsgEngine ) )
		{
			return false ;
		}

		autoEvent.WaitEvent() ;

		if ( !IsWindow( autoEvent ) )
		{
			return false ;
		}

		m_hWnd = autoEvent ;
		return true ;
	}

	bool DestroyWin()
	{
		if ( IsWindow( m_hWnd ) )
		{
			SendMessage( m_hWnd, WM_CLOSE, 0, 0 ) ;
		}

		bool bRet = ( IsWindow( m_hWnd ) == false ) ;
		m_hWnd = NULL ;
		return bRet ;
	}

private:
	typedef struct tagME_PARAM
	{
		tagME_PARAM()
		{
			SecureZeroMemory( &cs, sizeof( cs ) ) ;
			pEvent = NULL ;
			pHandler = NULL ;
		}

		CREATESTRUCT cs ;
		CwEventAuto< HWND >* pEvent ;
		CwThreadWindow* pHandler ;

	} ME_PARAM, *LPME_PARAM ;

	static DWORD WINAPI MsgEngine( LPME_PARAM pParam )
	{
		// Sync create window
		{
			CREATESTRUCT& cs = pParam->cs ;
			CwEventAuto< HWND >& autoEvent = *( pParam->pEvent ) ;
			HWND hWnd = CreateWindow( cs.lpszClass, cs.lpszName,
				cs.style,
				cs.x, cs.y, cs.cx, cs.cy,
				cs.hwndParent, cs.hMenu, cs.hInstance, pParam->pHandler ) ;

			autoEvent = hWnd ;
			autoEvent.EventTime() ;

			if ( !IsWindow( hWnd ) )
			{
				return -1 ;
			}
		}

		MSG msg = { 0 } ;
		BOOL fGotMessage = FALSE ;
		while ( ( fGotMessage = GetMessage( &msg, NULL, 0, 0 ) ) != 0 && fGotMessage != -1 )
		{
			TranslateMessage( &msg ) ;
			DispatchMessage( &msg ) ;
		}

		return 0 ;
	}

public:
	static LRESULT CALLBACK MsgEnter( _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam )
	{
		switch ( Msg )
		{
		case WM_NCCREATE:
			{
				LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam ;
				SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)pCS->lpCreateParams ) ;
				return DefWindowProc( hWnd, Msg, wParam, lParam ) ;
			}
		
		case WM_DESTROY :
			{
				PostQuitMessage( 0 ) ;
				break ;
			}

		case WM_SYSCOMMAND :
			{
				if ( wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER )
				{
					return 0 ;
				}

				break ;
			}

		case WM_POWERBROADCAST :
			{
				if ( wParam == PBT_APMQUERYSUSPEND )
				{
					return BROADCAST_QUERY_DENY ;
				}

				break ;
			}
		
		default :
			{
				CwThreadWindow* pHandler = (CwThreadWindow*)GetWindowLongPtr( hWnd, GWLP_USERDATA ) ;
				if ( pHandler != NULL )
				{
					return pHandler->MsgHandler( hWnd, Msg, wParam, lParam ) ;
				}
			}
		}
		
		return DefWindowProc( hWnd, Msg, wParam, lParam ) ;
	}

protected:
	virtual LRESULT MsgHandler( _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam )
	{
		return 0 ;
	}

public:
	operator HWND () { return m_hWnd ; }

protected:
	HWND	m_hWnd ;
};

#define WMT_PROC_BEGIN( x )	\
		public :\
		static const wchar_t* GetClassName() { return x ; }\
		private :\
		virtual LRESULT MsgHandler( _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam ) {\
		switch( Msg ) {
#define WMT_PROC_MSGHANDLE( m, p )		case ( m ) : { HANDLE_##m( hWnd, wParam, lParam, p ) ; break ; }
#define WMT_PROC_MSGPRIVATE( m, p )		case ( m ) : { return p( hWnd, Msg, wParam, lParam ) ; }
#define WMT_PROC_END()					default : return DefWindowProc( hWnd, Msg, wParam, lParam ) ; } return 0 ; }

//////////////////////////////////////////////////////////////////////////

class CwcTaskbarList4 : public CwComBase
{
public:
	CwcTaskbarList4( _In_ HWND hWnd = NULL )
	{
		m_hWnd = hWnd ;
		m_pTaskbarList.CoCreateInstance( CLSID_TaskbarList ) ;
	}

	inline bool SetProgressCleanupState()
	{
		return SetProgressState( TBPF_NOPROGRESS ) ;
	}

	inline bool SetProgressWaitState()
	{
		return SetProgressState( TBPF_INDETERMINATE ) ;
	}

	inline bool SetProgressOKState()
	{
		SetProgressState( TBPF_NORMAL ) ;
		return SetProgressValue( 100, 100 ) ;
	}

	inline bool SetProgressPauseState()
	{
		SetProgressState( TBPF_PAUSED ) ;
		return SetProgressValue( 100, 100 ) ;
	}

	inline bool SetProgressErrState()
	{
		SetProgressState( TBPF_ERROR ) ;
		return SetProgressValue( 100, 100 ) ;
	}

	inline bool SetProgressState( _In_ TBPFLAG flag )
	{
		if ( m_pTaskbarList == NULL )
		{
			return false ;
		}
		
		return m_pTaskbarList->SetProgressState( m_hWnd, flag ) == S_OK ;
	}

	inline bool SetProgressValue( _In_ ULONGLONG ullCompleted, _In_ ULONGLONG ullTotal )
	{
		if ( m_pTaskbarList == NULL )
		{
			return false ;
		}
		
		return m_pTaskbarList->SetProgressValue( m_hWnd, ullCompleted, ullTotal ) == S_OK ;
	}

	inline HWND put_Window( _In_ HWND hWnd )
	{
		HWND hRet = m_hWnd ;
		m_hWnd = hWnd ;
		return hRet ;
	}

protected:
	HWND						m_hWnd ;
	CComQIPtr< ITaskbarList4 >	m_pTaskbarList ;
};

//////////////////////////////////////////////////////////////////////////

class CwAutoHideCursor
{
public:
	CwAutoHideCursor()
	{
		m_mouse_show = TRUE ;
		GetCursorPos( &m_mouse_pos ) ;
		m_mouse_tick.TickStart() ;
	}

public:
	void OnAutoHideCursor()
	{
		POINT now_pos = { 0 } ;
		GetCursorPos( &now_pos ) ;

		if ( ( now_pos.x != m_mouse_pos.x ) || ( now_pos.y != m_mouse_pos.y ) ) // mouse moved.
		{
			if ( m_mouse_show == FALSE )
			{
				while ( ShowCursor( TRUE ) < 0 ) {}
				m_mouse_show = TRUE ;
			}

			m_mouse_tick.TickStart() ;
		}

		else
		{
			if ( ( m_mouse_show == TRUE ) && ( m_mouse_tick.TickNow() > 5.0f ) )
			{
				while ( ShowCursor( FALSE ) >= 0 ) {}
				m_mouse_show = FALSE ;
			}
		}

		m_mouse_pos = now_pos ;
	}

private:
	BOOL		m_mouse_show	;
	POINT		m_mouse_pos		;
	CwTickCount	m_mouse_tick	;
};

//////////////////////////////////////////////////////////////////////////


#pragma once

#include <windowsx.h>

//////////////////////////////////////////////////////////////////////////

class Cw2Rect : public RECT
{
public:
	Cw2Rect()				{ clear() ; }
	Cw2Rect( _In_ RECT rc )	{ *this = rc ; }
	~Cw2Rect()				{ clear() ; }
	
	inline BOOL clear()	{ return SetRectEmpty( this ) ; }
	inline BOOL empty()	{ return IsRectEmpty( this ) ; }
	
	inline LONG get_x()					{ return left ; }
	inline void put_x( _In_ LONG x )	{ left = x ; }
	
	inline LONG get_y()					{ return top ; }
	inline void put_y( _In_ LONG y )	{ top = y ; }
	
	inline LONG get_width()					{ return right - left ; }
	inline void put_width( _In_ LONG w )	{ right = w + left ; }
	
	inline LONG get_height()				{ return bottom - top ; }
	inline void put_height( _In_ LONG h )	{ bottom = h + top ; }
	
	inline BOOL offset( _In_ int nX, _In_ int nY ) { return OffsetRect( this, nX, nY ) ; }
	inline BOOL draw_rect( _In_ HDC dc ) { return Rectangle( dc, left, top, right, bottom ) ; }
	
	inline Cw2Rect& operator = ( RECT rc ) { SetRect( this, rc.left, rc.top, rc.right, rc.bottom ) ; return *this ; }
};

//////////////////////////////////////////////////////////////////////////

#define W2WINDOW_PROC_BEGIN( x )																						\
		public:																											\
		static const wchar_t* GetWindowClassName() { return x ; }														\
		private:																										\
		virtual LRESULT MessageProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) {			\
		switch( uMsg ) {
#define W2WINDOW_PROC_MSGHANDLE(  m, p )		case ( m )	: return HANDLE_##m( hWnd, wParam, lParam, p )	;
#define W2WINDOW_PROC_MSGPRIVATE( m, p )		case ( m )	: return p( hWnd, uMsg, wParam, lParam )		;
#define W2WINDOW_PROC_END()						case WM_NULL : break ; } return DefWindowProc( hWnd, uMsg, wParam, lParam ) ; }

__interface IW2_WINDOW_PROC
{
	virtual LRESULT MessageProc( _In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam ) = 0 ;
};

inline LRESULT CALLBACK WSL2_WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_NCCREATE :
		{
			LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam ;
			SetWindowLongPtr( hWnd, GWLP_USERDATA, (ULONG_PTR)(IW2_WINDOW_PROC*)pCS->lpCreateParams ) ;
			break ;
		}
	
	case WM_DESTROY :
		PostQuitMessage( 0 ) ;
		break ;
	
	default :
		{
			IW2_WINDOW_PROC* pCallback = (IW2_WINDOW_PROC*)GetWindowLongPtr( hWnd, GWLP_USERDATA ) ;
			if ( pCallback != nullptr )
			{
				return pCallback->MessageProc( hWnd, uMsg, wParam, lParam ) ;
			}
			
			break ;
		}
	}
	
	return DefWindowProc( hWnd, uMsg, wParam, lParam ) ;
}

//////////////////////////////////////////////////////////////////////////

class Cw2Window : public IW2_WINDOW_PROC
{
public:
	Cw2Window()
	{
		m_hWnd = NULL ;
	}

	~Cw2Window()
	{
		CloseWin() ;
	}

public:
	BOOL CreateWin( _In_ DWORD dwStyle = WS_POPUP, _In_ const wchar_t* pszTitle = L"" )
	{
		if ( IsWindow( m_hWnd ) )
		{
			return FALSE ;
		}
		
		m_hWnd = CreateWindow( GetWindowClassName(), pszTitle,
			dwStyle, 0, 0, 0, 0, NULL, NULL, NULL, (IW2_WINDOW_PROC*)this ) ;
		
		return IsWindow( m_hWnd ) ;
	}

	BOOL CloseWin()
	{
		if ( IsWindow( m_hWnd ) )
		{
			SendMessage( m_hWnd, WM_CLOSE, 0, 0 ) ;
			m_hWnd = NULL ;
		}
		
		return TRUE ;
	}

private:
	W2WINDOW_PROC_BEGIN( L"Cw2Window" )
	W2WINDOW_PROC_END()

protected:
	HWND	m_hWnd ;
};

//////////////////////////////////////////////////////////////////////////

inline BOOL WSL2_RegisterWindowClassDefault()
{
	WNDCLASSEX wcx = { 0 } ;
	wcx.cbSize = sizeof( wcx ) ;
	wcx.style = CS_HREDRAW | CS_VREDRAW ;
	wcx.lpfnWndProc = WSL2_WindowProc ;
	wcx.hCursor = LoadCursor( NULL, IDC_ARROW ) ;
	wcx.hIcon = LoadIcon( NULL, IDI_SHIELD ) ;
	wcx.hbrBackground = GetStockBrush( NULL_BRUSH ) ;
	wcx.lpszClassName = Cw2Window::GetWindowClassName() ;
	return RegisterClassEx( &wcx ) ;
}

//////////////////////////////////////////////////////////////////////////

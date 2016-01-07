
#pragma once

#include <GdiPlus.h>
#pragma comment( lib, "Gdiplus.lib" )
using namespace Gdiplus ;

//////////////////////////////////////////////////////////////////////////

class CvGDIPlusInit
{
public:
	CvGDIPlusInit()
	{
		GdiplusStartupInput gdiplusStartupInput ;
		GdiplusStartup( &gdipToken, &gdiplusStartupInput, NULL ) ;
	}

	~CvGDIPlusInit()
	{
		GdiplusShutdown( gdipToken ) ;
	}

private:
	ULONG_PTR	gdipToken ;
};

inline bool GDIP_DrawString( _In_ HDC dc, _In_ wstring str, _In_ RECT rc,
							_In_ Color clr = Color::Yellow, _In_ const wchar_t* fn = L"Î¢ÈíÑÅºÚ" )
{
	static CvGDIPlusInit gdiplus ;
	
	SolidBrush green( clr ) ;
	Gdiplus::StringFormat strformat ;
	
	Gdiplus::Graphics memory( dc ) ;
	Gdiplus::FontFamily fontfamily( fn ) ;
	const Gdiplus::Font f( &fontfamily, ( rc.right - rc.left ) / 5.0f, FontStyleRegular, UnitPixel ) ;
	
	strformat.SetAlignment( StringAlignmentCenter ) ;
	strformat.SetLineAlignment( StringAlignmentCenter ) ;
	
	return !memory.DrawString( str.c_str(), str.length(), &f, RectF( (Gdiplus::REAL)rc.left, (Gdiplus::REAL)rc.top, Gdiplus::REAL( rc.right - rc.left ), Gdiplus::REAL( rc.bottom - rc.top ) ), &strformat, &green ) ;
}

inline BOOL GDIPlus_DrawStream( _In_ HDC dc, _In_ IStream* pStream, _In_ RECT rcRegionDst, _In_ LPRECT pRegionSrc )
{
	if ( pStream == NULL )
	{
		return FALSE ;
	}
	
	static CvGDIPlusInit gdiplus ;
	
	// Convert IStream to Image
	Gdiplus::Image image( pStream ) ;

	// Fix source rect.
	CwRect rcSrc ;
	if ( pRegionSrc != NULL )
	{
		rcSrc = *pRegionSrc ;
	}

	else
	{
		rcSrc.put_Width( image.GetWidth() ) ;
		rcSrc.put_Height( image.GetHeight() ) ;
	}

	Gdiplus::RectF rcfSrc( (float)rcSrc.get_X(), (float)rcSrc.get_Y(), 
		(float)rcSrc.get_Width(), (float)rcSrc.get_Height() ) ;

	CwRect rcDst ;
	rcDst = rcRegionDst ;
	Gdiplus::RectF rcfDest( (float)rcDst.get_X(), (float)rcDst.get_Y(), 
		(float)rcDst.get_Width(), (float)rcDst.get_Height() ) ;
	
	Gdiplus::Graphics gdipGraphics( dc ) ;
	return gdipGraphics.DrawImage( &image, rcfDest, rcfSrc.X, rcfSrc.Y, rcfSrc.Width, rcfSrc.Height, UnitPixel ) == Gdiplus::Status::Ok ;
}

//////////////////////////////////////////////////////////////////////////

inline void IStream_Release( _In_ IStream* pStream )
{
	pStream->Release() ;
}

CWSLHANDLE( CvIStream, IStream*, NULL, IStream_Release )

#pragma pack ( push, 1 )
typedef struct tagBMP_SPACE
{
	BITMAPFILEHEADER	bfh ;
	BITMAPINFOHEADER	bih ;
	BYTE				raw[ 1 ] ;
} BMP_SAPCE, *LPBMP_SAPCE ;
#pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////

class CvRenderGDIPlus : public CwRenderDIBToDevice
{
public:
	WOBJ_CLASSNAME_ONLY( L"Render_GDIPlus" )
		
	virtual BOOL RenderSurface( _In_ IMediaSurface* pSurface, _In_ RECT rcRegionDst, _In_opt_ LPRECT pRegionSrc, _In_opt_ DWORD dwFlags )
	{
		// Check params.
		if ( pSurface == NULL )
		{
			return FALSE ;
		}
		
		// Check surface compatibility.
		CwLocalAllocer< LPBMP_SAPCE > bufRGB ;
		BITMAPINFO bmi = { 0 } ;
		bmi.bmiHeader = pSurface->GetSurfaceContext() ;
		switch ( bmi.bmiHeader.biCompression )
		{
		case IMediaSurface::MSRS_TYPE_JPEG :
			break ;
		case IMediaSurface::MSRS_TYPE_PNGX :
			break ;
		case IMediaSurface::MSRS_TYPE_RGBX :
			{
				// Check buffer size.
				BITMAPINFOHEADER bih = bmi.bmiHeader ;
				bih.biCompression = BI_RGB ;

				BITMAPFILEHEADER bfh = { 0 } ;
				bfh.bfType = 0x4D42 ;
				bfh.bfOffBits = sizeof( bih ) + sizeof( bfh ) ;
				bfh.bfSize = bfh.bfOffBits + bmi.bmiHeader.biSizeImage ;
				if ( bufRGB.Alloc( bfh.bfSize ) )
				{
					bufRGB->bfh = bfh ;
					bufRGB->bih = bih ;
					break ;
				}

				return FALSE ;
			}
		default :
			return FALSE ;
		}

		// Core.
		{
			CMediaAutoLockSurface als( pSurface ) ;
			if ( !als.Ready() )
			{
				return FALSE ;
			}

			HGLOBAL hImage = als ;
			if ( bufRGB.Size() )
			{
				// Copy surface data to m_bufImage
				memcpy_s( bufRGB->raw, bufRGB.Size() - bufRGB->bfh.bfOffBits, als, bufRGB->bih.biSizeImage ) ;
				hImage = bufRGB ;
			}
			
			// Convert Surface to IStream
			CvIStream stream ;
			CreateStreamOnHGlobal( hImage, FALSE, stream ) ;
			if ( stream.InvalidHandle() )
			{
				return FALSE ;
			}
			
			return GDIPlus_DrawStream( m_dc, stream, rcRegionDst, pRegionSrc ) ;
		}
		
		return FALSE ;
	}
};

//////////////////////////////////////////////////////////////////////////

// #define GDIP_ENC_IMAGE_JPEG		L"image/jpeg"
// 
// inline CLSID WSL_GDIP_GetEncoderCLSID( const wchar_t* MimeType )
// {
// 	UINT num = 0, size = 0 ;
// 	if ( GetImageEncodersSize( &num, &size ) != Ok )
// 	{
// 		return CLSID_NULL ;
// 	}
// 
// 	if ( size == 0 )
// 	{
// 		return CLSID_NULL ;
// 	}
// 
// 	Cw2ByteBuffer buf = new byte[ size ] ;
// 	if ( buf.InvalidHandle() )
// 	{
// 		return CLSID_NULL ;
// 	}
// 
// 	ImageCodecInfo* pICI = (ImageCodecInfo*)(byte*)buf ;
// 	if ( GetImageEncoders( num, size, pICI ) != Ok )
// 	{
// 		return CLSID_NULL ;
// 	}
// 
// 	for ( UINT idx = 0 ; idx < num ; idx++ )
// 	{
// 		if ( wcscmp( pICI[ idx ].MimeType, MimeType ) == 0 )
// 		{
// 			return pICI[ idx ].Clsid ;
// 		}
// 	}
// 
// 	return CLSID_NULL ;
// }
// 
// inline int WSL2_GDIP_ImageRelease( Gdiplus::Image* pImage )
// {
// 	if ( pImage != nullptr )
// 	{
// 		delete pImage ;
// 	}
// 
// 	return 0 ;
// }
// 
// Cw2AutoHandle( Cw2GDIPImage, Gdiplus::Image*, nullptr, WSL2_GDIP_ImageRelease ) ;

// Cw2GDIPImage img_jpeg = new Image( stream_jpeg ) ;
// if ( img_jpeg.InvalidHandle() )
// {
// 	return S_FALSE ;
// }
// 
// EncoderParameters ep = { 0 } ;
// ep.Count = 1 ;
// ep.Parameter[ 0 ].Guid = EncoderQuality ;
// ep.Parameter[ 0 ].Type = EncoderParameterValueTypeLong ;
// ep.Parameter[ 0 ].NumberOfValues = 1 ;
// 
// ULONG quality = 10 ;
// ep.Parameter[ 0 ].Value = &quality ;
// 
// if ( img_rgb->Save( stream_jpeg, &g_jpeg_encoder, &ep ) != Ok )
// {
// 	return S_FALSE ;
// }

//////////////////////////////////////////////////////////////////////////

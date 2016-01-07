
#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavcodec/dxva2.h>
#include <libswresample/swresample.h>
}

#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avformat.lib" )
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avdevice.lib" )
#pragma comment( lib, "swscale.lib" )
#pragma comment( lib, "dxva2.lib" )
#pragma comment( lib, "swresample.lib" )

//////////////////////////////////////////////////////////////////////////

class Cw2FFmpegAllSupport
{
public:
	Cw2FFmpegAllSupport()
	{
		av_register_all() ;
	}
};

class Cw2FFmpegDeviceSupport
{
public:
	Cw2FFmpegDeviceSupport()
	{
		avdevice_register_all() ;
	}
};

class Cw2FFmpegAVCodecSupport
{
public:
	Cw2FFmpegAVCodecSupport()
	{
		avcodec_register_all() ;
	}
};

//////////////////////////////////////////////////////////////////////////

Cw2AutoHandle( Cw2FFmpegAVFormatContext, AVFormatContext*, nullptr, avformat_free_context ) ;
Cw2AutoHandle( Cw2FFmpegAVCodecContextOpen, AVCodecContext*, nullptr, avcodec_close ) ;
Cw2AutoHandle( Cw2FFmpegAVFrame, AVFrame*, nullptr, av_frame_free ) ;

Cw2AutoHandle( Cw2FFmpegAVIOContext, AVIOContext*, nullptr, avio_close ) ;
Cw2AutoHandle( Cw2FFmpegAVDictionary, AVDictionary*, nullptr, av_dict_free ) ;

//////////////////////////////////////////////////////////////////////////

class Cw2FFmpegPictureContext
{
public:
	Cw2FFmpegPictureContext()
	{
		ClearContext() ;
	}

	~Cw2FFmpegPictureContext()
	{
		ClearContext() ;
	}

	int ClearContext()
	{
		pix_fmt = AV_PIX_FMT_NONE ;
		width = 0 ;
		height = 0 ;
		return 0 ;
	}

	inline int GetSize()
	{
		return avpicture_get_size( pix_fmt, width, height ) ;
	}

public:
	enum AVPixelFormat	pix_fmt	;
	int					width	;
	int					height	;
};

class Cw2FFmpegPictureFrame
{
public:
	Cw2FFmpegPictureFrame( enum AVPixelFormat pix_fmt, int width, int height )
	{
		if ( avpicture_alloc( &pic, pix_fmt, width, height ) == 0 )
		{
			ctx.pix_fmt = pix_fmt ;
			ctx.width = width ;
			ctx.height = height ;
		}
	}

	Cw2FFmpegPictureFrame()
	{
		avpicture_free( &pic ) ;
	}

public:
	Cw2FFmpegPictureContext	ctx ;
	AVPicture				pic ;
};

class Cw2FFmpegPicture : public Cw2FFmpegPictureContext
{
public:
	Cw2FFmpegPicture()
	{
		FreePicture() ;
	}

	~Cw2FFmpegPicture()
	{
		FreePicture() ;
	}

	inline int FreePicture()
	{
		SecureZeroMemory( &pic, sizeof( pic ) ) ;
		return ClearContext() ;
	}

	inline int Fill( const void* ptr )
	{
		return avpicture_fill( &pic, (const uint8_t*)ptr, pix_fmt, width, height ) ;
	}

public:
	AVPicture	pic ;
};

Cw2AutoHandle( Cw2FFmpegSwsContext, SwsContext*, nullptr, sws_freeContext ) ;

class Cw2FFmpegSws
{
public:
	BOOL Init( _In_ enum AVPixelFormat pix_fmt_src, _In_ enum AVPixelFormat pix_fmt_dst,
		_In_ int width_src, _In_ int height_src,
		_In_ int width_dst = 0, _In_ int height_dst = 0, _In_ int flags = SWS_BILINEAR )
	{
		if ( !m_sws.InvalidHandle() )
		{
			return FALSE ;
		}

		try
		{
			if ( width_dst == 0 )
			{
				width_dst = width_src ;
			}

			if ( height_dst == 0 )
			{
				height_dst = height_src ;
			}

			m_sws = sws_getContext( width_src, height_src, pix_fmt_src,
				width_dst, height_dst, pix_fmt_dst, flags, nullptr, nullptr, nullptr ) ;
			if ( m_sws.InvalidHandle() )
			{
				throw -1 ;
			}

			m_src.pix_fmt = pix_fmt_src ;
			m_src.width = width_src ;
			m_src.height = height_src ;

			m_dst.pix_fmt = pix_fmt_dst ;
			m_dst.width = width_dst ;
			m_dst.height = height_dst ;
		}

		catch ( ... )
		{
			Uninit() ;
		}

		return !m_sws.InvalidHandle() ;
	}

	inline BOOL Scale( _In_ const void* src_ptr, _In_ size_t src_size,
		_In_ const void* dst_ptr, _In_ size_t dst_size )
	{
		if ( m_src.Fill( src_ptr ) != src_size )
		{
			return FALSE ;
		}

		return Scale( m_src.pic.data, m_src.pic.linesize, dst_ptr, dst_size ) ;
	}

	inline BOOL Scale( _In_ const uint8_t *const srcSlice[], _In_ const int srcStride[],
		_In_ const void* dst_ptr, _In_ size_t dst_size )
	{
		if ( m_dst.Fill( dst_ptr ) != dst_size )
		{
			return FALSE ;
		}

		return sws_scale( m_sws, srcSlice, srcStride, 0, m_src.height, m_dst.pic.data, m_dst.pic.linesize ) > 0 ;
	}

	inline BOOL Scale( _In_ const uint8_t *const srcSlice[], _In_ const int srcStride[],
		_In_ uint8_t *const dstSlice[], _In_ int dstStride[] )
	{
		return sws_scale( m_sws, srcSlice, srcStride, 0, m_src.height, dstSlice, dstStride ) > 0 ;
	}

	int Uninit()
	{
		m_sws.FreeHandle() ;
		m_dst.FreePicture() ;
		m_src.FreePicture() ;
		return 0 ;
	}

public:
	Cw2FFmpegPicture	m_src	;
	Cw2FFmpegPicture	m_dst	;
	Cw2FFmpegSwsContext	m_sws	;
};

//////////////////////////////////////////////////////////////////////////

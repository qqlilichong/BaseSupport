
#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/audio_fifo.h>
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

Cw2AutoHandle( Cw2FFmpegAVFormatContext, AVFormatContext*, nullptr, avformat_close_input ) ;
Cw2AutoHandle( Cw2FFmpegAVDictionary, AVDictionary*, nullptr, av_dict_free ) ;
Cw2AutoHandle( Cw2FFmpegAVCodecContextOpen, AVCodecContext*, nullptr, avcodec_close ) ;
Cw2AutoHandle( Cw2FFmpegAVFrame, AVFrame*, nullptr, av_frame_free ) ;
Cw2AutoHandle( Cw2FFmpegAVIOContext, AVIOContext*, nullptr, avio_close ) ;
Cw2AutoHandle( Cw2FFmpegAVAudioFifo, AVAudioFifo*, nullptr, av_audio_fifo_free ) ;

//////////////////////////////////////////////////////////////////////////

class Cw2FFmpegAVIOAuto
{
public:
	Cw2FFmpegAVIOAuto()
	{
		m_ctx = nullptr ;
	}

	~Cw2FFmpegAVIOAuto()
	{
		FinishIO() ;
	}

public:
	int InitIO( AVFormatContext* ctx, const char* url, int flags = AVIO_FLAG_READ_WRITE )
	{
		if ( IsReady() )
		{
			return -1 ;
		}
		
		if ( avio_open( m_io, url, flags ) >= 0 )
		{
			m_ctx = ctx ;
			m_ctx->pb = m_io ;
			if ( avformat_write_header( m_ctx, nullptr ) != 0 )
			{
				FinishIO() ;
				return -1 ;
			}

			return 0 ;
		}

		return -1 ;
	}

	void FinishIO()
	{
		if ( m_ctx )
		{
			av_write_trailer( m_ctx ) ;
			m_ctx->pb = nullptr ;
			m_ctx = nullptr ;
		}

		m_io.FreeHandle() ;
	}

	int WritePacket( AVPacket& pkt )
	{
		if ( IsReady() )
		{
			return av_write_frame( m_ctx, &pkt ) ;
		}

		return -1 ;
	}

	bool IsReady()
	{
		return m_ctx != nullptr ;
	}

private:
	Cw2FFmpegAVIOContext	m_io	;
	AVFormatContext*		m_ctx	;
};

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
	Cw2FFmpegPictureFrame()
	{
		AVPicture p = { 0 } ;
		pic = p ;
	}

	~Cw2FFmpegPictureFrame()
	{
		Uninit() ;
	}

	int Init( enum AVPixelFormat pix_fmt, int width, int height )
	{
		if ( avpicture_alloc( &pic, pix_fmt, width, height ) == 0 )
		{
			ctx.pix_fmt = pix_fmt ;
			ctx.width = width ;
			ctx.height = height ;
			return 0 ;
		}

		return -1 ;
	}

	void Uninit()
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

inline void WSL2_FFmpegInit()
{
	Cw2FFmpegAllSupport s1 ;
	Cw2FFmpegDeviceSupport s2 ;
	Cw2FFmpegAVCodecSupport s3 ;
}

__interface IW2_FFSINK
{
	virtual int OnAVFrame( Cw2FFmpegAVFrame& frame ) = 0 ;
	virtual int OnRenderUpdate( IDirect3DDevice9* pRender, LPRECT pDst, LPRECT pSrc ) = 0 ;
	virtual int OnSurfaceReady( IDirect3DSurface9* pSurface ) = 0 ;
	virtual int OnAudioReady( Cw2FFmpegAVAudioFifo& fifo ) = 0 ;
};

class Cw2FFSink : public IW2_FFSINK
{
public:
	virtual int OnAVFrame( Cw2FFmpegAVFrame& frame )
	{
		return -1 ;
	}

	virtual int OnRenderUpdate( IDirect3DDevice9* pRender, LPRECT pDst, LPRECT pSrc )
	{
		return -1 ;
	}

	virtual int OnSurfaceReady( IDirect3DSurface9* pSurface )
	{
		return -1 ;
	}

	virtual int OnAudioReady( Cw2FFmpegAVAudioFifo& fifo )
	{
		return -1 ;
	}
};

class Cw2FFSinkList
{
public:
	void AddSink( IW2_FFSINK* pSink )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		if ( pSink )
		{
			m_list.push_back( pSink ) ;
		}
	}

	void RemoveSink( IW2_FFSINK* pSink )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		if ( pSink )
		{
			m_list.remove( pSink ) ;
		}
	}

	int GetSinkSize()
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;
		return m_list.size() ;
	}

	void Invoke_OnAVFrame( Cw2FFmpegAVFrame& frame )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		for ( auto& sink : m_list )
		{
			sink->OnAVFrame( frame ) ;
		}
	}

	void Invoke_OnRenderUpdate( IDirect3DDevice9* pRender, LPRECT pDst, LPRECT pSrc )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		for ( auto& sink : m_list )
		{
			sink->OnRenderUpdate( pRender, pDst, pSrc ) ;
		}
	}

	void Invoke_OnSurfaceReady( IDirect3DSurface9* pSurface )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		for ( auto& sink : m_list )
		{
			sink->OnSurfaceReady( pSurface ) ;
		}
	}

	void Invoke_OnAudioReady( Cw2FFmpegAVAudioFifo& fifo )
	{
		Cw2AutoLock< decltype( m_lock ) > lock( &m_lock ) ;

		for ( auto& sink : m_list )
		{
			sink->OnAudioReady( fifo ) ;
		}
	}

private:
	list< IW2_FFSINK* >		m_list	;
	Cw2US_CS				m_lock	;
};

//////////////////////////////////////////////////////////////////////////

class Cw2FileEncoder : public Cw2FFSink
{
public:
	Cw2FileEncoder()
	{
		m_audio_pts = 0 ;
		m_video_pts.TickStart() ;
	}
	
	~Cw2FileEncoder()
	{
		Uninit() ;
	}

public:
	int OpenFile( const char* url )
	{
		Cw2FFmpegAVFormatContext& avfc_file = m_avfc_file ;

		if ( avfc_file.InvalidHandle() )
		{
			avfc_file = avformat_alloc_context() ;
			avfc_file->oformat = av_guess_format( nullptr, url, nullptr ) ;
			if ( avfc_file->oformat == nullptr )
			{
				return -1 ;
			}

			return 0 ;
		}

		return -1 ;
	}

	int AddVideoStream( int width, int height, int bitrate, map< string, string >* options )
	{
		Cw2FFmpegAVFormatContext& avfc_file = m_avfc_file ;
		Cw2FFmpegAVCodecContextOpen& encoder_video = m_encoder_video ;

		if ( encoder_video.InvalidHandle() )
		{
			auto codec_video = avcodec_find_encoder( AV_CODEC_ID_H264 ) ;
			auto stream_video = avformat_new_stream( avfc_file, codec_video ) ;
			if ( stream_video == nullptr )
			{
				return -1 ;
			}

			encoder_video = stream_video->codec ;
			encoder_video->pix_fmt = AV_PIX_FMT_YUV420P ;
			encoder_video->width = width ;
			encoder_video->height = height ;
			encoder_video->time_base.num = 1 ;
			encoder_video->time_base.den = 30 ;
			encoder_video->bit_rate = bitrate ;
			encoder_video->gop_size = 250 ;
			encoder_video->me_range = 16 ;
			encoder_video->max_qdiff = 4 ;
			encoder_video->qcompress = 0.6f ;
			encoder_video->qmin = 10 ;
			encoder_video->qmax = 51 ;
			encoder_video->max_b_frames = 3 ;
			encoder_video->flags |= CODEC_FLAG_GLOBAL_HEADER ;

			stream_video->time_base = encoder_video->time_base ;
			encoder_video->opaque = stream_video ;
			
			Cw2FFmpegAVDictionary encoder_options ;
			if ( options )
			{
				for ( auto& i : *options )
				{
					av_dict_set( encoder_options, i.first.c_str(), i.second.c_str(), 0 ) ;
				}

				// preset
				// ultrafast superfast veryfast faster fast medium slow slower veryslow placebo

				// crf
				// 0 - 51

				// tune
				// film animation grain stillimage psnr ssim fastdecode zerolatency

				// qp
				// 0

				// profile
				// baseline,main.high,high10,high422,high444
			}

			if ( avcodec_open2( encoder_video, codec_video, encoder_options ) != 0 )
			{
				return -1 ;
			}
		}

		m_video_pts.TickStart() ;
		return 0 ;
	}
	
	int AddAudioStream( AVCodecContext* decoder_audio_ptr, int bitrate, map< string, string >* options )
	{
		Cw2FFmpegAVFormatContext& avfc_file = m_avfc_file ;
		Cw2FFmpegAVCodecContextOpen& encoder_audio = m_encoder_audio ;
		
		if ( encoder_audio.InvalidHandle() )
		{
			auto codec_audio = avcodec_find_encoder( AV_CODEC_ID_AAC ) ;
			auto stream_audio = avformat_new_stream( avfc_file, codec_audio ) ;
			if ( stream_audio == nullptr )
			{
				return -1 ;
			}

			encoder_audio = stream_audio->codec ;
			encoder_audio->channels = decoder_audio_ptr->channels ;
			encoder_audio->channel_layout = av_get_default_channel_layout( encoder_audio->channels ) ;
			encoder_audio->sample_rate = decoder_audio_ptr->sample_rate ;
			encoder_audio->sample_fmt = decoder_audio_ptr->sample_fmt ;
			encoder_audio->bit_rate = bitrate ;
			encoder_audio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER ;
			
			stream_audio->time_base.num = 1 ;
			stream_audio->time_base.den = encoder_audio->sample_rate ;
			encoder_audio->opaque = stream_audio ;

			Cw2FFmpegAVDictionary encoder_options ;
			if ( options )
			{
				for ( auto& i : *options )
				{
					av_dict_set( encoder_options, i.first.c_str(), i.second.c_str(), 0 ) ;
				}
			}

			if ( avcodec_open2( encoder_audio, codec_audio, encoder_options ) != 0 )
			{
				return -1 ;
			}
		}
		
		m_audio_pts = 0 ;
		return 0 ;
	}

	int OpenStreamIO( const char* url )
	{
		return m_io_file.InitIO( m_avfc_file, url ) ;
	}

	void Uninit()
	{
		m_io_file.FinishIO() ;
		m_encoder_audio.FreeHandle() ;
		m_encoder_video.FreeHandle() ;
		m_avfc_file.FreeHandle() ;
	}

private:
	virtual int OnAudioReady( Cw2FFmpegAVAudioFifo& fifo )
	{
		auto& encoder_audio = m_encoder_audio ;
		if ( encoder_audio.InvalidHandle() )
		{
			return 0 ;
		}

		auto& io_file = m_io_file ;
		auto audio_stream = (AVStream*)encoder_audio->opaque ;

		while ( av_audio_fifo_size( fifo ) >= encoder_audio->frame_size )
		{
			Cw2FFmpegAVFrame layout_frame_audio = av_frame_alloc() ;
			layout_frame_audio->nb_samples = encoder_audio->frame_size ;
			layout_frame_audio->channel_layout = encoder_audio->channel_layout ;
			layout_frame_audio->format = encoder_audio->sample_fmt ;
			layout_frame_audio->sample_rate = encoder_audio->sample_rate ;
			av_frame_get_buffer( layout_frame_audio, 0 ) ;

			if ( av_audio_fifo_read( fifo, (void**)layout_frame_audio->data, layout_frame_audio->nb_samples ) != layout_frame_audio->nb_samples )
			{
				break ;
			}

			AVPacket enc_pkt ;
			av_init_packet( &enc_pkt ) ;
			enc_pkt.data = nullptr ;
			enc_pkt.size = 0 ;
			
			layout_frame_audio->pts = m_audio_pts ;
			m_audio_pts += layout_frame_audio->nb_samples ;
			
			int enc_ok = 0 ;
			avcodec_encode_audio2( encoder_audio, &enc_pkt, layout_frame_audio, &enc_ok ) ;
			if ( enc_ok )
			{
				enc_pkt.stream_index = audio_stream->index ;
				io_file.WritePacket( enc_pkt ) ;
			}
			
			av_free_packet( &enc_pkt ) ;
		}
		
		return 0 ;
	}

	virtual int OnSurfaceReady( IDirect3DSurface9* pSurface )
	{
		if ( m_encoder_video.InvalidHandle() )
		{
			return 0 ;
		}

		Cw2FFmpegAVCodecContextOpen& encoder_video = m_encoder_video ;
		auto video_stream = (AVStream*)encoder_video->opaque ;
		Cw2FFmpegAVIOAuto& io_file = m_io_file ;

		D3DSURFACE_DESC desc ;
		if ( pSurface->GetDesc( &desc ) != 0 )
		{
			return 0 ;
		}

		if ( desc.Width != encoder_video->width )
		{
			return 0 ;
		}

		if ( desc.Height != encoder_video->height )
		{
			return 0 ;
		}

		AVPixelFormat pix_fmt = AV_PIX_FMT_NONE ;
		switch ( desc.Format )
		{
		case D3DFMT_X8R8G8B8 :
			pix_fmt = AV_PIX_FMT_BGRA ;
			break ;

		default :
			return 0 ;
		}

		Cw2FFmpegSws sws ;
		if ( !sws.Init( pix_fmt, m_encoder_video->pix_fmt, encoder_video->width, encoder_video->height ) )
		{
			return 0 ;
		}

		Cw2FFmpegPictureFrame encode_frame ;
		if ( encode_frame.Init( sws.m_dst.pix_fmt, sws.m_dst.width, sws.m_dst.height ) != 0 )
		{
			return 0 ;
		}

		bool sws_ok = false ;
		D3DLOCKED_RECT d3dlr = { 0 } ;
		if ( pSurface->LockRect( &d3dlr, NULL, D3DLOCK_DONOTWAIT ) == 0 )
		{
			AVPicture src_pic = { 0 } ;
			src_pic.data[ 0 ] = (byte*)d3dlr.pBits ;
			src_pic.linesize[ 0 ] = d3dlr.Pitch ;

			if ( sws.Scale( src_pic.data, src_pic.linesize, encode_frame.pic.data, encode_frame.pic.linesize ) )
			{
				sws_ok = true ;
			}

			pSurface->UnlockRect() ;
		}

		if ( !sws_ok )
		{
			return 0 ;
		}

		AVPacket enc_pkt ;
		av_init_packet( &enc_pkt ) ;
		enc_pkt.data = nullptr ;
		enc_pkt.size = 0 ;

		AVFrame enc_frame = { 0 } ;
		enc_frame.format = encode_frame.ctx.pix_fmt ;
		enc_frame.width = encode_frame.ctx.width ;
		enc_frame.height = encode_frame.ctx.height ;
		enc_frame.data[ 0 ] = encode_frame.pic.data[ 0 ] ;
		enc_frame.data[ 1 ] = encode_frame.pic.data[ 1 ] ;
		enc_frame.data[ 2 ] = encode_frame.pic.data[ 2 ] ;
		enc_frame.linesize[ 0 ] = encode_frame.pic.linesize[ 0 ] ;
		enc_frame.linesize[ 1 ] = encode_frame.pic.linesize[ 1 ] ;
		enc_frame.linesize[ 2 ] = encode_frame.pic.linesize[ 2 ] ;

		if ( m_audio_pts == 0 )
		{
			m_video_pts.TickNow() ;
		}

		enc_frame.pts = int64_t( m_video_pts.TickNow() / av_q2d( video_stream->time_base ) ) ;

		int enc_ok = 0 ;
		avcodec_encode_video2( encoder_video, &enc_pkt, &enc_frame, &enc_ok ) ;
		if ( enc_ok )
		{
			enc_pkt.stream_index = video_stream->index ;
			io_file.WritePacket( enc_pkt ) ;
		}

		av_free_packet( &enc_pkt ) ;

		return 0 ;
	}

private:
	Cw2FFmpegAVFormatContext	m_avfc_file		;
	
	Cw2FFmpegAVCodecContextOpen m_encoder_video	;
	Cw2TickCount				m_video_pts		;
	
	Cw2FFmpegAVCodecContextOpen m_encoder_audio	;
	int64_t						m_audio_pts		;

	Cw2FFmpegAVIOAuto			m_io_file		;
};

//////////////////////////////////////////////////////////////////////////

class Cw2DrawPad : public Cw2FFSink, public Cw2FFSinkList, public IW2_IOCP_STATUS
{
public:
	Cw2DrawPad() : m_vpp_video( this ), m_vpp_audio( this )
	{
		m_vpp_video.EnterVPP() ;
		m_vpp_audio.EnterVPP() ;
	}

public:
	void AddViewRect( RECT rc )
	{
		Cw2AutoLock< decltype( m_viewmap_lock ) > lock( &m_viewmap_lock ) ;
		
		RCSTATUS rcs ;
		rcs.rcView = rc ;
		rcs.nStatus = 0 ;
		m_viewmap.push_back( rcs ) ;
	}

private:
	virtual int OnAVFrame( Cw2FFmpegAVFrame& frame )
	{
		if ( this->GetSinkSize() == 0 )
		{
			return 0 ;
		}

		const auto nb_samples = frame->nb_samples ;
		if ( nb_samples == 0 )
		{
			return 0 ;
		}

		Cw2AutoLock< decltype( m_fifo_audio_lock ) > lock( &m_fifo_audio_lock ) ;

		if ( m_fifo_audio.InvalidHandle() )
		{
			m_fifo_audio = av_audio_fifo_alloc( (AVSampleFormat)frame->format, frame->channels, 1 ) ;
			if ( m_fifo_audio.InvalidHandle() )
			{
				return 0 ;
			}
		}

		if ( av_audio_fifo_realloc( m_fifo_audio, nb_samples ) < 0 )
		{
			return 0 ;
		}
		
		if ( av_audio_fifo_write( m_fifo_audio, (void**)frame->data, nb_samples ) != nb_samples )
		{
			return 0 ;
		}
		
		m_vpp_audio.PostVPP( 0, AVMEDIA_TYPE_AUDIO, nullptr ) ;
		return 0 ;
	}

	virtual int OnRenderUpdate( IDirect3DDevice9* pRender, LPRECT pDst, LPRECT pSrc )
	{
		if ( this->GetSinkSize() == 0 )
		{
			return 0 ;
		}

		if ( UpdateViewMap( pDst ) )
		{
			m_vpp_video.PostVPP( 0, AVMEDIA_TYPE_VIDEO, pRender ) ;
		}

		return 0 ;
	}

	bool UpdateViewMap( LPRECT pRC )
	{
		Cw2AutoLock< decltype( m_viewmap_lock ) > lock( &m_viewmap_lock ) ;

		if ( pRC == nullptr )
		{
			return false ;
		}

		RECT& rc = *pRC ;

		for ( auto& view : m_viewmap )
		{
			if ( view.rcView.left != rc.left )
			{
				continue ;
			}

			if ( view.rcView.top != rc.top )
			{
				continue ;
			}

			if ( view.rcView.right != rc.right )
			{
				continue ;
			}

			if ( view.rcView.bottom != rc.bottom )
			{
				continue ;
			}

			view.nStatus = 1 ;

			for ( auto& view : m_viewmap )
			{
				if ( view.nStatus == 0 )
				{
					return false ;
				}
			}

			for ( auto& view : m_viewmap )
			{
				view.nStatus = 0 ;
			}

			return true ;
		}

		return false ;
	}

	struct RCSTATUS
	{
		RECT	rcView	;
		int		nStatus	;
	};

private:
	virtual int OnGetStatus( LONGLONG iocp_length, ULONG_PTR iocp_id,
		DWORD NumberOfBytesTransferred,
		ULONG_PTR CompletionKey,
		LPVOID lpOverlapped )
	{
		if ( NumberOfBytesTransferred == 0 && CompletionKey == 0 && lpOverlapped == nullptr )
		{
			return -1 ;
		}

		if ( CompletionKey == AVMEDIA_TYPE_VIDEO )
		{
			auto pRender = (IDirect3DDevice9*)lpOverlapped ;

			CComQIPtr< IDirect3DSurface9 > backsurface ;
			if ( pRender->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backsurface ) != 0 )
			{
				return 0 ;
			}

			D3DSURFACE_DESC desc ;
			if ( backsurface->GetDesc( &desc ) != 0 )
			{
				return 0 ;
			}

			CComQIPtr< IDirect3DSurface9 > copysurface ;
			if ( pRender->CreateOffscreenPlainSurface( desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &copysurface, NULL ) != 0 )
			{
				return 0 ;
			}

			if ( pRender->GetRenderTargetData( backsurface, copysurface ) != 0 )
			{
				return 0 ;
			}

			this->Invoke_OnSurfaceReady( copysurface ) ;
		}

		else if ( CompletionKey == AVMEDIA_TYPE_AUDIO )
		{
			Cw2AutoLock< decltype( m_fifo_audio_lock ) > lock( &m_fifo_audio_lock ) ;
			this->Invoke_OnAudioReady( m_fifo_audio ) ;
		}

		return 0 ;
	}

	virtual int OnCleanStatus( ULONG_PTR iocp_id,
		DWORD NumberOfBytesTransferred,
		ULONG_PTR CompletionKey,
		LPVOID lpOverlapped )
	{
		return 0 ;
	}

private:
	list< RCSTATUS >		m_viewmap			;
	Cw2US_CS				m_viewmap_lock		;
	
	Cw2VPP					m_vpp_video			;
	
	Cw2VPP					m_vpp_audio			;
	Cw2FFmpegAVAudioFifo	m_fifo_audio		;
	Cw2US_CS				m_fifo_audio_lock	;
};

//////////////////////////////////////////////////////////////////////////

class Cw2DrawWnd : public Cw2FFSink, public Cw2FFSinkList
{
public:
	Cw2DrawWnd()
	{
		RECT rc = { 0 } ;
		m_rc = rc ;
		m_draw_wnd = NULL ;
		m_d3dev_ptr = nullptr ;
	}

public:
	int Init( HWND hWnd, RECT rc, Cw2D3D9Device* ptr )
	{
		if ( ptr == nullptr || m_d3dev_ptr != nullptr )
		{
			return -1 ;
		}

		m_d3dev_ptr = ptr ;
		m_draw_wnd = hWnd ;
		m_rc = rc ;
		return 0 ;
	}

private:
	virtual int OnAVFrame( Cw2FFmpegAVFrame& src_frame )
	{
		AVPixelFormat fmt = (AVPixelFormat)src_frame->format ;
		const int width = src_frame->width ;
		const int height = src_frame->height ;
		if ( width == 0 || height == 0 )
		{
			return 0 ;
		}
		
		AVFrame dst_frame = { 0 } ;
		dst_frame.format = fmt ;
		dst_frame.data[ 0 ] = src_frame->data[ 0 ] ;
		dst_frame.linesize[ 0 ] = src_frame->linesize[ 0 ] ;

		Cw2FFmpegPictureFrame sws_pic ;

		if ( fmt == AV_PIX_FMT_YUYV422 )
		{
			Cw2FFmpegSws sws ;
			if ( !sws.Init( fmt, AV_PIX_FMT_YUYV422, width, height ) )
			{
				return 0 ;
			}
			
			if ( sws_pic.Init( sws.m_dst.pix_fmt, sws.m_dst.width, sws.m_dst.height ) != 0 )
			{
				return 0 ;
			}

			if ( !sws.Scale( src_frame->data, src_frame->linesize, sws_pic.pic.data, sws_pic.pic.linesize ) )
			{
				return 0 ;
			}
			
			dst_frame.format = sws_pic.ctx.pix_fmt ;
			dst_frame.data[ 0 ] = sws_pic.pic.data[ 0 ] ;
			dst_frame.linesize[ 0 ] = sws_pic.pic.linesize[ 0 ] ;
		}

		CComQIPtr< IDirect3DSurface9 > surface ;
		if ( m_d3dev_ptr->CreateOffscreenSurface( width, height, D3DFMT_YUY2, &surface ) != 0 )
		{
			return 0 ;
		}

		if ( true )
		{
			D3DLOCKED_RECT d3dlr = { 0 } ;
			if ( surface->LockRect( &d3dlr, NULL, D3DLOCK_DONOTWAIT ) == 0 )
			{
				AVPicture dst_pic = { 0 } ;
				dst_pic.data[ 0 ] = (byte*)d3dlr.pBits ;
				dst_pic.linesize[ 0 ] = d3dlr.Pitch ;
				
				av_image_copy( dst_pic.data,
					dst_pic.linesize,
					(const uint8_t**)dst_frame.data,
					dst_frame.linesize,
					(AVPixelFormat)dst_frame.format, width, height ) ;
				
				surface->UnlockRect() ;
			}
		}

		if ( m_d3dev_ptr->UpdateBackSurface( surface, &m_rc ) == 0 )
		{
			if ( m_d3dev_ptr->Present( NULL, &m_rc, m_draw_wnd ) == 0 )
			{
				this->Invoke_OnRenderUpdate( *m_d3dev_ptr, &m_rc, NULL ) ;
			}
		}

		return 0 ;
	}

private:
	HWND			m_draw_wnd	;
	RECT			m_rc		;
	Cw2D3D9Device*	m_d3dev_ptr	;
};

//////////////////////////////////////////////////////////////////////////

class Cw2OpenCam : public IThreadEngine2Routine, public Cw2FFSinkList
{
public:
	Cw2OpenCam() : m_engine( this )
	{

	}

	~Cw2OpenCam()
	{
		CloseCam() ;
	}

public:
	int OpenCam( const char* cam_id, const int width, const int height,
		const char* _v = "mjpeg",
		const char* _p = nullptr )
	{
		if ( m_avfc_cam.InvalidHandle() && m_vdecoder_cam.InvalidHandle() )
		{
			auto _s = WSL2_String_FormatA( "%dx%d", width, height ) ;

			if ( OpenCam( cam_id, _s.c_str(), _v, _p ) == 0 )
			{
				m_engine.EngineStart() ;
				return 0 ;
			}
		}

		return -1 ;
	}

	void CloseCam()
	{
		m_engine.EngineStop() ;

		m_frame_video_cam.FreeHandle() ;
		m_vdecoder_cam.FreeHandle() ;
		m_avfc_cam.FreeHandle() ;
	}

private:
	virtual int OnEngineRoutine( DWORD tid )
	{
		this->ReadPacket() ;
		return 0 ;
	}

private:
	int OpenCam( const char* cam_id, const char* _s, const char* _v, const char* _p )
	{
		auto& vdecoder_cam = m_vdecoder_cam ;
		auto& avfc_cam = m_avfc_cam ;

		try
		{
			auto avif_dshow_ptr = av_find_input_format( "dshow" ) ;
			if ( avif_dshow_ptr == nullptr )
			{
				throw -1 ;
			}

			Cw2FFmpegAVDictionary avif_options ;

			if ( _s )
			{
				av_dict_set( avif_options, "video_size", _s, 0 ) ;
			}

			// av_dict_set( avif_options, "framerate", _r, 0 ) ;

			if ( _v )
			{
				av_dict_set( avif_options, "vcodec", _v, 0 ) ;
			}

			if ( _p )
			{
				av_dict_set( avif_options, "pixel_format", _p, 0 ) ;
			}
			
			if ( avformat_open_input( avfc_cam, WSL2_String_FormatA( "video=%s", cam_id ).c_str(), avif_dshow_ptr, avif_options ) != 0 )
			{
				throw -1 ;
			}

			if ( avformat_find_stream_info( avfc_cam, nullptr ) < 0 )
			{
				throw -1 ;
			}

			for ( decltype( avfc_cam->nb_streams ) stream_idx = 0 ; stream_idx < avfc_cam->nb_streams ; ++stream_idx )
			{
				auto avcc = avfc_cam->streams[ stream_idx ]->codec ;
				if ( avcc->codec_type == AVMEDIA_TYPE_VIDEO )
				{
					if ( avcodec_open2( avcc, avcodec_find_decoder( avcc->codec_id ), nullptr ) != 0 )
					{
						throw -1 ;
					}

					avcc->opaque = (void*)stream_idx ;
					vdecoder_cam = avcc ;
					break ;
				}
			}

			m_frame_video_cam = av_frame_alloc() ;
			if ( m_frame_video_cam.InvalidHandle() )
			{
				throw -1 ;
			}
		}

		catch( ... )
		{
			m_frame_video_cam.FreeHandle() ;
			vdecoder_cam.FreeHandle() ;
			avfc_cam.FreeHandle() ;
			return -1 ;
		}

		return 0 ;
	}

	int ReadPacket()
	{
		auto& avfc_cam = m_avfc_cam ;
		auto& vdecoder_cam = m_vdecoder_cam ;
		auto& frame_video_cam = m_frame_video_cam ;

		if ( true )
		{
			AVPacket pkt ;
			av_init_packet( &pkt ) ;
			pkt.data = nullptr ;
			pkt.size = 0 ;
			if ( av_read_frame( avfc_cam, &pkt ) >= 0 )
			{
				auto orig_pkt = pkt ;

				do
				{
					auto ret = this->DecodePacket( pkt ) ;
					if ( ret < 0 )
					{
						break ;
					}

					pkt.data += ret ;
					pkt.size -= ret ;

				} while( pkt.size > 0 ) ;

				av_free_packet( &orig_pkt ) ;
			}
		}

		return 0 ;
	}

	int DecodePacket( AVPacket& pkt )
	{
		auto& vdecoder_cam = m_vdecoder_cam ;
		auto& frame_video_cam = m_frame_video_cam ;

		int decoded = pkt.size ;
		auto video_stream_index = (int)vdecoder_cam->opaque ;

		if ( video_stream_index == pkt.stream_index )
		{
			int got_frame = 0 ;
			auto ret = avcodec_decode_video2( vdecoder_cam, frame_video_cam, &got_frame, &pkt ) ;
			if ( ret < 0 )
			{
				return ret ;
			}

			if ( got_frame )
			{
				this->Invoke_OnAVFrame( frame_video_cam ) ;
			}
		}

		return decoded ;
	}

private:
	Cw2FFmpegAVFormatContext	m_avfc_cam			;
	Cw2FFmpegAVCodecContextOpen	m_vdecoder_cam		;
	Cw2FFmpegAVFrame			m_frame_video_cam	;

private:
	Cw2ThreadEngine2			m_engine			;
};

//////////////////////////////////////////////////////////////////////////

class Cw2OpenAudioCapture : public IThreadEngine2Routine, public Cw2FFSinkList
{
public:
	Cw2OpenAudioCapture() : m_engine( this )
	{

	}

	~Cw2OpenAudioCapture()
	{
		CloseAudioCapture() ;
	}

public:
	int OpenAudioCapture( const char* ac_id )
	{
		if ( m_avfc_ac.InvalidHandle() && m_adecoder_ac.InvalidHandle() )
		{
			string sid = WSL2_String_FormatA( "audio=%s", ac_id ) ;
			wstring wsid = WSL2_String2W( sid.c_str() ) ;
			auto uid = WSL2_String2UTF8( wsid.c_str() ) ;
			if ( OpenAudioCapture2( uid.get() ) == 0 )
			{
				m_engine.EngineStart() ;
				return 0 ;
			}
		}
		
		return -1 ;
	}

	void CloseAudioCapture()
	{
		m_engine.EngineStop() ;
		
		m_frame_audio_ac.FreeHandle() ;
		m_adecoder_ac.FreeHandle() ;
		m_avfc_ac.FreeHandle() ;
	}

	AVCodecContext* GetAudioDecoder()
	{
		return m_adecoder_ac ;
	}

private:
	virtual int OnEngineRoutine( DWORD tid )
	{
		this->ReadPacket() ;
		return 0 ;
	}

private:
	int OpenAudioCapture2( const char* ac_id )
	{
		auto& avfc_ac = m_avfc_ac ;
		auto& adecoder_ac = m_adecoder_ac ;
		auto& frame_audio_ac = m_frame_audio_ac ;

		try
		{
			auto avif_dshow_ptr = av_find_input_format( "dshow" ) ;
			if ( avif_dshow_ptr == nullptr )
			{
				throw -1 ;
			}

			if ( avformat_open_input( avfc_ac, ac_id, avif_dshow_ptr, nullptr ) != 0 )
			{
				throw -1 ;
			}

			if ( avformat_find_stream_info( avfc_ac, nullptr ) < 0 )
			{
				throw -1 ;
			}

			for ( decltype( avfc_ac->nb_streams ) stream_idx = 0 ; stream_idx < avfc_ac->nb_streams ; ++stream_idx )
			{
				auto avcc = avfc_ac->streams[ stream_idx ]->codec ;
				if ( avcc->codec_type == AVMEDIA_TYPE_AUDIO )
				{
					if ( avcodec_open2( avcc, avcodec_find_decoder( avcc->codec_id ), nullptr ) != 0 )
					{
						throw -1 ;
					}

					avcc->opaque = (void*)stream_idx ;
					adecoder_ac = avcc ;
					break ;
				}
			}

			frame_audio_ac = av_frame_alloc() ;
			if ( frame_audio_ac.InvalidHandle() )
			{
				throw -1 ;
			}
		}

		catch( ... )
		{
			frame_audio_ac.FreeHandle() ;
			adecoder_ac.FreeHandle() ;
			avfc_ac.FreeHandle() ;
			return -1 ;
		}

		return 0 ;
	}

	int ReadPacket()
	{
		auto& avfc_ac = m_avfc_ac ;
		auto& adecoder_ac = m_adecoder_ac ;
		auto& frame_audio_ac = m_frame_audio_ac ;

		if ( true )
		{
			AVPacket pkt ;
			av_init_packet( &pkt ) ;
			pkt.data = nullptr ;
			pkt.size = 0 ;
			if ( av_read_frame( avfc_ac, &pkt ) >= 0 )
			{
				auto orig_pkt = pkt ;

				do
				{
					auto ret = this->DecodePacket( pkt ) ;
					if ( ret < 0 )
					{
						break ;
					}

					pkt.data += ret ;
					pkt.size -= ret ;

				} while( pkt.size > 0 ) ;

				av_free_packet( &orig_pkt ) ;
			}
		}

		return 0 ;
	}

	int DecodePacket( AVPacket& pkt )
	{
		auto& adecoder_ac = m_adecoder_ac ;
		auto& frame_audio_ac = m_frame_audio_ac ;

		int decoded = pkt.size ;
		auto audio_stream_index = (int)adecoder_ac->opaque ;

		if ( audio_stream_index == pkt.stream_index )
		{
			int got_frame = 0 ;
			auto ret = avcodec_decode_audio4( adecoder_ac, frame_audio_ac, &got_frame, &pkt ) ;
			if ( ret < 0 )
			{
				return ret ;
			}
			
			if ( got_frame )
			{
				this->Invoke_OnAVFrame( frame_audio_ac ) ;
			}
			
			decoded = FFMIN( ret, pkt.size ) ;
		}

		return decoded ;
	}

private:
	Cw2FFmpegAVFormatContext	m_avfc_ac			;
	Cw2FFmpegAVCodecContextOpen	m_adecoder_ac		;
	Cw2FFmpegAVFrame			m_frame_audio_ac	;

private:
	Cw2ThreadEngine2			m_engine			;
};

//////////////////////////////////////////////////////////////////////////

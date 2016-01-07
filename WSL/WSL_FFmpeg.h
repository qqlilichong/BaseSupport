
#pragma once

#include "WSL_General.h"

//////////////////////////////////////////////////////////////////////////

class CwMediaTime
{
public:
	CwMediaTime()
	{
		m_nHour = m_nMinute = m_nSecond = m_nMilliseconds = 0 ;
	}
	
	bool IsValid()
	{
		return m_nHour || m_nMinute || m_nSecond || m_nMilliseconds ;
	}
	
	bool operator = ( _In_ wstring& strTime )
	{
		return *this = strTime.c_str() ;
	}
	
	bool operator = ( _In_ const _TCHAR* pszTime )
	{
		// Use for format 00:00:00.00
		if ( _stscanf_s( pszTime, _TEXT( "%u:%u:%u.%u" ), 
			&m_nHour, &m_nMinute, &m_nSecond, &m_nMilliseconds ) == 4 )
		{
			return FixTime() ;
		}
		
		return FALSE ;
	}
	
	bool operator += ( _In_ CwMediaTime& obj )
	{
		m_nHour += obj.m_nHour ;
		m_nMinute += obj.m_nMinute ;
		m_nSecond += obj.m_nSecond ;
		m_nMilliseconds += obj.m_nMilliseconds ;
		return FixTime() ;
	}
	
	bool operator > ( _In_ CwMediaTime& obj )
	{
		ULONGLONG t1 = ( m_nHour * 60 * 60 ) + ( m_nMinute * 60 ) + ( m_nSecond ) ;
		ULONGLONG t2 = ( obj.m_nHour * 60 * 60 ) + ( obj.m_nMinute * 60 ) + ( obj.m_nSecond ) ;
		return t1 >= t2 ;
	}
	
	operator wstring ()
	{
		CwVa_List vRet ;
		vRet.Fmt( _TEXT( "%u:%u:%u.%u" ), m_nHour, m_nMinute, m_nSecond, m_nMilliseconds ) ;
		return (_TCHAR*)vRet ;
	}

protected:
	bool FixTime()
	{
		int nBegin = m_nMilliseconds / 100 ;
		if ( nBegin > 0 )
		{
			m_nSecond += nBegin ;
			m_nMilliseconds = m_nMilliseconds % 100 ;
		}

		nBegin = m_nSecond / 60 ;
		if ( nBegin > 0 )
		{
			m_nMinute += nBegin ;
			m_nSecond = m_nSecond % 60 ;
		}

		nBegin = m_nMinute / 60 ;
		if ( nBegin > 0 )
		{
			m_nHour += nBegin ;
			m_nMinute = m_nMinute % 60 ;
		}
		
		return true ;
	}

protected:
	unsigned int	m_nHour ;
	unsigned int	m_nMinute ;
	unsigned int	m_nSecond ;
	unsigned int	m_nMilliseconds ;
};

//////////////////////////////////////////////////////////////////////////

inline bool WSL_IsVideoFile( _In_ wstring strFile )
{
	static wstring strEXT = L"avi mp4 mkv flv wmv asf rm rmvb m2ts ts" ;
	strFile = WSL_SstrPathFileExt( strFile ) ;
	WSL_Sstr2lower( strFile ) ;
	strFile = WSL_SstrSubflag( strEXT, strFile.c_str(), L"" ) ;
	return strFile.length() > 0 ;
}

//////////////////////////////////////////////////////////////////////////

inline void FFmpegFileFormatFilters( _Inout_ wstring& strFileFormat )
{
	if ( strFileFormat.find( TEXT( "mp4" ) ) != wstring::npos )
	{
		strFileFormat = TEXT( "mp4" ) ;
	}
	
	else if ( strFileFormat.find( TEXT( "ogg" ) ) != wstring::npos )
	{
		strFileFormat = TEXT( "avi" ) ;
	}
}

//////////////////////////////////////////////////////////////////////////

inline bool FFmpegMediaInfo( _In_ const _TCHAR* pszFile, _Out_ CwMediaTime& mtDuration, _Out_opt_ wstring* pFileFormat = NULL, _In_opt_ bool b64 = false )
{
	const _TCHAR* pszffmpeg = b64 ? TEXT( "ffmpeg_x64" ) : TEXT( "ffmpeg_x86" ) ;
	
	wstring strOutInfo ;
	if ( !WSL_ReadProcStdoutAndStderr( strOutInfo, NULL, TEXT( "%s -i %s" ), pszffmpeg, pszFile ) )
	{
		return false ;
	}
	
	mtDuration = WSL_SstrSubflag( strOutInfo, TEXT( "Duration: " ), TEXT( ", start" ) ) ;
	
	if ( pFileFormat != NULL )
	{
		*pFileFormat = WSL_SstrSubflag( strOutInfo, TEXT( "Input #0, " ), TEXT( ", from" ) ) ;
		FFmpegFileFormatFilters( *pFileFormat ) ;
		return pFileFormat->length() > 0 && mtDuration.IsValid() ;
	}
	
	return mtDuration.IsValid() ;
}

//////////////////////////////////////////////////////////////////////////

inline bool FFmpegConvert( _In_ LPCTSTR pszInFile, _In_ LPCTSTR pszOutFile, _In_opt_ bool b64 = false )
{
	const _TCHAR* pszffmpeg = b64 ? TEXT( "ffmpeg_x64" ) : TEXT( "ffmpeg_x86" ) ;
	DWORD dwFFmpegExitCode = INFINITE ;
	if ( WSL_CreateProc< CREATE_NO_WINDOW, FALSE >( NULL, NULL, &dwFFmpegExitCode, 
		TEXT( "%s -i %s -vcodec copy -acodec copy -y %s" ), pszffmpeg, pszInFile, pszOutFile ) )
	{
		return dwFFmpegExitCode == 0 ;
	}
	
	return false ;
}

//////////////////////////////////////////////////////////////////////////

inline bool FFmpegSplitte( _In_ LPCTSTR pszInFile, _In_ LPCTSTR pszOutFile, _In_ CwMediaTime& mtCutNow, _In_ UINT nFS, _In_opt_ bool b64 = false )
{
	wstring strSS = mtCutNow ;
	
	const _TCHAR* pszffmpeg = b64 ? TEXT( "ffmpeg_x64" ) : TEXT( "ffmpeg_x86" ) ;
	DWORD dwFFmpegExitCode = INFINITE ;
	if ( WSL_CreateProc< CREATE_NO_WINDOW, FALSE >( NULL, NULL, &dwFFmpegExitCode, 
		TEXT( "%s -ss %s -i %s -fs %u -vcodec copy -acodec copy -y %s" ), pszffmpeg, strSS.c_str(), pszInFile, nFS, pszOutFile ) )
	{
		return dwFFmpegExitCode == 0 ;
	}
	
	return false ;
}

//////////////////////////////////////////////////////////////////////////

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avformat.lib" )
#pragma comment( lib, "swscale.lib" )
#pragma comment( lib, "avutil.lib" )

//////////////////////////////////////////////////////////////////////////

class CwAVPictureContextBase
{
public:
	CwAVPictureContextBase()
	{
		CleanContext() ;
	}

	~CwAVPictureContextBase()
	{
		CleanContext() ;
	}
	
	inline void CleanContext()
	{
		m_pixformat = AV_PIX_FMT_NONE ;
		m_nWidth = m_nHeight = 0 ;
	}
	
	inline bool operator == ( _In_ CwAVPictureContextBase& obj )
	{
		if ( m_pixformat == obj.m_pixformat && m_nWidth == obj.m_nWidth && m_nHeight == obj.m_nHeight )
		{
			return true ;
		}
		
		return false ;
	}

	inline int GetSize()
	{
		return avpicture_get_size( m_pixformat, m_nWidth, m_nHeight ) ;
	}
	
	enum AVPixelFormat	m_pixformat	;
	int					m_nWidth	;
	int					m_nHeight	;
};

class CwAVPictureContext : public CwAVPictureContextBase
{
public:
	CwAVPictureContext()
	{
		FreePicture() ;
	}

	~CwAVPictureContext()
	{
		FreePicture() ;
	}

	inline void FreePicture()
	{
		SecureZeroMemory( &m_picture, sizeof( m_picture ) ) ;
		CleanContext() ;
	}
	
	inline int Fill( const void* ptr )
	{
		return avpicture_fill( &m_picture, (const uint8_t*)ptr, m_pixformat, m_nWidth, m_nHeight ) ;
	}
	
	inline AVPicture* operator -> ()
	{
		return &m_picture ;
	}

private:
	AVPicture	m_picture ;
};

//////////////////////////////////////////////////////////////////////////

CWSLHANDLE( CwSwsContext, SwsContext*, NULL, sws_freeContext )

class CwFFmpegSWS
{
public:
	bool InitSWS( _In_ enum AVPixelFormat nFmtSrc, _In_ enum AVPixelFormat nFmtDst,
		_In_ int nWidthSrc, _In_ int nHeightSrc,
		_In_ int nWidthDst = 0, _In_ int nHeightDst = 0, _In_ int flags = SWS_POINT )
	{
		if ( !m_swsContext.InvalidHandle() )
		{
			return false ;
		}

		try
		{
			nWidthDst = ( nWidthDst == 0 ? nWidthSrc : nWidthDst ) ;
			nHeightDst = ( nHeightDst == 0 ? nHeightSrc : nHeightDst ) ;
			m_swsContext = sws_getContext( nWidthSrc, nHeightSrc, nFmtSrc,
				nWidthDst, nHeightDst, nFmtDst,
				flags, NULL, NULL, NULL ) ;
			if ( m_swsContext.InvalidHandle() )
			{
				throw -1 ;
			}
			
			m_avpicDst.m_pixformat = nFmtDst ;
			m_avpicDst.m_nWidth = nWidthDst ;
			m_avpicDst.m_nHeight = nHeightDst ;
			
			m_avpicSrc.m_pixformat = nFmtSrc ;
			m_avpicSrc.m_nWidth = nWidthSrc ;
			m_avpicSrc.m_nHeight = nHeightSrc ;
		}
		
		catch ( int nError )
		{
			UnInitSWS() ;
			return nError == 0 ;
		}
		
		return true ;
	}
	
	inline bool Scale( _In_ const uint8_t *const srcSlice[], _In_ const int srcStride[], _In_ const void* ptr, _In_ size_t ptr_size )
	{
		if ( m_swsContext.InvalidHandle() )
		{
			return false ;
		}

		if ( m_avpicDst.Fill( ptr ) != (int)ptr_size )
		{
			return false ;
		}
		
		return sws_scale( m_swsContext,
			srcSlice, srcStride,
			0, m_avpicSrc.m_nHeight,
			m_avpicDst->data, m_avpicDst->linesize ) > 0 ;
	}
	
	inline void UnInitSWS()
	{
		m_avpicDst.FreePicture() ;
		m_avpicSrc.CleanContext() ;
		m_swsContext.FreeHandle() ;
	}

private:
	CwSwsContext			m_swsContext ;
	CwAVPictureContextBase	m_avpicSrc ;
	CwAVPictureContext		m_avpicDst ;
};

//////////////////////////////////////////////////////////////////////////

class CwFFmpegAVCodecSupport
{
public:
	CwFFmpegAVCodecSupport()
	{
		avcodec_register_all() ;
	}
};

//////////////////////////////////////////////////////////////////////////

class CwFFmpegDecoder
{
public:
	CwFFmpegDecoder()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		m_video_frame = av_frame_alloc() ;
		m_audio_frame = av_frame_alloc() ;
		m_video_codec_ptr = NULL ;
		m_audio_codec_ptr = NULL ;
	}

	~CwFFmpegDecoder()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		CloseAudioCodec() ;
		CloseVideoCodec() ;
		av_frame_free( &m_audio_frame ) ;
		av_frame_free( &m_video_frame ) ;
	}

public:
	inline bool OpenVideoCodec( _In_ AVCodecID idCodec )
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		if ( IsVideoCodecOpening() )
		{
			return false ;
		}

		AVCodec* pCodec = avcodec_find_decoder( idCodec ) ;
		if ( pCodec == NULL )
		{
			return false ;
		}
		
		m_video_codec_ptr = avcodec_alloc_context3( pCodec ) ;
		if ( avcodec_open2( m_video_codec_ptr, pCodec, NULL ) != 0 )
		{
			CloseVideoCodec() ;
			return false ;
		}
		
		return true ;
	}
	
	inline bool OpenAudioCodec( _In_ AVCodecID idCodec )
	{
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		if ( IsAudioCodecOpening() )
		{
			return false ;
		}

		AVCodec* pCodec = avcodec_find_decoder( idCodec ) ;
		if ( pCodec == NULL )
		{
			return false ;
		}
		
		m_audio_codec_ptr = avcodec_alloc_context3( pCodec ) ;
		if ( avcodec_open2( m_audio_codec_ptr, pCodec, NULL ) != 0 )
		{
			CloseAudioCodec() ;
			return false ;
		}
		
		return true ;
	}

	inline void CloseVideoCodec()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		if ( IsVideoCodecOpening() )
		{
			avcodec_close( m_video_codec_ptr ) ;
			av_free( m_video_codec_ptr ) ;
			m_video_codec_ptr = NULL ;
		}
	}

	inline void CloseAudioCodec()
	{
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		if ( IsAudioCodecOpening() )
		{
			avcodec_close( m_audio_codec_ptr ) ;
			av_free( m_audio_codec_ptr ) ;
			m_audio_codec_ptr = NULL ;
		}
	}
	
	inline int DecodeVideoPacket( _In_ AVPacket* packet )
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		if ( !IsVideoCodecOpening() )
		{
			return 0 ;
		}
		
		int got_picture_ptr = 0 ;
		avcodec_decode_video2( m_video_codec_ptr, m_video_frame, &got_picture_ptr, packet ) ;
		return got_picture_ptr ;
	}
	
	inline int DecodeAudioPacket( _In_ AVPacket* packet )
	{
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		if ( !IsAudioCodecOpening() )
		{
			return 0 ;
		}
		
		int got_frame_ptr = 0 ;
		avcodec_decode_audio4( m_audio_codec_ptr, m_audio_frame, &got_frame_ptr, packet ) ;
		return got_frame_ptr ;
	}

public:
	inline bool IsVideoCodecOpening()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		return m_video_codec_ptr != NULL ;
	}
	
	inline bool IsAudioCodecOpening()
	{
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		return m_audio_codec_ptr != NULL ;
	}
	
	inline AVPixelFormat get_pix_fmt()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		if ( IsVideoCodecOpening() )
		{
			return m_video_codec_ptr->pix_fmt ;
		}
		
		return AV_PIX_FMT_NONE ;
	}
	
	inline AVFrame* get_video_frame()
	{
		CwStackLock< CwUserSync_CS > lock1( &m_lockvc ) ;
		if ( IsVideoCodecOpening() )
		{
			return m_video_frame ;
		}
		
		return NULL ;
	}

	inline AVFrame* get_audio_frame()
	{
		CwStackLock< CwUserSync_CS > lock2( &m_lockac ) ;
		if ( IsAudioCodecOpening() )
		{
			return m_audio_frame ;
		}

		return NULL ;
	}
	
	inline void LockVideoCodec()
	{
		return m_lockvc.Lock() ;
	}

	inline void UnlockVideoCodec()
	{
		return m_lockvc.Unlock() ;
	}

	inline void LockAudioCodec()
	{
		return m_lockac.Lock() ;
	}

	inline void UnlockAudioCodec()
	{
		return m_lockac.Unlock() ;
	}

private:
	CwUserSync_CS	m_lockvc ;
	CwUserSync_CS	m_lockac ;

private:
	AVCodecContext*	m_video_codec_ptr	;
	AVCodecContext*	m_audio_codec_ptr	;
	AVFrame*		m_video_frame		;
	AVFrame*		m_audio_frame		;
};

//////////////////////////////////////////////////////////////////////////

class CwFFmpegAVSupport
{
public:
	CwFFmpegAVSupport()
	{
		av_register_all() ;
		avformat_network_init() ;
	}
};

//////////////////////////////////////////////////////////////////////////

CWSLHANDLE( CwAVFmtCtx, AVFormatContext*, NULL, avformat_close_input )

class CwFFmpegAV
{
public:
	inline bool OpenAVInput( _In_ const char* pszInput )
	{
		if ( IsAVOpening() )
		{
			return false ;
		}

		try
		{
			if ( avformat_open_input( m_avFormatContext, pszInput, NULL, NULL ) != 0 )
			{
				throw -1 ;
			}

			if ( avformat_find_stream_info( m_avFormatContext, NULL ) < 0 )
			{
				throw -2 ;
			}
		}

		catch ( int )
		{
			CloseAV() ;
			return false ;
		}

		return true ;
	}

	inline int GetBestStream( _In_ enum AVMediaType type )
	{
		if ( !IsAVOpening() )
		{
			return -1 ;
		}

		return av_find_best_stream( m_avFormatContext, type, -1, -1, NULL, 0 ) ;
	}
	
	inline int ReadFrame( _In_ AVPacket* pkt )
	{
		if ( !IsAVOpening() )
		{
			return -1 ;
		}
		
		return av_read_frame( m_avFormatContext, pkt ) ;
	}
	
	inline int Seek( int stream_index, int64_t timestamp, int flags )
	{
		return av_seek_frame( m_avFormatContext, stream_index, timestamp, flags ) ;
	}
	
	inline void CloseAV()
	{
		m_avFormatContext.FreeHandle() ;
	}

	inline double GetStreamTimeBase( _In_ const int nStreamIndex )
	{
		if ( !IsAVOpening() || nStreamIndex < 0 )
		{
			return 0 ;
		}
		
		return av_q2d( m_avFormatContext->streams[ nStreamIndex ]->time_base ) ;
	}

	inline AVCodecID GetStreamCodecID( _In_ const int nStreamIndex )
	{
		if ( !IsAVOpening() || nStreamIndex < 0 )
		{
			return AV_CODEC_ID_NONE ;
		}
		
		return m_avFormatContext->streams[ nStreamIndex ]->codec->codec_id ;
	}
	
	inline bool h264_mp4toannexb( _In_ const int nStreamIndex, _In_ AVPacket* pkt )
	{
		if ( !IsAVOpening() || nStreamIndex < 0 )
		{
			return false ;
		}

		if ( GetStreamCodecID( nStreamIndex ) != AV_CODEC_ID_H264 )
		{
			return false ;
		}

		AVBitStreamFilterContext* pBSFC = av_bitstream_filter_init( "h264_mp4toannexb" ) ;
		if ( pBSFC == NULL )
		{
			return false ;
		}

		uint8_t* dummy = NULL ;
		int dummy_size = 0 ;
		av_bitstream_filter_filter( pBSFC, m_avFormatContext->streams[ nStreamIndex ]->codec, NULL, &dummy, &dummy_size, NULL, 0, 0 ) ;
		av_bitstream_filter_close( pBSFC ) ;
		
		AVPacket temp ;
		av_init_packet( &temp ) ;
		temp.data = (uint8_t*)m_avFormatContext->streams[ nStreamIndex ]->codec->extradata ;
		temp.size = m_avFormatContext->streams[ nStreamIndex ]->codec->extradata_size ;
		return av_copy_packet( pkt, &temp ) == 0 ;
	}

public:
	inline bool IsAVOpening() { return !m_avFormatContext.InvalidHandle() ; }

protected:
	CwAVFmtCtx	m_avFormatContext ;
};

//////////////////////////////////////////////////////////////////////////

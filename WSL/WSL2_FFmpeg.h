
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

//////////////////////////////////////////////////////////////////////////

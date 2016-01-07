
#pragma once

#include <Mmreg.h>
#include <Dsound.h>
#pragma comment( lib, "Dsound.lib" )

//////////////////////////////////////////////////////////////////////////

class Cw2DSound
{
public:
	Cw2DSound( DWORD dwLevel = DSSCL_PRIORITY, HWND hWnd = GetDesktopWindow() )
	{
		if ( DirectSoundCreate( NULL, &m_dsound, NULL ) == S_OK )
		{
			m_dsound->SetCooperativeLevel( hWnd, dwLevel ) ;
		}
	}

	operator IDirectSound* ()
	{
		return m_dsound ;
	}

private:
	CComQIPtr< IDirectSound, &IID_IDirectSound > m_dsound ;
};

//////////////////////////////////////////////////////////////////////////

class Cw2DSoundBuffer
{
public:
	Cw2DSoundBuffer()
	{
		m_last_play_pos = 0 ;
		SecureZeroMemory( &m_wfm, sizeof( m_wfm ) ) ;
		SecureZeroMemory( &m_desc, sizeof( m_desc ) ) ;
	}

	~Cw2DSoundBuffer()
	{
		FreeDSoundBuffer() ;
	}

	int CreateDSoundBuffer( IDirectSound* pDS, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample )
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;

		if ( m_dsound_buffer != nullptr )
		{
			return -1 ;
		}

		SecureZeroMemory( &m_wfm, sizeof( m_wfm ) ) ;
		m_wfm.wFormatTag = WAVE_FORMAT_PCM ;
		m_wfm.nChannels = nChannels ;
		m_wfm.nSamplesPerSec = nSamplesPerSec ;
		m_wfm.wBitsPerSample = wBitsPerSample ;
		m_wfm.nBlockAlign = m_wfm.wBitsPerSample / 8 * m_wfm.nChannels ;
		m_wfm.nAvgBytesPerSec = m_wfm.nSamplesPerSec * m_wfm.nBlockAlign ;
		
		SecureZeroMemory( &m_desc, sizeof( m_desc ) ) ;
		m_desc.dwSize = sizeof( m_desc ) ;
		m_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPAN ;
		m_desc.dwBufferBytes = m_wfm.nAvgBytesPerSec / 2 ;
		m_desc.lpwfxFormat = &m_wfm ;

		if ( pDS->CreateSoundBuffer( &m_desc, &m_dsound_buffer, NULL ) != S_OK )
		{
			return -2 ;
		}

		m_last_play_pos = 0 ;
		return 0 ;
	}

	int FreeDSoundBuffer()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;

		m_last_play_pos = 0 ;
		SecureZeroMemory( &m_wfm, sizeof( m_wfm ) ) ;
		SecureZeroMemory( &m_desc, sizeof( m_desc ) ) ;

		StopPlay() ;
		m_dsound_buffer.Release() ;
		return 0 ;
	}
	
	int ResetPlayLooping()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;
		
		if ( m_dsound_buffer == nullptr )
		{
			return -1 ;
		}

		m_last_play_pos = 0 ;
		m_dsound_buffer->Stop() ;
		
		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;
		if ( m_dsound_buffer->Lock( 0, m_desc.dwBufferBytes,
			&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) != S_OK )
		{
			return -2 ;
		}
		
		SecureZeroMemory( audio_buffer_ptr, audio_buffer_size ) ;
		m_dsound_buffer->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
		
		if ( m_dsound_buffer->SetCurrentPosition( 0 ) != S_OK )
		{
			return -3 ;
		}
		
		if ( m_dsound_buffer->Play( 0, 0, DSBPLAY_LOOPING ) != S_OK )
		{
			return -4 ;
		}
		
		return 0 ;
	}

	int StopPlay()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;

		if ( m_dsound_buffer == nullptr )
		{
			return -1 ;
		}

		m_last_play_pos = 0 ;
		m_dsound_buffer->Stop() ;

		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;
		if ( m_dsound_buffer->Lock( 0, m_desc.dwBufferBytes,
			&audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) != S_OK )
		{
			return -2 ;
		}

		SecureZeroMemory( audio_buffer_ptr, audio_buffer_size ) ;
		m_dsound_buffer->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;

		if ( m_dsound_buffer->SetCurrentPosition( 0 ) != S_OK )
		{
			return -3 ;
		}

		return 0 ;
	}

	WAVEFORMATEX GetWFM()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;
		return m_wfm ;
	}

	DSBUFFERDESC GetDesc()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;
		return m_desc ;
	}

	int LoopDSoundBuffer()
	{
		Cw2AutoLock< decltype( m_dsound_lock ) > auto_lock( &m_dsound_lock ) ;

		if ( m_dsound_buffer == nullptr )
		{
			return -1 ;
		}
		
		const DWORD mid_position = m_desc.dwBufferBytes / 2 ;
		
		DWORD play_position = 0 ;
		if ( m_dsound_buffer->GetCurrentPosition( &play_position, nullptr ) != S_OK )
		{
			return -2 ;
		}
		
		LPVOID audio_buffer_ptr = nullptr ;
		DWORD audio_buffer_size = 0 ;
		
		if ( ( play_position >= mid_position ) && ( m_last_play_pos < mid_position ) )
		{
			m_dsound_buffer->Lock( 0, mid_position, &audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
		}
		
		else if ( play_position < m_last_play_pos )
		{
			m_dsound_buffer->Lock( mid_position, mid_position, &audio_buffer_ptr, &audio_buffer_size, nullptr, nullptr, 0 ) ;
		}
		
		m_last_play_pos = play_position ;

		if ( audio_buffer_size )
		{
			const int status = OnFillDSoundAudioBuffer( audio_buffer_ptr, audio_buffer_size ) ;
			m_dsound_buffer->Unlock( audio_buffer_ptr, audio_buffer_size, nullptr, 0 ) ;
			
			if ( status != 0 )
			{
				ResetPlayLooping() ;
			}
		}
		
		return 0 ;
	}

private:
	virtual int OnFillDSoundAudioBuffer( LPVOID audio_buffer_ptr, DWORD audio_buffer_size )
	{
		return 0 ;
	}

private:
	Cw2US_CS													m_dsound_lock	;
	DWORD														m_last_play_pos	;
	WAVEFORMATEX												m_wfm			;
	DSBUFFERDESC												m_desc			;
	CComQIPtr< IDirectSoundBuffer, &IID_IDirectSoundBuffer >	m_dsound_buffer	;
};

class Cw2DSoundRender : public IThreadEngine2Routine, public Cw2DSoundBuffer
{
public:
	Cw2DSoundRender() : m_thread_render( this )
	{
		m_render_status = -1 ;
	}

	~Cw2DSoundRender()
	{
		m_render_status = -1 ;
		Cw2DSoundBuffer::StopPlay() ;
		m_audio_stream.SetOffset( 0 ) ;
		m_thread_render.EngineStop() ;
		m_audio_stream.FreeBytesStream() ;
	}

	int CreateDSoundRender( IDirectSound* pDS, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample )
	{
		if ( Cw2DSoundBuffer::CreateDSoundBuffer( pDS, nSamplesPerSec, nChannels, wBitsPerSample ) != 0 )
		{
			return -1 ;
		}

		DSBUFFERDESC desc = Cw2DSoundBuffer::GetDesc() ;
		if ( m_audio_stream.CreateBytesStream( desc.dwBufferBytes * 2 ) != 0 )
		{
			return -2 ;
		}

		return 0 ;
	}
	
	int PlayLooping()
	{
		m_audio_stream.SetOffset( 0 ) ;
		
		if ( Cw2DSoundBuffer::ResetPlayLooping() != 0 )
		{
			return -1 ;
		}
		
		m_render_status = 0 ;
		m_thread_render.EngineStart() ;
		return 0 ;
	}

	int StopPlay()
	{
		m_render_status = 1 ;
		Cw2DSoundBuffer::StopPlay() ;
		m_audio_stream.SetOffset( 0 ) ;
		return 0 ;
	}
	
	int FillAudioData( LPVOID pData, DWORD dwDataSize )
	{
		if ( m_render_status == 0 )
		{
			return m_audio_stream.FillBytes( pData, dwDataSize ) ;
		}

		return 2 ;
	}

private:
	virtual int OnEngineRoutine( DWORD tid )
	{
		Sleep( 1 ) ;
		return Cw2DSoundBuffer::LoopDSoundBuffer() ;
	}

	virtual int OnFillDSoundAudioBuffer( LPVOID audio_buffer_ptr, DWORD audio_buffer_size )
	{
		return m_audio_stream.PickBytes( audio_buffer_ptr, audio_buffer_size ) ;
	}

private:
	int					m_render_status	;
	Cw2BytesStream		m_audio_stream	;
	Cw2ThreadEngine2	m_thread_render ;
};

//////////////////////////////////////////////////////////////////////////

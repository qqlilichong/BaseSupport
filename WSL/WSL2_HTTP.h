
#pragma once

#include <http.h>
#pragma comment( lib, "httpapi.lib" )

#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
	do														\
	{														\
		RtlZeroMemory( (resp), sizeof(*(resp)) );			\
		(resp)->StatusCode = (status);                      \
		(resp)->pReason = (reason);                         \
		(resp)->ReasonLength = (USHORT) strlen(reason);     \
	} while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)				\
	do																\
	{																\
		(Response).Headers.KnownHeaders[(HeaderId)].pRawValue =		\
		(RawValue);													\
		(Response).Headers.KnownHeaders[(HeaderId)].RawValueLength =\
		(USHORT) strlen(RawValue);									\
	} while(FALSE)

interface IW2_WIN_HTTP_SERVER_EVENT
{
	virtual int OnPostReceiveFile( const wchar_t* full_url, const wchar_t* file ) = 0 ;
	virtual int OnGetRequest( const wchar_t* full_url, string& str_chunk ) = 0 ;
	virtual int OnServerIdle() = 0 ;
};

//////////////////////////////////////////////////////////////////////////

class Cw2WinHTTPInitV1
{
public:
	Cw2WinHTTPInitV1()
	{
		HTTPAPI_VERSION Version = HTTPAPI_VERSION_1 ;
		HttpInitialize( Version, HTTP_INITIALIZE_SERVER, NULL ) ;
	}

	~Cw2WinHTTPInitV1()
	{
		HttpTerminate( HTTP_INITIALIZE_SERVER, NULL ) ;
	}
};

//////////////////////////////////////////////////////////////////////////

class Cw2WinHTTPServer : public IThreadEngine2Routine
{
public:
	Cw2WinHTTPServer() : m_thread( this ), m_RequestBufferLength( 4096 )
	{
		m_RequestBuffer = new byte[ m_RequestBufferLength ] ;
		if ( !m_RequestBuffer.InvalidHandle() )
		{
			HttpCreateHttpHandle( m_hReqQueue, 0 ) ;
		}

		if ( !m_hReqQueue.InvalidHandle() )
		{
			m_iocp = CreateIoCompletionPort( m_hReqQueue, NULL, 0, 0 ) ;
		}

		m_pRequest = (PHTTP_REQUEST)(byte*)m_RequestBuffer ;
		m_pEvent = nullptr ;
	}

public:
	int StartHTTPServer( const wchar_t* URL, const wchar_t* pPostRecFile, IW2_WIN_HTTP_SERVER_EVENT* pEvent )
	{
		if ( m_iocp.InvalidHandle() )
		{
			return -1 ;
		}

		if ( HttpAddUrl( m_hReqQueue, URL, NULL ) != NO_ERROR )
		{
			return -2 ;
		}

		if ( m_thread.EngineStart() != 0 )
		{
			return -3 ;
		}

		if ( ReceiveHttpRequest() != NO_ERROR )
		{
			return -4 ;
		}

		m_strPostRecFile = pPostRecFile ;
		m_pEvent = pEvent ;
		return 0 ;
	}

private:
	ULONG ReceiveHttpRequest()
	{
		if ( m_iocp.InvalidHandle() )
		{
			return ERROR_NOT_ENOUGH_MEMORY ;
		}

		SecureZeroMemory( (byte*)m_RequestBuffer, m_RequestBufferLength ) ;
		SecureZeroMemory( &m_over, sizeof( m_over ) ) ;

		HTTP_REQUEST_ID requestId ;
		HTTP_SET_NULL_ID( &requestId ) ;

		const ULONG result = HttpReceiveHttpRequest( m_hReqQueue, requestId, 0, m_pRequest, m_RequestBufferLength, NULL, &m_over ) ;
		if ( result == ERROR_IO_PENDING  )
		{
			return NO_ERROR ;
		}

		return result ;
	}

	ULONG OnHttpRequest()
	{
		ULONG result = ERROR_CONNECTION_INVALID ;

		switch ( m_pRequest->Verb )
		{
		case HttpVerbGET :
			{
				result = SendHttpGetResponse() ;
				break ;
			}

		case HttpVerbPOST :
			{
				result = SendHttpPostResponse() ;
				break ;
			}

		default:
			{
				result = SendHttpResponse( 503, "Not Implemented", nullptr ) ;
				break ;
			}
		}

		return result ;
	}

	ULONG SendHttpResponse( USHORT StatusCode, PSTR pReason, PSTR pEntityString )
	{
		HTTP_RESPONSE response ;
		INITIALIZE_HTTP_RESPONSE( &response, StatusCode, pReason ) ;
		ADD_KNOWN_HEADER( response, HttpHeaderContentType, "text/html" ) ;
		
		HTTP_DATA_CHUNK dataChunk ;
		if ( pEntityString )
		{
			if ( strlen( pEntityString ) )
			{
				dataChunk.DataChunkType = HttpDataChunkFromMemory ;
				dataChunk.FromMemory.pBuffer = pEntityString ;
				dataChunk.FromMemory.BufferLength = (ULONG)strlen( pEntityString ) ;

				response.EntityChunkCount = 1 ;
				response.pEntityChunks = &dataChunk ;
			}
		}

		return HttpSendHttpResponse( m_hReqQueue, m_pRequest->RequestId, 0, &response, NULL, NULL, NULL, 0, NULL, NULL ) ;
	}

	ULONG SendHttpGetResponse()
	{
		string str_chunk ;

		if ( m_pEvent != nullptr )
		{
			m_pEvent->OnGetRequest( m_pRequest->CookedUrl.pFullUrl, str_chunk ) ;
		}
		
		return SendHttpResponse( 200, "OK", (PSTR)str_chunk.c_str() ) ;
	}

	ULONG SendHttpPostResponse()
	{
		if ( m_pRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS )
		{
			if ( TransportPostResponse() == NO_ERROR )
			{
				if ( m_pEvent != nullptr )
				{
					m_pEvent->OnPostReceiveFile( m_pRequest->CookedUrl.pFullUrl, m_strPostRecFile.c_str() ) ;
				}
			}
		}
		
		return SendHttpResponse( 200, "OK", nullptr ) ;
	}

	ULONG TransportPostResponse()
	{
		Cw2IO hTempFile = CreateFile( m_strPostRecFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ;
		if ( hTempFile.InvalidHandle() )
		{
			return ERROR_NOT_ENOUGH_MEMORY ;
		}

		for ( ULONG TotalBytesRead = 0, BytesRead = 0 ; ; )
		{
			CHAR EntityBuffer[ 1024 ] = { 0 } ;

			BytesRead = 0 ;
			const ULONG result = HttpReceiveRequestEntityBody( m_hReqQueue, m_pRequest->RequestId, 0, EntityBuffer, sizeof( EntityBuffer ), &BytesRead, NULL ) ;

			switch ( result )
			{
			case NO_ERROR :
				{
					if ( BytesRead != 0 )
					{
						TotalBytesRead += BytesRead ;
						DWORD TempFileBytesWritten = 0 ;
						WriteFile( hTempFile, EntityBuffer, BytesRead, &TempFileBytesWritten, NULL ) ;
					}

					break ;
				}

			case ERROR_HANDLE_EOF :
				{
					if ( BytesRead != 0 )
					{
						TotalBytesRead += BytesRead ;
						DWORD TempFileBytesWritten = 0 ;
						WriteFile( hTempFile, EntityBuffer, BytesRead, &TempFileBytesWritten, NULL ) ;
					}
					
					return NO_ERROR ;
				}

			default :
				return result ;
			}
		}

		return NO_ERROR ;
	}

private: // IThreadEngine2Routine
	virtual int OnEngineRoutine( DWORD tid )
	{
		DWORD NumberOfBytesTransferred = 0 ;
		ULONG_PTR CompletionKey = 0 ;
		LPOVERLAPPED lpOverlapped = nullptr ;
		GetQueuedCompletionStatus( m_iocp, &NumberOfBytesTransferred, &CompletionKey, &lpOverlapped, 100 ) ;

		if ( NumberOfBytesTransferred == 0 && CompletionKey == 0 && lpOverlapped == nullptr )
		{
			if ( m_pEvent != nullptr )
			{
				m_pEvent->OnServerIdle() ;
			}

			return 0 ;
		}

		OnHttpRequest() ;
		ReceiveHttpRequest() ;
		return 0 ;
	}

private:
	OVERLAPPED			m_over					;
	PHTTP_REQUEST		m_pRequest				;
	Cw2ByteBuffer		m_RequestBuffer			;
	const ULONG			m_RequestBufferLength	;

private:
	IW2_WIN_HTTP_SERVER_EVENT*	m_pEvent			;
	wstring						m_strPostRecFile	;

private:
	Cw2Handle			m_hReqQueue	;
	Cw2Handle			m_iocp		;
	Cw2ThreadEngine2	m_thread	;
};

//////////////////////////////////////////////////////////////////////////

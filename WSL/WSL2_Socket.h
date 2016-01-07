
#pragma once

//////////////////////////////////////////////////////////////////////////

#include <Iphlpapi.h>

#include <IcmpAPI.h>
#pragma comment( lib, "iphlpapi.lib" )

#include <Winsock2.h>
#pragma comment( lib, "Ws2_32.lib" )

Cw2AutoHandle( Cw2ICMP, HANDLE, INVALID_HANDLE_VALUE, IcmpCloseHandle )	;
inline bool WSL2_PingIPV4( _In_ const char* ip, _In_ DWORD dwTimeOut )
{
	Cw2ICMP icmp = IcmpCreateFile() ;
	if ( icmp.InvalidHandle() )
	{
		return false ;
	}

	ICMP_ECHO_REPLY icmp_reply = { 0 } ;
	return IcmpSendEcho( icmp, inet_addr( ip ), NULL, 0, NULL, &icmp_reply, sizeof( icmp_reply ), dwTimeOut ) > 0 ;
}

//////////////////////////////////////////////////////////////////////////

Cw2AutoHandle( Cw2Socket, SOCKET, INVALID_SOCKET, closesocket )	;

class Cw2SocketStartup
{
public:
	Cw2SocketStartup()
	{
		const WORD wVersionRequested = MAKEWORD( 2, 2 ) ;
		WSADATA wsaData = { 0 } ;
		WSAStartup( wVersionRequested, &wsaData ) ;
	}

	~Cw2SocketStartup()
	{
		WSACleanup() ;
	}
};

inline SOCKET WSL2_Socket_Create( _In_ bool bStream, _In_ int af = AF_INET, _In_ DWORD flags = 0 )
{
	return socket( af, bStream ? SOCK_STREAM : SOCK_DGRAM, 0 ) ;
}

inline int WSL2_Socket_BindIPV4( _In_ SOCKET sock, _In_ UINT port, _In_ const char* ip = nullptr )
{
	sockaddr_in addr = { 0 } ;
	addr.sin_family = AF_INET ;
	addr.sin_addr.s_addr = ( ip == nullptr ? INADDR_ANY : inet_addr( ip ) ) ;
	addr.sin_port = htons( port ) ;
	return bind( sock, (const sockaddr*)&addr, sizeof( addr ) ) ;
}

inline int WSL2_Socket_Listen( _In_ SOCKET sock_listen, _In_ int backlog = 5 )
{
	return listen( sock_listen, backlog ) ;
}

inline SOCKET WSL2_Socket_Accept( _In_ SOCKET sock_listen )
{
	sockaddr_in addr = { 0 } ;
	INT addrlen = sizeof( addr ) ;
	return accept( sock_listen, (sockaddr*)&addr, &addrlen ) ;
}

inline int WSL2_Socket_Connect( _In_ SOCKET sock, _In_ const char* ip, _In_ unsigned int port )
{
	sockaddr_in addr = { 0 } ;
	addr.sin_family = AF_INET ;
	addr.sin_addr.s_addr = inet_addr( ip ) ;
	addr.sin_port = htons( port ) ;
	return connect( sock, (const sockaddr*)&addr, sizeof( addr ) ) ;
}

inline int WSL2_Socket_Send( _In_ SOCKET sock, _In_ const void* ptr, _In_ unsigned int size, _In_ int flags = 0 )
{
	return send( sock, (const char*)ptr, size, flags ) ;
}

inline int WSL2_Socket_Recv( _In_ SOCKET sock, _In_ void* ptr, _In_ unsigned int size, _In_ int flags = 0 )
{
	return recv( sock, (char*)ptr, size, flags ) ;
}

//////////////////////////////////////////////////////////////////////////

#define SSDP_MAKEFOURCC(ch0, ch1, ch2, ch3) ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define SSDP_VERIFY		SSDP_MAKEFOURCC( 'S', 'S', 'D', 'P' )
#define SSDP_DONE		"SSDPDONE"
#define SSDP_FAIL		"SSDPFAIL"

typedef struct w2SOCK_STREAM_DISP
{
	int		disp_verify ;
	int		disp_flag	;
	int		disp_type	;
	int		disp_size	;
} SOCK_STREAM_DISP, *LPSOCK_STREAM_DISP ;

#pragma pack( push, 1 )
typedef struct w2SD_HEADER
{
	BITMAPFILEHEADER bfh ;
	BITMAPINFOHEADER bih ;

	w2SD_HEADER()
	{
		SecureZeroMemory( &bfh, sizeof( bfh ) ) ;
		SecureZeroMemory( &bih, sizeof( bih ) ) ;

		bfh.bfType = 0x4d42 ;
		bfh.bfReserved1 = 0x3d32 ;
		bfh.bfReserved2 = 0x2d22 ;

		bih.biSize = sizeof( bih ) ;
		bih.biPlanes = 1 ;
		bih.biBitCount = 32 ;
	}

} SD_HEADER, *LPSD_HEADER ;
#pragma pack( pop )

//////////////////////////////////////////////////////////////////////////

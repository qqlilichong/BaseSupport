
#pragma once

#include "WSL_IO.h"

#include <WinSock2.h>
#pragma comment( lib, "ws2_32.lib" )

#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#include <icmpapi.h>

//////////////////////////////////////////////////////////////////////////

class CwSocketStartup
{
public:
	CwSocketStartup()
	{
		const WORD wVersionRequested = MAKEWORD( 2, 2 ) ;
		WSADATA wsaData = { 0 } ;
		WSAStartup( wVersionRequested, &wsaData ) ;
		//wsaData.wVersion == wVersionRequested ;
	}

	~CwSocketStartup()
	{
		WSACleanup() ;
	}
};

//////////////////////////////////////////////////////////////////////////

CWSLHANDLE( CwSockHandle, SOCKET, INVALID_SOCKET, closesocket )

class CwSocket
{
public:
	CwSocket()
	{
		static CwSocketStartup wsa ;
	}

public:
	bool CreateSock( _In_ bool bStream, _In_ int af = AF_INET, _In_ DWORD dwFlags = 0 )
	{
		if ( !m_sock.InvalidHandle() )
		{
			return false ;
		}

		const int type = ( bStream ? SOCK_STREAM : SOCK_DGRAM ) ;
		const int protocol = ( bStream ? IPPROTO_TCP : IPPROTO_UDP ) ;
		m_sock = WSASocket( af, type, protocol, NULL, 0, dwFlags ) ;
		return m_sock.InvalidHandle() == false ;
	}
	
	bool BindIPV4( _In_ u_short port, _In_ const char* ip = NULL )
	{
		if ( m_sock.InvalidHandle() )
		{
			return false ;
		}
		
		sockaddr_in service = { 0 } ;
		service.sin_family = AF_INET ;
		service.sin_addr.s_addr = ( ip == NULL ? INADDR_ANY : inet_addr( ip ) ) ;
		service.sin_port = htons( port ) ;
		return bind( m_sock, (const sockaddr*)&service, sizeof( service ) ) == 0 ;
	}
	
	void CloseSock()
	{
		m_sock.FreeHandle() ;
	}

protected:
	CwSockHandle	m_sock ;
};

//////////////////////////////////////////////////////////////////////////

class CwSocketUDP : public CwSocket
{
public:
	CwSocketUDP()
	{
		if ( CreateSock( false ) )
		{
			BOOL bOpen = TRUE ;
			setsockopt( m_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bOpen, sizeof( bOpen ) ) ;
		}
	}
	
	bool Sync_SendString_IPV4( _In_ const char* str, _In_ const sockaddr_in& addr )
	{
		if ( m_sock.InvalidHandle() )
		{
			return false ;
		}
		
		WSABUF buf = { 0 } ;
		buf.buf = (CHAR*)str ;
		buf.len = sizeof( char ) * strlen( str ) ;
		
		DWORD dwSent = 0 ;
		return WSASendTo( m_sock, &buf, 1, &dwSent, 0, (const sockaddr*)&addr, sizeof( addr ), NULL, NULL ) == 0 ;
	}
	
	bool Sync_SendString_IPV4( _In_ const char* str, _In_ const u_short port, _In_ const char* ip = NULL )
	{
		sockaddr_in addr = { 0 } ;
		addr.sin_family = AF_INET ;
		addr.sin_addr.s_addr = ( ip == NULL ? INADDR_BROADCAST : inet_addr( ip ) ) ;
		addr.sin_port = htons( port ) ;
		return Sync_SendString_IPV4( str, addr ) ;
	}
	
	bool Sync_RecvString_IPV4( _Out_ string& str, _Out_opt_ sockaddr_in* pAddrFrom = NULL )
	{
		if ( m_sock.InvalidHandle() )
		{
			return false ;
		}
		
		CwLocalAllocer< char* > alloc ;
		if ( !alloc.Alloc( WSL_VM_RegionSize() ) )
		{
			return false ;
		}
		
		WSABUF buf = { 0 } ;
		buf.buf = (CHAR*)(char*)alloc ;
		buf.len = alloc.Size() - sizeof( char ) ;
		
		sockaddr_in addr = { 0 } ;
		int nAddr = sizeof( addr ) ;

		DWORD dwRecvd = 0 ;
		DWORD dwFlag = 0 ;
		if ( WSARecvFrom( m_sock, &buf, 1, &dwRecvd, &dwFlag, (sockaddr*)&addr, &nAddr, NULL, NULL ) != 0 )
		{
			return false ;
		}

		if ( pAddrFrom != NULL )
		{
			*pAddrFrom = addr ;
		}

		str = alloc ;
		return true ;
	}
};

//////////////////////////////////////////////////////////////////////////
// ICMP
CWSLHANDLE( CwICMP, HANDLE, INVALID_HANDLE_VALUE, IcmpCloseHandle )
inline bool WSL_PingIPV4( _In_ const char* ip, _In_ DWORD dwTimeOut )
{
	CwICMP icmp = IcmpCreateFile() ;
	if ( icmp.InvalidHandle() )
	{
		return false ;
	}
	
	ICMP_ECHO_REPLY icmp_reply = { 0 } ;
	return IcmpSendEcho( icmp, inet_addr( ip ), NULL, 0, NULL, &icmp_reply, sizeof( icmp_reply ), dwTimeOut ) > 0 ;
}

//////////////////////////////////////////////////////////////////////////

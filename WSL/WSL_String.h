
#pragma once

#include <tchar.h>

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <list>
using namespace std ;

#define TCLEN2SIZE( len )	( len * sizeof( _TCHAR ) )

//////////////////////////////////////////////////////////////////////////

template< typename S >
inline void WSL_Sstr2lower( _In_ S& str, _In_opt_ bool lower = true )
{
	std::transform( str.begin(), str.end(), str.begin(), lower ? tolower : toupper ) ;
}

//////////////////////////////////////////////////////////////////////////

template< typename S, typename C >
inline S WSL_SstrSubflag( _In_ S& str,
						 _In_ const C* pszBOF, _In_ const C* pszEOF,
						 _In_opt_ const int BOF_PT = 1, _In_opt_ const int EOF_PT = 1,
						 _In_opt_ const int EOF_OFFSET = 0 )
{
	S ret ;
	
	S::size_type posIF_BOF = S::npos ;
	switch ( BOF_PT )
	{
		case 1 : posIF_BOF = str.find( pszBOF ) ; break ;
		case 2 : posIF_BOF = str.rfind( pszBOF ) ; break ;
	}
	
	if ( posIF_BOF != S::npos )
	{
		S len = pszBOF ;
		posIF_BOF += len.length() ;
		
		S::size_type posIF_EOF = S::npos ;
		ret = pszEOF ;
		if ( ret.length() != 0 )
		{
			switch ( EOF_PT )
			{
				case 1 : posIF_EOF = str.find( pszEOF, posIF_BOF ) ; break ;
				case 2 : posIF_EOF = str.rfind( pszEOF ) ; break ;
			}
			
			if ( posIF_EOF == S::npos )
			{
				ret.clear() ;
				return ret ;
			}
		}
		
		ret = str.substr( posIF_BOF, posIF_EOF - posIF_BOF + EOF_OFFSET ) ;
	}
	
	return ret ;
}

//////////////////////////////////////////////////////////////////////////

template< typename S, typename C >
inline void WSL_Sstr2vector( _In_ S str, _Out_ vector< S >& vec, _In_ const C* pszFlag )
{
	S flag = pszFlag ;
	S::size_type BOF = S::npos ;
	while ( ( BOF = str.find( flag ) ) != S::npos )
	{
		vec.push_back( str.substr( 0, BOF ) ) ;
		str = str.substr( BOF + flag.length() ) ;
	}
	
	if ( str.length() )
	{
		vec.push_back( str ) ;
	}
}

//////////////////////////////////////////////////////////////////////////

// func  WSL_SstrPathFilePath
inline wstring WSL_SstrPathFilePath( _In_ wstring& str )
{
	return WSL_SstrSubflag( str, L"", L"\\", 1, 2, 1 ) ;
}

//////////////////////////////////////////////////////////////////////////

// func  WSL_SstrPathFileName
inline wstring WSL_SstrPathFileName( _In_ wstring& str )
{
	return WSL_SstrSubflag( str, L"\\", L".", 2, 2 ) ;
}

//////////////////////////////////////////////////////////////////////////

// func  WSL_SstrPathFileExt
inline wstring WSL_SstrPathFileExt( _In_ wstring& str )
{
	return WSL_SstrSubflag( str, L".", L"", 2, 2 ) ;
}

//////////////////////////////////////////////////////////////////////////

// func  WSL_SstrPathFileNameExt
inline wstring WSL_SstrPathFileNameExt( _In_ wstring& str )
{
	return WSL_SstrSubflag( str, L"\\", L"", 2, 2 ) ;
}

//////////////////////////////////////////////////////////////////////////

// func  WSL_SstrPathFilePathName
inline wstring WSL_SstrPathFilePathName( _In_ wstring& str )
{
	return WSL_SstrSubflag( str, L"", L".", 1, 2 ) ;
}

//////////////////////////////////////////////////////////////////////////

// func WSL_Sstr2int
#define DECLARE_WSL_Sstr2int( S, R )	\
inline int WSL_Sstr2int( _In_ S& str )\
{\
	int nRet = 0 ;\
	R stream( str ) ;\
	stream >> nRet ;\
	return nRet ;\
}\
	
DECLARE_WSL_Sstr2int( string, istringstream ) ;
DECLARE_WSL_Sstr2int( wstring, wistringstream ) ;

//////////////////////////////////////////////////////////////////////////

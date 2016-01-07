
#pragma once

//////////////////////////////////////////////////////////////////////////

#define	CWSLHANDLE( C, T, I, R )	\
class C{ public:	\
		C() { m_handle = I ; } C( T h ) { m_handle = h ; } ~C() { FreeHandle() ; }	\
		inline void FreeHandle() { if( !InvalidHandle() ) { R( *this ) ; m_handle = I ; } }	\
		inline bool InvalidHandle() { return m_handle == I ; }	\
		inline T DetachHandle() { T ret = m_handle ; m_handle = I ; return ret ; }	\
		operator T () { return m_handle ; }	\
		operator T* () { return &m_handle ; }	\
		T operator -> () { return m_handle ; }	\
		T operator = ( T h ) { FreeHandle() ; return m_handle = h ; }	\
		protected: T m_handle ; };

#define CWSLALLOCER_BEGIN( B, A, ... )	\
template< typename T >	\
class B##Allocer : public B { public:	\
		B##Allocer(){}	\
		template< typename H >	B##Allocer( H v ){ B& obj = *this ; obj = v ; }	\
		operator T () { return (T)m_handle ; }	\
		T operator -> () { return (T)*this ; }	\
		inline bool Alloc( size_t uBytes, __VA_ARGS__ ) { FreeHandle() ; m_handle = A(
#define CWSLALLOCER_ALLOCPARAM( ... )	__VA_ARGS__ ) ; return InvalidHandle() == false ; }
#define CWSLALLOCER_SIZE( ... )			inline size_t Size() { __VA_ARGS__ ; }
#define CWSLALLOCER_END()				 };

//////////////////////////////////////////////////////////////////////////

#define	KB					1024
#define MB					( KB * KB )
#define GB					( MB * KB )

//////////////////////////////////////////////////////////////////////////

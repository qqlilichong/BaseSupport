
#pragma once

//////////////////////////////////////////////////////////////////////////

#define		Cw2AutoHandle( class_name, handle_type, handle_invalid, handle_close )													\
class		class_name					{ public:																					\
			class_name()				{ m_handle = handle_invalid ; }																\
			class_name( handle_type h )	{ m_handle = h ; }																			\
			~class_name()				{ FreeHandle() ; }																			\
inline BOOL			InvalidHandle()		{ return m_handle == handle_invalid ; }														\
inline void			FreeHandle()		{ if ( !InvalidHandle() ) { handle_close( *this ) ; m_handle = handle_invalid ; } }			\
inline handle_type	DetachHandle()		{ handle_type ret = m_handle ; m_handle = handle_invalid ; return ret ; }					\
operator			handle_type ()		{ return m_handle ; }																		\
operator			handle_type* ()		{ return &m_handle ; }																		\
handle_type			operator -> ()		{ return m_handle ; }																		\
handle_type			operator = ( handle_type h ) { FreeHandle() ; return m_handle = h ; }											\
private: handle_type m_handle ; };

Cw2AutoHandle( Cw2Handle, HANDLE, NULL, CloseHandle )	;
Cw2AutoHandle( Cw2Library, HMODULE, NULL, FreeLibrary )	;
Cw2AutoHandle( Cw2Local, HLOCAL, NULL, LocalFree ) ;
Cw2AutoHandle( Cw2IO, HANDLE, INVALID_HANDLE_VALUE, CloseHandle ) ;

//////////////////////////////////////////////////////////////////////////

#define W2ERR_OK					0L	// no error.
#define W2ERR_UNKNOWN				-1L // unknown error.
#define W2ERR_NOTSUPPORT			-2L // not support.
#define W2ERR_NOMEMORY				-3L // no enough memory.
#define W2ERR_NOTREADY				-4L // no enough parameters to run.
#define W2ERR_PARAMFAILED			-5L // invalid parameters.

#define W2OBJ_DISPID_QI				0L	// query interface.

#define W2OBJ_CLASSNAME( x )		public : static const wchar_t* GetClassName() { return L"x" ; }

__interface IW2OBJECT
{
	virtual int FreeObject2() = 0 ;
	virtual INT_PTR Dispatch2( INT_PTR id, LPVOID ptr, SIZE_T size ) = 0 ;
};

class Iw2ObjectBase : public IW2OBJECT
{
public:
	W2OBJ_CLASSNAME( Iw2ObjectBase ) ;
	virtual ~Iw2ObjectBase() {}

public: // IW2OBJECT
	virtual int FreeObject2()
	{
		delete this ;
		return W2ERR_OK ;
	}
	
	virtual INT_PTR Dispatch2( INT_PTR id, LPVOID ptr, SIZE_T size )
	{
		return W2ERR_NOTSUPPORT ;
	}
};

template< class C >
inline C* WSL2_NewObject2()
{
	return new C ;
}

inline int WSL2_FreeObject2( IW2OBJECT* pObj )
{
	if ( pObj != nullptr )
	{
		return pObj->FreeObject2() ;
	}
	
	return W2ERR_PARAMFAILED ;
}

Cw2AutoHandle( Cw2Object, IW2OBJECT*, nullptr, WSL2_FreeObject2 ) ;

//////////////////////////////////////////////////////////////////////////

inline void WSL2_FreeByteBuffer( byte** pp )
{
	byte*& ptr = *pp ;
	if ( ptr != nullptr )
	{
		delete []ptr ;
		ptr = nullptr ;
	}
}

Cw2AutoHandle( Cw2ByteBuffer, byte*, nullptr, WSL2_FreeByteBuffer ) ;

//////////////////////////////////////////////////////////////////////////

class Cw2BytesStream
{
public:
	Cw2BytesStream()
	{
		m_buffer_max = 0 ;
		m_buffer_offset = 0 ;
	}

	~Cw2BytesStream()
	{
		FreeBytesStream() ;
	}

	int CreateBytesStream( size_t buffer_max )
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;

		if ( !m_buffer_stream.InvalidHandle() )
		{
			return -1 ;
		}

		m_buffer_stream = new byte[ buffer_max ] ;
		if ( m_buffer_stream.InvalidHandle() )
		{
			return -1 ;
		}

		m_buffer_max = buffer_max ;
		return SetOffset( 0 ) ;
	}

	int FreeBytesStream()
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;

		m_buffer_max = 0 ;
		m_buffer_offset = 0 ;
		m_buffer_stream.FreeHandle() ;
		return 0 ;
	}

	size_t GetOffset()
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;
		return m_buffer_offset ;
	}

	int SetOffset( size_t buffer_offset = 0 )
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;

		if ( m_buffer_stream.InvalidHandle() )
		{
			return -1 ;
		}

		if ( buffer_offset > m_buffer_max )
		{
			return -2 ;
		}

		m_buffer_offset = buffer_offset ;
		return 0 ;
	}

	int FillBytes( void* src_ptr, size_t src_size )
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;

		if ( m_buffer_stream.InvalidHandle() )
		{
			return -1 ;
		}

		if ( ( m_buffer_offset + src_size ) > m_buffer_max )
		{
			return 1 ;
		}

		byte* dst_ptr = m_buffer_stream ;
		memcpy( dst_ptr + m_buffer_offset, src_ptr, src_size ) ;
		m_buffer_offset += src_size ;
		return 0 ;
	}

	int PickBytes( void* dst_ptr, size_t pick_size )
	{
		Cw2AutoLock< decltype( m_buffer_lock ) > auto_lock( &m_buffer_lock ) ;

		if ( m_buffer_stream.InvalidHandle() )
		{
			return -1 ;
		}

		if ( m_buffer_offset < pick_size )
		{
			return -2 ;
		}

		byte* src_ptr = m_buffer_stream ;
		memcpy( dst_ptr, src_ptr, pick_size ) ;

		const size_t new_offset = m_buffer_offset - pick_size ;
		memcpy( src_ptr, src_ptr + pick_size, new_offset ) ;
		m_buffer_offset = new_offset ;

		return 0 ;
	}

private:
	Cw2US_CS		m_buffer_lock	;
	size_t			m_buffer_max	;
	size_t			m_buffer_offset	;
	Cw2ByteBuffer	m_buffer_stream	;
};

//////////////////////////////////////////////////////////////////////////

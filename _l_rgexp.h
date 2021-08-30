#ifndef __L_RGEXP_H
#define __L_RGEXP_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

//#define REGEXP_NO_TRIGGERS

#include <list>
#include <sstream>
#include <locale>

// _L_REGEXP_DEFAULTCHAR:
// This used for some function default arguments.
#ifndef _L_REGEXP_DEFAULTCHAR
#define _L_REGEXP_DEFAULTCHAR char
#endif //_L_REGEXP_DEFAULTCHAR

__REGEXP_BEGIN_NAMESPACE

// _l_rgexp.h

template < class t_TyChar >
class _regexp_base
{
private:
	typedef _regexp_base< t_TyChar >	_TyThis;
public:
	typedef t_TyChar	_TyChar;
#ifdef  _LIBCPP_VERSION
#ifdef __linux__
#ifdef BIEN_WCHAR_16BIT
#error Expected 32bit character type for linux.
#endif
	// Under linux libc++ we need to substitute wchar_t for char32_t for the osstream.
  typedef std::conditional_t< is_same_v< _TyChar, char32_t >, wchar_t, _TyChar > _TyCharOstream;
	typedef basic_ostream< _TyCharOstream, char_traits<_TyCharOstream> > _TyOstream;
#else
	typedef _TyChar _TyCharOstream;
	typedef basic_ostream< t_TyChar, char_traits<t_TyChar> > _TyOstream;
#endif 
#else
	typedef _TyChar _TyCharOstream;
	typedef basic_ostream< t_TyChar, char_traits<t_TyChar> > _TyOstream;
#endif

	typedef _nfa_context_base< t_TyChar >		_TyCtxtBase;
	typedef typename _TyCtxtBase::_TyRange	_TyRange;
	
	_TyThis & operator = ( _TyThis const & _r ) = delete;
	_regexp_base() = default;
	virtual ~_regexp_base() = default;

	virtual void ConstructNFA( _TyCtxtBase & _rNfa, size_t _stLevel = 0 ) const = 0;

	virtual bool FIsLiteral() const _BIEN_NOTHROW		{ return false; }
	virtual bool FMatchesEmpty() const _BIEN_NOTHROW	{ return false; }

	// Allocation - this is overridden by the element receiving
	//	the final regular expression
	virtual void Clone( _TyThis * _prbCopier, _TyThis ** _pprbStorage ) const = 0;

#ifdef __RGEXP_DUMP
	virtual void Dump( _TyOstream & _ros ) const = 0;
#endif //__RGEXP_DUMP

	// no assignment supported.

	friend _TyOstream & operator << ( _TyOstream & _ros, _TyThis const & _r )
	{
    typename _TyOstream::sentry s(_ros);
    if ( s ) 
			_r.Dump( _ros );
		return _ros;
	}

protected:

	virtual char *	_CPAllocate( size_t )
	{
		Assert( 0 );	// This should be overridden in final.
		return 0;
	}

	void						_Deallocate( char * ) _BIEN_NOTHROW
	{
		Assert( 0 );
	}

	// Full clone:
	template < class t_TyMostDerived >
	static void
	_CloneHelper( t_TyMostDerived const * _pmdCopy, 
								_TyThis * _prbCopier, 
								_TyThis ** _pprbStorage )
	{
		char * cp = _prbCopier->_CPAllocate( sizeof( t_TyMostDerived ) );
		_BIEN_TRY
		{
			*_pprbStorage = new ( cp ) t_TyMostDerived( *_pmdCopy );
		}
		_BIEN_UNWIND( _prbCopier->_Deallocate( cp ) );
	}

	// Partial clone:
	template < class t_TyMostDerived >
	static void
	_CloneHelper( t_TyMostDerived const * _pmdCopy, 
								_TyThis * _prbCopier, 
								_TyThis ** _pprbStorage, 
								std::false_type )
	{
		char * cp = _prbCopier->_CPAllocate( sizeof( t_TyMostDerived ) );
		_BIEN_TRY
		{
			*_pprbStorage = new ( cp ) t_TyMostDerived( *_pmdCopy, std::false_type() );
		}
		_BIEN_UNWIND( _prbCopier->_Deallocate( cp ) );
	}
};

template < class t_TyChar >
class _regexp_empty : public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar > _TyBase;
	typedef _regexp_empty< t_TyChar > _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;

	_regexp_empty()
	{
	}
	_regexp_empty( _TyThis const & _r, std::false_type = std::false_type() )
	{
	}  

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		// We call a helper function that knows how to construct an empty NFA:
		_rNfaCtxt.CreateEmptyNFA();
	}

	bool FIsLiteral() const  _BIEN_NOTHROW override { return true; }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return true; }

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		char rgc[4] = {'{', '0', '}', '\0'}; // clang didn't like this as a string: warning: illegal character encoding in string literal _ros << "{<D8>}";
		_ros << rgc;
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_empty< t_TyChar >
empty( t_TyChar = _L_REGEXP_DEFAULTCHAR() )
{
	return _regexp_empty< t_TyChar >();
}

template < class t_TyChar >
class _regexp_literal : public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar >		_TyBase;
	typedef _regexp_literal< t_TyChar >	_TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;

	t_TyChar	m_c;

	_regexp_literal( t_TyChar _c )
		: m_c( _c )
	{
	}
	_regexp_literal( _TyThis const & _r, std::false_type = std::false_type() )
		: m_c( _r.m_c )
	{
	}  

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateLiteralNFA( m_c );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "\'" << m_c << "\'";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_literal< t_TyChar >
literal( t_TyChar _c )
{
	return _regexp_literal< t_TyChar >( _c );
}

template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_litstr 
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar >									_TyBase;
	typedef _regexp_litstr< t_TyChar, t_TyAllocator >	_TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef basic_string< t_TyChar, char_traits< t_TyChar >, typename _Alloc_traits< t_TyChar, t_TyAllocator >::allocator_type > _TyString;
	
	_TyString	m_s;

	_regexp_litstr( const t_TyChar * _pc,
									t_TyAllocator const & _alloc = t_TyAllocator() )
		: m_s( _pc, _alloc )
	{
	}
	_regexp_litstr( _TyThis const & _r, std::false_type = std::false_type() )
    : m_s( _r.m_s )
  {
  }

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateStringNFA( m_s.c_str() );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return m_s.empty(); }

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "\"" << reinterpret_cast< const _TyCharOstream * >( &m_s[0] ) << "\"";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_litstr< t_TyChar >
litstr( const t_TyChar * _pc )
{
	return _regexp_litstr< t_TyChar >( _pc );
}

template < class t_TyChar, class t_TyAllocator >
__INLINE _regexp_litstr< t_TyChar, t_TyAllocator >
litstr( const t_TyChar * _pc, t_TyAllocator const & _rAlloc )
{
	return _regexp_litstr< t_TyChar >( _pc, _rAlloc );
}

template < class t_TyChar >
class _regexp_litrange 
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar >			_TyBase;
	typedef _regexp_litrange< t_TyChar >	_TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	using typename _TyBase::_TyRange;

	_TyRange	m_r;

	_regexp_litrange( t_TyChar _cFirst, t_TyChar _cLast )
		: m_r( _cFirst, _cLast )
	{
	}
	_regexp_litrange( _TyThis const & _r, std::false_type = std::false_type() )
		: m_r( _r.m_r )
  {
  }

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateRangeNFA( m_r );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }

	void	Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "['" << m_r.first << "'-'" << m_r.second << "']";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_litrange< t_TyChar >
litrange( t_TyChar _cFirst, t_TyChar _cLast )
{
	return _regexp_litrange< t_TyChar >( _cFirst, _cLast );
}

// _regexp_litnotset: a single literal not in the set given in the string.
template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_litnotset 
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar > _TyBase;
	typedef _regexp_litnotset _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef basic_string< t_TyChar, char_traits< t_TyChar >, typename _Alloc_traits< t_TyChar, t_TyAllocator >::allocator_type > _TyString;
	
	_TyString	m_s;

	_regexp_litnotset(	const t_TyChar * _pc,
											t_TyAllocator const & _alloc = t_TyAllocator() )
		: m_s( _pc, _alloc )
	{
	}
	// REVIEW:<dbien>: Don't remember why I have this for the copy constructor...
	_regexp_litnotset( _TyThis const & _r, std::false_type = std::false_type() )
    : m_s( _r.m_s )
  {
  }

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateLiteralNotInSetNFA( m_s.c_str() );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return false; } // assuming false here... the fact is that if it does matches empty then it only matches nothing...

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "[^" << m_s << "]";
	}
#endif //__RGEXP_DUMP
};
template < class t_TyChar >
__INLINE _regexp_litnotset< t_TyChar >
litnotset( const t_TyChar * _pc )
{
	return _regexp_litnotset< t_TyChar >( _pc );
}
template < class t_TyChar, class t_TyAllocator >
__INLINE _regexp_litnotset< t_TyChar, t_TyAllocator >
litnotset( const t_TyChar * _pc, t_TyAllocator const & _rAlloc )
{
	return _regexp_litnotset< t_TyChar >( _pc, _rAlloc );
}
// _regexp_litnotset_no_surrogates: a single literal not in the set given in the string.
// This class ensures that no surrogate UTF16 (which are also not valid as characters in UTF32)
//	are added to the recognition set.
// One thought was to somehow make the exclusion of surrogates part of a potential NfaContext option
//	but it wasn't clear that this would work.
template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_litnotset_no_surrogates 
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar > _TyBase;
	typedef _regexp_litnotset_no_surrogates _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef basic_string< t_TyChar, char_traits< t_TyChar >, typename _Alloc_traits< t_TyChar, t_TyAllocator >::allocator_type > _TyString;
	
	_TyString	m_s;

	_regexp_litnotset_no_surrogates(	const t_TyChar * _pc,
											t_TyAllocator const & _alloc = t_TyAllocator() )
		: m_s( _pc, _alloc )
	{
	}
	// REVIEW:<dbien>: Don't remember why I have this for the copy constructor...
	_regexp_litnotset_no_surrogates( _TyThis const & _r, std::false_type = std::false_type() )
    : m_s( _r.m_s )
  {
  }

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateLiteralNotInSetNFANoSurrogates( m_s.c_str() );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return false; } // assuming false here... the fact is that if it does matches empty then it only matches nothing...

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "[^" << reinterpret_cast< const _TyCharOstream * >( &m_s[0] ) << "]";
	}
#endif //__RGEXP_DUMP
};
template < class t_TyChar >
__INLINE _regexp_litnotset_no_surrogates< t_TyChar >
litnotset_no_surrogates( const t_TyChar * _pc )
{
	return _regexp_litnotset_no_surrogates< t_TyChar >( _pc );
}
template < class t_TyChar, class t_TyAllocator >
__INLINE _regexp_litnotset_no_surrogates< t_TyChar, t_TyAllocator >
litnotset_no_surrogates( const t_TyChar * _pc, t_TyAllocator const & _rAlloc )
{
	return _regexp_litnotset_no_surrogates< t_TyChar >( _pc, _rAlloc );
}

// _regexp_litanyinset: a single literal present in the set given in the string.
template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_litanyinset 
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar > _TyBase;
	typedef _regexp_litanyinset _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef basic_string< t_TyChar, char_traits< t_TyChar >, typename _Alloc_traits< t_TyChar, t_TyAllocator >::allocator_type > _TyString;
	
	_TyString	m_s;

	_regexp_litanyinset(	const t_TyChar * _pc,
											t_TyAllocator const & _alloc = t_TyAllocator() )
		: m_s( _pc, _alloc )
	{
	}
	// REVIEW:<dbien>: Don't remember why I have this for the copy constructor...
	_regexp_litanyinset( _TyThis const & _r, std::false_type = std::false_type() )
    : m_s( _r.m_s )
  {
  }

	void ConstructNFA( _TyCtxtBase & _rNfaCtxt, size_t _stLevel = 0 ) const override
	{
		_rNfaCtxt.CreateLiteralAnyInSetNFA( m_s.c_str() );
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return true; }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return false; }

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		// Note that this could look pretty ugly in the output...
		_ros << "[" << reinterpret_cast< const _TyCharOstream * >( &m_s[0] ) << "]";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_litanyinset< t_TyChar >
litanyinset( const t_TyChar * _pc )
{
	return _regexp_litanyinset< t_TyChar >( _pc );
}

template < class t_TyChar, class t_TyAllocator >
__INLINE _regexp_litanyinset< t_TyChar, t_TyAllocator >
litanyinset( const t_TyChar * _pc, t_TyAllocator const & _rAlloc )
{
	return _regexp_litanyinset< t_TyChar >( _pc, _rAlloc );
}

template < class t_TyRegExp1, class t_TyRegExp2 >
class _regexp_follows : public _regexp_base< typename t_TyRegExp1::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp1::_TyChar > _TyBase;
	typedef _regexp_follows< t_TyRegExp1, t_TyRegExp2 >	_TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp1	_TyRegExp1;
	typedef t_TyRegExp2	_TyRegExp2;

	t_TyRegExp1	m_re1;
	t_TyRegExp2	m_re2;

	_regexp_follows(	t_TyRegExp1 const & _re1,
										t_TyRegExp2	const & _re2 )
		: m_re1( _re1, std::false_type() ),
			m_re2( _re2, std::false_type() )
	{
	}
	_regexp_follows( _TyThis const & _r, std::false_type = std::false_type() )
		: m_re1( _r.m_re1, std::false_type() ),
			m_re2( _r.m_re2, std::false_type() )
  {
  }

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{ 
		return m_re1.FMatchesEmpty() && m_re2.FMatchesEmpty();
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa1, size_t _stLevel = 0 ) const override
	{
		// Create a new context to pass to expression two:
		// Nfa1 is used to create and destroy Nfa2:
		_TyCtxtBase *	pcbNfa2;
		_rcbNfa1.Clone( &pcbNfa2 );
		CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
			dtorNfa2( &_rcbNfa1, &_TyCtxtBase::DestroyOther, pcbNfa2 );

		m_re1.ConstructNFA( _rcbNfa1, _stLevel+2 );
		m_re2.ConstructNFA( *pcbNfa2, _stLevel+2 );

		_rcbNfa1.CreateFollowsNFA( *pcbNfa2 );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "( " << m_re1 << " ) *\n ( " << m_re2 << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_follows< t_TyRegExp1, t_TyRegExp2 >
follows( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
	return _regexp_follows< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 );
}

template < class t_TyRegExp1, class t_TyRegExp2 >
class _regexp_or : public _regexp_base< typename t_TyRegExp1::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp1::_TyChar > _TyBase;
	typedef _regexp_or< t_TyRegExp1, t_TyRegExp2 > _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp1	_TyRegExp1;
	typedef t_TyRegExp2	_TyRegExp2;

	t_TyRegExp1	m_re1;
	t_TyRegExp2	m_re2;

	_regexp_or(	t_TyRegExp1 const & _re1,
							t_TyRegExp2	const & _re2 )
		: m_re1( _re1, std::false_type() ),
			m_re2( _re2, std::false_type() )
	{
	}
	_regexp_or( _TyThis const & _r, std::false_type = std::false_type() )
		: m_re1( _r.m_re1, std::false_type() ),
			m_re2( _r.m_re2, std::false_type() )
  {
  }

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{ 
		return m_re1.FMatchesEmpty() || m_re2.FMatchesEmpty();
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa1, size_t _stLevel = 0 ) const override
	{
		// Create a new context to pass to expression two:
		// Nfa1 is used to create and destroy Nfa2:
		_TyCtxtBase *	pcbNfa2;
		_rcbNfa1.Clone( &pcbNfa2 );
		CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
			dtorNfa2( &_rcbNfa1, &_TyCtxtBase::DestroyOther, pcbNfa2 );

		m_re1.ConstructNFA( _rcbNfa1, _stLevel+2 );
		m_re2.ConstructNFA( *pcbNfa2, _stLevel+2 );

		_rcbNfa1.CreateOrNFA( *pcbNfa2 );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "( " << m_re1 << " ) |\n ( " << m_re2 << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_or< t_TyRegExp1, t_TyRegExp2 >
Or( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
	return _regexp_or< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 );
}

template < class t_TyRegExp >
class _regexp_zeroormore : public _regexp_base< typename t_TyRegExp::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp::_TyChar > _TyBase;
	typedef _regexp_zeroormore< t_TyRegExp > _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp	_TyRegExp;

	t_TyRegExp	m_re;

	_regexp_zeroormore(	t_TyRegExp const & _re )
		: m_re( _re, std::false_type() )
	{
	}
	_regexp_zeroormore( _TyThis const & _r,
                      std::false_type = std::false_type() )
		: m_re( _r.m_re, std::false_type() )
	{
	}

	bool FMatchesEmpty() const _BIEN_NOTHROW override
	{ 
		return true;
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa, size_t _stLevel = 0 ) const override
	{
		m_re.ConstructNFA( _rcbNfa, _stLevel+2 );
		_rcbNfa.CreateZeroOrMoreNFA();
	}

 	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "~( " << m_re << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp >
__INLINE _regexp_zeroormore< t_TyRegExp >
zeroormore( t_TyRegExp const & _re )
{
	return _regexp_zeroormore< t_TyRegExp >( _re );
}

template < class t_TyRegExp >
__INLINE _regexp_follows< t_TyRegExp, _regexp_zeroormore< t_TyRegExp > >
oneormore( t_TyRegExp const & _re )
{
	return follows( _re, zeroormore( _re ) );
}

template < class t_TyRegExp >
__INLINE _regexp_or< t_TyRegExp, _regexp_empty< typename t_TyRegExp::_TyChar > >
zeroorone( t_TyRegExp const & _re )
{
	typedef _regexp_empty< typename t_TyRegExp::_TyChar >	_TyEmpty;
	return Or( _re, _TyEmpty() );
}

// excludes - excludes patterns in <t_TyRegExp2> explicitly from matching.

template < class t_TyRegExp1, class t_TyRegExp2 >
class _regexp_excludes : public _regexp_base< typename t_TyRegExp1::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp1::_TyChar > _TyBase;
	typedef _regexp_excludes< t_TyRegExp1, t_TyRegExp2 > _TyThis;
protected:
	using _TyBase::_CloneHelper;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp1	_TyRegExp1;
	typedef t_TyRegExp2	_TyRegExp2;

	t_TyRegExp1	m_re1;
	t_TyRegExp2	m_re2;

	_regexp_excludes(	t_TyRegExp1 const & _re1,
							      t_TyRegExp2	const & _re2 )
		: m_re1( _re1, std::false_type() ),
			m_re2( _re2, std::false_type() )
	{
	}
	_regexp_excludes( _TyThis const & _r, 
                    std::false_type = std::false_type() )
		: m_re1( _r.m_re1, std::false_type() ),
			m_re2( _r.m_re2, std::false_type() )
  {
  }

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{
    // REVIEW: <dbien>: Is this correct ?
		return m_re1.FMatchesEmpty() && !m_re2.FMatchesEmpty();
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa1, size_t _stLevel = 0 ) const override
	{
		// Create a new context to pass to expression two:
		// Nfa1 is used to create and destroy Nfa2:
		_TyCtxtBase *	pcbNfa2;
		_rcbNfa1.Clone( &pcbNfa2 );
		CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
			dtorNfa2( &_rcbNfa1, &_TyCtxtBase::DestroyOther, pcbNfa2 );

		m_re1.ConstructNFA( _rcbNfa1, _stLevel+2 );
		m_re2.ConstructNFA( *pcbNfa2, _stLevel+2 );

		_rcbNfa1.CreateExcludesNFA( *pcbNfa2 );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "( " << m_re1 << " ) -\n ( " << m_re2 << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_excludes< t_TyRegExp1, t_TyRegExp2 >
excludes( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
	return _regexp_excludes< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 );
}

// completes - specifies a finishing pattern for a repeating sequence.

template < class t_TyRegExp1, class t_TyRegExp2 >
class _regexp_completes : public _regexp_base< typename t_TyRegExp1::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp1::_TyChar > _TyBase;
	typedef _regexp_completes< t_TyRegExp1, t_TyRegExp2 > _TyThis;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp1	_TyRegExp1;
	typedef t_TyRegExp2	_TyRegExp2;

	t_TyRegExp1	m_re1;
	t_TyRegExp2	m_re2;

	_regexp_completes(  t_TyRegExp1 const & _re1,
							        t_TyRegExp2	const & _re2 )
		: m_re1( _re1, std::false_type() ),
			m_re2( _re2, std::false_type() )
	{
	}
	_regexp_completes(  _TyThis const & _r, 
                      std::false_type = std::false_type() )
		: m_re1( _r.m_re1, std::false_type() ),
			m_re2( _r.m_re2, std::false_type() )
  {
  }

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{
		return m_re1.FMatchesEmpty() && m_re2.FMatchesEmpty();
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa1, size_t _stLevel = 0 ) const override
	{
		// Create a new context to pass to expression two:
		// Nfa1 is used to create and destroy Nfa2:
		_TyCtxtBase *	pcbNfa2;
		_rcbNfa1.Clone( &pcbNfa2 );
		CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
			dtorNfa2( &_rcbNfa1, &_TyCtxtBase::DestroyOther, pcbNfa2 );

		m_re1.ConstructNFA( _rcbNfa1, _stLevel+2 );
		m_re2.ConstructNFA( *pcbNfa2, _stLevel+2 );

		_rcbNfa1.CreateCompletesNFA( *pcbNfa2 );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_TyBase::_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "( " << m_re1 << " ) +\n ( " << m_re2 << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_completes< t_TyRegExp1, t_TyRegExp2 >
completes( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
	return _regexp_completes< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 );
}

template < class t_TyRegExp1, class t_TyRegExp2 >
class _regexp_lookahead : public _regexp_base< typename t_TyRegExp1::_TyChar >
{
private:
	typedef _regexp_base< typename t_TyRegExp1::_TyChar >											_TyBase;
	typedef _regexp_lookahead< t_TyRegExp1, t_TyRegExp2 >	_TyThis;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef t_TyRegExp1	_TyRegExp1;
	typedef t_TyRegExp2	_TyRegExp2;

	t_TyRegExp1	m_re1;
	t_TyRegExp2	m_re2;

	_regexp_lookahead(  t_TyRegExp1 const & _re1,
							        t_TyRegExp2	const & _re2 )
		: m_re1( _re1, std::false_type() ),
			m_re2( _re2, std::false_type() )
	{
	}
	_regexp_lookahead(  _TyThis const & _r, 
                      std::false_type = std::false_type() )
		: m_re1( _r.m_re1, std::false_type() ),
			m_re2( _r.m_re2, std::false_type() )
  {
  }

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{ 
		return m_re1.FMatchesEmpty() && m_re2.FMatchesEmpty();
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa1, size_t _stLevel = 0 ) const override
	{
		// Create a new context to pass to expression two:
		// Nfa1 is used to create and destroy Nfa2:
		_TyCtxtBase *	pcbNfa2;
		_rcbNfa1.Clone( &pcbNfa2 );
		CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
			dtorNfa2( &_rcbNfa1, &_TyCtxtBase::DestroyOther, pcbNfa2 );

		m_re1.ConstructNFA( _rcbNfa1, _stLevel+2 );
		m_re2.ConstructNFA( *pcbNfa2, _stLevel+2 );

		if ( _stLevel < 2 )
		{
			_rcbNfa1.CreateLookaheadNFA( *pcbNfa2 );
		}
		else
		{
			// We are not a toplevel lookahead - default to a follows NFA:
			_rcbNfa1.CreateFollowsNFA( *pcbNfa2 );
		}
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_TyBase::_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "( " << m_re1 << " ) /\n ( " << m_re2 << " )";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_lookahead< t_TyRegExp1, t_TyRegExp2 >
lookahead( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
	return _regexp_lookahead< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 );
}

// "Free" action - like a trigger, except no trigger semantics,
//  action is maintained even in sub-expressions.
// REVIEW: <dbien>: Is this useful for anything ? I forgot. :-)

template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_action
	: public _regexp_base< t_TyChar >,
		public _alloc_base< char, t_TyAllocator >
{
private:
	typedef _regexp_base< t_TyChar >									_TyBase;
	typedef _regexp_action< t_TyChar, t_TyAllocator >	_TyThis;
	typedef _alloc_base< char, t_TyAllocator >				_TyAllocBase;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef _l_action_object_base< t_TyChar, true >	_TyActionObjectBase;
	typedef _sdp_vbase< _TyActionObjectBase >				_TySdpActionBase;

	_dtorp< _TySdpActionBase >	m_pSdpAction;

	// Construct from base - this copies the base object.
	template < class t_TyActionObject >
	_regexp_action(	t_TyActionObject _ao,
										t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyAllocBase( _rAlloc )
	{
		typedef _sdpv< t_TyActionObject, t_TyAllocator >	_TySdp;
		m_pSdpAction = _TySdp::template construct1< t_TyActionObject const & >( _ao, _TyAllocBase::get_allocator() );
	}

	// We always clone the action actions.
	_regexp_action( const _TyThis & _r, std::false_type = std::false_type() )
		: _TyAllocBase( _r )
	{
		if ( _r.m_pSdpAction )
		{
			_r.m_pSdpAction->clone( &m_pSdpAction.PtrRef() );
		}
	}

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{ 
		return true;
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa, size_t _stLevel = 0 ) const override
	{
		_rcbNfa.CreateEmptyNFA();
		
		// If we have an action object then set into NFA:
		_rcbNfa.SetAction( m_pSdpAction, e_atFreeAction );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "action";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyActionObject, class t_TyAllocator >
_regexp_action< typename t_TyActionObject::_TyChar, t_TyAllocator >
action(	t_TyActionObject _ao, 
				t_TyAllocator const & _rAlloc = __L_DEFAULT_ALLOCATOR () )
{
	typedef typename t_TyActionObject::_TyChar	_TyChar;
	return _regexp_action< _TyChar, t_TyAllocator >( _ao, _rAlloc );
}

// Trigger action:
template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_trigger
	: public _regexp_base< t_TyChar >,
		public _alloc_base< char, t_TyAllocator >
{
private:
	typedef _regexp_base< t_TyChar >									_TyBase;
	typedef _regexp_trigger< t_TyChar, t_TyAllocator >	_TyThis;
	typedef _alloc_base< char, t_TyAllocator >				_TyAllocBase;
public:

	using _TyAllocBase::get_allocator;

	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef _l_action_object_base< t_TyChar, true >	_TyActionObjectBase;
	typedef _sdp_vbase< _TyActionObjectBase >				_TySdpActionBase;

	_dtorp< _TySdpActionBase >	m_pSdpAction;

	// Construct from base - this copies the base object.
	template < class t_TyActionObject >
	_regexp_trigger(	t_TyActionObject _ao,
										t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyAllocBase( _rAlloc )
	{
		typedef _sdpv< t_TyActionObject, t_TyAllocator >	_TySdp;
		m_pSdpAction = _TySdp::template construct1< t_TyActionObject const & >( _ao, get_allocator() );
	}

	// We always clone the trigger actions.
	_regexp_trigger( const _TyThis & _r, std::false_type = std::false_type() )
		: _TyAllocBase( _r.get_allocator() )
	{
		if ( _r.m_pSdpAction )
		{
			_r.m_pSdpAction->clone( &m_pSdpAction.PtrRef() );
		}
	}

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{
		// triggers have to match an input trigger so that don't match empty.
		return false;
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa, size_t _stLevel = 0 ) const override
	{
		VerifyThrowSz( !!m_pSdpAction, "Attempt to construct a trigger without a corresponding action object is not valid." );
		#ifndef REGEXP_NO_TRIGGERS
		bool fCreatedTrigger;
    try
    {
      fCreatedTrigger = _rcbNfa.FCreateTriggerNFA( **m_pSdpAction );
    }
    catch (regexp_trigger_found_first_exception & _rexc)
    {
      // Record the action to which we are responding:
      if (!!m_pSdpAction)
      {
        stringstream strstr;
        strstr << _rexc.what() << ":";
        (*m_pSdpAction)->RenderActionType(strstr, "Unknown");
        string strAction = strstr.str();
        _rexc.SetWhat(strAction);
        throw;
      }
    }
		
		// If we have an action object then set into NFA - if we created the trigger and didn't ignore it.
		if ( fCreatedTrigger )
			_rcbNfa.SetAction( m_pSdpAction, e_atTrigger );
		#else //!REGEXP_NO_TRIGGERS
		_rcbNfa.CreateRangeNFA( _TyRange(0,0) );
		#endif //!REGEXP_NO_TRIGGERS
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_TyBase::_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "trigger";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyActionObject, class t_TyAllocator >
_regexp_trigger< typename t_TyActionObject::_TyChar, t_TyAllocator >
trigger(	t_TyActionObject _ao, 
					t_TyAllocator const & _rAlloc = __L_DEFAULT_ALLOCATOR () )
{
	typedef typename t_TyActionObject::_TyChar	_TyChar;
	return _regexp_trigger< _TyChar, t_TyAllocator >( _ao, _rAlloc );
}

// unsatsifiable transition.
// Can be used to "prune" regions of the resultant DFA.

template < class t_TyChar >
class _regexp_unsatisfiable
	: public _regexp_base< t_TyChar >
{
private:
	typedef _regexp_base< t_TyChar >					_TyBase;
	typedef _regexp_unsatisfiable< t_TyChar >	_TyThis;
public:
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	
	size_t m_nUnsatisfiable;

	_regexp_unsatisfiable( size_t _nUnsatisfiable )
		: m_nUnsatisfiable( _nUnsatisfiable )
	{
	}

	_regexp_unsatisfiable(  const _TyThis & _r, 
                          std::false_type = std::false_type() )
		: m_nUnsatisfiable( _r.m_nUnsatisfiable )
	{
	}

	bool FMatchesEmpty() const _BIEN_NOTHROW override	
	{
		// not really sure what to return here - these get removed from the graph.
		return false;
	}

	void ConstructNFA( _TyCtxtBase & _rcbNfa, size_t _stLevel = 0 ) const override
	{
		_rcbNfa.CreateUnsatisfiableNFA( m_nUnsatisfiable );
	}

	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		_TyBase::_CloneHelper( this, _prbCopier, _pprbStorage );
	}

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "unsatisfiable(" << m_nUnsatisfiable << ")";
	}
#endif //__RGEXP_DUMP
};

template < class t_TyChar >
__INLINE _regexp_unsatisfiable< t_TyChar >
unsatisfiable( int _nUnsatisfiable, t_TyChar = _L_REGEXP_DEFAULTCHAR() )
{
	return _regexp_unsatisfiable< t_TyChar >( _nUnsatisfiable );
}

// Final regular expression:

template < class t_TyChar, class t_TyAllocator = __L_DEFAULT_ALLOCATOR >
class _regexp_final
	: public _regexp_base< t_TyChar >,
		public _alloc_base< char, t_TyAllocator >
{
private:
	typedef _regexp_base< t_TyChar >									_TyBase;
	typedef _regexp_final< t_TyChar, t_TyAllocator >	_TyThis;
	typedef _alloc_base< char, t_TyAllocator >				_TyAllocBase;
public:
	typedef t_TyChar _TyChar;
	using _TyAllocBase::get_allocator;
	using typename _TyBase::_TyCtxtBase;
	using typename _TyBase::_TyOstream;
	using typename _TyBase::_TyCharOstream;
	typedef _l_action_object_base< t_TyChar, true >	_TyActionObjectBase;
	typedef _sdp_vbase< _TyActionObjectBase >				_TySdpActionBase;

	_dtorp< _TyBase	>						m_pbre;
	size_t											m_stFinalSize;	
	_dtorp< _TySdpActionBase >	m_pSdpAction;

  typedef typename _Alloc_traits< typename list< _TyThis >::value_type, t_TyAllocator >::allocator_type _TyFinalListAllocator;
  typedef list< _TyThis, _TyFinalListAllocator >	_TyFinalList;
	_TyFinalList m_lAlternatives;

	// Construct from base - this copies the base object.
	_regexp_final(	_TyBase const & _r,
									t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyAllocBase( _rAlloc ),
			m_lAlternatives( _rAlloc )
	{
		_r.Clone( this, &m_pbre.PtrRef() );
	}

	// Full copy constructor - this copies everything.
	_regexp_final( _TyThis const & _r )
		: _TyAllocBase( _r.get_allocator() ),
			m_lAlternatives( _r.m_lAlternatives )
	{
		_r.m_pbre->Clone( this, &m_pbre.PtrRef() );
		if ( _r.m_pSdpAction )
		{
			_r.m_pSdpAction->clone( &m_pSdpAction.PtrRef() );
		}
	}

	// Partial copy constructor - this copies the current regular expression - 
	//	not any associated rules {m_lAlternatives} nor the action object.
	_regexp_final( _TyThis const & _r, std::false_type )
		: _TyAllocBase( _r.get_allocator() ),
			m_lAlternatives( _r.get_allocator() )
	{
		_r.m_pbre->Clone( this, &m_pbre.PtrRef() );
	}

	~_regexp_final()
	{
		char * pcDealloc = reinterpret_cast< char* >( m_pbre.Ptr() );
		m_pbre.Release(); // Call virtual destructor.
		_TyAllocBase::deallocate_n( pcDealloc, m_stFinalSize );
	}

	template < class t_TyActionObject >
	void	SetAction( t_TyActionObject _ao ) 
		requires( t_TyActionObject::_TyFIsToken::value )
	{
		typedef _sdpv< t_TyActionObject, t_TyAllocator >	_TySdp;
		m_pSdpAction = _TySdp::template construct1< t_TyActionObject const & >( _ao, get_allocator() );
	}

	// We have an associated list(and sublists) of final rules - these implement the 
  //  lexical analyzer. Before conversion to an NFA all the rules in this list ( and
	//	sub-lists within this list, etc. ) are merged to produce the complete lexical NFA.
	// The rules are disambiguated according to the place in the tree formed by
	//	the lists and sub-lists. This tree is traversed in a depth first manner.
	// The start rule has the highest precedence. Then the first in its list(a). Then
	//	the first in the list of (a), etc.
  // NOTE: The added rule is copied by this method, this means:
  //  a) All sub-rules of the added rule are copied.
  //  b) Any changes made to the added rule ( i.e. setting an action into it,
  //      or adding a sub-rule to it ) are not reflected here.
	void AddRule( _TyThis const & _r )
	{
		m_lAlternatives.push_back( _r );
	}

	// Construct an NFA from this rule into the passed context.
  // NOTE: <_stLevelDONTPASS> should be left defaulted.
	void ConstructNFA( _TyCtxtBase & _rcbNfa, 
                       size_t _stLevelDONTPASS = 0 ) const override
	{
		m_pbre->ConstructNFA( _rcbNfa, 
      _stLevelDONTPASS ? _stLevelDONTPASS : _stLevelDONTPASS + 1 );

		// If we have an action object then set into NFA:
		if ( m_pSdpAction )
		{
			_rcbNfa.SetAction( m_pSdpAction );
		}

		// Add additional rules:
		if ( !_stLevelDONTPASS && !m_lAlternatives.empty() )
		{
			// Each rule will be a child of a new start state:
			_rcbNfa.StartAddRules();	// Construct new start state.
			_AddAlternatives( _rcbNfa, this );	// start potential recursion.
		}
	}

	bool FIsLiteral() const _BIEN_NOTHROW override		{ return m_pbre->FIsLiteral(); }
	bool FMatchesEmpty() const _BIEN_NOTHROW override	{ return m_pbre->FMatchesEmpty(); }

#ifdef __RGEXP_DUMP
	void Dump( _TyOstream & _ros ) const override
	{
		_ros << "Final : " << *m_pbre;
	}
#endif //__RGEXP_DUMP

protected:

  // Clone this rule. 
	void Clone( _TyBase * _prbCopier, _TyBase ** _pprbStorage ) const override
	{
		// We will clone this - since we know ( cuz were in this method ) that
		//	this final is not at the top level - use the partial copy:
		_TyBase::_CloneHelper( this, _prbCopier, _pprbStorage, std::false_type() );
	}

// Allocation virtuals:
	char * _CPAllocate( size_t _st ) override
	{
		m_stFinalSize = _st;
		char *	cpAllocated;
		_TyAllocBase::allocate_n( cpAllocated, _st );
		return cpAllocated;
	}

	void _Deallocate( char * _cp, size_t _st ) _BIEN_NOTHROW
	{
		_TyAllocBase::deallocate_n( _cp, _st );
	}

	static void
	_AddAlternatives( _TyCtxtBase & _rcbNfa, const _TyThis * _pAdd )
	{
		// The rule directly inside <_pAdd> has been added -
		//	add each rule in the list inside <_pAdd> recursively:
		typename _TyFinalList::const_iterator it = _pAdd->m_lAlternatives.begin();
		typename _TyFinalList::const_iterator itEnd = _pAdd->m_lAlternatives.end();
		for ( ; it != itEnd; ++it )
		{
			{ // SCOPE
				_TyCtxtBase *	pcbNfaAlt;
				_rcbNfa.Clone( &pcbNfaAlt );
				CMFDtor1_void< _TyCtxtBase, _TyCtxtBase * >	
					dtorNfaAlt( &_rcbNfa, &_TyCtxtBase::DestroyOther, pcbNfaAlt );
				
				it->ConstructNFA( *pcbNfaAlt, 1 );	// construct NFA for alternative.

				_rcbNfa.AddAlternativeNFA( *pcbNfaAlt );
			}

			// If this state has any alternatives then recurse:
			if ( !it->m_lAlternatives.empty() )
			{
				_AddAlternatives( _rcbNfa, &(*it) );
			}
		}
	}
};

__REGEXP_END_NAMESPACE

__REGEXP_OP_BEGIN_NAMESPACE

template < class t_TyRegExp >
__INLINE _regexp_zeroormore< t_TyRegExp >
operator ~ ( t_TyRegExp const & _re )
{
	return _regexp_zeroormore< t_TyRegExp >( _re );
}

template < class t_TyRegExp >
__INLINE _regexp_follows< t_TyRegExp, _regexp_zeroormore< t_TyRegExp > >
operator ++ ( t_TyRegExp const & _re )
{
	return oneormore( _re );
}

template < class t_TyRegExp >
__INLINE _regexp_or< t_TyRegExp, _regexp_empty< typename t_TyRegExp::_TyChar > >
operator -- ( t_TyRegExp const & _re )
{
	return zeroorone( _re );
}

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_follows< t_TyRegExp1, t_TyRegExp2 >
operator * ( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
  return _regexp_follows< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 ); 
}

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_or< t_TyRegExp1, t_TyRegExp2 >
operator | ( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
  return _regexp_or< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 ); 
}

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_excludes< t_TyRegExp1, t_TyRegExp2 >
operator - ( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
  return _regexp_excludes< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 ); 
}

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_completes< t_TyRegExp1, t_TyRegExp2 >
operator + ( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
  return _regexp_completes< t_TyRegExp1, t_TyRegExp2 >( _re1, _re2 ); 
}

template < class t_TyRegExp1, class t_TyRegExp2 >
__INLINE _regexp_lookahead< t_TyRegExp1, t_TyRegExp2 >
operator / ( t_TyRegExp1 const & _re1, t_TyRegExp2 const & _re2 )
{
  return lookahead( _re1, _re2 ); 
}

__REGEXP_OP_END_NAMESPACE

#endif //__L_RGEXP_H

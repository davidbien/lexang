#ifndef __L_CHRNG_H
#define __L_CHRNG_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_chrng.h

// character range class.
// Range is [first,second] ( i.e. not [first,second). )
// Can use [0,0] for empty element - since not valid input.

#include <ostream>
#include "jsonstrm.h"

__REGEXP_BEGIN_NAMESPACE

// We store the elements as t_TyRangeEl which is larger than t_TyChar because there are values that are 
//	outside of the character range.
template < class t_TyRangeEl, class t_TyChar >
struct _fa_char_range : public pair< t_TyRangeEl, t_TyRangeEl >
{
private:
	typedef pair< t_TyRangeEl, t_TyRangeEl > _TyBase;
	typedef _fa_char_range _TyThis;
public:
	typedef t_TyRangeEl _TyRangeEl;
	typedef t_TyChar _TyChar;
	typedef typename _l_char_type_map< _TyChar >::_TyUnsigned _TyUnsignedChar;

	using _TyBase::first;
	using _TyBase::second;

	_fa_char_range( _fa_char_range const & ) = default;
	~_fa_char_range() = default;
	
	_fa_char_range()
		: _TyBase( 0, 0 )
	{
	}

	_fa_char_range( t_TyRangeEl _rFirst, t_TyRangeEl _rSecond )
		: _TyBase( _rFirst, _rSecond )
	{
		Assert( second >= first );
	}

	void set_empty()
	{
		first = 0;
		second = 0;
	}

	bool	empty() const _BIEN_NOTHROW
	{
		if ( !first )
		{
			Assert( !second );
			return true;
		}
		return false;
	}

	bool	operator < ( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return second < _r.first;
	}
	// This is for containers which can have overlapping char ranges.
	bool CanonicalLess( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return static_cast< _TyBase const & >( *this ) < _r;
	}

	bool	intersects( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		if ( first < _r.first )
			return second >= _r.first;
		else
			return first <= _r.second;
	}

	bool	contains( t_TyRangeEl const & _r ) const _BIEN_NOTHROW
	{
		return _r >= first && _r <= second;
	}

	bool	isconsecutiveleft( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return _r.second+1 == first;
	}
	bool	isconsecutiveright( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return second+1 == _r.first;
	}
	bool	isconsecutive( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return isconsecutiveleft( _r ) || isconsecutiveright( _r );
	}
	// Return this range without _r and return any remaining range in _rSecondResult.
	void remove( _TyThis const & _r, _TyThis & _rSecondResult )
	{
		if ( first < _r.first )
		{
			if ( second > _r.second )
			{
				_rSecondResult.first = _r.second+1;
				_rSecondResult.second = second;
			}
			else
			{
				_rSecondResult.set_empty();
				if ( second < _r.first )
					return;
			}
			second = _r.first-1;
		}
		else
		{
			_rSecondResult.set_empty();
			if ( first <= _r.second )
			{
				if ( second <= _r.second )
					set_empty();
				else
					first = _r.second+1;
			}
		}			
	}

	friend std::ostream & operator << ( std::ostream & _ros, const _TyThis & _r )
	{
    std::ostream::sentry s(_ros);
    if ( s ) 
		{
			_ros << "['" << ( _r.first ? _r.first : ' ' ) << 
						"'-'" << ( _r.second ? _r.second : ' ' ) << "']";
		}
		return _ros;
	}
	void DumpToString( string & _rstr ) const
	{
		PrintfStdStr( _rstr, "[%lu-%lu]", first, second );
	}

	template < class t_TyJsonValueLife >
	void ToJSONStream( t_TyJsonValueLife & _jvl  ) const
	{
		Assert( _jvl.FAtArrayValue() );
		if ( _jvl.FAtArrayValue() )
		{
			if ( !first )
				_jvl.WriteNullValue();
			else
			if ( ( first > 254/* _l_char_type_map< _TyChar >::ms_kcMax */ ) || ( first < 32 ) ) // filter out unprintable characters.
				_jvl.PrintfStringValue( "0x%lx", size_t((_TyUnsignedChar)first) );
			else
			{
				_TyChar ch = (_TyChar)first;
				_jvl.WriteStringValue( &ch, 1 );
			}
			if ( !second )
				_jvl.WriteNullValue();
			else
			if ( ( second > 254/* )_l_char_type_map< _TyChar >::ms_kcMax */ ) || ( second < 32 ) ) // filter out unprintable characters.
				_jvl.PrintfStringValue( "0x%lx", size_t((_TyUnsignedChar)second) );
			else
			{
				_TyChar ch = (_TyChar)second;
				_jvl.WriteStringValue( &ch, 1 );
			}
		}
	}
};

template < class t_TyNfaCharRange >
struct _fa_char_range_intersect
{
	bool operator ()( t_TyNfaCharRange const & _rL, t_TyNfaCharRange const & _rR ) const _BIEN_NOTHROW
	{
		return _rL.intersects( _rR );
	}
};

// Calls _TyNfaCharRange::CanonicalLess().
template < class t_TyNfaCharRange >
struct _fa_char_range_canonical_less
{
	bool operator ()( t_TyNfaCharRange const & _rL, t_TyNfaCharRange const & _rR ) const _BIEN_NOTHROW
	{
		return _rL.CanonicalLess( _rR );
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_CHRNG_H

#ifndef __L_CHRNG_H
#define __L_CHRNG_H

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
	}

	bool	empty() const _BIEN_NOTHROW
	{
		if ( !first )
		{
			assert( !second );
			return true;
		}
		return false;
	}

	bool	operator < ( _TyThis const & _r ) const _BIEN_NOTHROW
	{
		return second < _r.first;
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

	friend std::ostream & operator << ( std::ostream & _ros, const _TyThis & _r )
	{
		_ros << "['" << ( _r.first ? _r.first : ' ' ) << 
					 "'-'" << ( _r.second ? _r.second : ' ' ) << "']";
		return _ros;
	}

	template < class t_TyJsonValueLife >
	void ToJSONStream( t_TyJsonValueLife & _jvl  ) const
	{
		assert( _jvl.FAtArrayValue() );
		if ( _jvl.FAtArrayValue() )
		{
			if ( !first )
				_jvl.WriteNullValue();
			else
			if ( ( first > _l_char_type_map< _TyChar >::ms_kcMax ) || ( first < 32 ) ) // filter out unprintable characters.
				_jvl.WriteValue( (_TyUnsignedChar)first );
			else
			{
				_TyChar ch = (_TyChar)first;
				_jvl.WriteStringValue( &ch, 1 );
			}
			if ( !second )
				_jvl.WriteNullValue();
			else
			if ( ( second > _l_char_type_map< _TyChar >::ms_kcMax ) || ( second < 32 ) ) // filter out unprintable characters.
				_jvl.WriteValue( (_TyUnsignedChar)second );
			else
			{
				_TyChar ch = (_TyChar)second;
				_jvl.WriteStringValue( &ch, 1 );
			}
		}
	}
};

template < class _TyNfaCharRange >
struct _fa_char_range_intersect
#ifdef __LEXANG_USE_STLPORT
	: public binary_function< _TyNfaCharRange, _TyNfaCharRange, bool >
#endif //__LEXANG_USE_STLPORT
{
	bool operator ()( _TyNfaCharRange const & _rL, _TyNfaCharRange const & _rR ) const _BIEN_NOTHROW
	{
		return _rL.intersects( _rR );
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_CHRNG_H

#ifndef __L_CHRNG_H
#define __L_CHRNG_H

// _l_chrng.h

// character range class.
// Range is [first,second] ( i.e. not [first,second). )
// Can use [0,0] for empty element - since not valid input.

__REGEXP_BEGIN_NAMESPACE

template < class t_TyChar >
struct _fa_char_range : public pair< t_TyChar, t_TyChar >
{
private:
	typedef pair< t_TyChar, t_TyChar >	_TyBase;
	typedef _fa_char_range< t_TyChar >	_TyThis;
public:
	using _TyBase::first;
	using _TyBase::second;
	
	_fa_char_range()
		: _TyBase( 0, 0 )
	{
	}

	_fa_char_range( t_TyChar _rFirst, t_TyChar _rSecond )
		: _TyBase( _rFirst, _rSecond )
	{
	}

	bool	empty() const _STLP_NOTHROW
	{
		if ( !first )
		{
			assert( !second );
			return true;
		}
		return false;
	}

	bool	operator < ( _TyThis const & _r ) const _STLP_NOTHROW
	{
		return second < _r.first;
	}

	bool	intersects( _TyThis const & _r ) const _STLP_NOTHROW
	{
		if ( first < _r.first )
			return second >= _r.first;
		else
			return first <= _r.second;
	}

	bool	contains( t_TyChar const & _r ) const _STLP_NOTHROW
	{
		return _r >= first && _r <= second;
	}

	bool	isconsecutiveleft( _TyThis const & _r ) const _STLP_NOTHROW
	{
		return _r.second+1 == first;
	}
	bool	isconsecutiveright( _TyThis const & _r ) const _STLP_NOTHROW
	{
		return second+1 == _r.first;
	}
	bool	isconsecutive( _TyThis const & _r ) const _STLP_NOTHROW
	{
		return isconsecutiveleft( _r ) || isconsecutiveright( _r );
	}

	friend ostream & operator << ( ostream & _ros, const _TyThis & _r )
	{
		_ros << "['" << ( _r.first ? _r.first : ' ' ) << 
					 "'-'" << ( _r.second ? _r.second : ' ' ) << "']";
		return _ros;
	}

};

template < class _TyNfaCharRange >
struct _fa_char_range_intersect : 
	public binary_function< _TyNfaCharRange, _TyNfaCharRange, bool >
{
	bool operator ()( _TyNfaCharRange const & _rL, _TyNfaCharRange const & _rR ) const _STLP_NOTHROW
	{
		return _rL.intersects( _rR );
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_CHRNG_H

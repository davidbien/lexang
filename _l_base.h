#ifndef __L_BASE_H__
#define __L_BASE_H__

// _l_base.h

// Define the base classes for contexts.
// These aren't dependent on the other templates in the generator.

__REGEXP_BEGIN_NAMESPACE

enum EActionType
{
	e_atNormal = 0,
	e_atTrigger,
	e_atFreeAction
};

template < class t_TyChar >
class _fa_base
{
private:
	typedef _fa_base< t_TyChar >	_TyThis;
public:

	// Range is [first,second] ( i.e. not [first,second). )
	// Can use [0,0] for empty element - since not valid input.
	typedef t_TyChar																					_TyChar;
	typedef typename _l_char_type_map< _TyChar >::_TyUnsigned		_TyUnsignedChar;

	// We need to have non-alphabet elements in the range - in particular we need
	//	to define a trigger:
	typedef typename _l_char_type_map< _TyUnsignedChar >::_TyLarger	_TyRangeEl;
	typedef _fa_char_range< _TyRangeEl >													  _TyRange;

	// Now define the non-alphabet characters:
	static const _TyRangeEl	ms_kreTrigger;
	static const _TyRangeEl	ms_kreUnsatisfiableStart;

	typedef ptrdiff_t	_TyState;

	typedef _l_action_object_base< t_TyChar, true >									_TyActionObjectBase;
	typedef _sdp_vbase< _TyActionObjectBase >												_TySdpActionBase;

	_TyState	m_iCurState;

	_fa_base()
		: m_iCurState( 0 )
	{
	}

	_TyState	NStates() const _BIEN_NOTHROW
	{
		return m_iCurState;
	}

	void			SetNumStates(_TyState _iStates ) _BIEN_NOTHROW
	{
		m_iCurState = _iStates;
	}
};

template < class t_TyChar >
const typename _fa_base< t_TyChar >::_TyRangeEl
_fa_base< t_TyChar >::ms_kreTrigger = 
    _l_char_type_map< typename _fa_base< t_TyChar >::_TyUnsignedChar >::ms_kucTrigger;

template < class t_TyChar >
const typename _fa_base< t_TyChar >::_TyRangeEl
_fa_base< t_TyChar >::ms_kreUnsatisfiableStart = 
	  _l_char_type_map< typename _fa_base< t_TyChar >::_TyUnsignedChar >::ms_kucUnsatisfiableStart;

template < class t_TyChar >
class _context_base
{
private:
	typedef _context_base< t_TyChar >	_TyThis;
public:

	typedef _fa_base< t_TyChar >						_TyFaBase;
	typedef typename _TyFaBase::_TyRange							_TyRange;
	typedef typename _TyFaBase::_TyState							_TyState;
	typedef typename _TyFaBase::_TyActionObjectBase	  _TyActionObjectBase;
	typedef typename _TyFaBase::_TySdpActionBase			_TySdpActionBase;

	_TyFaBase & m_rFaBase;	// The NFA of which we are a constituent.

	_context_base( _TyFaBase & _rFaBase )
		: m_rFaBase( _rFaBase )
	{
	}

	_context_base( _TyThis const & _r )
		: m_rFaBase( _r.m_rFaBase )
	{
	}

	virtual ~_context_base()
	{
	}
};

template < class t_TyChar >
class _nfa_context_base : public _context_base< t_TyChar >
{
private:
	typedef _nfa_context_base< t_TyChar >	_TyThis;
	typedef _context_base< t_TyChar >	_TyBase;
public:

	typedef _fa_base< t_TyChar >						_TyNfaBase;
	typedef typename _TyBase::_TySdpActionBase _TySdpActionBase;
	typedef typename _TyBase::_TyRange _TyRange;

	_nfa_context_base( _TyNfaBase & _rNfaBase )
		: _TyBase( _rNfaBase )
	{
	}

	_nfa_context_base( _TyThis const & _r )
		: _TyBase( _r )
	{
	}

	virtual void	SetAction(	const _TySdpActionBase * _rSdp, 
														enum EActionType _eat = e_atNormal ) = 0;

// The most derived nfa_context will define these virtuals:
	virtual void	Clone( _TyThis ** _pp ) const = 0;
	virtual void	DestroyOther( _TyThis * _pThis ) _BIEN_NOTHROW = 0;
	virtual void	Dump( ostream & _ros ) const = 0;

	// NFA construction virtuals:
	virtual void	CreateEmptyNFA() = 0;
	virtual void	CreateLiteralNFA( t_TyChar const & _rc ) = 0;
	virtual void	CreateStringNFA( t_TyChar const * _pc ) = 0;
	virtual void	CreateRangeNFA( _TyRange const & _rr ) = 0;
	virtual void	CreateFollowsNFA( _TyThis & _rcb ) = 0;
	virtual void	CreateLookaheadNFA( _TyThis & _rcb ) = 0;
	virtual void	CreateTriggerNFA( ) = 0;
	virtual void	CreateOrNFA( _TyThis & _rcb ) = 0;
	virtual void	CreateZeroOrMoreNFA() = 0;
	virtual void	CreateExcludesNFA( _TyThis & _rcb ) = 0;
	virtual void	CreateCompletesNFA( _TyThis & _rcb ) = 0;
	virtual void	CreateUnsatisfiableNFA( size_t _nUnsatisfiable ) = 0;
	virtual void	StartAddRules() = 0;
	virtual void	AddAlternativeNFA( _TyThis & _rcb ) = 0;
};

__REGEXP_END_NAMESPACE

#endif //__L_BASE_H__

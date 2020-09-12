#ifndef __L_AXION_H
#define __L_AXION_H

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_axion.h

// Action objects.

#include <set>
#include <iostream>
#include "_aloctrt.h"

__REGEXP_BEGIN_NAMESPACE

typedef int	_TyActionIdent;
typedef unsigned long _TyLookaheadVector;

template < class t_TyChar, bool t_fInLexGen >
struct _l_action_object_base
{
private:
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyThis;
public:

	typedef t_TyChar	_TyChar;

	// We have a static set of trigger action disambiguating objects.
	typedef pair< _type_info_wrap, _type_info_wrap >		_TyPairTI;
  typedef less< _TyPairTI > _TyCompareTriggers;
  typedef typename _Alloc_traits< typename set<	_TyPairTI, _TyCompareTriggers >::value_type, __L_DEFAULT_ALLOCATOR >::allocator_type _TySetSameTriggersAllocator;
	typedef set<	_TyPairTI, _TyCompareTriggers, 
                _TySetSameTriggersAllocator >		_TySetSameTriggers;
	static _TySetSameTriggers	m_setSameTriggers;

	_l_action_object_base()
	{
	}
	virtual ~_l_action_object_base()
	{
	}

	// Indicates that these trigger actions are equivalent and, when ambiguity is found between both
	//	on a single state, then the one with the lower action id is to be chosen.
	//	( during NFA->DFA conversion )
	void	SameTriggerAs( _TyThis const & _r ) const
	{
		m_setSameTriggers.insert( 
      typename _TySetSameTriggers::value_type( typeid( *this ), typeid( _r ) ) );
	}

	bool	FIsSameAs(	_TyThis const & _r,
										bool _fUseSameTriggerMap ) const
	{
		if ( typeid( *this ) == typeid( _r ) )
		{
			return FSameDataAs( _r );
		}
		else
		if (	_fUseSameTriggerMap &&
					(	( m_setSameTriggers.end() != m_setSameTriggers.find( 
							typename _TySetSameTriggers::value_type( typeid( *this ), 
              typeid( _r ) ) ) ) ||
						( m_setSameTriggers.end() != m_setSameTriggers.find( 
							typename _TySetSameTriggers::value_type( typeid( _r ), 
              typeid( *this ) ) ) ) ) )
		{
			return true;
		}
		return false;
	}

	// We return true by default - most actions use unique typenames.
	// Actions that have significant data members should override.
	virtual	bool	FSameDataAs( _TyThis const & ) const
	{
		return true;
	}

	bool	operator < ( _TyThis const & _r ) const
	{
		type_info const & _rtiThis = typeid( *this );
		type_info const & _rtiR = typeid( _r );
		if ( _rtiThis == _rtiR )
		{
			return FDataLessThan( _r );
		}
		return _rtiThis.before( _rtiR );
	}

	virtual bool	FDataLessThan( _TyThis const & _r ) const
	{
		// the assumption is that there is no data.
		// derived classes with data should override.
		return false;
	}

	virtual void Render(ostream & _ros, const char * _pcCharName) const = 0;
  virtual void Render(stringstream & _ros, const char * _pcCharName) const = 0;
  virtual void RenderW(wostream & _ros, const wchar_t * _pcCharName) const = 0;
  virtual void RenderW(wstringstream & _ros, const wchar_t * _pcCharName) const = 0;
};

template < class t_TyChar, bool t_fInLexGen >
#if 0//def __GNUC__
set<	pair< _type_info_wrap, _type_info_wrap >, less< pair< _type_info_wrap, _type_info_wrap > >, __L_DEFAULT_ALLOCATOR >
#else //__GNUC__
typename _l_action_object_base< t_TyChar, t_fInLexGen >::_TySetSameTriggers
#endif //__GNUC__
#if 0 //def __GNUC__
_l_action_object_base< t_TyChar, t_fInLexGen >::m_setSameTriggers;
#else //__GNUC__
_l_action_object_base< t_TyChar, t_fInLexGen >::m_setSameTriggers;
//( 
//	typename _l_action_object_base< t_TyChar, t_fInLexGen >::_TyCompareTriggers(),
//	__L_DEFAULT_ALLOCATOR() );
#endif //__GNUC__

// When in the DFA the base class is trivial ( thus generating no virtuals ):
template < class t_TyChar >
struct _l_action_object_base< t_TyChar, false >
{
};

// Some default action objects - useful for debugging and testing:

// Translation into a simple integer token.
template < class t_TyChar, int t_iToken, bool t_fInLexGen = true >
struct _l_action_token
  : public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
  typedef _l_action_token< t_TyChar, t_iToken, t_fInLexGen >	_TyThis;
  typedef _l_action_object_base< t_TyChar, t_fInLexGen >			_TyBase;
public:

  _l_action_token()
  {
  }

  _l_action_token(_TyThis const & _r)
    : _TyBase(_r)
  {
  }

  void Render(ostream & _ros, const char * _pcCharName) const
  {
    return _DoRender(_ros, _pcCharName);
  }
  void Render(stringstream & _ros, const char * _pcCharName) const
  {
    return _DoRender(_ros, _pcCharName);
  }
  void RenderW(wostream & _ros, const wchar_t * _pcCharName) const
  {
    return _DoRenderW(_ros, _pcCharName);
  }
  void RenderW(wstringstream & _ros, const wchar_t * _pcCharName) const
  {
    return _DoRenderW(_ros, _pcCharName);
  }
  template < class t_TyOStream >
  void _DoRender(t_TyOStream & _ros, const char * _pcCharName) const
	{
		_ros << "_l_action_token< " << _pcCharName << ", " << t_iToken << ", false >";
	}
  template < class t_TyEOStream >
  void _DoRenderW(t_TyEOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_action_token< " << _pcCharName << L", " << t_iToken << L", false >";
	}

	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		_rA.SetToken( t_iToken );
		return true;
	}
};

// Print the token seen - useful for debugging.
template < class t_TyChar, int t_iToken, bool t_fInLexGen = true >
struct _l_action_print
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_action_print< t_TyChar, t_iToken, t_fInLexGen >	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen >			_TyBase;
public:

	_l_action_print()
	{
	}

	_l_action_print( _TyThis const & _r )
		: _TyBase( _r )
	{
	}

  void Render(ostream & _ros, const char * _pcCharName) const
  {
    return _DoRender(_ros, _pcCharName);
  }
  void Render(stringstream & _ros, const char * _pcCharName) const
  {
    return _DoRender(_ros, _pcCharName);
  }
  void RenderW(wostream & _ros, const wchar_t * _pcCharName) const
  {
    return _DoRenderW(_ros, _pcCharName);
  }
  void RenderW(wstringstream & _ros, const wchar_t * _pcCharName) const
  {
    return _DoRenderW(_ros, _pcCharName);
  }
  template < class t_tyOStream >
	void _DoRender(t_tyOStream & _ros, const char * _pcCharName) const
	{
		_ros << "_l_action_print< " << _pcCharName << ", " << t_iToken << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_action_print< " << _pcCharName << L", " << t_iToken << L", false >";
	}

	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		cout << "action(): token [" << t_iToken << "].\n";
		return true;
	}
};

__REGEXP_END_NAMESPACE

__BIENUTIL_BEGIN_NAMESPACE 
__REGEXP_USING_NAMESPACE

// Specialize base class mapping because of a bug in Intel compiler:
// Fails to link when attempting to use covariant return.
template < class t_TyChar, int t_iToken, bool m_fInLexGen >
struct __map_to_base_class< _l_action_token< t_TyChar, t_iToken, m_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, m_fInLexGen >	_TyBase;
};

template < class t_TyChar, int t_iToken, bool m_fInLexGen >
struct __map_to_base_class< _l_action_print< t_TyChar, t_iToken, m_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, m_fInLexGen >	_TyBase;
};

__BIENUTIL_END_NAMESPACE

#endif //__L_AXION_H

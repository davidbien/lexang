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

typedef int	vTyActionIdent;
typedef int	vTyTokenIdent;
typedef unsigned long vTyLookaheadVector;

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

	const char * SzTypeName() const
	{
		return typeid( *this ).name();
	}

	// Return the unique token ID associated with this object.
	virtual constexpr vTyTokenIdent GetTokenId() const = 0;

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
#if 0 // This is bogus but might be necessary eventually.
		type_info const & _rtiThis = typeid( *this );
		type_info const & _rtiR = typeid( _r );
		if ( _rtiThis == _rtiR )
		{
			return FDataLessThan( _r );
		}
#endif //0
		// We always sort by token id here as it must be unique.
		return GetTokenId() < _r.GetTokenId();
	}

#if 0
	virtual bool	FDataLessThan( _TyThis const & _r ) const
	{
		// the assumption is that there is no data.
		// derived classes with data should override.
		return false;
	}
#endif //0

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
template < class t_TyChar, vTyTokenIdent t_kiToken, bool t_fInLexGen = true >
struct _l_action_token
  : public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
  typedef _l_action_token< t_TyChar, t_kiToken, t_fInLexGen >	_TyThis;
  typedef _l_action_object_base< t_TyChar, t_fInLexGen >			_TyBase;
public:
	static constexpr vTyTokenIdent s_kiToken = t_kiToken;

  _l_action_token()
  {
  }
  _l_action_token(_TyThis const & _r)
    : _TyBase(_r)
  {
  }
	// Return the unique token ID associated with this object.
	constexpr vTyTokenIdent GetTokenId() const
	{
		return s_kiToken;
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
		_ros << "_l_action_token< " << _pcCharName << ", " << t_kiToken << ", false >";
	}
  template < class t_TyEOStream >
  void _DoRenderW(t_TyEOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_action_token< " << _pcCharName << L", " << t_kiToken << L", false >";
	}

	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		Trace( "Token[%d], Position[%ld].", s_kiToken, _rA.GetCurrentPosition() );
		return true;
	}
};

// Print the token seen - useful for debugging.
template < class t_TyChar, vTyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_action_print
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_action_print	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vTyTokenIdent s_kiTrigger = t_kiTrigger;

	_l_action_print() = default;
	_l_action_print( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	constexpr vTyTokenIdent GetTokenId() const
	{
		return s_kiToken;
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
		_ros << "_l_action_print< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_action_print< " << _pcCharName << L", " << s_kiTrigger << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		return true;
	}
};

// Trigger to record a position in a stream.
template < class t_TyChar, vTyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_position
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_position	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vTyTokenIdent s_kiTrigger = t_kiTrigger;
	_l_trigger_position() = default;
	_l_trigger_position( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	constexpr vTyTokenIdent GetTokenId() const
	{
		return t_kiTrigger;
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
		_ros << "_l_trigger_position< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_trigger_position< " << _pcCharName << L", " << s_kiTrigger << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		_rA.SetGotToken( t_kiTrigger );
		m_stPos = _rA.GetCurrentPosition();
		return true;
	}
protected:
	size_t m_stPos{ numeric_limits< size_t >::max() };
};

// Trigger to record an ending position in a stream.
template < class t_TyChar, vTyTokenIdent t_kiTrigger, vTyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_position_end
	: public _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen >
{
private:
	typedef _l_trigger_position_end	_TyThis;
	typedef _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_kiTrigger;
	static constexpr vTyTokenIdent s_kiTriggerBegin = t_kiTriggerBegin;
	_l_trigger_position_end() = default;
	_l_trigger_position_end( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
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
		_ros << "_l_trigger_position_end< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_trigger_position_end< " << _pcCharName << L", " << s_kiTrigger << ", " << s_kiTriggerBegin << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Assert( _rA.FGotTrigger( s_kiTriggerBegin ) ); // Should have seen this first.
		return _rA.FGotTrigger( s_kiTriggerBegin ) && _TyBase::action( _rA );
	}
};

// This is a triggered simple string that stores the string within it. This is not a resultant token but may be part of a resultant token.
template < class t_TyChar, vTyTokenIdent t_kiTrigger, vTyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_simple_string
	: public _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
private:
	typedef _l_trigger_simple_string	_TyThis;
	typedef _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiTriggerBegin;
	typedef _l_token< t_TyChar > _TyToken;
	typedef _l_trigger_position< t_TyChar, t_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	_l_trigger_simple_string() = default;
	_l_trigger_simple_string( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
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
		_ros << "_l_trigger_simple_string< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_trigger_simple_string< " << _pcCharName << L", " << s_kiTrigger << ", " << s_kiTriggerBegin << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.GetTrigger< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTriggerEnd ) ); // We just got it!
			vtyTokenPosition posEnd = rtEnd.GetClearPosition();
			m_tkString.SetBeginEnd( _rA.GetStream(), posBegin, posEnd );
		}
		return true;
	}
protected:
	_TyToken m_tkString;
};

// This is a triggered list of string that stores the string list within it. This is not a resultant token but may be part of a resultant token.
template < class t_TyChar, vTyTokenIdent t_kiTrigger, vTyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_string_list
	: public _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
private:
	typedef _l_trigger_string_list	_TyThis;
	typedef _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiTriggerBegin;
	typedef _l_token< t_TyChar > _TyToken;
	typedef _l_trigger_position< t_TyChar, t_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	static constexpr size_t stSizeSegArrayBlock = 1024;
	_l_trigger_string_list()
		: m_rgsTokens( stSizeSegArrayBlock/sizeof(_TyToken ) )
	{
	}
	_l_trigger_string_list( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
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
		_ros << "_l_trigger_string_list< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_trigger_string_list< " << _pcCharName << L", " << s_kiTrigger << ", " << s_kiTriggerBegin << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.GetTrigger< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTriggerEnd ) ); // We just got it!
			vtyTokenPosition posEnd = rtEnd.GetClearPosition();
			Assert( ( vtpNullTokenPosition != posBegin ) && ( vtpNullTokenPosition != posEnd ) && ( posEnd >= posBegin ) ); // This may fire and then we should investigate why and/or perhaps comment it out.
			if ( ( vtpNullTokenPosition != posBegin ) && ( vtpNullTokenPosition != posEnd ) && ( posEnd >= posBegin ) )
				m_rgsTokens.emplaceAtEnd( _rA.GetStream(), posBegin, posEnd ); // Add to the "list".
		}
		return true;
	}
protected:
	typedef SegArray< _TyToken, true > _TySegArrayTokens; // Avoid reallocation.
	_TySegArrayTokens m_rgsTokens;
};

// _l_token_simple_string:
// This is a token that is a simple, untranslated, string with a beginning position and an end position.
template < class t_tyTriggerBegin, class t_tyTriggerEnd, vTyTokenIdent s_kiToken, bool t_fInLexGen >
class _l_token_simple_string : public _l_action_object_base< t_TyChar, t_fInLexGen >
{
public:
	typedef typename t_tyTriggerBegin::_TyChar _TyChar;
private:
	typedef _l_token_simple_string	_TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen >			_TyBase;
public:
	static constexpr vTyTokenIdent s_kiToken = t_kiToken;
	static constexpr vTyTokenIdent s_kiTriggerBegin = t_tyTriggerBegin::t_kiToken;
	static constexpr vTyTokenIdent s_kiTriggerEnd = t_tyTriggerEnd::t_kiToken;
	typedef _l_token< t_TyChar > _TyToken;

	_l_token_simple_string() = default;
	_l_token_simple_string( _l_token_simple_string const & ) = default;
	// Return the unique token ID associated with this object.
	constexpr vTyTokenIdent GetTokenId() const
	{
		return s_kiToken;
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
		_ros << "_l_token_simple_string< ";
		{ //B
			t_tyTriggerBegin tBegin;
			tBegin._DoRender( _ros, _pcCharName );
			_ros << ", ";
		} //EB
		{ //B
			t_tyTriggerEnd tEnd;
			tEnd._DoRender( _ros, _pcCharName );
			_ros << ", ";
		} //EB
		_ros << t_kiToken << ", false >";
	}
  template < class t_tyWOStream >
  void _DoRenderW(t_tyWOStream & _ros, const wchar_t * _pcCharName) const
	{
		_ros << L"_l_token_simple_string< ";
		{ //B
			t_tyTriggerBegin tBegin;
			tBegin._DoRenderW( _ros, _pcCharName );
			_ros << L", ";
		} //EB
		{ //B
			t_tyTriggerEnd tEnd;
			tEnd._DoRenderW( _ros, _pcCharName );
			_ros << L", ";
		} //EB
		_ros << t_kiToken << L", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Assert( m_tkString.FIsNull() );
		// If we got the triggers we expected to see then populate the internal token from those
		//	triggers and clear the triggers' values.
		if ( _rA.FGotTrigger( s_kiTriggerBegin ) )
		{
			t_tyTriggerBegin & rtBegin = static_cast< t_tyTriggerBegin & >( _rA.GetTrigger< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTriggerEnd ) ); // How would we have otherwise completed the production?
			if ( _rA.FGotTrigger( s_kiTriggerEnd ) )
			{
				t_tyTriggerEnd & rtEnd = static_cast< t_tyTriggerEnd & >( _rA.GetTrigger< s_kiTriggerEnd >() );
				vtyTokenPosition posEnd = rtEnd.GetClearPosition();
				m_tkString.SetBeginEnd( _rA.GetStream(), posBegin, posEnd );
			}
		}
		else
		{
			Assert( !_rA.FGotTrigger( s_kiTriggerEnd ) ); // Else how could we have got the begin trigger?
		}
		return true;
	}
	// Get a copy of the contained token.
	void CopyToken( _TyToken & _rtk ) const
	{
		_rtk = m_tkString; // make a copy.
	}
	// Transfer the contained token to the caller.
	// This is the preferred manner as it clears the token.
	void TransferToken( _TyToken & _rtk )
	{
		_rtk = std::move( *this );
	}
protected:
	_TyToken m_tkString;
};

__REGEXP_END_NAMESPACE

__BIENUTIL_BEGIN_NAMESPACE 
__REGEXP_USING_NAMESPACE

// Specialize base class mapping because of a bug in Intel compiler:
// Fails to link when attempting to use covariant return.
template < class t_TyChar, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_token< t_TyChar, t_kiToken, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen >	_TyBase;
};

template < class t_TyChar, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_print< t_TyChar, t_kiToken, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen >	_TyBase;
};

template < class t_TyChar, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_print< t_TyChar, t_kiToken, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen >	_TyBase;
};

__BIENUTIL_END_NAMESPACE

#endif //__L_AXION_H

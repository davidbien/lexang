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
#include <tuple>
#include "_aloctrt.h"

__REGEXP_BEGIN_NAMESPACE

typedef int	vtyActionIdent;
typedef int	vtyTokenIdent;
typedef size_t vtyDataType;
typedef size_t vtyTokenPosition;
static constexpr vtyTokenPosition vtpNullTokenPosition = numeric_limits< vtyTokenPosition >::max();
typedef unsigned long vtyLookaheadVector;

// _l_action_object_base:
// This base class is only used when we are in the gneration stage.
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

	// I think that this returns the mangled name - making it less than useful. If it returned the non-mangled
	//	name it could be used for code generation.
	const char * SzTypeName() const
	{
		return typeid( *this ).name();
	}

	// Return the unique token ID associated with this object.
	// This is the virtual call. The non-virtual call is defined at most-derived class level.
	virtual constexpr vtyTokenIdent VGetTokenId() const = 0;

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
		return VGetTokenId() < _r.VGetTokenId();
	}

#if 0
	virtual bool	FDataLessThan( _TyThis const & _r ) const
	{
		// the assumption is that there is no data.
		// derived classes with data should override.
		return false;
	}
#endif //0

	virtual void RenderActionType(ostream & _ros, const char * _pcCharName) const = 0;
  virtual void RenderActionType(stringstream & _ros, const char * _pcCharName) const = 0;
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
template < class t_TyChar, vtyTokenIdent t_kiToken, bool t_fInLexGen = true >
struct _l_action_token
  : public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
  typedef _l_action_token _TyThis;
  typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vtyTokenIdent s_kiToken = t_kiToken;
  _l_action_token()
  {
  }
  _l_action_token(_TyThis const & _r)
    : _TyBase(_r)
  {
  }
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return s_kiToken;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return s_kiToken;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_action_token< " << _pcCharName << ", " << t_kiToken << ", false >";
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
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_action_print
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_action_print	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	_l_action_print() = default;
	_l_action_print( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return t_kiTrigger;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return t_kiTrigger;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_action_print< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		return true;
	}
};

// _l_trigger_bool:
// Trigger that stores a boolean. If the trigger fires then the boolean is true.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_bool
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_bool	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;

	_l_trigger_bool() = default;
	_l_trigger_bool( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return !m_f;
	}
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return s_kiTrigger;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return s_kiTrigger;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_trigger_bool< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool	action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		_rA.SetGotTrigger( s_kiTrigger ); // The only thing we do is record that we got the trigger.
		m_f = true;
		return true;
	}
	void swap( _TyThis & _r )
	{
		std::swap( m_f, _r.m_f );
	}
protected:
	bool m_f{false};
};

// Trigger to record a position in a stream.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_position
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_position	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	_l_trigger_position() = default;
	_l_trigger_position( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return ( m_tpPos == vtpNullTokenPosition );
	}
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return t_kiTrigger;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return t_kiTrigger;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_trigger_position< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		_rA.SetGotTrigger( t_kiTrigger );
		m_tpPos = _rA.GetCurrentPosition();
		return true;
	}
	// "consume" a token position. This should keep all action objects clear after usage with no additional work.
	vtyTokenPosition GetClearPosition()
	{
		vtyTokenPosition tpPos = m_tpPos;
		m_tpPos = vtpNullTokenPosition;
		return tpPos;
	}
	// The parser might want to just save a single position in the stream so implement this.
	void swap( _TyThis & _r )
	{
		std::swap( m_tpPos, _r.m_tpPos );
	}
protected:
	vtyTokenPosition m_tpPos{ vtpNullTokenPosition };
};

// Trigger to record an ending position in a stream.
// This is only used as a base class - and really it could be gotten rid of.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_position_end
	: public _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen >
{
private:
	typedef _l_trigger_position_end	_TyThis;
	typedef _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_kiTrigger;
	static constexpr vtyTokenIdent s_kiTriggerBegin = t_kiTriggerBegin;
	_l_trigger_position_end() = default;
	_l_trigger_position_end( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::FIsNull;
	using _TyBase::GetTokenId();
	using _TyBase::VGetTokenId();
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_trigger_position_end< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Assert( _rA.FGotTrigger( s_kiTriggerBegin ) ); // Should have seen this first.
		return _rA.FGotTrigger( s_kiTriggerBegin ) && _TyBase::action( _rA );
	}
	using _TyBase::GetClearPosition;
	// Unlikely this ever gets called but we imeplement it.
	void swap( _TyThis & _r )
	{
			_TyBase::swap( _r );
	}
};

// _l_trigger_strings:
// This is a triggered simple set of strings that stores the strings within it. 
// This is not a resultant token but may be part of a resultant token.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_strings
	: public _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
private:
	typedef _l_trigger_strings	_TyThis;
	typedef _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiTriggerBegin;
	typedef _l_token< t_TyChar > _TyToken;
	typedef _l_trigger_position< t_TyChar, t_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	_l_trigger_strings() = default;
	_l_trigger_strings( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return _TyBase::FIsNull() && m_tkStrings.FIsNull();
	}
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
	using _TyBase::VGetTokenId();
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_trigger_strings< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.GetActionObj< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTrigger ) ); // We just got it!
			vtyTokenPosition posEnd = GetClearPosition();
			Assert(	( vtpNullTokenPosition != posBegin ) &&
						( vtpNullTokenPosition != posEnd ) &&
						( posEnd > posBegin ) );
			if (	( vtpNullTokenPosition != posBegin ) &&
						( vtpNullTokenPosition != posEnd ) &&
						( posEnd > posBegin ) )
			{
				m_tkStrings.Append( posBegin, posEnd );
			}
		}
		return true;
	}
	template < class t_TyAnalyzer >
	void Append( t_TyAnalyzer & _rA, vtyTokenPosition _posBegin, vtyTokenPosition _posEnd, vtyDataType _nType = 0 )
	{
		m_tkStrings.Append( posBegin, posEnd, _nType );
	}
	void swap( _TyThis & _r )
	{
		// We swap the base because it's part of this object but probably the caller won't use the value - also probably the value is null.
		// Also the data within it is redundant as it would be contained with m_skStrings.
		_TyBase::swap( _r );
		m_tkStrings.swap( _r.m_tkStrings );
	}
protected:
	_TyToken m_tkStrings;
};

// _l_trigger_string_typed_range:
// This will store a range of input data into t_TyActionStoreData identified by the t_kdtType "type" of data.
// The beginning position of the data is in t_kiTriggerBegin.
// The ending position of the data is contained in this object.
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
class _l_trigger_string_typed_range
	: public _l_trigger_position_end< typename t_TyActionStoreData::_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
public:
	typedef typename t_TyActionStoreData::_TyChar _TyChar;
private:
	typedef _l_trigger_string_typed_range _TyThis;
	typedef _l_trigger_position_end< _TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
	static constexpr vtyDataType s_kdtType = t_kdtType;
	using _TyBase::FIsNull;
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiTriggerBegin;
	typedef _l_token< t_TyChar > _TyToken;
	typedef _l_trigger_position< _TyChar, s_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	typedef t_TyActionStoreData _tyActionStoreData;
	static constexpr vtyTokenIdent s_kiActionStoreData = _tyActionStoreData::GetTokenId();
	_l_trigger_string_typed_range() = default;
	_l_trigger_string_typed_range( _TyThis const & _r ) = default;
	using _TyBase::FIsNull;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
	using _TyBase::VGetTokenId();
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_ros << "_l_trigger_string_typed_range< " << s_kdtType << ", ";
		_tyActionStoreData::StaticRenderActionType( _ros, _pcCharName );
		_ros << ", " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.GetActionObj< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTrigger ) ); // We just got it!
			vtyTokenPosition posEnd = GetClearPosition();
			Assert(	( vtpNullTokenPosition != posBegin ) &&
						( vtpNullTokenPosition != posEnd ) &&
						( posEnd > posBegin ) );
			if (	( vtpNullTokenPosition != posBegin ) &&
						( vtpNullTokenPosition != posEnd ) &&
						( posEnd > posBegin ) )
			{
				_tyActionStoreData & raxnStoreData = static_cast< _tyActionStoreData & >( _rA.GetActionObj< s_kiActionStoreData >() );
				raxnStoreData.Append( _rA, posBegin, posEnd, s_kdtType );
			}
		}
		return true;
	}
	// There is no data in this object - only a position.
	void swap( _TyThis & _r )
	{
		_TyBase::swap( _r );
	}
};

#if 0 // This is redundant code - just use _l_trigger_strings.
// This is a triggered list of string that stores the string list within it. This is not a resultant token but may be part of a resultant token.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
class _l_trigger_string_list
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
	_l_trigger_string_list() = default;
	_l_trigger_string_list( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId();
	using _TyBase::VGetTokenId();
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName) const
	{
		_ros << "_l_trigger_string_list< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.GetActionObj< s_kiTriggerBegin >() );
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
	// Unless a single string might be composed of multiple pieces, we can just use a single token here.
	_TyToken m_tkStrings;
};
#endif //0 - unused

// _l_action_save_data_single:
// This action object can be used as a trigger object or a token object. 
// It saves data by copying it from it's constituent trigger objects.
// It only contains the ability to hold a single vector of constituent trigger data.
// These trigger objects may be aggregate contained _l_action_save_data_single objects.
template < vtyTokenIdent t_kiTriggerOrToken, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_single : public _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>::type::_TyChar, t_fInLexGen >
{
public:
	typedef typename tuple_element<0,std::tuple< t_TysTriggers...>::type::_TyChar _TyChar;
private:
	typedef _l_action_save_data_single _TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen > _TyBase;
public:
	typedef tuple< t_TysTriggers...> _TyTuple;
	static constexpr vtyTokenIdent s_kiTriggerOrToken = t_kiTriggerOrToken;
	_l_trigger_position() = default;
	_l_trigger_position( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return std::apply
    (
        []( t_TysTriggers const &... _tuple )
        {
					return ( ... && _tuple.FIsNull() );
        }, m_tuple
    );
	}
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return t_kiTriggerOrToken;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return t_kiTriggerOrToken;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		// Just used for rendering the types. We'd call a static method on each type in the tuple but I haven't figured out how to do that yet.
		_TyTuple tupleLocal; 
		_ros << "_l_action_save_data_single< " << s_kiTriggerOrToken << ", " << ", false, ";
		std::apply
    (
        [ &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcCharName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
		_ros << " >";
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		// We copy data from all constituent triggers regardless if that trigger fired. We may change this later.
		Assert( FIsNull() ); // Should be left in a null state because should have been "eaten" by the parser, or by another token, etc.
		std::apply
    (
        []( t_TysTriggers &... _tuple )
        {
					return ( ..., SwapWithTrigger(_tuple) );
        }, m_tuple
    );
		return true;
	}
	template < class t_TyAnalyzer, class t_TyTrigger >
	static void SwapWithTrigger( t_TyAnalyzer & _rA, t_TyTrigger & _rtThis )
	{
		constexpr vtyTokenIdent kidCur = t_TyTrigger::GetTokenId();
		t_TyTrigger & rtThat = static_cast< t_TyTrigger & >( _rA.GetActionObj< kidCur >() );
		_rtThis.swap( rtThat );
	}
	void swap( _TyThis & _r )
	{
		m_tuple.swap( _r.m_tuple );
	}
protected:
	_TyTuple m_tuple; // ya got yer tuple.
};

// _l_action_save_data_multiple:
// This action object can be used as a trigger object or a token object.
// It saves data by copying it from it's constituent trigger objects.
// It only contains the ability to hold a single vector of constituent trigger data.
// These trigger objects may be aggregate contained _l_action_save_data_multiple objects.
template < vtyTokenIdent t_kiTriggerOrToken, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_multiple : public _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>::type::_TyChar, t_fInLexGen >
{
public:
	typedef typename tuple_element<0,std::tuple< t_TysTriggers...>::type::_TyChar _TyChar;
private:
	typedef _l_action_save_data_multiple _TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen > _TyBase;
public:
	typedef tuple< t_TysTriggers...> _TyTuple;
	static constexpr vtyTokenIdent s_kiTriggerOrToken = t_kiTriggerOrToken;
	_l_trigger_position() = default;
	_l_trigger_position( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return !m_saTuples.NElements();
	}
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return t_kiTriggerOrToken;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return t_kiTriggerOrToken;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName)
	{
		_TyTuple tupleLocal; 
		_ros << "_l_action_save_data_multiple< " << s_kiTriggerOrToken << ", " << ", false, ";
		std::apply
    (
        [ &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcCharName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
		_ros << " >";
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		// We copy data from all constituent triggers regardless if that trigger fired. We may change this later.
		// We add data to the SegArray when this trigger/token fires.
		// Get a new element at the end of the segarray:
		_TyTuple & rtupleEnd = m_saTuples.emplaceAtEnd(); // This could throw but we don't expect swapping below to throw since no allocation is performed.
		Assert( rtupleEnd.FIsNull() );
		std::apply
    (
        []( t_TysTriggers &... _tuple )
        {
					return ( ..., SwapWithTrigger(_tuple) );
        }, rtupleEnd
    );
		return true;
	}
	template < class t_TyAnalyzer, class t_TyTrigger >
	static void SwapWithTrigger( t_TyAnalyzer & _rA, t_TyTrigger & _rtThis )
	{
		constexpr vtyTokenIdent kidCur = t_TyTrigger::GetTokenId();
		t_TyTrigger & rtThat = static_cast< t_TyTrigger & >( _rA.GetActionObj< kidCur >() );
		_rtThis.swap( rtThat );
	}
	void swap( _TyThis & _r )
	{
		m_saTuples.swap( _r.m_saTuples );
		Assert( FIsNull() ); // We generally expect an empty SegArray to be swapped in. This assertion can be removed if there is a usage case for swapping in a non-empty.
	}
protected:
	// We save multiple sets of tuple values in a SegArray.
	typedef SegArray< _TyTuple, true > _TySegArrayTuples;
	_TySegArrayTuples m_saTuples; // ya got yer tuples.
};

// _l_token_simple_string:
// This is a token that is a simple, untranslated, string with a beginning position and an end position.
template < class t_TyTriggerBegin, class t_TyTriggerEnd, vtyTokenIdent s_kiToken, bool t_fInLexGen >
class _l_token_simple_string : public _l_action_object_base< t_TyChar, t_fInLexGen >
{
public:
	typedef typename t_TyTriggerBegin::_TyChar _TyChar;
private:
	typedef _l_token_simple_string	_TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen >			_TyBase;
public:
	static constexpr vtyTokenIdent s_kiToken = t_kiToken;
	static constexpr vtyTokenIdent s_kiTriggerBegin = t_TyTriggerBegin::t_kiToken;
	static constexpr vtyTokenIdent s_kiTriggerEnd = t_TyTriggerEnd::t_kiToken;
	typedef _l_token< t_TyChar > _TyToken;

	_l_token_simple_string() = default;
	_l_token_simple_string( _l_token_simple_string const & ) = default;
	// Return the unique token ID associated with this object.
	static constexpr vtyTokenIdent GetTokenId()
	{
		return s_kiToken;
	}
	constexpr vtyTokenIdent VGetTokenId() const
	{
		return s_kiToken;
	}
  void RenderActionType(ostream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcCharName) const
  {
    return StaticRenderActionType(_ros, _pcCharName);
  }
  template < class t_TyOStream >
	void StaticRenderActionType(t_TyOStream & _ros, const char * _pcCharName) const
	{
		_ros << "_l_token_simple_string< ";
		{ //B
			t_TyTriggerBegin tBegin;
			tBegin.RenderActionType( _ros, _pcCharName );
			_ros << ", ";
		} //EB
		{ //B
			t_TyTriggerEnd tEnd;
			tEnd.RenderActionType( _ros, _pcCharName );
			_ros << ", ";
		} //EB
		_ros << t_kiToken << ", false >";
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Assert( m_tkStrings.FIsNull() );
		// If we got the triggers we expected to see then populate the internal token from those
		//	triggers and clear the triggers' values.
		if ( _rA.FGotTrigger( s_kiTriggerBegin ) )
		{
			t_TyTriggerBegin & rtBegin = static_cast< t_TyTriggerBegin & >( _rA.GetActionObj< s_kiTriggerBegin >() );
			vtyTokenPosition posBegin = rtBegin.GetClearPosition();
			Assert( _rA.FGotTrigger( s_kiTriggerEnd ) ); // How would we have otherwise completed the production?
			if ( _rA.FGotTrigger( s_kiTriggerEnd ) )
			{
				t_TyTriggerEnd & rtEnd = static_cast< t_TyTriggerEnd & >( _rA.GetActionObj< s_kiTriggerEnd >() );
				vtyTokenPosition posEnd = rtEnd.GetClearPosition();
				m_tkStrings.SetBeginEnd( _rA.GetStream(), posBegin, posEnd );
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
		_rtk = m_tkStrings; // make a copy.
	}
	// Transfer the contained token to the caller.
	// This is the preferred manner as it clears the token.
	void TransferToken( _TyToken & _rtk )
	{
		_rtk = std::move( *this );
	}
protected:
	_TyToken m_tkStrings;
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

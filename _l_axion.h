#pragma once

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
#include "_l_types.h"
#include "_l_data.h"

__REGEXP_BEGIN_NAMESPACE

// _l_action_object_base:
// This base class is only used when we are in the gneration stage.
template < class t_TyChar, bool t_fInLexGen >
struct _l_action_object_base
{
private:
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyThis;
public:
	static const bool s_fInLexGen = t_fInLexGen;
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

	virtual string VStrTypeName( const char * _pcCharName ) const = 0;

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

// _l_action_object_base< t_TyChar, false >:
// The action base for when we are inside the operating lexicographical analyzer.
// I was keeping this free of virtuals but I decided that some nice virtuals would
//	help the impl. These are for the end-user of the lexang.
template < class t_TyChar >
struct _l_action_object_base< t_TyChar, false >
{
	typedef _l_action_object_base _TyThis;
public:
	static const bool s_fInLexGen = false;
	typedef _l_value< t_TyChar > _TyValue;

	// Return the ID for this action object.
	virtual constexpr vtyTokenIdent VGetTokenId() const = 0;

	// Clear any data that is residing within this object.
	virtual void Clear() = 0;

	// Get the set of data (the "value") from the object in a generic form. Leave the object empty of data.
	virtual void GetAndClearValue( _TyValue & _rv ) = 0;
};

// Some default action objects - useful for debugging and testing:

// _l_action_token:
// This wrapper goes around a token action.
// It does some standard boilerplate stuff that can't be done in action objects that are shared between triggers and tokens (for instance).
// Note that each token must still have a unique token id as this merely pulls the token id from it's sub-object.
template < class t_TyActionObj >
struct _l_action_token
  : public t_TyActionObj
{
private:
  typedef _l_action_token _TyThis;
  typedef t_TyActionObj _TyBase;
public:
	typedef true_type _TyFIsToken; // Must wrap each action with something defining this as true_type.
	using _TyBase::s_kiToken;
	using _TyBase::s_fInLexGen;	
  _l_action_token()
  {
  }
  _l_action_token(_TyThis const & _r)
    : _TyBase(_r)
  {
  }
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId;
	using _TyBase::VGetTokenId;
	using _TyBase::Clear;
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
		string str = StaticStrTypeName( _pcCharName );
		_ros << str;
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string strBase = _TyBase::StaticStrTypeName( _pcCharName );
		string str;
		PrintfStdStr( str, "_l_action_token< %s >", strBase.c_str() );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		bool f = _TyBase::action( _rA );
		if ( f )
		{
			_rA.SetToken( this ); // Record that we got this token with the analyzer.
		}
		return f;
	}
};

// Print the token seen - useful for debugging.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_action_print
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_action_print	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	using _TyBase::s_fInLexGen;	
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
	void Clear()
	{
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
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_action_print< %s, %u, false >", _pcCharName, s_kiTrigger );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		return true;
	}
};

// _l_trigger_noop:
// This trigger is used by tokens that don't store any data.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_noop
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_noop	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	typedef t_TyChar _TyChar;
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
	using _TyBase::s_fInLexGen;	

	_l_trigger_noop() = default;
	_l_trigger_noop( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return true;
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
	void Clear()
	{
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
		_ros << "_l_trigger_noop< " << _pcCharName << ", " << s_kiTrigger << ", false >";
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_trigger_noop< %s, %u, false >", _pcCharName, s_kiTrigger );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		return true;
	}
	void swap( _TyThis & _r )
	{
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		_rv.Clear(); // No values here.
	}
protected:
};

// _l_trigger_bool:
// Trigger that stores a boolean. If the trigger fires then the boolean is true.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_bool
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_bool	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	typedef t_TyChar _TyChar;
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
	using _TyBase::s_fInLexGen;	

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
	void Clear()
	{
		m_f = false;
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
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_trigger_bool< %s, %u, false >", _pcCharName, s_kiTrigger );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		m_f = true;
		return true;
	}
	void swap( _TyThis & _r )
	{
		std::swap( m_f, _r.m_f );
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		_rv.SetValue( m_f );
		Clear();
	}
protected:
	bool m_f{false};
};

// Trigger to record a position in a stream.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_position
	: public _l_action_object_base< t_TyChar, t_fInLexGen >
{
private:
	typedef _l_trigger_position	_TyThis;
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
public:
	typedef t_TyChar _TyChar;
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
	using _TyBase::s_fInLexGen;	
	_l_trigger_position() = default;
	_l_trigger_position( _TyThis const & _r ) = default;
	bool FIsNull() const
	{
		return ( m_tpPos == vkdpNullDataPosition );
	}
	void Clear()
	{
		m_tpPos = vkdpNullDataPosition;
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
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_trigger_position< %s, %u, false >", _pcCharName, s_kiTrigger );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		m_tpPos = _rA.GetCurrentPosition();
		return true;
	}
	// "consume" a token position. This should keep all action objects clear after usage with no additional work.
	vtyDataPosition GetClearPosition()
	{
		vtyDataPosition tpPos = m_tpPos;
		m_tpPos = vkdpNullDataPosition;
		return tpPos;
	}
	// The parser might want to just save a single position in the stream so implement this.
	void swap( _TyThis & _r )
	{
		std::swap( m_tpPos, _r.m_tpPos );
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		_rv.SetValue( m_tpPos );
		Clear();
	}
protected:
	vtyDataPosition m_tpPos{ vkdpNullDataPosition };
};

// Trigger to record an ending position in a stream.
// This is only used as a base class - and really it could be gotten rid of.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct _l_trigger_position_end
	: public _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen >
{
private:
	typedef _l_trigger_position_end	_TyThis;
	typedef _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen > _TyBase;
public:
	typedef t_TyChar _TyChar;
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiToken;
	using _TyBase::s_fInLexGen;	
	static constexpr vtyTokenIdent s_kiTriggerBegin = t_kiTriggerBegin;
	_l_trigger_position_end() = default;
	_l_trigger_position_end( _TyThis const & _r ) = default;
	// Return the unique token ID associated with this object.
	using _TyBase::Clear;
	using _TyBase::FIsNull;
	using _TyBase::GetTokenId;
	using _TyBase::VGetTokenId;
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
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_trigger_position< %s, %u, %u, false >", _pcCharName, s_kiTrigger, s_kiTriggerBegin );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		return _TyBase::action( _rA );
	}
	using _TyBase::GetClearPosition;
	// Unlikely this ever gets called but we imeplement it.
	void swap( _TyThis & _r )
	{
			_TyBase::swap( _r );
	}
	using _TyBase::GetAndClearValue;
};

// _l_trigger_string:
// This is a triggered simple set of strings that stores the strings within it.
// This is not a resultant token but may be part of a resultant token.
// Conceptually this is a single string - i.e. it is translated (or may be and is intended to be) as a single string that has multiple segments with potentially different m_nType values.
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct _l_trigger_string
	: public _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
private:
	typedef _l_trigger_string	_TyThis;
	typedef _l_trigger_position_end< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
	typedef t_TyChar _TyChar;
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiToken;
	using _TyBase::s_kiTriggerBegin;
	using _TyBase::s_fInLexGen;	
	typedef _l_data< t_TyChar > _TyData;
	typedef _l_trigger_position< t_TyChar, t_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	_l_trigger_string() = default;
	_l_trigger_string( _TyThis const & _r ) = default;
	void Clear()
	{
		_TyBase::Clear();
		m_dtStrings.Clear();
	}
	bool FIsNull() const
	{
		return _TyBase::FIsNull() && m_dtStrings.FIsNull();
	}
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId;
	using _TyBase::VGetTokenId;
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
		_ros << "_l_trigger_string< " << _pcCharName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string str;
		PrintfStdStr( str, "_l_trigger_string< %s, %u, %u, false >", _pcCharName, s_kiTrigger, s_kiTriggerBegin );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.template GetActionObj< s_kiTriggerBegin >() );
			vtyDataPosition posBegin = rtBegin.GetClearPosition();
			vtyDataPosition posEnd = GetClearPosition();
			Assert(	( vkdpNullDataPosition != posBegin ) &&
						( vkdpNullDataPosition != posEnd ) &&
						( posEnd > posBegin ) );
			if (	( vkdpNullDataPosition != posBegin ) &&
						( vkdpNullDataPosition != posEnd ) &&
						( posEnd > posBegin ) )
			{
				m_dtStrings.Append( posBegin, posEnd, 0, s_kiTrigger );
			}
		}
		return true;
	}
	// Other triggers may call this method with a different trigger id than this one.
	template < class t_TyAnalyzer >
	void Append( t_TyAnalyzer & _rA, vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType, vtyDataTriggerId _nIdTrigger )
	{
		m_dtStrings.Append( _posBegin, _posEnd, _nType, _nIdTrigger );
	}
	void swap( _TyThis & _r )
	{
		// We swap the base because it's part of this object but probably the caller won't use the value - also probably the value is null.
		// Also the data within it is redundant as it would be contained with m_skStrings.
		_TyBase::swap( _r );
		m_dtStrings.swap( _r.m_dtStrings );
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		_rv.SetValue( std::move(m_dtStrings) );
		Assert( m_dtString.FIsNull() );
	}
protected:
	using _TyBase::GetClearPosition;
	_TyData m_dtStrings;
};

// _l_trigger_string_typed_range:
// This will store a range of input data into t_TyActionStoreData identified by the t_kdtType "type" of data.
// The beginning position of the data is in t_kiTriggerBegin.
// The ending position of the data is contained in this object.
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
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
	using _TyBase::s_kiTrigger;
	using _TyBase::s_kiToken;
	using _TyBase::s_kiTriggerBegin;
	using _TyBase::s_fInLexGen;	
	typedef _l_trigger_position< _TyChar, s_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
	typedef t_TyActionStoreData _tyActionStoreData;
	static constexpr vtyTokenIdent s_kiActionStoreData = _tyActionStoreData::GetTokenId();
	_l_trigger_string_typed_range() = default;
	_l_trigger_string_typed_range( _TyThis const & _r ) = default;
	using _TyBase::Clear;
	using _TyBase::FIsNull;
	// Return the unique token ID associated with this object.
	using _TyBase::GetTokenId;
	using _TyBase::VGetTokenId;
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
		_ros << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		string strActionStoreData( _tyActionStoreData::StaticStrTypeName( _pcCharName ) );
		string str;
		PrintfStdStr( str, "_l_trigger_string< %u, %s, %u, %u, false >", s_kdtType, strActionStoreData.c_str(), s_kiTrigger, s_kiTriggerBegin );
		return str;
	}
	// We pass the action object the most derived analyzer.
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		bool fRet = _TyBase::action( _rA );
		if ( fRet )
		{
			_TyTriggerBegin & rtBegin = static_cast< _TyTriggerBegin & >( _rA.template GetActionObj< s_kiTriggerBegin >() );
			vtyDataPosition posBegin = rtBegin.GetClearPosition();
			vtyDataPosition posEnd = GetClearPosition();
			Assert(	( vkdpNullDataPosition != posBegin ) &&
						( vkdpNullDataPosition != posEnd ) &&
						( posEnd > posBegin ) );
			if (	( vkdpNullDataPosition != posBegin ) &&
						( vkdpNullDataPosition != posEnd ) &&
						( posEnd > posBegin ) )
			{
				_tyActionStoreData & raxnStoreData = static_cast< _tyActionStoreData & >( _rA.template GetActionObj< s_kiActionStoreData >() );
				raxnStoreData.Append( _rA, posBegin, posEnd, s_kdtType, s_kiTrigger );
			}
		}
		return true;
	}
	// There is no data in this object - only a position.
	void swap( _TyThis & _r )
	{
		_TyBase::swap( _r );
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		// We shouldn't get here but if so call the base:
		AssertSz( false, "No reason to use this class within a token since it stores its value in another trigger/token.");
		_TyBase::GetAndClearValue( _rv );
	}
protected:
	using _TyBase::GetClearPosition;
};

// _l_action_save_data_single:
// This action object can be used as a trigger object or a token object. 
// It saves data by copying it from it's constituent trigger objects.
// It only contains the ability to hold a single vector of constituent trigger data.
// These trigger objects may be aggregate contained _l_action_save_data_single objects.
template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_single : public _l_action_object_base< typename tuple_element<0, tuple< t_TysTriggers...>>::type::_TyChar, t_fInLexGen >
{
public:
	typedef typename tuple_element<0, tuple< t_TysTriggers...>>::type::_TyChar _TyChar;
private:
	typedef _l_action_save_data_single _TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen > _TyBase;
public:
	typedef tuple< t_TysTriggers...> _TyTuple;
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
	using _TyBase::s_fInLexGen;	
	_l_action_save_data_single() = default;
	_l_action_save_data_single( _TyThis const & _r ) = default;
	void Clear()
	{
		std::apply
    (
        []( t_TysTriggers &... _tuple )
        {
					( _tuple.Clear(), ... );
        }, m_tuple
    );
	}
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
		// Just used for rendering the types. We'd call a static method on each type in the tuple but I haven't figured out how to do that yet.
		_TyTuple tupleLocal; 
		_ros << "_l_action_save_data_single< " << s_kiTrigger << ", false, ";
		std::apply
    (
        [ _pcCharName, &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcCharName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
		_ros << " >";
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		stringstream ss;
		StaticRenderActionType( ss, _pcCharName );
		return ss.str();
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		// We copy data from all constituent triggers regardless if that trigger fired. We may change this later.
		Assert( FIsNull() ); // Should be left in a null state because should have been "eaten" by the parser, or by another token, etc.
		std::apply
    (
        [&_rA]( t_TysTriggers &... _tuple )
        {
					return ( ..., SwapWithTrigger(_rA,_tuple) );
        }, m_tuple
    );
		return true;
	}
	template < class t_TyAnalyzer, class t_TyTrigger >
	static void SwapWithTrigger( t_TyAnalyzer & _rA, t_TyTrigger & _rtThis )
	{
		constexpr vtyTokenIdent kidCur = t_TyTrigger::GetTokenId();
		t_TyTrigger & rtThat = static_cast< t_TyTrigger & >( _rA.template GetActionObj< kidCur >() );
		rtThat.swap( _rtThis );
	}
	void swap( _TyThis & _r )
	{
		m_tuple.swap( _r.m_tuple );
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		_rv.SetSize( sizeof...(t_TysTriggers) );
		std::apply
    (
        [&_rv]( t_TysTriggers &... _tuple )
        {
					size_t nCurEl = 0;
					( ..., _tuple.GetAndClearValue( _rv[nCurEl++] ) );
        }, m_tuple
    );
	}
protected:
	_TyTuple m_tuple; // ya got yer tuple.
};

// _l_action_save_data_multiple:
// This action object can be used as a trigger object or a token object.
// It saves data by copying it from it's constituent trigger objects.
// It only contains the ability to hold a single vector of constituent trigger data.
// These trigger objects may be aggregate contained _l_action_save_data_multiple objects.
template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_multiple : public _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyChar, t_fInLexGen >
{
public:
	typedef typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyChar _TyChar;
private:
	typedef _l_action_save_data_multiple _TyThis;
	typedef _l_action_object_base< _TyChar, t_fInLexGen > _TyBase;
public:
	typedef tuple< t_TysTriggers...> _TyTuple;
	static const size_t s_kstInitSegArray = 32 * sizeof(_TyTuple);
	static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
	static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
	using _TyBase::s_fInLexGen;	
	_l_action_save_data_multiple()
		: m_saTuples( s_kstInitSegArray )
	{
	}
	_l_action_save_data_multiple( _TyThis const & _r ) = default;
	void Clear()
	{
		m_saTuples.Clear();
	}
	bool FIsNull() const
	{
		return !m_saTuples.NElements();
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
		_TyTuple tupleLocal; 
		_ros << "_l_action_save_data_multiple< " << s_kiTrigger << ", false, ";
		std::apply
    (
        [ _pcCharName, &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcCharName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
		_ros << " >";
	}
	string VStrTypeName( const char * _pcCharName ) const
	{
		return StaticStrTypeName( _pcCharName );
	}
	static string StaticStrTypeName( const char * _pcCharName )
	{
		stringstream ss;
		StaticRenderActionType( ss, _pcCharName );
		return ss.str();
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		Trace( "Trigger[%d], Position[%ld].", s_kiTrigger, _rA.GetCurrentPosition() );
		// We copy data from all constituent triggers regardless if that trigger fired. We may change this later.
		// We add data to the SegArray when this trigger/token fires.
		// Get a new element at the end of the segarray:
		_TyTuple & rtupleEnd = m_saTuples.emplaceAtEnd(); // This could throw but we don't expect swapping below to throw since no allocation is performed.
		std::apply
    (
        [&_rA]( t_TysTriggers &... _tuple )
        {
					return ( ..., SwapWithTrigger(_rA,_tuple) );
        }, rtupleEnd
    );
		return true;
	}
	template < class t_TyAnalyzer, class t_TyTrigger >
	static void SwapWithTrigger( t_TyAnalyzer & _rA, t_TyTrigger & _rtThis )
	{
		constexpr vtyTokenIdent kidCur = t_TyTrigger::GetTokenId();
		t_TyTrigger & rtThat = static_cast< t_TyTrigger & >( _rA.template GetActionObj< kidCur >() );
		_rtThis.swap( rtThat );
	}
	void swap( _TyThis & _r )
	{
		m_saTuples.swap( _r.m_saTuples );
		Assert( FIsNull() ); // We generally expect an empty SegArray to be swapped in. This assertion can be removed if there is a usage case for swapping in a non-empty.
	}
	void GetAndClearValue( _TyValue & _rv )
	{
		const size_type knEls = m_saTuples.NElements();
		_rv.SetSize( knEls );
		for ( size_type nEl = 0; nEl < knEls; ++nEl )
		{
			_TyValue & rvEl = _rv[nEl];
			_TyTuple & rtuple = m_saTuples[nEl];
			_rv.SetSize( sizeof...(t_TysTriggers) );
			std::apply
			(
					[&rvEl]( t_TysTriggers &... _tuple )
					{
						size_t nCurEl = 0;
						( ..., _tuple.GetAndClearValue( rvEl[nCurEl++] ) );
					}, rtuple
			);
		}
	}
protected:
	// We save multiple sets of tuple values in a SegArray.
	typedef SegArray< _TyTuple, std::true_type > _TySegArrayTuples;
	_TySegArrayTuples m_saTuples; // ya got yer tuples.
};

__REGEXP_END_NAMESPACE

__BIENUTIL_BEGIN_NAMESPACE 
__REGEXP_USING_NAMESPACE

// Specialize base class mapping for use with _sdpv<> since it depends on knowing the base class of the object.
// Fails to link when attempting to use covariant return.
template < class t_TyActionObject >
struct __map_to_base_class< _l_action_token< t_TyActionObject > >
{
	typedef _l_action_object_base< typename t_TyActionObject::_TyChar, t_TyActionObject::s_fInLexGen >	_TyBase;
};

template < class t_TyChar, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_print< t_TyChar, t_kiToken, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen >	_TyBase;
};

template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen  >
struct __map_to_base_class< _l_trigger_bool< t_TyChar, t_kiTrigger, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen  >
struct __map_to_base_class< _l_trigger_position< t_TyChar, t_kiTrigger, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct __map_to_base_class< _l_trigger_string< t_TyChar, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > >
{
	typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyBase;
};

template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen  >
struct __map_to_base_class< _l_trigger_string_typed_range< t_kdtType, t_TyActionStoreData, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > >
{
	typedef _l_action_object_base< typename t_TyActionStoreData::_TyChar, t_fInLexGen > _TyBase;
};

template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
struct __map_to_base_class< _l_action_save_data_single< t_kiTrigger, t_fInLexGen, t_TysTriggers... > >
{
	typedef _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyChar, t_fInLexGen > _TyBase;
};

template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
struct __map_to_base_class< _l_action_save_data_multiple< t_kiTrigger, t_fInLexGen, t_TysTriggers... > >
{
	typedef _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyChar, t_fInLexGen > _TyBase;
};

__BIENUTIL_END_NAMESPACE


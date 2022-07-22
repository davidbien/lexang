#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_axion.h

// Action objects.

#include <initializer_list>
#include <set>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>
#include "_aloctrt.h"
#include "_l_types.h"
#include "_l_data.h"
#include "_l_value.h"
#include "_l_traits.h"

__REGEXP_BEGIN_NAMESPACE

// _l_action_object_base:
// This base class is only used when we are in the generation stage.
template < class t_TyChar, bool t_fInLexGen >
struct _l_action_object_base
{
private:
  typedef _l_action_object_base< t_TyChar, t_fInLexGen > _TyThis;
public:
  static const bool s_fInLexGen = true;
  typedef t_TyChar	_TyChar;

  // We have a static set of trigger action disambiguating objects.
  typedef pair< _type_info_wrap, _type_info_wrap >		_TyPairTI;
  typedef less< _TyPairTI > _TyCompareTriggers;
  typedef typename _Alloc_traits< typename set<	_TyPairTI, _TyCompareTriggers >::value_type, __L_DEFAULT_ALLOCATOR >::allocator_type _TySetSameTriggersAllocator;
  typedef set<	_TyPairTI, _TyCompareTriggers, 
                _TySetSameTriggersAllocator >		_TySetSameTriggers;
  static _TySetSameTriggers	m_setSameTriggers;

  virtual ~_l_action_object_base() = default;
  _l_action_object_base() = default;

  virtual string VStrTypeName( const char * _pcTraitsName ) const = 0;

  // Return the unique token ID associated with this object.
  // This is the virtual call. The non-virtual call is defined at most-derived class level.
  virtual constexpr vtyTokenIdent VGetTokenId() const = 0;

  // This will return the set of dependent triggers for this action object, recursively.
  virtual void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const = 0;

  // Returning true for this only annotates the accept state with the token id of the accepting actions
  //	it won't generate an action object nor any calls (duh) to said non-existent action object.
  virtual bool FIsTokenIdOnly()
  {
    return false; // Only _l_action_token_id returns true for this.
  }

// REVIEW:<dbien>: Since triggers are now unique the below stuff is kinda meaningless at this point - should cull it and will eventually.
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

  virtual void RenderActionType(ostream & _ros, const char * _pcTraitsName) const = 0;
  virtual void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const = 0;
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
  typedef t_TyChar _TyChar;
  static const bool s_fInLexGen = false;
  ~_l_action_object_base() = default;
  _l_action_object_base() = default;
  _l_action_object_base( _l_action_object_base const & ) = default;
  // Return if the action object is in the null state.
  virtual bool VFIsNull() const = 0;
  // Return the ID for this action object.
  virtual constexpr vtyTokenIdent VGetTokenId() const = 0;
  // This will return the set of dependent triggers for this action object, recursively.
  virtual void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const = 0;
  // Clear any data that is residing within this object.
  virtual void Clear() = 0;
};

// _l_action_object_value_base:
// Lexical generator version:
template < class t_TyTraits, bool t_fInLexGen >
struct _l_action_object_value_base 
  : public _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen >
{
  typedef _l_action_object_value_base _TyThis;
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  using typename _TyBase::_TyChar;
  typedef _l_action_object_value_base _TyActionObjectBase;
  typedef __LEXOBJ_NAMESPACE _l_value< _TyChar, typename _TyTraits::_TyTpValueTraits > _TyValue;
  ~_l_action_object_value_base() = default;
  _l_action_object_value_base() = default;
  _l_action_object_value_base( _l_action_object_value_base const & ) = default;
};
// Incorporates the _l_value< t_TyTraits > type so that a virtual methods may be called with it.
// Lexical analyzer version.
template < typename t_TyTraits >
struct _l_action_object_value_base< t_TyTraits, false > 
    : public _l_action_object_base< typename t_TyTraits::_TyChar, false >
{
  typedef _l_action_object_value_base _TyThis;
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, false > _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  using typename _TyBase::_TyChar;
  typedef __LEXOBJ_NAMESPACE _l_value< _TyChar, typename _TyTraits::_TyTpValueTraits > _TyValue;
  typedef _TyThis _TyActionObjectBase;

  ~_l_action_object_value_base() = default;
  _l_action_object_value_base() = default;
  _l_action_object_value_base( _l_action_object_value_base const & ) = default;
  _l_action_object_value_base( _TyActionObjectBase * _paobNext )
    : m_paobNext( _paobNext )
  {
  }
  _TyThis * PGetAobNext() const
  {
    return m_paobNext;
  }
  using _TyBase::VFIsNull;
  using _TyBase::VGetTokenId;
  using _TyBase::VGetDependentTriggerSet;
  using _TyBase::Clear;
  // Get the set of data (the "value") from the object in a generic form. Leave the object empty of data.
  virtual void GetAndClearValue( _TyValue & _rv ) = 0;
protected:
  _TyThis * m_paobNext{nullptr}; // The previous action object in the list.
};

// _l_action_token_id:
// This is a "pseudo" action object in that it doesn't generate an action object in the lexical analyzer nor
// 	a call to an action method, it merely annotates any accept state(s) with its token id.
// As such we cannot obtain a token from an accept state annotated with this type of action.
// (though I may look into that in the future - I have no need of such functionality now - but it could be nice
//	when you didn't want the heavyweight _l_token system - as I don't want in the output validation work I am doing 
//	now which this is a part of - hence I just need to recognize a token from a set of potential tokens, but
//	I don't need to produce an actual _l_token containing an _l_value. )
// This needn't be fully templatized by the traits but that's how the system is currently working and I don't want 
//	to have to retemplatize everything again.
template < class t_TyTraits, vtyTokenIdent t_ktidToken, bool t_fInLexGen >
struct _l_action_token_id
  : public _l_action_object_value_base< t_TyTraits, t_fInLexGen >
{
private:
  typedef _l_action_token_id	_TyThis;
  typedef _l_action_object_value_base< t_TyTraits, t_fInLexGen > _TyBase;
public:
  using _TyBase::s_fInLexGen;	
  typedef true_type _TyFIsToken; // Must wrap each action with something defining this as true_type.
  static constexpr vtyTokenIdent s_ktidToken = t_ktidToken;
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_action_token_id() = default;
  _l_action_token_id( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_action_token_id( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return true;
  }
  // Return the unique token ID associated with this object.
  static constexpr vtyTokenIdent GetTokenId()
  {
    return t_ktidToken;
  }
  constexpr vtyTokenIdent VGetTokenId() const
  {
    return t_ktidToken;
  }
  // Returning true for this only annotates the accept state with the token id of the accepting actions
  //	it won't generate an action object nor any calls (duh) to said non-existent action object.
  virtual bool FIsTokenIdOnly()
  {
    return true; // Only _l_action_token_id returns true for this.
  }
  void Clear()
  {
  }
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
  }
  void GetAndClearValue( _TyValue & _rv )
  { // nothing to do.
  }
  // We keep these rendering routines intact but we don't expect this action to be rendered currently.
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_action_token_id< " << _pcTraitsName << ", " << s_ktidToken << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_action_token_id< %s, %d, false >", _pcTraitsName, s_ktidToken );
    return str;
  }
// No action method on this object:
  // template < class t_TyAnalyzer >
  // bool action( t_TyAnalyzer & _rA );
};

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
  typedef t_TyActionObj _TyActionObj;
  typedef true_type _TyFIsToken; // Must wrap each action with something defining this as true_type.
  using _TyBase::s_kiToken;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_action_token()
  {
  }
  _l_action_token( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_action_token(_TyThis const & _r)
    : _TyBase(_r)
  {
  }
  using _TyBase::VFIsNull;
  // Return the unique token ID associated with this object.
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  using _TyBase::Clear;
  using _TyBase::VGetDependentTriggerSet;
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    string str = StaticStrTypeName( _pcTraitsName );
    _ros << str;
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string strBase = _TyBase::StaticStrTypeName( _pcTraitsName );
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
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_action_print
  : public _l_action_object_value_base< t_TyTraits, t_fInLexGen >
{
private:
  typedef _l_action_print	_TyThis;
  typedef _l_action_object_value_base< t_TyTraits, t_fInLexGen > _TyBase;
public:
  using _TyBase::s_fInLexGen;	
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_action_print() = default;
  _l_action_print( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_action_print( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return true;
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
  void Clear()
  {
  }
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
  }
  void GetAndClearValue( _TyValue & _rv )
  { // nothing to do.
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_action_print< " << _pcTraitsName << ", " << s_kiTrigger << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_action_print< %s, %d, false >", _pcTraitsName, s_kiTrigger );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
    return true;
  }
};

// _l_trigger_noop:
// This trigger is used by tokens that don't store any data.
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_noop
  : public _l_action_object_value_base< t_TyTraits, t_fInLexGen >
{
private:
  typedef _l_trigger_noop	_TyThis;
  typedef _l_action_object_value_base< t_TyTraits, t_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_noop() = default;
  _l_trigger_noop( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_noop( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return true;
  }
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
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_noop< " << _pcTraitsName << ", " << s_kiTrigger << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_trigger_noop< %s, %d, false >", _pcTraitsName, s_kiTrigger );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
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
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_bool
  : public _l_action_object_value_base< t_TyTraits, t_fInLexGen >
{
private:
  typedef _l_trigger_bool	_TyThis;
  typedef _l_action_object_value_base< t_TyTraits, t_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_bool() = default;
  _l_trigger_bool( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_bool( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return FIsNull();
  }
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
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_bool< " << _pcTraitsName << ", " << s_kiTrigger << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_trigger_bool< %s, %d, false >", _pcTraitsName, s_kiTrigger );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
    m_f = true;
    return true;
  }
  void swap( _TyThis & _r )
  {
    std::swap( m_f, _r.m_f );
  }
  void GetAndClearValue( _TyValue & _rv )
  {
    _rv.SetVal( m_f );
    Clear();
  }
protected:
  bool m_f{false};
};

// Trigger to record a position in a stream.
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen >
struct _l_trigger_position
  : public _l_action_object_value_base< t_TyTraits, t_fInLexGen >
{
private:
  typedef _l_trigger_position	_TyThis;
  typedef _l_action_object_value_base< t_TyTraits, t_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_position() = default;
  _l_trigger_position( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_position( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return FIsNull();
  }
  bool FIsNull() const
  {
    return ( m_tpPos == vkdpNullDataPosition );
  }
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
    m_tpPos = vkdpNullDataPosition;
  }
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_position< " << _pcTraitsName << ", " << s_kiTrigger << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_trigger_position< %s, %d, false >", _pcTraitsName, s_kiTrigger );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
    m_tpPos = _rA.GetCurrentPosition(); // We know that the lex has already read the next character 
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
    _rv.SetVal( m_tpPos );
    Clear();
  }
protected:
  vtyDataPosition m_tpPos{ vkdpNullDataPosition };
};

// Trigger to record an ending position in a stream.
// This is only used as a base class - and really it could be gotten rid of.
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct _l_trigger_position_end
  : public _l_trigger_position< t_TyTraits, t_kiTrigger, t_fInLexGen >
{
private:
  typedef _l_trigger_position_end	_TyThis;
  typedef _l_trigger_position< t_TyTraits, t_kiTrigger, t_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;
  using _TyBase::s_kiTrigger;
  using _TyBase::s_kiToken;
  using _TyBase::s_fInLexGen;	
  static constexpr vtyTokenIdent s_kiTriggerBegin = t_kiTriggerBegin;
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_position_end() = default;
  _l_trigger_position_end( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_position_end( _TyThis const & _r ) = default;
  // Return the unique token ID associated with this object.
  using _TyBase::VFIsNull;
  using _TyBase::FIsNull;
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  using _TyBase::Clear;
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _TyBase::VGetDependentTriggerSet( _rbv );
    _rbv[s_kiTriggerBegin] = true;
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_position_end< " << _pcTraitsName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_trigger_position_end< %s, %d, %d, false >", _pcTraitsName, s_kiTrigger, s_kiTriggerBegin );
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
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct _l_trigger_string
  : public _l_trigger_position_end< t_TyTraits, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen >
{
private:
  typedef _l_trigger_string	_TyThis;
  typedef _l_trigger_position_end< t_TyTraits, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  using typename _TyBase::_TyChar;
  using _TyBase::s_kiTrigger;
  using _TyBase::s_kiToken;
  using _TyBase::s_kiTriggerBegin;
  using _TyBase::s_fInLexGen;	
  typedef __LEXOBJ_NAMESPACE _l_data<> _TyData;
  typedef _l_trigger_position< _TyTraits, t_kiTriggerBegin, t_fInLexGen > _TyTriggerBegin;
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;
  
  _l_trigger_string() = default;
  _l_trigger_string( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_string( _TyThis const & _r ) = default;
// Non-virtual accessors:
  _TyData const & RDataGet() const
  {
    return m_dtStrings;
  }
  _TyData & RDataGet()
  {
    return m_dtStrings;
  }
  bool VFIsNull() const
  {
    return FIsNull();
  }
  bool FIsNull() const
  {
    return _TyBase::FIsNull() && m_dtStrings.FIsNull();
  }
  void Clear()
  {
    _TyBase::Clear();
    m_dtStrings.Clear();
  }
  using _TyBase::VGetDependentTriggerSet;
  // Return the unique token ID associated with this object.
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_string< " << _pcTraitsName << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << ", false >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string str;
    PrintfStdStr( str, "_l_trigger_string< %s, %d, %d, false >", _pcTraitsName, s_kiTrigger, s_kiTriggerBegin );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
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
  template < class t_TyAnalyzer >
  void CheckSetTailEnd( t_TyAnalyzer & _rA, vtyDataTriggerId _nIdTriggerBegin, vtyDataPosition _posEnd, vtyDataType _nType, vtyDataTriggerId _nIdTrigger )
  {
    // Check to ensure that we match the current tail:
    _l_data_typed_range & rdtr = m_dtStrings.RTail();
    VerifyThrowSz( rdtr.id() == _nIdTriggerBegin, "Trigger id on tail of string array[%d], doesn't match the expected tail id[%d].", m_dtStrings.RTail().id(), _nIdTriggerBegin );
    Assert( vkdpNullDataPosition == rdtr.m_posEnd );
    Assert( _nType == rdtr.type() );
    rdtr.m_posEnd = _posEnd;
    // We reset the trigger to the current one:
    rdtr.m_nIdTrigger = _nIdTrigger;
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
    _rv.SetVal( std::move(m_dtStrings) );
    Assert( m_dtStrings.FIsNull() );
  }
protected:
  using _TyBase::GetClearPosition;
  _TyData m_dtStrings;
};

// _l_trigger_string_typed_range:
// This will store a range of input data into t_TyActionStoreData identified by the t_kdtType "type" of data.
// The beginning position of the data is in t_kiTriggerBegin.
// The ending position of the data is contained in this object.
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin >
class _l_trigger_string_typed_range
  : public _l_trigger_position_end< typename t_TyActionStoreData::_TyTraits, t_kiTrigger, t_kiTriggerBegin, t_TyActionStoreData::s_fInLexGen >
{
public:
  typedef typename t_TyActionStoreData::_TyTraits _TyTraits;
private:
  typedef _l_trigger_string_typed_range _TyThis;
  typedef _l_trigger_position_end< _TyTraits, t_kiTrigger, t_kiTriggerBegin, t_TyActionStoreData::s_fInLexGen > _TyBase;
public:
  static constexpr vtyDataType s_kdtType = t_kdtType;
  using typename _TyBase::_TyChar;
  using _TyBase::s_kiTrigger;
  using _TyBase::s_kiToken;
  using _TyBase::s_kiTriggerBegin;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  typedef _l_trigger_position< _TyTraits, s_kiTriggerBegin, s_fInLexGen > _TyTriggerBegin;
  typedef t_TyActionStoreData _TyActionStoreData;
  static constexpr vtyTokenIdent s_kiActionStoreData = _TyActionStoreData::GetTokenId();
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_string_typed_range() = default;
  _l_trigger_string_typed_range( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_string_typed_range( _TyThis const & _r ) = default;
  using _TyBase::VFIsNull;
  using _TyBase::FIsNull;
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  using _TyBase::Clear;
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _TyBase::VGetDependentTriggerSet( _rbv );
    // We must also call t_TyActionStoreData's trigger(s) dependent:
    t_TyActionStoreData asdTemp;
    asdTemp.VGetDependentTriggerSet( _rbv );
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_string_typed_range< " << s_kdtType << ", ";
    _TyActionStoreData::StaticRenderActionType( _ros, _pcTraitsName );
    _ros << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << " >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string strActionStoreData( _TyActionStoreData::StaticStrTypeName( _pcTraitsName ) );
    string str;
    PrintfStdStr( str, "_l_trigger_string_typed_range< %u, %s, %d, %d >", s_kdtType, strActionStoreData.c_str(), s_kiTrigger, s_kiTriggerBegin );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
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
        _TyActionStoreData & raxnStoreData = static_cast< _TyActionStoreData & >( _rA.template GetActionObj< s_kiActionStoreData >() );
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

// _l_trigger_string_typed_beginpoint:
// This object to be paired with _l_trigger_string_typed_endpoint for scenarios where partial triggers may be possible.
// This will store a range of input data into t_TyActionStoreData identified by the t_kdtType "type" of data.
// The beginning position of the data is contained in this object.
// The ending position will always be vkdpNullDataPosition until the corresponding _l_trigger_string_typed_endpoint populates it (or not).
// This is for use when it is possible the beginning trigger to fire and then not receive the ending trigger. This may signify a future error (and probably does)
//	but it leaves the lexical engine in an invalid state.
// This object solves that problem by storing the begin point directly in the eventual container (t_TyActionStoreData),
// Then when the corresponding _l_trigger_string_typed_endpoint comes along it checks to make sure that the begin point is at the end of the container and
//	it matches the trigger id from this trigger.
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger >
class _l_trigger_string_typed_beginpoint
  : public _l_trigger_position< typename t_TyActionStoreData::_TyTraits, t_kiTrigger, t_TyActionStoreData::s_fInLexGen >
{
public:
  typedef typename t_TyActionStoreData::_TyTraits _TyTraits;
private:
  typedef class _l_trigger_string_typed_beginpoint _TyThis;
  typedef _l_trigger_position< typename t_TyActionStoreData::_TyTraits, t_kiTrigger, t_TyActionStoreData::s_fInLexGen > _TyBase;
public:
  static constexpr vtyDataType s_kdtType = t_kdtType;
  using typename _TyBase::_TyChar;
  using _TyBase::s_kiTrigger;
  using _TyBase::s_kiToken;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  typedef t_TyActionStoreData _TyActionStoreData;
  static constexpr vtyTokenIdent s_kiActionStoreData = _TyActionStoreData::GetTokenId();
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_string_typed_beginpoint() = default;
  _l_trigger_string_typed_beginpoint( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_string_typed_beginpoint( _TyThis const & _r ) = default;
  using _TyBase::VFIsNull;
  using _TyBase::FIsNull;
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  using _TyBase::Clear;
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _TyBase::VGetDependentTriggerSet( _rbv );
    // We must also call t_TyActionStoreData's trigger(s) dependent:
    t_TyActionStoreData asdTemp;
    asdTemp.VGetDependentTriggerSet( _rbv );
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_string_typed_beginpoint< " << s_kdtType << ", ";
    _TyActionStoreData::StaticRenderActionType( _ros, _pcTraitsName );
    _ros << ", " << s_kiTrigger << " >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string strActionStoreData( _TyActionStoreData::StaticStrTypeName( _pcTraitsName ) );
    string str;
    PrintfStdStr( str, "_l_trigger_string_typed_beginpoint< %u, %s, %d >", s_kdtType, strActionStoreData.c_str(), s_kiTrigger );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
    bool fRet = _TyBase::action( _rA );
    if ( fRet )
    {
      vtyDataPosition posBegin = GetClearPosition();
      Assert( vkdpNullDataPosition != posBegin );
      if ( vkdpNullDataPosition != posBegin )
      {
        _TyActionStoreData & raxnStoreData = static_cast< _TyActionStoreData & >( _rA.template GetActionObj< s_kiActionStoreData >() );
        raxnStoreData.Append( _rA, posBegin, vkdpNullDataPosition, s_kdtType, s_kiTrigger );
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

// _l_trigger_string_typed_endpoint:
// This will store a range of input data into t_TyActionStoreData identified by the t_kdtType "type" of data.
// The beginning position of the data is contained in this object.
// The ending position will always be vkdpNullDataPosition.
// This is for use when the beginning trigger would be in a position that is untenable - i.e. at the beginning of a token, etc.
// The interpretation of this position is application dependent - the application must post-process and place the correct (begin,end)
//	positions - if it so desires as well - perhaps a single position is all that is required.
// Note that if t_kiTriggerBegin is not vktidInvalidIdTrigger then this object works in tandem with the corresponding
//	_l_trigger_string_typed_beginpoint< t_kiTriggerBegin >. See _l_trigger_string_typed_beginpoint for details (and below of course).
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin >
class _l_trigger_string_typed_endpoint
  : public _l_trigger_position< typename t_TyActionStoreData::_TyTraits, t_kiTrigger, t_TyActionStoreData::s_fInLexGen >
{
public:
  typedef typename t_TyActionStoreData::_TyTraits _TyTraits;
private:
  typedef class _l_trigger_string_typed_endpoint _TyThis;
  typedef _l_trigger_position< typename t_TyActionStoreData::_TyTraits, t_kiTrigger, t_TyActionStoreData::s_fInLexGen > _TyBase;
public:
  static constexpr vtyDataType s_kdtType = t_kdtType;
  static constexpr vtyTokenIdent s_kiTriggerBegin = t_kiTriggerBegin;
  using typename _TyBase::_TyChar;
  using _TyBase::s_kiTrigger;
  using _TyBase::s_kiToken;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  typedef t_TyActionStoreData _TyActionStoreData;
  static constexpr vtyTokenIdent s_kiActionStoreData = _TyActionStoreData::GetTokenId();
  using typename _TyBase::_TyActionObjectBase;

  _l_trigger_string_typed_endpoint() = default;
  _l_trigger_string_typed_endpoint( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_trigger_string_typed_endpoint( _TyThis const & _r ) = default;
  using _TyBase::VFIsNull;
  using _TyBase::FIsNull;
  using _TyBase::GetTokenId;
  using _TyBase::VGetTokenId;
  using _TyBase::Clear;
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _TyBase::VGetDependentTriggerSet( _rbv );
    // We must also call t_TyActionStoreData's trigger(s) dependent:
    t_TyActionStoreData asdTemp;
    asdTemp.VGetDependentTriggerSet( _rbv );
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _ros << "_l_trigger_string_typed_endpoint< " << s_kdtType << ", ";
    _TyActionStoreData::StaticRenderActionType( _ros, _pcTraitsName );
    _ros << ", " << s_kiTrigger << ", " << s_kiTriggerBegin << " >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    string strActionStoreData( _TyActionStoreData::StaticStrTypeName( _pcTraitsName ) );
    string str;
    PrintfStdStr( str, "_l_trigger_string_typed_endpoint< %u, %s, %d, %d >", s_kdtType, strActionStoreData.c_str(), s_kiTrigger, s_kiTriggerBegin );
    return str;
  }
  // We pass the action object the most derived analyzer.
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
    bool fRet = _TyBase::action( _rA );
    if ( fRet )
    {
      vtyDataPosition pos = GetClearPosition();
      Assert( vkdpNullDataPosition != pos );
      if ( vkdpNullDataPosition != pos )
      {
        _TyActionStoreData & raxnStoreData = static_cast< _TyActionStoreData & >( _rA.template GetActionObj< s_kiActionStoreData >() );
        if ( vktidInvalidIdTrigger == s_kiTriggerBegin )
          raxnStoreData.Append( _rA, pos, vkdpNullDataPosition, s_kdtType, s_kiTrigger );
        else
        {
          // Check to make sure that the data at the end of raxnStoreData's array matches our beginning trigger:
          raxnStoreData.CheckSetTailEnd( _rA, s_kiTriggerBegin, pos, s_kdtType, s_kiTrigger );
        }
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
template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
class _l_action_save_data_single 
  : public _l_action_object_value_base< typename tuple_element<0, tuple< t_TysTriggers...>>::type::_TyTraits, tuple_element<0, tuple< t_TysTriggers...>>::type::s_fInLexGen >
{
public:
  typedef typename tuple_element<0, tuple< t_TysTriggers...>>::type::_TyTraits _TyTraits;
private:
  typedef _l_action_save_data_single _TyThis;
  typedef _l_action_object_value_base< _TyTraits, tuple_element<0, tuple< t_TysTriggers...>>::type::s_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;	
  typedef tuple< t_TysTriggers...> _TyTuple;
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_action_save_data_single() = default;
  _l_action_save_data_single( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext )
  {
  }
  _l_action_save_data_single( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return FIsNull();
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
    std::apply
    (
        []( t_TysTriggers &... _tuple )
        {
          ( _tuple.Clear(), ... );
        }, m_tuple
    );
  }
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
    std::apply
    (
        [&_rbv]( t_TysTriggers const &... _tuple )
        {
          ( ..., _tuple.VGetDependentTriggerSet( _rbv ) );
        }, m_tuple
    );
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    // Just used for rendering the types. We'd call a static method on each type in the tuple but I haven't figured out how to do that yet.
    _TyTuple tupleLocal; 
    _ros << "_l_action_save_data_single< " << s_kiTrigger << ", ";
    std::apply
    (
        [ _pcTraitsName, &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcTraitsName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
    _ros << " >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    stringstream ss;
    StaticRenderActionType( ss, _pcTraitsName );
    return ss.str();
  }
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
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
    constexpr size_t knTriggers = sizeof...(t_TysTriggers);
    if ( 1 == knTriggers )
    {
      std::apply
      (
          [&_rv]( t_TysTriggers &... _tuple )
          {
            ( ..., _tuple.GetAndClearValue( _rv ) );
          }, m_tuple
      );
    }
    else
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
  }
  template < class t_TyTrigger >
  const t_TyTrigger& GetConstituentTriggerObj() const
  {
    return get< t_TyTrigger >( m_tuple );
  }
  template < class t_TyTrigger >
  t_TyTrigger& GetConstituentTriggerObj()
  {
    return get< t_TyTrigger >( m_tuple );
  }
protected:
  _TyTuple m_tuple; // ya got yer tuple.
};

// _l_action_save_data_multiple:
// This action object can be used as a trigger object or a token object.
// It saves data by copying it from it's constituent trigger objects.
// It only contains the ability to hold a single vector of constituent trigger data.
// These trigger objects may be aggregate contained _l_action_save_data_multiple objects.
template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
class _l_action_save_data_multiple 
  : public _l_action_object_value_base< typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyTraits, tuple_element<0,tuple< t_TysTriggers...>>::type::s_fInLexGen >
{
public:
  typedef typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyTraits _TyTraits;
private:
  typedef _l_action_save_data_multiple _TyThis;
  typedef _l_action_object_value_base< _TyTraits, tuple_element<0,tuple< t_TysTriggers...>>::type::s_fInLexGen > _TyBase;
public:
  using typename _TyBase::_TyChar;
  typedef tuple< t_TysTriggers...> _TyTuple;
  static const size_t s_kstInitSegArray = 32 * sizeof(_TyTuple);
  static constexpr vtyTokenIdent s_kiTrigger = t_kiTrigger;
  static constexpr vtyTokenIdent s_kiToken = t_kiTrigger;
  using _TyBase::s_fInLexGen;	
  using typename _TyBase::_TyValue;
  using typename _TyBase::_TyActionObjectBase;

  _l_action_save_data_multiple()
    : m_saTuples( s_kstInitSegArray )
  {
  }
  _l_action_save_data_multiple( _TyActionObjectBase * _paobNext )
    : _TyBase( _paobNext ),
      m_saTuples( s_kstInitSegArray )
  {
  }
  _l_action_save_data_multiple( _TyThis const & _r ) = default;
  bool VFIsNull() const
  {
    return FIsNull();
  }
  bool FIsNull() const
  {
    return !m_saTuples.NElements();
  }
  void Clear()
  {
    m_saTuples.Clear();
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
  void VGetDependentTriggerSet( std::vector< bool > & _rbv ) const
  {
    _rbv[s_kiTrigger] = true;
    _TyTuple tupleLocal;
    std::apply
    (
        [&_rbv]( t_TysTriggers &... _tuple )
        {
          ( _tuple.VGetDependentTriggerSet( _rbv ), ... );
        }, tupleLocal
    );
  }
  void RenderActionType(ostream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  void RenderActionType(stringstream & _ros, const char * _pcTraitsName) const
  {
    return StaticRenderActionType(_ros, _pcTraitsName);
  }
  template < class t_TyOStream >
  static void StaticRenderActionType(t_TyOStream & _ros, const char * _pcTraitsName)
  {
    _TyTuple tupleLocal; 
    _ros << "_l_action_save_data_multiple< " << s_kiTrigger << ", ";
    std::apply
    (
        [ _pcTraitsName, &_ros ]( t_TysTriggers const &... _tuple )
        {
            std::size_t n = sizeof...(t_TysTriggers);
            ( ( _tuple.RenderActionType(_ros,_pcTraitsName), ( _ros << (!--n ? "" : ", ") ) ), ...);
        }, tupleLocal
    );
    _ros << " >";
  }
  string VStrTypeName( const char * _pcTraitsName ) const
  {
    return StaticStrTypeName( _pcTraitsName );
  }
  static string StaticStrTypeName( const char * _pcTraitsName )
  {
    stringstream ss;
    StaticRenderActionType( ss, _pcTraitsName );
    return ss.str();
  }
  template < class t_TyAnalyzer >
  bool action( t_TyAnalyzer & _rA )
  {
    Trace( "Trigger[%d], Position[%llu].", s_kiTrigger, _rA.GetCurrentPosition() );
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
    const typename _TySegArrayTuples::_tySizeType knEls = m_saTuples.NElements();
    _rv.SetSize( knEls );
    for ( typename _TySegArrayTuples::_tySizeType nEl = 0; nEl < knEls; ++nEl )
    {
      _TyValue & rvEl = _rv[nEl];
      _TyTuple & rtuple = m_saTuples[nEl];
      rvEl.SetSize( sizeof...(t_TysTriggers) );
      std::apply
      (
          [&rvEl]( t_TysTriggers &... _tuple )
          {
            size_t nCurEl = 0;
            ( ..., _tuple.GetAndClearValue( rvEl[nCurEl++] ) );
          }, rtuple
      );
    }
    m_saTuples.SetSize( 0 ); // Don't Clear() it because that deallocates all the blocks - this leaves them allocated for the next token.
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
template < class t_TyTraits, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_token_id< t_TyTraits, t_kiToken, t_fInLexGen > >
{
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyActionObject >
struct __map_to_base_class< _l_action_token< t_TyActionObject > >
{
  typedef _l_action_object_base< typename t_TyActionObject::_TyChar, t_TyActionObject::s_fInLexGen > _TyBase;
};

template < class t_TyTraits, int t_kiToken, bool t_fInLexGen >
struct __map_to_base_class< _l_action_print< t_TyTraits, t_kiToken, t_fInLexGen > >
{
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen  >
struct __map_to_base_class< _l_trigger_bool< t_TyTraits, t_kiTrigger, t_fInLexGen > >
{
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen  >
struct __map_to_base_class< _l_trigger_position< t_TyTraits, t_kiTrigger, t_fInLexGen > >
{
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};

template < class t_TyTraits, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen >
struct __map_to_base_class< _l_trigger_string< t_TyTraits, t_kiTrigger, t_kiTriggerBegin, t_fInLexGen > >
{
  typedef _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};

template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin  >
struct __map_to_base_class< _l_trigger_string_typed_range< t_kdtType, t_TyActionStoreData, t_kiTrigger, t_kiTriggerBegin > >
{
  typedef _l_action_object_base< typename t_TyActionStoreData::_TyChar, t_TyActionStoreData::s_fInLexGen > _TyBase;
};

template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger  >
struct __map_to_base_class< _l_trigger_string_typed_beginpoint< t_kdtType, t_TyActionStoreData, t_kiTrigger > >
{
  typedef _l_action_object_base< typename t_TyActionStoreData::_TyChar, t_TyActionStoreData::s_fInLexGen > _TyBase;
};

template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin  >
struct __map_to_base_class< _l_trigger_string_typed_endpoint< t_kdtType, t_TyActionStoreData, t_kiTrigger, t_kiTriggerBegin > >
{
  typedef _l_action_object_base< typename t_TyActionStoreData::_TyChar, t_TyActionStoreData::s_fInLexGen > _TyBase;
};

template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
struct __map_to_base_class< _l_action_save_data_single< t_kiTrigger, t_TysTriggers... > >
{
  typedef _l_action_object_base< typename tuple_element<0,tuple< t_TysTriggers...>>::type::_TyChar, tuple_element<0, tuple< t_TysTriggers...>>::type::s_fInLexGen > _TyBase;
};

template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
struct __map_to_base_class< _l_action_save_data_multiple< t_kiTrigger, t_TysTriggers... > >
{
  typedef _l_action_object_base< typename tuple_element<0, tuple< t_TysTriggers...>>::type::_TyChar, tuple_element<0, tuple< t_TysTriggers...>>::type::s_fInLexGen > _TyBase;
};

__BIENUTIL_END_NAMESPACE


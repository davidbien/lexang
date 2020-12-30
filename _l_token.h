#pragma once

// _l_token.h
// This object represents a token result from the lexicographical analyzer.
// dbien
// 15DEC2020

// Design goals: Do as little as possible as late as possible - since the user may or may not ever want to do those things.
// When the user of the lexicographical analyzer gets the token m_value will be filled with positions, booleans, etc - 
//  i.e. the positions that were recorded when triggers fired and booleans that were set when triggers fired, etc.
// The _l_token is populated with a "user context" which contains a transport-context and a user-object. The transport context
//  holds information relevant to m_value's contents - i.e. in the case for a mapped file it will contain a pointer to the data
//  and a length for the token's total data. In the case of file transport (file desscriptor in Linux or HANDLEs in Windows)
//  the transport context will contain the backing memory and the position where this backing memory starts in the stream and
//	the length of the token. This allows the token to exist independently of the stream.
// The user of the token of course must understand the structure and meaning the aggregate value in m_value. The user can convert
//  those values to strings, etc. according to how the lexicographical analyzer is being used. In fact that is what use of the token
//  involves: perusing values and reacting to them. They can be converted to a given representation that is useful, etc.

#include "_l_ns.h"
#include "_l_types.h"
#include "_l_value.h"
#include "_l_strm.h"
#include "_l_data.h"

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyUserContext >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef typename t_TyUserContext::_TyChar _TyChar;
  typedef t_TyUserContext _TyUserContext;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyChar > _TyValue;
  typedef typename _TyValue::size_type size_type;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  _l_token() = delete;
  // We could make this protected, etc, but I don't worry that it will be accidetally called.
  _l_token( _TyUserContext && _rrucxt, _TyValue && _rrvalue, const _TyAxnObjBase * _paobCurToken )
    : m_scx( std::move( _rrucxt ) ),
      m_value( std::move( _rrvalue ) ),
      m_paobCurToken( _paobCurToken )
  {
  }
  // We are copyable:
  _l_token( _l_token const & ) = default;
  _l_token & operator =( _l_token const & _r )
  {
    _l_token copy( _r );
    swap( copy );
    return *this;
  }
  // We are moveable:
  _l_token( _l_token && ) = default;
  _l_token & operator =( _l_token && _rr )
  {
    _l_token moved( std::move( _rr ) );
    swap( moved );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_scx.swap( _r.m_scx );
    m_value.swap( _r.m_value );
    std::swap( m_paobCurToken, _r.m_paobCurToken );
  }

  _TyValue & GetValue()
  {
    return m_value;
  }
  const _TyValue & GetValue() const
  {
    return m_value;
  }

  const _TyAxnObjBase * PAxnObjGet() const
  {
    return m_paobCurToken;
  }
  vtyTokenIdent TokenGet() const
  {
    return m_paobCurToken->VGetTokenId();
  }
  _TyValue & operator [] ( size_type _nEl )
  {
    return m_value[_nEl];
  }
  const _TyValue & operator [] ( size_type _nEl ) const
  {
    return m_value[_nEl];
  }

  template < class t_tyStringView >
  void GetStringView( t_tyStringView & _rsvDest, _TyValue & _rval )
  {
    m_scx.GetStringView( _rsvDest, *this, _rval );
  }
  template < class t_tyStringView, class t_tyString >
  bool FGetStringViewOrString( t_tyStringView & _rsvDest, t_tyString & _rstrDest, _TyValue const & _rval )
  {
    return m_scx.FGetStringViewOrString( _rsvDest, _rstrDest, *this, _rval );
  }
  template < class t_tyString >
  void GetString( t_tyString & _rstrDest, _TyValue const & _rval )
  {
    m_scx.GetString( _rstrDest, *this, _rval );
  }

protected:
  _TyUserContext m_scx; // The context for the stream which is passed to various _l_value methods.
  _TyValue m_value; // This value's context is in m_scx.
  const _TyAxnObjBase * m_paobCurToken; // Pointer to the action object for this token - from which the token id is obtainable, etc.
};

__LEXOBJ_END_NAMESPACE

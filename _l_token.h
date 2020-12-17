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
#include "_l_value.h"
#include "_l_strm.h"

template < class t_TyUserContext >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef typename t_TyUserContext::_TyChar _TyChar;
  typedef t_TyUserContext _TyUserContext;
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar > _TyValue;

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

protected:
  _TyUserContext m_scx; // The context for the stream which is passed to various _l_value methods.
  _TyValue m_value; // This value's context is in m_scx.
  vtyTokenIdent m_tid; // We are this here token.
};

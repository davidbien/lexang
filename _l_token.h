#pragma once

// _l_token.h
// This object represents a token result from the lexicographical analyzer.
// dbien
// 15DEC2020

// Design goals: Do as little as possible as late as possible - since the user may or may not ever want to do those things.

template < class t_TyChar, class t_TyStreamContext >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyStreamContext _TyStreamContext;
  typedef _l_value< t_TyChar > _TyValue;

protected:
  _TyStreamContext m_scx; // The context for the stream which is passed to various _l_value methods.
  _TyValue m_value; // This value's context is in m_scx.
  vtyTokenIdent m_tid; // We are this here token.
};

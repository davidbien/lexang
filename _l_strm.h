#pragma once

// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

// Transports for the stream objects.

#include "_fdobjs.h"
#include "fdrotbuf.h"
#include "_l_ns.h"
#include "_l_types.h"
#include "_l_transport.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_stream: An input stream for the lexicographical analyzer.
template < class t_TyTransport, class t_TyUserObj >
class _l_stream
{
  typedef _l_stream _TyThis;
public:
  typedef t_TyTransport _TyTransport;
  typedef typename t_TyTransport::_TyChar _TyChar;
  typedef typename t_TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef t_TyUserObj _TyUserObj;
  typedef _l_user_context< _TyTransportCtxt, _TyUserObj > _TyUserContext;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyChar > _TyValue;
  typedef _l_token< _TyUserContext > _TyToken;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  _l_stream() = default;
  _l_stream( _l_stream const & ) = delete;
  _l_stream & operator =( _l_stream const & ) = delete;
  _l_stream( _l_stream && ) = default;
  _l_stream & operator =( _l_stream && ) = default;

  // Use this constructor for non-copyable objects or stateful objects that we don't want to copy, etc.
  _l_stream( _TyUserObj && _rruo )
    : m_uoUserObj( std::move( _rruo ) )
  {
  }
  _l_stream( _TyUserObj const & _ruo )
    : m_uoUserObj( _ruo )
  {
  }

  // Construct the transport object appropriately.
  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    (void)m_opttpImpl.emplace( std::forward< t_TysArgs >( _args )... );
  }

  vtyDataPosition PosCurrent() const
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->PosCurrent();
  }
  bool FAtTokenStart() const
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->FAtTokenStart();
  }

  // Get a single character from the transport and advance the input stream.
  bool FGetChar( _TyChar & _rc )
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->FGetChar( _rc );
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  void GetPToken( _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, unique_ptr< _TyToken > & _rupToken )
  {
    Assert( m_opttpImpl.has_value() );
    _TyValue value;
    _paobCurToken->GetAndClearValue( value );
    m_opttpImpl->GetPToken( _paobCurToken, _kdpEndToken, std::move( value ), m_uoUserObj, _rupToken );
  }
  // This method is called when an action object returns false from its action() method.
  // This will cause the entire token found to be discarded without further processing - the fastest way if ignoring a token.
  void DiscardData( const vtyDataPosition _kdpEndToken )
  {
    Assert( m_opttpImpl.has_value() );
    m_opttpImpl->DiscardData( _kdpEndToken );
  }

protected:
  typedef optional< t_TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  _TyUserObj m_uoUserObj; // This is the one copy of the user object that is referenced in each _TyUserContext object. This allows the user object to maintain stateful information is that is desireable.
};

__LEXOBJ_END_NAMESPACE
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
template < class t_TyTraits >
class _l_stream
{
  typedef _l_stream _TyThis;
public:
  typedef t_TyTraits _TyTraits;
  typedef typename _TyTraits::_TyTransport _TyTransport;
  typedef typename _TyTransport::_TyChar _TyChar;
  typedef typename _TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef typename _TyTraits::_TyUserObj _TyUserObj;
  using _TyPtrUserObj = typename _TyTraits::_TyPtrUserObj;
  typedef _l_user_context< _TyTraits > _TyUserContext;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyTraits > _TyValue;
  typedef _l_token< _TyTraits > _TyToken;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;
  typedef _l_action_object_value_base< _TyTraits, false > _TyAxnObjValueBase;

  _l_stream() = default;
  _l_stream( _l_stream const & ) = delete;
  _l_stream & operator =( _l_stream const & ) = delete;
  _l_stream( _l_stream && ) = default;
  _l_stream & operator =( _l_stream && ) = default;

  // Use this constructor for non-copyable objects or stateful objects that we don't want to copy, etc.
  _l_stream( _TyUserObj && _rruo )
    : m_upUserObj( make_unique< _TyUserObj >( std::move( _rruo ) ) )
  {
  }
  _l_stream( _TyUserObj const & _ruo )
    : m_upUserObj( make_unique< _TyUserObj >( _ruo ) )
  {
  }
  _TyTransport & GetTransport()
  {
    return *m_opttpImpl;
  }
  const _TyTransport & GetTransport() const
  {
    return *m_opttpImpl;
  }
  _TyUserObj & GetUserObj()
  {
    return *m_upUserObj;
  }
  const _TyUserObj & GetUserObj() const
  {
    return *m_upUserObj;
  }
  // Allow access to the pointer object for transfer out.
  _TyPtrUserObj & GetUserObjPtr()
  {
    return m_upUserObj;
  }
  // Construct the transport object appropriately.
  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    (void)m_opttpImpl.emplace( std::forward< t_TysArgs >( _args )... );
  }
  // Construct a specific transport type in a transport_var.
  template < class t_TyTransport, class ... t_TysArgs >
  void emplaceVarTransport( t_TysArgs&& ... _args )
  {
    if ( !m_opttpImpl )
      m_opttpImpl.emplace(); // Just create a default var_transport object with no active variant.
    m_opttpImpl->template emplaceTransport< t_TyTransport >( std::forward< t_TysArgs >( _args ) ... );
  }
  vtyDataPosition PosCurrent() const
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->PosCurrent();
  }
  vtyDataPosition PosTokenStart() const
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->PosTokenStart();
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
  void GetPToken( _TyAxnObjValueBase * _paobCurToken, const vtyDataPosition _kdpEndToken, unique_ptr< _TyToken > & _rupToken )
  {
    Assert( m_opttpImpl.has_value() );
    _TyValue value;
    _paobCurToken->GetAndClearValue( value );
    m_opttpImpl->GetPToken( _paobCurToken, _kdpEndToken, std::move( value ), *m_upUserObj, _rupToken );
  }
  // This method is called when an action object returns false from its action() method.
  // This will cause the entire token found to be discarded without further processing - the fastest way if ignoring a token.
  void DiscardData( const vtyDataPosition _kdpEndToken )
  {
    Assert( m_opttpImpl.has_value() );
    m_opttpImpl->DiscardData( _kdpEndToken );
  }
  // Return the current token string that has been read until this point.
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
  {
    Assert( m_opttpImpl.has_value() );
    m_opttpImpl->GetCurTokenString( _rstr );
  }
  bool FSpanChars( const _TyData & _rdt, const _TyChar * _pszCharSet ) const
  {
    Assert( m_opttpImpl.has_value() );
    return m_opttpImpl->FSpanChars( _rdt, _pszCharSet );
  }
  bool FMatchChars( const _TyData & _rdt, const _TyChar * _pszMatch ) const
  {
    Assert( m_opttpImpl.has_value() );
    if ( StrNLen( _pszMatch ) != _rdt.DataRangeGetSingle().length() )
      return false; // short circuit.
    return m_opttpImpl->FMatchChars( _rdt, _pszMatch );
  }
protected:
  typedef optional< _TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  unique_ptr< _TyUserObj > m_upUserObj; // This is the one copy of the user object that is referenced in each _TyUserContext object. Needs to be pointer so we can transfer to xml_document.
};

__LEXOBJ_END_NAMESPACE

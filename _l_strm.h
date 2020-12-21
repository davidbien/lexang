#pragma once

#include "_l_ns.h"
#include "_l_types.h"

// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

// Transports for the stream objects.

#include "_l_ns.h"
#include "_l_types.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_transport_base: base for all transports.
// Templatize by the character type contained in the file.
template < class t_TyChar >
class _l_transport_base
{
  typedef _l_transport_base _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar > _TyValue;
};

static const size_t vkstTransportFdTokenBufferSize = 256;
static_assert( sizeof( off_t ) == sizeof( vtyDataPosition ) );

// _l_transport_fd_ctxt:
// This is the context that is contained in an _l_token allowing the also contained _l_value to exist on its own(ish).
// Text translation specific to a given parser is built into the most derived class of this object.
template < class t_TyChar >
class _l_transport_fd_ctxt : public _l_transport_ctxt_base< t_TyChar >
{
  typedef _l_transport_fd_ctxt _TyThis;
  typedef _l_transport_ctxt_base< t_TyChar > _TyBase;
public:
  typedef _l_transport_fd< t_TyChar > _TyTransportFd;
  typedef SegArrayRotatingBuffer< t_TyChar > _TySegArrayBuffer;

  _l_transport_fd_ctxt( _TyTransportFd * _ptxpFd )
    : m_ptxpFd( _ptxpFd )
  {
  }
  _l_transport_fd_ctxt() = default;
  _l_transport_fd_ctxt( _l_transport_fd_ctxt const & ) = default;
  _l_transport_fd_ctxt( _l_transport_fd_ctxt && ) = default;
  _l_transport_fd_ctxt & operator =( _l_transport_fd_ctxt const & ) = default;
  _l_transport_fd_ctxt & operator =( _l_transport_fd_ctxt && ) = default;

#if 0
  // void InitTransportCtxt( _TyTransportFd * _ptxpFd, _TySegArrayBuffer && _rrsaTokenBuf )
  // {
  //   m_ptxpFd = _ptxpFd;
  //   m_saTokenBuf = std::move( _rrsaTokenBuf );
  // }
#endif //0

  _TyTransportFd * PGetTransport() const
  {
    return m_ptxpFd;
  }
  _TySegArrayBuffer & GetTokenBuffer()
  {
    return m_saTokenBuf;
  }
  const _TySegArrayBuffer & GetTokenBuffer() const
  {
    return m_saTokenBuf;
  }
protected:
  _TyTransportFd * m_ptxpFd{nullptr}; // We need to be able to return our token buffer to the transport when we are done with it.
  _TySegArrayBuffer m_saTokenBuf{vkstTransportFdTokenBufferSize}; // This contains the piece of the input that corresponds to the entirety of the token.
};

// _l_transport_fd:
// Transport using a file descriptor.
template < class t_TyChar >
class _l_transport_fd : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_fd _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;
  typedef _l_transport_fd_ctxt< t_TyChar > _TyTransportCtxt;

  _l_transport_fd( const char * _pszFileName )
  {
    errno = 0;
    int fd = open( _pszFileName, O_RDONLY );
    if ( -1 == fd )
      THROWNAMEDEXCEPTIONERRNO( errno, "Open of [%s] failed.", _pszFileName );
    m_fd = fd;
    m_fOwnFd = true;
  }
  // Attach to an fd like STDIN - in which case you would set _fOwnFd to false.
  _l_transport_fd( int _fd, bool _fOwnFd = false )
    : m_fd( _fd)
      m_fOwnFd( _fOwnFd )
  {
  }
  ~_l_transport_fd()
  {
    if ( m_fOwnFd && ( -1 != m_fd ) )
    {
      int fd = m_fd;
      m_fd = -1;
      errno = 0;
      int iCloseResult = close( fd );
      Assert( !iCloseResult );
      if ( -1 == iCloseResult )
        LOGSYSLOGERRNO( eslmtError, errno, "~_l_transport_fd(): close() returned an error.")
    }
  }

  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, unique_ptr< _TyToken > & _rupToken )
  {
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    _TyUserContext ucxt( _ruoUserObj, this );
    // This method ends the current token at _kdpEndToken - this causes some housekeeping within this object.
    m_frrFileDesBuffer.ConsumeData( m_posTokenStart, _kdpEndToken, ucxt.GetTokenBuffer() );
    m_posTokenStart = _kdpEndToken;
    unique_ptr< _TyToken > upToken = make_unique( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }

protected:
  // We define a "rotating buffer" that will hold a single token *no matter how large* (since this is how the algorithm works to allow for STDIN filters to work).
  typedef FdReadRotating< _TyChar > _TyFdReadRotating;
  _TyFdReadRotating m_frrFileDesBuffer{vkstTransportFdTokenBufferSize};
  off_t m_posTokenStart{0}; // The position in the stream at the start of the current token.
  int m_fd{-1};
  bool m_fOwnFd{false};
};

template < class t_TyChar >
class _l_transport_fixedmem_ctxt
{
  typedef _l_transport_fixedmem_ctxt _TyThis;
public:
  _l_transport_fixedmem_ctxt() = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt const & ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  typedef _l_transport_fixedmem< t_TyChar > _TyTransportFixedMem;
  typedef t_TyChar _TyChar;
  typedef std::pair< const t_TyChar *, vtyDataPosition > _TyPrMemView;

  _TyTransportFixedMem * PGetTransport() const
  {
    return m_ptxpFixedMem;
  }
  _TyPrMemView const & RPrmvCurrentToken() const
  {
    return m_prmvCurrentToken;
  }
  _TyPrMemView const & RPrmvFull() const
  {
    return m_ptxpFixedMem->m_prmvFull;
  }
public:
  _TyPrMemView m_prmvCurrentToken;
  _TyTransportFixedMem * m_ptxpFixedMem;
};

// _l_transport_fixedmem:
// Transport that uses a piece of memory.
template < class t_TyChar >
class _l_transport_fixedmem : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_fixedmem _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyTransportCtxt;
  typedef typename _TyTransportCtxt::_TyPrMemView _TyPrMemView;

  _l_transport_fixedmem( _l_transport_fixedmem const & _r ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis const & _r ) = delete;

  _l_transport_fixedmem( const _TyChar * _pcBase, vtyDataPosition _nLen )
    : m_pcBase( _pcBase ),
      m_nLen( _nLen )
  {
  }

  // Transfer the current token (conceptually in this case mostly) to the caller.
  void GetCurTransportContext( _TyTransportCtxt & _rcxt )
  {
    _rcxt.m_prmvCurrentToken = m_prmvCurrentToken;
    // Move the token to the start of the next token.
    m_prmvCurrentToken.first = _rcxt.m_prmvCurrentToken.first + _rcxt.m_prmvCurrentToken.second;
    m_prmvCurrentToken.second = 0;
  }
  // Caller returns any resources that were given out by GetCurTransportContext() - which in case is none.
  void ReturnTransportContext( _TyTransportCtxt & _rcxt )
  {
    // no-op - but clear the context anyway to make it work similar to _l_transport_fd - i.e. if caller uses it afterwards then it shouldn't work.
    _rcxt.m_prmvCurrentToken = nullptr;
    _rcxt.m_prmvCurrentToken = 0;
  }
protected:
  _TyPrMemView m_prmvFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  _TyPrMemView m_prmvCurrentToken; // The view for the current token.
};

template < class t_TyChar >
class _l_transport_mapped_ctxt : public _l_transport_fixedmem_ctxt< t_TyChar >
{
  typedef _l_transport_mapped_ctxt _TyThis;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyBase;
public:
  // Not sure if we need anything else here.
};

// _l_transport_mapped
// Transport that uses mapped memory.
template < class t_TyChar >
class _l_transport_mapped : public _l_transport_fixedmem< t_TyChar >
{
  typedef _l_transport_mapped _TyThis;
  typedef _l_transport_fixedmem< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;
  typedef _l_transport_mapped_ctxt< t_TyChar > _TyTransportCtxt;

};

// _l_user_context:
// This produces an aggregate which provides customizable treatment of a token's constituent _l_data_typed_range objects.
template < class t_TyTransportCtxt, class t_tyUserObj = _l_default_user_obj >
class _l_user_context : public t_TyTransportCtxt
{
  typedef _l_user_context _TyThis;
  typedef t_TyTransportCtxt _TyBase;
public:
  typedef t_tyUserObj _tyUserObj;
  typedef typename t_TyTransportCtxt::_TyChar _TyChar;
  typedef _l_token< _TyThis > _TyToken;

  _l_user_context() = delete;
  _l_user_context & operator =( _l_user_context const & ) = delete;
  _l_user_context( _TyUserObj & _ruo )
    : m_ruoUserObj( _ruo )
  {
  }
  template < class ... t_tysArgs >
  _l_user_context( _TyUserObj & _ruo, t_tysArgs && ... _args )
    : m_ruoUserObj( _ruo ),
      _TyBase( std::forward< t_tysArgs >(_args) ... )
  {
  }

  // We are copyable.
  _l_user_context( _TyThis const & ) = default; 

  // Generic initialization of transport context.
  template < class ... t_tysArgs >
  void InitTransportCtxt( t_tysArgs && ... _args )
  {
    _TyBase::InitTransportCtxt( std::forward< t_TysArgs>( _args ) ... );
  }


  template < class t_tyStringView >
  void GetStringView( t_tyStringView & _rsvDest, _TyToken & _rtok, _TyValue & _rval )
  {
    // yet another delegation...and add another parameter.
    m_ruoUserObj.GetStringView( _rsvDest, *(_TyBase*)this, _rtok, _rval );
  }
  template < class t_tyStringView, class t_tyString >
  bool FGetStringViewOrString( t_tyStringView & _rsvDest, t_tyString & _rstrDest,_TyToken & _rtok, _TyValue const & _rval )
  {
    // yet another delegation...and add another parameter.
    return m_ruoUserObj.FGetStringViewOrString( _rsvDest,  _rstrDest, *(_TyBase*)this, _rtok, _rval );
  }
  template < class t_tyString >
  void GetString( t_tyString & _rstrDest, _TyToken & _rtok, _TyValue const & _rval )
  {
    // yet another delegation...and add another parameter.
    m_ruoUserObj.GetString( _rstrDest, *(_TyBase*)this, _rtok, _rval );
  }
protected:
  t_tyUserObj & m_ruoUserObj; // This is a reference to the user object that was passed into the _l_stream constructor.
};

// _l_stream: An input stream for the lexicographical analyzer.
template < class t_TyTransport, class t_tyUserObj >
class _l_stream
{
  typedef _l_stream _TyThis;
public:
  typedef typename t_TyTransport::_TyChar _TyChar;
  typedef typename t_TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef _l_user_context< t_tyUserObj, _TyTransportCtxt > _TyUserContext;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_value< _TyChar > _TyValue;
  typedef _l_token< _TyUserContext > _TyToken;

  _l_stream() = delete;
  _l_stream( _l_stream const & ) = delete;
  _l_stream & operator =( _l_stream const & ) = delete;

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
  _TyT & emplaceTransport( t_TysArgs&&... _args )
  {
    m_opttpImpl.emplace( std::forward< t_TysArgs >( _args )... );
  }

  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  void GetPToken( _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, unique_ptr< _TyToken > & _rupToken )
  {
    _TyValue value;
    _paobCurToken->GetAndClearValue( value );
    m_opttpImpl.GetPToken( _paobCurToken, _kdpEndToken, std::move( value ), m_uoUserObj, _rupToken );
  }

protected:
  typedef optional< t_TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  _TyUserObj m_uoUserObj; // This is the one copy of the user object that is referenced in each _TyUserContext object. This allows the user object to maintain stateful information is that is desireable.
};

__LEXOBJ_END_NAMESPACE
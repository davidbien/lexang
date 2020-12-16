#pragma once

#include "_l_ns.h"
#include "_l_types.h"



// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

// Transports for the stream objects.

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
  typedef SegArray< t_TyChar, false_Type > _TySegArrayBuffer;

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
  off_t m_posTokenStart{numeric_limits< vtyDataPosition >::max()>}; // The position in the stream at the start of the current token.
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
      THROWNAMEDEXCEPTIONERRNO( errno, "_l_transport_fd::_l_transport_fd(): open of [%s] failed.", _pszFileName );
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
  // No need to provide a bunch of utility methods here...

  // Transfer the current context and do appropriate housekeeping...
  // This will transfer the current _TySegArrayBuffer to _rcxt because it will then be associated with a _l_token.
  void GetCurTransportContext( _TyTransportCtxt & _rcxt )
  {
    typename _TyTransportCtxt::_TySegArrayBuffer & rsab = _rcxt.GetTokenBuffer();
    rsab.SetSizeSmaller(0,false); // Keep the memory around for reuse - if there is any in this object.
    rsab.swap( m_saTokenBuf );
    if ( !m_saTokenBuf.FHasAnyCapacity() && !m_lListFreeSegArrays.empty() )
    {
      // We'll cycle through any buffers to be fair to them - pop from the back and push to the front:
      m_saTokenBuf.swap( m_lListFreeSegArrays.back() );
      m_lListFreeSegArrays.pop_back();
    }
    // else things will just happen normally - just not preallocated this time.
    _rcxt.m_ptxpFd = this; // So they can call us back to release the context.
    _rcxt.m_posTokenStart = m_posTokenStart;
    m_posTokenStart += rsab.m_saTokenBuf.GetSize(); // Set to the next token.
  }
  // Return the transport context to the transport so that it can be reused for a another token.
  // The user should be done using any data associated with the token.
  void ReturnTransportContext( _TyTransportCtxt & _rcxt )
  {
    Assert( _rcxt.m_ptxpFd == this ); // Doesn't matter that much but probably the indication of a bug if this happens.
    // REVIEW:<dbien>: We'd like to not have a list here and allocate memory every time we add to it but I haven't thought of a better way yet.
    typename _TyTransportCtxt::_TySegArrayBuffer & rsab = _rcxt.GetTokenBuffer();
    Assert( rsab.FHasAnyCapacity() ); // If we can get no-capacity segarrays returned then we shouldn't put them on the list.
    rsab.SetSizeSmaller(0,false);
    m_lListFreeSegArrays.emplace_front( std::move( rsab ) );
  }

protected:
  // We define a "rotating buffer" that will hold a single token *no matter how large* (since this is how the algorithm works to allow for STDIN filters to work).
  // Actually we don't really need the "rotating" functionality because we will store the entire token and then clear it once the data has been moved.
  // We will do this in all cases when using an fd currently as this simplifies the algorithm and also allows us to use basic_string_views for all data until
  //  we convert it into the character type that the user of the parser is using.
  // In this way we will never have to seek the stream which is preferable as seek is a system call and thus could result in a context switch or blocking, etc.
  typedef SegArray< t_TyChar, false_Type > _TySegArrayBuffer;
  // We are going to store the SegArrayBuffer for an individual token first here, then we will pass it to the _l_value for the token itself. This is only necessary for non-mapped/in-memory transports.
  typedef std::list< _TySegArrayBuffer > _TyListFreeSegArrays;
  _TyListFreeSegArrays m_lListFreeSegArrays;
  _TySegArrayBuffer m_saTokenBuf{vkstTransportFdTokenBufferSize}; // We need the token in memory to deal with it.
  off_t m_posAttached; // The position the fd was at when we were attached to it.
  off_t m_posTokenStart; // The position in the stream at the start of the current token.
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
  typedef t_TyChar _TyChar;
  typedef std::pair< const t_TyChar *, vtyDataPosition > _TyPrMemView;

  _TyPrMemView m_prmvCurrentToken;
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
template < class t_tyUserObj, class t_TyTransportCtxt >
class _l_user_context : public t_TyTransportCtxt
{
  typedef _l_user_context _TyThis;
  typedef t_TyTransportCtxt _TyBase;
public:
  typedef typename t_TyTransportCtxt::_TyChar _TyChar;
  



protected:
  t_tyUserObj m_uoUserObj;
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

  _l_stream() = default;

  // Construct the transport object appropriately.
  template < class... t_TysArgs >
  _TyT & emplaceTransport( t_TysArgs&&... _args )
  {
    m_opttpImpl.emplace( std::forward< t_TysArgs >( _args )... );
  }

  // Read the Convert the strings present in this _TyValue in-place to the character type t_TyCharConvertTo.
  template < class t_TyCharConvertTo >
  void ReadAndConvertStrings( _TyValue & _rv )
  {
    
  }

protected:
  typedef optional< t_TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  _TyChar m_posCur; // The current position in the stream. This is kept maintained with any transports that also have a current position.
};

#pragma once

// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

// Transports for the stream objects.

#include "_fdobj.h"
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

static const size_t vknchTransportFdTokenBufferSize = 256;
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
    : m_ptxpFd( _ptxpFd ),
      m_saTokenBuf( 0 ) // This is always set later.
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
  _TySegArrayBuffer m_saTokenBuf{ vknchTransportFdTokenBufferSize * sizeof( t_TyChar ) }; // This contains the piece of the input that corresponds to the entirety of the token.
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

  ~_l_transport_fd() = default;
  _l_transport_fd() = delete;
  _l_transport_fd( const _l_transport_fd & ) = delete;
  _l_transport_fd & operator =( _l_transport_fd const & ) = delete;
  // We could allow move construction but it isn't needed because we can emplace construct.
  _l_transport_fd( _l_transport_fd && ) = delete;
  _l_transport_fd & operator =( _l_transport_fd && ) = delete;

  _l_transport_fd( const char * _pszFileName )
  {
    errno = 0;
    int fd = open( _pszFileName, O_RDONLY );
    if ( -1 == fd )
      THROWNAMEDEXCEPTIONERRNO( errno, "Open of [%s] failed.", _pszFileName );
    m_fd.SetFd( fd, true );
    _InitNonTty();
  }
  // Attach to an fd like STDIN - in which case you would set _fOwnFd to false.
  // The fd will 
  _l_transport_fd( int _fd, size_t _posEnd = 0, bool _fOwnFd = false )
    : m_fd( _fd, _fOwnFd )
  {
    m_fisatty = isatty( m_fd.FdGet() );
    m_fisatty ? _InitTty() : _InitNonTty( _posEnd );
  }
  void _InitTty()
  {
    m_frrFileDesBuffer.Init( m_fd.FdGet(), 0, false, numeric_limits< vtyDataPosition >::max() );
  }
  void _InitNonTty( size_t _posEnd = 0 )
  {
    bool fReadAhead = false;
    _tySizeType posInit = 0;
    size_t posEnd = 0;
    size_t stchLenRead = std::numeric_limits< size_t >::max();

    // Then stat will return meaningful info:
    struct stat statBuf;
    errno = 0;
    int iStatRtn = ::stat( m_fd.FdGet(), &statBuf );
    if ( !!iStatRtn )
    {
      Assert( -1 == iStatRtn );
      THROWNAMEDEXCEPTIONERRNO( errno, "::stat() of fd[0x%x] failed.", m_fd.FdGet() );
    }
    // We will support any fd type here but we will only use read ahead on regular files.
    fReadAhead = !!S_ISREG( statBuf.st_mode );
    if ( fReadAhead )
    {
      errno = 0;
      off_t nbySeekCur = ::lseek( m_fd.FdGet(), 0, SEEK_CUR );
      errno = 0;
      if ( nbySeekCur < 0 )
      {
        Assert( -1 == nbySeekCur );
        THROWNAMEDEXCEPTIONERRNO( errno, "::seek() of fd[0x%x] failed.", m_fd.FdGet() );
      }
      VerifyThrowSz( !( nbySeekCur % sizeof( t_TyChar ) ), "Current offset in file is not a multiple of a character byte length." );
      posInit = nbySeekCur / sizeof( t_TyChar );
      // Validate any ending position given to us, also find the end regardless:
      errno = 0;
      off_t nbySeekEnd = ::lseek( m_fd.FdGet(), 0, SEEK_END );
      if ( nbySeekEnd < 0 )
      {
        Assert( -1 == nbySeekCur );
        THROWNAMEDEXCEPTIONERRNO( errno, "::seek() of fd[0x%x] failed.", m_fd.FdGet() );
      }
      posEnd = nbySeekEnd / sizeof( t_TyChar );
      VerifyThrowSz( _posEnd <= ( nbySeekEnd / sizeof( t_TyChar ) ), "Passed an _posEnd that is beyond the EOF." );
      if ( !_posEnd )
        VerifyThrowSz( !( nbySeekEnd % sizeof( t_TyChar ) ), "End of file position is not a multiple of a character byte length." );
      else
        posEnd = _posEnd;
      stchLenRead = posEnd - posInit;
    }
    m_frrFileDesBuffer.Init( m_fd.FdGet(), posInit, fReadAhead, stchLenRead );
  }

  vtyDataPosition PosCurrent() const
  {
    return m_frrFileDesBuffer.PosCurrent();
  }
  bool FAtTokenStart() const
  {
    return m_frrFileDesBuffer.PosCurrent() == m_frrFileDesBuffer.PosBase();
  }
  // Return the current character and advance the position.
  bool FGetChar( _tyChar & _rc )
  {
    return m_frrFileDesBuffer.FGetChar( _rc );
  }

  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_frrFileDesBuffer from [m_frrFileDesBuffer.m_saBuffer.IBaseElement(),_kdpEndToken).
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, unique_ptr< _TyToken > & _rupToken )
  {
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    typedef _l_token< _TyUserContext > _TyToken;
    _TyUserContext ucxt( _ruoUserObj, this );
    // This method ends the current token at _kdpEndToken - this causes some housekeeping within this object.
    m_frrFileDesBuffer.ConsumeData( _kdpEndToken, ucxt.GetTokenBuffer() );
    unique_ptr< _TyToken > upToken = make_unique< _TyToken >( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }

protected:
  // We define a "rotating buffer" that will hold a single token *no matter how large* (since this is how the algorithm works to allow for STDIN filters to work).
  typedef FdReadRotating< _TyChar > _TyFdReadRotating;
  _TyFdReadRotating m_frrFileDesBuffer{ vknchTransportFdTokenBufferSize * sizeof( t_TyChar ) };
  FdObj m_fd;
  bool m_fisatty{false};
};

template < class t_TyChar >
class _l_transport_fixedmem_ctxt
{
  typedef _l_transport_fixedmem_ctxt _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_transport_fixedmem< t_TyChar > _TyTransportFixedMem;
  typedef std::pair< const t_TyChar *, vtyDataPosition > _TyPrMemView;

  _l_transport_fixedmem_ctxt( _TyTransportFixedMem * _ptxpFixedMem, _TyPrMemView const & _rprmv )
    : m_ptxpFixedMem( _ptxpFixedMem ),
      m_prmvCurrentToken( _rprmv )
  {
  }
  _l_transport_fixedmem_ctxt() = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt && ) = default;
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt && ) = default;

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
    : m_prmvFull( _pcBase, _nLen ),
      m_prmvCurrentToken( _pcBase, 0 )
  {
  }
  
  void AssertValid() const
  {
#ifndef NDEBUG
    Assert( m_prmvCurrentToken.first >= m_prmvFull.first );
    Assert( ( m_prmvCurrentToken.first + m_prmvCurrentToken.second ) <= ( m_prmvFull.first + m_prmvFull.second ) );
#endif //!NDEBUG
  }

  vtyDataPosition PosCurrent() const
  {
    return ( m_prmvCurrentToken.first - m_prmvFull.first ) + m_prmvCurrentToken.second;
  }
  bool FAtTokenStart() const
  {
    return !m_prmvCurrentToken.second;
  }
  // Return the current character and advance the position.
  bool FGetChar( _tyChar & _rc )
  {
    if ( _FAtEnd() )
      return false;
    _rc = m_prmvCurrentToken.first[ m_prmvCurrentToken.second++ ];
    return true;
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_prmvCurrentToken from [m_posTokenStart,_kdpEndToken).
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, unique_ptr< _TyToken > & _rupToken )
  {
    Assert( _kdpEndToken <= m_prmvCurrentToken.first + m_prmvCurrentToken.second );
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    typedef _l_token< _TyUserContext > _TyToken;
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    Assert( nLenToken <= m_prmvCurrentToken.second );
    _TyUserContext ucxt( _ruoUserObj, this, nLenToken );
    m_prmvCurrentToken.first += nLenToken;
    m_prmvCurrentToken.second = 0;
    unique_ptr< _TyToken > upToken = make_unique< _TyToken >( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }

protected:
  bool _FAtEnd() const
  {
    return ( m_prmvCurrentToken.first + m_prmvCurrentToken.second ) == ( m_prmvFull.first + m_prmvFull.second );
  }
  vtyDataPosition _PosTokenStart() const
  {
    return m_prmvCurrentToken.first - m_prmvFull.first;
  }
  _TyPrMemView m_prmvFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  _TyPrMemView m_prmvCurrentToken; // The view for the current token's exploration.
};

template < class t_TyChar >
class _l_transport_mapped_ctxt : protected _l_transport_fixedmem_ctxt< t_TyChar >
{
  typedef _l_transport_mapped_ctxt _TyThis;
  typedef _l_transport_mapped_ctxt< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyPrMemView;
  typedef _l_transport_mapped< t_TyChar > _TyTransportMapped;

  _l_transport_mapped_ctxt( _TyTransportMapped * _ptxpMapped, _TyPrMemView const & _rprmv )
    : _TyBase( _ptxpMapped, _rprmv )
  {
  }
  _l_transport_mapped_ctxt() = default;
  _l_transport_mapped_ctxt( _l_transport_mapped_ctxt const & ) = default;
  _l_transport_mapped_ctxt( _l_transport_mapped_ctxt && ) = default;
  _l_transport_mapped_ctxt & operator =( _l_transport_mapped_ctxt const & ) = default;
  _l_transport_mapped_ctxt & operator =( _l_transport_mapped_ctxt && ) = default;

  _TyTransportMapped * PGetTransport() const
  {
    return static_cast< _TyTransportMapped * >( m_ptxpFixedMem );
  }
  using _TyBase::RPrmvCurrentToken;
  using _TyBase::RPrmvFull;
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
  typedef typename _TyTransportCtxt::_TyPrMemView _TyPrMemView;

  ~_l_transport_mapped()
  {
    if ( m_prmvFull.first != (const _TyChar*)MAP_FAILED )
    {
      int iRet = ::munmap( m_prmvFull.first, m_prmvFull.second );
      Assert( !iRet ); // not much to do about this...could log.
    }
  }
  // We are keeping the fd open for now for diagnostic purposes, etc. It would be referenced in the background
  //  by the mapping and remaing open but we wouldn't know what the descriptor value was.
  _l_transport_mapped( const char * _pszFileName )
    : _TyBase( (const _TyChar*)MAP_FAILED, 0 )// in case of throw so we don't call munmap with some random number in ~_l_transport_mapped().
  {
    errno = 0;
    int fd = open( _pszFileName, O_RDONLY );
    if ( -1 == fd )
      THROWNAMEDEXCEPTIONERRNO( errno, "Open of [%s] failed.", _pszFileName );
    m_fd.SetFd( fd, true );

    struct stat statBuf;
    errno = 0;
    int iStatRtn = ::stat( m_fd.FdGet(), &statBuf );
    if ( !!iStatRtn )
    {
      Assert( -1 == iStatRtn );
      THROWNAMEDEXCEPTIONERRNO( errno, "::stat() of fd[0x%x] failed.", m_fd.FdGet() );
    }
    VerifyThrowSz( !!S_ISREG( statBuf.st_mode ), "Can only map a regular file, _pszFileName[%s].", _pszFileName );
    errno = 0;
    m_prmvFull.second = statBuf.st_size;
    m_prmvFull.first = (const _TyChar*)mmap( 0, m_prmvFull.second, PROT_READ, MAP_SHARED | MAP_NORESERVE, m_fd.FdGet(), 0 );
    if ( m_prmvFull.first == (const _TyChar*)MAP_FAILED )
        THROWNAMEDEXCEPTIONERRNO( errno, "mmap() failed for _pszFileName[%s] st_size[%lu].", m_fd.FdGet(), statBuf.st_size );
    m_prmvCurrentToken.first = m_prmvFull.first;
    Assert( !m_prmvCurrentToken.second );
  }
  _l_transport_mapped() = delete;
  _l_transport_mapped( _l_transport_mapped const & ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis const & ) = delete;
  _l_transport_mapped( _l_transport_mapped && ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis && ) = delete;
  
  void AssertValid() const
  {
#ifndef NDEBUG
    _TyBase::AssertValid();
#endif //!NDEBUG
  }

  using _TyBase::PosCurrent;
  using _TyBase::FAtTokenStart;
  using _TyBase::FGetChar;
  using _TyBase::void GetPToken;
  
protected:
  using _TyBase::m_prmvFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  using _TyBase::m_prmvCurrentToken; // The view for the current token's exploration.
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
  bool FGetChar( _tyChar & _rc )
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
    m_opttpImpl.GetPToken( _paobCurToken, _kdpEndToken, std::move( value ), m_uoUserObj, _rupToken );
  }

protected:
  typedef optional< t_TyTransport > _TyOptTransport;
  _TyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  _TyUserObj m_uoUserObj; // This is the one copy of the user object that is referenced in each _TyUserContext object. This allows the user object to maintain stateful information is that is desireable.
};

__LEXOBJ_END_NAMESPACE
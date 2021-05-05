#pragma once

// _l_transport.h
// Transport objects for lexical analyzer.
// dbien : 26DEC2020 - moved from _l_strm.h cuz it was getting biggish.

#include <variant>
#include "_assert.h"
#include "fdrotbuf.h"
#include "_l_buf.h"
#include "_l_user.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_transport_base: base for all transports.
// Templatize by the character type contained in the file.
template < class t_TyChar >
class _l_transport_base
{
  typedef _l_transport_base _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_data<> _TyData;
};

static const size_t vknchTransportFdTokenBufferSize = 256;
static_assert( sizeof( vtySeekOffset ) == sizeof( vtyDataPosition ) );

// _l_transport_backed_ctxt:
// This is the context that is contained in an _l_token allowing the also contained _l_value to exist on its own entirely.
// Text translation specific to a given parser is built into the most derived class of this object.
template < class t_TyChar >
class _l_transport_backed_ctxt
{
  typedef _l_transport_backed_ctxt _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_backing_buf< t_TyChar > _TyBuffer;
  typedef _l_data<> _TyData;

  _l_transport_backed_ctxt( vtyDataPosition _posTokenStart, _TyBuffer && _rrbufTokenData )
    : m_posTokenStart( _posTokenStart ),
      m_bufTokenData( std::move( _rrbufTokenData ) )
  {
  }
  _l_transport_backed_ctxt( vtyDataPosition _posTokenStart )
    : m_posTokenStart( _posTokenStart )
  {
  }
  ~_l_transport_backed_ctxt() = default;
  _l_transport_backed_ctxt() = default;
  _l_transport_backed_ctxt( _l_transport_backed_ctxt const & ) = default;
  _l_transport_backed_ctxt & operator =( _l_transport_backed_ctxt const & ) = default;
  _l_transport_backed_ctxt( _l_transport_backed_ctxt && _rr )
    : m_bufTokenData( std::move( _rr.m_bufTokenData ) )
  {
    std::swap( m_posTokenStart, _rr.m_posTokenStart );
  }
  _l_transport_backed_ctxt & operator =( _l_transport_backed_ctxt && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
     std::swap( m_posTokenStart, _r.m_posTokenStart );
     m_bufTokenData.swap( _r.m_bufTokenData );
  }
  // A backed context can be constructed from any other context since the memory can be copied into the backing.
  template < class t_TyTransportCtxtOther >
  _l_transport_backed_ctxt( t_TyTransportCtxtOther const & _rOther )
    requires( TAreSameSizeTypes_v< _TyChar, typename t_TyTransportCtxtOther::_TyChar > )
    : m_bufTokenData( _rOther.PCBufferBegin(), _rOther.NLenToken() ),
      m_posTokenStart( _rOther.PosTokenStart() )  
  {
  }
  // A backed context can be assigned to any other context since the memory can be copied into the backing.
  template < class t_TyTransportCtxtOther >
  _TyThis & operator = ( t_TyTransportCtxtOther const & _rOther )
    requires( TAreSameSizeTypes_v< _TyChar, typename t_TyTransportCtxtOther::_TyChar > )
  {
    m_bufTokenData.SetBuffer( _rOther.PCBufferBegin(), _rOther.NLenToken() );
    m_posTokenStart = _rOther.PosTokenStart(); // this can't throw.
    return *this;
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( ( m_posTokenStart == (numeric_limits< vtyDataPosition >::max)() ) == m_bufTokenData.FIsNull() );
#endif //ASSERTSENABLED    
  }
  bool FIsNull() const
  {
    AssertValid();
    return ( m_posTokenStart == (numeric_limits< vtyDataPosition >::max)() );
  }
  vtyDataPosition PosTokenStart() const
  {
    return m_posTokenStart;
  }
  size_t NLenToken() const
  {
    return m_bufTokenData.length();
  }
  void GetTokenDataRange( _l_data_range & _rdr ) const
  {
    _rdr.m_posBegin = m_posTokenStart;
    _rdr.m_posEnd = m_posTokenStart + m_bufTokenData.length();
  }
  const _TyBuffer & GetTokenBuffer() const
  {
    return m_bufTokenData;
  }
  // Only support a non-const GetTokenBufffer() in _l_transport_backed_ctxt because it allows us to rearrange endianness.
  _TyBuffer & GetTokenBuffer()
  {
    return m_bufTokenData;
  }
  const _TyChar * PCBufferBegin() const
  {
    return GetTokenBuffer().begin();
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_range const & _rdr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    _AssertValidRange( _rdr.begin(), _rdr.end() );
    GetTokenBuffer().GetStringView( _rsv, _rdr.begin() - PosTokenStart(), _rdr.end() - PosTokenStart() );
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_typed_range const & _rdtr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    return GetStringView( _rsv, _rdtr.GetRangeBase() );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        _AssertValidRange( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end() );
      }
      else
      {
        _rdt.GetSegArrayDataRanges().ApplyContiguous( 0, _rdt.GetSegArrayDataRanges().NElements(), 
          [this]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
          {
            for( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
            {
              if ( !_pdtrBegin->FIsNull() )
                _AssertValidRange( _pdtrBegin->begin(), _pdtrBegin->end() );
            }
          }
        );
      }
    }
#endif //ASSERTSENABLED  
  }
protected:
  void _AssertValidRange( vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
  {
#if ASSERTSENABLED
    Assert( _posEnd >= _posBegin );
    Assert( _posBegin >= m_posTokenStart );
    Assert( ( vkdpNullDataPosition == _posEnd ) || ( _posEnd <= m_posTokenStart + m_bufTokenData.length() ) );
#endif //ASSERTSENABLED  
  }
  vtyDataPosition m_posTokenStart{ (numeric_limits< vtyDataPosition >::max)() };
  _TyBuffer m_bufTokenData;
};

// _l_transport_file:
// Transport using a file.
template < class t_TyChar, class t_TyBoolSwitchEndian >
class _l_transport_file : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_file _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using typename _TyBase::_TyChar;
  typedef t_TyBoolSwitchEndian _TyBoolSwitchEndian;
  using typename _TyBase::_TyData;
  static constexpr bool s_kfSwitchEndian = _TyBoolSwitchEndian::value;
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxt;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  ~_l_transport_file() = default;
  _l_transport_file() = delete;
  _l_transport_file( const _l_transport_file & ) = delete;
  _l_transport_file & operator =( _l_transport_file const & ) = delete;
  _l_transport_file( _l_transport_file && ) = default;
  _l_transport_file & operator =( _l_transport_file && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_frrFileDesBuffer.swap( _r.m_frrFileDesBuffer );
    m_file.swap( _r.m_file );
    std::swap( m_fisatty, _r.m_fisatty );
  }

  _l_transport_file( const char * _pszFileName )
  {
    PrepareErrNo();
    vtyFileHandle hFile = OpenReadOnlyFile( _pszFileName );
    if ( vkhInvalidFileHandle == hFile )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "Open of [%s] failed.", _pszFileName );
    m_file.SetHFile( hFile, true );
    _InitNonTty();
  }
  // We must read starting at the current seek position of the file. This skips any BOM - or whatever the caller wants to skip, etc.
  _l_transport_file( FileObj & _rfoFile )
  {
    VerifyThrowSz( _rfoFile.FIsOpen(), "Caller should pass an open file..." );
    m_file = std::move( _rfoFile );
    _InitNonTty();
  }
  // Attach to an hFile like STDIN - in which case you would set _fOwnFd to false.
  _l_transport_file( vtyFileHandle _hFile, size_t _posEnd /*= 0*/, bool _fOwnFd = false )
    : m_file( _hFile, _fOwnFd )
  {
    m_fisatty = FIsConsoleFileHandle( m_file.HFileGet() );
    m_fisatty ? _InitTty() : _InitNonTty( _posEnd );
  }
  static EFileCharacterEncoding GetSupportedCharacterEncoding()
  {
    return GetCharacterEncoding< _TyChar, _TyBoolSwitchEndian >();
  }
  bool FDependentTransportContexts() const
  {
    return false;
  }
  vtyDataPosition PosTokenStart() const
  {
    return m_frrFileDesBuffer.PosBase();
  }
  vtyDataPosition PosCurrent() const
  {
    return m_frrFileDesBuffer.PosCurrent();
  }
  bool FAtTokenStart() const
  {
    return m_frrFileDesBuffer.PosCurrent() == m_frrFileDesBuffer.PosBase();
  }
  void ResetToTokenStart()
  {
    m_frrFileDesBuffer.ResetPositionToBase();
  }
  // Return the current character and advance the position.
  bool FGetChar( _TyChar & _rc )
  {
    // Any endian switching is done within the buffer.
    return m_frrFileDesBuffer.FGetChar( _rc );
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_frrFileDesBuffer from [m_frrFileDesBuffer.m_saBuffer.IBaseElement(),_kdpEndToken).
  template < class t_TyToken, class t_TyValue, class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken,
                  t_TyValue & _rvalue, t_TyUserObj & _ruoUserObj,
                  unique_ptr< t_TyToken > & _rupToken )
  {
    typedef typename t_TyToken::_TyValue _TyValue;
    static_assert( is_same_v< t_TyValue, _TyValue > );
    typedef typename t_TyToken::_TyUserContext _TyUserContext;
    typedef typename _TyUserContext::_TyUserObj _TyUserObj;
    static_assert( is_same_v< t_TyUserObj, _TyUserObj > );
    Assert( _kdpEndToken >= m_frrFileDesBuffer.PosBase() );
    vtyDataPosition nLenToken = ( _kdpEndToken - m_frrFileDesBuffer.PosBase() );
    typedef typename _TyTransportCtxt::_TyBuffer _TyBuffer;
    _TyUserContext ucxt( _ruoUserObj, m_frrFileDesBuffer.PosBase(), _TyBuffer( nLenToken ) );
    // This method ends the current token at _kdpEndToken - this causes some housekeeping within this object.
    m_frrFileDesBuffer.ConsumeData( ucxt.GetTokenBuffer().begin(), ucxt.GetTokenBuffer().length() );
    unique_ptr< t_TyToken > upToken = make_unique< t_TyToken >( std::move( ucxt ), std::move( _rvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }
  _TyTransportCtxt CtxtEatCurrentToken( const vtyDataPosition _kdpEndToken )
  {
    Assert( _kdpEndToken >= m_frrFileDesBuffer.PosBase() );
    vtyDataPosition nLenToken = ( _kdpEndToken - m_frrFileDesBuffer.PosBase() );
    typedef typename _TyTransportCtxt::_TyBuffer _TyBuffer;
    _TyTransportCtxt tcxt( m_frrFileDesBuffer.PosBase(), _TyBuffer( nLenToken ) );
    m_frrFileDesBuffer.ConsumeData( tcxt.GetTokenBuffer().begin(), tcxt.GetTokenBuffer().length() );
    return tcxt;
  }
  void DiscardData( const vtyDataPosition _kdpEndToken )
  {
    m_frrFileDesBuffer.DiscardData( _kdpEndToken );
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
  {
    m_frrFileDesBuffer.GetCurrentString( _rstr );
  }
  bool FSpanChars( const _TyData & _rdt, const _TyChar * _pszCharSet ) const
  {
    Assert( _rdt.FContainsSingleDataRange() );
    AssertValidDataRange( _rdt );
    return m_frrFileDesBuffer.FSpanChars( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end(), _pszCharSet );
  }
  bool FMatchChars( const _TyData & _rdt, const _TyChar * _pszMatch ) const
  {
    Assert( _rdt.FContainsSingleDataRange() );
    Assert( StrNLen( _pszMatch ) == _rdt.DataRangeGetSingle().length() ); // by the time we get to here...
    AssertValidDataRange( _rdt );
    return m_frrFileDesBuffer.FMatchChars( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end(), _pszMatch );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        m_frrFileDesBuffer.AssertValidRange( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end() );
      }
      else
      {
        _rdt.GetSegArrayDataRanges().ApplyContiguous( 0, _rdt.GetSegArrayDataRanges().NElements(), 
          [this]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
          {
            for( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
            {
              if ( !_pdtrBegin->FIsNull() )
                m_frrFileDesBuffer.AssertValidRange( _pdtrBegin->begin(), _pdtrBegin->end() );
            }
          }
        );
      }
    }
#endif //ASSERTSENABLED  
  }
protected:
  void _InitTty()
  {
    m_frrFileDesBuffer.Init( m_file.HFileGet(), 0, false, (numeric_limits< vtyDataPosition >::max)() );
  }
  void _InitNonTty( size_t _posEnd = 0 )
  {
    bool fReadAhead = false;
    size_t posInit = 0;
    size_t posEnd = 0;
    size_t stchLenRead = (std::numeric_limits<size_t>::max)();

    // Then stat will return meaningful info:
    vtyHandleAttr attrHandle;
    int iGetAttrRtn = GetHandleAttrs( m_file.HFileGet(), attrHandle );
    if ( !!iGetAttrRtn )
    {
      Assert( -1 == iGetAttrRtn );
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "GetHandleAttrs() of hFile[0x%lx] failed.", (uint64_t)(m_file.HFileGet()) );
    }
    // We will support any hFile type here but we will only use read ahead on regular files.
    fReadAhead = FIsRegularFile_HandleAttr( attrHandle );
    if ( fReadAhead )
    {
      vtySeekOffset nbySeekCur;
      int iSeekResult = FileSeek( m_file.HFileGet(), 0, vkSeekCur, &nbySeekCur );
      if ( !!iSeekResult )
      {
        Assert( -1 == iSeekResult );
        THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "::seek() of hFile[0x%lx] failed.", (uint64_t)(m_file.HFileGet()) );
      }
      VerifyThrowSz( !( nbySeekCur % sizeof( _TyChar ) ), "Current offset in file is not a multiple of a character byte length." );
      posInit = nbySeekCur / sizeof( _TyChar );
      // Validate any ending position given to us, also find the end regardless:
      vtySeekOffset nbySeekEnd;
      iSeekResult = FileSeek( m_file.HFileGet(), 0, vkSeekEnd, &nbySeekEnd );
      if ( !!iSeekResult )
      {
        Assert( -1 == iSeekResult );
        THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "::seek() of hFile[0x%lx] failed.", (uint64_t)(m_file.HFileGet()) );
      }
      posEnd = nbySeekEnd / sizeof( _TyChar );
      VerifyThrowSz( _posEnd <= ( nbySeekEnd / sizeof( _TyChar ) ), "Passed an _posEnd that is beyond the EOF." );
      if ( !_posEnd )
        VerifyThrowSz( !( nbySeekEnd % sizeof( _TyChar ) ), "End of file position is not a multiple of a character byte length." );
      else
        posEnd = _posEnd;
      stchLenRead = posEnd - posInit;
      // Reset the seek to where we were at the beginning.
      (void)NFileSeekAndThrow( m_file.HFileGet(), nbySeekCur, vkSeekBegin );
    }
    // The initial position is always 0 - regardless of where we actually started in the file.
    // We could do it otherwise which would allow easier debugging but... this is how we are doing it now.
    m_frrFileDesBuffer.Init( m_file.HFileGet(), 0, fReadAhead, stchLenRead );
  }
  // We define a "rotating buffer" that will hold a single token *no matter how large* (since this is how the algorithm works to allow for STDIN filters to work).
  typedef FdReadRotating< _TyChar, s_kfSwitchEndian > _TyFdReadRotating;
  _TyFdReadRotating m_frrFileDesBuffer{ vknchTransportFdTokenBufferSize * sizeof( _TyChar ) };
  FileObj m_file;
  bool m_fisatty{false};
};

template < class t_TyChar >
class _l_transport_fixedmem_ctxt
{
  typedef _l_transport_fixedmem_ctxt _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_transport_fixedmem< t_TyChar > _TyTransportFixedMem;
  typedef _l_fixed_buf< _TyChar > _TyBuffer;
  typedef _l_data<> _TyData;

  _l_transport_fixedmem_ctxt( vtyDataPosition _posTokenStart, _TyBuffer const & _bufTokenData )
    : m_posTokenStart( _posTokenStart ),
      m_bufTokenData( _bufTokenData )
  {
  }
  _l_transport_fixedmem_ctxt() = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt && _rr )
    : m_bufTokenData( std::move( _rr.m_bufTokenData ) )
  {
    std::swap( m_posTokenStart, _rr.m_posTokenStart );
  }
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
     std::swap( m_posTokenStart, _r.m_posTokenStart );
     m_bufTokenData.swap( _r.m_bufTokenData );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( ( m_posTokenStart == (numeric_limits< vtyDataPosition >::max)() ) == m_bufTokenData.FIsNull() );
#endif //ASSERTSENABLED    
  }
  bool FIsNull() const
  {
    AssertValid();
    return ( m_posTokenStart == (numeric_limits< vtyDataPosition >::max)() );
  }
  vtyDataPosition PosTokenStart() const
  {
    return m_posTokenStart;
  }
  size_t NLenToken() const
  {
    return m_bufTokenData.length();
  }
  void GetTokenDataRange( _l_data_range & _rdr ) const
  {
    _rdr.m_posBegin = m_posTokenStart;
    _rdr.m_posEnd = m_posTokenStart + m_bufTokenData.length();
  }
  _TyBuffer const & GetTokenBuffer() const
  {
    return m_bufTokenData;
  }
  const _TyChar * PCBufferBegin() const
  {
    return GetTokenBuffer().begin();
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_range const & _rdr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    _AssertValidRange( _rdr.begin(), _rdr.end() );
    GetTokenBuffer().GetStringView( _rsv, _rdr.begin() - PosTokenStart(), _rdr.end() - PosTokenStart() );
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_typed_range const & _rdtr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    return GetStringView( _rsv, _rdtr.GetRangeBase() );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        _AssertValidRange( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end() );
      }
      else
      {
        _rdt.GetSegArrayDataRanges().ApplyContiguous( 0, _rdt.GetSegArrayDataRanges().NElements(), 
          [this]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
          {
            for( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
            {
              if ( !_pdtrBegin->FIsNull() )
                _AssertValidRange( _pdtrBegin->begin(), _pdtrBegin->end() );
            }
          }
        );
      }
    }
#endif //ASSERTSENABLED  
  }
protected:
  void _AssertValidRange( vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
  {
#if ASSERTSENABLED
    Assert( _posEnd >= _posBegin );
    Assert( _posBegin >= m_posTokenStart );
    Assert( ( vkdpNullDataPosition == _posEnd ) || ( _posEnd <= m_posTokenStart + m_bufTokenData.length() ) );
#endif //ASSERTSENABLED  
  }
  vtyDataPosition m_posTokenStart{ (numeric_limits< vtyDataPosition >::max)() };
  _TyBuffer m_bufTokenData;
};

// _l_transport_fixedmem:
// Transport that uses a piece of memory as an input stream.
template < class t_TyChar, class t_TyBoolSwitchEndian >
class _l_transport_fixedmem : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_fixedmem _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using typename _TyBase::_TyChar;
  typedef t_TyBoolSwitchEndian _TyBoolSwitchEndian;
  using typename _TyBase::_TyData;
  static constexpr bool s_kfSwitchEndian = _TyBoolSwitchEndian::value;
  // A transport that converts its input in any way must use a backed context when returning a token:
  using _TyTransportCtxt = typename std::conditional< s_kfSwitchEndian, _l_transport_backed_ctxt< _TyChar >, _l_transport_fixedmem_ctxt< _TyChar > >::type;
  friend _TyTransportCtxt;
  // However we need to still read from the fixed buffer and maintain those in fixed format within the impl:
  using _TyTransportCtxtFixed = _l_transport_fixedmem_ctxt< _TyChar >;
  typedef typename _TyTransportCtxtFixed::_TyBuffer _TyBufferFixed;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  ~_l_transport_fixedmem() = default;
  _l_transport_fixedmem() = default;
  _l_transport_fixedmem( _l_transport_fixedmem const & _r ) = default;
  _TyThis & operator = ( _TyThis const & _r ) = default;
  _l_transport_fixedmem( _l_transport_fixedmem && _rr ) = default;
  _TyThis & operator = ( _TyThis && _r ) = default;
  _l_transport_fixedmem( const _TyChar * _pcBase, vtyDataPosition _nLenChars )
    : m_bufFull( _pcBase, _nLenChars ),
      m_bufCurrentToken( _pcBase, 0 )
  {
  }
  void swap( _TyThis & _r )
  {
    m_bufFull.swap( _r.m_bufFull );
    m_bufCurrentToken.swap( _r.m_bufCurrentToken );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( m_bufCurrentToken.first >= m_bufFull.first );
    Assert( ( m_bufCurrentToken.first + m_bufCurrentToken.second ) <= ( m_bufFull.first + m_bufFull.second ) );
#endif //ASSERTSENABLED
  }
  static EFileCharacterEncoding GetSupportedCharacterEncoding()
  {
    return GetCharacterEncoding< _TyChar, _TyBoolSwitchEndian >();
  }
  bool FDependentTransportContexts() const
  {
    return false;
  }
  vtyDataPosition PosTokenStart() const
  {
    return ( m_bufCurrentToken.begin() - m_bufFull.begin() );
  }
  vtyDataPosition PosCurrent() const
  {
    return ( m_bufCurrentToken.begin() - m_bufFull.begin() ) + m_bufCurrentToken.length();
  }
  bool FAtTokenStart() const
  {
    return !m_bufCurrentToken.length();
  }
  void ResetToTokenStart()
  {
    m_bufCurrentToken.RLength() = 0;
  }
  // Return the current character and advance the position.
  bool FGetChar( _TyChar & _rc )
  {
    if ( _FAtEnd() )
      return false;
    if ( s_kfSwitchEndian )
      SwitchEndian( _rc = m_bufCurrentToken.begin()[ m_bufCurrentToken.RLength()++ ] );
    else
      _rc = m_bufCurrentToken.begin()[ m_bufCurrentToken.RLength()++ ];
    return true;
  }

  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_bufCurrentToken from [m_posTokenStart,_kdpEndToken).
  template < class t_TyToken, class t_TyValue, class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase* _paobCurToken, const vtyDataPosition _kdpEndToken,
                  t_TyValue & _rvalue, t_TyUserObj& _ruoUserObj, unique_ptr< t_TyToken >& _rupToken)
  {
    typedef typename t_TyToken::_TyValue _TyValue;
    static_assert( is_same_v< t_TyValue, _TyValue > );
    typedef typename t_TyToken::_TyUserContext _TyUserContext;
    typedef typename _TyUserContext::_TyUserObj _TyUserObj;
    static_assert( is_same_v< t_TyUserObj, _TyUserObj > );
    Assert( _kdpEndToken >= _PosTokenStart() );
    Assert( _kdpEndToken <= _PosTokenEnd() );
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    Assert( nLenToken <= m_bufCurrentToken.length() );
    typedef typename _TyTransportCtxt::_TyBuffer _TyBuffer;
    _TyUserContext ucxt( _ruoUserObj, _PosTokenStart(), _TyBuffer( m_bufCurrentToken.begin(), nLenToken ) );
    if ( s_kfSwitchEndian )
      SwitchEndian( ucxt.GetTokenBuffer().begin(), ucxt.GetTokenBuffer().end() );
    m_bufCurrentToken.RCharP() += nLenToken;
    m_bufCurrentToken.RLength() = 0;
    unique_ptr< t_TyToken > upToken = make_unique< t_TyToken >( std::move( ucxt ), std::move( _rvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }
  _TyTransportCtxt CtxtEatCurrentToken( const vtyDataPosition _kdpEndToken )
  {
    Assert( _kdpEndToken >= _PosTokenStart() );
    Assert( _kdpEndToken <= _PosTokenEnd() );
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    typedef typename _TyTransportCtxt::_TyBuffer _TyBuffer;
    _TyBuffer bufToken( m_bufCurrentToken.begin(), nLenToken );
    if ( s_kfSwitchEndian )
      SwitchEndian( bufToken.begin(), bufToken.end() );
    vtyDataPosition posTokenStart = _PosTokenStart();
    m_bufCurrentToken.RCharP() += nLenToken;
    m_bufCurrentToken.RLength() = 0;
    return _TyTransportCtxt( posTokenStart, std::move( bufToken ) );
  }
  void DiscardData( const vtyDataPosition _kdpEndToken )
  {
    Assert( _kdpEndToken >= _PosTokenStart() );
    Assert( _kdpEndToken <= _PosTokenEnd() );
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    m_bufCurrentToken.RCharP() += nLenToken;
    m_bufCurrentToken.RLength() = 0;
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
    requires( sizeof( typename t_TyString::value_type ) == sizeof( _TyChar ) )
  {
    _rstr.assign( (typename t_TyString::value_type const *)m_bufCurrentToken.begin(), m_bufCurrentToken.length() );
    if ( s_kfSwitchEndian )
      SwitchEndian( &_rstr[0], _rstr.length() );
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
    requires( !s_kfSwitchEndian && ( sizeof( typename t_TyString::value_type ) != sizeof( _TyChar ) ) )
  {
    ConvertString( _rstr, m_bufCurrentToken.begin(), m_bufCurrentToken.length() );
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
    requires( s_kfSwitchEndian && ( sizeof( typename t_TyString::value_type ) != sizeof( _TyChar ) ) )
  {
    // We must convert the string after switching the endianness or things won't work...
    basic_string< _TyChar > strTemp;
    static size_t knchMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyChar );
    _TyChar * pcBuf;
    if ( m_bufCurrentToken.length() > knchMaxAllocaSize )
    {
      strTemp.resize( m_bufCurrentToken.length() );
      pcBuf = &strTemp[0];
    }
    else
      pcBuf = (_TyChar*)alloca( m_bufCurrentToken.length() * sizeof( _TyChar ) );
    memcpy( pcBuf, m_bufCurrentToken.begin(), m_bufCurrentToken.length() * sizeof( _TyChar ) );
    SwitchEndian( pcBuf, m_bufCurrentToken.length() );
    ConvertString( _rstr, pcBuf, m_bufCurrentToken.length() );
  }
  bool FSpanChars( const _TyData & _rdt, const _TyChar * _pszCharSet ) const
    requires( !s_kfSwitchEndian )
  {
    Assert( _rdt.FContainsSingleDataRange() );
    AssertValidDataRange( _rdt );
    vtyDataPosition posBegin = _rdt.DataRangeGetSingle().begin() - _PosTokenStart();
    return _rdt.DataRangeGetSingle().length() == StrSpn( m_bufCurrentToken.RCharP() + posBegin, _rdt.DataRangeGetSingle().length(), _pszCharSet );
  }
  bool FSpanChars( const _TyData & _rdt, const _TyChar * _pszCharSet ) const
    requires( s_kfSwitchEndian )
  {
    Assert( _rdt.FContainsSingleDataRange() );
    AssertValidDataRange( _rdt );
    // Provide an endian-switched _pszCharSet to StrSpn() because it doesn't care and it's easier.
    size_t stLenCharSet = StrNLen( _pszCharSet );
    _TyChar * pcBufCharSet = (_TyChar*)alloca( ( stLenCharSet + 1 ) * sizeof( _TyChar ) );
    memcpy( pcBufCharSet, _pszCharSet, ( stLenCharSet + 1 ) * sizeof( _TyChar ) );
    SwitchEndian( pcBufCharSet, stLenCharSet );
    vtyDataPosition posBegin = _rdt.DataRangeGetSingle().begin() - _PosTokenStart();
    return _rdt.DataRangeGetSingle().length() == StrSpn( m_bufCurrentToken.RCharP() + posBegin, _rdt.DataRangeGetSingle().length(), pcBufCharSet );
  }
  bool FMatchChars( const _TyData & _rdt, const _TyChar * _pszMatch ) const
    requires( !s_kfSwitchEndian )
  {
    Assert( _rdt.FContainsSingleDataRange() );
    AssertValidDataRange( _rdt );
    vtyDataPosition posBegin = _rdt.DataRangeGetSingle().begin() - _PosTokenStart();
    vtyDataPosition posEnd = _rdt.DataRangeGetSingle().end() - _PosTokenStart();
    const _TyChar * pcTokBegin = m_bufCurrentToken.RCharP() + posBegin;
    const _TyChar * pcTokEnd = m_bufCurrentToken.RCharP() + posEnd;
    return pcTokEnd == mismatch( pcTokBegin, pcTokEnd, _pszMatch ).first;
  }
  bool FMatchChars( const _TyData & _rdt, const _TyChar * _pszMatch ) const
    requires( s_kfSwitchEndian )
  {
    // Provide an endian-switched _pszMatch to mismatch() because...
    Assert( _rdt.FContainsSingleDataRange() );
    AssertValidDataRange( _rdt );
    size_t stLenMatch = _rdt.DataRangeGetSingle().length();
    _TyChar * pcBufMatch = (_TyChar*)alloca( stLenMatch * sizeof( _TyChar ) );
    memcpy( pcBufMatch, _pszMatch, stLenMatch * sizeof( _TyChar ) );
    SwitchEndian( pcBufMatch, stLenMatch );
    vtyDataPosition posBegin = _rdt.DataRangeGetSingle().begin() - _PosTokenStart();
    vtyDataPosition posEnd = _rdt.DataRangeGetSingle().end() - _PosTokenStart();
    const _TyChar * pcTokBegin = m_bufCurrentToken.RCharP() + posBegin;
    const _TyChar * pcTokEnd = m_bufCurrentToken.RCharP() + posEnd;
    return pcTokEnd == mismatch( pcTokBegin, pcTokEnd, pcBufMatch ).first;
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        _AssertValidRange( _rdt.DataRangeGetSingle().begin(), _rdt.DataRangeGetSingle().end() );
      }
      else
      {
        _rdt.GetSegArrayDataRanges().ApplyContiguous( 0, _rdt.GetSegArrayDataRanges().NElements(), 
          [this]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
          {
            for( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
            {
              if ( !_pdtrBegin->FIsNull() )
                _AssertValidRange( _pdtrBegin->begin(), _pdtrBegin->end() );
            }
          }
        );
      }
    }
#endif //ASSERTSENABLED  
  }
protected:
  void _AssertValidRange( vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
  {
#if ASSERTSENABLED  
    Assert( _posEnd >= _posBegin );
    Assert( _posBegin >= _PosTokenStart() );
    Assert( _posEnd <= _PosTokenEnd() );
#endif //ASSERTSENABLED  
  }
  bool _FAtEnd() const
  {
    return m_bufCurrentToken.end() == m_bufFull.end();
  }
  vtyDataPosition _PosTokenStart() const
  {
    return m_bufCurrentToken.begin() - m_bufFull.begin();
  }
  vtyDataPosition _PosTokenEnd() const
  {
    return _PosTokenStart() + m_bufCurrentToken.length();
  }
  _TyBufferFixed m_bufFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  _TyBufferFixed m_bufCurrentToken; // The view for the current token's exploration.
};

// _l_transport_mapped
// Transport that uses mapped memory.
template < class t_TyChar, class t_TyBoolSwitchEndian >
class _l_transport_mapped : protected _l_transport_fixedmem< t_TyChar, t_TyBoolSwitchEndian >
{
  typedef _l_transport_mapped _TyThis;
  typedef _l_transport_fixedmem< t_TyChar, t_TyBoolSwitchEndian > _TyBase;
public:
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyBoolSwitchEndian;
  using typename _TyBase::_TyData;
  using typename _TyBase::_TyTransportCtxt;
  using _TyBase::s_kfSwitchEndian;

  _l_transport_mapped() = default;
  _l_transport_mapped( _l_transport_mapped const & _r ) = delete;
  _l_transport_mapped& operator =( _l_transport_mapped const & _r ) = delete;
  _l_transport_mapped( _l_transport_mapped && _rr ) = default;
  _l_transport_mapped& operator =( _l_transport_mapped && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
    m_fmoMappedFile.swap( _r.m_fmoMappedFile );
  }
  ~_l_transport_mapped()
  {
  }
  // We are keeping the hFile open for now for diagnostic purposes, etc. It would be referenced in the background
  //  by the mapping and remaing open but we wouldn't know what the descriptor value was.
  _l_transport_mapped( const char * _pszFileName )
    : _TyBase( (const _TyChar*)vkpvNullMapping, 0 )// in case of throw so we don't call munmap with some random number in ~_l_transport_mapped().
  {
    FileObj foFile( OpenReadOnlyFile( _pszFileName ) );
    if ( !foFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "OpenReadOnlyFile() of [%s] failed.", _pszFileName );
    size_t stSizeMapping;
    FileMappingObj fmoFile( MapReadOnlyHandle( foFile.HFileGet(), &stSizeMapping ) );
    if ( !fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "MapReadOnlyHandle() of [%s] failed, stSizeMapping[%lu].", _pszFileName, stSizeMapping );
    VerifyThrowSz( !( stSizeMapping % sizeof( _TyChar ) ), "File size[%lu] is not an integral multiple of character size.", stSizeMapping );
    m_bufFull.RLength() = stSizeMapping / sizeof( _TyChar );
    m_bufFull.RCharP() = (const _TyChar*)fmoFile.Pv();
    m_bufCurrentToken.RCharP() = m_bufFull.begin();
    Assert( !m_bufCurrentToken.length() );
    m_fmoMappedFile.swap( fmoFile ); // We now own the mapped file.
  }
  // We must map starting at the current seek position of the file. This skips any BOM - or whatever the caller wants to skip, etc.
  // However we must store the resultant offset from the actual map point. This is because the map address is aligned to page size boundaries.
  _l_transport_mapped( FileObj & _rfoFile )
    : _TyBase( (const _TyChar*)vkpvNullMapping, 0 )// in case of throw so we don't call munmap with some random number in ~_l_transport_mapped().
  {
    VerifyThrowSz( _rfoFile.FIsOpen(), "Caller should pass an open file..." );
	  size_t stMapAtPosition = (size_t)NFileSeekAndThrow(_rfoFile.HFileGet(), 0, vkSeekCur);
    size_t stSizeMapping;
    FileMappingObj fmoFile( MapReadOnlyHandle( _rfoFile.HFileGet(), &stSizeMapping, &stMapAtPosition ) );
    if ( !fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "MapReadOnlyHandle() of handle[%lu] failed, stSizeMapping[%lu].", (size_t)_rfoFile.HFileGet(), stSizeMapping );
    size_t nbyLenMapping = stSizeMapping - stMapAtPosition;
    VerifyThrowSz( !( nbyLenMapping % sizeof( _TyChar ) ), "Mapping size[%lu] is not an integral multiple of character size.", nbyLenMapping );
    m_bufFull.RLength() = nbyLenMapping / sizeof( _TyChar );
    m_bufFull.RCharP() = (const _TyChar*)fmoFile.Pby(stMapAtPosition);
    m_bufCurrentToken.RCharP() = m_bufFull.begin();
    Assert( !m_bufCurrentToken.length() );
    m_fmoMappedFile.swap( fmoFile ); // We now own the mapped file.
  }
  static EFileCharacterEncoding GetSupportedCharacterEncoding()
  {
    return GetCharacterEncoding< _TyChar, _TyBoolSwitchEndian >();
  }
  bool FDependentTransportContexts() const
  {
    // If we are switching endian and using a backing context then we don't need to keep this around.
    // Need to keep this mapped file open to maintain the validity of the returned transport_ctxts.
    return !s_kfSwitchEndian;
  }  
  void AssertValid() const
  {
#if ASSERTSENABLED
    _TyBase::AssertValid();
#endif //ASSERTSENABLED
  }

  using _TyBase::PosCurrent;
  using _TyBase::PosTokenStart;
  using _TyBase::FAtTokenStart;
  using _TyBase::ResetToTokenStart;
  using _TyBase::FGetChar;
  using _TyBase::GetPToken;
  using _TyBase::CtxtEatCurrentToken;
  using _TyBase::DiscardData;
  using _TyBase::GetCurTokenString;
  using _TyBase::FSpanChars;
  using _TyBase::FMatchChars;
protected:
  using _TyBase::m_bufFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  using _TyBase::m_bufCurrentToken; // The view for the current token's exploration.
  FileMappingObj m_fmoMappedFile;
};

// Produce a variant that is the set of all potential transports that the user of the object may care to use.
// We provide no functionality in this class
template < class t_TyVariant >
class _l_transport_var_ctxt
{
  typedef _l_transport_var_ctxt _TyThis;
public:
  // We only want uniquely typed transport contexts here.
  typedef t_TyVariant _TyVariant;
  typedef typename variant_alternative<0,_TyVariant>::type::_TyChar _TyChar;
  typedef _l_data<> _TyData;
  // Construction by moved contained context:
  template < class t_TyTransportContext >
  _l_transport_var_ctxt( t_TyTransportContext && _rrtcxt )
    : m_var( _rrtcxt )
  {
  }
  ~ _l_transport_var_ctxt() = default;
  _l_transport_var_ctxt() = default;
  _l_transport_var_ctxt( _l_transport_var_ctxt && _rr ) noexcept = default;
  _l_transport_var_ctxt & operator=( _TyThis && _rr )
  {
    _l_transport_var_ctxt acquire( std::move(_rr) );
    swap(acquire);
    return *this;
  }
  _l_transport_var_ctxt( const _TyThis & ) = default;
  _l_transport_var_ctxt & operator=( const _TyThis & ) = delete;
  _l_transport_var_ctxt & operator=( _TyThis _new )
  {
    swap(_new);
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_var.swap( _r.m_var );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    std::visit(_VisitHelpOverloadFCall {
      []( const auto & _tcxt )
      {
        _tcxt.AssertValid();
      }
    }, m_var );
#endif //ASSERTSENABLED    
  }
  _TyVariant & GetVariant()
  {
    return m_var;
  }
  const _TyVariant & GetVariant() const
  {
    return m_var;
  }
  bool FIsNull() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      []( const auto & _tcxt )
      {
        return _tcxt.FIsNull();
      }
    }, m_var );
  }
  vtyDataPosition PosTokenStart() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      []( const auto & _tcxt )
      {
        return _tcxt.PosTokenStart();
      }
    }, m_var );
  }
  const _TyChar * PCBufferBegin() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      []( const auto & _tcxt )
      {
        return _tcxt.PCBufferBegin();
      }
    }, m_var );
  }
  size_t NLenToken() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      []( const auto & _tcxt )
      {
        return _tcxt.NLenToken();
      }
    }, m_var );
  }
  void GetTokenDataRange( _l_data_range & _rdr ) const
  {
    std::visit(_VisitHelpOverloadFCall {
      [&_rdr]( const auto & _tcxt )
      {
        _tcxt.GetTokenDataRange( _rdr );
      }
    }, m_var );
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_range const & _rdr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    std::visit(_VisitHelpOverloadFCall {
      [&_rsv,&_rdr]( const auto & _tcxt )
      {
        _tcxt.GetStringView( _rsv, _rdr );
      }
    }, m_var );
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_typed_range const & _rdtr ) const
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    return GetStringView( _rsv, _rdtr.GetRangeBase() );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
    std::visit(_VisitHelpOverloadFCall {
      [&_rdt]( const auto & _tcxt )
      {
        _tcxt.AssertValidDataRange( _rdt );
      }
    }, m_var );
  }
protected:
  _TyVariant m_var;
};

template < class ... t_TysTransports >
class _l_transport_var : public _l_transport_base< typename tuple_element<0,tuple< t_TysTransports...>>::type::_TyChar >
{
  typedef _l_transport_var _TyThis;
  typedef _l_transport_base< typename tuple_element<0,tuple< t_TysTransports...>>::type::_TyChar > _TyBase;
public:
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyData;
  typedef unique_variant_t< variant< typename t_TysTransports::_TyTransportCtxt ... > > _TyVariantTransportCtxt;
  typedef _l_transport_var_ctxt< _TyVariantTransportCtxt > _TyTransportCtxt;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;
  typedef variant< monostate, t_TysTransports... > _TyVariant;

  _l_transport_var() = default;
  _l_transport_var( _l_transport_var const & _r ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis & operator = ( _TyThis const & _r ) = delete;
  _l_transport_var(_l_transport_var&&) noexcept = default;
  _TyThis & operator = (_TyThis && ) noexcept = default;
  void swap(_TyThis& _r)
  {
    m_var.swap(_r.m_var);
  }

  template < class t_TyTransport, class ... t_TysArgs >
  void emplaceTransport( t_TysArgs&& ... _args )
  {
    m_var.template emplace< t_TyTransport >( std::forward< t_TysArgs >( _args ) ... );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      []( auto & _transport )
      {
        _transport.AssertValid();
      }
    }, m_var );
#endif //ASSERTSENABLED
  }
  static size_t GetSupportedEncodingBitVector()
  {
    size_t grfEncodings = 0;
    ( ( grfEncodings |= ( 1ull << t_TysTransports::GetSupportedCharacterEncoding() ) ), ... );
    return grfEncodings;
  }
  bool FDependentTransportContexts() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return false;
      },
      []( auto & _transport )
      {
        return _transport.FDependentTransportContexts();
      }
    }, m_var );
  }  
  vtyDataPosition PosTokenStart() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return vtyDataPosition();
      },
      []( auto & _transport )
      {
        return _transport.PosTokenStart();
      }
    }, m_var );
  }
  vtyDataPosition PosCurrent() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return vtyDataPosition();
      },
      []( auto & _transport )
      {
        return _transport.PosCurrent();
      }
    }, m_var );
  }
  bool FAtTokenStart() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return false;
      },
      []( auto & _transport )
      {
        return _transport.FAtTokenStart();
      }
    }, m_var );
  }
  void ResetToTokenStart()
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      []( auto & _transport )
      {
        _transport.ResetToTokenStart();
      }
    }, m_var );
  }
  
  // Return the current character and advance the position.
  bool FGetChar( _TyChar & _rc )
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return false;
      },
      [&_rc]( auto & _transport )
      {
        return _transport.FGetChar(_rc);
      }
    }, m_var );
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_bufCurrentToken from [m_posTokenStart,_kdpEndToken).
  template < class t_TyToken, class t_TyValue, class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  t_TyValue & _rvalue, t_TyUserObj & _ruoUserObj,
                  unique_ptr< t_TyToken > & _rupToken )
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      [_paobCurToken,_kdpEndToken,&_rvalue,&_ruoUserObj,&_rupToken]( auto & _transport )
      {
        typedef typename t_TyToken::_TyValue _TyValue;
        static_assert( is_same_v< t_TyValue, _TyValue > );
        typedef typename t_TyToken::_TyUserContext _TyUserContext;
        typedef typename _TyUserContext::_TyUserObj _TyUserObj;
        static_assert( is_same_v< t_TyUserObj, _TyUserObj > );
        _TyUserContext ucxt( _ruoUserObj, _transport.CtxtEatCurrentToken( _kdpEndToken ) ); // move it right on in - this negates the need for a default constructor.
        unique_ptr< t_TyToken > upToken = make_unique< t_TyToken >( std::move( ucxt ), std::move( _rvalue ), _paobCurToken );
        upToken.swap( _rupToken );
      }
    }, m_var );
  }
  void DiscardData( const vtyDataPosition _kdpEndToken )
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      [_kdpEndToken]( auto & _transport )
      {
        _transport.DiscardData( _kdpEndToken );
      }
    }, m_var );
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      [&_rstr]( auto & _transport )
      {
        _transport.GetCurTokenString(_rstr);
      }
    }, m_var );
  }
  bool FSpanChars( const _TyData & _rdt, const _TyChar * _pszCharSet ) const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return false;
      },
      [&_rdt,_pszCharSet]( auto & _transport )
      {
        return _transport.FSpanChars(_rdt,_pszCharSet);
      }
    }, m_var );
  }
  bool FMatchChars( const _TyData & _rdt, const _TyChar * _pszMatch ) const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
        return false;
      },
      [&_rdt,_pszMatch]( auto & _transport )
      {
        return _transport.FMatchChars(_rdt,_pszMatch);
      }
    }, m_var );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      [&_rdt]( auto & _transport )
      {
        _transport.AssertValidDataRange(_rdt);
      }
    }, m_var );
#endif //ASSERTSENABLED  
  }
protected:
  _TyVariant m_var;
};

__LEXOBJ_END_NAMESPACE

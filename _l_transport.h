#pragma once

// _l_transport.h
// Transport objects for lexical analyzer.
// dbien : 26DEC2020 - moved from _l_strm.h cuz it was getting biggish.

#include <variant>
#include "_assert.h"
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
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar > _TyValue;
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
  typedef _l_data< t_TyChar > _TyData;

  _l_transport_backed_ctxt( vtyDataPosition _posTokenStart, vtyDataPosition _posTokenEnd )
    : m_posTokenStart( _posTokenStart ),
      m_bufTokenData( _posTokenEnd > _posTokenStart ? ( _posTokenEnd - _posTokenStart ) : 0 )
  {
    VerifyThrowSz( _posTokenEnd >= _posTokenStart, "duh" ); // Above we don't try to allocate a huge amount of memory and then we also throw if passed bogus parameters.
  }
  ~_l_transport_backed_ctxt() = default;
  _l_transport_backed_ctxt() = default;
  _l_transport_backed_ctxt( _l_transport_backed_ctxt const & ) = default;
  _l_transport_backed_ctxt( _l_transport_backed_ctxt && ) = default;
  _l_transport_backed_ctxt & operator =( _l_transport_backed_ctxt const & ) = default;
  _l_transport_backed_ctxt & operator =( _l_transport_backed_ctxt && ) = default;

  vtyDataPosition PosTokenStart() const
  {
    return m_posTokenStart;
  }
  const _TyBuffer & GetTokenBuffer() const
  {
    return m_bufTokenData;
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_typed_range const & _rdtr )
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    _AssertValidRange( _rdtr.begin(), _rdtr.end() );
    GetTokenBuffer().GetStringView( _rsv, _rdtr.begin() - _rdtr.PosTokenStart(), _rdtr.end() - _rdtr.PosTokenStart() );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        _AssertValidRange( _rdt.begin(), _rdt.end() );
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
    Assert( _posEnd <= m_posTokenStart + m_bufTokenData.length() );
#endif //ASSERTSENABLED  
  }
  vtyDataPosition m_posTokenStart{ (numeric_limits< vtyDataPosition >::max)() };
  _TyBuffer m_bufTokenData;
};

// _l_transport_file:
// Transport using a file.
template < class t_TyChar >
class _l_transport_file : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_file _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using typename  _TyBase::_TyChar;
  using typename _TyBase::_TyData;
  using typename _TyBase::_TyValue;
  typedef _l_transport_backed_ctxt< t_TyChar > _TyTransportCtxt;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  ~_l_transport_file() = default;
  _l_transport_file() = delete;
  _l_transport_file( const _l_transport_file & ) = delete;
  _l_transport_file & operator =( _l_transport_file const & ) = delete;
  // We could allow move construction but it isn't needed because we can emplace construct.
  _l_transport_file( _l_transport_file && ) = delete;
  _l_transport_file & operator =( _l_transport_file && ) = delete;

  _l_transport_file( const char * _pszFileName )
  {
    PrepareErrNo();
    vtyFileHandle hFile = OpenReadOnlyFile( _pszFileName );
    if ( vkhInvalidFileHandle == hFile )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "Open of [%s] failed.", _pszFileName );
    m_file.SetHFile( hFile, true );
    _InitNonTty();
  }
  // Attach to an hFile like STDIN - in which case you would set _fOwnFd to false.
  // The hFile will 
  _l_transport_file( vtyFileHandle _hFile, size_t _posEnd = 0, bool _fOwnFd = false )
    : m_file( _hFile, _fOwnFd )
  {
    m_fisatty = FIsConsoleFileHandle( m_file.HFileGet() );
    m_fisatty ? _InitTty() : _InitNonTty( _posEnd );
  }
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
      VerifyThrowSz( !( nbySeekCur % sizeof( t_TyChar ) ), "Current offset in file is not a multiple of a character byte length." );
      posInit = nbySeekCur / sizeof( t_TyChar );
      // Validate any ending position given to us, also find the end regardless:
      vtySeekOffset nbySeekEnd;
      iSeekResult = FileSeek( m_file.HFileGet(), 0, vkSeekEnd, &nbySeekEnd );
      if ( !!iSeekResult )
      {
        Assert( -1 == iSeekResult );
        THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "::seek() of hFile[0x%lx] failed.", (uint64_t)(m_file.HFileGet()) );
      }
      posEnd = nbySeekEnd / sizeof( t_TyChar );
      VerifyThrowSz( _posEnd <= ( nbySeekEnd / sizeof( t_TyChar ) ), "Passed an _posEnd that is beyond the EOF." );
      if ( !_posEnd )
        VerifyThrowSz( !( nbySeekEnd % sizeof( t_TyChar ) ), "End of file position is not a multiple of a character byte length." );
      else
        posEnd = _posEnd;
      stchLenRead = posEnd - posInit;
    }
    m_frrFileDesBuffer.Init( m_file.HFileGet(), posInit, fReadAhead, stchLenRead );
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
  bool FGetChar( _TyChar & _rc )
  {
    return m_frrFileDesBuffer.FGetChar( _rc );
  }

  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_frrFileDesBuffer from [m_frrFileDesBuffer.m_saBuffer.IBaseElement(),_kdpEndToken).
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken,
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, 
                  unique_ptr< _l_token< _l_user_context< _TyTransportCtxt, t_TyUserObj > > > & _rupToken )
  {
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    typedef _l_token< _TyUserContext > _TyToken;
    Assert( _kdpEndToken >= m_frrFileDesBuffer.PosBase() );
    _TyUserContext ucxt( _ruoUserObj, m_frrFileDesBuffer.PosBase(), _kdpEndToken );
    // This method ends the current token at _kdpEndToken - this causes some housekeeping within this object.
    m_frrFileDesBuffer.ConsumeData( ucxt.GetTokenBuffer().begin(), ucxt.GetTokenBuffer().length() );
    unique_ptr< _TyToken > upToken = make_unique< _TyToken >( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }
  _TyTransportCtxt CtxtEatCurrentToken( const vtyDataPosition _kdpEndToken )
  {
    Assert( _kdpEndToken >= m_frrFileDesBuffer.PosBase() );
    _TyTransportCtxt tcxt( m_frrFileDesBuffer.PosBase(), _kdpEndToken );
    m_frrFileDesBuffer.ConsumeData( tcxt.GetTokenBuffer().begin(), tcxt.GetTokenBuffer().length() );
    return _TyTransportCtxt( std::move( tcxt ) );
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
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        m_frrFileDesBuffer.AssertValidRange( _rdt.begin(), _rdt.end() );
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
  // We define a "rotating buffer" that will hold a single token *no matter how large* (since this is how the algorithm works to allow for STDIN filters to work).
  typedef FdReadRotating< _TyChar > _TyFdReadRotating;
  _TyFdReadRotating m_frrFileDesBuffer{ vknchTransportFdTokenBufferSize * sizeof( t_TyChar ) };
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
  typedef _l_data< _TyChar > _TyData;

  _l_transport_fixedmem_ctxt( vtyDataPosition _posTokenStart, _TyBuffer const & _bufTokenData )
    : m_posTokenStart( _posTokenStart ),
      m_bufTokenData( _bufTokenData )
  {
  }
  _l_transport_fixedmem_ctxt() = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt( _l_transport_fixedmem_ctxt && ) = default;
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt const & ) = default;
  _l_transport_fixedmem_ctxt & operator =( _l_transport_fixedmem_ctxt && ) = default;

  vtyDataPosition PosTokenStart() const
  {
    return m_posTokenStart;
  }
  _TyBuffer const & GetTokenBuffer() const
  {
    return m_bufTokenData;
  }
  template < class t_TyStrView >
  void GetStringView(  t_TyStrView & _rsv, _l_data_typed_range const & _rdtr )
    requires( sizeof( typename t_TyStrView::value_type ) == sizeof( _TyChar ) )
  {
    _AssertValidRange( _rdtr.begin(), _rdtr.end() );
    GetTokenBuffer().GetStringView( _rsv, _rdtr.begin() - _rdtr.PosTokenStart(), _rdtr.end() - _rdtr.PosTokenStart() );
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
    Assert( _posEnd <= m_posTokenStart + m_bufTokenData.length() );
#endif //ASSERTSENABLED  
  }
  vtyDataPosition m_posTokenStart{ (numeric_limits< vtyDataPosition >::max)() };
  _TyBuffer m_bufTokenData;
};

// _l_transport_fixedmem:
// Transport that uses a piece of memory.
template < class t_TyChar >
class _l_transport_fixedmem : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_fixedmem _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
  friend _l_transport_fixedmem_ctxt< t_TyChar >;
public:
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyData;
  using typename _TyBase::_TyValue;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyTransportCtxt;
  typedef typename _TyTransportCtxt::_TyBuffer _TyBuffer;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  _l_transport_fixedmem( _l_transport_fixedmem const & _r ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis const & _r ) = delete;
  _l_transport_fixedmem( const _TyChar * _pcBase, vtyDataPosition _nLen )
    : m_bufFull( _pcBase, _nLen ),
      m_bufCurrentToken( _pcBase, 0 )
  {
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( m_bufCurrentToken.first >= m_bufFull.first );
    Assert( ( m_bufCurrentToken.first + m_bufCurrentToken.second ) <= ( m_bufFull.first + m_bufFull.second ) );
#endif //ASSERTSENABLED
  }
  vtyDataPosition PosCurrent() const
  {
    return ( m_bufCurrentToken.begin() - m_bufFull.begin() ) + m_bufCurrentToken.length();
  }
  bool FAtTokenStart() const
  {
    return !m_bufCurrentToken.length();
  }
  // Return the current character and advance the position.
  bool FGetChar( _TyChar & _rc )
  {
    if ( _FAtEnd() )
      return false;
    _rc = m_bufCurrentToken.begin()[ m_bufCurrentToken.RLength()++ ];
    return true;
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_bufCurrentToken from [m_posTokenStart,_kdpEndToken).
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, 
                  unique_ptr< _l_token< _l_user_context< _TyTransportCtxt, t_TyUserObj > > > & _rupToken )
  {
    Assert( _kdpEndToken >= _PosTokenStart() );
    Assert( _kdpEndToken <= _PosTokenEnd() );
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    typedef _l_token< _TyUserContext > _TyToken;
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    Assert( nLenToken <= m_bufCurrentToken.length() );
    _TyUserContext ucxt( _ruoUserObj, _PosTokenStart(), _TyBuffer( m_bufCurrentToken.begin(), nLenToken ) );
    m_bufCurrentToken.RCharP() += nLenToken;
    m_bufCurrentToken.RLength() = 0;
    unique_ptr< _TyToken > upToken = make_unique< _TyToken >( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
    upToken.swap( _rupToken );
  }
  _TyTransportCtxt CtxtEatCurrentToken( const vtyDataPosition _kdpEndToken )
  {
    Assert( _kdpEndToken >= _PosTokenStart() );
    Assert( _kdpEndToken <= _PosTokenEnd() );
    vtyDataPosition nLenToken = ( _kdpEndToken - _PosTokenStart() );
    _TyBuffer bufToken( m_bufCurrentToken.first, nLenToken );
    vtyDataPosition posTokenStart = _PosTokenStart();
    m_bufCurrentToken.RCharP() += nLenToken;
    m_bufCurrentToken.RLength() = 0;
    return _TyTransportCtxt( posTokenStart, bufToken );
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
  }
  template < class t_TyString >
  void GetCurTokenString( t_TyString & _rstr ) const
    requires( sizeof( typename t_TyString::value_type ) != sizeof( _TyChar ) )
  {
    ConvertString( _rstr, m_bufCurrentToken.begin(), m_bufCurrentToken.length() );
  }
  void AssertValidDataRange( _TyData const & _rdt ) const
  {
#if ASSERTSENABLED
    if ( !_rdt.FIsNull() )
    {
      if ( _rdt.FContainsSingleDataRange() )
      {
        _AssertValidRange( _rdt.begin(), _rdt.end() );
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
  _TyBuffer m_bufFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  _TyBuffer m_bufCurrentToken; // The view for the current token's exploration.
};

// _l_transport_mapped
// Transport that uses mapped memory.
template < class t_TyChar >
class _l_transport_mapped : public _l_transport_fixedmem< t_TyChar >
{
  typedef _l_transport_mapped _TyThis;
  typedef _l_transport_fixedmem< t_TyChar > _TyBase;
public:
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyData;
  using typename _TyBase::_TyValue;
  typedef _l_transport_fixedmem_ctxt< t_TyChar > _TyTransportCtxt;
  typedef typename _TyTransportCtxt::_TyPrMemView _TyPrMemView;

  ~_l_transport_mapped()
  {
    if ( m_bufFull.first != (const _TyChar*)vkpvNullMapping )
    {
      int iRet = ::munmap( m_bufFull.first, m_bufFull.second );
      Assert( !iRet ); // not much to do about this...could log.
    }
  }
  // We are keeping the hFile open for now for diagnostic purposes, etc. It would be referenced in the background
  //  by the mapping and remaing open but we wouldn't know what the descriptor value was.
  _l_transport_mapped( const char * _pszFileName )
    : _TyBase( (const _TyChar*)vkpvNullMapping, 0 )// in case of throw so we don't call munmap with some random number in ~_l_transport_mapped().
  {
    FileObj foFile( OpenReadOnlyFile( _pszFileName ) );
    if ( foFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "OpenReadOnlyFile() of [%s] failed.", _pszFileName );
    size_t stSizeMapping;
    FileMappingObj fmoFile( MapReadOnlyHandle( foFile.HFileGet(), &stSizeMapping ) );
    if ( !fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "MapReadOnlyHandle() of [%s] failed, stSizeMapping[%lu].", _pszFileName, stSizeMapping );
    m_bufFull.second = stSizeMapping;
    m_bufFull.first = (const _TyChar*)fmoFile.PvTransferHandle();
    m_bufCurrentToken.first = m_bufFull.first;
    Assert( !m_bufCurrentToken.second );
  }
  _l_transport_mapped() = delete;
  _l_transport_mapped( _l_transport_mapped const & ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis const & ) = delete;
  _l_transport_mapped( _l_transport_mapped && ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis && ) = delete;
  
  void AssertValid() const
  {
#if ASSERTSENABLED
    _TyBase::AssertValid();
#endif //ASSERTSENABLED
  }

  using _TyBase::PosCurrent;
  using _TyBase::FAtTokenStart;
  using _TyBase::FGetChar;
  using _TyBase::GetPToken;
  using _TyBase::CtxtEatCurrentToken;
  using _TyBase::DiscardData;
  using _TyBase::GetCurTokenString;
protected:
  using _TyBase::m_bufFull; // The full view of the fixed memory that we are passing through the lexical analyzer.
  using _TyBase::m_bufCurrentToken; // The view for the current token's exploration.
};

// Produce a variant that is the set of all potential transports that the user of the object may care to use.
// We provide no functionality in this class
template < class ... t_TysTransportContexts >
class _l_transport_var_ctxt
{
  typedef _l_transport_var_ctxt _TyThis;
public:
  // We only want uniquely typed transport contexts here.
  typedef unique_variant_t< variant< t_TysTransportContexts... > > _TyVariant;
  // Construction by moved contained context:
  template < class t_TyTransportContext >
  _l_transport_var_ctxt( t_TyTransportContext && _rrtcxt )
  {
    m_var.template emplace< t_TyTransportContext >( std::move( _rrtcxt ) );
  }
  ~ _l_transport_var_ctxt() = default;
  _l_transport_var_ctxt() = delete; // We don't want to have a monostate unless we need to and I don't think we do.
  _l_transport_var_ctxt( _l_transport_var_ctxt && _rr ) = default;
  _l_transport_var_ctxt & operator=( _TyThis && _rr )
  {
    _l_transport_var_ctxt acquire( std::move(_rr) );
    swap(acquire);
    return *this;
  }
  _l_transport_var_ctxt( const _TyThis & ) = default;
  _l_transport_var_ctxt & operator=( const _TyThis & _r )
  {
    _l_transport_var_ctxt copy( _r );
    swap(copy);
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_var.swap( _r.m_var );
  }
  _TyVariant & GetVariant()
  {
    return m_var;
  }
  const _TyVariant & GetVariant() const
  {
    return m_var;
  }
// operations:
  // This will convert any fixed memory context to a backed memory context.
  template < class t_TyTransportCtxt >
  void ConvertToBacked()
  {
    // Might be need eventually.
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
  using typename _TyBase::_TyValue;
  typedef _l_transport_var_ctxt< typename t_TysTransports::_TyTransportCtxt ... > _TyTransportCtxt;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  _l_transport_var( _l_transport_var const & _r ) = delete; // Don't let any transports be copyable since they can't all be copyable.
  _TyThis const & operator = ( _TyThis const & _r ) = delete;

  template < class t_TyTransport, class ... t_TysArgs >
  void emplaceTransport( t_TysArgs ... _args )
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
      []( auto _transport )
      {
        _transport.AssertValid();
      }
    }, m_var );
#endif //ASSERTSENABLED
  }

  vtyDataPosition PosCurrent() const
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      []( auto _transport )
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
      },
      []( auto _transport )
      {
        return _transport.FAtTokenStart();
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
      },
      [&_rc]( auto _transport )
      {
        return _transport.FGetChar(_rc);
      }
    }, m_var );
  }
  // Return a token backed by a user context obtained from the transport plus a reference to our local UserObj.
  // This also consumes the data in the m_bufCurrentToken from [m_posTokenStart,_kdpEndToken).
  template < class t_TyUserObj >
  void GetPToken( const _TyAxnObjBase * _paobCurToken, const vtyDataPosition _kdpEndToken, 
                  _TyValue && _rrvalue, t_TyUserObj & _ruoUserObj, 
                  unique_ptr< _l_token< _l_user_context< _TyTransportCtxt, t_TyUserObj > > > & _rupToken )
  {
    typedef _l_user_context< _TyTransportCtxt, t_TyUserObj > _TyUserContext;
    typedef _l_token< _TyUserContext > _TyToken;
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Transport object hasn't been created.");
      },
      [_paobCurToken,_kdpEndToken,_rrvalue{move(_rrvalue)},&_ruoUserObj,&_rupToken]( auto _transport )
      {
        _TyUserContext ucxt( _ruoUserObj, _transport.CtxtEatCurrentToken( _kdpEndToken ) ); // move it right on in - this negates the need for a default constructor.
        unique_ptr< _TyToken > upToken = make_unique< _TyToken >( std::move( ucxt ), std::move( _rrvalue ), _paobCurToken );
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
      [_kdpEndToken]( auto _transport )
      {
        _transport.DiscardData( _kdpEndToken );
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
      [&_rdt]( auto _transport )
      {
        _transport.AssertValidDataRange(_rdt);
      }
    }, m_var );
#endif //ASSERTSENABLED  
  }
protected:
  variant< monostate, t_TysTransports... > m_var;
};


__LEXOBJ_END_NAMESPACE

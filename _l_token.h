#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

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
// I used to store a pointer to the action object for the token returned from the lexcial analyzer but this doesn't allow synthesis
//  of tokens via encoding transformation (for instance) and other pseudo tokens. If the action object pointer is required then
//  a switch-statement lookup from tokenid could be implemented but really there isn't a great reason (that I can think of) for
//  the action object pointer to be present in the token since it was only ever used to get the token id.

#include "_l_ns.h"
#include "_l_types.h"
#include "_l_value.h"
#include "_l_strm.h"
#include "_l_data.h"

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef t_TyTransportCtxt _TyTransportCtxt;
  typedef t_TyUserObj _TyUserObj;
  typedef t_TyTpValueTraits _TyTpValueTraits;
  typedef typename _TyTransportCtxt::_TyChar _TyChar;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef _l_user_context< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyUserContext;
  typedef _l_data<> _TyData;
	typedef _l_value< _TyChar, _TyTpValueTraits > _TyValue;
  typedef typename _TyValue::size_type size_type;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  _l_token() = delete;
  _l_token( _TyUserContext && _rrucxt, _TyValue && _rrvalue, const _TyAxnObjBase * _paobCurToken )
    : m_scx( std::move( _rrucxt ) ),
      m_value( std::move( _rrvalue ) ),
      m_tidAccept( _paobCurToken->VGetTokenId() )
  {
  }
  // Create a null token with an empty value object and no backing.
  // This is for placeholder tokens, etc.
  _l_token( _TyUserContext && _rrucxt, const _TyAxnObjBase * _paobCurToken )
    : m_scx( std::move( _rrucxt ) ),
      m_tidAccept( _paobCurToken->VGetTokenId() )
  {
  }
  _l_token( _TyUserObj & _ruoUserObj, const _TyAxnObjBase * _paobCurToken )
    : m_scx( _ruoUserObj ),
      m_tidAccept( _paobCurToken->VGetTokenId() )
  {
  }
  _l_token( _TyUserObj & _ruoUserObj, vtyTokenIdent _tidAccept )
    : m_scx( _ruoUserObj ),
      m_tidAccept( _tidAccept )
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
  _l_token( _l_token && _rr )
    : m_scx( std::move( _rr.m_scx ) )
  {
    m_value.swap( _rr.m_value );
    std::swap( m_tidAccept, _rr.m_tidAccept );
  }
  _l_token & operator =( _l_token && _rr )
  {
    _l_token acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_scx.swap( _r.m_scx );
    m_value.swap( _r.m_value );
    std::swap( m_tidAccept, _r.m_tidAccept );
  }
  // Support conversion from other token types. We must take the user object from the container to which we are copying.
  // Rely on container to translate the token to the new character type.
  template < class t_TyContainerNew, class t_TyToken >
  _l_token( t_TyContainerNew & _rNewContainer, t_TyToken const & _rtokCopy, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
    : m_scx( _rNewContainer.GetUserObj(), _rtokCopy.GetTransportCtxt().PosTokenStart() ),
      m_value( _rNewContainer, _rtokCopy.GetValue(), _ptccCopyCtxt ), // any namespace references/declarations need resolving in the new container.
      m_tidAccept( _rtokCopy.GetTokenId() )
  {
    _ConvertDataPositions( _rtokCopy );
  }
  template < class t_TyContainerNew, class t_TyToken >
  _l_token( t_TyContainerNew & _rNewContainer, t_TyToken && _rrtokCopy, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
    : m_scx( _rNewContainer.GetUserObj(), _rrtokCopy.GetTransportCtxt().PosTokenStart() ),
      m_value( _rNewContainer, std::move( _rrtokCopy.GetValue() ), _ptccCopyCtxt ), // any namespace references/declarations need resolving in the new container.
      m_tidAccept( _rrtokCopy.GetTokenId() )
  {
    _ConvertDataPositions( _rrtokCopy );
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( ( vktidInvalidIdToken == m_tidAccept ) == m_value.FIsNull() );
    Assert( ( vktidInvalidIdToken == m_tidAccept ) == m_scx.FIsNull() );
#endif //ASSERTSENABLED
  }
  bool FIsNull() const
  {
    AssertValid();
    return vktidInvalidIdToken == m_tidAccept;
  }
  _TyValue & GetValue()
  {
    return m_value;
  }
  const _TyValue & GetValue() const
  {
    return m_value;
  }
  _TyUserObj & GetUserObj()
  {
    return m_scx.GetUserObj();
  }
  const _TyUserObj & GetUserObj() const
  {
    return m_scx.GetUserObj();
  }
  _TyUserContext & GetUserContext()
  {
    return m_scx;
  }
  const _TyUserContext & GetUserContext() const
  {
    return m_scx;
  }
  _TyTransportCtxt & GetTransportCtxt()
  {
    return m_scx.GetTransportCtxt();
  }
  const _TyTransportCtxt & GetTransportCtxt() const
  {
    return m_scx.GetTransportCtxt();
  }
  vtyTokenIdent GetTokenId() const
  {
    return m_tidAccept;
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
  template < class t_tyStringView >
  void GetStringView( t_tyStringView & _rsvDest )
  {
    m_scx.GetStringView( _rsvDest, *this, GetValue() );
  }
  template < class t_tyStringView >
  void KGetStringView( t_tyStringView & _rsvDest, _TyValue const & _rval ) const
  {
    m_scx.KGetStringView( _rsvDest, *this, _rval );
  }
  template < class t_tyStringView >
  void KGetStringView( t_tyStringView & _rsvDest, _l_data_range const & _rdr ) const
  {
    m_scx.KGetStringView( _rsvDest, *this, _rdr );
  }
  template < class t_tyStringView >
  void KGetStringView( t_tyStringView & _rsvDest ) const
  {
    m_scx.KGetStringView( _rsvDest, *this, GetValue() );
  }
  template < class t_tyStringView, class t_tyString >
  bool FGetStringViewOrString( t_tyStringView & _rsvDest, t_tyString & _rstrDest, _TyValue const & _rval ) const
  {
    return m_scx.FGetStringViewOrString( _rsvDest, _rstrDest, *this, _rval );
  }
  template < class t_tyStringView, class t_tyString >
  bool FGetStringViewOrString( t_tyStringView & _rsvDest, t_tyString & _rstrDest ) const
  {
    return m_scx.FGetStringViewOrString( _rsvDest, _rstrDest, *this, GetValue() );
  }
  template < class t_tyString >
  void GetString( t_tyString & _rstrDest, _TyValue const & _rval ) const
  {
    m_scx.GetString( _rstrDest, *this, _rval );
  }
  template < class t_tyString >
  void GetString( t_tyString & _rstrDest ) const
  {
    m_scx.GetString( _rstrDest, *this, GetValue() );
  }
  // Return the beginning and ending positions of the token in the stream.
  void GetTokenDataRange( _l_data_range & _rdr ) const
  {
    m_scx.GetTokenDataRange( _rdr );
  }
  // Provide emplacement construction into the value as a shorthand here.
  template < class t_TyValue, class ... t_TysArgs >
  t_TyValue & emplaceValue( t_TysArgs && ... _args )
  {
    return m_value.template emplaceArgs< t_TyValue >( std::forward< t_TysArgs >( _args ) ... );
  }
protected:
  template < class t_TyTokenCopy >
  void _ConvertDataPositions( t_TyTokenCopy const & _rtokCopy )
    requires( TAreSameSizeTypes_v< _TyChar, typename t_TyTokenCopy::_TyChar > )
  {
    // Just directly copy the token context from the source token context.
    m_scx.GetTransportCtxt() = _rtokCopy.GetTransportCtxt();
  }
  template < class t_TyTokenCopy >
  void _ConvertDataPositions( t_TyTokenCopy const & _rtokCopy )
    requires( !TAreSameSizeTypes_v< _TyChar, typename t_TyTokenCopy::_TyChar > ) // converting.
  {
    // Get the count of positions and then obtain a pointer to each.
    // Sort the pointers to the positions by the ordinal position value.
    // Then convert the encoding piecewise, updating the positions as we go with the current offest at each according to the encoding translation.
    size_t nDataPositions = m_value.CountDataPositions();
    if ( !nDataPositions )
      return; // nada para hacer.
    nDataPositions += 2; // This gives us an extra position at the beginning and the end for algorithmic purposes.
    size_t nbySizeRgPtrPos = nDataPositions * sizeof ( vtyDataPosition * );
    // many times the positions will be in order, so we will recognize this as we accumulate positions - we still want the pointers so it is easy to update:
    typedef vector< vtyDataPosition *, default_init_allocator< vtyDataPosition * > > _TyRgPtrDP; // Avoid zero-init on resize().
    _TyRgPtrDP rgptrDP;
    pair< vtyDataPosition **, vtyDataPosition ** > prpptrDP;
    if ( nbySizeRgPtrPos > vknbyMaxAllocaSize )
    {
      rgptrDP.resize( nDataPositions );
      prpptrDP.first = &rgptrDP[0];
    }
    else
      prpptrDP.first = (vtyDataPosition **)alloca( nbySizeRgPtrPos );
    prpptrDP.second = prpptrDP.first + (nDataPositions-1); // skip the last allocated position for now - GetSortedPositionPtrs() uses the extra position at the front.
    m_value.GetSortedPositionPtrs( prpptrDP );
    ++prpptrDP.first; // The first was for the algorithm.
    Assert( *(prpptrDP.second[-1]) >= _rtokCopy.GetTransportCtxt().PosTokenStart() );
    // Now if the last position in the token is beyond the last data position then add it to the end.
    vtyDataPosition posLast = _rtokCopy.GetTransportCtxt().PosTokenStart() + _rtokCopy.GetTransportCtxt().NLenToken();
    if ( posLast > *(prpptrDP.second[-1]) )
      *prpptrDP.second++ = &posLast;
    // We copy the entire token from the beginning as the user may want to access different parts than those referenced by data positions.
    size_t nchCopy = _rtokCopy.GetTransportCtxt().NLenToken();
    // We need a buffer to convert into which we will then copy into the token buffer. We want to size it such that it must be big
    // enough to hold the conversion - regardless of content.
    size_t nchAllowBuf = nchCopy * utf_traits< _TyChar >::max_length;
    _TyStdStr strTempBuf;
    size_t nbyBufSize = nchAllowBuf * sizeof( _TyChar );
    _TyChar * pchBufBeg;
    if ( nbyBufSize > vknbyMaxAllocaSize )
    {
      strTempBuf.resize( nchAllowBuf );
      pchBufBeg = &strTempBuf[0];
    }
    else
      pchBufBeg = (_TyChar*)alloca( nbyBufSize );
    // Now move through the token buffer, converting pieces as we need to due to positional references and variable length characters:
    _TyChar * pchBufCur = pchBufBeg;
    _TyChar * const pchBufEnd = pchBufBeg + nchAllowBuf;
    typedef typename t_TyTokenCopy::_TyChar _TyCharCopy;
    const _TyCharCopy * pchCopyCur = _rtokCopy.GetTransportCtxt().PCBufferBegin();
    vtyDataPosition posCur = _rtokCopy.GetTransportCtxt().PosTokenStart();
    ssize_t offchCur = 0; // Offset to current position due to encoding transformation.
    for ( vtyDataPosition ** ppdpCur = prpptrDP.first; prpptrDP.second != ppdpCur; ++ppdpCur )
    {
      // Convert the data between posCur and *ppdpCur:
      if ( posCur != **ppdpCur )
      {
        Assert( **ppdpCur > posCur );
        _TyChar * pchBufBefore = pchBufCur;
        const _TyCharCopy * pchCopyNext = PCConvertString( pchCopyCur, size_t( **ppdpCur - posCur ), pchBufCur, ( pchBufEnd - pchBufCur ) );
        VerifyThrowSz( !!pchCopyNext, "Error found during conversion of encodings." );
        offchCur += ( pchBufCur - pchBufBefore ) - ( pchCopyNext - pchCopyCur );
        posCur = **ppdpCur;
        pchCopyCur = pchCopyNext;
      }
      **ppdpCur += offchCur;
    }
    // All positions converted in-place and prpptrDP.second[-1] is at the end of the converted buffer.
    GetTransportCtxt().GetTokenBuffer().SetBuffer( pchBufBeg, size_t( *(prpptrDP.second[-1]) - _rtokCopy.GetTransportCtxt().PosTokenStart() ) );
  }
  _TyUserContext m_scx; // The context for the stream which is passed to various _l_value methods.
  _TyValue m_value; // This value's context is in m_scx.
  vtyTokenIdent m_tidAccept{vktidInvalidIdToken}; // token id.
};

__LEXOBJ_END_NAMESPACE

namespace std
{
__LEXOBJ_USING_NAMESPACE
  // override std::swap so that it is efficient:
  template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
  void swap(_l_token< t_TyTransportCtxt, t_TyUserObj, t_TyTpValueTraits >& _rl, _l_token< t_TyTransportCtxt, t_TyUserObj, t_TyTpValueTraits >& _rr)
  {
    _rl.swap(_rr);
  }
}

#pragma once

// _l_user.h
// "User" objects for the lexical analyzer.
// dbien
// 17DEC2020

// These provide a "default translation" that many lexical analyzers can just use as-is.
// Users may want to override the methods in some cases - especially where character translation is required.
// A good example is where a piece of text may represent a hexidecimal number, etc. The translator in the overridden
//  user object would translate and also potentially check for overflow, etc.

#include "_l_ns.h"
#include "_l_types.h"
#include "_l_data.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_user_context:
// This produces an aggregate which provides customizable treatment of a token's constituent _l_data_typed_range objects.
template < class t_TyTraits >
class _l_user_context : public t_TyTraits::_TyTransportCtxt
{
  typedef _l_user_context _TyThis;
  typedef typename t_TyTraits::_TyTransportCtxt _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  typedef typename _TyTraits::_TyUserObj _TyUserObj;
  typedef typename _TyTraits::_TyChar _TyChar;
  typedef _l_token< _TyTraits > _TyToken;
  typedef _l_value< _TyTraits > _TyValue;

  _l_user_context() = delete;
  _l_user_context & operator =( _l_user_context const & ) = delete;
  _l_user_context( _TyUserObj & _ruo )
    : m_ruoUserObj( _ruo )
  {
  }
  template < class ... t_TysArgs >
  _l_user_context( _TyUserObj & _ruo, t_TysArgs && ... _args )
    : m_ruoUserObj( _ruo ),
      _TyBase( std::forward< t_TysArgs >(_args) ... )
  {
  }

  // We are copyable.
  _l_user_context( _TyThis const & ) = default; 

  // Generic initialization of transport context.
  template < class ... t_TysArgs >
  void InitTransportCtxt( t_TysArgs && ... _args )
  {
    _TyBase::InitTransportCtxt( std::forward< t_TysArgs>( _args ) ... );
  }

  template < class t_TyStringView >
  void GetStringView( t_TyStringView & _rsvDest, _TyToken & _rtok, _TyValue & _rval )
  {
    // yet another delegation...and add another parameter.
    m_ruoUserObj.GetStringView( _rsvDest, *(_TyBase*)this, _rtok, _rval );
  }
  template < class t_TyStringView >
  void KGetStringView( t_TyStringView & _rsvDest, _TyToken & _rtok, _TyValue const & _rval )
  {
    // yet another delegation...and add another parameter.
    m_ruoUserObj.KGetStringView( _rsvDest, *(_TyBase*)this, _rtok, _rval );
  }
  template < class t_TyStringView, class t_TyString >
  bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest,_TyToken & _rtok, _TyValue const & _rval )
  {
    // yet another delegation...and add another parameter.
    return m_ruoUserObj.FGetStringViewOrString( _rsvDest,  _rstrDest, *(_TyBase*)this, _rtok, _rval );
  }
  template < class t_TyString >
  void GetString( t_TyString & _rstrDest, _TyToken & _rtok, _TyValue const & _rval )
  {
    // yet another delegation...and add another parameter.
    m_ruoUserObj.GetString( _rstrDest, *(_TyBase*)this, _rtok, _rval );
  }
protected:
  _TyUserObj & m_ruoUserObj; // This is a reference to the user object that was passed into the _l_stream constructor.
};

template < class t_ty >
struct TFIsTransportVarCtxt
{
  static constexpr bool value = false;  
};
template < class ... t_tys >
struct TFIsTransportVarCtxt< _l_transport_var_ctxt< t_tys ... > >
{
  static constexpr bool value = false;  
};
template < class ... t_tys >
inline constexpr bool TFIsTransportVarCtxt_v = TFIsTransportVarCtxt< t_tys ... >::value;

template < class t_TyChar >
class _l_default_user_obj
{
  typedef _l_default_user_obj _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_data< _TyChar > _TyData;
  typedef _l_transport_backed_ctxt< _TyChar > _TyTransportCtxtBacked;
  typedef _l_transport_fixedmem_ctxt< _TyChar > _TyTransportCtxtFixedMem;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;

  template < class t_TyStream >
  bool FProcessAndFilterToken( const _TyAxnObjBase * _paobCurToken, t_TyStream const & _rstrm, vtyDataPosition _posEndToken ) const
  {
    return false;
  }

  // These are the default GetString*() impls. They just concatenates segmented strings regardless of the m_nType value.
  // _rval is a constituent value of _rtok.m_value or may be _rtok.m_value itself. We expect _rval's _TyData object to be
  //  occupied and we will convert it to either a string or a string_view depending on various things...
  // 1) If the character type of the returned string matches _TyChar:
    // a) If _rval<_TyData> contains only a single value and it doesn't cross a segmented memory boundary then we can return a
    //    stringview and we will also update the value with a stringview as it is inexpensive.
    // b) If _rval<_TyData> contains only a single value but it cross a segmented memory boundary then we will create a string
    //    of the appropriate length and then stream the segarray data into the string.
    // c) If _rval<_TyData> contains multiple values then we have to create a string of length adding all the sub-lengths together
    //    and then stream each piece.
  // 2) If the character type doesn't match then need to first create the string - hopefully on the stack using alloca() - and then
  //    pass it to the string conversion.
  // In all cases where we produce a new string we store that string in _rval - we must because we are returning a stringview to it.
  // This means that we may store data in a character representation that isn't _TyChar in _rval and that's totally fine (at least for me).
// Generic transport methods:
// For all transport types these converting methods are exactly the same.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue & _rval )
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    typedef typename t_TyStringView::value_type _TyCharConvertTo;
    typedef typename t_TyToken::_TyValue::template get_string_type< _TyCharConvertTo > _TyStrConvertTo;
    _TyStrConvertTo strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _TyStrConvertTo & rstr = _rval.emplaceVal( std::move( strConverted ) );
    _rsvDest = t_TyStringView( (const _TyCharConvertTo*)&rstr[0], rstr.length() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue const & _rval )
    requires ( sizeof( typename t_TyStringView::value_type ) != sizeof( _TyChar ) )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    t_TyString strConverted;
    GetString( strConverted, _rcxt, _rtok, _rval );
    _rstrDest = std::move( strConverted );
    return false;
  }
// var transport:
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rsvDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        _rcxtTransport.GetStringView( _rsvDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue const & _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rsvDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        _rcxtTransport.KGetStringView( _rsvDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue& _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rsvDest,&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        _rcxtTransport.FGetStringViewOrString( _rsvDest, _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue& _rval )
    requires ( TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    return visit(_VisitHelpOverloadFCall {
      [&_rstrDest,&_rtok,&_rval]( auto & _rcxtTransport )
      {
        _rcxtTransport.GetString( _rstrDest, _rcxtTransport, _rtok, _rval );
      }
    }, _rcxt.GetVariant() );
  }
// Non-converting GetString*.
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void GetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() )
    {
      typedef typename t_TyToken::_TyValue::template get_string_type< _TyChar > _TyStringImpl;
      _TyStringImpl strBacking;
      GetString( strBacking, _rcxt, _rtok, _rval );
      _TyStringImpl & rstr = _rval.emplaceVal( std::move( strBacking ) );;
      _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)&rstr[0], rstr.length() );
    }
    else
    {
      // We could set the stringview into the object since it doesn't really hurt:
      _rcxt.GetStringView( _rsvDest, kdtr );
      _rval.SetVal( _rsvDest );
    }
  }
  template < class t_TyStringView, class t_TyToken, class t_TyTransportCtxt >
  static void KGetStringView( t_TyStringView & _rsvDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue const & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rsvDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    VerifyThrowSz( kdtr.FContainsSingleDataRange(), "KGetStringView() is only valid for single data ranges." );
    _rcxt.GetStringView( _rsvDest, kdtr );
  }
  template < class t_TyStringView, class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static bool FGetStringViewOrString( t_TyStringView & _rsvDest, t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue & _rval )
    requires ( ( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    static_assert( sizeof( typename t_TyStringView::value_type ) == sizeof( typename t_TyString::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    _rcxt.AssertValidDataRange( kdtr );
    if ( !kdtr.FContainsSingleDataRange() )
    {
      GetString( _rstrDest, _rcxt, _rtok, _rval );
      return false;
    }
    else
    {
      _rcxt.GetStringView( _rsvDest, kdtr );
      return true;
    }
  }
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, const typename t_TyToken::_TyValue & _rval )
    requires ( ( sizeof( typename t_TyString::value_type ) == sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a string:
    vtyDataPosition nCharsCount = kdtr.CountChars();
    vtyDataPosition nCharsRemaining = nCharsCount;
    t_TyString strBacking( nCharsRemaining, 0 ); // Return the type the caller asked for.
    if ( kdtr.FContainsSingleDataRange() )
    {
      memcpy( &strBacking[0], _rcxt.GetTokenBuffer().begin() + kdtr.begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      _TyChar * pcCur = (_TyChar*)&strBacking[0]; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            Assert( nCharsRemaining >= pdtrCur->length() );
            vtyDataPosition nCharsCopy = min( nCharsRemaining, pdtrCur->length() );
            Assert( nCharsCopy == pdtrCur->length() ); // should have reserved enough.
            memcpy( pcCur, _rcxt.GetTokenBuffer().begin() + pdtrCur->begin() - _rcxt.PosTokenStart(), nCharsCopy * sizeof( _TyChar ) );
            pcCur += nCharsCopy;
            nCharsRemaining -= nCharsCopy;
          }
        }
      );
      Assert( !nCharsRemaining ); // Should have eaten everything.
    }
    strBacking.resize( nCharsCount - nCharsRemaining );
    _rstrDest = std::move( strBacking );
  }

// Converting GetString*.
  template < class t_TyString, class t_TyToken, class t_TyTransportCtxt >
  static void GetString( t_TyString & _rstrDest, t_TyTransportCtxt & _rcxt, t_TyToken & _rtok, typename t_TyToken::_TyValue const & _rval )
    requires ( ( sizeof( typename t_TyString::value_type ) != sizeof( _TyChar ) ) && !TFIsTransportVarCtxt_v< t_TyTransportCtxt > )
  {
    Assert( _rstrDest.empty() );
    typedef typename t_TyString::value_type _TyCharConvertTo;
    Assert( _rval.FHasTypedData() ); // We are converting the _TyData object that is in _rval.
    const _TyData kdtr = _rval.template GetVal< _TyData >();
    if ( kdtr.FIsNull() )
      return;
    _rcxt.AssertValidDataRange( kdtr );
    // Then we must back with a converted string, attempt to use an alloca() buffer:
    static size_t knchMaxAllocaSize = vknbyMaxAllocaSize / sizeof( _TyChar );
    typename t_TyToken::_TyValue::template get_string_type< _TyChar > strTempBuf; // For when we have more than knchMaxAllocaSize.
    vtyDataPosition nCharsCount = kdtr.CountChars();
    vtyDataPosition nCharsRemaining = nCharsCount;
    _TyChar * pcBuf;
    if ( nCharsCount > knchMaxAllocaSize )
    {
      strTempBuf.resize( nCharsCount );
      pcBuf = &strTempBuf[0];
    }
    else
      pcBuf = (_TyChar*)alloca( nCharsCount * sizeof( _TyChar ) );
    if ( kdtr.FContainsSingleDataRange() )
    {
      // REVIEW: Could just use a string view here instead and not copy to the stack.
      memcpy( pcBuf, _rcxt.GetTokenBuffer().begin() + kdtr.DataRangeGetSingle().begin() - _rcxt.PosTokenStart(), nCharsRemaining * sizeof( _TyChar ) );
      nCharsRemaining = 0;
    }
    else
    {
      _TyChar * pcCur = pcBuf; // Current output pointer.
      kdtr.GetSegArrayDataRanges().ApplyContiguous( 0, kdtr.GetSegArrayDataRanges().NElements(), 
        [&pcCur,&nCharsRemaining,&_rcxt]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          const _l_data_typed_range * pdtrCur = _pdtrBegin;
          for ( ; nCharsRemaining && ( _pdtrEnd != pdtrCur ); ++pdtrCur )
          {
            Assert( nCharsRemaining >= pdtrCur->length() );
            vtyDataPosition nCharsCopy = min( nCharsRemaining, pdtrCur->length() );
            Assert( nCharsCopy == pdtrCur->length() ); // should have reserved enough.
            memcpy( pcCur, _rcxt.GetTokenBuffer().begin() + pdtrCur->begin() - _rcxt.PosTokenStart(), nCharsCopy * sizeof( _TyChar ) );
            pcCur += nCharsCopy;
            nCharsRemaining -= nCharsCopy;
          }
        }
      );
      Assert( !nCharsRemaining );
    }
    if ( nCharsRemaining )
      nCharsCount -= nCharsRemaining;
    t_TyString strConverted;
    ConvertString( strConverted, pcBuf, nCharsCount );
    _rstrDest = std::move( strConverted );
  }
};

__LEXOBJ_END_NAMESPACE

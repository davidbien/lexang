#pragma once

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_value.h
// This will hold the "value" of a token. The value of a token is often a multi-level aggregate.
// dbien
// 11DEC2020

#include "_l_inc.h"
#include <string_view>
#include <variant>
#include "segarray.h"

__LEXOBJ_BEGIN_NAMESPACE

// Note that these to templates represent the standard pattern "overload" for variants.
template< class... Ts > 
struct _VisitHelpOverloadFCall : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> _VisitHelpOverloadFCall(Ts...) -> _VisitHelpOverloadFCall<Ts...>;

template< class t_TyChar, size_t s_knValsSegSize = 32 >
class _l_value
{
  typedef _l_value _TyThis;
public:
  typedef _l_data< t_TyChar > _TyData;
  static constexpr size_t s_knbySegArrayInit = s_knValsSegSize * sizeof(_TyThis);
  typedef SegArray< _TyThis, std::true_type > _TySegArrayValues;
  typedef typename _TySegArrayValues::_tySizeType _tySizeType;
  typedef _tySizeType size_type;
  // Use monostate to allow an "empty" value here.
  // These are the possible values of a action object.
  // We add all three types of STL strings to the variant - at least under linux - char, char16_t and char32_t.
  // Specifying them in this manner should allow for no code changes under Windows even though wchar_t is char16_t under Windows.
  typedef basic_string< char > _TyStrChar8;
  typedef basic_string< char16_t > _TyStrChar16;
  typedef basic_string< char32_t > _TyStrChar32;
  template < class t_TyChar >
  using get_string_type = basic_string< t_TyChar >;

  // Also allow basic_string_views of every type since that can be used until character translation is needed.
  typedef basic_string_view< char > _TyStrViewChar8;
  typedef basic_string_view< char16_t > _TyStrViewChar16;
  typedef basic_string_view< char32_t > _TyStrViewChar32;
  template < class t_TyChar >
  using get_string_view_type = basic_string_view< t_TyChar >;

  // Also need views into segmented arrays which may not be contiguous memory - i.e. a given view may span chunk boundary(s).
  // Want to avoid translating the string until the reader asks for it to be translated - or even giving it a contiguous chunk of memory.
  typedef SegArrayView< char > _TySegArrayViewChar8;
  typedef SegArrayView< char16_t > _TySegArrayViewChar16;
  typedef SegArrayView< char32_t > _TySegArrayViewChar32;
  template < class t_TyChar >
  using get_SegArrayView_type = SegArrayView< t_TyChar >;

  // Our big old variant.
  typedef variant<  monostate, bool, vtyDataPosition, _TyData, 
                    _TyStrChar8, _TyStrViewChar8, _TySegArrayViewChar8, 
                    _TyStrChar16, _TyStrViewChar16, _TySegArrayViewChar16,
                    _TyStrChar32, _TyStrViewChar32, _TySegArrayViewChar32,
                    _TySegArrayValues > _TyVariant;

  _l_value() = default;
  _l_value(_l_value const & ) = default;
  _l_value & operator =( _l_value const & ) = default;
  void swap( _TyThis & _r )
  {
    m_var.swap( _r.m_var );
  }

  bool FIsNull() const
  {
    return holds_alternative<monostate>( m_var );
  }
  void Clear()
  {
    m_var.emplace<monostate>();
  }

  template < class t_Ty >
  bool FIsA() const
  {
    return holds_alternative<t_Ty>( m_var );
  }
  bool FIsArray() const
  {
    return holds_alternative<_TySegArrayValues>( m_var );
  }
  bool FHasTypedData() const
  {
    return FIsA< _TyData >();
  }

  template < class t_Ty, class... t_tysArgs>
  _tyT & emplaceArgs( t_tysArgs &&... _args )
  {
    return m_var.emplace<t_Ty>( std::forward<t_tysArgs>(_args)... );
  }
  template < class t_Ty >
  _tyT & emplaceVal( t_Ty const & _r )
  {
    return m_var.emplace<t_Ty>( _r );
  }
  template < class t_Ty >
  _tyT & emplaceVal( t_Ty && _rr )
  {
    return m_var.emplace<t_Ty>( std::move( _rr ) );
  }

  template < class t_Ty >
  t_Ty & GetVal()
  {
    Assert( holds_alternative<t_Ty>( m_var) ); // assert before we throw...
    return get< t_Ty >( m_var );
  }
  template < class t_Ty >
  const t_Ty & GetVal() const
  {
    Assert( holds_alternative<t_Ty>( m_var) ); // assert before we throw...
    return get< t_Ty >( m_var );
  }
  // These should work in many cases.
  template < class t_Ty >
  t_Ty & SetVal( t_Ty const & _rv )
  {
    if ( holds_alternative< t_Ty >( m_var ) )
      return get< t_Ty >( m_var ) = _rv;
    else
      return m_var.emplace<t_Ty>( _rv );
  }
  template < class t_Ty >
  t_Ty & SetVal( t_Ty && _rrv )
  {
    if ( holds_alternative< t_Ty >( m_var ) )
      return get< t_Ty >( m_var ) = std::move( _rrv );
    else
      return m_var.emplace<t_Ty>(std::move(_rrv));
  }

  // This ensures we have an array of _l_values within and then sets the size to the passed size.
  void SetSize( size_type _stNEls )
  {
    if ( !FIsArray() )
      m_var.emplace<_TySegArrayValues>( s_knbySegArrayInit );
    get< _TySegArrayValues >( m_var ).SetSize( _stNEls );
  }
  // This fails with a throw if we don't contain an array.
  void GetSize() const
  {
    Assert( FIsArray() );
    return get< _TySegArrayValues >( m_var ).NElements();
  }

  _TyThis & operator [] ( size_type _nEl )
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var )[ _nEl ]; // We let this throw a bad type exception as it will if we don't contain an array.
  }
  _TyThis const & operator [] ( size_type _nEl ) const
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var )[ _nEl ]; // We let this throw a bad type exception as it will if we don't contain an array.
  }

  // This will return a string view of the given type.
  // It will:
  // 1) If the current state is representable as a string view of the given type then set the type contained in the value to such a string view.
  // 2) If the current state is not representable as a string view of the given type then it will create a string of the given type and store
  //    that in the variant and return a string view on that string.
  // We will convert from one representation to another according to the caller's whims...
  // If the current state is not a string-related state then we throw.
  template < class t_TyToken, class t_TyStringView >
  void GetStringView( t_TyToken & _rtok, t_TyStringView & _rsv )
  {
    static_assert( TIsStringView_v< t_TyStringView > );
    Assert( _rsv.empty() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.")
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.")
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.")
      },
      [this,&_rtok,&_rsv](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rtok.GetStringView( _rsv, *this );
      },
      [this,&_rsv](_TyStrChar8 const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar8 const & _rsv8)
      {
        _GetStringView( _rsv, _rsv8 );
      },
      [this,&_rsv](_TySegArrayViewChar8 const & _rsav)
      {
        _GetStringView( _rsv, _rsav );
      },
      [this,&_rsv](_TyStrChar16 const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar16 const & _rsv16)
      {
        _GetStringView( _rsv, _rsv16 );
      },
      [this,&_rsv](_TySegArrayViewChar16 const & _rsav)
      {
        _GetStringView( _rsv, _rsav );
      },
      [this,&_rsv](_TyStrChar32 const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar32 const & _rsv32)
      {
        _GetStringView( _rsv, _rsv32 );
      },
      [this,&_rsv](_TySegArrayViewChar32 const & _rsav)
      {
        _GetStringView( _rsv, _rsav );
      },
      [](_TySegArrayValues &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.")
      },
    }, m_var );
  }
protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStrViewDest, class t_TySource >
  static void _GetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( is_same_v< t_TyStrViewDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    _rsvDest = t_TyStrViewDest( &_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStrViewDest, class t_TySource >
  void _GetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( !is_same_v< t_TyStrViewDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    get_string_type< t_TyStrViewDest::value_type > strConverted;
    ConvertString( strConverted, _rsrc );
    m_var.emplace< get_string_type< t_TyStrViewDest::value_type > >( std::move( strConverted ) );
    get_string_type< t_TyStrViewDest::value_type > & rstrInVar = std::get< get_string_type< t_TyStrViewDest::value_type > >( m_var );
    _rsvDest = t_TyStrViewDest( &rstrInVar[0], rstrInVar.length() );
  }
  // SegArrayView will do the conversion efficiently using alloca() so we just need one version of this method:
  template < class t_TyStrViewDest, class t_TySource >
  void _GetStringView( t_TyStrViewDest & _rsvDest, const t_TySource & _rsrc )
    // requires are satisfied by being every scenario except for the above.
  {
    get_string_type< t_TyStrViewDest::value_type > str;
    bool fGotView = _rsrc.FGetStringViewOrString( _rsvDest, str );
    if ( !fGotView )
    {
      m_var.emplace< get_string_type< t_TyStrViewDest::value_type > >( std::move( str ) );
      get_string_type< t_TyStrViewDest::value_type > & rstrInVar = std::get< get_string_type< t_TyStrViewDest::value_type > >( m_var );
      _rsvDest = t_TyStrViewDest( &rstrInVar[0], rstrInVar.length() );
    }
  }
public:

  // This is much like the above but we won't change the current state of the value (hence it is a const method).
  // This method can be used to get a modifiable string object without allocating anything else inside the token.
  //  In that way it can be more efficient if the user only needs to query the value once as would often be the case.
  template < class t_TyToken, class t_TyString >
  void GetString( t_TyToken & _rtok, t_TyString & _rstr ) const
  {
    Assert( _rstr.empty() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.")
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.")
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.")
      },
      [this,&_rtok,&_rstr](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        _rtok.GetString( _rstr, *this );
      },
      [&_rstr](_TyStrChar8 const & _rstr8)
      {
        _GetString( _rstr, _rstr8 );
      },
      [&_rstr](_TyStrViewChar8 const & _rsv8)
      {
        _GetString( _rstr, _rsv8 );
      },
      [&_rstr](_TySegArrayViewChar8 const & _rsav)
      {
        _GetString( _rstr, _rsav );
      },
      [&_rstr](_TyStrChar16 const & _rstr16)
      {
        _GetString( _rstr, _rstr16 );
      },
      [&_rstr](_TyStrViewChar16 const & _rsv16)
      {
        _GetString( _rstr, _rsv16 );
      },
      [&_rstr](_TySegArrayViewChar16 const & _rsav)
      {
        _GetString( _rstr, _rsav );
      },
      [&_rstr](_TyStrChar32 const & _rstr32)
      {
        _GetString( _rstr, _rstr32 );
      },
      [&_rstr](_TyStrViewChar32 const & _rsv32)
      {
        _GetString( _rstr, _rsv32 );
      },
      [&_rstr](_TySegArrayViewChar32 const & _rsav)
      {
        _GetString( _rstr, _rsav );
      },
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.")
      },
    }, m_var );
  }

protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStringDest, class t_TySource >
  static void _GetString( t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( is_same_v< t_TyStringDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    _rstrDest.assign( &_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStringDest, class t_TySource >
  static void _GetString( t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( !is_same_v< t_TyStringDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    ConvertString( _rstrDest, _rsrc ); // that was easy.
  }
  // SegArrayView will do the conversion efficiently using alloca() so we just need one version of this method:
  template < class t_TyStringDest, class t_TySource >
  static void _GetString( t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    // requires are satisfied by being every scenario except for the above.
  {
    _rsrc.GetString( _rstrDest );
  }
public:

  // Like the above but as it is const it will only return a string view if that returnable from the current data.
  // Otherwise it will return a string.
  // Returns true if able to return a stringview, otherwise returns a string and returns false.
  // The time to use this method is when you want a flat piece of string, that you only want to look at once, that you
  //  don't want to modify - since you might get a stringview.
  template < class t_TyToken, class t_TyStringView, class t_TyString >
  bool FGetStringViewOrString( t_TyToken & _rtok, t_TyStringView & _rsv, t_TyString & _rstr ) const
  {
    Assert( _rstr.empty() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.")
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.")
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.")
      },
      [this,&_rtok,&_rsv,&_rstr](_TyData const & _rdt)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        return _rtok.FGetStringViewOrString( _rsv, _rstr, *this );
      },
      [&_rsv,&_rstr](_TyStrChar8 const & _rstr8)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr8 );
      },
      [&_rsv,&_rstr](_TyStrViewChar8 const & _rsv8)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv8 );
      },
      [&_rsv,&_rstr](_TySegArrayViewChar8 const & _rsav)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsav );
      },
      [&_rsv,&_rstr](_TyStrChar16 const & _rstr16)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr16 );
      },
      [&_rsv,&_rstr](_TyStrViewChar16 const & _rsv16)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv16 );
      },
      [&_rsv,&_rstr](_TySegArrayViewChar16 const & _rsav)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsav );
      },
      [&_rsv,&_rstr](_TyStrChar32 const & _rstr32)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr32 );
      },
      [&_rsv,&_rstr](_TyStrViewChar32 const & _rsv32)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv32 );
      },
      [&_rsv,&_rstr](_TySegArrayViewChar32 const & _rsav)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsav );
      },
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.")
      },
    }, m_var );
  }
protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStringViewDest, class t_TyStringDest, class t_TySource >
  static bool _FGetStringViewOrString( t_TyStringViewDest & _rsvDest, t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( is_same_v< t_TyStringDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    _rsvDest = t_TyStrViewDest( &_rsrc[0], _rsrc.length() );
    return true;
  }
  // Converting version for string or view source - we must convert the existing value into the passed string.
  template < class t_TyStringViewDest, class t_TyStringDest, class t_TySource >
  static bool _FGetStringViewOrString( t_TyStringViewDest & _rsvDest, t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( !is_same_v< t_TyStringDest::value_type, t_TySource::value_type > 
      && ( is_same_v< t_TySource, get_string_view_type< t_TySource::value_type > > || is_same_v< t_TySource, get_string_type< t_TySource::value_type > > ) )
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    ConvertString( _rstrDest, _rsrc ); // that was easy.
    return false;
  }
  // SegArrayView will do the conversion efficiently using alloca() so we just need one version of this method:
  template < class t_TyStringViewDest, class t_TyStringDest, class t_TySource >
  static bool _FGetStringViewOrString( t_TyStringViewDest & _rsvDest, t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    // requires are satisfied by being every scenario except for the above.
  {
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    return _rsrc.FGetStringViewOrString( _rsvDest, _rstrDest );
  }
public:

  // Output the data to a JsoValue.
  template < class t_TyCharOut >
  void ToJsoValue( JsoValue< t_TyCharOut > & _rjv ) const
  {
    Assert( _rjv.FIsNull() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        _rjv.SetNullValue();
      },
      [&_rjv](bool _f)
      {  
        _rjv.SetBoolValue( _f );
      },
      [&_rjv](vtyDataPosition _dp)
      {
        _rjv.SetValue( _dp );
      },
      [&_rjv](_TyData const & _rdt)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rdt.ToJsoValue( _rjv );
      },
      [&_rjv](_TyStrChar8 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar8 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TySegArrayViewChar8 const & _rsav)
      {
        _TyStrChar8 str;
        _TyStrViewChar8 sv;
        bool fSV = _rsav.FGetStringViewOrString( sv, str );
        if ( fSV )
          _rjv.SetStringValue( sv );
        else
          _rjv.SetStringValue( str );
      },
      [&_rjv](_TyStrChar16 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar16 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TySegArrayViewChar16 const & _rsav)
      {
        _TyStrChar16 str;
        _TyStrViewChar16 sv;
        bool fSV = _rsav.FGetStringViewOrString( sv, str );
        if ( fSV )
          _rjv.SetStringValue( sv );
        else
          _rjv.SetStringValue( str );
      },
      [&_rjv](_TyStrChar32 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar32 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TySegArrayViewChar32 const & _rsav)
      {
        _TyStrChar32 str;
        _TyStrViewChar32 sv;
        bool fSV = _rsav.FGetStringViewOrString( sv, str );
        if ( fSV )
          _rjv.SetStringValue( sv );
        else
          _rjv.SetStringValue( str );
      },
      [&_rjv](_TySegArrayValues const & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        _rjv.SetArrayCapacity( knEls ); // preallocate
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].ToJsoValue( _rjv.CreateOrGetEl( nEl ) );
      },
    }, m_var );
  }

  // Convert stream position ranges to strings throughout the entire value (i.e. recursively) using the stream _rs.
  // If there are strings that are present which are not in the representation t_TyCharOut then these are converted to t_TyCharOut.
  // This will turn all strings into "usable" version - according to the transport backing the stream.
  //  1) All _TyData _l_values are converted into either:
  //		a) SegArrayView<t_TyChar>s if they are single, simple, _TyData(s) with no processing required and the backing an fd or other non-in-memory backing.
  //    b) basic_string_view<t_TyChar>s if they are single, simple, _TyData(s) with no processing required and the backing an mapped file or in-memory backing.
  //    c) basic_string<t_TyChar>s if processing is required for single- or multi-part _TyData(s).
  //  2) Any other values that aren't represented in t_TyCharOut form are converted to t_TyCharOut form.
  template < class t_TyTransportCtxt, class t_TyCharOut = t_TyChar >
  void ProcessStrings( t_TyTransportCtxt & _rcxt )
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) {},
      [](bool _f) {},
      [&_rjv](vtyDataPosition _dp) {},
      [&_rjv](_TyData const & _rdt)
      {
        // Allow the stream to decide how to deal with any set of strings that might be present.
        _rcxt.ProcessStrings< t_TyCharOut >( *this );
      },
      [&_rjv](_TyStrChar8 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrViewChar8 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TySegArrayViewChar8 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrChar16 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrViewChar16 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TySegArrayViewChar16 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrViewChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TySegArrayViewChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TySegArrayValues const & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].ProcessStrings( _rcxt );
      },
    }, m_var );
  }

protected:
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_string_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  { // no-op.
  }
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_string_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( !is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rstr );
    m_var.emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_string_view_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  { // no-op.
  }
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_string_view_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( !is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rstr );
    m_var.emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_SegArrayView_type< t_TyCharConvertFrom > & _rsav ) 
    requires ( is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  { // no-op.
  }
  template < class t_TyCharConvertFrom, class t_TyCharConvertTo >
  static void _ConvertStringValue( get_SegArrayView_type< t_TyCharConvertFrom > & _rsav ) 
    requires ( !is_same_v< t_TyCharConvertFrom, t_TyCharConvertTo > )
  {
    get_string_view_type< t_TyCharConvertFrom > sv;
    get_string_type< t_TyCharConvertFrom > str;
    bool fSV = _rsav.FGetStringViewOrString( sv, str );
    get_string_type< t_TyCharConvertTo > strConverted;
    if ( fSV )
      ConvertString( strConverted, sv );
    else
      ConvertString( strConverted, str );
    m_var.emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  _TyVariant m_var;
};

#if 0 // Not sure about this - may store in _l_token.
// _l_value_w_buffer
// This is a value object that comes with a buffer the contents of which are referred to by some members of the
//	aggregated value object. This is all to speed and allow non-seeking file support such that STDIN can be used
//  as an input stream.
// Note that only the topmost object is 
template< class t_TyChar, size_t s_knValsSegSize = 1024 >
class _l_value_w_buffer : public _l_value< t_TyChar, s_knValsSegSize >
{
  typedef _l_value_w_buffer _TyThis;
  typedef _l_value< t_TyChar, s_knValsSegSize > _TyBase;
public:
  using _TyBase::_TyData;
  using _TyBase::s_knbySegArrayInit;
  using _TyBase::_TySegArrayValues;
  using _TyBase::_tySizeType;
  using _TyBase::size_type;
  typedef SegArray< t_TyChar, false_Type > _TySegArrayBuffer;

  _l_value_w_buffer() = default;
  _l_value_w_buffer( vtyDataPosition _posStart, _TySegArrayBuffer && _rrbuf )
    : m_posStart( _posStart )
  {
    m_buf.swap( _rrbuf );
  }

  void SwapSegArrayBuf( _TySegArrayBuffer & _rbuf )
  {
    m_buf.swap( _rbuf );
  }
protected:
  _TySegArrayBuffer m_buf{s_knbySegArrayInit}; // some of the values in the aggregated _l_value may refer to memory in this segarray.
  vtyDataPosition m_posStart{vkdpNullDataPosition}; // The position in the input stream corresponding to the start of data in m_buf.
};
#endif 0

__LEXOBJ_END_NAMESPACE

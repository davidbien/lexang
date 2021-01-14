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
#include "_util.h"
#include "_namdexc.h"
#include "segarray.h"

__LEXOBJ_BEGIN_NAMESPACE

template< class t_TyTraits >
class _l_value
{
  typedef _l_value _TyThis;
public:
  typedef t_TyTraits _TyTraits;
  typedef typename _TyTraits::_TyChar;
  typedef _l_data< _TyChar > _TyData;
  static constexpr size_t s_knValsSegSize = _TyTraits::s_knValsSegSize;
#ifdef _MSC_VER
   // vc has a problem with asking for the size of an incomplete type here.
  static const size_t s_knbySegArrayInit;
#else // gcc and clang like it fine.
  static constexpr size_t s_knbySegArrayInit = s_knValsSegSize * sizeof(_TyThis);
#endif
  typedef SegArray< _TyThis, std::true_type > _TySegArrayValues;
  typedef typename _TySegArrayValues::_tySizeType _tySizeType;
  typedef _tySizeType size_type;
  // These are the possible values of a action object.
  // We add all three types of STL strings to the variant - at least under linux - char, char16_t and char32_t.
  // Specifying them in this manner should allow for no code changes under Windows even though wchar_t is char16_t under Windows.
  typedef basic_string< char > _TyStrChar;
  typedef basic_string< char8_t > _TyStrChar8;
  typedef basic_string< char16_t > _TyStrChar16;
  typedef basic_string< wchar_t > _TyStrWChar;
  typedef basic_string< char32_t > _TyStrChar32;
  template < class t__TyChar >
  using get_string_type = basic_string< t__TyChar >;
  template <>
  using get_string_type<char8_t> = basic_string<char>;
#ifdef BIEN_WCHAR_16BIT
  template <>
  using get_string_type<wchar_t> = basic_string<char16_t>;
#else //BIEN_WCHAR_32BIT
  template <>
  using get_string_type<wchar_t> = basic_string<char32_t>;
#endif //BIEN_WCHAR_32BIT
  typedef basic_string< _TyChar > _TyStdString; // The impl's string.

  // Also allow basic_string_views of every type since that can be used until character translation is needed.
  typedef basic_string_view< char > _TyStrViewChar;
  typedef basic_string_view< char8_t > _TyStrViewChar8;
  typedef basic_string_view< char16_t > _TyStrViewChar16;
  typedef basic_string_view< wchar_t > _TyStrViewWChar;
  typedef basic_string_view< char32_t > _TyStrViewChar32;
  template < class t__TyChar >
  using get_string_view_type = basic_string_view< t__TyChar >;
  template <>
  using get_string_view_type<char8_t> = basic_string_view<char>;
#ifdef BIEN_WCHAR_16BIT
  template <>
  using get_string_view_type<wchar_t> = basic_string_view<char16_t>;
#else //BIEN_WCHAR_32BIT
  template <>
  using get_string_view_type<wchar_t> = basic_string_view<char32_t>;
#endif //BIEN_WCHAR_32BIT
  typedef basic_string_view< _TyChar > _TyStdStringView;

  // Our big old variant.
  // Use monostate to allow an "empty" value here.
  // First define the "base" variant which doesn't include any user defined types.
  typedef variant<  monostate, bool, vtyDataPosition, _TyData, 
                    _TyStrChar, _TyStrViewChar,
                    _TyStrChar16, _TyStrViewChar16,
                    _TyStrChar32, _TyStrViewChar32,
                    _TySegArrayValues > _TyVariantBase;
  // Now define our full variant type by concatenating on any user defined types.
  typedef concatenator_pack< _TyVariantBase, typename _TyTraits::_TyValueTraits > ::type _TyVariant;

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
    m_var.template emplace<monostate>();
  }

  template < class t_Ty >
  bool FIsA() const
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    return holds_alternative<_TyRemoveRef>( m_var );
  }
  bool FIsArray() const
  {
    return holds_alternative<_TySegArrayValues>( m_var );
  }
  bool FHasTypedData() const
  {
    return FIsA< _TyData >();
  }
  bool FEmptyTypedData() const
  {
    return FHasTypedData() && GetVal<_TyData>().FIsNull();
  }

  template < class t_Ty, class... t_tysArgs>
  t_Ty & emplaceArgs( t_tysArgs &&... _args )
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    return m_var.template emplace<_TyRemoveRef>( std::forward<t_tysArgs>(_args)... );
  }
  template < class t_Ty >
  t_Ty & emplaceVal( t_Ty const & _r )
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    return m_var.template emplace<_TyRemoveRef>( _r );
  }
  template < class t_Ty >
  t_Ty & emplaceVal( t_Ty && _rr )
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    return m_var.template emplace<_TyRemoveRef>( std::move( _rr ) );
  }

  template < class t_Ty >
  t_Ty & GetVal()
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    Assert( holds_alternative<_TyRemoveRef>( m_var) ); // assert before we throw...
    return get< _TyRemoveRef >( m_var );
  }
  template < class t_Ty >
  const t_Ty & GetVal() const
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    Assert( holds_alternative<_TyRemoveRef>( m_var) ); // assert before we throw...
    return get< _TyRemoveRef >( m_var );
  }
  // These should work in many cases.
  template < class t_Ty >
  t_Ty & SetVal( t_Ty const & _rv )
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    if ( holds_alternative< _TyRemoveRef >( m_var ) )
      return get< _TyRemoveRef >( m_var ) = _rv;
    else
      return m_var.template emplace<_TyRemoveRef>( _rv );
  }
  template < class t_Ty >
  t_Ty & SetVal( t_Ty && _rrv )
  {
    using _TyRemoveRef = remove_reference_t< t_Ty >;
    if ( holds_alternative< _TyRemoveRef >( m_var ) )
      return get< _TyRemoveRef >( m_var ) = std::move( _rrv );
    else
      return m_var.template emplace<_TyRemoveRef>(std::move(_rrv));
  }
  // Provide converting SetVal's for the same-sized character types.
  _TyStrChar & SetVal( _TyStrChar8 const & _r )
  {
    _TyStrChar str( (const char*)&_r[0], _r.length() );
    return SetVal( std::move( str ) );
  }
#ifdef BIEN_WCHAR_16BIT
  _TyStrChar16 & 
#else //BIEN_WCHAR_32BIT
  _TyStrChar32 & 
#endif //BIEN_WCHAR_32BIT
  SetVal( _TyStrWChar const & _r )
  {
#ifdef BIEN_WCHAR_16BIT
    _TyStrChar16 str( (const char16_t*)&_r[0], _r.length() );
#else //BIEN_WCHAR_32BIT
    _TyStrChar32 str( (const char32_t*)&_r[0], _r.length() );
#endif
    return SetVal( std::move( str ) );
  }
  _TyStrViewChar & SetVal( _TyStrViewChar8 const & _r )
  {
    _TyStrViewChar sv( (const char*)&_r[0], _r.length() );
    return SetVal( std::move( sv ) );
  }
#ifdef BIEN_WCHAR_16BIT
  _TyStrViewChar16 & 
#else //BIEN_WCHAR_32BIT
  _TyStrViewChar32 & 
#endif //BIEN_WCHAR_32BIT
  SetVal( _TyViewStrWChar const & _r )
  {
#ifdef BIEN_WCHAR_16BIT
    _TyStrViewChar16 sv( (const char16_t*)&_r[0], _r.length() );
#else //BIEN_WCHAR_32BIT
    _TyStrViewChar32 sv( (const char32_t*)&_r[0], _r.length() );
#endif
    return SetVal( std::move( sv ) );
  }
  
  // This ensures we have an array of _l_values within and then sets the size to the passed size.
  void SetSize( size_type _stNEls )
  {
    if ( !FIsArray() )
      m_var.template emplace<_TySegArrayValues>( s_knbySegArrayInit );
    get< _TySegArrayValues >( m_var ).SetSize( _stNEls );
  }
  // This fails with a throw if we don't contain an array.
  void GetSize() const
  {
    return GetValueArray().NElements();
  }
  const _TySegArrayValues & GetValueArray() const
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var ); // We let this throw a bad type exception as it will if we don't contain an array.
  }
  _TySegArrayValues & GetValueArray()
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var ); // We let this throw a bad type exception as it will if we don't contain an array.
  }
  _TyThis & operator [] ( size_type _nEl )
  {
    return GetValueArray()[_nEl];
  }
  _TyThis const & operator [] ( size_type _nEl ) const
  {
    return GetValueArray()[_nEl];
  }

  template < class t_TyToken, class t_TyStringView >
  void KGetStringView( t_TyToken & _rtok, t_TyStringView & _rsv )
  {
    static_assert( TIsStringView_v< t_TyStringView > );
    Assert( _rsv.empty() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [this,&_rtok,&_rsv](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rtok.KGetStringView( _rsv, *this );
      },
      [this,&_rsv](_TyStrChar const & _rstr)
      {
        _KGetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar const & _rsv8)
      {
        _KGetStringView( _rsv, _rsv8 );
      },
      [this,&_rsv](_TyStrChar16 const & _rstr)
      {
        _KGetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar16 const & _rsv16)
      {
        _KGetStringView( _rsv, _rsv16 );
      },
      [this,&_rsv](_TyStrChar32 const & _rstr)
      {
        _KGetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar32 const & _rsv32)
      {
        _KGetStringView( _rsv, _rsv32 );
      },
      [](_TySegArrayValues &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv]( auto _userDefinedType )
      {
        _userDefinedType.KGetStringView( _rsv, _rtok );
      }
    }, m_var );
  }
protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStrViewDest, class t_TySource >
  static void _KGetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( sizeof( typename t_TyStrViewDest::value_type) == sizeof( typename t_TySource::value_type ) )
  {
    _rsvDest = t_TyStrViewDest( (typename t_TyStrViewDest::value_type)*&_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStrViewDest, class t_TySource >
  void _KGetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( sizeof( typename t_TyStrViewDest::value_type) != sizeof( typename t_TySource::value_type ) )
  {
    static_assert( false );
    THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a constant string view of a character type not matching the character type of the lex.");
  }
public:

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
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [this,&_rtok,&_rsv](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rtok.GetStringView( _rsv, *this );
      },
      [this,&_rsv](_TyStrChar const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar const & _rsv8)
      {
        _GetStringView( _rsv, _rsv8 );
      },
      [this,&_rsv](_TyStrChar16 const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar16 const & _rsv16)
      {
        _GetStringView( _rsv, _rsv16 );
      },
      [this,&_rsv](_TyStrChar32 const & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar32 const & _rsv32)
      {
        _GetStringView( _rsv, _rsv32 );
      },
      [](_TySegArrayValues &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv]( auto _userDefinedType )
      {
        _userDefinedType.GetStringView( _rsv, _rtok );
      }
    }, m_var );
  }
protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStrViewDest, class t_TySource >
  static void _GetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( sizeof( typename t_TyStrViewDest::value_type) == sizeof( typename t_TySource::value_type ) )
  {
    _rsvDest = t_TyStrViewDest( (typename t_TyStrViewDest::value_type)*&_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStrViewDest, class t_TySource >
  void _GetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( sizeof( typename t_TyStrViewDest::value_type) != sizeof( typename t_TySource::value_type ) )
  {
    get_string_type< typename t_TyStrViewDest::value_type > strConverted;
    ConvertString( strConverted, _rsrc );
    m_var.template emplace< get_string_type< typename t_TyStrViewDest::value_type > >( std::move( strConverted ) );
    get_string_type< typename t_TyStrViewDest::value_type > & rstrInVar = std::get< get_string_type< typename t_TyStrViewDest::value_type > >( m_var );
    _rsvDest = t_TyStrViewDest( &rstrInVar[0], rstrInVar.length() );
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
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [this,&_rtok,&_rstr](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        _rtok.GetString( _rstr, *this );
      },
      [&_rstr](_TyStrChar const & _rstr8)
      {
        _GetString( _rstr, _rstr8 );
      },
      [&_rstr](_TyStrViewChar const & _rsv8)
      {
        _GetString( _rstr, _rsv8 );
      },
      [&_rstr](_TyStrChar16 const & _rstr16)
      {
        _GetString( _rstr, _rstr16 );
      },
      [&_rstr](_TyStrViewChar16 const & _rsv16)
      {
        _GetString( _rstr, _rsv16 );
      },
      [&_rstr](_TyStrChar32 const & _rstr32)
      {
        _GetString( _rstr, _rstr32 );
      },
      [&_rstr](_TyStrViewChar32 const & _rsv32)
      {
        _GetString( _rstr, _rsv32 );
      },
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv]( auto _userDefinedType )
      {
        _userDefinedType.GetString( _rstr, _rtok );
      }
    }, m_var );
  }

protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStringDest, class t_TySource >
  static void _GetString( t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( sizeof( typename t_TyStringDest::value_type ) == sizeof( typename t_TySource::value_type ) )
  {
    _rstrDest.assign( (typename t_TyStringDest::value_type*)&_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStringDest, class t_TySource >
  static void _GetString( t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( sizeof( typename t_TyStringDest::value_type ) != sizeof( typename t_TySource::value_type ) )
  {
    ConvertString( _rstrDest, _rsrc ); // that was easy.
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
        THROWNAMEDBADVARIANTACCESSEXCEPTION("empty contains no data.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [this,&_rtok,&_rsv,&_rstr](_TyData const & _rdt)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        return _rtok.FGetStringViewOrString( _rsv, _rstr, *this );
      },
      [&_rsv,&_rstr](_TyStrChar const & _rstr8)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr8 );
      },
      [&_rsv,&_rstr](_TyStrViewChar const & _rsv8)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv8 );
      },
      [&_rsv,&_rstr](_TyStrChar16 const & _rstr16)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr16 );
      },
      [&_rsv,&_rstr](_TyStrViewChar16 const & _rsv16)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv16 );
      },
      [&_rsv,&_rstr](_TyStrChar32 const & _rstr32)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rstr32 );
      },
      [&_rsv,&_rstr](_TyStrViewChar32 const & _rsv32)
      {
        return _FGetStringViewOrString( _rsv, _rstr, _rsv32 );
      },
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv,&_rstr]( auto _userDefinedType )
      {
        return _userDefinedType.FGetStringViewOrString( _rsv, _rstr, _rtok );
      }
    }, m_var );
  }
protected:
  // Non-converting version for source strings and stringviews:
  template < class t_TyStringViewDest, class t_TyStringDest, class t_TySource >
  static bool _FGetStringViewOrString( t_TyStringViewDest & _rsvDest, t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( sizeof( typename t_TyStringDest::value_type ) == sizeof( typename t_TySource::value_type ) )
  {
    static_assert( sizeof( typename t_TyStringViewDest::value_type ) == sizeof( typename t_TyStringDest::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    _rsvDest = t_TyStrViewDest( (const t_TyStringViewDest::value_type*)&_rsrc[0], _rsrc.length() );
    return true;
  }
  // Converting version for string or view source - we must convert the existing value into the passed string.
  template < class t_TyStringViewDest, class t_TyStringDest, class t_TySource >
  static bool _FGetStringViewOrString( t_TyStringViewDest & _rsvDest, t_TyStringDest & _rstrDest, const t_TySource & _rsrc )
    requires ( sizeof( typename t_TyStringDest::value_type ) != sizeof( typename t_TySource::value_type ) )
  {
    static_assert( sizeof( typename t_TyStringViewDest::value_type ) == sizeof( typename t_TyStringDest::value_type ) );
    Assert( _rsvDest.empty() );
    Assert( _rstrDest.empty() );
    ConvertString( _rstrDest, _rsrc ); // that was easy.
    return false;
  }
public:

  // Output the data to a JsoValue.
  template < class t_TyCharOut >
  void ToJsoValue( JsoValue< t_TyCharOut > & _rjv ) const
  {
    Assert( _rjv.FIsNullOrEmpty() );
    std::visit(_VisitHelpOverloadFCall {
      [&_rjv](monostate) 
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
      [&_rjv](_TyStrChar const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrChar16 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar16 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrChar32 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TyStrViewChar32 const & _rstr)
      {
        _rjv.SetStringValue( _rstr );
      },
      [&_rjv](_TySegArrayValues const & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        _rjv.SetArrayCapacity( knEls ); // preallocate
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].ToJsoValue( _rjv.CreateOrGetEl( nEl ) );
      },
      [&_rjv]( auto _userDefinedType )
      {
        _userDefinedType.ToJsoValue( _rjv );
      }
    }, m_var );
  }

  // Convert stream position ranges to strings throughout the entire value (i.e. recursively) using the stream _rs.
  template < class t_TyCharOut = _TyChar, class t_TyToken >
  void ProcessStrings( t_TyToken & _rtok )
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) {},
      [](bool _f) {},
      [](vtyDataPosition _dp) {},
      [this,&_rtok](_TyData const & _rdt)
      {
        // Delegate to the token in this case:
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        basic_string_view< t_TyCharOut > sv;
        _rtok.GetStringView( sv, *this );
      },
      [this](_TyStrChar const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrChar16 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar16 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rtok](_TySegArrayValues & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].template ProcessStrings< t_TyCharOut >( _rtok );
      },
      [&_rtok]( auto _userDefinedType )
      {
        _userDefinedType.ProcessStrings( _rtok );
      }
    }, m_var );
  }

protected:
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  static void _ConvertStringValue( const get_string_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) == sizeof( t_TyCharConvertTo ) )
  { // no-op.
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  void _ConvertStringValue( const get_string_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) != sizeof( t_TyCharConvertTo ) )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rstr );
    m_var.template emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  static void _ConvertStringValue( const get_string_view_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) == sizeof( t_TyCharConvertTo ) )
  { // no-op.
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  void _ConvertStringValue( const get_string_view_type< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) != sizeof( t_TyCharConvertTo ) )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rstr );
    m_var.template emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  _TyVariant m_var;
};

#ifdef WIN32
template< class t_TyTraits >
inline const size_t
_l_value< t_TyTraits >::s_knbySegArrayInit = _l_value::s_knValsSegSize * sizeof(_l_value);
#endif //WIN32

namespace std
{
  // override std::swap so that it is efficient:
  template < class t_TyChar, size_t s_knbySegSize >
  void swap( _l_data< t_TyChar, s_knbySegSize > & _rl, _l_data< t_TyChar, s_knbySegSize > & _rr )
  {
    _rl.swap( _rr );
  }
}

__LEXOBJ_END_NAMESPACE

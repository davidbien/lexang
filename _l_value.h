#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
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

// About 4% faster on the unit tests in retail using the LogArray impl. Not scientific but significant.
//#define _L_VALUE_USE_SEGARRAY

#ifdef _L_VALUE_USE_SEGARRAY
#include "segarray.h"
#else //!_L_VALUE_USE_SEGARRAY
#include "_logarray.h"
#endif //!_L_VALUE_USE_SEGARRAY

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyChar >
struct _l_value_get_string_type
{
  typedef basic_string< t_TyChar > type;
};
template < >
struct _l_value_get_string_type< char >
{
  typedef basic_string< char8_t > type;
};
template < >
struct _l_value_get_string_type< wchar_t >
{
#ifdef BIEN_WCHAR_16BIT
  typedef basic_string< char16_t > type;
#else //!BIEN_WCHAR_16BIT
  typedef basic_string< char32_t > type;
#endif //!BIEN_WCHAR_16BIT
};
template < class t_TyChar >
struct _l_value_get_string_view_type
{
  typedef basic_string_view< t_TyChar > type;
};
template < >
struct _l_value_get_string_view_type< char >
{
  typedef basic_string_view< char8_t > type;
};
template < >
struct _l_value_get_string_view_type< wchar_t >
{
#ifdef BIEN_WCHAR_16BIT
  typedef basic_string_view< char16_t > type;
#else //!BIEN_WCHAR_16BIT
  typedef basic_string_view< char32_t > type;
#endif //!BIEN_WCHAR_16BIT
};

typedef int64_t vtySignedLvalueInt;

template< class t_TyChar, class t_TyTpValueTraits, size_t t_knValsSegSize >
class _l_value
{
  typedef _l_value _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef t_TyTpValueTraits _TyTpValueTraits;
  typedef _l_data<> _TyData;
#ifdef _L_VALUE_USE_SEGARRAY
  static constexpr size_t s_knValsSegSize = t_knValsSegSize;
#ifdef _MSC_VER
   // vc has a problem with asking for the size of an incomplete type here.
  static const size_t s_knbySegArrayInit;
#else // gcc and clang like it fine.
  static constexpr size_t s_knbySegArrayInit = s_knValsSegSize * sizeof(_TyThis);
#endif
  typedef SegArray< _TyThis, std::true_type > _TySegArrayValues;
  typedef typename _TySegArrayValues::_tySizeType size_type;
#else // !_L_VALUE_USE_SEGARRAY
  // Use the LogArray<> instead as it is more stingy on memory.
  typedef LogArray< _TyThis, 2, 4 > _TySegArrayValues;
  typedef size_t size_type;
#endif // !_L_VALUE_USE_SEGARRAY
  // These are the possible values of a action object.
  // We add all three types of STL strings to the variant - at least under linux - char, char16_t and char32_t.
  // Specifying them in this manner should allow for no code changes under Windows even though wchar_t is char16_t under Windows.
  typedef basic_string< char > _TyStrChar;
  typedef basic_string< char8_t > _TyStrChar8;
  typedef basic_string< char16_t > _TyStrChar16;
  typedef basic_string< wchar_t > _TyStrWChar;
  typedef basic_string< char32_t > _TyStrChar32;
  template < class t__TyChar >
  using get_string_type = typename _l_value_get_string_type< t__TyChar >::type;
  typedef basic_string< _TyChar > _TyStdString; // The impl's string.

  // Also allow basic_string_views of every type since that can be used until character translation is needed.
  typedef basic_string_view< char > _TyStrViewChar;
  typedef basic_string_view< char8_t > _TyStrViewChar8;
  typedef basic_string_view< char16_t > _TyStrViewChar16;
  typedef basic_string_view< wchar_t > _TyStrViewWChar;
  typedef basic_string_view< char32_t > _TyStrViewChar32;
  template < class t__TyChar >
  using get_string_view_type = typename _l_value_get_string_view_type< t__TyChar >::type;
  typedef basic_string_view< _TyChar > _TyStdStringView;

  // Our big old variant.
  // Use monostate to allow an "empty" value here.
  // First define the "base" variant which doesn't include any user defined types.
  // We'd like to keep the number of items in this less than 16 since that causes the internal impl of the variant to change after that point.
  // If you look at the impl it seems that it is best to put the most used variant values first.
  typedef variant<  monostate, _TySegArrayValues, _TyData, vtyDataPosition, bool, vtySignedLvalueInt, 
                    _TyStrChar8, _TyStrViewChar8,
                    _TyStrChar16, _TyStrViewChar16,
                    _TyStrChar32, _TyStrViewChar32 > _TyVariantBase;
  // Now define our full variant type by concatenating on any user defined types.
  typedef typename concatenator_pack< _TyVariantBase, _TyTpValueTraits > ::type _TyVariant;

  _l_value() = default;
  // Note that, while we allow copying, the user values in _TyTpValueTraits may not.
  _l_value(_l_value const & ) = default;
  _l_value & operator =( _l_value const & ) = default;
  _l_value( _l_value && _rr )
  {
    swap( _rr );
  }
  _l_value & operator =( _l_value && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    m_var.swap( _r.m_var );
  }
  template < class t_TyContainerNew, class t_TyValueOther >
  _l_value( t_TyContainerNew & _rNewContainer, t_TyValueOther const & _rOther, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
  {
    _CopyFrom( _rNewContainer, _rOther, _ptccCopyCtxt );
  }
  template < class t_TyContainerNew, class t_TyValueOther >
  _l_value( t_TyContainerNew & _rNewContainer, t_TyValueOther && _rrOther, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt = nullptr )
  {
    _MoveFrom( _rNewContainer, std::move( _rrOther ), _ptccCopyCtxt );
  }
  // Construct contained variant with type:
  template < class t_TyContained >
  _l_value( const t_TyContained & _rt )
    : m_var( _rt )
  {
  }
  template < class t_TyContained >
  _l_value( t_TyContained && _rrt )
    : m_var( std::move( _rrt ) )
  {
  }
  bool FIsNull() const
  {
    return holds_alternative<monostate>( m_var );
  }
  void Clear()
  {
    m_var.template emplace<monostate>();
  }
  _TyVariant & GetVariant()
  {
    return m_var;
  }
  const _TyVariant & GetVariant() const
  {
    return m_var;
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
  bool FIsBool() const
  {
    return FIsA< bool >();
  }
  bool FIsString() const
  {
    return  holds_alternative<_TyStrChar8>( m_var ) || holds_alternative<_TyStrViewChar8>( m_var ) ||
            holds_alternative<_TyStrChar16>( m_var ) || holds_alternative<_TyStrViewChar16>( m_var ) ||
            holds_alternative<_TyStrChar32>( m_var ) || holds_alternative<_TyStrViewChar32>( m_var );
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
  _TyData & GetTypedData()
  {
    return GetVal<_TyData>();
  }
  const _TyData & GetTypedData() const
  {
    return GetVal<_TyData>();
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
  // REVIEW:<dbien>: The below is clunky - rework it at some point.
  // Provide converting SetVal's for the same-sized character types.
  _TyStrChar8 & SetVal( _TyStrChar const & _r )
  {
    _TyStrChar8 str( (const char8_t*)&_r[0], _r.length() );
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
  _TyStrViewChar8 & SetVal( _TyStrViewChar const & _r )
  {
    _TyStrViewChar8 sv( (const char8_t*)&_r[0], _r.length() );
    return SetVal( std::move( sv ) );
  }
#ifdef BIEN_WCHAR_16BIT
  _TyStrViewChar16 & 
#else //BIEN_WCHAR_32BIT
  _TyStrViewChar32 & 
#endif //BIEN_WCHAR_32BIT
  SetVal(_TyStrViewWChar const & _r )
  {
#ifdef BIEN_WCHAR_16BIT
    _TyStrViewChar16 sv( (const char16_t*)&_r[0], _r.length() );
#else //BIEN_WCHAR_32BIT
    _TyStrViewChar32 sv( (const char32_t*)&_r[0], _r.length() );
#endif
    return SetVal( sv );
  }
  // If this isn't an array then make it an array.
  _TySegArrayValues & SetArray()
  {
    if ( !FIsArray() )
#ifdef _L_VALUE_USE_SEGARRAY
      return m_var.template emplace<_TySegArrayValues>( s_knbySegArrayInit );
#else //!_L_VALUE_USE_SEGARRAY
      return m_var.template emplace<_TySegArrayValues>();
#endif //!_L_VALUE_USE_SEGARRAY
    else
      return GetValueArray();
  }
  // This ensures we have an array of _l_values within and then sets the size to the passed size.
  void SetSize( size_type _stNEls )
  {
    SetArray();
    get< _TySegArrayValues >( m_var ).SetSize( _stNEls );
  }
  // This fails with a throw if we don't contain an array.
  size_t GetSize() const
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
  // Non-const method auto-resizes the array.
  _TyThis & operator [] ( size_type _nEl )
  {
    if ( _nEl >= GetSize() )
       SetSize( _nEl + 1 );
    return GetValueArray()[_nEl];
  }
  _TyThis const & operator [] ( size_type _nEl ) const
  {
    return GetValueArray()[_nEl];
  }
  // emplace an element at the end of the current array. Must already be an array.
  template < class ... t_TysArgs >
  _TyThis & emplace_back( t_TysArgs ... _args )
  {
    return GetValueArray().emplaceAtEnd( std::forward< t_TysArgs >( _args ) ... );
  }
  // This returns a copy of the current value, however it will throw if the current value contains a non-copyable user value type.
  // Non-throwing version:
  _l_value GetCopy() const
    requires( is_copy_constructible_v< _TyVariant > )
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) -> _l_value
      {
        return _l_value();
      },
      [](bool _f) -> _l_value
      {
        return _l_value( _f );
      },
      [](vtyDataPosition _pos) -> _l_value
      {
        return _l_value( _pos );
      },
      [](vtySignedLvalueInt _si) -> _l_value
      {
        return _l_value( _si );
      },
      [](_TyData const & _dt) -> _l_value
      {
        return _l_value( _dt );
      },
      [](_TyStrChar8 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar8 const & _rsv8) -> _l_value
      {
        return _l_value( _rsv8 );
      },
      [](_TyStrChar16 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar16 const & _rsv16) -> _l_value
      {
        return _l_value( _rsv16 );
      },
      [](_TyStrChar32 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar32 const & _rsv32) -> _l_value
      {
        return _l_value( _rsv32 );
      },
      [](_TySegArrayValues const & _rsa) -> _l_value
      {
        return _l_value( _rsa );
      },
      []( const auto & _userDefinedType ) -> _l_value
      {
        return _l_value( _userDefinedType );
      }
    }, m_var );
  }
  // Throwing version:.
  _l_value GetCopy() const
    requires( !is_copy_constructible_v< _TyVariant > )
  {
    return std::visit(_VisitHelpOverloadFCall {
      [](monostate) -> _l_value
      {
        return _l_value();
      },
      [](bool _f) -> _l_value
      {
        return _l_value( _f );
      },
      [](vtyDataPosition _pos) -> _l_value
      {
        return _l_value( _pos );
      },
      [](vtySignedLvalueInt _si) -> _l_value
      {
        return _l_value( _si );
      },
      [](_TyData const & _dt) -> _l_value
      {
        return _l_value( _dt );
      },
      [](_TyStrChar8 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar8 const & _rsv8) -> _l_value
      {
        return _l_value( _rsv8 );
      },
      [](_TyStrChar16 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar16 const & _rsv16) -> _l_value
      {
        return _l_value( _rsv16 );
      },
      [](_TyStrChar32 const & _rstr) -> _l_value
      {
        return _l_value( _rstr );
      },
      [](_TyStrViewChar32 const & _rsv32) -> _l_value
      {
        return _l_value( _rsv32 );
      },
      [](_TySegArrayValues const & _rsa) -> _l_value
      {
        VerifyThrowSz( false, "GetCopy() throwing due to user-defined types not being copyable.");
        return _l_value();
      },
      []( const auto & _userDefinedType ) -> _l_value
      {
        VerifyThrowSz( false, "GetCopy() throwing due to user-defined types not being copyable.");
        return _l_value();
      }
    }, m_var );
  }
  template < class t_TyToken, class t_TyStringView >
  void KGetStringView( t_TyToken & _rtok, t_TyStringView & _rsv ) const
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
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
      },
      [this,&_rtok,&_rsv](_TyData const &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rtok.KGetStringView( _rsv, *this );
      },
      [this,&_rsv](_TyStrChar8 const & _rstr)
      {
        _KGetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar8 const & _rsv8)
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
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv]( const auto & _userDefinedType )
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
    _rsvDest = t_TyStrViewDest( (typename t_TyStrViewDest::value_type*)*&_rsrc[0], _rsrc.length() );
  }
  // Converting version for string or view source - we must convert the existing value to the desired view's type and then return a view on the new value.
  template < class t_TyStrViewDest, class t_TySource >
  static void _KGetStringView( t_TyStrViewDest & _rsvDest, t_TySource const & _rsrc )
    requires ( sizeof( typename t_TyStrViewDest::value_type) != sizeof( typename t_TySource::value_type ) )
  {
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
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
      },
      [this,&_rtok,&_rsv](_TyData &)
      {
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate:
        _rtok.GetStringView( _rsv, *this );
      },
      [this,&_rsv](_TyStrChar8 & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar8 & _rsv8)
      {
        _GetStringView( _rsv, _rsv8 );
      },
      [this,&_rsv](_TyStrChar16 & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar16 & _rsv16)
      {
        _GetStringView( _rsv, _rsv16 );
      },
      [this,&_rsv](_TyStrChar32 & _rstr)
      {
        _GetStringView( _rsv, _rstr );
      },
      [this,&_rsv](_TyStrViewChar32 & _rsv32)
      {
        _GetStringView( _rsv, _rsv32 );
      },
      [](_TySegArrayValues &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string view of an array of _l_values.");
      },
      [&_rtok,&_rsv]( auto & _userDefinedType )
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
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
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
      [&_rtok,&_rstr]( const auto & _userDefinedType )
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
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
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
      [&_rtok,&_rsv,&_rstr]( const auto & _userDefinedType )
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
    _rsvDest = t_TyStrViewDest( (const typename t_TyStringViewDest::value_type*)&_rsrc[0], _rsrc.length() );
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
    _rsvDest = t_TyStrViewDest( (const typename t_TyStringViewDest::value_type*)&_rstrDest[0], _rstrDest.length() );
    return false;
  }
public:
  // This applies the functor to a string type - specifically - i.e. one of the six string and string view types.
  // Application to any other type will cause a throw.
  template < class t_TyFunctor >
  void ApplyString( t_TyFunctor && _rrftor ) const // const version
  {
    Assert( FIsString() );
    std::visit( _VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("monostate has no strings.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
      },
      [](_TySegArrayValues const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string from an array of _l_values.");
      },
      [](_TyData const &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("ApplyString() only works on string types.");
      },
      []( const auto & _userDefinedType )
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("ApplyString() only works on string types - not any user defined types that have been grafted in.");
      },
      // We must spell out all the various string types instead of using auto since
      //  we may have user defined types and we must reserve the use of auto for them.
      // requires statements would get around this and I'll do that later probably.
      [_rrftor = FWD_CAPTURE(_rrftor)]( _TyStrChar8 const & _rstr )
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar8 const & _rsv8)
      {
        access_fwd( _rrftor )( &_rsv8[0], &_rsv8[0] + _rsv8.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrChar16 const & _rstr)
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar16 const & _rsv16)
      {
        access_fwd( _rrftor )( &_rsv16[0], &_rsv16[0] + _rsv16.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrChar32 const & _rstr)
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar32 const & _rsv32)
      {
        access_fwd( _rrftor )( &_rsv32[0], &_rsv32[0] + _rsv32.length() );
      }
    }, m_var );
  }
  template < class t_TyFunctor >
  void ApplyString( t_TyFunctor && _rrftor ) // non-const version
  {
    Assert( FIsString() );
    std::visit( _VisitHelpOverloadFCall {
      [](monostate) 
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("monostate has no strings.");
      },
      [](bool)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("bool is not a string.");
      },
      [](vtyDataPosition)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtyDataPosition is not a string.");
      },
      [](vtySignedLvalueInt)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("vtySignedLvalueInt is not a string.");
      },
      [](_TySegArrayValues &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("Can't get a string from an array of _l_values.");
      },
      [](_TyData &)
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("ApplyString() only works on string types.");
      },
      []( auto & _userDefinedType )
      {
        THROWNAMEDBADVARIANTACCESSEXCEPTION("ApplyString() only works on string types - not any user defined types that have been grafted in.");
      },
      // We must spell out all the various string types instead of using auto since
      //  we may have user defined types and we must reserve the use of auto for them.
      // requires statements would get around this and I'll do that later probably.
      [_rrftor = FWD_CAPTURE(_rrftor)]( _TyStrChar8 & _rstr )
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar8 & _rsv8)
      {
        access_fwd( _rrftor )( &_rsv8[0], &_rsv8[0] + _rsv8.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrChar16 & _rstr)
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar16 & _rsv16)
      {
        access_fwd( _rrftor )( &_rsv16[0], &_rsv16[0] + _rsv16.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrChar32 & _rstr)
      {
        access_fwd( _rrftor )( &_rstr[0], &_rstr[0] + _rstr.length() );
      },
      [_rrftor = FWD_CAPTURE(_rrftor)](_TyStrViewChar32 & _rsv32)
      {
        access_fwd( _rrftor )( &_rsv32[0], &_rsv32[0] + _rsv32.length() );
      }
    }, m_var );
  }
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
      [&_rjv](vtySignedLvalueInt _i)
      {
        _rjv.SetValue( _i );
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
      [&_rjv]( const auto & _userDefinedType )
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
      [](vtySignedLvalueInt _i) {},
      [this,&_rtok](_TyData & _rdt)
      {
        // Delegate to the token in this case:
        // _rdt might hold a single _l_data_typed_range or an array of _l_data_typed_range, delegate.
        basic_string_view< t_TyCharOut > sv;
        _rtok.GetStringView( sv, *this );
      },
      [this](_TyStrChar8 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar8 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrChar16 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar16 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrChar32 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [this](_TyStrViewChar32 & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rtok](_TySegArrayValues & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].template ProcessStrings< t_TyCharOut >( _rtok );
      },
      [&_rtok]( auto & _userDefinedType )
      {
        _userDefinedType.ProcessStrings( _rtok );
      }
    }, m_var );
  }
  // Count the number of positions in the aggregate object. A single _l_data_range has 2 positions.
  size_t CountDataPositions() const
  {
    size_t nPositions = 0;
    std::visit(_VisitHelpOverloadFCall {
      []( const auto &  ) { },
      [&nPositions]( vtyDataPosition _dp )
      {
        ++nPositions;
      },
      [&nPositions]( _TyData const & _rdt )
      {
        nPositions += _rdt.NPositions() * 2; // each position is a (beg,end) pair.
      },
      [&nPositions](_TySegArrayValues const & _rrg)
      {
        _rrg.ApplyContiguous( 0, _rrg.NElements(), 
          [&nPositions]( _TyThis const * _pvalBegin, _TyThis const * _pvalEnd )
          {
            for ( _TyThis const * pvalCur = _pvalBegin; _pvalEnd != pvalCur; ++pvalCur )
              nPositions += pvalCur->CountDataPositions();
          }
        );
      },
    }, m_var );
    return nPositions;
  }
  // return a sorted array of pointers to all positions within the aggregate value.
  // The caller should allocate CountDataPositions()+1 so we can use the first element for the algorithm.
  typedef pair< vtyDataPosition **, vtyDataPosition ** > _TyPrPtrDataPosition;
  void GetSortedPositionPtrs( _TyPrPtrDataPosition & _rprpptrDP )
  {
    Assert( ( _rprpptrDP.second - _rprpptrDP.first ) == CountDataPositions()+1 ); // This assumes that the caller wants a pointer to all the positions.
    bool fIsSorted = true; // whether things are currently sorted - they mostly will be since the aggregate is formed from sequential data.
    vtyDataPosition ** ppdpFirst = _rprpptrDP.first; // squirrel away first here.
    vtyDataPosition posBeginNull = 0;
    *_rprpptrDP.first++ = &posBeginNull; // For the algorhtm we don't need to test for the beginning of the array while testing for fIsSorted.
    _GetPositionPtrs( fIsSorted, _rprpptrDP );
    // Assert( _rprpptrDP.first == _rprpptrDP.second ); // Should have filled it up - actually no if there are null positions.
    _rprpptrDP.second = _rprpptrDP.first; // account for the potential of null positions within the set of positions.
    _rprpptrDP.first = ppdpFirst; // restore.
    if ( !fIsSorted )
    {
      std::sort( _rprpptrDP.first+1, _rprpptrDP.second,
        []( vtyDataPosition * _pdpLeft, vtyDataPosition * _pdpRight ) 
        {
          return *_pdpLeft < *_pdpRight;
        }
      );
    }
  }
protected:
// recurses.
  template < class t_TyContainerNew, class t_TyValueOther >
  void _CopyFrom( t_TyContainerNew & _rNewContainer, t_TyValueOther const & _rOther, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt )
  {
    Assert( FIsNull() );
    // We must enumerate all held types to single out any user added types:
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) { },
      [this]( bool _f )
      {
        SetVal( _f );
      },
      [this]( vtyDataPosition _dp )
      {
        SetVal( _dp );
      },
      [this]( vtySignedLvalueInt _i )
      {
        SetVal( _i );
      },
      [this]( _TyData const & _rdt )
      {
        SetVal( _rdt );
      },
      [this](_TyStrChar8 const & _rstr)
      {
        SetVal( _rstr );
      },
      [this](_TyStrViewChar8 const & _rsv8)
      {
        SetVal( _rsv8 );
      },
      [this](_TyStrChar16 const & _rstr)
      {
        SetVal( _rstr );
      },
      [this](_TyStrViewChar16 const & _rsv16)
      {
        SetVal( _rsv16 );
      },
      [this](_TyStrChar32 const & _rstr)
      {
        SetVal( _rstr );
      },
      [this](_TyStrViewChar32 const & _rsv32)
      {
        SetVal( _rsv32 );
      },
      [this,&_rNewContainer,_ptccCopyCtxt]( typename t_TyValueOther::_TySegArrayValues const & _rsaOther )
      {
        _TyThis * plvalContainerOld;
        if ( _ptccCopyCtxt )
        {
          plvalContainerOld = _ptccCopyCtxt->m_plvalContainerCur;
          _ptccCopyCtxt->m_plvalContainerCur = this;
        }
        _TySegArrayValues & rsaThis = SetArray();
        _rsaOther.ApplyContiguous( 0, _rsaOther.NElements(), 
          [&rsaThis,&_rNewContainer,_ptccCopyCtxt]( t_TyValueOther const * _pvalBegin, t_TyValueOther const * const _pvalEnd )
          {
            for ( t_TyValueOther const * pvalCur = _pvalBegin; _pvalEnd != pvalCur; ++pvalCur )
              rsaThis.emplaceAtEnd( _rNewContainer, *pvalCur, _ptccCopyCtxt );
          }
        );
        if ( _ptccCopyCtxt )
          _ptccCopyCtxt->m_plvalContainerCur = plvalContainerOld;
      },
      [this,&_rNewContainer,_ptccCopyCtxt]( const auto & _userDefinedTypeOther )
      {
        // The container must set the type here after translating it appropriately if necessary.
        _rNewContainer.TranslateUserType( _ptccCopyCtxt, _userDefinedTypeOther, *this );
      }
    }, _rOther.GetVariant() );
  }
  template < class t_TyContainerNew, class t_TyValueOther >
  void _MoveFrom( t_TyContainerNew & _rNewContainer, t_TyValueOther && _rrOther, typename t_TyContainerNew::_TyTokenCopyContext * _ptccCopyCtxt )
  {
    Assert( FIsNull() );
    // We must enumerate all held types to single out any user added types:
    std::visit(_VisitHelpOverloadFCall {
      [](monostate) { },
      [this]( bool _f )
      {
        SetVal( _f );
      },
      [this]( vtyDataPosition _dp )
      {
        SetVal( _dp );
      },
      [this]( vtySignedLvalueInt _i )
      {
        SetVal( _i );
      },
      [this]( _TyData & _rdt )
      {
        emplaceVal( std::move( _rdt ) );
      },
      [this](_TyStrChar8 & _rstr)
      {
        emplaceVal( std::move( _rstr ) );
      },
      [this](_TyStrViewChar8 & _rsv)
      {
        emplaceVal( _rsv );
      },
      [this](_TyStrChar16 & _rstr)
      {
        emplaceVal( std::move( _rstr ) );
      },
      [this](_TyStrViewChar16 & _rsv)
      {
        emplaceVal( _rsv );
      },
      [this](_TyStrChar32 & _rstr)
      {
        emplaceVal( std::move( _rstr ) );
      },
      [this](_TyStrViewChar32 & _rsv)
      {
        emplaceVal( _rsv );
      },
      [this,&_rNewContainer,_ptccCopyCtxt]( typename t_TyValueOther::_TySegArrayValues & _rsaOther )
      {
        _TyThis * plvalContainerOld;
        if ( _ptccCopyCtxt )
        {
          plvalContainerOld = _ptccCopyCtxt->m_plvalContainerCur;
          _ptccCopyCtxt->m_plvalContainerCur = this;
        }
        // We can't move the segarray of values - even if it is the same type than our internal segarray - since
        //  we are changing containers - and so we must let the container translate any user defined objects.
        // If the container doesn't matter and we aren't changing types then just use the move constructor.
        // We move each resultant _l_value<> object encountered in the aggregate.
        _TySegArrayValues & rsaThis = SetArray();
        _rsaOther.ApplyContiguous( 0, _rsaOther.NElements(), 
          [&rsaThis,&_rNewContainer,_ptccCopyCtxt]( t_TyValueOther * _pvalBegin, t_TyValueOther * const _pvalEnd )
          {
            for ( t_TyValueOther * pvalCur = _pvalBegin; _pvalEnd != pvalCur; ++pvalCur )
              rsaThis.emplaceAtEnd( _rNewContainer, std::move( *pvalCur ), _ptccCopyCtxt );
          }
        );
        if ( _ptccCopyCtxt )
          _ptccCopyCtxt->m_plvalContainerCur = plvalContainerOld;
      },
      [this,&_rNewContainer,_ptccCopyCtxt]( auto & _userDefinedTypeOther )
      {
        // The container must set the type here after translating it appropriately if necessary.
        _rNewContainer.TranslateUserType( _ptccCopyCtxt, std::move( _userDefinedTypeOther ), *this );
      }
    }, _rrOther.GetVariant() );
  }
  void _GetPositionPtrs( bool & _rfIsSorted, _TyPrPtrDataPosition & _rprpptrDP )
  {
    std::visit(_VisitHelpOverloadFCall {
      []( auto &  ) { },
      [&_rfIsSorted,&_rprpptrDP]( vtyDataPosition & _rdp )
      {
        if ( _rfIsSorted )
          _rfIsSorted = _rdp >= *_rprpptrDP.first[-1];
        if ( _rprpptrDP.first < _rprpptrDP.second )
          *_rprpptrDP.first++ = &_rdp;
      },
      [&_rfIsSorted,&_rprpptrDP]( _TyData & _rdt )
      {
        _rdt._GetPositionPtrs( _rfIsSorted, _rprpptrDP ); // not for public consumption because it is optimized to work with this method.
      },
      [&_rfIsSorted,&_rprpptrDP]( _TySegArrayValues & _rrg )
      {
        _rrg.ApplyContiguous( 0, _rrg.NElements(), 
          [&_rfIsSorted,&_rprpptrDP]( _TyThis * _pvalBegin, _TyThis * _pvalEnd )
          {
            for ( _TyThis * pvalCur = _pvalBegin; _pvalEnd != pvalCur; ++pvalCur )
              pvalCur->_GetPositionPtrs( _rfIsSorted, _rprpptrDP );
          }
        );
      },
    }, m_var );
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  static void _ConvertStringValue( const basic_string< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) == sizeof( t_TyCharConvertTo ) )
  { // no-op.
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  void _ConvertStringValue( const basic_string< t_TyCharConvertFrom > & _rstr ) 
    requires ( sizeof( t_TyCharConvertFrom ) != sizeof( t_TyCharConvertTo ) )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rstr );
    m_var.template emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  static void _ConvertStringValue( const basic_string_view< t_TyCharConvertFrom > & _rsv ) 
    requires ( sizeof( t_TyCharConvertFrom ) == sizeof( t_TyCharConvertTo ) )
  { // no-op.
  }
  template < class t_TyCharConvertTo, class t_TyCharConvertFrom  >
  void _ConvertStringValue( const basic_string_view< t_TyCharConvertFrom > & _rsv ) 
    requires ( sizeof( t_TyCharConvertFrom ) != sizeof( t_TyCharConvertTo ) )
  {
    get_string_type< t_TyCharConvertTo > strConverted;
    ConvertString( strConverted, _rsv );
    m_var.template emplace< get_string_type< t_TyCharConvertTo > >( std::move( strConverted ) );
  }
  _TyVariant m_var;
};

#ifdef _L_VALUE_USE_SEGARRAY
#ifdef WIN32
template< class t_TyChar, class t_TyTpValueTraits, size_t t_knValsSegSize >
inline const size_t
_l_value< t_TyChar, t_TyTpValueTraits, t_knValsSegSize >::s_knbySegArrayInit = _l_value::s_knValsSegSize * sizeof(_l_value);
#endif //WIN32
#endif //_L_VALUE_USE_SEGARRAY

__LEXOBJ_END_NAMESPACE

namespace std
{
  __LEXOBJ_USING_NAMESPACE
  template< class t_TyChar, class t_TyTpValueTraits, size_t t_knValsSegSize >
  void swap(_l_value< t_TyChar, t_TyTpValueTraits, t_knValsSegSize >& _rl, _l_value< t_TyChar, t_TyTpValueTraits, t_knValsSegSize >& _rr)
  {
    _rl.swap(_rr);
  }
}

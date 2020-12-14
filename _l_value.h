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

__REGEXP_BEGIN_NAMESPACE

template< class... Ts > 
struct _VisitHelpOverloadFCall : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> _VisitHelpOverloadFCall(Ts...) -> _VisitHelpOverloadFCall<Ts...>;

template< class t_TyChar, size_t s_knbySegSize = 1024 >
class _l_value
{
  typedef _l_value _TyThis;
public:
  typedef _l_data< t_TyChar > _TyData;
  static constexpr size_t s_knSegArrayInit = s_knbySegSize;
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

  typedef variant< monostate, bool, vtyDataPosition, _TyData, _TyStrChar8, _TyStrViewChar8, _TyStrChar16, _TyStrViewChar16, _TyStrChar32, _TyStrViewChar32, _TySegArrayValues > _TyVariant;

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

  // These should work in many cases.
  template < class t_Ty >
  SetValue( t_Ty const & _rv )
  {
    m_var.emplace<t_Ty>( _rv );
  }
  template < class t_Ty >
  SetValue( t_Ty && _rrv )
  {
    m_var.emplace<t_Ty>(std::move(_rrv));
  }

  // This ensures we have an array of _l_values within and then sets the size to the passed size.
  void SetSize( size_type _stNEls )
  {
    if ( !FIsArray() )
      m_var.emplace<_TySegArrayValues>( s_knSegArrayInit );
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

  // Output the data to a JsoValue.
  template < class t_TyCharOut >
  void ToJsoValue( JsoValue< t_TyCharOut > & _rjv ) const
  {
    Assert( _rjv.FIsNull() );
    std::visit(_VisitHelpOverloadFCall {
      [](monostate _ms) {},
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
    }, m_var );
  }

  // Convert stream position ranges to strings throughout the entire value (i.e. recursively) using the stream _rs.
  // If there are strings that are present which are not in the representation t_TyCharOut then these are converted to t_TyCharOut.
  template < class t_TyStream, class t_TyCharOut = t_TyChar >
  void ConvertStrings( t_TyStream & _rs )
  {
    std::visit(_VisitHelpOverloadFCall {
      [](monostate _ms) {},
      [](bool _f) {},
      [&_rjv](vtyDataPosition _dp) {},
      [&_rjv](_TyData const & _rdt)
      {
        // Allow the stream to decide how to deal with any set of strings that might be present.
        _rs.ConvertStrings< t_TyCharOut >( *this );
      },
      [&_rjv](_TyStrChar8 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrViewChar8 const & _rstr)
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
      [&_rjv](_TyStrChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TyStrViewChar32 const & _rstr)
      {
        _ConvertStringValue< t_TyCharOut >( _rstr );
      },
      [&_rjv](_TySegArrayValues const & _rrg)
      {
        const size_type knEls = _rrg.NElements();
        for ( size_type nEl = 0; nEl < knEls; ++nEl )
          _rrg[nEl].ConvertStrings( _rs );
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
  _TyVariant m_var;
};

__REGEXP_END_NAMESPACE

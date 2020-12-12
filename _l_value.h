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
#include <variant>

__REGEXP_BEGIN_NAMESPACE

template< class t_tyChar, size_t s_knbySegSize = 1024 >
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
  typedef variant< monostate, bool, vtyDataPosition, _TyData, _TySegArrayValues > _TyVariant;

  _l_value() = default;
  _l_value(_l_value const & ) = default;
  _l_value & operator =( _l_value const & ) = default;

  bool FIsNull() const
  {
    return holds_alternative<monostate>( m_var );
  }
  void Clear()
  {
    m_var.emplace<monostate>();
  }

  template < class t_ty >
  bool FIsA() const
  {
    return holds_alternative<t_ty>( m_var );
  }
  bool FIsArray() const
  {
    return holds_alternative<_TySegArrayValues>( m_var );
  }

  // These should work in many cases.
  template < class t_ty >
  SetValue( t_ty const & _rv )
  {
    m_var.emplace<t_ty>( _rv );
  }
  template < class t_ty >
  SetValue( t_ty && _rrv )
  {
    m_var.emplace<t_ty>(std::move(_rrv));
  }

  // This ensures we have an array of _l_values within and then sets the size to the passed size.
  void SetSize( size_type _stNEls )
  {
    if ( !FIsArray() )
      m_var.emplace<_TySegArrayValues>( s_knSegArrayInit );
    get< _TySegArrayValues >( m_var ).SetSize( _stNEls );
  }

  _TyThis & operator [] ( size_type _nEl )
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var )[ _nEl ]; // We let this throw a bad type exception as it will if we 
  }
  _TyThis const & operator [] ( size_type _nEl ) const
  {
    Assert( FIsArray() ); // Assert and then we will throw below.
    return get< _TySegArrayValues >( m_var )[ _nEl ]; // We let this throw a bad type exception as it will if we 
  }

protected:
  _TyVariant m_var;
};


__REGEXP_END_NAMESPACE

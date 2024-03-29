#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_buf.h
// Simple fixed and allocating buffers for use with tokens in the lexical analyzer.
// dbien
// 28DEC2020

#include <utility>
#include "_l_ns.h"
#include "_l_types.h"

__LEXOBJ_BEGIN_NAMESPACE

// The fixed version.
template < class t_TyChar >
class _l_fixed_buf : public pair< const t_TyChar *, size_t >
{
  typedef _l_fixed_buf _TyThis;
  typedef pair< const t_TyChar *, size_t > _TyBase;
public:
  typedef t_TyChar _TyChar;
  _l_fixed_buf( const t_TyChar * _pc, size_t _len )
    : _TyBase( _pc, _len )
  {
  }
  ~_l_fixed_buf() = default;
  _l_fixed_buf() = default;
  _l_fixed_buf( _l_fixed_buf const & _r ) = default;
  _l_fixed_buf & operator =( _TyThis const & _r ) = default;
  _l_fixed_buf( _l_fixed_buf && _rr )
  {
    swap( _rr );
  }
  _l_fixed_buf & operator =( _TyThis && _rr )
  {
    _TyThis acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
  }
  bool FIsNull() const
  {
    AssertValid();
    return !first;
  }
  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( !!first == !!second );
#endif //ASSERTSENABLED
  }
  const _TyChar * begin() const
  {
    return first;
  }
  const _TyChar * end() const
  {
    return first + second;
  }
  size_t length() const
  {
    return second;
  }
  const _TyChar * const & RCharP() const
  {
    return first;
  }
  const _TyChar * & RCharP()
  {
    return first;
  }
  size_t & RLength()
  {
    return second;
  }
  template < class t_TyStringView >
  void GetStringView( t_TyStringView & _rsvDest, vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
    requires( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) )
  {
    AssertValid();
    Assert( _rsvDest.empty() );
    Assert( _posEnd >= _posBegin );
    if ( _posEnd == _posBegin )
      return; // empty.
    Assert( _posEnd <= second );
    _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type*)first + _posBegin, size_t( _posEnd - _posBegin ) );
  }
protected:
  using _TyBase::first;
  using _TyBase::second;
};

// A very simple allocated buffer that only needs a pointer and a length.
template < class t_TyChar >
class _l_backing_buf : public pair< t_TyChar *, size_t >
{
  typedef _l_backing_buf _TyThis;
  typedef pair< t_TyChar *, size_t > _TyBase;
public:
  typedef t_TyChar _TyChar;
  explicit _l_backing_buf( size_t _len )
  {
    first = DBG_NEW _TyChar[ _len * sizeof( _TyChar ) ];
    second = _len;
  }
  _l_backing_buf( const _TyChar * _pch, size_t _len )
  {
    first = DBG_NEW _TyChar[ _len * sizeof( _TyChar ) ];
    second = _len;
    if ( _pch )
      memcpy( first, _pch, _len * sizeof( _TyChar ) );
  }
  ~_l_backing_buf()
  {
    if ( first )
      ::delete [] first;
  }
  _l_backing_buf() = default;
  _l_backing_buf( _l_backing_buf const & _r )
  {
    if ( _r.second )
    {
      first = DBG_NEW _TyChar[ _r.second ];
      second = _r.second;
      memcpy( first, _r.first, second * sizeof( _TyChar ) );
    }
  }
  _l_backing_buf & operator =( _TyThis const & _r )
  {
    _l_backing_buf copy( _r );
    swap( copy );
    return *this;
  }
  _l_backing_buf( _l_backing_buf && _rr )
  {
    swap( _rr );
  }
  _l_backing_buf & operator =( _TyThis && _rr )
  {
    _l_backing_buf acquire( std::move( _rr ) );
    swap( acquire );
    return *this;
  }
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
  }
  // We support copying a fixed buffer (for instance):
  template < class t_TyBufOther >
  _l_backing_buf( t_TyBufOther const & _rOther )
    requires ( TAreSameSizeTypes_v< _TyChar, typename t_TyBufOther::_TyChar > )
  {
    Assert( FIsNull() );
    if ( !_rOther.FIsNull() )
    {
      first = DBG_NEW _TyChar[ _rOther.length() ];
      second = _rOther.length();
      memcpy( first, _rOther.begin(), second * sizeof( _TyChar ) );
    }
  }
  // We support assignment to a fixed buffer (for instance):
  template < class t_TyBufOther >
  _TyThis & operator = ( t_TyBufOther const & _rOther )
    requires ( TAreSameSizeTypes_v< _TyChar, typename t_TyBufOther::_TyChar > )
  {
    _l_backing_buf copy( _rOther );
    swap( copy );
    return *this;
  }
  void Clear()
  {
    AssertValid();
    if ( first )
    {
      _TyChar * pc = first;
      first = nullptr;
      second = 0;
      ::delete [] pc;
    }
  }
  bool FIsNull() const
  {
    AssertValid();
    return !first;
  }
  void AssertValid() const
  {
#ifndef NDEBUG
    Assert( !!first == !!second );
#endif     
  }
  _TyChar * begin()
  {
    return first;
  }
  const _TyChar * begin() const
  {
    return first;
  }
  _TyChar * end()
  {
    return first + second;
  }
  const _TyChar * end() const
  {
    return first + second;
  }
  size_t length() const
  {
    return second;
  }
  // Set the contents and length of the buffer.
  void SetBuffer( const _TyChar * _pcBuf, size_t _nchLen )
  {
    Clear();
    first = DBG_NEW _TyChar[ _nchLen ];
    second = _nchLen;
    memcpy( first, _pcBuf, second * sizeof( _TyChar ) );
  }
  template < class t_TyStringView >
  void GetStringView( t_TyStringView & _rsvDest, vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
    requires( sizeof( typename t_TyStringView::value_type ) == sizeof( _TyChar ) )
  {
    AssertValid();
    Assert( _rsvDest.empty() );
    Assert( _posEnd >= _posBegin );
    if ( _posEnd == _posBegin )
      return; // empty.
    Assert( _posEnd <= second );
    _rsvDest = t_TyStringView( (const typename t_TyStringView::value_type *)first + _posBegin, size_t( _posEnd - _posBegin ) );
  }
protected:
  using _TyBase::first;
  using _TyBase::second;
};

__LEXOBJ_END_NAMESPACE
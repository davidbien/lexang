#pragma once

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
class _l_fixed_buf : public pair< const t_TyChar *, vtyDataPosition >
{
  typedef _l_fixed_buf _TyThis;
  typedef pair< const t_TyChar *, vtyDataPosition > _TyBase;
public:
  typedef t_TyChar _TyChar;
  _l_fixed_buf( const t_TyChar * _pc, vtyDataPosition _len )
    : _TyBase( _pc, _len )
  {
  }
  ~_l_fixed_buf() = default;
  _l_fixed_buf() = default;
  _l_fixed_buf( _l_fixed_buf const & _r ) = default;
  _l_fixed_buf & operator =( _TyThis const & _r ) = default;
  _l_fixed_buf( _l_fixed_buf && _rr ) = default;
  _l_fixed_buf & operator =( _TyThis && _rr ) = default;
  void swap( _TyThis & _r )
  {
    _TyBase::swap( _r );
  }
  bool FIsNull()
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
  const _TyChar * begin() const
  {
    return first;
  }
  const _TyChar * end() const
  {
    return first + second;
  }
  vtyDataPosition length() const
  {
    return second;
  }
  const _TyChar * & RCharP()
  {
    return first;
  }
  vtyDataPosition & RLength()
  {
    return second;
  }
  template < class t_TyStringView >
  void GetStringView( t_TyStringView & _rsvDest, vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
  {
    AssertValid();
    Assert( _rsvDest.empty() );
    Assert( _posEnd >= _posBegin );
    if ( _posEnd == _posBegin )
      return; // empty.
    Assert( _posEnd <= second );
    _rsvDest = t_TyStringView( first + _posBegin, _posEnd - _posBegin );
  }
protected:
  using _TyBase::first;
  using _TyBase::second;
};

// A very simple allocated buffer that only needs a pointer and a length.
template < class t_TyChar >
class _l_backing_buf : public pair< t_TyChar *, vtyDataPosition >
{
  typedef _l_backing_buf _TyThis;
  typedef pair< t_TyChar *, vtyDataPosition > _TyBase;
public:
  typedef t_TyChar _TyChar;
  _l_backing_buf( vtyDataPosition _len )
  {
    m_buf.first = ::new _TyChar[ _len * sizeof( _TyChar ) ];
    m_buf.second = _len;
  }
  ~_l_backing_buf()
  {
    if ( m_buf.first )
      ::delete [] m_buf.first;
  }
  _l_backing_buf() = default;
  _l_backing_buf( _l_backing_buf const & _r )
  {
    m_buf.second = _r.second;
    m_buf.first = ::new _TyChar[ m_buf.second ];
    memcpy( m_buf.first, _r.m_buf.first, m_buf.second * sizeof( _TyChar ) );
  }
  _l_backing_buf & operator =( _TyThis const & _r )
  {
    _l_backing_buf copy( _r );
    swap( copy );
  }
  _l_backing_buf( _l_backing_buf && _rr )
  {
    swap( _rr );
  }
  _l_backing_buf & operator =( _TyThis && _rr )
  {
    _l_backing_buf acquire( std::move( _rr ) );
    swap( acquire );
  }
  void swap( _TyThis & _r )
  {
    m_buf.swap( _r.m_buf );
  }
  bool FIsNull()
  {
    AssertValid();
    return !m_buf.first;
  }
  void AssertValid() const
  {
#ifndef NDEBUG
    Assert( !!m_buf.first == !!m_buf.second );
#endif     
  }
  _TyChar * begin()
  {
    return m_buf.first;
  }
  const _TyChar * begin() const
  {
    return m_buf.first;
  }
  _TyChar * end()
  {
    return m_buf.first + m_buf.second;
  }
  const _TyChar * end() const
  {
    return m_buf.first + m_buf.second;
  }
  vtyDataPosition length() const
  {
    return m_buf.second;
  }
  template < class t_TyStringView >
  void GetStringView( t_TyStringView & _rsvDest, vtyDataPosition _posBegin, vtyDataPosition _posEnd ) const
  {
    AssertValid();
    Assert( _rsvDest.empty() );
    Assert( _posEnd >= _posBegin );
    if ( _posEnd == _posBegin )
      return; // empty.
    Assert( _posEnd <= m_buf.second );
    _rsvDest = t_TyStringView( m_buf.first + _posBegin, _posEnd - _posBegin );
  }
};

__LEXOBJ_END_NAMESPACE
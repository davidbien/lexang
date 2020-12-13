#pragma once

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_data.h
// The representation of a data of part of a token.
// dbien : 30NOV2020

// A token contains either:
// 1) A (beg,end,datamarker) tuple. <or>
// 2) A set of such tuples in positional order.
// That's it. No translation is performed from input data as that process is beyond the scope of the token data itself.

#include <stdint.h>
#include "_assert.h"
#include "segarray.h"
#include "_l_inc.h"

__REGEXP_BEGIN_NAMESPACE

// _l_data_range_pod: A simple token range in a token stream.
// We want this to be a POD struct so we can use it in a anonymous union.
struct _l_data_range_pod
{
  vtyDataPosition m_posBegin;
  vtyDataPosition m_posEnd;
};
// _l_data_range_pod: A simple token range in a token stream.
// We want this to be a POD struct so we can use it in a anonymous union.
struct _l_data_typed_range_pod : public _l_data_range_pod
{
  vtyDataType m_nType;
};

// l_token_range: Non-pod version.
class _l_data_range
{
  typedef _l_data_range _TyThis;
public: 
  _l_data_range() = default;
  _l_data_range( _l_data_range const & ) = default;
  _l_data_range( vtyDataPosition _posBegin, vtyDataPosition _posEnd )
    : m_posBegin( _posBegin ),
      m_posEnd( _posEnd )
  {
  }
  _l_data_range & operator =( _TyThis const & _r ) = default;
  
// Make these accessible.
  vtyDataPosition m_posBegin{ numeric_limits< vtyDataPosition >::max() };
  vtyDataPosition m_posEnd{ numeric_limits< vtyDataPosition >::max() };
};

// _l_data_typed_range: Use protected inheritance to hide conversion to base.
class _l_data_typed_range : protected _l_data_range
{
  typedef _l_data_typed_range _TyThis;
  typedef _l_data_range _TyBase;
public:
  _l_data_typed_range() = default;
  _l_data_typed_range( _l_data_typed_range const & ) = default;
  _l_data_typed_range & operator = ( _l_data_typed_range const & ) = default;

  // Provide access to base class via explicit accessor:
  _TyBase const & GetRangeBase() const
  {
    return *this;
  }
  _TyBase & GetRangeBase()
  {
    return *this;
  }

  using _TyBase::m_posBegin;
  using _TyBase::m_posEnd;
  vtyDataType m_nType{0}; // the 0th type always signifies "plain text".
};

// by default we will keep the segment size small but large enough that most strings will fit in it.
template < class t_TyChar, size_t s_knbySegSize = 512 >
class _l_data
{
  typedef _l_data _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef SegArray< _l_data_typed_range, std::false_type, vtyDataPosition > _TySegArray;
  typedef _TySegArray::_tySizeType size_type;
  static constexpr size_t s_knSegArrayInit = s_knbySegSize;
  static_assert( sizeof( _TySegArray ) == sizeof( _TySegArray ) ); // No reason for this not to be the case.
  typedef std::basic_string< _TyChar > _TyStdStr;

  union
  {
    struct
    {
      uint64_t m_u64Marker; // When m_u64Marker is 0xffffffffffffffff then m_dtrData contains valid data, else m_rgbySegArray is populated.
      _l_data_typed_range_pod m_dtrData;
    };
    unsigned char m_rgbySegArray[ sizeof( _TySegArray ) ];
  };

  void AssertValid() const
#if ASSERTSENABLED
  {
    if ( FContainsSinglePos() )
    {
      Assert( ( numeric_limits< vtyDataPosition >::max() != m_dtrData.m_posBegin ) || ( numeric_limits< vtyDataPosition >::max() == m_dtrData.m_posEnd ) );
      Assert( ( numeric_limits< vtyDataPosition >::max() != m_dtrData.m_posBegin ) || !m_dtrData.m_nType );
      Assert( m_dtrData.m_posEnd >= m_dtrData.m_posBegin );
    }
    else
    {
      _PSegArray()->AssertValid();
    }
  }
#else //!ASSERTSENABLED
  { }
#endif //!ASSERTSENABLED

  _l_data()
  {
    _SetMembersNull();
  }
  _l_data( vtyDataPosition _posBegin, vtyDataPosition _posEnd )
  {
    _SetContainsSinglePos();
    m_dtrData.m_posBegin = _posBegin;
    m_dtrData.m_posEnd = _posEnd;
    m_dtrData.m_nType = 0;
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( _l_data_range const & _rdr )
  {
    _SetContainsSinglePos();
    m_dtrData.m_posBegin = _rdr.m_posBegin;
    m_dtrData.m_posEnd = _rdr.m_posEnd;
    m_dtrData.m_nType = 0;
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( _l_data_typed_range const & _rttr )
  {
    static_assert( sizeof( _l_data_typed_range ) == sizeof( _l_data_typed_range_pod ) );
    _SetContainsSinglePos();
    memcpy( &m_dtrData, &_rttr, sizeof m_dtrData );
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( const _l_data & _r )
  {
    _r.AssertValid();
    if ( _r.FContainsSinglePos() )
    {
      memcpy( this, &_r, sizeof *this );
    }
    else
    {
      _SetMembersNull(); // nullify in case of throw.
      _TySegArray saCopy( *_r._PSegArray() );
      new ( m_rgbySegArray ) _TySegArray( std::move( saCopy ) );
      AssertValid();
    }
  }
  _l_data( _l_data && _rr )
  {
    _SetMembersNull(); // Leave the caller in a valid state.
    swap( _rr );
  }
  ~_l_data()
  {
    if ( !FContainsSinglePos() )
      _ClearSegArray();
  }
  void swap( _l_data & _r )
  {
    unsigned char rgucSwap[ sizeof(*this) ];
    memcpy( rgucSwap, this, sizeof(*this) );
    memcpy( this, &_r, sizeof(*this) );
    memcpy( &_r, rgucSwap, sizeof(*this) );
  }
  _l_data & operator =( _l_data const & _r )
  {
    _l_data copy( _r );
    swap( copy );
    return *this;
  }
  _l_data & operator =( _l_data && _rr )
  {
    SetNull();
    swap( _rr );
    return *this;
  }

  bool FContainsSinglePos() const
  {
    return m_u64Marker == 0xffffffffffffffff;
  }
  size_t NPositions() const
  {
    AssertValid();
    if ( FIsNull() )
      return 0;
    else
    if ( FContainsSinglePos() )
      return 1;
    else
      return _PSegArray()->NElements();
  }
  bool FContainsData() const
  {
    AssertValid();
    if ( FContainsSinglePos() )
      return !!( m_posEnd > m_posBegin );
    else
      return !!_PSegArray()->NElements(); // Assuming we don't put empty elements in the segarray.
  }
  bool FIsNull() const
  {
    AssertValid();
    return FContainsSinglePos() && ( numeric_limits< vtyDataPosition >::max() == m_dtrData.m_posBegin );
  }
  void SetNull()
  {
    if ( !FContainsSinglePos() )
      _ClearSegArray();
    _SetMembersNull();
  }
  void Clear()
  {
    SetNull();
  }

  void Set( _l_data_range const & _rdr )
  {
    Set( _rdr.m_posBegin, _rdr.m_posEnd );
  }
  void Set( _l_data_typed_range const & _rttr )
  {
    Set( _rttr.m_posBegin, _rttr.m_posEnd, _rttr.m_nType );
  }
  void Set( vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType = 0 )
  {
    Assert( _posEnd >= _posBegin );
    if ( !FContainsSinglePos() )
      _ClearSegArray();
    _SetContainsSinglePos();
    m_dtrData.m_posBegin = _posBegin;
    m_dtrData.m_posEnd = _posEnd;
    m_dtrData.m_nType = _nType;
    AssertValid();
  }
  void Append( _l_data_range const & _rdr )
  {
    Append( _rdr.m_posBegin, _rdr.m_posEnd );
  }
  void Append( _l_data_typed_range const & _rttr )
  {
    Append( _rttr.m_posBegin, _rttr.m_posEnd, _rttr.m_nType );
  }
  void Append( vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType = 0 )
  {
    if ( FIsNull() )
      return Set( _posBegin, _posEnd, _nType );
    else
    if ( FContainsSinglePos() )
    {
      _TySegArray saNew( s_knSegArrayInit );
      saNew.Overwrite( 0, (const ns_re::_l_data_typed_range *)&m_dtrData, 1 );
      new ( m_rgbySegArray ) _TySegArray( std::move( saNew ) );
      AssertValid();
    }
    _l_data_typed_range_pod ttrWrite = { _posBegin, _posEnd, _nType };
    _PSegArray()->Overwrite( _PSegArray()->NElements(), (const ns_re::_l_data_typed_range *)&ttrWrite, 1 );
    AssertValid();
  }
  template < class t_TyCharOut >
  void ToJsoValue( JsoValue< t_TyCharOut > & _rjv ) const
  {
    if ( FIsNull() )
      _rjv.SetNullValue();
    else
    if ( FContainsSinglePos() )
      _ToJsoValue( _rjv, m_dtrData );
    else
    {
      const size_type knEls = _PSegArray()->NElements();
      for ( size_type n = 0; n < knEls; ++n )
        _ToJsoValue( reinterpret_cast< _l_data_typed_range_pod const & >( _rjv[n] ) );
    }
  }
protected:
  static void _ToJsoValue( JsoValue< t_TyCharOut > & _rjv, _l_data_typed_range_pod const & _rttr )
  {
    Assert( _rjv.FIsNull() );
    _rjv.SetValueType( ejvtObject );
    _rjv.SetValue( "b", _rttr.m_posBegin );
    _rjv.SetValue( "e", _rttr.m_posEnd );
    _rjv.SetValue( "t", _rttr.m_nType );
  }
  void _SetContainsSinglePos()
  {
    m_u64Marker = 0xffffffffffffffff;
  }
  void _SetMembersNull()
  {
    _SetContainsSinglePos();
    m_dtrData.m_posBegin = numeric_limits< vtyDataPosition >::max();
    m_dtrData.m_posEnd = numeric_limits< vtyDataPosition >::max();
    m_dtrData.m_nType = 0;
  }
  _TySegArray * _PSegArray()
  {
    Assert( !FContainsSinglePos() );
    return (_TySegArray*)m_rgbySegArray;
  }
  const _TySegArray * _PSegArray() const
  {
    Assert( !FContainsSinglePos() );
    return (_TySegArray*)m_rgbySegArray;
  }
  void _ClearSegArray()
  {
    Assert( !FContainsSinglePos() );
    _PSegArray()->~_TySegArray();
  }
};

__REGEXP_END_NAMESPACE

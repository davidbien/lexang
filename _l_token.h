#pragma once

// _l_token.h
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

typedef size_t vtyTokenPosition;
typedef size_t vtyDataType;

// _l_token_range_pod: A simple token range in a token stream.
// We want this to be a POD struct so we can use it in a anonymous union.
class _l_token_range_pod
{
  vtyTokenPosition m_posBegin;
  vtyTokenPosition m_posEnd;
};
// _l_token_range_pod: A simple token range in a token stream.
// We want this to be a POD struct so we can use it in a anonymous union.
class _l_token_typed_range_pod : public _l_token_range_pod
{
  vtyDataType m_nType;
};

// l_token_range: Non-pod version.
class _l_token_range
{
  typedef _l_token_range _TyThis;
public: 
  _l_token_range() = default;
  _l_token_range( _l_token_range const & ) = default;
  _l_token_range( vtyTokenPosition _posBegin, vtyTokenPosition _posEnd )
    : m_posBegin( _posBegin ),
      m_posEnd( _posEnd )
  {
  }
  _l_token_range & operator =( _TyThis const & _r ) = default;
  
// Make these accessible.
  vtyTokenPosition m_posBegin{ numeric_limits< vtyTokenPosition >::max() };
  vtyTokenPosition m_posEnd{ numeric_limits< vtyTokenPosition >::max() };
};

// _l_token_typed_range: Use protected inheritance to hide conversion to base.
class _l_token_typed_range : protected _l_token_range
{
  typedef _l_token_typed_range _TyThis;
  typedef _l_token_range _TyBase;
public:
  _l_token_typed_range() = default;
  _l_token_typed_range( _l_token_typed_range const & ) = default;
  _l_token_typed_range & operator = ( _l_token_typed_range const & ) = default;

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
template <  class t_TyStream, class t_TyChar, uint32_t s_knbySegSize = 512 >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef t_TyStream _TyStream;
  typedef t_TyChar _TyChar;
  typedef SegArray< _l_token_typed_range, false, vtyTokenPosition > _TySegArray;
  static constexpr s_knSegArrayInitTypedRange = s_knbySegSize / sizeof( _l_token_typed_range );
  static_assert( sizeof( _TySegArray ) == sizeof( _TySegArray ) ); // No reason for this not to be the case.
  typedef std::basic_string< _TyChar > _TyStdStr;

  union
  {
    struct
    {
      uint64_t m_u64Marker; // When m_u64Marker is 0xffffffffffffffff then m_ttrData contains valid data, else m_rgbySegArray is populated.
      _l_token_typed_range_pod m_ttrData;
    };
    unsigned char m_rgbySegArray[ sizeof( _TySegArray ) ];
  };

  void AssertValid() const
#if ASSERTSENABLED
  {
    if ( FContainsSinglePos() )
    {
      Assert( ( numeric_limits< vtyTokenPosition >::max() != m_ttrData.m_posBegin ) || ( numeric_limits< vtyTokenPosition >::max() == m_ttrData.m_posEnd ) );
      Assert( ( numeric_limits< vtyTokenPosition >::max() != m_ttrData.m_posBegin ) || !m_ttrData.m_nType );
      Assert( m_ttrData.m_posEnd >= m_ttrData.m_posBegin );
    }
    else
    {
      _PSegArray()->AssertValid();
    }
  }
#else //!ASSERTSENABLED
  { }
#endif //!ASSERTSENABLED

  _l_token()
  {
    _SetMembersNull();
  }
  _l_token( vtyTokenPosition _posBegin, vtyTokenPosition _posEnd )
  {
    _SetContainsSinglePos();
    m_ttrData.m_posBegin = _posBegin;
    m_ttrData.m_posEnd = _posEnd;
    m_ttrData.m_nType = 0;
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_token( _l_token_range const & _rtr )
  {
    _SetContainsSinglePos();
    m_ttrData.m_posBegin = _rtr.m_posBegin;
    m_ttrData.m_posEnd = _rtr.m_posEnd;
    m_ttrData.m_nType = 0;
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_token( _l_token_typed_range const & _rttr )
  {
    static_assert( sizeof( _l_token_typed_range ) == sizeof( _l_token_typed_range_pod ) );
    _SetContainsSinglePos();
    memcpy( &m_ttrData, &_rttr, sizeof m_ttrData );
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_token( const _l_token & _r )
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
  _l_token( _l_token && _rr )
  {
    _SetMembersNull(); // Leave the caller in a valid state.
    swap( _rr );
  }
  ~_l_token()
  {
    if ( !FContainsSinglePos() )
      _ClearSegArray();
  }
  void swap( _l_token & _r )
  {
    unsigned char rgucSwap[ sizeof(*this) ];
    memcpy( rgucSwap, this, sizeof(*this) );
    memcpy( this, &_r, sizeof(*this) );
    memcpy( &_r, rgucSwap, sizeof(*this) );
  }
  _l_token & operator =( _l_token const & _r )
  {
    _l_token copy( _r );
    swap( copy );
    return *this;
  }
  _l_token & operator =( _l_token && _rr )
  {
    SetNull();
    swap( _rr );
    return *this;
  }

  bool FContainsSinglePos() const
  {
    AssertValid();
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
    return FContainsSinglePos() && ( numeric_limits< vtyTokenPosition >::max() == m_ttrData.m_posBegin );
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

  void Set( _l_token_range const & _rtr )
  {
    Set( _rtr.m_posBegin, _rtr.m_posEnd );
  }
  void Set( _l_token_typed_range const & _rttr )
  {
    Set( _rttr.m_posBegin, _rttr.m_posEnd, _rttr.m_nType );
  }
  void Set( vtyTokenPosition _posBegin, vtyTokenPosition _posEnd, vtyDataType _nType = 0 )
  {
    Assert( _posEnd >= _posBegin );
    if ( !FContainsSinglePos() )
      _ClearSegArray();
    _SetContainsSinglePos();
    m_ttrData.m_posBegin = _posBegin;
    m_ttrData.m_posEnd = _posEnd;
    m_ttrData.m_nType = _nType;
    AssertValid();
  }
  void Append( _l_token_range const & _rtr )
  {
    Append( _rtr.m_posBegin, _rtr.m_posEnd );
  }
  void Append( _l_token_typed_range const & _rttr )
  {
    Append( _rttr.m_posBegin, _rttr.m_posEnd, _rttr.m_nType );
  }
  void Append( vtyTokenPosition _posBegin, vtyTokenPosition _posEnd, vtyDataType _nType = 0 )
  {
    if ( FIsNull() )
      return Set( _posBegin, _posEnd, _nType );
    else
    if ( FContainsSinglePos() )
    {
      _TySegArray saNew( s_knSegArrayInit );
      saNew.Overwrite( 0, &m_ttrData, sizeof m_ttrData );
      new ( m_rgbySegArray ) _TySegArray( std::move( saNew ) );
      AssertValid();
    }
    _l_token_typed_range_pod ttrWrite = { _posBegin, _posEnd, _nType };
    _PSegArray()->Overwrite( _PSegArray()->NElements(), &ttrWrite, sizeof ttrWrite );
    AssertValid();
  }
protected:
  void _SetContainsSinglePos()
  {
    m_u64Marker = 0xffffffffffffffff;
  }
  void _SetMembersNull()
  {
    _SetContainsSinglePos();
    m_ttrData.m_posBegin = numeric_limits< vtyTokenPosition >::max();
    m_ttrData.m_posEnd = numeric_limits< vtyTokenPosition >::max();
    m_ttrData.m_nType = 0;
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

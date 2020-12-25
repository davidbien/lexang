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

__LEXOBJ_BEGIN_NAMESPACE

#if 0 // trying to rid these.
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
#endif //0

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

  void AssertValid() const
  {
#if ASSERTSENABLED
    Assert( ( m_posBegin != numeric_limits< vtyDataPosition >::max() ) || ( m_posEnd == numeric_limits< vtyDataPosition >::max() ) );
    Assert( m_posEnd >= m_posBegin );
#endif //ASSERTSENABLED
  }

  vtyDataPosition begin() const
  {
    return m_posBegin;
  }
  vtyDataPosition end() const
  {
    return m_posEnd;
  }
  vtyDataPosition length() const
  {
    AssertValid();
    return m_posEnd - m_posBegin;
  }
  
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

  _l_data_typed_range( vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType, vtyDataTriggerId _nIdTrigger )
    : _TyBase( _posBegin, _posEnd ),
      m_nType( _nType ),
      m_nIdTrigger( _nIdTrigger )
  {
  }

  void AssertValid() const
  {
#if ASSERTSENABLED
    _TyBase::AssertValid();
    Assert( ( m_posBegin != numeric_limits< vtyDataPosition >::max() ) || ( m_nIdTrigger == vktidInvalidIdTrigger ) );
    Assert( ( m_posBegin != numeric_limits< vtyDataPosition >::max() ) || !m_nType );
#endif //ASSERTSENABLED  
  }

  // Provide access to base class via explicit accessor:
  _TyBase const & GetRangeBase() const
  {
    return *this;
  }
  _TyBase & GetRangeBase()
  {
    return *this;
  }
  
  using _TyBase::begin;
  using _TyBase::end;
  using _TyBase::length;
  vtyDataType type() const
  {
    return m_nType;
  }
  vtyDataType id() const
  {
    return m_nIdTrigger;
  }

  using _TyBase::m_posBegin;
  using _TyBase::m_posEnd;
  vtyDataType m_nType{0}; // the 0th type always signifies "plain text".
  vtyDataTriggerId m_nIdTrigger{vktidInvalidIdTrigger}; // This is the trigger id of the trigger that created the data range.
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
      _l_data_typed_range m_dtrData;
    };
    unsigned char m_rgbySegArray[ sizeof( _TySegArray ) ];
  };

  void AssertValid() const
  {
#if ASSERTSENABLED
    if ( FContainsSingleDataRange() )
    {
      Assert( ( numeric_limits< vtyDataPosition >::max() != m_dtrData.m_posBegin ) || ( numeric_limits< vtyDataPosition >::max() == m_dtrData.m_posEnd ) );
      Assert( ( numeric_limits< vtyDataPosition >::max() != m_dtrData.m_posBegin ) || !m_dtrData.m_nType );
      Assert( ( numeric_limits< vtyDataPosition >::max() != m_dtrData.m_posBegin ) || ( vktidInvalidIdTrigger == m_dtrData.m_nIdTrigger ) );
      Assert( m_dtrData.m_posEnd >= m_dtrData.m_posBegin );
    }
    else
    {
      _PSegArray()->AssertValid();
    }
#endif //ASSERTSENABLED
  }

  _l_data()
    : m_dtrData()
  {
    _SetContainsSingleDataRange();
  }
  _l_data( vtyDataPosition _posBegin, vtyDataPosition _posEnd )
    : m_dtrData( _posBegin, _posEnd )
  {
    _SetContainsSingleDataRange();
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( _l_data_range const & _rdr )
    : m_dtrData( _rdr )
  {
    _SetContainsSingleDataRange();
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( _l_data_typed_range const & _rdtr )
  {
    _SetContainsSingleDataRange();
    memcpy( &m_dtrData, &_rdtr, sizeof m_dtrData );
    AssertValid(); // We don't expect someone to set in invalid members but it is possible.
  }
  _l_data( const _l_data & _r )
  {
    _r.AssertValid();
    if ( _r.FContainsSingleDataRange() )
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
    if ( !FContainsSingleDataRange() )
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

  bool FContainsSingleDataRange() const
  {
    return m_u64Marker == 0xffffffffffffffff;
  }
  size_t NPositions() const
  {
    AssertValid();
    if ( FIsNull() )
      return 0;
    else
    if ( FContainsSingleDataRange() )
      return 1;
    else
      return _PSegArray()->NElements();
  }
  bool FContainsData() const
  {
    AssertValid();
    if ( FContainsSingleDataRange() )
      return !!( m_dtrData.m_posEnd > m_dtrData.m_posBegin );
    else
      return !!_PSegArray()->NElements(); // Assuming we don't put empty elements in the segarray.
  }
  // Return the count of characters comprising all the ranges in this.
  size_t CountChars() const
  {
    if ( FIsNull() )
      return 0;
    else
    if ( FContainsSingleDataRange() )
      return m_dtrData.length();
    else
    {
      size_t nChars = 0;
      _PSegArray()->ApplyContiguous( 0, _PSegArray()->NElements(),
        [&nChars]( const _l_data_typed_range * _pdtrBegin, const _l_data_typed_range * _pdtrEnd )
        {
          for ( ; _pdtrEnd != _pdtrBegin; ++_pdtrBegin )
            nChars += _pdtrBegin->length();
        }
      );
      return nChars;
    }
  }
  bool FIsNull() const
  {
    AssertValid();
    return FContainsSingleDataRange() && ( numeric_limits< vtyDataPosition >::max() == m_dtrData.m_posBegin );
  }
  void SetNull()
  {
    if ( !FContainsSingleDataRange() )
      _ClearSegArray();
    _SetMembersNull();
  }
  void Clear()
  {
    SetNull();
  }

  bool FGetSingleDataRange( _l_data_range & _rdr ) const
  {
    if ( FContainsSingleDataRange() )
    {
      _rdr = m_dtrData.GetRangeBase();
      return true;
    }
    return false;
  }
  bool FGetSingleDataRange( _l_data_typed_range & _rdtr ) const
  {
    if ( FContainsSingleDataRange() )
    {
      _rdtr = m_dtrData;
      return true;
    }
    return false;
  }
  _l_data_typed_range DataRangeGetSingle() const
  {
    Assert( FContainsSingleDataRange() );
    return m_dtrData;
  }
  void Set( _l_data_range const & _rdr )
  {
    Set( _l_data_typed_range( _rdr ) );
  }
  void Set( vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType, vtyDataTriggerId _nIdTrigger )
  {
    Set( _l_data_typed_range( _posBegin, _posEnd, _nType, _nIdTrigger ) );
  }
  void Set( _l_data_typed_range const & _rdtr )
  {
    _rdtr.AssertValid();
    if ( !FContainsSingleDataRange() )
      _ClearSegArray();
    _SetContainsSingleDataRange();
    m_dtrData = _rdtr;
    AssertValid();
  }
  void Append( _l_data_range const & _rdr )
  {
    Append( _l_data_typed_range( _rdr ) );
  }
  void Append( vtyDataPosition _posBegin, vtyDataPosition _posEnd, vtyDataType _nType, vtyDataTriggerId _nIdTrigger )
  {
    Append( _l_data_typed_range( _posBegin, _posEnd, _nType, _nIdTrigger ) );
  }
  void Append( _l_data_typed_range const & _rdtr )
  {
    if ( FIsNull() )
      return Set( _rdtr );
    else
    if ( FContainsSingleDataRange() )
    {
      _TySegArray saNew( s_knSegArrayInit );
      saNew.Overwrite( 0, &m_dtrData, 1 );
      new ( m_rgbySegArray ) _TySegArray( std::move( saNew ) );
      AssertValid();
    }
    _PSegArray()->Overwrite( _PSegArray()->NElements(), &_rdtr, 1 );
    AssertValid();
  }
  _TySegArray & GetSegArrayDataRanges()
  {
    return *_PSegArray();
  }
  const _TySegArray & GetSegArrayDataRanges() const
  {
    return *_PSegArray();
  }
  template < class t_TyCharOut >
  void ToJsoValue( JsoValue< t_TyCharOut > & _rjv ) const
  {
    if ( FIsNull() )
      _rjv.SetNullValue();
    else
    if ( FContainsSingleDataRange() )
      _ToJsoValue( _rjv, m_dtrData );
    else
    {
      const size_type knEls = _PSegArray()->NElements();
      for ( size_type n = 0; n < knEls; ++n )
        _ToJsoValue( _rjv[n], _PSegArray()->ElGet( n ) );
    }
  }
protected:
  template < class t_TyCharOut >
  static void _ToJsoValue( JsoValue< t_TyCharOut > & _rjv, _l_data_typed_range const & _rdtr )
  {
    Assert( _rjv.FIsNull() );
    _rjv.SetValueType(ejvtObject);
    _rjv("b").SetValue(_rdtr.m_posBegin);
    _rjv("e").SetValue(_rdtr.m_posEnd );
    _rjv("t").SetValue( _rdtr.m_nType );
    _rjv("i").SetValue( _rdtr.m_nIdTrigger );
  }
  void _SetContainsSingleDataRange()
  {
    m_u64Marker = 0xffffffffffffffff;
  }
  void _SetMembersNull()
  {
    _SetContainsSingleDataRange();
    m_dtrData.m_posBegin = numeric_limits< vtyDataPosition >::max();
    m_dtrData.m_posEnd = numeric_limits< vtyDataPosition >::max();
    m_dtrData.m_nType = 0;
    m_dtrData.m_nIdTrigger = vktidInvalidIdTrigger;
  }
  _TySegArray * _PSegArray()
  {
    Assert( !FContainsSingleDataRange() );
    return (_TySegArray*)m_rgbySegArray;
  }
  const _TySegArray * _PSegArray() const
  {
    Assert( !FContainsSingleDataRange() );
    return (_TySegArray*)m_rgbySegArray;
  }
  void _ClearSegArray()
  {
    Assert( !FContainsSingleDataRange() );
    _PSegArray()->~_TySegArray();
  }
};

__LEXOBJ_END_NAMESPACE

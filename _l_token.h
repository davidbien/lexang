#pragma once

// _l_token.h
// The representation of a single token.
// dbien : 30NOV2020

// We want this token to hold various representations of a token:
// 1) A stream and (beg,end) positions within that stream.
// 2) A pointer and (beg,end) positions within that memory.
// 3) An in-memory stream - i.e. segmented array (segarray).
// It would also be nice if there were reference counting so that token could
//  be shared read-only.
// It should be able to transform from (1) or (2) to (3) when information is added to the existing token.
// Would like to templatize by allocator but I have to propagate it and it's annoying right now. Later.


#include <stdint.h>
#include "segarray.h"

// by default we will keep the segment size small but large enough that most strings will fit in it.
template <  class t_tyStream, class t_TyChar, class t_TyPosition = size_t, uint32_t s_knbySegSize = 4096 >
class _l_token
{
  typedef _l_token _TyThis;
public:
  typedef t_tyStream _tyStream;
  typedef t_TyChar _TyChar;
  typedef t_TyPosition _TyPosition;
  typedef SegArray< t_TyChar, false, t_TyPosition > _TySegArray;
  static constexpr s_knSegArrayInit = s_knbySegSize / sizeof( _TyChar );
  typedef std::basic_string< _TyChar > 

  union
  {
    struct
    {
      uint64_t m_u64Marker; // When m_u64Marker is 0xffffffffffffffff then (m_psStream,m_posBegin,m_posEnd) are valid, else m_rgbySegArray is populated.
      const _tyStream * m_psStream;
      t_TyPosition m_posBegin;
      t_TyPosition m_posEnd; // Not inclusive.
    };
    unsigned char m_rgbySegArray[ _TySegArray ];
  };

  _l_token()
  {
    SetContainsPositions();
    m_psStream = nullptr;
  }
  _l_token( const _tyStream * _psStream, t_TyPosition _posBegin, t_TyPosition _posEnd )
  {
    SetContainsPositions();
    m_psStream = _psStream;
    m_posBegin = _posBegin;
    m_posEnd = _posEnd;
  }
  _l_token( const t_TyChar * _pc, t_TyPosition _nChars )
  {
    // Nullify in case of throw.
    SetContainsPositions();
    m_psStream = nullptr;
    Append( _pc, _nChars );
  }
  _l_token( const _l_token & _r )
  {
    SetContainsPositions();
    if ( _r.FContainsPositions() )
    {
      m_psStream = _r.m_psStream;
      m_posBegin = _r.m_posBegin;
      m_posEnd = _r.m_posEnd;
    }
    else
    {
      m_psStream = nullptr; // nullify in case of throw.
      _TySegArray saCopy( *_r._PSegArray() );
      new ( m_rgbySegArray ) _TySegArray( std::move( saCopy ) );
      Assert( !FContainsPositions() );
    }
  }
  _l_token( _l_token && _rr )
  {
    SetContainsPositions();
    m_psStream = nullptr;
    swap( _rr );
  }
  ~_l_token()
  {
    if ( !FContainsPositions() )
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

  void SetContainsPositions()
  {
    m_u64Marker = 0xffffffffffffffff;
  }
  bool FContainsPositions() const
  {
    return m_u64Marker == 0xffffffffffffffff;
  }
  bool FHasStream() const
  {
    return FContainsPositions() && !!m_psStream;
  }
  _TyPosition NLengthChars() const
  {
    Assert( !FContainsPositions() || !m_psStream || ( m_posEnd >= m_posBegin ) );
    return FContainsPositions() ? ( m_psStream ? ( m_posEnd - m_posBegin ) : 0 ) : _PSegArray()->NElements();
  }
  bool FIsNull() const
  {
    return FContainsPositions() && ( m_psStream == nullptr );
  }
  void SetNull()
  {
    if ( !FContainsPositions() )
      _ClearSegArray();
    SetContainsPositions();
    m_psStream = nullptr;
  }

  void SetBeginEnd( _tyStream & _rsStream, vtyTokenPosition _posBegin, vtyTokenPosition _posEnd )
  {
    Assert( _posEnd >= _posBegin );
    if ( !FContainsPositions() )
      _ClearSegArray();
    SetContainsPositions();
    m_psStream = &_rsStream;
    if ( _posEnd >= _posBegin )
    {
      m_posBegin = _posBegin;
      m_posEnd = _posEnd;
    }
    else
    {
      m_posEnd = m_posBegin = 0;
    }
  }

  // Append a set of characters to the end of any data.
  void Append( const t_TyChar * _pc, t_TyPosition _nChars ) noexcept(false)
  {
    Assert( _nChars == StrNLen(_pc) ); // We don't expect to be writing embedded nulls - at least at this point.
    if ( _nChars > 0 )
    {
      if ( FContainsPositions() )
        _ConvertToSegArray();
      _PSegArray()->Overwrite( _PSegArray()->NElements(), _pc, _nChars );
    }
  }
  void ToString( _TyStdStr & _rstr ) const
  {
    Assert( !_rstr.length() ); // we expect an empty string.
    _TyPosition nLen = NLengthChars();
    _rstr.resize( nLen ); // reserves nLen+1 memory.
    if ( !nLen )
      return;
    if ( FContainsPositions() )
    {
      m_psStream->ReadAtPos( &_rstr[0], m_posBegin * sizeof(_TyChar), nLen * sizeof(_TyChar) );
    }
    else
    {
      _PSegArray()->Read( 0, &_rstr[0], nLen );
    }
    Assert( !_rstr[nLen] ); // set by resize().
  }
protected:
  _TySegArray * _PSegArray()
  {
    Assert( !FContainsPositions() );
    return (_TySegArray*)m_rgbySegArray;
  }
  const _TySegArray * _PSegArray() const
  {
    Assert( !FContainsPositions() );
    return (_TySegArray*)m_rgbySegArray;
  }
  void _ConvertToSegArray() noexcept(false)
  {
    Assert( FContainsPositions() );
    Assert( FHasStream() );
    if ( FContainsPositions() && NLengthChars() )
    {
      _TySegArray saNew( s_knSegArrayInit );
      saNew.OverwriteFromStream( 0, *m_psStream, m_posBegin, m_posEnd - m_posBegin );
      new( m_rgbySegArray ) _TySegArray( std::move( saNew ) ); // swap it in.
      Assert( !FContainsPositions() );
    }
  }
  void _ClearSegArray()
  {
    Assert( !FContainsPositions() );
    _PSegArray()->~_TySegArray();
    SetContainsPositions();
  }
};
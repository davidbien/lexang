#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_tag.h
// The registers of the lexical analyzer for tagged DFAs - see _l_dfatag.h for an overview of tagged DFAs.
//
// While moving through the input for a token of a tagged DFA there may be more than one way in which the input read
//	thus far might yet be matched, each having crossed triggers at different positions. A register records the
//	triggers crossed, and where, for one of those ways. The tag operation states of the DFA ( _l_state_tagops<> )
//	copy registers and record the crossing of triggers in them. When the token has been found the triggers
//	recorded in the register for the way in which the token was matched are fired, in the order crossed.
//
// How a register records triggers depends upon the rules. The generator chooses, for all of the DFAs of a lexical
//	analyzer, the cheapest that will do - it is the last template argument of _l_analyzer<>:
// _l_tag_none: There are no tagged DFAs: the triggers of the rules, if any, are transitions which the lexical
//	analyzer takes and fires as it moves through the input. There are no registers at all.
// _l_tag_registers: Each trigger is crossed at most once in matching a token and in a known order: a register is
//	the position at which each trigger was crossed. Nothing is allocated while moving through the input.
// _l_tag_history: A trigger may be crossed any number of times - it is within a loop - or the triggers may be
//	crossed in different orders. A register refers to the last trigger crossed in a history of the triggers
//	crossed which is shared by the registers: a tree in which each entry refers to the trigger crossed before it.
//	Copying a register is then merely the copy of an index.

#include <vector>
#include <cstring>
#include <algorithm>
#include "_l_ns.h"
#include "_l_types.h"
#include "_l_state.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_tag_none: The lexical analyzer has no tagged DFAs.
struct _l_tag_none
{
  static constexpr int s_knTagPolicy = 0;
};

// _l_tag_base: That which the lexical analyzer needs for a token of a tagged DFA whatever the kind of register.
template < class t_TyChar >
struct _l_tag_base
{
  typedef _l_tag_op< t_TyChar > _TyTagOp;
  typedef typename _l_an_mostbase< t_TyChar >::_TyPMFnAccept _TyPMFnAccept;

  bool m_fTagDfa{false}; // Is the DFA of the current token a tagged DFA.
  bool m_fFiring{false}; // Are we firing the triggers of the token found: the current position is then that of the trigger.
  unsigned char m_nSnapshotFire{0}; // The snapshot whose triggers are those of the token found.
  // In an accepting state register zero is the register for the way in which the token is matched. We need it as it
  //	was in the last accepting state once the token has been found: 0 is the last accepting state, 1 the last
  //	accepting state which was not a lookahead state. The registers only change in a tag operation state so we merely
  //	note that a snapshot of register zero is pending in an accepting state and take it upon entering the next tag
  //	operation state - if there is one: usually there isn't and we then use register zero itself.
  bool m_rgfSnapshotPending[2] = { false, false };
  vtyDataPosition m_posFiring{vkdpNullDataPosition}; // The position of the trigger being fired.
  vtyDataPosition m_posFinish{vkdpNullDataPosition}; // The position at which the triggers crossed at the end of the token fire.
};

// _l_tag_registers: A register is the position at which each trigger was crossed.
template < class t_TyChar >
class _l_tag_registers : public _l_tag_base< t_TyChar >
{
  typedef _l_tag_registers _TyThis;
  typedef _l_tag_base< t_TyChar > _TyBase;
public:
  using typename _TyBase::_TyTagOp;
  using typename _TyBase::_TyPMFnAccept;
  static constexpr int s_knTagPolicy = 1;

  // Start a token of a tagged DFA which requires _nRegisters registers and whose triggers have _nSlots slots.
  void Start( size_t _nRegisters, size_t _nSlots )
  {
    m_nRegisters = _nRegisters;
    m_nSlots = _nSlots;
    // Two more registers for the snapshots.
    if ( m_rgpos.size() < ( _nRegisters + 2 ) * _nSlots )
      m_rgpos.resize( ( _nRegisters + 2 ) * _nSlots );
    if ( m_rgpmfnTriggers.size() < _nSlots )
      m_rgpmfnTriggers.resize( _nSlots );
    // We start in register zero having crossed no triggers. Every other register is copied before it is used.
    std::fill_n( m_rgpos.begin(), _nSlots, vkdpNullDataPosition );
  }
  void Copy( size_t _nDst, size_t _nSrc )
  {
    memcpy( m_rgpos.data() + _nDst * m_nSlots, m_rgpos.data() + _nSrc * m_nSlots, m_nSlots * sizeof( vtyDataPosition ) );
  }
  // Record that the trigger of the operation was crossed at _pos in the register _nDst.
  void Trigger( size_t _nDst, _TyTagOp const & _rop, vtyDataPosition _pos )
  {
    Assert( ( _rop.m_nSlot >= 0 ) && ( size_t( _rop.m_nSlot ) < m_nSlots ) );
    m_rgpos[ _nDst * m_nSlots + size_t( _rop.m_nSlot ) ] = _pos;
    m_rgpmfnTriggers[ size_t( _rop.m_nSlot ) ] = _rop.m_pmfnTrigger;
  }
  // Keep a copy of register zero: the register for the way in which the token is matched in an accepting state.
  void Snapshot( size_t _nSnapshot )
  {
    Copy( m_nRegisters + _nSnapshot, 0 );
  }
  // Call _rrfire( trigger, position ) for each trigger recorded in the snapshot - or in register zero if the snapshot
  //  is still pending - in the order the triggers were crossed.
  template < class t_TyFire >
  void Fire( size_t _nSnapshot, bool _fSnapshotPending, t_TyFire && _rrfire )
  {
    const size_t knRegister = _fSnapshotPending ? size_t(0) : ( m_nRegisters + _nSnapshot );
    // The slots are in an order in which the triggers are always crossed.
    for ( size_t nSlot = 0; nSlot < m_nSlots; ++nSlot )
    {
      const vtyDataPosition kpos = m_rgpos[ knRegister * m_nSlots + nSlot ];
      if ( vkdpNullDataPosition != kpos )
        _rrfire( m_rgpmfnTriggers[ nSlot ], kpos );
    }
  }
protected:
  size_t m_nRegisters{0};
  size_t m_nSlots{0};
  std::vector< vtyDataPosition > m_rgpos; // The registers and then the two snapshots: m_nSlots positions each.
  std::vector< _TyPMFnAccept > m_rgpmfnTriggers; // The trigger of each slot - recorded as each is crossed.
};

// _l_tag_history: A register refers to the last trigger crossed within a history of triggers shared by the registers.
template < class t_TyChar >
class _l_tag_history : public _l_tag_base< t_TyChar >
{
  typedef _l_tag_history _TyThis;
  typedef _l_tag_base< t_TyChar > _TyBase;
public:
  using typename _TyBase::_TyTagOp;
  using typename _TyBase::_TyPMFnAccept;
  static constexpr int s_knTagPolicy = 2;
  typedef uint32_t _TyIndex; // One more than the index of an entry in the history - zero when no trigger has been crossed.
  // An entry in the history: a trigger crossed at a position and the entry for the trigger crossed before it.
  struct _TyEntry
  {
    _TyPMFnAccept m_pmfnTrigger;
    vtyDataPosition m_pos;
    _TyIndex m_nPrev;
  };

  void Start( size_t _nRegisters, size_t /* _nSlots */ )
  {
    m_rgEntries.clear();
    m_rgRegisters.assign( _nRegisters, 0 );
    m_rgSnapshots[0] = m_rgSnapshots[1] = 0;
    m_nCompactAt = m_nCompactMin;
  }
  void Copy( size_t _nDst, size_t _nSrc )
  {
    m_rgRegisters[ _nDst ] = m_rgRegisters[ _nSrc ];
  }
  void Trigger( size_t _nDst, _TyTagOp const & _rop, vtyDataPosition _pos )
  {
    if ( m_rgEntries.size() >= m_nCompactAt )
      _Compact();
    _TyEntry entry = { _rop.m_pmfnTrigger, _pos, m_rgRegisters[ _nDst ] };
    m_rgEntries.push_back( entry );
    m_rgRegisters[ _nDst ] = (_TyIndex)m_rgEntries.size();
  }
  void Snapshot( size_t _nSnapshot )
  {
    m_rgSnapshots[ _nSnapshot ] = m_rgRegisters[ 0 ];
  }
  template < class t_TyFire >
  void Fire( size_t _nSnapshot, bool _fSnapshotPending, t_TyFire && _rrfire )
  {
    // The history leads backward from the last trigger crossed.
    m_rgUtil.clear();
    for ( _TyIndex nCur = _fSnapshotPending ? m_rgRegisters[ 0 ] : m_rgSnapshots[ _nSnapshot ]; !!nCur; nCur = m_rgEntries[ nCur-1 ].m_nPrev )
      m_rgUtil.push_back( nCur-1 );
    for ( size_t n = m_rgUtil.size(); n--; )
    {
      _TyEntry const & rentry = m_rgEntries[ m_rgUtil[ n ] ];
      _rrfire( rentry.m_pmfnTrigger, rentry.m_pos );
    }
  }
  // The history is compacted - the entries that no register refers to are removed - when it reaches this size
  //	and then whenever it doubles in size.
  void SetCompactMin( size_t _nCompactMin )
  {
    m_nCompactMin = _nCompactMin;
  }
  size_t NEntries() const
  {
    return m_rgEntries.size();
  }
protected:
  // The entries of the ways of matching that have been abandoned remain in the history until the token is found.
  // For a very long token they could be many so we remove them from time to time: we keep the entries that a
  //	register or a snapshot leads to. An entry is always after the entry it refers to so we can do this in place.
  void _Compact()
  {
    m_rgUtil.assign( m_rgEntries.size(), 0 );
    auto fnMark = [this]( _TyIndex _nCur )
    {
      for ( ; !!_nCur && !m_rgUtil[ _nCur-1 ]; _nCur = m_rgEntries[ _nCur-1 ].m_nPrev )
        m_rgUtil[ _nCur-1 ] = 1;
    };
    for ( _TyIndex nRegister : m_rgRegisters )
      fnMark( nRegister );
    fnMark( m_rgSnapshots[0] );
    fnMark( m_rgSnapshots[1] );
    _TyIndex nKept = 0;
    for ( size_t nEntry = 0; nEntry < m_rgEntries.size(); ++nEntry )
    {
      if ( !m_rgUtil[ nEntry ] )
        continue;
      _TyEntry entry = m_rgEntries[ nEntry ];
      if ( !!entry.m_nPrev )
        entry.m_nPrev = m_rgUtil[ entry.m_nPrev-1 ]; // Its new index: it was kept as this entry leads to it.
      m_rgEntries[ nKept ] = entry;
      m_rgUtil[ nEntry ] = ++nKept;
    }
    for ( _TyIndex & rnRegister : m_rgRegisters )
    {
      if ( !!rnRegister )
        rnRegister = m_rgUtil[ rnRegister-1 ];
    }
    for ( _TyIndex & rnSnapshot : m_rgSnapshots )
    {
      if ( !!rnSnapshot )
        rnSnapshot = m_rgUtil[ rnSnapshot-1 ];
    }
    m_rgEntries.resize( nKept );
    m_nCompactAt = (std::max)( m_nCompactMin, size_t( nKept ) * 2 );
  }
  std::vector< _TyEntry > m_rgEntries; // The history.
  std::vector< _TyIndex > m_rgRegisters;
  _TyIndex m_rgSnapshots[2] = { 0, 0 };
  std::vector< _TyIndex > m_rgUtil;
  size_t m_nCompactMin{4096};
  size_t m_nCompactAt{4096};
};

__LEXOBJ_END_NAMESPACE

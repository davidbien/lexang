#ifndef __L_LXOBJ_H
#define __L_LXOBJ_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_lxobj.h

#include <memory>
#include <vector>
#include <string>
#include <stddef.h>
#include "bien_assert.h"
#include <wchar.h>
#include <unordered_set>
#include <typeinfo>
#include "bienutil.h"
#include "_namdexc.h"
#include "_basemap.h"
#include "_l_ns.h"
#include "_l_types.h"
#include "_l_chrtr.h"
#include "_ticont.h"
#include <functional>
#include <algorithm>

#include "_l_axion.h"
#include "_l_state.h"
#include "_l_strm.h"
#include "_l_tag.h"

#ifndef NDEBUG
#define LEXOBJ_STRICT
#endif //!NDEBUG

__LEXOBJ_BEGIN_NAMESPACE

// This exception is thrown when a token is not found in a non-empty stream.
class _l_no_token_found_exception : public _t__Named_exception<>
{
  typedef _t__Named_exception<> _tyBase;
public:
  _l_no_token_found_exception(const string_type &__s)
      : _tyBase(__s)
  {
  }
  _l_no_token_found_exception(const char *_pcFmt, va_list args)
  {
    RenderVA(_pcFmt, args);
  }
};
// By default we will always add the __FILE__, __LINE__ even in retail for debugging purposes.
#define THROWNOTOKENFOUND(MESG, ...) ExceptionUsage<_l_no_token_found_exception>::ThrowFileLineFunc(__FILE__, __LINE__, FUNCTION_PRETTY_NAME, MESG, ##__VA_ARGS__)

template <class t_TyChar>
struct _l_compare_input_with_range
{
  bool operator()(_l_transition<t_TyChar> const &_rrl,
                  typename _l_transition<t_TyChar>::_TyUnsignedChar const &_rucr) const
  {
    return _rrl.m_last < _rucr;
  }
};

// _l_an_mostbase: Must be templatized by t_TyChar to keep other stuff simple as well.
template < class t_TyChar >
struct _l_an_mostbase
{
private:
  typedef _l_an_mostbase _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_action_object_base< _TyChar, false > _TyAxnObjBase;
  typedef bool (_TyThis::*_TyPMFnAccept)();

  void SetToken( _TyAxnObjBase * _paobCurToken )
  {
    m_paobCurToken = _paobCurToken;
  }
  _TyAxnObjBase * PGetToken() const
  { 
    return m_paobCurToken;
  }
protected:
  _TyAxnObjBase * m_paobCurToken{nullptr}; // A pointer to the current token found by the lexang.
};

template <class t_TyTraits, bool t_fSupportLookahead>
struct _l_an_lookaheadbase;

// When lookahead is not supported we need no state at all.
template <class t_TyTraits>
struct _l_an_lookaheadbase<t_TyTraits, false>
    : public _l_an_mostbase<typename t_TyTraits::_TyChar>
{
private:
  typedef _l_an_lookaheadbase _TyThis;
  typedef _l_an_mostbase<typename t_TyTraits::_TyChar> _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  using _TyChar = typename _TyTraits::_TyChar;
  typedef _l_state_proto<_TyChar> _TyStateProto;
  typedef typename _l_char_type_map<_TyChar>::_TyUnsigned _TyUnsignedChar;
};

// The state required to support lookahead rules ( trailing context ): "token / trailing" - see _l_lkahd.h for an overview.
// As we move through the input for a token we record:
// 1) Each position at which we are in a lookahead accept state: one in which the "token" portion of at least one
//    lookahead rule has matched.
// 2) The characters of the input - so that we can move backward through them without requiring that of the transport.
// When a lookahead rule is the rule which matches we move backward from the end of the match through the DFA which
//  matches the trailing context of the rule in reverse. The first accepting state of that DFA we find at a position
//  recorded in (1) for that rule is the end of the token: this gives the longest token for which the remainder of the
//  match is matched by the trailing context.
template <class t_TyTraits>
struct _l_an_lookaheadbase<t_TyTraits, true>
    : public _l_an_mostbase<typename t_TyTraits::_TyChar>
{
private:
  typedef _l_an_lookaheadbase _TyThis;
  typedef _l_an_mostbase<typename t_TyTraits::_TyChar> _TyBase;
public:
  typedef t_TyTraits _TyTraits;
  using _TyChar = typename _TyTraits::_TyChar;
  typedef _l_state_proto<_TyChar> _TyStateProto;
  typedef _l_transition<_TyChar> _TyTransition;
  typedef typename _l_char_type_map<_TyChar>::_TyUnsigned _TyUnsignedChar;

  // A position at which we were in a lookahead accept state.
  struct _TyLookaheadAccept
  {
    vtyDataPosition m_pos;
    const _TyStateProto * m_psp;
  };
  std::vector< _TyLookaheadAccept > m_rgLookaheadAccepts; // In order of position. There may be more than one at a position due to triggers.
  std::vector< _TyUnsignedChar > m_rgucLookaheadToken; // The characters read for the current token.
  vtyDataPosition m_posLookaheadTokenStart{vkdpNullDataPosition}; // The position of the first character of m_rgucLookaheadToken.
  bool m_fLastAcceptIsLookahead{false}; // Is the last accept state a lookahead state: then the end of the token is yet to be found.
  // The last accepting state that was not a lookahead state: should we not find a (non-empty) token for a lookahead rule we use this.
  const _TyStateProto * m_pspLastNonLookaheadAccept{nullptr};
  vtyDataPosition m_posLastNonLookaheadAccept{vkdpNullDataPosition};

  _l_an_lookaheadbase() = default;

  void _InitLookahead( vtyDataPosition _posTokenStart )
  {
    m_rgLookaheadAccepts.clear();
    m_rgucLookaheadToken.clear();
    m_posLookaheadTokenStart = _posTokenStart;
    m_fLastAcceptIsLookahead = false;
    m_pspLastNonLookaheadAccept = nullptr;
    m_posLastNonLookaheadAccept = vkdpNullDataPosition;
  }
  // Is there a lookahead accept state recorded at _pos for the lookahead rule with the action id _aiLookahead?
  // _rnLookaheadAccepts is the count of those recorded that are yet to be considered: we move backward through them
  //  as _pos moves backward.
  bool _FIsLookaheadAcceptAt( vtyDataPosition _pos, vtyActionIdent _aiLookahead, size_t & _rnLookaheadAccepts ) const
  {
    while ( _rnLookaheadAccepts && ( m_rgLookaheadAccepts[ _rnLookaheadAccepts-1 ].m_pos > _pos ) )
      --_rnLookaheadAccepts;
    for ( size_t n = _rnLookaheadAccepts; n && ( m_rgLookaheadAccepts[ n-1 ].m_pos == _pos ); --n )
    {
      if ( m_rgLookaheadAccepts[ n-1 ].m_psp->FIsLookaheadAcceptFor( _aiLookahead ) )
        return true;
    }
    return false;
  }
  // A lookahead rule has matched in its entirety: the state _pspLookahead at position _posEndMatch.
  // Return the position of the end of the token or vkdpNullDataPosition if there is no non-empty token.
  vtyDataPosition _PosFindLookaheadTokenEnd( const _TyStateProto * _pspLookahead, vtyDataPosition _posEndMatch ) const
  {
    Assert( _pspLookahead->FIsLookaheadState() );
    Assert( _posEndMatch >= m_posLookaheadTokenStart );
    Assert( size_t( _posEndMatch - m_posLookaheadTokenStart ) <= m_rgucLookaheadToken.size() );
    const vtyActionIdent aiLookahead = _pspLookahead->AIGetLookahead();
    size_t nLookaheadAccepts = m_rgLookaheadAccepts.size();
    const _TyStateProto * pspReverse = _pspLookahead->PspGetLookaheadReverse();
    Assert( !!pspReverse );
    if ( !pspReverse )
      return vkdpNullDataPosition;
    // Move backward from the end of the match. An empty token is not a token: we would never move forward.
    for ( vtyDataPosition posCur = _posEndMatch; posCur > m_posLookaheadTokenStart; --posCur )
    {
      // If the reverse DFA accepts here then the trailing context matches from posCur to the end of the match.
      if ( !!pspReverse->m_flAccept && _FIsLookaheadAcceptAt( posCur, aiLookahead, nLookaheadAccepts ) )
        return posCur;
      const _TyUnsignedChar ucCur = m_rgucLookaheadToken[ size_t( posCur - 1 - m_posLookaheadTokenStart ) ];
      const _TyTransition * ptrCur = pspReverse->m_rgt;
      const _TyTransition * const ptrEnd = ptrCur + pspReverse->m_nt;
      for ( ; ( ptrEnd != ptrCur ) && !( ( ucCur <= ptrCur->m_last ) && ( ucCur >= ptrCur->m_first ) ); ++ptrCur )
        ;
      if ( ptrEnd == ptrCur )
        break; // The trailing context cannot begin any earlier than this.
      pspReverse = ptrCur->m_psp;
    }
    return vkdpNullDataPosition;
  }
};

// t_TyTagPolicy: The kind of registers for the tagged DFAs of the lexical analyzer: _l_tag_none when there are no
//  tagged DFAs, _l_tag_registers<> or _l_tag_history<> - see _l_tag.h. The generator chooses this.
template < class t_TyTraits, bool t_fSupportLookahead, bool t_fSupportTriggers, bool t_fTrace, class t_TyTagPolicy >
struct _l_analyzer : public _l_an_lookaheadbase< t_TyTraits, t_fSupportLookahead >
{
private:
  typedef _l_analyzer _TyThis;
  typedef _l_an_lookaheadbase< t_TyTraits, t_fSupportLookahead > _TyBase;
public:
  using typename _TyBase::_TyTraits;
  using typename _TyBase::_TyChar;
  using typename _TyBase::_TyStateProto;
  using typename _TyBase::_TyUnsignedChar;
  using typename _TyBase::_TyPMFnAccept;
  using typename _TyBase::_TyAxnObjBase;
  typedef _l_action_object_value_base< _TyTraits, false > _TyAxnObjValueBase;
  using _TyTransport = typename _TyTraits::_TyTransport;
  using _TyTransportCtxt = typename _TyTraits::_TyTransportCtxt;
  using _TyUserObj = typename _TyTraits::_TyUserObj;
  using _TyPtrUserObj = typename _TyTraits::_TyPtrUserObj;
  using _TyTpValueTraits = typename _TyTraits::_TyTpValueTraits;
  typedef basic_string< _TyChar > _TyStdStr;
  typedef _l_stream< _TyTraits > _TyStream;
  typedef _l_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyToken;

  typedef _l_transition<_TyChar> _TyTransition;
  typedef _l_compare_input_with_range<_TyChar> _TyCompSearch;

  static constexpr bool s_kfSupportLookahead = t_fSupportLookahead;
  static constexpr bool s_kfSupportTriggers = t_fSupportTriggers;
  static constexpr bool s_kfTrace = t_fTrace;
  typedef t_TyTagPolicy _TyTagPolicy;
  static constexpr int s_knTagPolicy = t_TyTagPolicy::s_knTagPolicy;
  static constexpr bool s_kfTagged = ( s_knTagPolicy > 0 ); // Does this lexical analyzer support tagged DFAs.

  _TyStream m_stream; // the stream within which is the transport object and user context, etc.
  _TyTagPolicy m_tag; // The registers for tagged DFAs - empty when there are none.

  _TyAxnObjValueBase * m_paobActionListHead{nullptr};
private: // Not sure why I made this private but it must have been very important!!! (j/k) so I am going to leave it.
  const _TyStateProto *m_pspLastAccept{nullptr}; // The last encountered accept state;
public:
  const _TyStateProto *m_pspStart{nullptr}; // The last encountered accept state;
  const _TyStateProto *m_pspCur{nullptr}; // Current state.
  vtyDataPosition m_posLastAccept{vkdpNullDataPosition};
  // The start of the current token is stored in the transport.
  _TyUnsignedChar m_ucCur{0}; // The current character obtained from the transport.
  // Whether there is a current character: the transport gave one. Not whether m_ucCur is zero - a character of the
  //  input may be: zero where there is none was "the end of the input", so a zero in the input was taken for that,
  //  and it was counted into the token before it since "no character was read ahead".
  bool m_fHaveCur{false};
  _TyCompSearch m_compSearch;        // search object.
  // The ranges of a state are searched in order when there are this many or fewer, with a binary search when more - see _getnext().
  static constexpr vTyNTransitions s_knTransitionsSearchedInOrder = 32;

  _l_analyzer() = delete;
  _l_analyzer(const _l_analyzer &) = delete;
  _l_analyzer &operator=(_l_analyzer const &) = delete;

  // There may be no action objects at all (actually that's not true, given the design - but I allow for it).
  _l_analyzer(const _TyStateProto *_pspStart, _TyAxnObjValueBase * _paobActionListHead )
      : m_pspStart(_pspStart),
        m_paobActionListHead( _paobActionListHead )
  {
  }

  // Construct the transport object appropriately. This is how to open a file, etc.
  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    GetStream().emplaceTransport( std::forward< t_TysArgs >( _args )... );
  }
  // Emplace a specific type of transport within a transport_var.
  template < class t_TyTransport, class... t_TysArgs >
  void emplaceVarTransport( t_TysArgs&&... _args )
  {
    GetStream().template emplaceVarTransport< t_TyTransport >( std::forward< t_TysArgs >( _args )... );
  }

  using _TyBase::SetToken;
  using _TyBase::PGetToken;
  _TyStream & GetStream()
  {
    return m_stream;
  }
  const _TyStream & GetStream() const
  {
    return m_stream;
  }
  _TyTransport & GetTransport()
  {
    return m_stream.GetTransport();
  }
  const _TyTransport & GetTransport() const
  {
    return m_stream.GetTransport();
  }
  _TyUserObj & GetUserObj()
  {
    return m_stream.GetUserObj();
  }
  const _TyUserObj & GetUserObj() const
  {
    return m_stream.GetUserObj();
  }
  _TyPtrUserObj & GetUserObjPtr()
  {
    return m_stream.GetUserObjPtr();
  }
  // The triggers of a tagged DFA are fired once the token has been found: the current position is then the position
  //  at which the trigger being fired was crossed.
  vtyDataPosition GetCurrentPosition() const
  {
    if constexpr ( s_kfTagged )
    {
      if ( m_tag.m_fFiring )
        return m_tag.m_posFiring;
    }
    return _PosCurrent();
  }
  // The position of the current character - that at which the lexical analyzer is: for use while moving through the input.
  vtyDataPosition _PosCurrent() const
  {
    return GetStream().PosCurrent() - m_fHaveCur;
  }
  _TyTagPolicy & GetTagPolicy()
  {
    return m_tag;
  }
  // The start state of a tagged DFA refers to one of these methods according to the kind of registers that the DFA
  //  requires: thus the states of a tagged DFA can only be used with a lexical analyzer which has such registers.
  // These are never called.
  bool TagRequireRegisters()
  {
    static_assert( s_knTagPolicy >= 1, "The DFA is a tagged DFA: the lexical analyzer must be generated with the DFA or with _l_generator<>::set_min_tag_policy( 1 )." );
    return true;
  }
  bool TagRequireHistory()
  {
    static_assert( s_knTagPolicy >= 2, "The DFA is a tagged DFA which requires the history of its triggers: the lexical analyzer must be generated with the DFA or with _l_generator<>::set_min_tag_policy( 2 )." );
    return true;
  }
  // This clear the data out of all triggers and tokens. This should be used after input is given to the lex which it fails
  //  to regognize as a token. In that case the various triggers tha may have fired along the way will still contain data.
  void ClearTokenData()
  {
    for ( _TyAxnObjValueBase * paxnCur = static_cast< _TyAxnObjValueBase * >( m_paobActionListHead ); !!paxnCur; paxnCur = paxnCur->PGetAobNext() )
      paxnCur->Clear();
  }
  bool FIsClearOfTokenData( vtyTokenIdent * _ptidNonNull = 0 ) const
  {
    for ( _TyAxnObjValueBase * paxnCur = m_paobActionListHead; !!paxnCur; paxnCur = paxnCur->PGetAobNext() )
    {
      if ( !paxnCur->VFIsNull() )
      {
        // The callers check this with VerifyThrowOrBreakSz(): that breaks into the debugger when one is attached.
        if ( !!_ptidNonNull )
          *_ptidNonNull = paxnCur->VGetTokenId();
        return false;
      }
    }
    return true;
  }
#if TRACESENABLED
#define LXOBJ_DOTRACE(MESG, ...) _DoTrace( __FILE__, __LINE__, FUNCTION_PRETTY_NAME, MESG, ##__VA_ARGS__)
#else //!TRACESENABLED
#define LXOBJ_DOTRACE(MESG, ...)
#endif //!TRACESENABLED
  void _DoTrace(const char *_szFile, unsigned int _nLine, const char *_szFunction, const char *_szMesg, ...)
  {
    std::string strCur;
    if ( m_fHaveCur )
    {
      if ((m_ucCur > 32) && (m_ucCur < 127))
        (void)FPrintfStdStrNoThrow(strCur, "%c (%llu)", (char)m_ucCur, uint64_t(m_ucCur) );
      else
        (void)FPrintfStdStrNoThrow(strCur, "%llu", uint64_t(m_ucCur));
    }

    std::string strMesg;
    if ( strCur.length() )
    {
      (void)FPrintfStdStrNoThrow(strMesg, "Cur char[%s]. %s", strCur.c_str(), _szMesg );
    }
    else
    {
      strMesg = _szMesg;
    }

    Assert(!!m_pspCur);
    if (s_kfTrace)
    {
      n_SysLog::vtyJsoValueSysLog jvDetail = m_pspCur->GetJsonValue( t_fSupportLookahead );
      va_list ap;
      va_start(ap, _szMesg);
      Trace_LogMessageVArg(eabiIgnore, _szFile, _nLine, _szFunction, &jvDetail, strMesg.c_str(), ap);
    }
  }

  void
  _CheckAcceptState()
  {
    if (m_pspCur->m_flAccept)
    {
      if constexpr (t_fSupportLookahead)
      {
        const vtyDataPosition posCur = _PosCurrent();
        // Record that the token of some lookahead rule(s) may end here.
        if ( m_pspCur->FIsLookaheadAcceptState() )
          this->m_rgLookaheadAccepts.push_back( { posCur, m_pspCur } );
        switch (m_pspCur->m_flAccept)
        {
        case kucLookahead:
        case kucLookaheadAcceptAndLookahead:
        {
          // A lookahead rule has matched in its entirety. As with any accepting state this is now the longest match.
          // We find where the token ends - it is before the trailing context - only if this remains the longest
          //  match: see _FFoundAcceptState().
          m_pspLastAccept = m_pspCur;
          m_posLastAccept = posCur;
          this->m_fLastAcceptIsLookahead = true;
          _TagSnapshotPending( false );
        }
        break;
        case kucAccept:
        case kucLookaheadAcceptAndAccept:
        case kucAntiAccepting: // An explicitly excluded match - see _FFoundAcceptState().
        {
          m_pspLastAccept = this->m_pspLastNonLookaheadAccept = m_pspCur;
          m_posLastAccept = this->m_posLastNonLookaheadAccept = posCur;
          this->m_fLastAcceptIsLookahead = false;
          _TagSnapshotPending( true );
        }
        break;
        default: // kucLookaheadAccept: recorded above.
        break;
        }
      }
      else
      {
        LXOBJ_DOTRACE( "Accept found.");
        m_pspLastAccept = m_pspCur;
        m_posLastAccept = _PosCurrent();
        _TagSnapshotPending( false );
      }
    }
  }

  // Tagged DFAs: in an accepting state register zero is the register for the way in which the token is matched.
  // We need it as it is here should this be the last accepting state: but it only changes in a tag operation state so
  //  we just note that a snapshot is pending - see _l_tag_base<> and _execute_tag_ops().
  // _fNonLookahead: this is also the last accepting state which is not a lookahead state.
  void _TagSnapshotPending( bool _fNonLookahead )
  {
    if constexpr ( s_kfTagged )
    {
      m_tag.m_rgfSnapshotPending[ 0 ] = true;
      if ( _fNonLookahead )
        m_tag.m_rgfSnapshotPending[ 1 ] = true;
    }
  }
  // Tagged DFAs: the token has been found - m_pspLastAccept is its accepting state. Fire the triggers that were
  //  crossed in matching it: those recorded as we moved through the input and then those crossed at the end of the
  //  token - which are those of the accepting state. Each fires with the current position being that at which it was crossed.
  void _TagFireTriggers()
  {
    if constexpr ( s_kfTagged )
    {
      if ( !m_tag.m_fTagDfa )
        return;
      struct _TyFiring
      {
        _TyTagPolicy & m_rtag;
        _TyFiring( _TyTagPolicy & _rtag ) : m_rtag( _rtag ) { m_rtag.m_fFiring = true; }
        ~_TyFiring() { m_rtag.m_fFiring = false; }
      } firing( m_tag );
      const size_t knSnapshot = m_tag.m_nSnapshotFire;
      m_tag.Fire( knSnapshot, m_tag.m_rgfSnapshotPending[ knSnapshot ], [this]( _TyPMFnAccept _pmfnTrigger, vtyDataPosition _pos )
      {
        m_tag.m_posFiring = _pos;
        (void)(this->*_pmfnTrigger)();
      } );
      if ( !!m_pspLastAccept->m_nTriggers && !m_pspLastAccept->m_pspTrigger )
      {
        m_tag.m_posFiring = m_tag.m_posFinish;
        _TyPMFnAccept *ppmfnTrigger = m_pspLastAccept->PPMFnGetTriggerBegin();
        _TyPMFnAccept * const ppmfnTriggerEnd = ppmfnTrigger + m_pspLastAccept->m_nTriggers;
        for ( ; ppmfnTrigger != ppmfnTriggerEnd; ++ppmfnTrigger )
          (void)(this->**ppmfnTrigger)();
      }
    }
  }

  // Return whether we found an accepting state for the current token. Call when no further transitions are possible.
  // An anti-accepting state is the result of the excludes operator: it is entered when the input matches that which is to
  //  be excluded. As with _l_match<>::PszMatch(), if the last "accepting" state we encountered was anti-accepting then the
  //  input was explicitly excluded and there is no token - we do not fall back to an earlier, shorter, match. If we passed
  //  through an anti-accepting state to a later accepting state then we have a token as usual.
  // An anti-accepting state has no action - we must not attempt to obtain one from it.
  // If the last accepting state was a lookahead state then the rule has matched including its trailing context: we find
  //  where the token ends. If there is no non-empty token then we use the last accepting state that was not a lookahead state.
  bool _FFoundAcceptState()
  {
    if constexpr ( s_kfTagged )
      m_tag.m_posFinish = m_posLastAccept; // The triggers crossed at the end of the token fire at the end of the match.
    if constexpr ( t_fSupportLookahead )
    {
      if ( !!m_pspLastAccept && this->m_fLastAcceptIsLookahead )
      {
        const vtyDataPosition posEndToken = this->_PosFindLookaheadTokenEnd( m_pspLastAccept, m_posLastAccept );
        if ( vkdpNullDataPosition != posEndToken )
          m_posLastAccept = posEndToken;
        else
        {
          m_pspLastAccept = this->m_pspLastNonLookaheadAccept;
          m_posLastAccept = this->m_posLastNonLookaheadAccept;
          if constexpr ( s_kfTagged )
          {
            m_tag.m_nSnapshotFire = 1; // The triggers are then those of that accepting state.
            m_tag.m_posFinish = m_posLastAccept;
          }
        }
        this->m_fLastAcceptIsLookahead = false;
      }
    }
    if ( !!m_pspLastAccept && m_pspLastAccept->FIsAntiAcceptingState() )
      m_pspLastAccept = 0;
    return !!m_pspLastAccept;
  }

  void _InitGetToken( const _TyStateProto *_pspStart )
  {
    m_pspCur = !_pspStart ? m_pspStart : _pspStart; // Start at the beginning again...
    // A generated analyzer containing multiple DFAs has no default start state - the caller must then pass one.
    VerifyThrowSz( !!m_pspCur, "No start state: this analyzer has no default start state so one must be passed in." );
    m_pspLastAccept = 0; // Regardless.
    if constexpr (t_fSupportLookahead)
      this->_InitLookahead( GetStream().PosTokenStart() );
    m_posLastAccept = vkdpNullDataPosition;
    if constexpr ( s_kfTagged )
    {
      m_tag.m_fTagDfa = false; // The start state of a tagged DFA will tell us if this is one.
      m_tag.m_nSnapshotFire = 0;
      m_tag.m_rgfSnapshotPending[ 0 ] = m_tag.m_rgfSnapshotPending[ 1 ] = false;
    }
  }

  // Just return a single token. Return false if one isn't found.
  // If _rpuToken already contains a token then that token object is reused for the token found - its contents are replaced.
  //  This saves allocating a token object for each token: a caller that is done with the token can just pass it back in.
  //  If no token is found then _rpuToken is left as it was.
  // Caller can append a set of token ids to ignore while parsing. This will short-circuit things - not creating a token from the trigger data - just clearing the trigger data.
  // For instance an XML parser may be in "ignore comments and processing instructions" mode and then we would just skip those after recognizing them and move on to the next token if any.
  bool FGetToken( unique_ptr< _TyToken > & _rpuToken, vtyTokenIdent * _ptidIgnoreBegin = nullptr, vtyTokenIdent * _ptidIgnoreEnd = nullptr, const _TyStateProto *_pspStart = nullptr, bool _fThrowOnNoToken = false )
  {
#ifdef LEXOBJ_STRICT
    {//B: We should be clear at the beginning of GetToken.
      vtyTokenIdent tidNonNull;
      VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "At beginning: Token id[%u] still has data in it - this will result in bogus translations."
        "This can happen when a trigger has fired that is not contained in a token and hence never gets cleared."
        "That's a bogus trigger anyway and you should just include it in the eventual token(s) where it fires.", tidNonNull  );
    }//EB
#endif //LEXOBJ_STRICT
    LXOBJ_DOTRACE("At start.");
    bool fIgnoredToken;
    do
    {
      fIgnoredToken = false;
      _InitGetToken( _pspStart );
      Assert( GetStream().FAtTokenStart() ); // We shouldn't be mid-token.
      _NextChar();
      do
      {
        _CheckAcceptState();
      } 
      while ( _getnext() );

      if ( _FFoundAcceptState() )
      {
        _TagFireTriggers();
        _TyPMFnAccept pmfnAccept = m_pspLastAccept->PMFnGetAction();
        m_pspLastAccept = 0; // Regardless.
        Assert( !!pmfnAccept ); // Without an accept action we don't even know what token we found.
        if ( !!pmfnAccept )
        {
          // See of the token wants to be accepted as the current action.
          // This method will call SetToken() to set the current token.
          Assert( !PGetToken() ); // Why should we have a token now.
          AssertStatement( SetToken(nullptr) );
          if ( (this->*pmfnAccept)() )
          {
            VerifyThrowSz( PGetToken(), "No token after calling the accept action. The token accept action method must set an action object pointer to a member action object as the token." );
            _TyAxnObjValueBase * paobCurToken = static_cast< _TyAxnObjValueBase * >( PGetToken() );
            SetToken(nullptr);
            // Check if this is one of the tokens we are to ignore, the process and potentially filter the token using the user object.
            if ( ( !_ptidIgnoreBegin || ( _ptidIgnoreEnd == find( _ptidIgnoreBegin, _ptidIgnoreEnd, paobCurToken->VGetTokenId() ) ) ) &&
                 !GetStream().GetUserObj().FProcessAndFilterToken( paobCurToken, GetStream(), m_posLastAccept ) )
            {
              // Delegate to the stream to obtain the token as it needs to get the context from the transport.
              // This reuses any token object that the caller passed in.
              GetStream().GetPToken( paobCurToken, m_posLastAccept, _rpuToken );
              // Getting the token should result in a completely clear set of action objects for the entire lexical analyzer (invariant of the algorithm/system).
  #ifdef LEXOBJ_STRICT
              vtyTokenIdent tidNonNull;
              VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "Token id[%u] still has data in it - this will result in bogus translations."
                "This can happen when a trigger has fired that is not contained in a token and hence never gets cleared."
                "That's a bogus trigger anyway and you should just include it in the eventual token(s) where it fires.", tidNonNull  );
  #endif //LEXOBJ_STRICT
              return true;
            }
            else
            {
              // Ignoring this token:
              fIgnoredToken = true;
              LXOBJ_DOTRACE("Ignoring token:[%u]", paobCurToken->VGetTokenId() );
              // We only need clear this token - the rest of the action objects in the state machine should be clear (must be clear):
              paobCurToken->Clear();
  #ifdef LEXOBJ_STRICT
              vtyTokenIdent tidNonNull;
              VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "Token id[%u] still has data in it - this will result in bogus translations."
                "This can happen when a trigger has fired that is not contained in a token and hence never gets cleared."
                "That's a bogus trigger anyway and you should just include it in the eventual token(s) where it fires.", tidNonNull  );
  #endif //LEXOBJ_STRICT
              GetStream().DiscardData( m_posLastAccept ); // Skip everything that we found.
            }
          }
          else
          {
            Assert( !PGetToken() ); // If the accept method rejects the token then it shouldn't set one.
            // We had hit a dead end. To continue from here we must return to the start state.
            // If the token action didn't accept then it should have cleared all token data:
  #ifdef LEXOBJ_STRICT
            vtyTokenIdent tidNonNull;
            VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "Token id[%u] still has data in it - this will result in bogus translations.", tidNonNull  ); 
  #endif //LEXOBJ_STRICT
            // As with FGetTokens() we skip everything that we found and go on to the next token: returning false here would
            //  be indistinguishable from reaching the end of the input.
            // We could keep a stack of previously found accepting states and move to them but that's kinda ill defined and costs allocation space. Something to think about.
            const bool kfMatchedInput = ( m_posLastAccept > GetStream().PosTokenStart() );
            GetStream().DiscardData( m_posLastAccept );
            fIgnoredToken = kfMatchedInput; // Continue with the next token - unless we matched nothing: we would never move forward.
          }
        }
      }
      else
      {
        // No accepting state found: triggers may have fired along the way - e.g. the caller may try an optional token from one
        //  start state and on failure scan the same input again from another. Clear them so that the next scan starts clean.
        ClearTokenData();
      }
      // No accepting state found. If we aren't at EOF then we should throw - or if we are at EOF and not at the start of a token.
      if ( !fIgnoredToken && ( m_fHaveCur || !GetStream().FAtTokenStart() ) && _fThrowOnNoToken )
      {
        _TyStdStr strCurToken;
        GetStream().GetCurTokenString( strCurToken );
        THROWNOTOKENFOUND(m_ucCur ? "Not at EOF, current token:[%s] m_ucCur[%c]" : "Not at EOF, current token:[%s]", StrConvertString<char>( strCurToken ).c_str(), (char)m_ucCur );
      }
    }
    while( fIgnoredToken );
    return false;
  }

  // Keep getting tokens until we hit eof or the callback method returns false.
  // The callback receives a unique_ptr<> to the token: it may take ownership of the token by moving it out. If it doesn't
  //  then the token object is reused for the next token.
  // If we process things as much as possible - i.e. get tokens until we are supposed to, then we return true and _pspStateFailing is set to nullptr.
  // If we encounter a state where we cannot move forward on input then we return false. The current state is then still set to the state from which we were unable to continue.
  template < class t_tyCallback >
  bool FGetTokens( t_tyCallback _callback, const _TyStateProto *_pspStart = nullptr )
  {
    Assert( GetStream().FAtTokenStart() ); // We shouldn't be mid-token.
    unique_ptr< _TyToken > upToken; // Reused for each token unless the callback takes ownership of it.
    do
    {
      _InitGetToken( _pspStart );
      Assert( GetStream().FAtTokenStart() ); // We shouldn't be mid-token.
      _NextChar();
      LXOBJ_DOTRACE("At start.");
      do
      {
        _CheckAcceptState();
      } 
      while ( _getnext() );

      if ( _FFoundAcceptState() )
      {
        _TagFireTriggers();
        _TyPMFnAccept pmfnAccept = m_pspLastAccept->PMFnGetAction();
        m_pspLastAccept = 0; // Regardless.
        Assert( !!pmfnAccept ); // Without an accept action we don't even know what token we found.
        if ( !!pmfnAccept )
        {
          // See of the token wants to be accepted as the current action.
          // This method will call SetToken() to set the current token.
          Assert( !PGetToken() ); // Why should we have a token now.
          __DEBUG_STMT( SetToken(nullptr) );
          if ( (this->*pmfnAccept)() )
          {
            VerifyThrowSz( PGetToken(), "No token after calling the accept action. The token accept action method must set an action object pointer to a member action object as the token." );
            _TyAxnObjValueBase * paobCurToken = static_cast< _TyAxnObjValueBase * >( PGetToken() );
            SetToken(nullptr);
            GetStream().GetPToken( paobCurToken, m_posLastAccept, upToken );
#ifdef LEXOBJ_STRICT
            vtyTokenIdent tidNonNull;
            VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "Token id[%u] still has data in it - this will result in bogus translations."
              "This can happen when a trigger has fired that is not contained in a token and hence never gets cleared."
              "That's a bogus trigger anyway and you should just include it in the eventual token(s) where it fires.", tidNonNull  );
#endif //LEXOBJ_STRICT
            if ( !_callback( upToken ) )
              return true; // The caller is done with getting tokens for now.
            // else continue to get tokens.
          }
          else
          {
            Assert( !PGetToken() ); // If the accept method rejects the token then it shouldn't set one.
            // We had hit a dead end. To continue from here we must return to the start state.
            // If the token action didn't accept then it should have cleared all token data:
#ifdef LEXOBJ_STRICT
            vtyTokenIdent tidNonNull;
            VerifyThrowOrBreakSz( FIsClearOfTokenData( &tidNonNull ), "Token id[%u] still has data in it - this will result in bogus translations.", tidNonNull  ); 
#endif //LEXOBJ_STRICT
            GetStream().DiscardData( m_posLastAccept ); // Skip everything that we found... kinda harsh..
            // We could keep a stack of previously found accepting states and move to them but that's kinda ill defined and costs allocation space. Something to think about.
          }
        }
      }
      else
      {
        // No accepting state found: triggers may have fired along the way - clear them so the analyzer can be used again.
        ClearTokenData();
        // If we aren't at EOF then we should throw - or if we are at EOF and not at the start of a token.
        if ( m_fHaveCur || !GetStream().FAtTokenStart() )
        {
          _TyStdStr strCurToken;
          GetStream().GetCurTokenString( strCurToken );
          THROWNOTOKENFOUND("Not at EOF, m_ucCur[%c] current token:[%s]", (char)m_ucCur, StrConvertString<char>( strCurToken ).c_str() );
        }
        return false;
      }
    } while (m_fHaveCur);
    return true; // Hit EOF having processed everything.
  }

protected:
  void _NextChar()
  {
    m_ucCur = 0;
    m_fHaveCur = m_stream.FGetChar( m_ucCur );
    if constexpr (t_fSupportLookahead)
    {
      // Record the characters of the token: we may need to move backward through them - see _l_an_lookaheadbase<>.
      if ( m_fHaveCur )
        this->m_rgucLookaheadToken.push_back( m_ucCur );
    }
  }
  void _execute_triggers()
  {
    // Execute the triggers and then advance the state to the trigger state:
    _TyPMFnAccept *ppmfnTrigger = m_pspCur->PPMFnGetTriggerBegin();
    _TyPMFnAccept *ppmfnTriggerEnd = ppmfnTrigger + m_pspCur->m_nTriggers;
    // Change now - this allows the trigger to change the state if desired.
    m_pspCur = m_pspCur->m_pspTrigger;
    for (; ppmfnTrigger != ppmfnTriggerEnd; ++ppmfnTrigger)
    {
      (void)(this->**ppmfnTrigger)();
    }
  }

  // Tagged DFAs: perform the operations of a tag operation state and move to the state it leads to.
  void _execute_tag_ops()
  {
    typedef _l_tag_op< _TyChar > _TyTagOp;
    const _TyTagOp *pop = m_pspCur->PTagOpsBegin();
    const _TyTagOp * const popEnd = pop + m_pspCur->m_nTriggers;
    m_pspCur = m_pspCur->m_pspTrigger;
    // The registers are about to change: take any snapshot of register zero that is pending for an accepting state.
    if ( m_tag.m_rgfSnapshotPending[ 0 ] )
    {
      m_tag.Snapshot( 0 );
      m_tag.m_rgfSnapshotPending[ 0 ] = false;
    }
    if constexpr ( t_fSupportLookahead )
    {
      if ( m_tag.m_rgfSnapshotPending[ 1 ] )
      {
        m_tag.Snapshot( 1 );
        m_tag.m_rgfSnapshotPending[ 1 ] = false;
      }
    }
    // We enter a tag operation state having consumed the character of the transition - the triggers were crossed before it.
    const vtyDataPosition kposBefore = _PosCurrent() - 1;
    for ( ; pop != popEnd; ++pop )
    {
      switch ( pop->m_usKind )
      {
        case kusTagOpCopy:
          m_tag.Copy( pop->m_nDst, pop->m_nSrc );
        break;
        case kusTagOpTrigger:
          m_tag.Trigger( pop->m_nDst, *pop, kposBefore );
        break;
        default:
          Assert( kusTagOpStart == pop->m_usKind );
          // The start state of a tagged DFA: we have yet to consume anything.
          m_tag.Start( pop->m_nDst, pop->m_nSrc );
          m_tag.m_fTagDfa = true;
        break;
      }
    }
  }

  // Move to the next state - return true if we either advanced the state or both advanced the state and the stream.
  bool _getnext()
  {
    switch (m_pspCur->m_nt)
    {
      case 0:
        break;

      case 1:
      {
        if (m_ucCur <= m_pspCur->m_rgt[0].m_last &&
            m_ucCur >= m_pspCur->m_rgt[0].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[0].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
      break;

      case 2:
      {
        if (m_ucCur <= m_pspCur->m_rgt[0].m_last &&
            m_ucCur >= m_pspCur->m_rgt[0].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[0].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[1].m_last &&
                m_ucCur >= m_pspCur->m_rgt[1].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[1].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
      break;

      case 3:
      {
        if (m_ucCur <= m_pspCur->m_rgt[0].m_last &&
            m_ucCur >= m_pspCur->m_rgt[0].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[0].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[1].m_last &&
                m_ucCur >= m_pspCur->m_rgt[1].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[1].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[2].m_last &&
                m_ucCur >= m_pspCur->m_rgt[2].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[2].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
      break;

      case 4:
      {
        if (m_ucCur <= m_pspCur->m_rgt[0].m_last &&
            m_ucCur >= m_pspCur->m_rgt[0].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[0].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[1].m_last &&
                m_ucCur >= m_pspCur->m_rgt[1].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[1].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[2].m_last &&
                m_ucCur >= m_pspCur->m_rgt[2].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[2].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[3].m_last &&
                m_ucCur >= m_pspCur->m_rgt[3].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[3].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
      break;

      case 5:
      {
        if (m_ucCur <= m_pspCur->m_rgt[0].m_last &&
            m_ucCur >= m_pspCur->m_rgt[0].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[0].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[1].m_last &&
                m_ucCur >= m_pspCur->m_rgt[1].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[1].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[2].m_last &&
                m_ucCur >= m_pspCur->m_rgt[2].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[2].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[3].m_last &&
                m_ucCur >= m_pspCur->m_rgt[3].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[3].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
        else if (m_ucCur <= m_pspCur->m_rgt[4].m_last &&
                m_ucCur >= m_pspCur->m_rgt[4].m_first)
        {
          m_pspCur = m_pspCur->m_rgt[4].m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
      break;

      default:
      {
        // The ranges are in order: the first whose last is not below the character is the one - or there is none.
        // The characters of most input are among the first of the ranges, so this finds them sooner than a binary
        //  search does - and with branches which are predicted. As measured, reading XML: the states in which most
        //  of a document is read have from 9 ( UTF-16, UTF-32 ) to 18 ( UTF-8 ) ranges, and it is faster for them all.
        if ( m_pspCur->m_nt <= s_knTransitionsSearchedInOrder )
        {
          const _TyTransition * ptCur = m_pspCur->m_rgt;
          const _TyTransition * const ptEnd = ptCur + m_pspCur->m_nt;
          for ( ; ( ptEnd != ptCur ) && ( ptCur->m_last < m_ucCur ); ++ptCur )
            ;
          if ( ( ptEnd != ptCur ) && ( m_ucCur >= ptCur->m_first ) )
          {
            m_pspCur = ptCur->m_psp;
            _NextChar();
            LXOBJ_DOTRACE( "Moved to state." );
            return true;
          }
          break;
        }
        // Enough elements to warrant a binary search:
        const _TyTransition *ptLwr =
            lower_bound(m_pspCur->m_rgt,
                        m_pspCur->m_rgt + m_pspCur->m_nt,
                        m_ucCur, m_compSearch);
        // lower_bound() returns the end of the table when m_ucCur is beyond every range - don't dereference it then.
        if ((ptLwr != m_pspCur->m_rgt + m_pspCur->m_nt) &&
            (m_ucCur >= ptLwr->m_first) &&
            (m_ucCur <= ptLwr->m_last))
        {
          m_pspCur = ptLwr->m_psp;
          _NextChar();
          LXOBJ_DOTRACE( "Moved to state." );
          return true;
        }
      }
    }
    if (t_fSupportTriggers)
    {
      // Then must check for a trigger transition:
      if (m_pspCur->m_nTriggers)
      {
        if constexpr ( s_kfTagged )
        {
          if ( m_pspCur->FIsTagOpState() )
          {
            _execute_tag_ops();
            LXOBJ_DOTRACE( "Executed tag operations." );
            return true; // advanced the state.
          }
          // The triggers of an accepting state of a tagged DFA are those crossed at the end of its token - they
          //  don't lead anywhere: we fire them if this turns out to be the token - see _TagFireTriggers().
          if ( !m_pspCur->m_pspTrigger )
            return false;
        }
        // Then have one - execute the triggers:
        Assert( !!m_pspCur->m_pspTrigger );
        _execute_triggers();
        LXOBJ_DOTRACE( "Executed triggers." );
        return true; // advanced the state.
      }
      else
      if ( !!m_pspCur->m_pspTrigger ) // This is the "gateway trigger" to a trigger state.
      {
        Assert( !!m_pspCur->m_pspTrigger->m_nTriggers ); // We should be headed to a trigger state.
        m_pspCur = m_pspCur->m_pspTrigger; // Move to the trigger.
        return true; // advanced the state.
      }
    }
    return false;
  }
};

#if 0 // This code needs updating and it probably will never be used anyway.
// When the rules indicate unique accepting states ( i.e. with no out transitions )
//	and no lookaheads then we can use this faster analyzer base:
template <class t_TyTransport, class t_TyUserObj, bool t_fSupportLookahead,
          bool t_fSupportTriggers, bool t_fTrace>
struct _l_analyzer_unique_onematch
    : public _l_analyzer<t_TyTransport, class t_TyUserObj, false, t_fSupportTriggers, t_fTrace>
{
private:
  typedef _l_analyzer<t_TyTransport, class t_TyUserObj, false, t_fSupportTriggers, t_fTrace> _TyBase;
  using _TyBase::_getnext;
  using _TyBase::m_pcCur;
  using _TyBase::m_pspCur;
  using _TyBase::m_pspStart;

public:
  typedef typename _TyBase::_TyUnsignedChar _TyUnsignedChar;
  typedef typename _TyBase::_TyStateProto _TyStateProto;
  typedef typename _TyBase::_TyPMFnAccept _TyPMFnAccept;

  _l_analyzer_unique_onematch(_TyStateProto *_pspStart)
      : _TyBase(_pspStart)
  {
  }

  // get the next accepted state. Return false if no accepted state found.
  // Update <_rpc> appropriately.
  bool get(t_TyChar *&_rpc)
  {
    m_pcCur = reinterpret_cast<_TyUnsignedChar *>(_rpc);

    m_pspCur = m_pspStart;
    do
    {
    } while (_getnext());

    if (m_pspCur->m_flAccept)
    {
      _rpc = m_pcCur;

      _TyPMFnAccept pmfnAccept;
      if (pmfnAccept = m_pspCur->PMFnGetAction())
      {
        (void)(this->*pmfnAccept)();
      }
      return true;
    }
    else
    {
      // No accepting state found - don't modify string.
      return false;
    }
  }
};
#endif //0

#if 0 // unused code - don't see the point.
// Simple token analyzer - works with _l_action_token:
template <class t_TyBaseAnalyzer>
struct _l_token_analyzer
    : public t_TyBaseAnalyzer
{
private:
  typedef _l_token_analyzer<t_TyBaseAnalyzer> _TyThis;
  typedef t_TyBaseAnalyzer _TyBase;

public:
  typedef typename _TyBase::_TyChar _TyChar;

  _l_token_analyzer()
      : t_TyBaseAnalyzer()
  {
  }
};
#endif //0

__LEXOBJ_END_NAMESPACE

#endif //__L_LXOBJ_H

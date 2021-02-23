#pragma once

// _l_state.h
// The state and transition objects for the implementation of the lexical analyzer.
// dbien
// 18DEC2020 - moved from _l_lxobj.h - original code from 1999.

#include "_l_ns.h"
#include "_l_types.h"
#include "_l_chrtr.h"

__LEXOBJ_BEGIN_NAMESPACE

template <class t_TyChar>
struct _l_transition
{
  typedef typename _l_char_type_map<t_TyChar>::_TyUnsigned _TyUnsignedChar;
  _TyUnsignedChar m_first;
  _TyUnsignedChar m_last;
  _l_state_proto<t_TyChar> *m_psp;
  static std::string StaticGetJsonText(_TyUnsignedChar _uc)
  {
    std::string str;
    if ((_uc > 32) && (_uc < 127))
      (void)FPrintfStdStrNoThrow(str, "%c (%lu)", (char)_uc, (uint64_t)_uc);
    else
      (void)FPrintfStdStrNoThrow(str, "%lu", (uint64_t)_uc);
    return str;
  }
  n_SysLog::vtyJsoValueSysLog GetJsonValue() const
  {
    n_SysLog::vtyJsoValueSysLog jv(ejvtObject);
    jv("f").SetStringValue( StaticGetJsonText( m_first ) );
    jv("l").SetStringValue( StaticGetJsonText( m_last ) );
#ifdef LXOBJ_STATENUMBERS
    jv("n").SetValue( m_psp->m_nState );
#endif //LXOBJ_STATENUMBERS
    return (jv);
  }
};

// prototype struct - never gets instantiated - only cast to:
// don't ever use this as arg to sizeof().
template <class t_TyChar>
struct _l_state_proto
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;
  typedef typename _TyAnalyzer::_TyPMFnAccept _TyPMFnAccept;
  typedef _l_transition<t_TyChar> _TyTransition;

// Common members, all instantiations of _l_state<> have these.
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState; // For debugging, not used by the lexang.
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _TyTransition m_rgt[7]; // You can access the transitions - i.e. it is ok to access them without using an accessor - 7 is just a random number - more or less.
private: // Variable length structure - use accessors.
  _TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;             // The associated lookahead action id.
  vtyLookaheadVector m_rgValidLookahead[2]; // bit vector for valid associated lookahead actions.
  _TyPMFnAccept m_rgpmfnTriggers[7];        // Array of pointers to trigger functions - there can be more or less than 7 - random number.
public:
  _TyPMFnAccept PMFnGetAction() const
  {
    Assert(m_flAccept);
    return *(_TyPMFnAccept *)((char *)this + m_usOffsetAccept);
  }
  vtyActionIdent AIGetLookahead() const
  {
    typedef _l_state<t_TyChar, 1, true, true, 3, 3> _TyStateAccept1Trans;
    return *(vtyActionIdent *)((char *)this + m_usOffsetAccept +
                               (offsetof(_TyStateAccept1Trans, m_aiLookahead) - offsetof(_TyStateAccept1Trans, m_pmfnAccept)));
  }
  vtyLookaheadVector *PBeginValidLookahead() const
  {
    Assert(m_flAccept == kucLookaheadAccept ||
           m_flAccept == kucLookaheadAcceptAndAccept ||
           m_flAccept == kucLookaheadAcceptAndLookahead);
    typedef _l_state<t_TyChar, 1, true, true, 3, 3> _TyStateAccept1Trans;
    return (vtyLookaheadVector *)((char *)this + m_usOffsetAccept +
                                  (offsetof(_TyStateAccept1Trans, m_rgValidLookahead) - offsetof(_TyStateAccept1Trans, m_pmfnAccept)));
  }
  _TyPMFnAccept *PPMFnGetTriggerBegin() const
  {
    return (_TyPMFnAccept *)((char *)this + m_usOffsetTriggers);
  }
  n_SysLog::vtyJsoValueSysLog GetJsonValue( bool _fLookahead ) const
  {
    n_SysLog::vtyJsoValueSysLog jv(ejvtObject);
#ifdef LXOBJ_STATENUMBERS
    jv("State").SetValue(m_nState);
#endif //LXOBJ_STATENUMBERS
    if ( m_nt )
    {
      jv("nTransitions").SetValue(m_nt);
      n_SysLog::vtyJsoValueSysLog & jvTransitions = jv("Transitions");
      const _TyTransition * ptCur = m_rgt;
      const _TyTransition * const ptEnd = ptCur + m_nt;
      for( ; ptEnd != ptCur; ++ptCur )
        jvTransitions( ptCur - m_rgt ) = ptCur->GetJsonValue();
    }
    Assert( !m_nTriggers == !m_pspTrigger );
    if ( m_nTriggers )
    {
      jv("nTriggers") = m_nTriggers;
#ifdef LXOBJ_STATENUMBERS
      if ( !!m_pspTrigger )
        jv("TriggerState") = m_pspTrigger->m_nState;
#endif //LXOBJ_STATENUMBERS
    }
    if ( _fLookahead )
    {
      // TODO - later.
    }
    return jv;
  }
};

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, false, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, false, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, true, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, true, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, true, true, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iLookaheadVectorEls, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, true, t_iLookaheadVectorEls, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  vtyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions, int t_iLookaheadVectorEls>
struct _l_state<t_TyChar, t_iTransitions, true, true, t_iLookaheadVectorEls, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  vtyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, false, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, false, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, true, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, true, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, true, true, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iLookaheadVectorEls, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, true, t_iLookaheadVectorEls, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  vtyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iLookaheadVectorEls>
struct _l_state<t_TyChar, 0, true, true, t_iLookaheadVectorEls, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _TyStateFlags m_flAccept;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  vtyTokenIdent m_tidAccept; // This is the token id of any accept action associated with this state or vktidInvalidIdToken.
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  vtyActionIdent m_aiLookahead;
  vtyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

__LEXOBJ_END_NAMESPACE

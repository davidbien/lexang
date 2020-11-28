#ifndef __L_LXOBJ_H
#define __L_LXOBJ_H

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_lxobj.h

#include <memory>
#include <stddef.h>
#include "_assert.h"
#include <wchar.h>
#ifdef __LEXANG_USE_STLPORT
#include <stl/_alloc.h>
#include <hash_set>
#else //__LEXANG_USE_STLPORT
#include <unordered_set>
#endif //__LEXANG_USE_STLPORT
#include <typeinfo>
#include "bienutil.h"
#include "_basemap.h"
#include "_l_ns.h"
#include "_l_chrtr.h"
#include "_ticont.h"
#include <functional>
#include <algorithm>

#include "_l_axion.h"

#ifdef __LEXANG_USE_STLPORT
#define _STLP_ZERO_SIZE_ARRAYS // This isn't part of the current STLport but we still have it here.
#endif                         //__LEXANG_USE_STLPORT

__LEXOBJ_BEGIN_NAMESPACE

__REGEXP_USING_NAMESPACE

#ifdef LXOBJ_STATENUMBERS
typedef unsigned short _TyStateNumber;  // Type for state number.
#endif                                  // LXOBJ_STATENUMBERS
typedef unsigned short _TyNTransitions; // Type for number of transitions ( could make unsigned short ).
typedef unsigned short _TyNTriggers;
typedef signed char _TyStateFlags; // Type for state flags.

const unsigned char kucAccept = 1;          // Normal accept state.
const unsigned char kucLookahead = 2;       // Lookahead state.
const unsigned char kucLookaheadAccept = 3; // Lookahead accept state.
// This state is both a lookahead accept state and a normal accept state.
// If the lookahead suffix is seen then that action is performed - if the lookahead
//	suffix is never seen then this action is performed.
const unsigned char kucLookaheadAcceptAndAccept = 4;
// Similar to above.
const unsigned char kucLookaheadAcceptAndLookahead = 5;

template <class t_TyChar>
struct _l_state_proto;

template <class t_TyChar>
struct _l_an_mostbase;

template <class t_TyChar, bool t_fSupportLookahead, bool t_fSupportTriggers, bool t_fTrace = false>
struct _l_analyzer;

template <class t_TyChar, int t_iTransitions,
          bool t_fAccept, bool t_fLookahead,
          int t_iLookaheadVectorEls,
          int t_iTriggers>
struct _l_state;

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
    return std::move( str );
  }
  n_SysLog::vtyJsoValueSysLog GetJsonValue() const
  {
    n_SysLog::vtyJsoValueSysLog jv(ejvtObject);
    jv("f").SetStringValue( StaticGetJsonText( m_first ) );
    jv("l").SetStringValue( StaticGetJsonText( m_last ) );
#ifdef LXOBJ_STATENUMBERS
    jv("n").SetValue( m_psp->m_nState );
#endif //LXOBJ_STATENUMBERS
    return std::move(jv);
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
  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _TyTransition m_rgt[7]; // You can access the transitions.
private:                            // Variable length structure - use accessors.
  _TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;             // The associated lookahead action id.
  _TyLookaheadVector m_rgValidLookahead[2]; // bit vector for valid associated lookahead actions.
  _TyPMFnAccept m_rgpmfnTriggers[7];        // Array of pointers to trigger functions.
public:
  _TyPMFnAccept PMFnGetAction()
  {
    Assert(m_flAccept);
    return *(_TyPMFnAccept *)((char *)this + m_usOffsetAccept);
  }
  _TyActionIdent AIGetLookahead()
  {
    typedef _l_state<t_TyChar, 1, true, true, 3, 3> _TyStateAccept1Trans;
    return *(_TyActionIdent *)((char *)this + m_usOffsetAccept +
                               (offsetof(_TyStateAccept1Trans, m_aiLookahead) - offsetof(_TyStateAccept1Trans, m_pmfnAccept)));
  }
  _TyLookaheadVector *PBeginValidLookahead()
  {
    Assert(m_flAccept == kucLookaheadAccept ||
           m_flAccept == kucLookaheadAcceptAndAccept ||
           m_flAccept == kucLookaheadAcceptAndLookahead);
    typedef _l_state<t_TyChar, 1, true, true, 3, 3> _TyStateAccept1Trans;
    return (_TyLookaheadVector *)((char *)this + m_usOffsetAccept +
                                  (offsetof(_TyStateAccept1Trans, m_rgValidLookahead) - offsetof(_TyStateAccept1Trans, m_pmfnAccept)));
  }
  _TyPMFnAccept *PPMFnGetTriggerBegin()
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
    return std::move(jv);
  }
};

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, false, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, false, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, true, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, true, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions>
struct _l_state<t_TyChar, t_iTransitions, true, true, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTransitions, int t_iLookaheadVectorEls, int t_iTriggers>
struct _l_state<t_TyChar, t_iTransitions, true, true, t_iLookaheadVectorEls, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  _TyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iTransitions, int t_iLookaheadVectorEls>
struct _l_state<t_TyChar, t_iTransitions, true, true, t_iLookaheadVectorEls, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  _l_transition<t_TyChar> m_rgt[t_iTransitions];
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  _TyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, false, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, false, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, false, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, true, false, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, true, 0, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar>
struct _l_state<t_TyChar, 0, true, true, 0, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar, int t_iLookaheadVectorEls, int t_iTriggers>
struct _l_state<t_TyChar, 0, true, true, t_iLookaheadVectorEls, t_iTriggers>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  _TyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
  typename _TyAnalyzer::_TyPMFnAccept m_rgpmfnTriggers[t_iTriggers];
};
#ifndef _STLP_ZERO_SIZE_ARRAYS
template <class t_TyChar, int t_iLookaheadVectorEls>
struct _l_state<t_TyChar, 0, true, true, t_iLookaheadVectorEls, 0>
{
  typedef _l_an_mostbase<t_TyChar> _TyAnalyzer;

  _TyStateFlags m_flAccept;
#ifdef LXOBJ_STATENUMBERS
  _TyStateNumber m_nState;
#endif //LXOBJ_STATENUMBERS
  _TyNTransitions m_nt;
  _TyNTriggers m_nTriggers;
  _l_state_proto<t_TyChar> *m_pspTrigger; // Transition on trigger.
  unsigned short m_usOffsetAccept;
  unsigned short m_usOffsetTriggers;
  typename _TyAnalyzer::_TyPMFnAccept m_pmfnAccept;
  _TyActionIdent m_aiLookahead;
  _TyLookaheadVector m_rgValidLookahead[t_iLookaheadVectorEls];
};
#endif //!_STLP_ZERO_SIZE_ARRAYS

template <class t_TyChar>
struct _l_compare_input_with_range
#ifdef __LEXANG_USE_STLPORT
    : public binary_function<_l_transition<t_TyChar>,
                             typename _l_transition<t_TyChar>::_TyUnsignedChar, bool>
#endif //__LEXANG_USE_STLPORT
{
  bool operator()(_l_transition<t_TyChar> const &_rrl,
                  typename _l_transition<t_TyChar>::_TyUnsignedChar const &_rucr) const
  {
    return _rrl.m_last < _rucr;
  }
};

template <class t_TyChar>
struct _l_an_mostbase
{
private:
  typedef _l_an_mostbase<t_TyChar> _TyThis;

public:
  typedef bool (_TyThis::*_TyPMFnAccept)();
};

template <class t_TyChar, bool t_fSupportLookahead>
struct _l_an_lookaheadbase;

template <class t_TyChar>
struct _l_an_lookaheadbase<t_TyChar, false>
    : public _l_an_mostbase<t_TyChar>
{
  typedef _l_state_proto<t_TyChar> _TyStateProto;
  typedef typename _l_char_type_map<t_TyChar>::_TyUnsigned _TyUnsignedChar;

protected:
  // Declare these so the code will compile - they may be optimized out.
  static _TyStateProto *m_pspLookaheadAccept;  // The lookahead accept state.
  static _TyUnsignedChar *m_pcLookaheadAccept; // The position in the buffer when lookahead accept encountered.
  static _TyStateProto *m_pspLookahead;        // The lookahead state.
};

template <class t_TyChar>
typename _l_an_lookaheadbase<t_TyChar, false>::_TyStateProto *
    _l_an_lookaheadbase<t_TyChar, false>::m_pspLookaheadAccept;
template <class t_TyChar>
typename _l_an_lookaheadbase<t_TyChar, false>::_TyUnsignedChar *
    _l_an_lookaheadbase<t_TyChar, false>::m_pcLookaheadAccept;
template <class t_TyChar>
typename _l_an_lookaheadbase<t_TyChar, false>::_TyStateProto *
    _l_an_lookaheadbase<t_TyChar, false>::m_pspLookahead;

template <class t_TyChar>
struct _l_an_lookaheadbase<t_TyChar, true>
    : public _l_an_mostbase<t_TyChar>
{
  typedef _l_state_proto<t_TyChar> _TyStateProto;
  typedef typename _l_char_type_map<t_TyChar>::_TyUnsigned _TyUnsignedChar;

  _TyStateProto *m_pspLookaheadAccept;  // The lookahead accept state.
  _TyUnsignedChar *m_pcLookaheadAccept; // The position in the buffer when lookahead accept encountered.
  _TyStateProto *m_pspLookahead;        // The lookahead state.

  _l_an_lookaheadbase()
      : m_pspLookaheadAccept(0),
        m_pspLookahead(0)
  {
  }
};

template <class t_TyChar, bool t_fSupportLookahead,
          bool t_fSupportTriggers, bool t_fTrace>
struct _l_analyzer : public _l_an_lookaheadbase<t_TyChar, t_fSupportLookahead>
{
private:
  typedef _l_an_lookaheadbase<t_TyChar, t_fSupportLookahead> _TyBase;
  typedef _l_analyzer _TyThis;

protected:
  using _TyBase::m_pcLookaheadAccept;
  using _TyBase::m_pspLookahead;
  using _TyBase::m_pspLookaheadAccept;

public:
  typedef typename _TyBase::_TyStateProto _TyStateProto;

private:
  _TyStateProto *m_pspLastAccept; // The last encountered accept state;
public:
  typedef typename _TyBase::_TyUnsignedChar _TyUnsignedChar;
  typedef typename _TyBase::_TyPMFnAccept _TyPMFnAccept;

  typedef t_TyChar _TyChar;
  typedef _l_transition<t_TyChar> _TyTransition;
  typedef _l_compare_input_with_range<t_TyChar> _TyCompSearch;

  static constexpr bool s_kfSupportLookahead = t_fSupportLookahead;
  static constexpr bool s_kfSupportTriggers = t_fSupportTriggers;
  static constexpr bool s_kfTrace = t_fTrace;

  _TyStateProto *m_pspStart;         // Start state.
  _TyStateProto *m_pspCur;           // Current state.
  _TyUnsignedChar *m_pcLastAccept{}; // The position in the buffer when last accept encountered.
  _TyCompSearch m_compSearch;        // search object.
  _TyUnsignedChar *m_pcCur{};        // Current search string.

  _l_analyzer() = delete;
  _l_analyzer(const _l_analyzer &) = delete;
  _l_analyzer &operator=(_l_analyzer const &) = delete;

  _l_analyzer(_TyStateProto *_pspStart)
      : m_pspStart(_pspStart),
        m_pspLastAccept(0)
  {
  }

#define LXOBJ_DOTRACE(MESG...) _DoTrace( __FILE__, __LINE__, __PRETTY_FUNCTION__, MESG)
  void _DoTrace(const char *_szFile, unsigned int _nLine, const char *_szFunction, const char *_szMesg, ...)
  {
    std::string strCur;
    if ( !!m_pcCur && !!*m_pcCur )
    {
      if ((*m_pcCur > 32) && (*m_pcCur < 127))
        (void)FPrintfStdStrNoThrow(strCur, "%c (%lu)", (char)*m_pcCur, uint64_t(*m_pcCur) );
      else
        (void)FPrintfStdStrNoThrow(strCur, "%lu", uint64_t(*m_pcCur));
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

  // get the next accepted state. Return false if no accepted state found.
  // Update <_rpc> appropriately.
  bool get(t_TyChar *&_rpc)
  {
    m_pcCur = reinterpret_cast<_TyUnsignedChar *>(_rpc);

    do
    {
      m_pspCur = m_pspStart;
      LXOBJ_DOTRACE("At start.");
      do
      {
        if (m_pspCur->m_flAccept)
        {
          if (t_fSupportLookahead)
          {
            // Then need to check the type of accept state:
            switch (m_pspCur->m_flAccept)
            {
            case kucAccept:
            {
              m_pspLastAccept = m_pspCur;
              m_pcLastAccept = m_pcCur;
            }
            break;
            case kucLookahead:
            {
              // The lookahead state ( the last state in the lookahead NFA ).
              // There are a couple of possibilities:
              // 1) We haven't seen the associated kucLookaheadAccept state - in which case
              //		ignore this state.
              // 2) We have seen the associated kucLookaheadAccept state:
              //		a) and we have already seen this lookahead state <or>
              //		b) we haven't yet seen this lookahead state.
              if (m_pspLookaheadAccept)
              {
                // Then might not be the associated accept state:
                _TyActionIdent aiLA = m_pspLookaheadAccept->AIGetLookahead();
                _TyActionIdent aiCur = m_pspCur->AIGetLookahead();
                if (((aiLA < 0) &&
                     (*(m_pspLookaheadAccept->PBeginValidLookahead() +
                        aiCur / (CHAR_BIT * sizeof(_TyLookaheadVector))) &
                      (1 << aiCur % (CHAR_BIT * sizeof(_TyLookaheadVector))))) ||
                    (aiLA == aiCur))
                {
                  // REVIEW: May be no need for {m_pspLookahead}.
                  m_pspLastAccept = m_pspLookahead = m_pspCur;
                  m_pcLastAccept = m_pcLookaheadAccept;
                }
                else
                {
                  // Dangerous set of lookahead patterns.
                  Assert(0); // When does this happen ?
                             // If and when this does happen then likely due to optimization.
                             // Then the lookahead state for another lookahead accept state - ignore
                             //	 it completely.
                }
              }
              // We haven't seen the lookahead accept - ignore.
            }
            break;
            case kucLookaheadAccept:
            {
              m_pspLookaheadAccept = m_pspCur;
              m_pcLookaheadAccept = m_pcCur;
            }
            break;
            case kucLookaheadAcceptAndAccept:
            {
              // We have an ambiguous state that is both an accepting state and a
              //	lookahead accepting state.
              m_pspLastAccept = m_pspLookaheadAccept = m_pspCur;
              m_pcLastAccept = m_pcLookaheadAccept = m_pcCur;
            }
            break;
            case kucLookaheadAcceptAndLookahead:
            {
              // We have an ambiguous state that is both a lookahead state and a
              //	lookahead accepting state.
              if (m_pspLookaheadAccept)
              {
                // Then might not be the associated accept state:
                _TyActionIdent aiLA = m_pspLookaheadAccept->AIGetLookahead();
                _TyActionIdent aiCur = m_pspCur->AIGetLookahead();
                if (((aiLA < 0) &&
                     (*(m_pspLookaheadAccept->PBeginValidLookahead() +
                        aiCur / (CHAR_BIT * sizeof(_TyLookaheadVector))) &
                      (1 << aiCur % (CHAR_BIT * sizeof(_TyLookaheadVector))))) ||
                    (aiLA == aiCur))
                {
                  // REVIEW: May be no need for {m_pspLookahead}.
                  m_pspLastAccept = m_pspLookahead = m_pspCur;
                  m_pcLastAccept = m_pcLookaheadAccept;
                }
                else
                {
                  Assert(0); // When does this happen ?
                             // If and when this does happen then likely due to optimization.
                             // Then the lookahead state for another lookahead accept state - ignore
                             //	 it completely.
                }
              }
              // We haven't seen the lookahead accept - ignore the lookahead.

              // Record the lookahead accept:
              m_pspLookaheadAccept = m_pspCur;
              m_pcLookaheadAccept = m_pcCur;
            }
            break;
            }
          }
          else
          {
            LXOBJ_DOTRACE( "Accept found.");
            m_pspLastAccept = m_pspCur;
            m_pcLastAccept = m_pcCur;
          }
        }
      } while (_getnext());

      if (m_pspLastAccept)
      {
        m_pcCur = m_pcLastAccept;

        _TyPMFnAccept pmfnAccept;
        if (!!(pmfnAccept = m_pspLastAccept->PMFnGetAction()))
        {
          m_pspLastAccept = 0;
          if ((this->*pmfnAccept)())
          {
            _rpc = reinterpret_cast<t_TyChar *>(m_pcCur);
            return true;
          }
        }
        m_pspLastAccept = 0;
        if (t_fSupportLookahead)
        {
          m_pspLookaheadAccept = 0;
        }
        // else continue
      }
      else
      {
        // No accepting state found - don't modify string.
        return false;
      }
    } while (*m_pcCur);

    return false; // at eof - caller must check.
  }

protected:
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

  bool _getnext()
  {
    switch (m_pspCur->m_nt)
    {
    case 0:
      if (t_fSupportTriggers)
      {
        // Then must check for a trigger transition:
        if (m_pspCur->m_nTriggers)
        {
          // Then have one - execute the triggers:
          Assert( !!m_pspCur->m_pspTrigger );
          _execute_triggers();
          LXOBJ_DOTRACE( "Moved to trigger." );
          return true; // advanced the state.
        }
      }
      return false;
      break;

    case 1:
    {
      if (*m_pcCur <= m_pspCur->m_rgt[0].m_last &&
          *m_pcCur >= m_pspCur->m_rgt[0].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[0].m_psp;
        ++m_pcCur;
        LXOBJ_DOTRACE( "Moved to state." );
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            LXOBJ_DOTRACE( "Moved to trigger." );
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    break;

    case 2:
    {
      if (*m_pcCur <= m_pspCur->m_rgt[0].m_last &&
          *m_pcCur >= m_pspCur->m_rgt[0].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[0].m_psp;
        ++m_pcCur;
        LXOBJ_DOTRACE( "Moved to state." );
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[1].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[1].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[1].m_psp;
        ++m_pcCur;
        LXOBJ_DOTRACE( "Moved to state." );
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            LXOBJ_DOTRACE( "Moved to trigger." );
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    break;

    case 3:
    {
      if (*m_pcCur <= m_pspCur->m_rgt[0].m_last &&
          *m_pcCur >= m_pspCur->m_rgt[0].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[0].m_psp;
        ++m_pcCur;
        LXOBJ_DOTRACE( "Moved to state." );
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[1].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[1].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[1].m_psp;
        ++m_pcCur;
        LXOBJ_DOTRACE( "Moved to state." );
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[2].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[2].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[2].m_psp;
        ++m_pcCur;
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    break;

    case 4:
    {
      if (*m_pcCur <= m_pspCur->m_rgt[0].m_last &&
          *m_pcCur >= m_pspCur->m_rgt[0].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[0].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[1].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[1].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[1].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[2].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[2].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[2].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[3].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[3].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[3].m_psp;
        ++m_pcCur;
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    break;

    case 5:
    {
      if (*m_pcCur <= m_pspCur->m_rgt[0].m_last &&
          *m_pcCur >= m_pspCur->m_rgt[0].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[0].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[1].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[1].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[1].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[2].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[2].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[2].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[3].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[3].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[3].m_psp;
        ++m_pcCur;
        return true;
      }
      else if (*m_pcCur <= m_pspCur->m_rgt[4].m_last &&
               *m_pcCur >= m_pspCur->m_rgt[4].m_first)
      {
        m_pspCur = m_pspCur->m_rgt[4].m_psp;
        ++m_pcCur;
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    break;

    default:
    {
      // Enough elements to warrant a binary search:
      _TyTransition *ptLwr =
          lower_bound(m_pspCur->m_rgt,
                      m_pspCur->m_rgt + m_pspCur->m_nt,
                      *m_pcCur, m_compSearch);
      if ((*m_pcCur >= ptLwr->m_first) &&
          (*m_pcCur <= ptLwr->m_last))
      {
        m_pspCur = ptLwr->m_psp;
        ++m_pcCur;
        return true;
      }
      else
      {
        if (t_fSupportTriggers)
        {
          // Then must check for a trigger transition:
          if (m_pspCur->m_nTriggers)
          {
            // Then have one - execute the triggers:
            _execute_triggers();
            return true; // advanced the state.
          }
        }
        return false;
      }
    }
    }
  }
};

// When the rules indicate unique accepting states ( i.e. with no out transitions )
//	and no lookaheads then we can use this faster analyzer base:

template <class t_TyChar, bool t_fSupportTriggers>
struct _l_analyzer_unique_onematch
    : public _l_analyzer<t_TyChar, false, t_fSupportTriggers>
{
private:
  typedef _l_analyzer<t_TyChar, false, t_fSupportTriggers> _TyBase;
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

  int m_iToken;

  _l_token_analyzer()
      : t_TyBaseAnalyzer()
  {
  }

  void SetToken(int _iToken)
  {
    m_iToken = _iToken;
  }
};

__LEXOBJ_END_NAMESPACE

#endif //__L_LXOBJ_H

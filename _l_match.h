#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_match.h
// Match strings to simple state machines.
// dbien
// 19FEB2021

// This is to be used for output validation.
// As such we don't need an object because there are no triggers and no actions that will be executed( even if there are actions in the state machine).
// We will follow triggers appropriately if t_kfFollowTriggers is true, otherwise triggers are not followed. Regardless the trigger action will not be called if present.
// We do not support lookahead for this.

#include "_l_state.h"

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyChar, bool t_kfFollowTriggers = false >
class _l_match
{
  typedef _l_match _TyThis;
public:
  typedef t_TyChar _TyChar;
  static constexpr bool s_kfFollowTriggers = t_kfFollowTriggers;
  typedef _l_state_proto< _TyChar > _TyStateProto;
  typedef _l_transition< _TyChar > _TyTransition;

  // Return the point to which we match the given _pspStart as the start state.
  // If !!_ppspLastAccept then we set that to the last accept that we encountered - note that this could be an anti-accepting state.
  // If the result is an anti-accepting state and _ppspLastAccept is not given then nullptr is returned, otherwise the pointer
  //  returned points to the first entrance into any anti-accepting state.
  static const _TyChar * PszMatch( const _TyStateProto * _pspStart, const _TyChar * _pszStart, size_t _nchLen, const _TyStateProto ** _ppspLastAccept = nullptr )
    requires ( !s_kfFollowTriggers ) // Don't support triggers in this method.
  {
    Assert( !!_pspStart );
    const _TyChar * pchCur = _pszStart;
    const _TyChar * const pchEnd = pchCur + _nchLen;
    const _TyStateProto * pspLastAccept = _pspStart->m_flAccept ? _pspStart : nullptr;
    const _TyChar * pchLastAccept = pspLastAccept ? pchCur : nullptr;
    for( const _TyStateProto * pspCur = _pspStart; pchEnd != pchCur; ++pchCur ) 
    {
      // I'm just gonna do a simple for loop here and then expect the optimizer to so it's work instead of trying to optimize - or even do a
      //  simple binary search.
      const _TyTransition * ptrCur = pspCur->m_rgt;
      const _TyTransition * const ptrEnd = pspCur->m_rgt + pspCur->m_nt;
      _TyChar chCur = *pchCur;
      for ( ; ( ptrEnd != ptrCur ) && !( ( chCur <= ptrCur->m_last ) && ( chCur >= ptrCur->m_first ) ); ++ptrCur )
        ;
      if ( ptrEnd == ptrCur )
        break; // no transition on this character from this state and we aren't following triggers.
      pspCur = ptrCur->m_psp;
      if ( pspCur->m_flAccept )
      {
        // We want the first encountered anti-accepting state and the last encountered accepting state.
        if ( !pspCur->FIsAntiAcceptingState() || !pspLastAccept || !pspLastAccept->FIsAntiAcceptingState() )
        {
          pspLastAccept = pspCur;
          pchLastAccept = pchCur + 1; // We just ate a character.
        }
      }
    }
    if ( !!_ppspLastAccept )
      *_ppspLastAccept = pspLastAccept; // This may be null.
    if ( !pspLastAccept || pspLastAccept->FIsAntiAcceptingState() )
      return _ppspLastAccept ? pchLastAccept : nullptr; // return as far as we got until we failed unless this is the only way the caller knows about failure.
    return pchLastAccept; // accepted 
  }
  // Trigger-following version: We support any trigger configuration (i.e. compressed or not) in this version, but we will not execute any triggers.
  // This is for reusing productions that might also be used for input and thus may have triggers.
  static const _TyChar * PszMatch( const _TyStateProto * _pspStart, const _TyChar * _pszStart, size_t _nchLen, const _TyStateProto ** _ppspLastAccept = nullptr )
    requires ( s_kfFollowTriggers )
  {
    Assert( !!_pspStart );
    const _TyChar * pchCur = _pszStart;
    const _TyChar * const pchEnd = pchCur + _nchLen;
    const _TyStateProto * pspLastAccept = _pspStart->m_flAccept ? _pspStart : nullptr;
    const _TyChar * pchLastAccept = pspLastAccept ? pchCur : nullptr;
    for( const _TyStateProto * pspCur = _pspStart; pchEnd != pchCur;  ) 
    {
      // I'm just gonna do a simple for loop here and then expect the optimizer to so it's work instead of trying to optimize - or even do a
      //  simple binary search.
      const _TyTransition * ptrCur = pspCur->m_rgt;
      const _TyTransition * const ptrEnd = pspCur->m_rgt + pspCur->m_nt;
      _TyChar chCur = *pchCur;
      for ( ; ( ptrEnd != ptrCur ) && !( ( chCur <= ptrCur->m_last ) && ( chCur >= ptrCur->m_first ) ); ++ptrCur )
        ;
      if ( ptrEnd == ptrCur )
      {
        if ( pspCur->m_pspTrigger ) // Follow a trigger transition if there is one - in this case no character is eaten.
          pspCur = pspCur->m_pspTrigger;
        else
          break; // no transition or trigger.
      }
      else
      {
        ++pchCur; // we ate a char.
        pspCur = ptrCur->m_psp;
      }
      if ( pspCur->m_flAccept )
      {
        pspLastAccept = pspCur;
        pchLastAccept = pchCur;
      }
    }
    if ( !!_ppspLastAccept )
      *_ppspLastAccept = pspLastAccept; // This may be null.
    if ( !pspLastAccept || pspLastAccept->FIsAntiAcceptingState() )
      return _ppspLastAccept ? pchLastAccept : nullptr; // return as far as we got until we failed unless this is the only way the caller knows about failure.
    return pchLastAccept; // accepted 
  }
};

__LEXOBJ_END_NAMESPACE

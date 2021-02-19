#pragma once

// _l_match.h
// Match strings to simple state machines.
// dbien
// 19FEB2021

// This is to be used for output validation.
// As such we don't need an object because there are no triggers and no actions that will be executed( even if there are actions in the state machine).
// We will follow triggers appropriately if t_kfFollowTriggers is true, otherwise triggers are not followed. Regardless the trigger action will not be called if present.
// We do not support lookahead for this.

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

    Assert( !pspLastAccept || !pspLastAccept->FIsAntiAcceptingState() ); // If we always always fail then what good is even trying to match unless this is just an "always fail" rule.

    // We stop right away when we encounter an anti-accepting state. Since we aren't following triggers then each loop increments the string position.
    for( const _TyStateProto * pspCur = _pspStart; ( pchEnd != pchCur ) && !pspCur->FIsAntiAcceptingState(); ++pchCur ) 
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
        pspLastAccept = pspCur;
    }

    if ( !!_ppspLastAccept )
      *_ppspLastAccept = pspLastAccept; // This may be null.
    if ( !pspLastAccept || pspLastAccept->FIsAntiAcceptingState() )
      return _ppspLastAccept ? pchCur : nullptr; // return as far as we got until we failed unless this is the only way the caller knows about failure.
    return pchLastAccept; // accepted 
  }

};
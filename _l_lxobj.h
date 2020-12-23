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
#include <unordered_set>
#include <typeinfo>
#include "bienutil.h"
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

__LEXOBJ_BEGIN_NAMESPACE

template <class t_TyChar>
struct _l_compare_input_with_range
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
  static vtyDataPosition m_posLookaheadAccept; // The position in the buffer when lookahead accept encountered.
  static _TyStateProto *m_pspLookahead;        // The lookahead state.
};

template <class t_TyChar>
typename _l_an_lookaheadbase<t_TyChar, false>::_TyStateProto *
    _l_an_lookaheadbase<t_TyChar, false>::m_pspLookaheadAccept;
template <class t_TyChar>
vtyDataPosition _l_an_lookaheadbase<t_TyChar, false>::m_posLookaheadAccept{vkdpNullDataPosition};
template <class t_TyChar>
typename _l_an_lookaheadbase<t_TyChar, false>::_TyStateProto *
    _l_an_lookaheadbase<t_TyChar, false>::m_pspLookahead;

template <class t_TyChar>
struct _l_an_lookaheadbase<t_TyChar, true>
    : public _l_an_mostbase<t_TyChar>
{
  typedef _l_state_proto<t_TyChar> _TyStateProto;
  typedef typename _l_char_type_map<t_TyChar>::_TyUnsigned _TyUnsignedChar;

  _TyStateProto *m_pspLookaheadAccept{nullptr}; // The lookahead accept state.
  vtyDataPosition m_posLookaheadAccept{vkdpNullDataPosition}; // The position in the buffer when lookahead accept encountered.
  _TyStateProto *m_pspLookahead{nullptr}; // The lookahead state.

  _l_an_lookaheadbase()
  {
  }
};

template <class t_TyTransport, class t_TyUserObj, bool t_fSupportLookahead,
          bool t_fSupportTriggers, bool t_fTrace>
struct _l_analyzer : public _l_an_lookaheadbase< typename t_TyTransport::_TyChar, t_fSupportLookahead>
{
private:
  typedef _l_an_lookaheadbase< typename t_TyTransport::_TyChar, t_fSupportLookahead> _TyBase;
  typedef _l_analyzer _TyThis;
protected:
  using _TyBase::m_posLookaheadAccept;
  using _TyBase::m_pspLookahead;
  using _TyBase::m_pspLookaheadAccept;
public:
  typedef typename _TyBase::_TyStateProto _TyStateProto;
  typedef typename t_TyTransport::_TyChar _TyChar;
  typedef t_TyTransport _TyTransport;
  typedef t_TyUserObj _TyUserObj;
  typedef _l_stream< _TyTransport, _TyUserObj > _TyStream;
  typedef typename _TyStream::_TyUserContext _TyUserContext;
  typedef _l_token< _TyUserContext > _TyToken;

  using typename _TyBase::_TyUnsignedChar;
  using typename _TyBase::_TyPMFnAccept;
  using typename _TyBase::_TyAxnObjBase;

  typedef _l_transition<_TyChar> _TyTransition;
  typedef _l_compare_input_with_range<_TyChar> _TyCompSearch;

  static constexpr bool s_kfSupportLookahead = t_fSupportLookahead;
  static constexpr bool s_kfSupportTriggers = t_fSupportTriggers;
  static constexpr bool s_kfTrace = t_fTrace;

  _TyStream m_stream; // the stream within which is the transport object and user context, etc.

private: // Not sure why I made this private but it must have been very important!!! (j/k) so I am going to leave it.
  _TyStateProto *m_pspLastAccept; // The last encountered accept state;
public:
  _TyStateProto *m_pspCur{nullptr}; // Current state.
  vtyDataPosition m_posLastAccept{vkdpNullDataPosition};
  _TyUnsignedChar *m_pcLastAccept{}; // The position in the buffer when last accept encountered.
  // The start of the current token is stored in the transport.
  _TyUnsignedChar m_ucCur; // The current character obtained from the transport.
  _TyCompSearch m_compSearch;        // search object.

  _l_analyzer() = delete;
  _l_analyzer(const _l_analyzer &) = delete;
  _l_analyzer &operator=(_l_analyzer const &) = delete;

  _l_analyzer(_TyStateProto *_pspStart)
      : m_pspLastAccept(0)
  {
  }

  // Construct the transport object appropriately. This is how to open a file, etc.
  template < class... t_TysArgs >
  void emplaceTransport( t_TysArgs&&... _args )
  {
    GetStream().emplaceTransport( std::forward< t_TysArgs >( _args )... );
  }

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
  size_t GetCurrentPosition() const
  {
    return GetStream().PosCurrent();
  }
#define LXOBJ_DOTRACE(MESG...) _DoTrace( __FILE__, __LINE__, __PRETTY_FUNCTION__, MESG)
  void _DoTrace(const char *_szFile, unsigned int _nLine, const char *_szFunction, const char *_szMesg, ...)
  {
    std::string strCur;
    if ( !!m_ucCur )
    {
      if ((m_ucCur > 32) && (m_ucCur < 127))
        (void)FPrintfStdStrNoThrow(strCur, "%c (%lu)", (char)m_ucCur, uint64_t(m_ucCur) );
      else
        (void)FPrintfStdStrNoThrow(strCur, "%lu", uint64_t(m_ucCur));
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
      if (t_fSupportLookahead)
      {
        // Then need to check the type of accept state:
        switch (m_pspCur->m_flAccept)
        {
        case kucAccept:
        {
          m_pspLastAccept = m_pspCur;
          m_posLastAccept = GetStream().PosCurrent();
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
            vtyActionIdent aiLA = m_pspLookaheadAccept->AIGetLookahead();
            vtyActionIdent aiCur = m_pspCur->AIGetLookahead();
            if (((aiLA < 0) &&
                  (*(m_pspLookaheadAccept->PBeginValidLookahead() +
                    aiCur / (CHAR_BIT * sizeof(vtyLookaheadVector))) &
                  (1 << aiCur % (CHAR_BIT * sizeof(vtyLookaheadVector))))) ||
                (aiLA == aiCur))
            {
              // REVIEW: May be no need for {m_pspLookahead}.
              m_pspLastAccept = m_pspLookahead = m_pspCur;
              m_posLastAccept = m_posLookaheadAccept;
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
          m_posLookaheadAccept = GetStream().PosCurrent();
        }
        break;
        case kucLookaheadAcceptAndAccept:
        {
          // We have an ambiguous state that is both an accepting state and a
          //	lookahead accepting state.
          m_pspLastAccept = m_pspLookaheadAccept = m_pspCur;
          m_posLastAccept = m_posLookaheadAccept = GetStream().PosCurrent();
        }
        break;
        case kucLookaheadAcceptAndLookahead:
        {
          // We have an ambiguous state that is both a lookahead state and a
          //	lookahead accepting state.
          if (m_pspLookaheadAccept)
          {
            // Then might not be the associated accept state:
            vtyActionIdent aiLA = m_pspLookaheadAccept->AIGetLookahead();
            vtyActionIdent aiCur = m_pspCur->AIGetLookahead();
            if (((aiLA < 0) &&
                  (*(m_pspLookaheadAccept->PBeginValidLookahead() +
                    aiCur / (CHAR_BIT * sizeof(vtyLookaheadVector))) &
                  (1 << aiCur % (CHAR_BIT * sizeof(vtyLookaheadVector))))) ||
                (aiLA == aiCur))
            {
              // REVIEW: May be no need for {m_pspLookahead}.
              m_pspLastAccept = m_pspLookahead = m_pspCur;
              m_posLastAccept = m_posLookaheadAccept;
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
          m_posLookaheadAccept = GetStream().PosCurrent();
        }
        break;
        }
      }
      else
      {
        LXOBJ_DOTRACE( "Accept found.");
        m_pspLastAccept = m_pspCur;
        m_posLastAccept = GetStream().PosCurrent();
      }
    }
  }

  // Just return a single token. Return false if you didn't get one.
  bool FGetToken( _TyStateProto *_pspStart, unique_ptr< _TyToken > & _rpuToken )
  {
    Assert( GetStream().FAtTokenStart() ); // We shouldn't be mid-token.
    do
    {
      m_pspCur = _pspStart; // Start at the beginning again...
      LXOBJ_DOTRACE("At start.");
      do
      {
        _CheckAcceptState();
      } 
      while ( _getnext() );

      if ( m_pspLastAccept )
      {
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
            unique_ptr< _TyToken > upToken; // We could use a shared_ptr but this seems sufficient at least for now.
            // Delegate to the stream to obtain the token as it needs to get the context from the transport.
            _TyAxnObjBase * paobCurToken = PGetToken();
            SetToken(nullptr);
            GetStream().GetPToken( paobCurToken, m_posLastAccept, upToken );
            if ( !_callback( upToken ) )
              return true; // The caller is done with getting tokens for now.
            // else continue to get tokens.
          }
          else
          {
            Assert( !PGetToken() ); // If the accept method rejects the token then it shouldn't set one.
            // We had hit a dead end. To continue from here we must return to the start state.
            // If the token action didn't accept then it should have cleared all token data:
            Assert( FIsClearOfTokenData() );
            GetStream().DiscardData( m_posLastAccept ); // Skip everything that we found... kinda harsh...also very poorly defined.
            // We could keep a stack of previously found accepting states and move to them but that's kinda ill defined and costs allocation space. Something to think about.
          }
          m_posLastAccept = vkdpNullDataPosition; // sanity.
        }
        if (t_fSupportLookahead)
        {
          m_pspLookaheadAccept = 0;
        }
      }
      else
      {
        // No accepting state found.
        return false;
      }
    } while (m_ucCur);
  }

  // Keep getting tokens until we hit eof or the callback method returns false.
  // If we process things as much as possible - i.e. get tokens until we are supposed to, then we return true and _pspStateFailing is set to nullptr.
  // If we encounter a state where we cannot move forward on input then we return false. The current state is then still set to the state from which we were unable to continue.
  template < class t_tyCallback >
  bool FGetTokens( _TyStateProto * _pspStart, t_tyCallback _callback )
  {
    Assert( GetStream().FAtTokenStart() ); // We shouldn't be mid-token.
    do
    {
      m_pspCur = _pspStart; // Start at the beginning again...
      LXOBJ_DOTRACE("At start.");
      do
      {
        _CheckAcceptState();
      } 
      while ( _getnext() );

      if ( m_pspLastAccept )
      {
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
            unique_ptr< _TyToken > upToken; // We could use a shared_ptr but this seems sufficient at least for now.
            _TyAxnObjBase * paobCurToken = PGetToken();
            SetToken(nullptr);
            GetStream().GetPToken( paobCurToken, m_posLastAccept, upToken );
            if ( !_callback( upToken ) )
              return true; // The caller is done with getting tokens for now.
            // else continue to get tokens.
          }
          else
          {
            Assert( !PGetToken() ); // If the accept method rejects the token then it shouldn't set one.
            // We had hit a dead end. To continue from here we must return to the start state.
            // If the token action didn't accept then it should have cleared all token data:
            Assert( FIsClearOfTokenData() );
            GetStream().DiscardData( m_posLastAccept ); // Skip everything that we found... kinda harsh..
            // We could keep a stack of previously found accepting states and move to them but that's kinda ill defined and costs allocation space. Something to think about.
          }
          m_posLastAccept = vkdpNullDataPosition; // sanity.
        }
        if (t_fSupportLookahead)
        {
          m_pspLookaheadAccept = 0;
        }
      }
      else
      {
        // No accepting state found.
        return false;
      }
    } while (m_ucCur);
  }

protected:
  void _NextChar()
  {
    m_ucCur = 0;
    (void))m_stream.FGetChar( m_ucCur );
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
        // Enough elements to warrant a binary search:
        _TyTransition *ptLwr =
            lower_bound(m_pspCur->m_rgt,
                        m_pspCur->m_rgt + m_pspCur->m_nt,
                        m_ucCur, m_compSearch);
        if ((m_ucCur >= ptLwr->m_first) &&
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

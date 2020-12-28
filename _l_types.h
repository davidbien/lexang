#pragma once

// _l_types.h
// Types and templates predeclarations.

#include "_l_ns.h"
#include <limits>

// Debugging, etc.
#define LXOBJ_STATENUMBERS

__REGEXP_BEGIN_NAMESPACE

typedef int32_t	vtyActionIdent;
typedef int32_t	vtyTokenIdent; // allow negative numbers to signify something, not sure yet.
typedef unsigned long vtyLookaheadVector;

typedef size_t vtyDataPosition;
static constexpr vtyDataPosition vkdpNullDataPosition = numeric_limits< vtyDataPosition >::max();
typedef uint32_t vtyDataType;
typedef vtyTokenIdent vtyDataTriggerId;
vtyDataTriggerId vktidInvalidIdTrigger = numeric_limits< vtyDataTriggerId >::min();

template < class t_TyChar, bool t_fInLexGen >
struct _l_action_object_base;
template < class t_TyActionObj >
struct _l_action_token;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_action_print;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_noop;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_bool;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_position;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_position_end;
template < class t_TyChar, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_string;
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
class _l_trigger_string_typed_range;
template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_single;
template < vtyTokenIdent t_kiTrigger, bool t_fInLexGen, class... t_TysTriggers >
class _l_action_save_data_multiple;

__REGEXP_END_NAMESPACE

__LEXOBJ_BEGIN_NAMESPACE

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

// _l_state.h:
template <class t_TyChar>
struct _l_state_proto;

template <class t_TyChar>
struct _l_an_mostbase;

template <class t_TyTransport, class t_tyUserObj, bool t_fSupportLookahead, bool t_fSupportTriggers, bool t_fTrace = false>
struct _l_analyzer;

template <class t_TyChar, int t_iTransitions,
          bool t_fAccept, bool t_fLookahead,
          int t_iLookaheadVectorEls,
          int t_iTriggers>
struct _l_state;

template <class t_TyChar>
struct _l_transition;

// _l_value.h:
template< class t_TyChar, size_t s_knValsSegSize = 32 >
class _l_value;

// _l_token.h:
template < class t_TyUserContext >
class _l_token;

// _l_stream.h:
template < class t_TyChar >
class _l_default_user_obj;
template < class t_TyTransportCtxt, class t_tyUserObj >
class _l_user_context;
template < class t_TyChar >
class _l_transport_base;
template < class t_TyChar >
class _l_transport_backed_ctxt;
template < class t_TyChar >
class _l_transport_fd;
template < class t_TyChar >
class _l_transport_fixedmem_ctxt;
template < class t_TyChar >
class _l_transport_fixedmem;
template < class t_TyChar >
class _l_transport_mapped;
template < class ... t_TysTransportContexts >
class _l_transport_var_ctxt;
template < class ... t_TysTransports >
class _l_transport_var;
template < class t_TyTransport, class t_tyUserObj >
class _l_stream;

__LEXOBJ_END_NAMESPACE

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
typedef uint64_t vtyLookaheadVector;

typedef size_t vtyDataPosition;
static constexpr vtyDataPosition vkdpNullDataPosition = (numeric_limits< vtyDataPosition >::max)();
typedef uint32_t vtyDataType;
typedef vtyTokenIdent vtyDataTriggerId;
static constexpr vtyDataTriggerId vktidInvalidIdTrigger = (numeric_limits< vtyDataTriggerId >::min)();
static constexpr vtyTokenIdent vktidInvalidIdToken = vktidInvalidIdTrigger;

// _l_axion.h:
template < class t_TyChar, bool t_fInLexGen >
struct _l_action_object_base;
template < class t_TyTraits, bool t_fInLexGen >
struct _l_action_object_value_base;
template < class t_TyActionObj >
struct _l_action_token;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_action_print;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_noop;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_bool;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, bool t_fInLexGen = true >
struct _l_trigger_position;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_position_end;
template < class t_TyTraits, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin, bool t_fInLexGen = true >
struct _l_trigger_string;
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin >
class _l_trigger_string_typed_range;
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger >
class _l_trigger_string_typed_beginpoint;
template < vtyDataType t_kdtType, class t_TyActionStoreData, vtyTokenIdent t_kiTrigger, vtyTokenIdent t_kiTriggerBegin = vktidInvalidIdTrigger >
class _l_trigger_string_typed_endpoint;
template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
class _l_action_save_data_single;
template < vtyTokenIdent t_kiTrigger, class... t_TysTriggers >
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
// Anti-accepting state: If this state is reached there will be no out transition (this is enforced during generation).
// The pattern received is specifically anti-accepting and lexical analysis should stop.
const unsigned char kucAntiAccepting = 6;

// _l_state.h:
template <class t_TyChar>
struct _l_state_proto;

template <class t_TyChar>
struct _l_an_mostbase;

template <class t_TyTraits, bool t_fSupportLookahead, bool t_fSupportTriggers, bool t_fTrace = false>
struct _l_analyzer;

template <class t_TyChar, int t_iTransitions,
          bool t_fAccept, bool t_fLookahead,
          int t_iLookaheadVectorEls,
          int t_iTriggers>
struct _l_state;

template <class t_TyChar>
struct _l_transition;

// _l_data.h:
class _l_data_range;
class _l_data_typed_range;
template < size_t s_knbySegSize = 512 >
class _l_data;

// _l_buf.h:
template < class t_TyChar > class _l_fixed_buf;
template < class t_TyChar > class _l_backing_buf;

// _l_value.h:
template< class t_TyChar, class t_TyTpValueTraits, size_t t_knValsSegSize = 16 >
class _l_value;

// _l_token.h:
template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
class _l_token;

// _l_stream.h:
template < class t_TyChar >
class _l_default_user_obj;
template < class t_TyTransportCtxt, class t_TyUserObj, class t_TyTpValueTraits >
class _l_user_context;
template < class t_TyChar >
class _l_transport_base;
template < class t_TyChar >
class _l_transport_backed_ctxt;
template < class t_TyChar, class t_TyBoolSwitchEndian = false_type >
class _l_transport_file;
template < class t_TyChar >
class _l_transport_fixedmem_ctxt;
template < class t_TyChar, class t_TyBoolSwitchEndian = false_type >
class _l_transport_fixedmem;
template < class t_TyChar, class t_TyBoolSwitchEndian = false_type >
class _l_transport_mapped;
template < class t_TyVariant >
class _l_transport_var_ctxt;
template < class ... t_TysTransports >
class _l_transport_var;
template < class t_TyTraits >
class _l_stream;

// TFIsTransportVarCtxt:
template < class t_ty >
struct TFIsTransportVarCtxt
{
  static constexpr bool value = false;  
};
template < class ... t_tys >
struct TFIsTransportVarCtxt< _l_transport_var_ctxt< t_tys ... > >
{
  static constexpr bool value = true;
};
template < class t_TyTransportCtxt >
inline constexpr bool TFIsTransportVarCtxt_v = TFIsTransportVarCtxt< t_TyTransportCtxt >::value;

// TFIsTransportVar:
template < class t_ty >
struct TFIsTransportVar
{
  static constexpr bool value = false;  
};
template < class ... t_tys >
struct TFIsTransportVar< _l_transport_var< t_tys ... > >
{
  static constexpr bool value = true;  
};
template < class t_TyTransport >
inline constexpr bool TFIsTransportVar_v = TFIsTransportVar< t_TyTransport >::value;

__LEXOBJ_END_NAMESPACE

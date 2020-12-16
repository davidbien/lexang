#pragma once

// _l_types.h
// Types and templates predeclarations.

#include "_l_ns.h"
#include <limits>

__REGEXP_BEGIN_NAMESPACE

typedef int	vtyActionIdent;
typedef int	vtyTokenIdent;
typedef size_t vtyDataType;
typedef unsigned long vtyLookaheadVector;

typedef size_t vtyDataPosition;
static constexpr vtyDataPosition vtpNullDataPosition = numeric_limits< vtyDataPosition >::max();
typedef size_t vtyDataType;

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


// Transports:
template < class t_TyChar >
class _l_transport_base;

template < class t_TyChar >
class _l_transport_fd_ctxt;
template < class t_TyChar >
class _l_transport_fd;

template < class t_TyChar >
class _l_transport_fixedmem_ctxt;
template < class t_TyChar >
class _l_transport_fixedmem;

template < class t_TyChar >
class _l_transport_mapped_ctxt;

template < class t_TyChar >
class _l_transport_mapped;

template < class t_TyTransport >
class _l_stream;



__REGEXP_END_NAMESPACE

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

__REGEXP_END_NAMESPACE

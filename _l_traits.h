#pragma once

// _l_traits.h
// Traits for lexical analyzer generation and operation.
// dbien
// 09JAN2021

// This will allow injection of user defined types into the _l_value class ( as one benefit ).
// It centralizes the types for all lexang objects.

#include "_l_ns.h"
#include "_l_types.h"

__LEXOBJ_BEGIN_NAMESPACE

// _l_value_traits:
// This is used to add additional user-defined types to _l_value's variant object.
template < class ... t_TysUserValueTypes >
struct _l_value_traits
{
};

template < class t_TyTransport, class t_TyUserObj, class t_TyValueTraits = _l_value_traits<> >
struct _l_traits
{
  typedef t_TyTransport _TyTransport;
  typedef t_TyUserObj _TyUserObj;
  typedef typename t_TyUserObj::_TyChar _TyChar;
  typedef t_TyValueTraits _TyValueTraits;
  // The number of values per segment in the _l_value segmented array.
  static constexpr size_t s_knValsSegSize = 16;
};

__LEXOBJ_END_NAMESPACE
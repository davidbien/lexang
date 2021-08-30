#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_traits.h
// Traits for lexical analyzer generation and operation.
// dbien
// 09JAN2021

// This will allow injection of user defined types into the _l_value class ( as one benefit ).
// It centralizes the types for all lexang objects.

#include <tuple>
#include "_l_ns.h"
#include "_l_types.h"

__LEXOBJ_BEGIN_NAMESPACE

template < class t_TyTransport, class t_TyUserObj, class t_TyTpValueTraits = tuple<> >
struct _l_traits
{
  typedef t_TyTransport _TyTransport;
  typedef t_TyUserObj _TyUserObj;
  typedef unique_ptr< _TyUserObj > _TyPtrUserObj;
  typedef typename t_TyUserObj::_TyChar _TyChar;
  typedef typename _TyTransport::_TyTransportCtxt _TyTransportCtxt;
  typedef t_TyTpValueTraits _TyTpValueTraits;
  typedef _l_value< _TyChar, _TyTpValueTraits > _TyValue;
  typedef _l_token< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyToken;
  typedef _l_user_context< _TyTransportCtxt, _TyUserObj, _TyTpValueTraits > _TyUserContext;
  // The number of values per segment in the _l_value segmented array.
  static constexpr size_t s_knValsSegSize = 16;
};

__LEXOBJ_END_NAMESPACE
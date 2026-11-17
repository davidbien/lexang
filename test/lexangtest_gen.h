#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangtest_gen.h
// Common includes and utilities for the lexang tests which create NFAs and DFAs from regular expressions:
//	the generator of the test lexical analyzers ( lexangtestgen.cpp ) and the DFA unit tests ( lexangdfatest.cpp ).

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif //WIN32
#define DBG_NEW new

#include <memory>
typedef std::allocator< char >	_TyDefaultAllocator;

#include "_compat.h"
#include "_heapchk.h"

#define _L_REGEXP_DEFAULTCHAR	char8_t
#define __L_DEFAULT_ALLOCATOR _TyDefaultAllocator
#include "_l_inc.h"
#include <fstream>
#include <tuple>

#include "jsonstrm.h"
#include "jsonobjs.h"
#include "_compat.inl"
#include "syslogmgr.inl"
#include "lexangtest_types.h"

namespace ns_lexangtest
{
__REGEXP_USING_NAMESPACE

typedef TyTestChar _TyCTok;
typedef _TyDefaultAllocator  _TyAllocator;
typedef _regexp_final< _TyCTok, _TyAllocator > _TyFinal;
typedef _regexp_trigger< _TyCTok, _TyAllocator > _TyTrigger;
typedef _dfa< _TyCTok, _TyAllocator >	_TyDfa;
typedef _TyDfa::_TyDfaCtxt _TyDfaCtxt;
typedef _nfa< _TyCTok, _TyAllocator > _TyNfa;
typedef _TyNfa::_TyNfaCtxt _TyNfaCtxt;

// Shorthand for the regular expressions - as used by the XML parser's generator.
#define l(x)	literal<_TyCTok>(x)
#define ls(x)	litstr<_TyCTok>(x)
#define lr(x,y)	litrange<_TyCTok>(x,y)
#define lanyof(x) litanyinset<_TyCTok>(x)
#define lnot(x) litnotset<_TyCTok>(x)
#define t(...) _TyTrigger(__VA_ARGS__) // variadic since the trigger types contain commas.

// Create a DFA from the set of rules in _rFinal.
// _fOptimizeDfa: When true a dead state is created and the DFA is optimized, otherwise we produce the DFA as
//	created directly from the NFA.
// _fCompressTransitions: Compress the transitions as is done before generating a lexical analyzer.
inline void
GenDfa( _TyFinal const & _rFinal, _TyDfa & _rDfa, _TyDfaCtxt & _rDfaCtxt, bool _fOptimizeDfa = true,
				bool _fCompressTransitions = true, uint32_t _grfNFACreationOptions = 0 )
{
	{//B
		_TyNfa nfa( _grfNFACreationOptions, _rDfa.get_allocator() );
		_TyNfaCtxt nfaCtxt( nfa );
		_rFinal.ConstructNFA( nfaCtxt );
		_create_dfa< _TyNfa, _TyDfa >	cdfa( nfa, nfaCtxt, _rDfa, _rDfaCtxt, _fOptimizeDfa );
		cdfa.create();
	}//EB
	if ( _fOptimizeDfa )
	{
		_optimize_dfa< _TyDfa >	optdfa( _rDfa, _rDfaCtxt );
		(void)optdfa.optimize();
	}
  _rDfaCtxt.ProcessUnsatisfiableTranstitions();
	if ( _fCompressTransitions )
  	_rDfa.CompressTransitions();
}

} // namespace ns_lexangtest

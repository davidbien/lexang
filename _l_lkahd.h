#ifndef __L_LKAHD_H
#define __L_LKAHD_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_lkahd.h

// Support for lookahead rules ( trailing context ): "token / trailing".
//
// A lookahead rule matches when the input matches <token> followed by <trailing>, but only the portion matched by
//	<token> is the resultant token - the trailing context remains in the input. The rule competes with the other
//	rules on the length of the entire match, <token> followed by <trailing>, as with the unix lex.
//
// Having matched the entire rule we must find where the token ends. This is ambiguous when the end of <token> can
//	also match the beginning of <trailing> - for instance x* / xy against "xxxy": the token might be thought to end
//	after "xxx" ( the last point at which <token> matched ) but then the remaining "y" is not matched by <trailing>.
// We define the token to be the longest prefix of the entire match which is matched by <token> and for which the
//	remainder of the entire match is matched by <trailing>. For the above example the token is "xx".
//
// To find that point efficiently:
// 1) As the lexical analyzer moves forward through the input it records each position at which it is in a "lookahead
//		accept" state - a state in which the <token> of some lookahead rule(s) has been matched.
// 2) When the entire rule has matched it moves backward from the end of the match through a DFA which matches
//		<trailing> in reverse. Each accepting state of that DFA it enters is a position from which <trailing> matches
//		through to the end of the match. The first such position ( i.e. the longest token ) which was also recorded in
//		(1) for this rule is the end of the token.
//
// This module creates the DFA of (2): the "reverse trailing context DFA". We create it thusly:
// a) Create the optimized DFA for <trailing> on its own - any triggers within it are ignored. As this is the normal
//		creation of a DFA all of the regular expression operators may be used within <trailing>.
// b) Create the NFA which is the reverse of that DFA - see _nfa<>::CreateReverseOfDfa().
// c) Create the optimized DFA from that NFA.
// The lexical analyzer generator then generates the states of the DFA of (c) alongside those of the lexical analyzer
//	and each lookahead state refers to the start state of the DFA for its rule.

#include <memory>

__REGEXP_BEGIN_NAMESPACE

// The reverse trailing context DFA for a single lookahead rule.
template < class t_TyChar, class t_TyAllocator >
struct _dfa_lookahead_reverse
{
	typedef _dfa< t_TyChar, t_TyAllocator > _TyDfa;
	typedef _dfa_context< t_TyChar, t_TyAllocator > _TyDfaCtxt;

	_TyDfa m_dfa;
	_TyDfaCtxt m_dctxt;

	_dfa_lookahead_reverse( t_TyAllocator const & _rAlloc )
		: m_dfa( _rAlloc ),
			m_dctxt( m_dfa )
	{
	}
	_dfa_lookahead_reverse( _dfa_lookahead_reverse const & ) = delete;
	_dfa_lookahead_reverse & operator = ( _dfa_lookahead_reverse const & ) = delete;
};

// Create the reverse trailing context DFA for the trailing context _rreTrailing.
template < class t_TyChar, class t_TyAllocator >
std::shared_ptr< _dfa_lookahead_reverse< t_TyChar, t_TyAllocator > >
_l_create_lookahead_reverse( _regexp_base< t_TyChar > const & _rreTrailing, t_TyAllocator const & _rAlloc )
{
	typedef _dfa_lookahead_reverse< t_TyChar, t_TyAllocator > _TyLookaheadReverse;
	typedef typename _TyLookaheadReverse::_TyDfa _TyDfa;
	typedef typename _TyLookaheadReverse::_TyDfaCtxt _TyDfaCtxt;
	typedef _nfa< t_TyChar, t_TyAllocator > _TyNfa;
	typedef typename _TyNfa::_TyNfaCtxt _TyNfaCtxt;

	// (a) The optimized DFA for the trailing context. We leave the transitions uncompressed.
	_TyDfa dfaTrailing( _rAlloc );
	_TyDfaCtxt dctxtTrailing( dfaTrailing );
	{//B
		_TyNfa nfaTrailing( ( 1 << encoIgnoreTriggers ), _rAlloc );
		_TyNfaCtxt nctxtTrailing( nfaTrailing );
		// We pass a level such that a lookahead within the trailing context is just a "follows".
		_rreTrailing.ConstructNFA( nctxtTrailing, 2 );
		_create_dfa< _TyNfa, _TyDfa > cdfa( nfaTrailing, nctxtTrailing, dfaTrailing, dctxtTrailing, true );
		cdfa.create();
	}//EB
	{//B
		_optimize_dfa< _TyDfa > optdfa( dfaTrailing, dctxtTrailing );
		(void)optdfa.optimize();
	}//EB
	dctxtTrailing.ProcessUnsatisfiableTranstitions();

	// (b),(c) The optimized DFA for the reverse of the trailing context.
	std::shared_ptr< _TyLookaheadReverse > spReverse = std::make_shared< _TyLookaheadReverse >( _rAlloc );
	{//B
		_TyNfa nfaReverse( 0, _rAlloc );
		_TyNfaCtxt nctxtReverse( nfaReverse );
		nfaReverse.CreateReverseOfDfa( nctxtReverse, dfaTrailing, dctxtTrailing );
		_create_dfa< _TyNfa, _TyDfa > cdfa( nfaReverse, nctxtReverse, spReverse->m_dfa, spReverse->m_dctxt, true );
		cdfa.create();
	}//EB
	{//B
		_optimize_dfa< _TyDfa > optdfa( spReverse->m_dfa, spReverse->m_dctxt );
		(void)optdfa.optimize();
	}//EB
	spReverse->m_dctxt.ProcessUnsatisfiableTranstitions();
	spReverse->m_dfa.CompressTransitions();
	return spReverse;
}

__REGEXP_END_NAMESPACE

#endif //__L_LKAHD_H

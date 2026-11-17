#ifndef __L_DFATAG_H
#define __L_DFATAG_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_dfatag.h
// Tagged DFAs: the creation of a DFA in which the position of every trigger is resolved correctly whatever the rules.
//
// The problem:
// _create_dfa<> treats a trigger as a transition between states on a pseudo-character and the lexical analyzer takes
//	such a transition only when no transition on the current character exists. This is fast and it needs no state
//	other than the current state but it decides where a trigger is crossed without knowing what follows. For instance
//	for the rule a* <trigger> a b and the input "aab" the trigger must be crossed after the first 'a' but the lexical
//	analyzer consumes both 'a's and then fails. It also limits the rules: a state may be left by a single trigger and
//	a trigger may not leave an accepting state. _dfa_context<>::GetTriggerAmbiguities() and
//	_dfa_context<>::FCheckTriggerStructure() detect rules for which that method doesn't work.
//
// The solution:
// Here a trigger is instead an empty transition which is "tagged" with the trigger ( as with the tagged DFAs of
//	Laurikari and of Trofimovich's re2c ). While moving through the input there may be more than one way in which
//	the input read thus far might yet be matched, each having crossed triggers at different positions. The lexical
//	analyzer keeps the triggers crossed, and where, for each of these in a "register". When the token has been
//	found the triggers recorded for the way in which it was actually matched are fired, in order, each at the position
//	recorded for it.
// A state of the DFA is the set of NFA states reached having consumed a character ( the "kernel" ) partitioned into
//	classes: those of a class were reached by crossing the same triggers at the same positions and so share a
//	register. The classes are ordered by precedence - see below.
// A transition on a character c from a DFA state is found thusly:
//	1) Find every NFA state that can be reached from the kernel without consuming input, crossing triggers along
//		the way: the "closure". We record the triggers crossed to reach each.
//	2) Move from those on c. This gives the kernel of the destination. The triggers crossed in (1) to reach the
//		NFA states which had a transition on c - and only those - were crossed at the position before c.
//	3) The transition is thus associated with operations upon the registers: for each class of the destination,
//		copy the register of the class that it came from and then record the triggers crossed.
// Deciding which triggers to cross having seen the next character is what keeps this cheap: for instance for
//	( [a-z]+ <trigger> )+ the trigger is recorded only upon the character which ends the letters rather than
//	speculatively after each letter - and there is only ever a single class.
// The triggers crossed between the last character of a token and the end of its rule are recorded for the
//	accepting state: they fire after those in the register.
//
// Which way of matching is the one - precedence:
// When the same NFA state is reached in more than one way only the one of highest precedence is kept, and that
//	decides which triggers fire for a token that is matched in more than one way:
//	1) The way which crosses triggers later wins: at the first position at which two ways cross different triggers
//		the one which crosses none there wins.
//	2) If both cross triggers at that position then the one which crosses fewer wins.
//	3) Then the one whose first differing trigger was first used within the rules wins.
// (1) is what the lexical analyzer does for the untagged DFAs - where that works - so the result is the same.
// Which rule matches a token is decided as it is for any DFA: the longest match and then the earliest rule.
//
// The DFA produced:
// We produce the DFA within the same _dfa<> so that the optimizer and the generator work as they do for any DFA:
//	a transition which has operations enters a "tag operation state" which has a single transition, on a
//	pseudo-trigger which identifies the list of operations, to the destination. Thus two states are equivalent to
//	the optimizer only when their transitions perform the same operations. An accepting state is in an accept
//	partition according to the triggers crossed at the end of its token as well.
// The registers are numbered such that in an accepting state the register for the way in which the token is matched
//	is register zero - the lexical analyzer need only keep a copy of that register for the last accepting state.
//
// Pattern exclusion ( anti-accepting states ):
// For any DFA the state reached by matching an excluded pattern is anti-accepting: there is no token. Here that is
//	so of the ways of matching which began as the way which matched the excluded pattern did - not of the others.
//	For ( ( [xy]+ - "y" ) <trigger> )+ and the input "xy" the way which crosses the trigger between the 'x' and
//	the 'y' matches the excluded pattern but the way which doesn't cross it there does not, and that is the way of
//	highest precedence: "xy" is a token - as the lexical analyzer finds with an untagged DFA, never crossing the
//	trigger there. Whereas for ( x <trigger> y+ ) - "xy" the excluded pattern crosses no trigger and the way which
//	matches the rule does, having begun as that which matched the excluded pattern: "xy" is not a token.
//	1) An excluded pattern excludes the rules ( of higher action id - as usual ) which are matched by a way which
//		extends the way that matched the excluded pattern: it crossed the same triggers at the same positions and
//		then perhaps more.
//	2) If any rule that is matched remains then the state accepts for the one with the lowest action id as usual.
//	3) Otherwise the state is anti-accepting if a rule which is matched was excluded.
// To decide (1) a state of the DFA must know, for each pair of its classes, whether one extends the other - so
//	when the rules have excluded patterns that is part of what a state of the DFA is.
// As what an excluded pattern excludes depends upon the way in which it was matched, the ways of reaching an NFA
//	state from which only excluded patterns can be matched are kept: one of higher precedence doesn't replace
//	those of lower precedence as it does for any other NFA state. But not all of them need be kept:
//	a) Such a way of matching matters only for the ways which extend it: when none from which a rule may yet be
//		matched does it is dropped. Otherwise, for ( ( [xy]+ - "y" ) <trigger> )+, the way which crossed the trigger
//		before the last character would remain, within the excluded pattern, in a register of its own.
//	b) A way which extends another way of reaching the same such NFA state is redundant: all that it would exclude
//		the other excludes. This is what bounds the number of ways kept - and so the number of states of the DFA:
//		those which remain for an NFA state are each extended by a different way from which a rule may be matched.
//
// Equivalent NFA states:
// The NFA often has states which are not the same state but from which exactly the same can happen: for instance
//	x+ is the NFA for x followed by that for x*, so having matched the first x we are in a different state than
//	having matched any later x - yet what may follow is the same. Were we to treat these as different states then,
//	for ( x+ <trigger> )+, the way of matching which crossed the trigger before the last x would be in a state of
//	its own - and so would be kept, in a register of its own, until the next character: there would be operations
//	upon the registers for every character. So before creating the DFA we find the NFA states that are equivalent -
//	the same accepting states are reached without consuming input, and the same characters lead to equivalent
//	states, by crossing the same triggers - and use a single representative for each set of equivalent states.
// The states of two copies of a sub-expression are then equivalent - as with ( x <trigger> )+ which is the NFA
//	for ( x <trigger> ) followed by that for ( x <trigger> )*.
//
// "Completed by" ( unsatisfiable transitions ):
// _dfa_context<>::ProcessUnsatisfiableTranstitions() excises portions of the completed DFA. Here we do the same as
//	we go: when the closure reaches the tail of a "completed by" expression then the expression has completed and
//	the closure is replaced by that of the state following the tail.

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <numeric>

__REGEXP_BEGIN_NAMESPACE

template < class t_TyNfa, class t_TyDfa >
struct _create_dfa_tagged : public _create_dfa< t_TyNfa, t_TyDfa >
{
private:
	typedef _create_dfa_tagged _TyThis;
	typedef _create_dfa< t_TyNfa, t_TyDfa > _TyBase;
protected:
	using typename _TyBase::_TyNfa;
	using typename _TyBase::_TyNfaCtxt;
	using typename _TyBase::_TyDfa;
	using typename _TyBase::_TyDfaCtxt;
	using typename _TyBase::_TyState;
	using typename _TyBase::_TySetStatesNfa;
	using typename _TyBase::_TyGraphNodeDfa;
	using typename _TyBase::_TyAcceptAction;
	using _TyBase::m_rNfa;
	using _TyBase::m_rNfaCtxt;
	using _TyBase::m_rDfa;
	using _TyBase::m_rDfaCtxt;
	using _TyBase::m_fCreateDeadState;
	using _TyBase::m_pssAcceptingNfa;
	using _TyBase::m_pssCur;
	using _TyBase::m_fAmbiguousAntiAccepting;
	using _TyBase::_ClassifyNewDfaState;
	using _TyBase::_ConstructAcceptPartition;

	typedef typename _TyNfa::_TyRange _TyRange;
	typedef typename _TyNfa::_TyRangeEl _TyRangeEl;
	typedef typename _TyNfa::_TyGraph _TyGraphNfa;
	typedef typename _TyNfa::_TyGraphNode _TyGraphNodeNfa;
	typedef typename _TyDfa::_TyAlphaIndex _TyAlphaIndex;
	typedef typename _TyDfa::_TySetStates _TySetStatesDfa;
	typedef typename _TyDfa::_TyTagOp _TyTagOp;
	typedef typename _TyDfa::_TyTagOpList _TyTagOpList;
	typedef _fa_base< typename _TyNfa::_TyChar > _TyFaBase;

	// The triggers ( tags ) crossed at a single position in the order crossed.
	typedef std::vector< int > _TySeq;
	// The order of precedence of the triggers crossed at a single position: fewer first then lexicographically.
	static int _ICompareSeq( _TySeq const & _rl, _TySeq const & _rr )
	{
		if ( _rl.size() != _rr.size() )
			return ( _rl.size() < _rr.size() ) ? -1 : 1;
		for ( size_t n = 0; n < _rl.size(); ++n )
			if ( _rl[ n ] != _rr[ n ] )
				return ( _rl[ n ] < _rr[ n ] ) ? -1 : 1;
		return 0;
	}

	// Do the triggers _rseqExtending begin with the triggers _rseq.
	static bool _FSeqExtends( _TySeq const & _rseq, _TySeq const & _rseqExtending )
	{
		return ( _rseqExtending.size() >= _rseq.size() ) && std::equal( _rseq.begin(), _rseq.end(), _rseqExtending.begin() );
	}

	// An NFA state of the kernel of a DFA state and the rank of its class: zero is the highest precedence.
	struct _TyItem
	{
		_TyState m_stNfa;
		uint32_t m_nRank;
		bool operator < ( _TyItem const & _r ) const
		{
			return ( m_stNfa != _r.m_stNfa ) ? ( m_stNfa < _r.m_stNfa ) : ( m_nRank < _r.m_nRank );
		}
	};
	typedef std::vector< _TyItem > _TyKernel; // In order of the NFA state.

	// An NFA state reached from the kernel of a DFA state without consuming input: the rank of the class of the
	//	kernel item from which it is reached and the triggers crossed on the way - those of highest precedence.
	struct _TyConfig
	{
		_TyState m_stNfa;
		uint32_t m_nRank;
		_TySeq m_seq;
	};

	// What we need to know of each DFA state.
	struct _TyStateInfo
	{
		_TyGraphNodeDfa * m_pgn{ nullptr };
		bool m_fOpState{ false }; // A tag operation state - or the dead state: neither has a kernel.
		std::vector< _TyConfig > m_rgConfigs; // The closure of the kernel - until the state has been processed.
		std::vector< unsigned short > m_rgRegOfRank; // The register of each class by rank.
		// Whether the triggers crossed by each class extend those crossed by each other class: [ extended * nClasses + extending ].
		// Empty when the rules have no excluded patterns - we don't need to know this then.
		std::vector< bool > m_rgfExtends;
		int m_iTagFinish{ -1 }; // For an accepting state: the index of the triggers crossed at the end of the token - if any.
	};

	// The transitions of the reachable states of the NFA.
	struct _TyLinkEmpty
	{
		_TyState m_stChild;
		int m_iTag; // The trigger crossed by this transition or -1.
	};
	struct _TyLinkChar
	{
		_TyState m_stChild;
		size_t m_nAtomFirst; // The elements of the alphabet of the DFA on which this transition is taken: [m_nAtomFirst,m_nAtomLast].
		size_t m_nAtomLast;
	};
	struct _TyLinkUnsat
	{
		_TyState m_stChild;
		_TyRangeEl m_reUnsat; // An odd unsatisfiable: the tail of a "completed by" expression.
	};
	struct _TyNfaState
	{
		std::vector< _TyLinkEmpty > m_rgEmpty; // Empty and trigger transitions.
		std::vector< _TyLinkChar > m_rgChar;
		std::vector< _TyLinkUnsat > m_rgUnsat;
		std::vector< _TyState > m_rgChildren; // Every state this state has a transition to.
		// If this state is the start of the tail of a "completed by" expression - the tail being two transitions on the
		//	same odd unsatisfiable - then this is the state following the tail, otherwise -1.
		_TyState m_stAfterCompleted{ -1 };
	};

	std::vector< _TyNfaState > m_rgNfaStates; // By NFA state.
	std::vector< _TyState > m_rgstReachable; // The NFA states that can be reached from the start state.
	std::vector< _TyState > m_rgstRepresentative; // By NFA state: the representative of the NFA states equivalent to it.
	// By NFA state: can no accepting state other than an anti-accepting state be reached from this state: it is then
	//	within an excluded pattern - or can lead nowhere at all.
	std::vector< bool > m_rgfOnlyExcluded;
	std::vector< _TyRange > m_rgAtoms; // The characters of the alphabet - these are the first elements of the alphabet of the DFA.
	std::vector< _TyStateInfo > m_rgStates; // By DFA state.
	// The DFA state for each kernel - and which of its classes extend which when the rules have excluded patterns.
	typedef std::pair< _TyKernel, std::vector< bool > > _TyKernelKey;
	std::map< _TyKernelKey, size_t > m_mapKernels;
	bool m_fAntiAccepting{ false }; // Do the rules have any excluded patterns.
	std::map< _TyTagOpList, size_t > m_mapOpLists; // The index in m_rDfa.m_rgTagOpLists of each list of operations.
	std::map< std::pair< size_t, size_t >, _TyGraphNodeDfa * > m_mapOpStates; // ( list of operations, destination ) -> the tag operation state.
	std::map< _TySeq, int > m_mapFinishes; // The index in m_rDfa.m_rgTagFinishes for the triggers crossed at the end of a token.
	size_t m_nMaxClasses{ 0 };
	_TyGraphNodeDfa * m_pgnUnlinked{ nullptr }; // A new DFA state that is yet to be linked into the graph.

	// Used by _Closure():
	std::vector< int64_t > m_rgRankBest; // By NFA state: the rank of the best way of reaching it found thus far or -1.
	std::vector< _TySeq > m_rgSeqBest;
	std::vector< _TyState > m_rgReached;
	// For the NFA states from which only excluded patterns can be matched: the best way of reaching each for each rank.
	std::map< std::pair< _TyState, uint32_t >, _TySeq > m_mapOnlyExcludedBest;
	std::vector< std::pair< _TyState, uint32_t > > m_rgToProcess; // ( NFA state, rank ).
	// Used by _ProcessState():
	std::vector< std::vector< std::pair< _TyState, uint32_t > > > m_rgrgCandidates; // By atom: ( NFA state moved to, class moved from ).

public:
	_create_dfa_tagged( _TyNfa & _rNfa, _TyNfaCtxt & _rNfaCtxt, _TyDfa & _rDfa, _TyDfaCtxt & _rDfaCtxt, bool _fCreateDeadState )
		: _TyBase( _rNfa, _rNfaCtxt, _rDfa, _rDfaCtxt, _fCreateDeadState, 16 )
	{
	}

	// Create the tagged DFA from the NFA.
	bool create()
	{
		Assert( !m_rDfa.NStates() );
		const size_t knStatesNfa = (size_t)m_rNfa.NStates();

		// The accepting states of the NFA without the triggers: a trigger is not a state of a tagged DFA.
		_TySetStatesNfa ssAccepting( knStatesNfa, m_rNfa.get_allocator() );
		ssAccepting.clear();
		m_rNfaCtxt.GetAcceptingNodeSet( ssAccepting );
		for ( typename _TyNfa::_TySetAcceptIT itAccept = m_rNfa.m_pSetAcceptStates->begin(); itAccept != m_rNfa.m_pSetAcceptStates->end(); ++itAccept )
		{
			if ( e_aatTrigger == itAccept->second.m_eaatType )
				ssAccepting.clearbit( (size_t)itAccept->first );
			else
			if ( e_aatAntiAccepting == itAccept->second.m_eaatType )
				m_fAntiAccepting = true;
		}
		_TySetStatesNfa ssCur( knStatesNfa, m_rNfa.get_allocator() );
		ssCur.clear();
		m_pssAcceptingNfa = &ssAccepting;
		m_pssCur = &ssCur;

		// The alphabet of the DFA: the characters of the NFA's alphabet and then a pseudo-trigger for each list of
		//	operations - we add those as we find them.
		for ( typename _TyNfa::_TyAlphabet::const_iterator citAlpha = m_rNfa.m_setAlphabet.begin(); citAlpha != m_rNfa.m_setAlphabet.end(); ++citAlpha )
		{
			if ( !!citAlpha->first && ( citAlpha->first < _TyFaBase::ms_kreTriggerStart ) )
			{
				m_rgAtoms.push_back( *citAlpha );
				m_rDfa.m_setAlphabet.insert( *citAlpha );
			}
		}
		m_rgrgCandidates.resize( m_rgAtoms.size() );
		// Set up max original actions - disambiguating actions may be added.
		m_rDfa.m_iMaxActions = m_rNfa.m_iActionCur;

		_CollectNfa();
		m_rgRankBest.assign( knStatesNfa, -1 );
		m_rgSeqBest.resize( knStatesNfa );
		_FindOnlyExcluded();
		_FindEquivalentNfaStates();

		_TyGraphNodeDfa * pgnDead = 0;
		try
		{
			if ( m_fCreateDeadState )
			{
				// As with _create_dfa<> the dead state is the zeroth state - the optimizer expects it there.
				m_rDfa._NewStartState( &pgnDead );
				_TyStateInfo siDead;
				siDead.m_pgn = pgnDead;
				siDead.m_fOpState = true;
				m_rgStates.push_back( siDead );
			}
			{//B
				_TyKernel kernelStart;
				_TyItem itemStart = { m_rgstRepresentative[ (size_t)m_rNfaCtxt.m_pgnStart->RElConst() ], 0 };
				kernelStart.push_back( itemStart );
				m_rDfa._NewStartState( &m_rDfaCtxt.m_pgnStart );
				_AddState( _TyKernelKey( kernelStart, std::vector< bool >( m_fAntiAccepting ? 1 : 0, true ) ), m_rDfaCtxt.m_pgnStart );
			}//EB
			// Process each state - this adds states.
			for ( size_t st = 0; st < m_rgStates.size(); ++st )
			{
				if ( !m_rgStates[ st ].m_fOpState )
					_ProcessState( st );
			}

			// The pseudo-triggers are now known: they are transitions like any other from here on.
			m_rDfa.m_nTriggers = m_rDfa.m_rgTagOpLists.size();
			m_rDfa.m_nUnsatisfiableTransitions = 0;
			const size_t knAlphabet = m_rDfa.AlphabetSize();
			Assert( knAlphabet == m_rgAtoms.size() + m_rDfa.m_rgTagOpLists.size() );
			if ( m_fCreateDeadState )
			{
				// Every state needs a transition on every element of the alphabet: the optimizer depends upon it.
				std::vector< bool > rgfHave;
				for ( size_t st = 1; st < m_rgStates.size(); ++st )
				{
					_TyGraphNodeDfa * pgn = m_rgStates[ st ].m_pgn;
					rgfHave.assign( knAlphabet, false );
					{//B
						typename _TyDfa::_TyGraph::_TyLinkPosIterConst lpi( pgn->PPGLChildHead() );
						for ( ; !lpi.FIsLast(); lpi.NextChild() )
							rgfHave[ (size_t)*lpi ] = true;
					}//EB
					for ( size_t ai = 0; ai < knAlphabet; ++ai )
					{
						if ( !rgfHave[ ai ] )
							m_rDfa._NewTransition( pgn, (_TyAlphaIndex)ai, pgnDead, 0 );
					}
				}
				if ( pgnDead->FParents() )
				{
					for ( size_t ai = 0; ai < knAlphabet; ++ai )
						m_rDfa._NewTransition( pgnDead, (_TyAlphaIndex)ai, pgnDead, 0 );
				}
			}
		}
		catch ( ... )
		{
			// Destroy what isn't part of the graph that the context owns.
			if ( m_pgnUnlinked )
				m_rDfa.DestroySubGraph( m_pgnUnlinked );
			if ( pgnDead && !pgnDead->FParents() )
				m_rDfa.DestroySubGraph( pgnDead );
			throw;
		}
		// The transitions of each state in the same order: as the optimizer and CompressTransitions() expect.
		m_rDfa.SortTransitions();

		// A trigger is never a transition from an anti-accepting state here.
		m_fAmbiguousAntiAccepting = false;
		_ConstructAcceptPartition();
		_SplitAcceptPartitionByFinish();
		_CreateTags();

		VerifyThrowSz( m_nMaxClasses + 1 < _TyDfa::s_knTagRegisterTemp, "Too many registers are required [%zu].", m_nMaxClasses + 1 );
		m_rDfa.m_nTagRegisters = m_nMaxClasses + 1; // One more for the temporary.
		m_rDfa.m_fTagged = true;
		if ( m_fCreateDeadState )
			m_rDfa.m_fHasDeadState = true;
		m_rDfa.m_fHasLookaheads = m_rNfa.m_fHasLookaheads;
		m_rDfa.m_mapLookaheadReverse = m_rNfa.m_mapLookaheadReverse;
		m_pssAcceptingNfa = 0;
		m_pssCur = 0;
		return true;
	}

protected:
	// Record the transitions of each state of the NFA that can be reached from its start state.
	// We don't use the NFA's node lookup to find the states as it contains the nodes of states that no longer exist.
	void _CollectNfa()
	{
		const size_t knStatesNfa = (size_t)m_rNfa.NStates();
		m_rgNfaStates.resize( knStatesNfa );
		std::vector< bool > rgfSeen( knStatesNfa, false );
		std::vector< const _TyGraphNodeNfa * > rgpgnToProcess;
		rgpgnToProcess.push_back( m_rNfaCtxt.m_pgnStart );
		rgfSeen[ (size_t)m_rNfaCtxt.m_pgnStart->RElConst() ] = true;
		while ( !rgpgnToProcess.empty() )
		{
			const _TyGraphNodeNfa * pgnCur = rgpgnToProcess.back();
			rgpgnToProcess.pop_back();
			m_rgstReachable.push_back( pgnCur->RElConst() );
			_TyNfaState & rns = m_rgNfaStates[ (size_t)pgnCur->RElConst() ];
			typename _TyGraphNfa::_TyLinkPosIterConst lpi( pgnCur->PPGLChildHead() );
			for ( ; !lpi.FIsLast(); lpi.NextChild() )
			{
				const _TyRange & rrng = *lpi;
				const _TyState stChild = lpi.PGNChild()->RElConst();
				rns.m_rgChildren.push_back( stChild );
				if ( !rgfSeen[ (size_t)stChild ] )
				{
					rgfSeen[ (size_t)stChild ] = true;
					rgpgnToProcess.push_back( lpi.PGNChild() );
				}
				if ( !rrng.first )
				{
					_TyLinkEmpty le = { stChild, -1 };
					rns.m_rgEmpty.push_back( le );
				}
				else
				if ( rrng.first >= _TyFaBase::ms_kreUnsatisfiableStart )
				{
					// Only the odd unsatisfiables matter to us: they are the tail of a "completed by" expression.
					if ( ( rrng.first - _TyFaBase::ms_kreUnsatisfiableStart ) % 2 )
					{
						_TyLinkUnsat lu = { stChild, rrng.first };
						rns.m_rgUnsat.push_back( lu );
					}
				}
				else
				if ( rrng.first >= _TyFaBase::ms_kreTriggerStart )
				{
					// A trigger is a pair of transitions: the trigger is crossed on the first ( even ) of them.
					const _TyRangeEl kreTrigger = rrng.first - _TyFaBase::ms_kreTriggerStart;
					_TyLinkEmpty le = { stChild, ( kreTrigger % 2 ) ? -1 : int( kreTrigger / 2 ) };
					rns.m_rgEmpty.push_back( le );
				}
				else
				{
					// Each transition is on a range which is the union of consecutive elements of the alphabet.
					typename std::vector< _TyRange >::const_iterator citFirst = std::lower_bound( m_rgAtoms.begin(), m_rgAtoms.end(), rrng.first,
						[]( _TyRange const & _rrng, _TyRangeEl const & _rre ) { return _rrng.first < _rre; } );
					VerifyThrowSz( ( citFirst != m_rgAtoms.end() ) && ( citFirst->first == rrng.first ), "A transition's range doesn't begin at an element of the alphabet." );
					typename std::vector< _TyRange >::const_iterator citLast = citFirst;
					for ( ; citLast->second < rrng.second; ++citLast )
						VerifyThrowSz( ( citLast + 1 ) != m_rgAtoms.end(), "A transition's range doesn't end at an element of the alphabet." );
					VerifyThrowSz( citLast->second == rrng.second, "A transition's range doesn't end at an element of the alphabet." );
					_TyLinkChar lc = { stChild, size_t( citFirst - m_rgAtoms.begin() ), size_t( citLast - m_rgAtoms.begin() ) };
					rns.m_rgChar.push_back( lc );
				}
			}
		}
		// Find the start of the tail of each "completed by" expression: two transitions on the same odd unsatisfiable.
		for ( _TyState stNfa : m_rgstReachable )
		{
			_TyNfaState & rns = m_rgNfaStates[ (size_t)stNfa ];
			for ( _TyLinkUnsat const & rluFirst : rns.m_rgUnsat )
			{
				for ( _TyLinkUnsat const & rluSecond : m_rgNfaStates[ (size_t)rluFirst.m_stChild ].m_rgUnsat )
				{
					if ( rluSecond.m_reUnsat == rluFirst.m_reUnsat )
						rns.m_stAfterCompleted = rluSecond.m_stChild;
				}
			}
		}
	}

	// Record that _stNfa can be reached thusly if this is of higher precedence than any way found thus far.
	// For a state from which only excluded patterns can be matched we keep the best way for each rank - see the overview.
	void _Relax( _TyState _stNfa, uint32_t _nRank, _TySeq const & _rseq )
	{
		const size_t kst = (size_t)_stNfa;
		if ( m_rgfOnlyExcluded[ kst ] )
		{
			std::pair< typename std::map< std::pair< _TyState, uint32_t >, _TySeq >::iterator, bool > pib =
				m_mapOnlyExcludedBest.insert( std::make_pair( std::make_pair( _stNfa, _nRank ), _rseq ) );
			if ( !pib.second )
			{
				if ( _ICompareSeq( _rseq, pib.first->second ) >= 0 )
					return;
				pib.first->second = _rseq;
			}
		}
		else
		{
			if ( m_rgRankBest[ kst ] < 0 )
				m_rgReached.push_back( _stNfa );
			else
			if ( ( int64_t( _nRank ) > m_rgRankBest[ kst ] ) ||
					 ( ( int64_t( _nRank ) == m_rgRankBest[ kst ] ) && ( _ICompareSeq( _rseq, m_rgSeqBest[ kst ] ) >= 0 ) ) )
				return;
			m_rgRankBest[ kst ] = _nRank;
			m_rgSeqBest[ kst ] = _rseq;
		}
		m_rgToProcess.push_back( std::make_pair( _stNfa, _nRank ) );
	}
	void _ClearReached()
	{
		for ( _TyState stNfa : m_rgReached )
		{
			m_rgRankBest[ (size_t)stNfa ] = -1;
			m_rgSeqBest[ (size_t)stNfa ].clear();
		}
		m_rgReached.clear();
		m_mapOnlyExcludedBest.clear();
	}
	// Get the configurations found by _Relax() in order of the NFA state and then the rank.
	void _GetReached( std::vector< _TyConfig > & _rrgConfigs )
	{
		_rrgConfigs.clear();
		_rrgConfigs.reserve( m_rgReached.size() + m_mapOnlyExcludedBest.size() );
		for ( _TyState stNfa : m_rgReached )
		{
			_TyConfig cfg;
			cfg.m_stNfa = stNfa;
			cfg.m_nRank = (uint32_t)m_rgRankBest[ (size_t)stNfa ];
			cfg.m_seq = m_rgSeqBest[ (size_t)stNfa ];
			_rrgConfigs.push_back( std::move( cfg ) );
		}
		for ( typename std::map< std::pair< _TyState, uint32_t >, _TySeq >::const_iterator cit = m_mapOnlyExcludedBest.begin(); cit != m_mapOnlyExcludedBest.end(); ++cit )
		{
			_TyConfig cfg;
			cfg.m_stNfa = cit->first.first;
			cfg.m_nRank = cit->first.second;
			cfg.m_seq = cit->second;
			_rrgConfigs.push_back( std::move( cfg ) );
		}
		std::sort( _rrgConfigs.begin(), _rrgConfigs.end(), []( _TyConfig const & _rl, _TyConfig const & _rr )
			{ return ( _rl.m_stNfa != _rr.m_stNfa ) ? ( _rl.m_stNfa < _rr.m_stNfa ) : ( _rl.m_nRank < _rr.m_nRank ); } );
	}

	// Find each NFA state that can be reached from the kernel without consuming input and the way of doing so which
	//	has the highest precedence. The result is in order of the NFA state - and then of the rank for the states
	//	from which only excluded patterns can be matched: those have a configuration for each rank that reaches them.
	// _pseqCompleted: If non-null then it receives the triggers crossed to reach the tail of a "completed by" expression
	//	if one is reached, with *_pfCompleted then true.
	void _Closure( _TyKernel const & _rkernel, std::vector< _TyConfig > & _rrgConfigs, bool * _pfCompleted = nullptr, _TySeq * _pseqCompleted = nullptr )
	{
		Assert( m_rgReached.empty() && m_mapOnlyExcludedBest.empty() && m_rgToProcess.empty() );
		if ( _pfCompleted )
			*_pfCompleted = false;
		{//B
			_TySeq seqEmpty;
			for ( _TyItem const & ritem : _rkernel )
				_Relax( ritem.m_stNfa, ritem.m_nRank, seqEmpty );
		}//EB
		_TySeq seqCur;
		for ( ;; )
		{
			// A way of reaching a state only ever extends a way of reaching another and extending a way never raises its
			//	precedence above that of another: so this ends with the best way for each state.
			while ( !m_rgToProcess.empty() )
			{
				const _TyState stCur = m_rgToProcess.back().first;
				const uint32_t knRank = m_rgToProcess.back().second;
				m_rgToProcess.pop_back();
				const _TySeq * pseqBest;
				if ( m_rgfOnlyExcluded[ (size_t)stCur ] )
					pseqBest = &m_mapOnlyExcludedBest.find( std::make_pair( stCur, knRank ) )->second;
				else
				{
					if ( m_rgRankBest[ (size_t)stCur ] != int64_t( knRank ) )
						continue; // A way of higher precedence has since been found - and is to be processed.
					pseqBest = &m_rgSeqBest[ (size_t)stCur ];
				}
				_TyNfaState const & rns = m_rgNfaStates[ (size_t)stCur ];
				for ( _TyLinkEmpty const & rle : rns.m_rgEmpty )
				{
					seqCur = *pseqBest; // A copy: this may change as we go.
					if ( rle.m_iTag >= 0 )
						seqCur.push_back( rle.m_iTag );
					_Relax( rle.m_stChild, knRank, seqCur );
				}
			}
			_GetReached( _rrgConfigs );
			// If we have reached the tail of a "completed by" expression then the expression has completed: all that
			//	remains is that which follows it - by the way of highest precedence in which it completed.
			const _TyConfig * pcfgCompleted = nullptr;
			for ( _TyConfig const & rcfg : _rrgConfigs )
			{
				if ( m_rgNfaStates[ (size_t)rcfg.m_stNfa ].m_stAfterCompleted < 0 )
					continue;
				if ( !pcfgCompleted || ( rcfg.m_nRank < pcfgCompleted->m_nRank ) ||
						 ( ( rcfg.m_nRank == pcfgCompleted->m_nRank ) && ( _ICompareSeq( rcfg.m_seq, pcfgCompleted->m_seq ) < 0 ) ) )
					pcfgCompleted = &rcfg;
			}
			if ( !pcfgCompleted )
				break;
			const _TyState stFollowing = m_rgNfaStates[ (size_t)pcfgCompleted->m_stNfa ].m_stAfterCompleted;
			const uint32_t knRank = pcfgCompleted->m_nRank;
			seqCur = pcfgCompleted->m_seq;
			if ( _pfCompleted && !*_pfCompleted )
			{
				*_pfCompleted = true;
				*_pseqCompleted = seqCur;
			}
			_ClearReached();
			_Relax( stFollowing, knRank, seqCur );
		}
		_ClearReached();
	}

	// Find the NFA states from which no accepting state other than an anti-accepting state can be reached.
	void _FindOnlyExcluded()
	{
		if ( !m_fAntiAccepting )
		{
			m_rgfOnlyExcluded.assign( m_rgNfaStates.size(), false ); // No state is within an excluded pattern.
			return;
		}
		m_rgfOnlyExcluded.assign( m_rgNfaStates.size(), true );
		for ( typename _TyNfa::_TySetAcceptIT itAccept = m_rNfa.m_pSetAcceptStates->begin(); itAccept != m_rNfa.m_pSetAcceptStates->end(); ++itAccept )
		{
			if ( ( e_aatTrigger != itAccept->second.m_eaatType ) && ( e_aatAntiAccepting != itAccept->second.m_eaatType ) )
				m_rgfOnlyExcluded[ (size_t)itAccept->first ] = false;
		}
		// A state reaches what its children reach: until nothing changes.
		for ( bool fChanged = true; fChanged; )
		{
			fChanged = false;
			for ( _TyState stNfa : m_rgstReachable )
			{
				if ( !m_rgfOnlyExcluded[ (size_t)stNfa ] )
					continue;
				for ( _TyState stChild : m_rgNfaStates[ (size_t)stNfa ].m_rgChildren )
				{
					if ( !m_rgfOnlyExcluded[ (size_t)stChild ] )
					{
						m_rgfOnlyExcluded[ (size_t)stNfa ] = false;
						fChanged = true;
						break;
					}
				}
			}
		}
	}

	// Find the NFA states which are equivalent - see the overview - and choose a representative for each set of them.
	// Two states are equivalent when, from each:
	//	a) the same accepting states ( of any kind ) are reached without consuming input, crossing the same triggers,
	//	b) the tail of a "completed by" expression is reached or not, crossing the same triggers, and
	//	c) each character leads to equivalent states, crossing the same triggers before it.
	// We find the equivalences as the optimizer does for the states of a DFA: partition the states by (a) and (b)
	//	and then by (c) until the partition no longer changes.
	void _FindEquivalentNfaStates()
	{
		const size_t knStatesNfa = m_rgNfaStates.size();
		m_rgstRepresentative.resize( knStatesNfa );
		for ( size_t st = 0; st < knStatesNfa; ++st )
			m_rgstRepresentative[ st ] = (_TyState)st;
		// For each state: the accepting states reached and the triggers crossed, whether a "completed by" completes,
		//	and the states that each character leads to with the triggers crossed before it.
		typedef std::pair< _TyState, _TySeq > _TyReached; // ( NFA state, triggers crossed ).
		struct _TyStep
		{
			size_t m_nAtom;
			_TyState m_stTo;
			_TySeq m_seq;
		};
		typedef std::pair< std::pair< bool, _TySeq >, std::vector< _TyReached > > _TyInitialKey;
		std::map< _TyInitialKey, size_t > mapInitial;
		std::vector< size_t > rgClass( knStatesNfa, 0 );
		std::vector< std::vector< _TyStep > > rgrgSteps( knStatesNfa );
		{//B
			_TyKernel kernel( 1 );
			std::vector< _TyConfig > rgConfigs;
			for ( _TyState stNfa : m_rgstReachable )
			{
				kernel[ 0 ].m_stNfa = stNfa;
				kernel[ 0 ].m_nRank = 0;
				_TyInitialKey key;
				_Closure( kernel, rgConfigs, &key.first.first, &key.first.second );
				std::vector< _TyStep > & rrgSteps = rgrgSteps[ (size_t)stNfa ];
				for ( _TyConfig const & rcfg : rgConfigs )
				{
					if ( m_pssAcceptingNfa->isbitset( (size_t)rcfg.m_stNfa ) )
						key.second.push_back( _TyReached( rcfg.m_stNfa, rcfg.m_seq ) );
					for ( _TyLinkChar const & rlc : m_rgNfaStates[ (size_t)rcfg.m_stNfa ].m_rgChar )
					{
						for ( size_t nAtom = rlc.m_nAtomFirst; nAtom <= rlc.m_nAtomLast; ++nAtom )
						{
							_TyStep step = { nAtom, rlc.m_stChild, rcfg.m_seq };
							rrgSteps.push_back( step );
						}
					}
				}
				rgClass[ (size_t)stNfa ] = mapInitial.insert( std::make_pair( key, mapInitial.size() ) ).first->second;
			}
		}//EB
		// Refine: the key of a state is its class and, for each character, the classes it leads to - with the triggers
		//	of highest precedence for each.
		typedef std::pair< std::pair< size_t, size_t >, _TySeq > _TyStepKey; // ( ( character, class led to ), triggers crossed ).
		typedef std::pair< size_t, std::vector< _TyStepKey > > _TyRefineKey;
		auto fnStepKeyLess = []( _TyStepKey const & _rl, _TyStepKey const & _rr )
		{
			if ( _rl.first != _rr.first )
				return _rl.first < _rr.first;
			return _ICompareSeq( _rl.second, _rr.second ) < 0;
		};
		size_t nClasses = mapInitial.size();
		std::vector< size_t > rgClassNew( knStatesNfa, 0 );
		for ( ;; )
		{
			std::map< _TyRefineKey, size_t > mapRefine;
			for ( _TyState stNfa : m_rgstReachable )
			{
				_TyRefineKey key;
				key.first = rgClass[ (size_t)stNfa ];
				for ( _TyStep const & rstep : rgrgSteps[ (size_t)stNfa ] )
					key.second.push_back( _TyStepKey( std::make_pair( rstep.m_nAtom, rgClass[ (size_t)rstep.m_stTo ] ), rstep.m_seq ) );
				std::sort( key.second.begin(), key.second.end(), fnStepKeyLess );
				key.second.erase( std::unique( key.second.begin(), key.second.end(),
					[]( _TyStepKey const & _rl, _TyStepKey const & _rr ) { return _rl.first == _rr.first; } ), key.second.end() );
				rgClassNew[ (size_t)stNfa ] = mapRefine.insert( std::make_pair( key, mapRefine.size() ) ).first->second;
			}
			rgClass.swap( rgClassNew );
			if ( mapRefine.size() == nClasses )
				break; // A refinement only ever splits classes.
			nClasses = mapRefine.size();
		}
		// The representative of each class is its lowest numbered state.
		const _TyState kstNone = -1;
		std::vector< _TyState > rgstRepOfClass( nClasses, kstNone );
		for ( _TyState stNfa : m_rgstReachable )
		{
			_TyState & rstRep = rgstRepOfClass[ rgClass[ (size_t)stNfa ] ];
			if ( ( kstNone == rstRep ) || ( stNfa < rstRep ) )
				rstRep = stNfa;
		}
		for ( _TyState stNfa : m_rgstReachable )
			m_rgstRepresentative[ (size_t)stNfa ] = rgstRepOfClass[ rgClass[ (size_t)stNfa ] ];
	}

	// Add the new DFA state _pgn for the kernel - and which of its classes extend which - _rkey.
	size_t _AddState( _TyKernelKey const & _rkey, _TyGraphNodeDfa * _pgn )
	{
		_TyKernel const & _rkernel = _rkey.first;
		const size_t kstNew = m_rgStates.size();
		Assert( kstNew == (size_t)_pgn->RElConst() );
		_TyStateInfo si;
		si.m_pgn = _pgn;
		si.m_rgfExtends = _rkey.second;
		_Closure( _rkernel, si.m_rgConfigs );
		uint32_t nClasses = 0;
		for ( _TyItem const & ritem : _rkernel )
			nClasses = (std::max)( nClasses, ritem.m_nRank + 1 );
		Assert( si.m_rgfExtends.empty() || ( si.m_rgfExtends.size() == size_t( nClasses ) * nClasses ) );

		// The rule which matches here, if any: that of the accepting state with the lowest action id - an accepting
		//	state for the token of a lookahead rule isn't one as the rule has yet to match - once we have dealt with
		//	pattern exclusion - see the overview.
		const _TyConfig * pcfgWinner = 0;
		m_pssCur->clear();
		for ( _TyConfig const & rcfg : si.m_rgConfigs )
			m_pssCur->setbit( (size_t)rcfg.m_stNfa );
		{//B
			struct _TyAccept
			{
				const _TyConfig * m_pcfg;
				vtyActionIdent m_aiAction;
				bool m_fAntiAccepting;
				bool m_fExcluded;
			};
			std::vector< _TyAccept > rgAccepts;
			bool fAnyAntiAccepting = false;
			for ( _TyConfig const & rcfg : si.m_rgConfigs )
			{
				if ( !m_pssAcceptingNfa->isbitset( (size_t)rcfg.m_stNfa ) )
					continue;
				typename _TyNfa::_TySetAcceptIT itAccept = m_rNfa.m_pSetAcceptStates->find( rcfg.m_stNfa );
				Assert( m_rNfa.m_pSetAcceptStates->end() != itAccept );
				const EActionActionType keaat = itAccept->second.m_eaatType;
				if ( ( e_aatAccept == keaat ) || ( e_aatLookahead == keaat ) || ( e_aatAntiAccepting == keaat ) )
				{
					_TyAccept accept = { &rcfg, itAccept->second.m_aiAction, e_aatAntiAccepting == keaat, false };
					rgAccepts.push_back( accept );
					fAnyAntiAccepting = fAnyAntiAccepting || accept.m_fAntiAccepting;
				}
			}
			if ( fAnyAntiAccepting )
			{
				// (1) An excluded pattern excludes the rules matched by a way which extends the way that matched it.
				for ( _TyAccept const & rAnti : rgAccepts )
				{
					if ( !rAnti.m_fAntiAccepting )
						continue;
					for ( _TyAccept & rAccept : rgAccepts )
					{
						if ( rAccept.m_fAntiAccepting || ( rAccept.m_aiAction < rAnti.m_aiAction ) )
							continue;
						Assert( !m_rgfOnlyExcluded[ (size_t)rAccept.m_pcfg->m_stNfa ] );
						// Does the way to rAccept extend the way to rAnti: if they are of the same class then by the triggers crossed
						//	here, otherwise the class must extend that of rAnti - which must then have crossed no trigger here.
						if ( ( rAccept.m_pcfg->m_nRank == rAnti.m_pcfg->m_nRank ) ? _FSeqExtends( rAnti.m_pcfg->m_seq, rAccept.m_pcfg->m_seq )
									: ( rAnti.m_pcfg->m_seq.empty() && si.m_rgfExtends[ size_t( rAnti.m_pcfg->m_nRank ) * nClasses + rAccept.m_pcfg->m_nRank ] ) )
							rAccept.m_fExcluded = true;
					}
				}
			}
			// (2) The rule which remains with the lowest action id.
			vtyActionIdent aiMin = (std::numeric_limits< vtyActionIdent >::max)();
			for ( _TyAccept const & rAccept : rgAccepts )
			{
				if ( !rAccept.m_fAntiAccepting && !rAccept.m_fExcluded && ( rAccept.m_aiAction < aiMin ) )
				{
					aiMin = rAccept.m_aiAction;
					pcfgWinner = rAccept.m_pcfg;
				}
			}
			if ( fAnyAntiAccepting )
			{
				// Leave only that which decides the kind of accepting state as it is decided for any DFA: by the NFA states.
				// (3) If no rule remains then the state is anti-accepting if a rule which is matched was excluded: we then
				//	leave the anti-accepting states - each rule excluded was excluded by one with a lower action id.
				bool fAnyExcluded = false;
				for ( _TyAccept const & rAccept : rgAccepts )
					fAnyExcluded = fAnyExcluded || rAccept.m_fExcluded;
				for ( _TyAccept const & rAccept : rgAccepts )
				{
					if ( rAccept.m_fExcluded || ( rAccept.m_fAntiAccepting && ( !!pcfgWinner || !fAnyExcluded ) ) )
						m_pssCur->clearbit( (size_t)rAccept.m_pcfg->m_stNfa );
				}
			}
		}//EB
		_ClassifyNewDfaState( _pgn );

		// Number the registers: that of the class which matches the token is zero, the rest are in order of rank.
		m_nMaxClasses = (std::max)( m_nMaxClasses, size_t( nClasses ) );
		si.m_rgRegOfRank.resize( nClasses );
		{//B
			unsigned short nReg = pcfgWinner ? 1 : 0;
			for ( uint32_t nRank = 0; nRank < nClasses; ++nRank )
				si.m_rgRegOfRank[ nRank ] = ( pcfgWinner && ( pcfgWinner->m_nRank == nRank ) ) ? (unsigned short)0 : nReg++;
		}//EB
		if ( pcfgWinner && !pcfgWinner->m_seq.empty() )
		{
			std::pair< typename std::map< _TySeq, int >::iterator, bool > pib = m_mapFinishes.insert( std::make_pair( pcfgWinner->m_seq, (int)m_rDfa.m_rgTagFinishes.size() ) );
			if ( pib.second )
				m_rDfa.m_rgTagFinishes.push_back( pcfgWinner->m_seq );
			si.m_iTagFinish = pib.first->second;
		}
		m_mapKernels[ _rkey ] = kstNew;
		m_rgStates.push_back( std::move( si ) );
		return kstNew;
	}

	// The operations for a transition: _rrgMoves is, for each class of the destination, ( the destination register,
	//	the source register, the triggers crossed ). The registers are all copied as one: we order the copies such that
	//	no register is overwritten before it is read - using the temporary register when registers are exchanged.
	struct _TyMove
	{
		unsigned short m_nDst;
		unsigned short m_nSrc;
		const _TySeq * m_pseq;
	};
	void _GetOpList( std::vector< _TyMove > & _rrgMoves, _TyTagOpList & _rol )
	{
		_rol.clear();
		std::sort( _rrgMoves.begin(), _rrgMoves.end(), []( _TyMove const & _rl, _TyMove const & _rr ) { return _rl.m_nDst < _rr.m_nDst; } );
		std::vector< std::pair< unsigned short, unsigned short > > rgCopies; // ( destination, source ).
		for ( _TyMove const & rmv : _rrgMoves )
		{
			if ( rmv.m_nDst != rmv.m_nSrc )
				rgCopies.push_back( std::make_pair( rmv.m_nDst, rmv.m_nSrc ) );
		}
		while ( !rgCopies.empty() )
		{
			// Find a copy whose destination is not the source of any other.
			size_t nCopy = 0;
			for ( ; nCopy < rgCopies.size(); ++nCopy )
			{
				size_t nOther = 0;
				for ( ; nOther < rgCopies.size(); ++nOther )
				{
					if ( ( nOther != nCopy ) && ( rgCopies[ nOther ].second == rgCopies[ nCopy ].first ) )
						break;
				}
				if ( nOther == rgCopies.size() )
					break;
			}
			if ( nCopy < rgCopies.size() )
			{
				_TyTagOp op = { rgCopies[ nCopy ].first, rgCopies[ nCopy ].second, -1 };
				_rol.push_back( op );
				rgCopies.erase( rgCopies.begin() + nCopy );
			}
			else
			{
				// Registers are being exchanged: keep the first destination in the temporary - those that read it now read that.
				const unsigned short knDst = rgCopies.front().first;
				_TyTagOp op = { _TyDfa::s_knTagRegisterTemp, knDst, -1 };
				_rol.push_back( op );
				for ( std::pair< unsigned short, unsigned short > & rpr : rgCopies )
				{
					if ( rpr.second == knDst )
						rpr.second = _TyDfa::s_knTagRegisterTemp;
				}
			}
		}
		// Then record the triggers crossed.
		for ( _TyMove const & rmv : _rrgMoves )
		{
			for ( int iTag : *rmv.m_pseq )
			{
				_TyTagOp op = { rmv.m_nDst, rmv.m_nDst, iTag };
				_rol.push_back( op );
			}
		}
	}

	// Add the transitions from a DFA state - this adds the states they lead to that we don't yet have.
	void _ProcessState( size_t _st )
	{
		std::vector< _TyConfig > rgConfigs;
		rgConfigs.swap( m_rgStates[ _st ].m_rgConfigs ); // We are done with these after this.
		const std::vector< unsigned short > rgRegOfRank( m_rgStates[ _st ].m_rgRegOfRank ); // A copy: m_rgStates grows.
		const std::vector< bool > rgfExtends( m_rgStates[ _st ].m_rgfExtends );
		_TyGraphNodeDfa * const pgnSrc = m_rgStates[ _st ].m_pgn;
		const size_t knConfigs = rgConfigs.size();

		// The configurations fall into classes according to the triggers they cross from here: number them in order of precedence.
		std::vector< size_t > rgOrder( knConfigs );
		std::iota( rgOrder.begin(), rgOrder.end(), size_t( 0 ) );
		std::sort( rgOrder.begin(), rgOrder.end(), [&rgConfigs]( size_t _nl, size_t _nr )
		{
			_TyConfig const & rl = rgConfigs[ _nl ];
			_TyConfig const & rr = rgConfigs[ _nr ];
			if ( rl.m_nRank != rr.m_nRank )
				return rl.m_nRank < rr.m_nRank;
			const int kiCompare = _ICompareSeq( rl.m_seq, rr.m_seq );
			return kiCompare ? ( kiCompare < 0 ) : ( _nl < _nr );
		} );
		std::vector< uint32_t > rgClassOfConfig( knConfigs );
		std::vector< size_t > rgConfigOfClass; // A configuration of each class.
		for ( size_t n = 0; n < knConfigs; ++n )
		{
			_TyConfig const & rcfg = rgConfigs[ rgOrder[ n ] ];
			if ( !n || ( rcfg.m_nRank != rgConfigs[ rgOrder[ n-1 ] ].m_nRank ) || !!_ICompareSeq( rcfg.m_seq, rgConfigs[ rgOrder[ n-1 ] ].m_seq ) )
				rgConfigOfClass.push_back( rgOrder[ n ] );
			rgClassOfConfig[ rgOrder[ n ] ] = uint32_t( rgConfigOfClass.size() - 1 );
		}

		// For each element of the alphabet: the NFA states moved to ( the representative of each ) and the class moved from.
		for ( size_t nConfig = 0; nConfig < knConfigs; ++nConfig )
		{
			for ( _TyLinkChar const & rlc : m_rgNfaStates[ (size_t)rgConfigs[ nConfig ].m_stNfa ].m_rgChar )
			{
				for ( size_t nAtom = rlc.m_nAtomFirst; nAtom <= rlc.m_nAtomLast; ++nAtom )
					m_rgrgCandidates[ nAtom ].push_back( std::make_pair( m_rgstRepresentative[ (size_t)rlc.m_stChild ], rgClassOfConfig[ nConfig ] ) );
			}
		}

		_TyKernelKey key;
		_TyKernel & kernel = key.first;
		std::vector< bool > & rgfExtendsDst = key.second;
		const size_t knClassesSrc = rgRegOfRank.size();
		std::vector< uint32_t > rgClassesUsed;
		std::vector< bool > rgfLive, rgfKeep;
		auto fnGetClassesUsed = [&rgClassesUsed]( std::vector< std::pair< _TyState, uint32_t > > const & _rrgCandidates )
		{
			rgClassesUsed.clear();
			for ( std::pair< _TyState, uint32_t > const & rpr : _rrgCandidates )
				rgClassesUsed.push_back( rpr.second );
			std::sort( rgClassesUsed.begin(), rgClassesUsed.end() );
			rgClassesUsed.erase( std::unique( rgClassesUsed.begin(), rgClassesUsed.end() ), rgClassesUsed.end() );
		};
		// Does the class _nClassExtending of the configurations extend the class _nClassExtended: it came from the same
		//	class of this state and crosses the same triggers and then perhaps more, or it came from a class which
		//	extends that of the other and the other crosses no triggers.
		auto fnExtends = [&]( uint32_t _nClassExtended, uint32_t _nClassExtending ) -> bool
		{
			_TyConfig const & rcfgExtended = rgConfigs[ rgConfigOfClass[ _nClassExtended ] ];
			_TyConfig const & rcfgExtending = rgConfigs[ rgConfigOfClass[ _nClassExtending ] ];
			return ( rcfgExtended.m_nRank == rcfgExtending.m_nRank ) ? _FSeqExtends( rcfgExtended.m_seq, rcfgExtending.m_seq )
				: ( rcfgExtended.m_seq.empty() && rgfExtends[ size_t( rcfgExtended.m_nRank ) * knClassesSrc + rcfgExtending.m_nRank ] );
		};
		std::vector< _TyMove > rgMoves;
		_TyTagOpList ol;
		for ( size_t nAtom = 0; nAtom < m_rgrgCandidates.size(); ++nAtom )
		{
			std::vector< std::pair< _TyState, uint32_t > > & rrgCandidates = m_rgrgCandidates[ nAtom ];
			if ( rrgCandidates.empty() )
				continue; // No transition - or to the dead state: added once we know the whole alphabet.
			// Where an NFA state is moved to from more than one class the one of highest precedence wins - but for a state
			//	from which only excluded patterns can be matched: we keep each of the classes.
			std::sort( rrgCandidates.begin(), rrgCandidates.end() );
			rrgCandidates.erase( std::unique( rrgCandidates.begin(), rrgCandidates.end(),
				[this]( std::pair< _TyState, uint32_t > const & _rl, std::pair< _TyState, uint32_t > const & _rr )
				{ return ( _rl.first == _rr.first ) && ( !m_rgfOnlyExcluded[ (size_t)_rl.first ] || ( _rl.second == _rr.second ) ); } ), rrgCandidates.end() );
			if ( m_fAntiAccepting )
			{
				// Of those kept for such a state a class which extends another is redundant: all that it would exclude the other excludes.
				rgfKeep.assign( rrgCandidates.size(), true );
				bool fAnyRedundant = false;
				for ( size_t nFirst = 0; nFirst < rrgCandidates.size(); )
				{
					size_t nEnd = nFirst + 1;
					for ( ; ( nEnd < rrgCandidates.size() ) && ( rrgCandidates[ nEnd ].first == rrgCandidates[ nFirst ].first ); ++nEnd )
						;
					for ( size_t nCur = nFirst; ( nEnd - nFirst > 1 ) && ( nCur < nEnd ); ++nCur )
					{
						for ( size_t nOther = nFirst; ( nOther < nEnd ) && rgfKeep[ nCur ]; ++nOther )
						{
							if ( ( nOther != nCur ) && fnExtends( rrgCandidates[ nOther ].second, rrgCandidates[ nCur ].second ) )
								rgfKeep[ nCur ] = false;
						}
						fAnyRedundant = fAnyRedundant || !rgfKeep[ nCur ];
					}
					nFirst = nEnd;
				}
				if ( fAnyRedundant )
				{
					size_t nKept = 0;
					for ( size_t nCur = 0; nCur < rrgCandidates.size(); ++nCur )
					{
						if ( rgfKeep[ nCur ] )
							rrgCandidates[ nKept++ ] = rrgCandidates[ nCur ];
					}
					rrgCandidates.resize( nKept );
				}
			}
			// The classes that remain, in order of precedence, are the classes of the destination.
			fnGetClassesUsed( rrgCandidates );
			if ( m_fAntiAccepting )
			{
				// A class which is only within excluded patterns matters only to the classes which extend it: if there are
				//	none which may yet match a rule then we drop it - see the overview.
				rgfLive.assign( rgClassesUsed.size(), false );
				bool fAnyOnlyExcluded = false;
				for ( std::pair< _TyState, uint32_t > const & rpr : rrgCandidates )
				{
					if ( !m_rgfOnlyExcluded[ (size_t)rpr.first ] )
						rgfLive[ std::lower_bound( rgClassesUsed.begin(), rgClassesUsed.end(), rpr.second ) - rgClassesUsed.begin() ] = true;
					else
						fAnyOnlyExcluded = true;
				}
				if ( fAnyOnlyExcluded )
				{
					rgfKeep = rgfLive;
					bool fAnyDropped = false;
					for ( size_t nClass = 0; nClass < rgClassesUsed.size(); ++nClass )
					{
						if ( rgfLive[ nClass ] )
							continue;
						for ( size_t nClassLive = 0; ( nClassLive < rgClassesUsed.size() ) && !rgfKeep[ nClass ]; ++nClassLive )
							rgfKeep[ nClass ] = rgfLive[ nClassLive ] && fnExtends( rgClassesUsed[ nClass ], rgClassesUsed[ nClassLive ] );
						fAnyDropped = fAnyDropped || !rgfKeep[ nClass ];
					}
					if ( fAnyDropped )
					{
						rrgCandidates.erase( std::remove_if( rrgCandidates.begin(), rrgCandidates.end(), [&]( std::pair< _TyState, uint32_t > const & _rpr )
							{ return !rgfKeep[ std::lower_bound( rgClassesUsed.begin(), rgClassesUsed.end(), _rpr.second ) - rgClassesUsed.begin() ]; } ), rrgCandidates.end() );
						if ( rrgCandidates.empty() )
							continue; // No rule can be matched from here.
						fnGetClassesUsed( rrgCandidates );
					}
				}
			}
			kernel.clear();
			for ( std::pair< _TyState, uint32_t > const & rpr : rrgCandidates )
			{
				_TyItem item = { rpr.first, uint32_t( std::lower_bound( rgClassesUsed.begin(), rgClassesUsed.end(), rpr.second ) - rgClassesUsed.begin() ) };
				kernel.push_back( item );
			}
			rrgCandidates.clear();
			// Which classes of the destination extend which.
			rgfExtendsDst.clear();
			if ( m_fAntiAccepting )
			{
				const size_t knClassesDst = rgClassesUsed.size();
				rgfExtendsDst.resize( knClassesDst * knClassesDst, false );
				for ( size_t nExtended = 0; nExtended < knClassesDst; ++nExtended )
				{
					for ( size_t nExtending = 0; nExtending < knClassesDst; ++nExtending )
						rgfExtendsDst[ nExtended * knClassesDst + nExtending ] = fnExtends( rgClassesUsed[ nExtended ], rgClassesUsed[ nExtending ] );
				}
			}

			// Find or add the destination.
			size_t stDst;
			typename std::map< _TyKernelKey, size_t >::const_iterator citKernel = m_mapKernels.find( key );
			if ( m_mapKernels.end() != citKernel )
				stDst = citKernel->second;
			else
			{
				Assert( !m_pgnUnlinked );
				m_rDfa._NewStartState( &m_pgnUnlinked ); // Merely a new state - we link it below.
				stDst = _AddState( key, m_pgnUnlinked );
			}
			_TyGraphNodeDfa * const pgnDst = m_rgStates[ stDst ].m_pgn;

			// The operations: the register of each class of the destination is that of the class it came from plus the triggers crossed.
			rgMoves.clear();
			for ( size_t nRankDst = 0; nRankDst < rgClassesUsed.size(); ++nRankDst )
			{
				_TyConfig const & rcfg = rgConfigs[ rgConfigOfClass[ rgClassesUsed[ nRankDst ] ] ];
				_TyMove mv = { m_rgStates[ stDst ].m_rgRegOfRank[ nRankDst ], rgRegOfRank[ rcfg.m_nRank ], &rcfg.m_seq };
				rgMoves.push_back( mv );
			}
			_GetOpList( rgMoves, ol );
			if ( ol.empty() )
				m_rDfa._NewTransition( pgnSrc, (_TyAlphaIndex)nAtom, pgnDst, 0 );
			else
			{
				// The transition enters a tag operation state whose single transition performs the operations.
				std::pair< typename std::map< _TyTagOpList, size_t >::iterator, bool > pibOpList = m_mapOpLists.insert( std::make_pair( ol, m_rDfa.m_rgTagOpLists.size() ) );
				if ( pibOpList.second )
				{
					const _TyRangeEl kreOpList = _TyFaBase::ms_kreTriggerStart + (_TyRangeEl)m_rDfa.m_rgTagOpLists.size();
					VerifyThrowSz( kreOpList < _TyFaBase::ms_kreUnsatisfiableStart, "Too many lists of tag operations." );
					m_rDfa.m_rgTagOpLists.push_back( ol );
					m_rDfa.m_setAlphabet.insert( _TyRange( kreOpList, kreOpList ) );
				}
				const size_t kiOpList = pibOpList.first->second;
				typename std::map< std::pair< size_t, size_t >, _TyGraphNodeDfa * >::const_iterator citOpState = m_mapOpStates.find( std::make_pair( kiOpList, stDst ) );
				if ( m_mapOpStates.end() != citOpState )
					m_rDfa._NewTransition( pgnSrc, (_TyAlphaIndex)nAtom, citOpState->second, 0 );
				else
				{
					_TyGraphNodeDfa * pgnOpState;
					m_rDfa._NewAcceptingState( pgnSrc, (_TyAlphaIndex)nAtom, &pgnOpState, 0 );
					_TyStateInfo siOpState;
					siOpState.m_pgn = pgnOpState;
					siOpState.m_fOpState = true;
					Assert( m_rgStates.size() == (size_t)pgnOpState->RElConst() );
					m_rgStates.push_back( siOpState );
					m_mapOpStates[ std::make_pair( kiOpList, stDst ) ] = pgnOpState;
					m_rDfa._NewTransition( pgnOpState, (_TyAlphaIndex)( m_rgAtoms.size() + kiOpList ), pgnDst, 0 );
				}
			}
			m_pgnUnlinked = 0; // Any new state is now part of the graph.
		}
	}

	// States which are in the same accept partition but which cross different triggers at the end of the token mustn't
	//	be equivalent: split the partitions accordingly and record the triggers in the action of each.
	void _SplitAcceptPartitionByFinish()
	{
		typedef typename _TyDfaCtxt::_TyPartAcceptStates _TyPartAcceptStates;
		std::vector< typename _TyPartAcceptStates::iterator > rgitParts;
		for ( typename _TyPartAcceptStates::iterator itPart = m_rDfaCtxt.m_partAccept.begin(); itPart != m_rDfaCtxt.m_partAccept.end(); ++itPart )
			rgitParts.push_back( itPart );
		const size_t knStatesDfa = (size_t)m_rDfa.NStates();
		for ( typename _TyPartAcceptStates::iterator itPart : rgitParts )
		{
			std::map< int, _TySetStatesDfa > mapByFinish;
			{//B
				_TySetStatesDfa const & rss = itPart->first.RObject();
				for ( size_t st = rss.getfirstset(); rss.size() != st; st = rss.getnextset( st ) )
				{
					const int kiFinish = m_rgStates[ st ].m_iTagFinish;
					typename std::map< int, _TySetStatesDfa >::iterator itByFinish = mapByFinish.find( kiFinish );
					if ( mapByFinish.end() == itByFinish )
					{
						_TySetStatesDfa ssNew( knStatesDfa, m_rDfa.get_allocator() );
						ssNew.clear();
						itByFinish = mapByFinish.insert( std::make_pair( kiFinish, ssNew ) ).first;
					}
					itByFinish->second.setbit( st );
				}
			}//EB
			if ( mapByFinish.size() <= 1 )
			{
				if ( !mapByFinish.empty() )
					itPart->second.m_iTagFinish = mapByFinish.begin()->first;
				continue;
			}
			_TyAcceptAction aaCopy( itPart->second );
			m_rDfaCtxt.m_partAccept.erase( itPart );
			for ( typename std::map< int, _TySetStatesDfa >::iterator itByFinish = mapByFinish.begin(); itByFinish != mapByFinish.end(); ++itByFinish )
			{
				aaCopy.m_iTagFinish = itByFinish->first;
				typename _TyPartAcceptStates::value_type vt( itByFinish->second, aaCopy );
#if ASSERTSENABLED
				std::pair< typename _TyPartAcceptStates::iterator, bool > pib =
#endif //ASSERTSENABLED
				m_rDfaCtxt.m_partAccept.insert( vt );
				Assert( pib.second );
			}
		}
	}

	// Record the tags - the triggers - of the DFA and decide whether there is an order in which they are always crossed.
	void _CreateTags()
	{
		const size_t knTags = (size_t)m_rNfa.m_nTriggers / 2;
		{//B
			typename _TyDfa::_TyTag tagInit = { vktidInvalidIdToken, -1, -1 };
			m_rDfa.m_rgTags.assign( knTags, tagInit );
		}//EB
		if ( !knTags )
			return;
		for ( typename _TyNfa::_TyMapTokenIdToTriggerTransition::const_iterator citTrigger = m_rNfa.m_mapTokenIdToTriggerTransition.begin();
					citTrigger != m_rNfa.m_mapTokenIdToTriggerTransition.end(); ++citTrigger )
			m_rDfa.m_rgTags[ (size_t)citTrigger->second / 2 ].m_tidTrigger = citTrigger->first;

		// The generator obtains the trigger actions from this map.
		m_rDfa.m_pMapTriggers.template emplace< typename _TyDfa::_TyCompareAI const &, typename _TyDfa::_TyAllocator const & >(
			typename _TyDfa::_TyCompareAI(), m_rDfa.get_allocator() );
		for ( typename _TyNfa::_TySetAcceptIT itAccept = m_rNfa.m_pSetAcceptStates->begin(); itAccept != m_rNfa.m_pSetAcceptStates->end(); ++itAccept )
		{
			if ( e_aatTrigger != itAccept->second.m_eaatType )
				continue;
			const vtyTokenIdent ktidTrigger = ( *itAccept->second.m_pSdpAction )->VGetTokenId();
			for ( typename _TyDfa::_TyTag & rtag : m_rDfa.m_rgTags )
			{
				if ( ( rtag.m_tidTrigger == ktidTrigger ) && ( rtag.m_aiTrigger < 0 ) )
				{
					rtag.m_aiTrigger = itAccept->second.m_aiAction;
					typename _TyDfa::_TyMapTriggers::value_type vt( itAccept->second.m_aiAction, itAccept->second );
					m_rDfa.m_pMapTriggers->insert( vt );
				}
			}
		}
		for ( typename _TyDfa::_TyTag const & rtag : m_rDfa.m_rgTags )
			VerifyThrowSz( rtag.m_aiTrigger >= 0, "No action for the trigger with token id [%d].", (int)rtag.m_tidTrigger );

		// Trigger A precedes trigger B if there is any way to get from where A is crossed to where B is crossed.
		// If that relation has no cycles then on any path through the NFA each trigger is crossed at most once and
		//	in an order which is consistent with any topological order of the relation.
		std::vector< std::vector< _TyState > > rgrgAfter( knTags ); // The states entered by crossing each trigger.
		std::vector< std::vector< _TyState > > rgrgBefore( knTags ); // The states from which each trigger is crossed.
		for ( size_t stNfa = 0; stNfa < m_rgNfaStates.size(); ++stNfa )
		{
			for ( _TyLinkEmpty const & rle : m_rgNfaStates[ stNfa ].m_rgEmpty )
			{
				if ( rle.m_iTag >= 0 )
				{
					rgrgAfter[ (size_t)rle.m_iTag ].push_back( rle.m_stChild );
					rgrgBefore[ (size_t)rle.m_iTag ].push_back( (_TyState)stNfa );
				}
			}
		}
		std::vector< std::vector< bool > > rgrgfPrecedes( knTags, std::vector< bool >( knTags, false ) );
		std::vector< bool > rgfSeen;
		std::vector< _TyState > rgstToProcess;
		for ( size_t nTag = 0; nTag < knTags; ++nTag )
		{
			rgfSeen.assign( m_rgNfaStates.size(), false );
			for ( _TyState stAfter : rgrgAfter[ nTag ] )
			{
				if ( !rgfSeen[ (size_t)stAfter ] )
				{
					rgfSeen[ (size_t)stAfter ] = true;
					rgstToProcess.push_back( stAfter );
				}
			}
			while ( !rgstToProcess.empty() )
			{
				const _TyState stCur = rgstToProcess.back();
				rgstToProcess.pop_back();
				for ( _TyState stChild : m_rgNfaStates[ (size_t)stCur ].m_rgChildren )
				{
					if ( !rgfSeen[ (size_t)stChild ] )
					{
						rgfSeen[ (size_t)stChild ] = true;
						rgstToProcess.push_back( stChild );
					}
				}
			}
			for ( size_t nTagOther = 0; nTagOther < knTags; ++nTagOther )
			{
				for ( _TyState stBefore : rgrgBefore[ nTagOther ] )
				{
					if ( rgfSeen[ (size_t)stBefore ] )
						rgrgfPrecedes[ nTag ][ nTagOther ] = true;
				}
			}
		}
		// A topological order - choosing the lowest numbered trigger at each step.
		std::vector< int > rgnSlots( knTags, -1 );
		size_t nSlot = 0;
		for ( ; nSlot < knTags; ++nSlot )
		{
			size_t nTag = 0;
			for ( ; nTag < knTags; ++nTag )
			{
				if ( rgnSlots[ nTag ] >= 0 )
					continue;
				size_t nTagOther = 0;
				for ( ; nTagOther < knTags; ++nTagOther )
				{
					if ( ( rgnSlots[ nTagOther ] < 0 ) && rgrgfPrecedes[ nTagOther ][ nTag ] )
						break; // A trigger that has no slot yet precedes this one - or this one precedes itself.
				}
				if ( nTagOther == knTags )
					break;
			}
			if ( nTag == knTags )
				break; // A cycle.
			rgnSlots[ nTag ] = (int)nSlot;
		}
		m_rDfa.m_fTagSlots = ( nSlot == knTags ) && ( knTags <= s_knMaxTagSlots );
		if ( m_rDfa.m_fTagSlots )
		{
			for ( size_t nTag = 0; nTag < knTags; ++nTag )
				m_rDfa.m_rgTags[ nTag ].m_nSlot = rgnSlots[ nTag ];
		}
	}
public:
	// With more triggers than this we record the history of the triggers rather than keeping a position for each
	//	trigger in each register: copying a register then costs more than it is worth.
	static constexpr size_t s_knMaxTagSlots = 16;
};

// How the triggers within a set of rules are to be dealt with when creating the DFA for the rules.
enum ETriggerDfaKind
{
	etdkAuto,
		// Create the usual, untagged, DFA unless the lexical analyzer couldn't then fire the triggers correctly:
		//	in that case create a tagged DFA. This costs nothing for rules which don't need a tagged DFA.
	etdkUntagged,
		// Always create an untagged DFA. The generator refuses such a DFA if its triggers are ambiguous.
	etdkTagged,
		// Create a tagged DFA whenever the rules contain triggers.
	etdkETriggerDfaKindCount
};

// Return the DFA and its context to their initial state: destroy them and construct them again in place.
template < class t_TyDfa >
void _l_reset_dfa( t_TyDfa & _rDfa, typename t_TyDfa::_TyContext & _rDfaCtxt )
{
	typedef typename t_TyDfa::_TyContext _TyDfaCtxt;
	typename t_TyDfa::_TyAllocator alloc( _rDfa.get_allocator() );
	_rDfaCtxt.~_TyDfaCtxt();
	_rDfa.~t_TyDfa();
	new ( (void*)&_rDfa ) t_TyDfa( alloc );
	new ( (void*)&_rDfaCtxt ) _TyDfaCtxt( _rDfa );
}

// Create the DFA for the set of rules in _rFinal, ready to be added to a _l_generator<>.
// _rDfa and _rDfaCtxt must be newly constructed.
// _fOptimizeDfa: When false the DFA is not optimized - nor does it have a dead state: it is as created from the NFA.
// _fCompressTransitions: Compress the transitions - as must be done before generating a lexical analyzer.
// _grfNFACreationOptions: See ENFACreationOptions.
// _etdk: How to deal with triggers - see ETriggerDfaKind.
// _pstrWhyTagged: If non-null and a tagged DFA is created because an untagged DFA would not do then this receives the reason.
// Returns whether a tagged DFA was created - which is also recorded in the DFA: _dfa<>::m_fTagged.
template < class t_TyFinal, class t_TyDfa >
bool _l_create_dfa( t_TyFinal const & _rFinal, t_TyDfa & _rDfa, typename t_TyDfa::_TyContext & _rDfaCtxt,
										bool _fOptimizeDfa = true, bool _fCompressTransitions = true, uint32_t _grfNFACreationOptions = 0,
										ETriggerDfaKind _etdk = etdkAuto, std::string * _pstrWhyTagged = nullptr )
{
	typedef t_TyDfa _TyDfa;
	typedef _nfa< typename _TyDfa::_TyChar, typename _TyDfa::_TyAllocator > _TyNfa;
	typedef typename _TyNfa::_TyNfaCtxt _TyNfaCtxt;
	Assert( !_rDfa.NStates() );

	bool fHasTriggers = false;
	bool fCreateTagged = false;
	std::string strWhyTagged;
	try
	{
		{//B
			_TyNfa nfa( _grfNFACreationOptions, _rDfa.get_allocator() );
			_TyNfaCtxt nfaCtxt( nfa );
			_rFinal.ConstructNFA( nfaCtxt );
			fHasTriggers = !!nfa.m_nTriggers;
			fCreateTagged = fHasTriggers && ( etdkTagged == _etdk );
			if ( !fCreateTagged )
			{
				_create_dfa< _TyNfa, _TyDfa > cdfa( nfa, nfaCtxt, _rDfa, _rDfaCtxt, _fOptimizeDfa );
				cdfa.create();
			}
		}//EB
		if ( !fCreateTagged )
		{
			if ( _fOptimizeDfa )
			{
				_optimize_dfa< _TyDfa > optdfa( _rDfa, _rDfaCtxt );
				(void)optdfa.optimize();
			}
			_rDfaCtxt.ProcessUnsatisfiableTranstitions();
			// Can the lexical analyzer cross the triggers of this DFA as transitions and find what the rules specify ?
			if ( fHasTriggers && ( etdkAuto == _etdk ) )
				fCreateTagged = !_rDfaCtxt.FTriggersUnambiguous( strWhyTagged );
		}
	}
	catch ( std::exception const & _rexc )
	{
		// Rules whose triggers an untagged DFA cannot represent may also fail during its creation.
		if ( fCreateTagged || !fHasTriggers || ( etdkAuto != _etdk ) )
			throw;
		fCreateTagged = true;
		strWhyTagged = _rexc.what();
	}
	if ( fCreateTagged )
	{
		if ( !!_rDfa.NStates() || !!_rDfaCtxt.m_pgnStart )
			_l_reset_dfa( _rDfa, _rDfaCtxt );
		{//B
			// The creation of the untagged DFA changes the NFA so we create it anew.
			_TyNfa nfa( _grfNFACreationOptions, _rDfa.get_allocator() );
			_TyNfaCtxt nfaCtxt( nfa );
			_rFinal.ConstructNFA( nfaCtxt );
			_create_dfa_tagged< _TyNfa, _TyDfa > cdfa( nfa, nfaCtxt, _rDfa, _rDfaCtxt, _fOptimizeDfa );
			cdfa.create();
		}//EB
		if ( _fOptimizeDfa )
		{
			_optimize_dfa< _TyDfa > optdfa( _rDfa, _rDfaCtxt );
			(void)optdfa.optimize();
		}
		// There are no unsatisfiable transitions in a tagged DFA - this merely checks that no portion is orphaned.
		_rDfaCtxt.ProcessUnsatisfiableTranstitions();
		if ( _pstrWhyTagged )
			*_pstrWhyTagged = strWhyTagged;
	}
	if ( _fCompressTransitions )
		_rDfa.CompressTransitions();
	return fCreateTagged;
}

__REGEXP_END_NAMESPACE

#endif //__L_DFATAG_H

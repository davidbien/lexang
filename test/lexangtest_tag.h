#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangtest_tag.h
// For the tests of tagged DFAs ( see _l_dfatag.h ):
// 1) A reference against which we check the tagged DFAs and the lexical analyzers generated from them: a simulation
//		of the NFA for a set of rules which keeps, for each NFA state it is in, the triggers crossed ( and where ) by
//		the way of reaching that state which has the highest precedence. The reference knows nothing of kernels,
//		classes, registers, operations or of equivalent NFA states - nor does it drop any way of matching.
// 2) The sets of rules for which lexangtestgen.cpp generates lexical analyzers and which lexanglextest.cpp checks
//		against the reference.
// lexangtest_gen.h must be included before this.

#include <map>
#include <vector>
#include <string>
#include <limits>
#include <ostream>
#include <algorithm>

namespace ns_lexangtest
{

// The result of matching a string against a set of rules is the token id of the rule which matched or:
static const int s_knNoMatch = -1; // The string is not matched.
static const int s_knMatchNoToken = 0; // The string is matched by a rule without an action.

// A trigger and the position at which it was crossed.
struct TagEvent
{
	int m_nTrigger; // The token id of the trigger - or its index while matching.
	size_t m_pos;
	bool operator == ( TagEvent const & _r ) const
	{
		return ( m_nTrigger == _r.m_nTrigger ) && ( m_pos == _r.m_pos );
	}
};
typedef std::vector< TagEvent > TyRgTagEvents;
struct TagMatch
{
	int m_nToken{ s_knNoMatch };
	size_t m_nLength{ 0 };
	TyRgTagEvents m_rgEvents;
	bool operator == ( TagMatch const & _r ) const
	{
		return ( m_nToken == _r.m_nToken ) && ( m_nLength == _r.m_nLength ) && ( m_rgEvents == _r.m_rgEvents );
	}
};
inline std::ostream & operator << ( std::ostream & _ros, TagMatch const & _r )
{
	_ros << "{token:" << _r.m_nToken << ",length:" << _r.m_nLength;
	for ( TagEvent const & rev : _r.m_rgEvents )
		_ros << ",(" << rev.m_nTrigger << "@" << rev.m_pos << ")";
	return _ros << "}";
}
inline TagMatch ExpectTagMatch( int _nToken, size_t _nLength, std::initializer_list< TagEvent > _ilEvents = {} )
{
	TagMatch tm;
	tm.m_nToken = _nToken;
	tm.m_nLength = _nLength;
	tm.m_rgEvents.assign( _ilEvents );
	return tm;
}

// The reference: simulate the NFA.
struct TagReference
{
	typedef _TyNfa::_TyGraph _TyGraph;
	typedef _TyNfa::_TyGraphNode _TyGraphNode;
	typedef _TyNfa::_TyRange _TyRange;
	typedef _TyNfa::_TyRangeEl _TyRangeEl;
	typedef _TyNfa::_TyState _TyState;
	// The ways of reaching each NFA state we are in - each being the triggers crossed. There is the one way, that of
	//	highest precedence, but for the states from which only excluded patterns can be matched: what an excluded
	//	pattern excludes depends upon the way in which it was matched so for those we keep each way which crossed
	//	different triggers before the current position - the one of highest precedence of those.
	typedef std::map< _TyState, std::vector< TyRgTagEvents > > _TyMapStates;

	_TyNfa m_nfa;
	_TyNfaCtxt m_nctxt;
	std::vector< vtyTokenIdent > m_rgtidTriggers; // The token id of each trigger by its index - the order of first use within the rules.
	// By NFA state: can no accepting state other than an anti-accepting state be reached from this state: it is then
	//	within an excluded pattern - or can lead nowhere at all.
	std::vector< bool > m_rgfOnlyExcluded;
	std::ostream * m_posTrace{ nullptr }; // If set then Match() writes the accepting NFA states it is in at each position - for debugging.

	TagReference( _TyFinal const & _rFinal )
		: m_nctxt( m_nfa )
	{
		_rFinal.ConstructNFA( m_nctxt );
		_TyNfa::_TySetStates ssAccept( (size_t)m_nfa.NStates(), m_nfa.get_allocator() );
		m_nctxt.GetAcceptingNodeSet( ssAccept ); // This completes the set of accepting states of the NFA.
		m_rgtidTriggers.resize( (size_t)m_nfa.m_nTriggers / 2 );
		for ( auto const & rvt : m_nfa.m_mapTokenIdToTriggerTransition )
			m_rgtidTriggers[ (size_t)rvt.second / 2 ] = rvt.first;
		// Find the states from which a rule may yet be matched - of the states reachable from the start state.
		m_rgfOnlyExcluded.assign( (size_t)m_nfa.NStates(), true );
		for ( auto const & rvt : *m_nfa.m_pSetAcceptStates )
		{
			if ( ( e_aatTrigger != rvt.second.m_eaatType ) && ( e_aatAntiAccepting != rvt.second.m_eaatType ) )
				m_rgfOnlyExcluded[ (size_t)rvt.first ] = false;
		}
		std::vector< _TyState > rgReachable;
		{//B
			std::vector< bool > rgfSeen( (size_t)m_nfa.NStates(), false );
			std::vector< _TyState > rgToProcess( 1, m_nctxt.m_pgnStart->RElConst() );
			rgfSeen[ (size_t)rgToProcess[0] ] = true;
			while ( !rgToProcess.empty() )
			{
				const _TyState stCur = rgToProcess.back();
				rgToProcess.pop_back();
				rgReachable.push_back( stCur );
				_TyGraph::_TyLinkPosIterConst lpi( m_nfa.PGNGetNode( stCur )->PPGLChildHead() );
				for ( ; !lpi.FIsLast(); lpi.NextChild() )
				{
					const _TyState stChild = lpi.PGNChild()->RElConst();
					if ( !rgfSeen[ (size_t)stChild ] )
					{
						rgfSeen[ (size_t)stChild ] = true;
						rgToProcess.push_back( stChild );
					}
				}
			}
		}//EB
		for ( bool fChanged = true; fChanged; )
		{
			fChanged = false;
			for ( _TyState stCur : rgReachable )
			{
				if ( !m_rgfOnlyExcluded[ (size_t)stCur ] )
					continue;
				_TyGraph::_TyLinkPosIterConst lpi( m_nfa.PGNGetNode( stCur )->PPGLChildHead() );
				for ( ; !lpi.FIsLast(); lpi.NextChild() )
				{
					if ( !m_rgfOnlyExcluded[ (size_t)lpi.PGNChild()->RElConst() ] )
					{
						m_rgfOnlyExcluded[ (size_t)stCur ] = false;
						fChanged = true;
						break;
					}
				}
			}
		}
	}
	// The order of precedence: -1 if _rl has precedence over _rr. At the first position at which they cross different
	//	triggers the one crossing fewer has precedence, then the one whose first differing trigger has the lower index.
	static int ICompare( TyRgTagEvents const & _rl, TyRgTagEvents const & _rr )
	{
		size_t nl = 0, nr = 0;
		while ( ( nl < _rl.size() ) || ( nr < _rr.size() ) )
		{
			const size_t kposL = ( nl < _rl.size() ) ? _rl[ nl ].m_pos : (std::numeric_limits< size_t >::max)();
			const size_t kposR = ( nr < _rr.size() ) ? _rr[ nr ].m_pos : (std::numeric_limits< size_t >::max)();
			const size_t kpos = (std::min)( kposL, kposR );
			size_t nlEnd = nl, nrEnd = nr;
			for ( ; ( nlEnd < _rl.size() ) && ( _rl[ nlEnd ].m_pos == kpos ); ++nlEnd )
				;
			for ( ; ( nrEnd < _rr.size() ) && ( _rr[ nrEnd ].m_pos == kpos ); ++nrEnd )
				;
			if ( ( nlEnd - nl ) != ( nrEnd - nr ) )
				return ( ( nlEnd - nl ) < ( nrEnd - nr ) ) ? -1 : 1;
			for ( ; nl < nlEnd; ++nl, ++nr )
			{
				if ( _rl[ nl ].m_nTrigger != _rr[ nr ].m_nTrigger )
					return ( _rl[ nl ].m_nTrigger < _rr[ nr ].m_nTrigger ) ? -1 : 1;
			}
		}
		return 0;
	}
	// Do the triggers _rrgExtending begin with the triggers _rrg.
	static bool FExtends( TyRgTagEvents const & _rrg, TyRgTagEvents const & _rrgExtending )
	{
		return ( _rrgExtending.size() >= _rrg.size() ) && std::equal( _rrg.begin(), _rrg.end(), _rrgExtending.begin() );
	}
	// The triggers of _rrg crossed before _pos.
	static TyRgTagEvents RgBefore( TyRgTagEvents const & _rrg, size_t _pos )
	{
		TyRgTagEvents rgBefore;
		for ( TagEvent const & rev : _rrg )
			if ( rev.m_pos < _pos )
				rgBefore.push_back( rev );
		return rgBefore;
	}
	// Record that _st can be reached thusly, at _pos, if this is of higher precedence than the way it replaces.
	bool FRelax( _TyMapStates & _rmap, _TyState _st, TyRgTagEvents const & _rrgEvents, size_t _pos )
	{
		std::vector< TyRgTagEvents > & rrgWays = _rmap[ _st ];
		for ( TyRgTagEvents & rrgWay : rrgWays )
		{
			if ( m_rgfOnlyExcluded[ (size_t)_st ] && ( RgBefore( rrgWay, _pos ) != RgBefore( _rrgEvents, _pos ) ) )
				continue; // Not the way this would replace.
			if ( ICompare( _rrgEvents, rrgWay ) >= 0 )
				return false;
			rrgWay = _rrgEvents;
			return true;
		}
		rrgWays.push_back( _rrgEvents );
		return true;
	}
	// Add each state reachable from those of _rmap without consuming input - crossing triggers at _pos.
	void Closure( _TyMapStates & _rmap, size_t _pos )
	{
		// Process every way of reaching every state until nothing changes.
		for ( bool fChanged = true; fChanged; )
		{
			fChanged = false;
			std::vector< std::pair< _TyState, TyRgTagEvents > > rgWays;
			for ( auto const & rvt : _rmap )
				for ( TyRgTagEvents const & rrgWay : rvt.second )
					rgWays.push_back( std::make_pair( rvt.first, rrgWay ) );
			for ( auto const & rprWay : rgWays )
			{
				_TyGraph::_TyLinkPosIterConst lpi( m_nfa.PGNGetNode( rprWay.first )->PPGLChildHead() );
				for ( ; !lpi.FIsLast(); lpi.NextChild() )
				{
					const _TyRange & rrng = *lpi;
					const bool kfEmpty = !rrng.first;
					const bool kfTrigger = ( rrng.first >= _fa_base< _TyCTok >::ms_kreTriggerStart ) && ( rrng.first < _fa_base< _TyCTok >::ms_kreUnsatisfiableStart );
					if ( !kfEmpty && !kfTrigger )
						continue;
					TyRgTagEvents rgEvents( rprWay.second );
					if ( kfTrigger && !( ( rrng.first - _fa_base< _TyCTok >::ms_kreTriggerStart ) % 2 ) )
					{
						TagEvent ev = { int( ( rrng.first - _fa_base< _TyCTok >::ms_kreTriggerStart ) / 2 ), _pos };
						rgEvents.push_back( ev );
					}
					if ( FRelax( _rmap, lpi.PGNChild()->RElConst(), rgEvents, _pos ) )
						fChanged = true;
				}
			}
		}
	}
	TagMatch Match( const char * _psz )
	{
		TagMatch tmLast;
		const size_t knLen = strlen( _psz );
		_TyMapStates mapCur;
		mapCur[ m_nctxt.m_pgnStart->RElConst() ].push_back( TyRgTagEvents() );
		for ( size_t pos = 0; ; ++pos )
		{
			Closure( mapCur, pos );
			// The rule which matches here is that of the accepting state with the lowest action id - once we have dealt with
			//	pattern exclusion:
			//	1) An excluded pattern excludes the rules ( of higher action id ) that are matched by a way which extends
			//		the way which matched the excluded pattern: it crossed the same triggers at the same positions then perhaps more.
			//	2) If a rule which is matched remains then the one with the lowest action id is the token.
			//	3) Otherwise there is no token - nor do we fall back to a shorter match - if a rule which is matched was excluded.
			{//B
				struct Accept
				{
					const _TyNfa::_TySetAcceptVT * m_pvtAccept;
					const TyRgTagEvents * m_prgEvents;
					bool m_fExcluded;
				};
				std::vector< Accept > rgAccepts;
				for ( auto const & rvt : mapCur )
				{
					auto itAccept = m_nfa.m_pSetAcceptStates->find( rvt.first );
					if ( m_nfa.m_pSetAcceptStates->end() == itAccept )
						continue;
					const EActionActionType keaat = itAccept->second.m_eaatType;
					if ( ( e_aatAccept != keaat ) && ( e_aatAntiAccepting != keaat ) )
						continue;
					for ( TyRgTagEvents const & rrgWay : rvt.second )
					{
						Accept accept = { &*itAccept, &rrgWay, false };
						rgAccepts.push_back( accept );
					}
				}
				if ( m_posTrace )
				{
					*m_posTrace << "position " << pos << ":";
					for ( Accept const & rAccept : rgAccepts )
					{
						*m_posTrace << " " << rAccept.m_pvtAccept->first << "[" << ( ( e_aatAntiAccepting == rAccept.m_pvtAccept->second.m_eaatType ) ? "anti " : "accept " )
							<< rAccept.m_pvtAccept->second.m_aiAction << "](";
						for ( TagEvent const & rev : *rAccept.m_prgEvents )
							*m_posTrace << rev.m_nTrigger << "@" << rev.m_pos << " ";
						*m_posTrace << ")";
					}
					*m_posTrace << "\n";
				}
				bool fAnyExcluded = false;
				for ( Accept const & rAnti : rgAccepts )
				{
					if ( e_aatAntiAccepting != rAnti.m_pvtAccept->second.m_eaatType )
						continue;
					for ( Accept & rAccept : rgAccepts )
					{
						if ( ( e_aatAntiAccepting != rAccept.m_pvtAccept->second.m_eaatType ) &&
								 ( rAccept.m_pvtAccept->second.m_aiAction > rAnti.m_pvtAccept->second.m_aiAction ) &&
								 FExtends( *rAnti.m_prgEvents, *rAccept.m_prgEvents ) )
							rAccept.m_fExcluded = fAnyExcluded = true;
					}
				}
				const Accept * pAcceptToken = nullptr;
				for ( Accept const & rAccept : rgAccepts )
				{
					if ( ( e_aatAntiAccepting != rAccept.m_pvtAccept->second.m_eaatType ) && !rAccept.m_fExcluded &&
							 ( !pAcceptToken || ( rAccept.m_pvtAccept->second.m_aiAction < pAcceptToken->m_pvtAccept->second.m_aiAction ) ) )
						pAcceptToken = &rAccept;
				}
				if ( pAcceptToken )
				{
					tmLast = TagMatch();
					tmLast.m_nToken = !pAcceptToken->m_pvtAccept->second.m_pSdpAction ? s_knMatchNoToken : (int)( *pAcceptToken->m_pvtAccept->second.m_pSdpAction )->VGetTokenId();
					tmLast.m_nLength = pos;
					for ( TagEvent const & rev : *pAcceptToken->m_prgEvents )
					{
						TagEvent ev = { (int)m_rgtidTriggers[ (size_t)rev.m_nTrigger ], rev.m_pos };
						tmLast.m_rgEvents.push_back( ev );
					}
				}
				else
				if ( fAnyExcluded )
					tmLast = TagMatch(); // Explicitly excluded: there is no token.
			}//EB
			if ( pos == knLen )
				break;
			const _TyRangeEl kreCur = (_TyRangeEl)(unsigned char)_psz[ pos ];
			_TyMapStates mapNext;
			for ( auto const & rvt : mapCur )
			{
				_TyGraph::_TyLinkPosIterConst lpi( m_nfa.PGNGetNode( rvt.first )->PPGLChildHead() );
				for ( ; !lpi.FIsLast(); lpi.NextChild() )
				{
					const _TyRange & rrng = *lpi;
					if ( !!rrng.first && ( rrng.first <= kreCur ) && ( rrng.second >= kreCur ) )
					{
						for ( TyRgTagEvents const & rrgWay : rvt.second )
							(void)FRelax( mapNext, lpi.PGNChild()->RElConst(), rrgWay, pos + 1 );
					}
				}
			}
			if ( mapNext.empty() )
				break;
			mapCur.swap( mapNext );
		}
		return tmLast;
	}
};

// The rules: each trigger appends its id and position to the log of triggers fired.
template < vtyTokenIdent t_ktid >
using TyT = TyTestTriggerLog< t_ktid >;

// tagregisters: rules whose triggers an untagged DFA cannot represent but for which each trigger is crossed at most
//	once and in a known order: the lexical analyzer keeps a position for each trigger in each register.
//	Token0: < a* <T1> a b >													The trigger must be crossed before the last 'a'.
//	Token1: < a* <T1> (a|b)* <T2> b* <T3> ]					Three triggers whose positions depend upon what follows.
//	Token2: x ( <T1> a | <T2> b ) y									Two triggers leave a state.
//	Token3: a+
//	Token4: a+ <T1> b+ <T2>													A trigger leaves an accepting state and a trigger ends a token.
//	Token5: spaces				Token6: b
inline _TyFinal GetTagRegistersRules()
{
	__REGEXP_OP_USING_NAMESPACE // The operators of the regular expressions - only within the rules.
	_TyFinal Rule0 = l(u8'<') * ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'a') * l(u8'b') * l(u8'>');
	_TyFinal Rule1 = l(u8'<') * ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * ~( l(u8'a') | l(u8'b') ) * t( TyT< s_ktidTrigger2 >() )
										* ~l(u8'b') * t( TyT< s_ktidTrigger3 >() ) * l(u8']');
	_TyFinal Rule2 = l(u8'x') * ( t( TyT< s_ktidTrigger1 >() ) * l(u8'a') | t( TyT< s_ktidTrigger2 >() ) * l(u8'b') ) * l(u8'y');
	_TyFinal Rule3 = ++l(u8'a');
	_TyFinal Rule4 = ++l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * ++l(u8'b') * t( TyT< s_ktidTrigger2 >() );
	_TyFinal Rule5 = ++l(u8' ');
	_TyFinal Rule6 = l(u8'b');
	Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
	Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
	Rule2.SetAction( TyTestToken< s_ktidToken2 >() );
	Rule3.SetAction( TyTestToken< s_ktidToken3 >() );
	Rule4.SetAction( TyTestToken< s_ktidToken4 >() );
	Rule5.SetAction( TyTestToken< s_ktidToken5 >() );
	Rule6.SetAction( TyTestToken< s_ktidToken6 >() );
	_TyFinal All( Rule0 );
	All.AddRule( Rule1 );
	All.AddRule( Rule2 );
	All.AddRule( Rule3 );
	All.AddRule( Rule4 );
	All.AddRule( Rule5 );
	All.AddRule( Rule6 );
	return All;
}

// taghistory: rules whose triggers are within loops, or are crossed in different orders: the lexical analyzer keeps
//	the history of the triggers crossed.
//	Token0: ( a* <T1> a b )+ c											The position of each crossing depends upon what follows.
//	Token1: < ( (a|b)+ <T2> | & a ; )+ >						As the XML CharData: the trigger is crossed after each run of characters.
//	Token2: [ ( a <T1> | b <T2> )* ]								The triggers are crossed in any order, any number of times.
//	Token3: spaces				Token4: a single 'a', 'b' or 'c'.
inline _TyFinal GetTagHistoryRules()
{
	__REGEXP_OP_USING_NAMESPACE
	_TyFinal Rule0 = ++( ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'a') * l(u8'b') ) * l(u8'c');
	_TyFinal Rule1 = l(u8'<') * ++( ++( l(u8'a') | l(u8'b') ) * t( TyT< s_ktidTrigger2 >() ) | l(u8'&') * l(u8'a') * l(u8';') ) * l(u8'>');
	_TyFinal Rule2 = l(u8'[') * ~( l(u8'a') * t( TyT< s_ktidTrigger1 >() ) | l(u8'b') * t( TyT< s_ktidTrigger2 >() ) ) * l(u8']');
	_TyFinal Rule3 = ++l(u8' ');
	_TyFinal Rule4 = l(u8'a') | l(u8'b') | l(u8'c');
	Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
	Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
	Rule2.SetAction( TyTestToken< s_ktidToken2 >() );
	Rule3.SetAction( TyTestToken< s_ktidToken3 >() );
	Rule4.SetAction( TyTestToken< s_ktidToken4 >() );
	_TyFinal All( Rule0 );
	All.AddRule( Rule1 );
	All.AddRule( Rule2 );
	All.AddRule( Rule3 );
	All.AddRule( Rule4 );
	return All;
}

// xmlish: rules in the manner of an XML lexical analyzer - pattern exclusion, "completed by" and triggers within
//	loops - for which an untagged DFA suffices. We generate lexical analyzers from both an untagged and a tagged DFA
//	for these rules: they must find the same tokens and fire the same triggers at the same positions.
//	Token0: a tag: < <T1> name <T2> ( space <T3> name <T4> = " ( <T1> characters <T2> | & name ; )* " )* >
//	Token1: character data which doesn't contain "]]>" and references: ( characters <T3> | & name ; )+
//	Token2: a comment: <!-- up to the first -->
inline _TyFinal GetXmlishRules()
{
	__REGEXP_OP_USING_NAMESPACE
	_TyFinal Name = ++lr(u8'a',u8'c');
	_TyFinal Reference = l(u8'&') * Name * l(u8';');
	_TyFinal AttrValue = l(u8'"') * ~( t( TyT< s_ktidTrigger1 >() ) * ++lnot(u8"&<\"") * t( TyT< s_ktidTrigger2 >() ) | Reference ) * l(u8'"');
	_TyFinal Tag = l(u8'<') * t( TyT< s_ktidTrigger1 >() ) * Name * t( TyT< s_ktidTrigger2 >() )
									* ~( l(u8' ') * t( TyT< s_ktidTrigger3 >() ) * Name * t( TyT< s_ktidTrigger4 >() ) * l(u8'=') * AttrValue ) * l(u8'>');
	_TyFinal CharData = ++lnot(u8"<&") - ( ~lnot(u8"<&") * ls(u8"]]>") * ~lnot(u8"<&") );
	_TyFinal CharDataAndReferences = ++( CharData * t( TyT< s_ktidTrigger3 >() ) | Reference );
	_TyFinal Comment = ls(u8"<!--") * ( ~lnot(u8"&") + ls(u8"-->") );
	Tag.SetAction( TyTestToken< s_ktidToken0 >() );
	CharDataAndReferences.SetAction( TyTestToken< s_ktidToken1 >() );
	Comment.SetAction( TyTestToken< s_ktidToken2 >() );
	_TyFinal All( Tag );
	All.AddRule( CharDataAndReferences );
	All.AddRule( Comment );
	return All;
}

} // namespace ns_lexangtest

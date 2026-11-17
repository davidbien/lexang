
//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangtagexclusion.cpp
// Worked examples of pattern exclusion ( operator - ) combined with triggers in a tagged DFA - see _l_dfatag.h.
// For each example we print, for each input, the result of:
//	1) The reference of lexangtest_tag.h: a direct simulation of the NFA that knows nothing of registers.
//	2) The tagged DFA ( optimized ), walked by performing its tag operations.
//	3) The DFA for the same rules with the triggers removed: what exclusion means without triggers.
// Each result is checked against the result written down here, so the examples stay true: the program fails ( and
//	ctest with it ) if the behaviour changes. When you change the semantics on purpose, update the expectations and
//	the commentary below together.
//
// ---------------------------------------------------------------------------------------------------------------
// The problem
// ---------------------------------------------------------------------------------------------------------------
// Exclusion without triggers: A - B builds B as a side branch of the NFA which ends in a dead end, an
//	"anti-accepting" state. When the input read so far reaches the end of B at the same position as the end of a
//	rule, that rule - and any rule with a higher action id, i.e. added later - gives no token at that position.
//	Exclusion is thus decided at the position at which a token would end. B may be matched wherever A begins,
//	including at each iteration of a loop which contains A - B.
// With triggers a token can usually be matched in more than one "way": each way is the sequence of triggers it
//	crossed and the positions at which it crossed them. The match of an excluded pattern also has a way: the
//	triggers crossed before B began and within it. So which of the ways of matching the token does an excluded
//	match cancel?
//
// ---------------------------------------------------------------------------------------------------------------
// The solution chosen
// ---------------------------------------------------------------------------------------------------------------
// 1) An excluded match cancels only the ways of matching which extend its own way: those that crossed the same
//		triggers at the same positions ( and then perhaps more ). An excluded match which crossed no triggers is
//		extended by every way, so then exclusion is exactly what it is without triggers.
// 2) Precedence first, then exclusion: of the ways which reach a rule's accepting state only that of highest
//		precedence is kept ( the way which crosses triggers later wins - see _l_dfatag.h ) and exclusion is then
//		applied to that way. When that way is cancelled the rule gives no token, even if a way of lower precedence
//		would not have been cancelled ( examples E and E2 ).
// 3) If a rule remains the one with the lowest action id gives the token; otherwise the position is anti-accepting
//		if a rule was cancelled.
// Why: this is what the lexical analyzer does with an untagged DFA for the rules for which an untagged DFA suffices.
//	The untagged lexical analyzer takes a trigger only when there is no transition on the next character, so the ways
//	of matching are separated by the triggers - an excluded pattern reached after crossing a trigger is not in the
//	same DFA state as the ways which didn't cross it. Keeping the same semantics makes the choice between an untagged
//	and a tagged DFA, which _l_create_dfa() makes automatically, invisible to the user of the rules.
//
// The alternative considered - and not chosen for now - is "exclusion first, then precedence": cancel the ways
//	which extend an excluded match and then choose the best way remaining. Then the triggers would only decide which
//	positions are reported, never whether there is a token ( example E would give a token ). But the tagged DFA would
//	have to keep the ways of lower precedence for every accepting NFA state - more registers and more DFA states -
//	and it would disagree with the untagged lexical analyzer ( example A ).
//
// A consequence for the construction: normally a way of lower precedence to an NFA state is dropped. But while within
//	an excluded pattern the ways of lower precedence matter: in example A, reading "aba...", the way which began a new
//	word at position 2 has lower precedence yet if that word turns out to be "ab" its way, [T1@2], is what decides
//	what is cancelled. So for the NFA states from which only excluded patterns can be matched all ways are kept except:
//	a) a way which no remaining way extends - it can cancel nothing - and
//	b) a way which extends another way kept for the same NFA state - the shorter one cancels at least as much.
//	Without these the DFA for character data minus "]]>" within a loop of triggers never stops growing.
//
// Notation below: T1@n means that trigger T1 was crossed after n characters, i.e. before the character at index n.

#include "lexangtest_gen.h"
#include "lexangtest_tag.h"
#include <cstdio>
#include <sstream>

std::string g_strProgramName;

namespace ns_lexangtest
{
__REGEXP_OP_USING_NAMESPACE

// A DFA for a set of rules, matched by walking its graph. For a tagged DFA we perform its tag operations as the
//	lexical analyzer does and report the triggers of the way in which the token was matched. Returns the longest
//	match: an anti-accepting state after an accepting one means no token.
struct ExclusionDfa
{
	typedef _TyDfa::_TyGraph _TyGraph;
	typedef _TyDfa::_TyGraphNode _TyGraphNode;
	_TyDfa m_dfa;
	_TyDfaCtxt m_dctxt;
	ExclusionDfa( _TyFinal const & _rFinal, ETriggerDfaKind _etdk )
		: m_dctxt( m_dfa )
	{
		(void)GenDfa( _rFinal, m_dfa, m_dctxt, true, false, 0, _etdk );
		m_dfa._CreateRangeLookup();
	}
	TagMatch Match( const char * _psz )
	{
		TagMatch tmLast;
		bool fAntiAccepting = false;
		const size_t knLen = strlen( _psz );
		const _TyDfa::_TyAlphaIndex kaiFirstOpList = (_TyDfa::_TyAlphaIndex)( m_dfa.AlphabetSize() - m_dfa.m_nTriggers );
		std::vector< TyRgTagEvents > rgRegisters( m_dfa.m_nTagRegisters + 1 );
		auto fnRegister = [&]( unsigned short _nReg ) -> TyRgTagEvents &
		{
			return rgRegisters[ ( _TyDfa::s_knTagRegisterTemp == _nReg ) ? ( m_dfa.m_nTagRegisters - 1 ) : (size_t)_nReg ];
		};
		const _TyGraphNode * pgnCur = m_dctxt.m_pgnStart;
		for ( size_t pos = 0; ; ++pos )
		{
			const size_t kstCur = (size_t)pgnCur->RElConst();
			if ( m_dctxt.m_pssAccept->isbitset( kstCur ) )
			{
				for ( auto it = m_dctxt.m_partAccept.begin(); it != m_dctxt.m_partAccept.end(); ++it )
				{
					if ( !it->first.RObject().isbitset( kstCur ) )
						continue;
					const int keaat = (int)it->second.m_eaatType;
					if ( ( e_aatAccept != keaat ) && ( e_aatAntiAccepting != keaat ) )
						break;
					tmLast = TagMatch();
					fAntiAccepting = ( e_aatAntiAccepting == keaat );
					if ( !fAntiAccepting )
					{
						tmLast.m_nToken = !it->second.m_pSdpAction ? s_knMatchNoToken : (int)( *it->second.m_pSdpAction )->VGetTokenId();
						tmLast.m_nLength = pos;
						if ( m_dfa.m_fTagged )
						{
							// The register for the way in which the token was matched is register zero, then the triggers
							//	crossed at the end of the token.
							for ( TagEvent const & rev : rgRegisters[ 0 ] )
								tmLast.m_rgEvents.push_back( TagEvent{ (int)m_dfa.m_rgTags[ (size_t)rev.m_nTrigger ].m_tidTrigger, rev.m_pos } );
							if ( it->second.m_iTagFinish >= 0 )
								for ( int iTag : m_dfa.m_rgTagFinishes[ (size_t)it->second.m_iTagFinish ] )
									tmLast.m_rgEvents.push_back( TagEvent{ (int)m_dfa.m_rgTags[ (size_t)iTag ].m_tidTrigger, pos } );
						}
					}
					break;
				}
			}
			if ( pos == knLen )
				break;
			const _TyDfa::_TyAlphaIndex ai = m_dfa._LookupAlphaSetNum( (_TyDfa::_TyRangeEl)(unsigned char)_psz[ pos ] );
			if ( ai < 0 )
				break;
			_TyGraph::_TyLinkPosIterConst lpi( pgnCur->PPGLChildHead() );
			for ( ; !lpi.FIsLast() && ( *lpi != ai ); lpi.NextChild() )
				;
			if ( lpi.FIsLast() )
				break;
			pgnCur = lpi.PGNChild();
			if ( m_dfa.m_fTagged )
			{
				// A tag operation state has a single transition: on the pseudo-trigger for its list of operations.
				_TyGraph::_TyLinkPosIterConst lpiOps( pgnCur->PPGLChildHead() );
				if ( !lpiOps.FIsLast() && ( *lpiOps >= kaiFirstOpList ) )
				{
					for ( _TyDfa::_TyTagOp const & rop : m_dfa.m_rgTagOpLists[ (size_t)( *lpiOps - kaiFirstOpList ) ] )
					{
						if ( rop.m_iTag < 0 )
						{
							TyRgTagEvents rgCopy( fnRegister( rop.m_nSrc ) );
							fnRegister( rop.m_nDst ).swap( rgCopy );
						}
						else
							fnRegister( rop.m_nDst ).push_back( TagEvent{ rop.m_iTag, pos } ); // Crossed before the character.
					}
					pgnCur = lpiOps.PGNChild();
				}
			}
		}
		if ( fAntiAccepting )
			tmLast = TagMatch();
		return tmLast;
	}
};

// "no token" or e.g. "Token0 len 4 T1@4".
std::string StrMatch( TagMatch const & _r )
{
	if ( _r.m_nToken < 0 )
		return "no token";
	std::ostringstream os;
	os << "Token" << ( _r.m_nToken - s_ktidToken0 ) << " len " << _r.m_nLength;
	for ( TagEvent const & rev : _r.m_rgEvents )
		os << " T" << ( rev.m_nTrigger - s_ktidTrigger1 + 1 ) << "@" << rev.m_pos;
	return os.str();
}

// An input and the results we expect: with the triggers, and for the same rules without the triggers.
struct ExclusionCase
{
	const char * m_pszInput;
	const char * m_pszWith;
	const char * m_pszWithout;
};

size_t g_nFailures = 0;

// Match each input with the reference, the tagged DFA and the DFA without triggers and check the results.
void Show( const char * _pszTitle, _TyFinal const & _rWith, _TyFinal const & _rWithout, std::initializer_list< ExclusionCase > _il )
{
	printf( "=== %s\n", _pszTitle );
	TagReference ref( _rWith );
	ExclusionDfa dfaTagged( _rWith, etdkTagged );
	ExclusionDfa dfaWithout( _rWithout, etdkUntagged );
	for ( ExclusionCase const & rec : _il )
	{
		const std::string strRef = StrMatch( ref.Match( rec.m_pszInput ) );
		const std::string strTagged = StrMatch( dfaTagged.Match( rec.m_pszInput ) );
		const std::string strWithout = StrMatch( dfaWithout.Match( rec.m_pszInput ) );
		const bool kfOk = ( strRef == rec.m_pszWith ) && ( strTagged == rec.m_pszWith ) && ( strWithout == rec.m_pszWithout );
		printf( "  %-8s with triggers: %-20s without: %-14s%s\n", rec.m_pszInput, strTagged.c_str(), strWithout.c_str(), kfOk ? "" : "  *** UNEXPECTED" );
		if ( !kfOk )
		{
			printf( "    expected with triggers [%s] without [%s]; the reference gave [%s]\n", rec.m_pszWith, rec.m_pszWithout, strRef.c_str() );
			++g_nFailures;
		}
	}
}

#define T1 t( TyT< s_ktidTrigger1 >() )
#define T2 t( TyT< s_ktidTrigger2 >() )

void Run()
{
	// A: ( ( [ab]+ - "ab" ) <T1> )+ - a list of words each followed by T1, no word may be "ab".
	// For "abab" the ways which reach the end include: one word "abab" [T1@4]; "ab" + "ab" [T1@2, T1@4];
	//	"a" + "bab" [T1@1, T1@4]. The "ab" which ends at 4 began a second word at 2, so to be there T1@2 was crossed:
	//	the excluded match's way is [T1@2]. Precedence chooses one word, [T1@4], which doesn't begin with [T1@2], so
	//	the token stands. Without the trigger an excluded "ab" ends at 4 and nothing distinguishes the ways: no token.
	//	The trigger turns "some segment that ends here is ab" into "the segment we actually divided at is ab".
	// For "ab" the excluded match begins at 0 having crossed nothing: every way extends it - no token, as without.
	{
		_TyFinal R = ++( ( ++lr(u8'a',u8'b') - ls(u8"ab") ) * T1 );
		_TyFinal P = ++( ( ++lr(u8'a',u8'b') - ls(u8"ab") ) );
		R.SetAction( TyTestToken< s_ktidToken0 >() );
		P.SetAction( TyTestToken< s_ktidToken0 >() );
		Show( "A: ( ( [ab]+ - \"ab\" ) <T1> )+", R, P, {
			{ "ab", "no token", "no token" },
			{ "aab", "Token0 len 3 T1@3", "no token" },
			{ "abb", "Token0 len 3 T1@3", "Token0 len 3" },
			{ "abab", "Token0 len 4 T1@4", "no token" },
			{ "aabab", "Token0 len 5 T1@5", "no token" },
			{ "ba", "Token0 len 2 T1@2", "Token0 len 2" },
			{ "b", "Token0 len 1 T1@1", "Token0 len 1" } } );
	}
	// B: the same with the trigger before each word ( a rule may not begin with a trigger, hence the 'c' ).
	// For "cab" the excluded "ab" has the way [T1@1]. One word is [T1@1] and "a" + "b" is [T1@1, T1@2]: both begin
	//	with [T1@1], both are cancelled - no token. Moving the trigger changes which ways "began the same way".
	{
		_TyFinal R = l(u8'c') * ++( T1 * ( ++lr(u8'a',u8'b') - ls(u8"ab") ) );
		_TyFinal P = l(u8'c') * ++( ++lr(u8'a',u8'b') - ls(u8"ab") );
		R.SetAction( TyTestToken< s_ktidToken0 >() );
		P.SetAction( TyTestToken< s_ktidToken0 >() );
		Show( "B: c ( <T1> ( [ab]+ - \"ab\" ) )+", R, P, {
			{ "cab", "no token", "no token" },
			{ "caab", "Token0 len 4 T1@1", "no token" },
			{ "cabb", "Token0 len 4 T1@1", "Token0 len 4" },
			{ "cabab", "Token0 len 5 T1@1", "no token" },
			{ "caabab", "Token0 len 6 T1@1", "no token" },
			{ "cba", "Token0 len 3 T1@1", "Token0 len 3" } } );
	}
	// C and D: across rules. An excluded pattern also cancels the later rules that end where it does.
	// C: R0's excluded "bb" has the way [T1@1]. R1 never crossed T1 so R1 is untouched: "abb" is Token1. A trigger
	//	crossed before an excluded pattern scopes the exclusion to its own rule.
	// D: R0's excluded "bb" crossed nothing - it cancels R1 just as it does without triggers: no token.
	{
		_TyFinal R0 = l(u8'a') * T1 * ( ++l(u8'b') - ls(u8"bb") );
		_TyFinal R1 = ls(u8"abb") * T2;
		_TyFinal P0 = l(u8'a') * ( ++l(u8'b') - ls(u8"bb") );
		_TyFinal P1 = ls(u8"abb");
		R0.SetAction( TyTestToken< s_ktidToken0 >() );
		P0.SetAction( TyTestToken< s_ktidToken0 >() );
		R1.SetAction( TyTestToken< s_ktidToken1 >() );
		P1.SetAction( TyTestToken< s_ktidToken1 >() );
		_TyFinal All( R0 );
		All.AddRule( R1 );
		_TyFinal AllP( P0 );
		AllP.AddRule( P1 );
		Show( "C: R0 = a <T1> ( b+ - \"bb\" ) ; R1 = \"abb\" <T2>", All, AllP, {
			{ "ab", "Token0 len 2 T1@1", "Token0 len 2" },
			{ "abb", "Token1 len 3 T2@3", "no token" },
			{ "abbb", "Token0 len 4 T1@1", "Token0 len 4" } } );
	}
	{
		_TyFinal R0 = l(u8'a') * ( ++l(u8'b') - ls(u8"bb") ) * T1;
		_TyFinal R1 = ls(u8"abb") * T2;
		_TyFinal P0 = l(u8'a') * ( ++l(u8'b') - ls(u8"bb") );
		_TyFinal P1 = ls(u8"abb");
		R0.SetAction( TyTestToken< s_ktidToken0 >() );
		P0.SetAction( TyTestToken< s_ktidToken0 >() );
		R1.SetAction( TyTestToken< s_ktidToken1 >() );
		P1.SetAction( TyTestToken< s_ktidToken1 >() );
		_TyFinal All( R0 );
		All.AddRule( R1 );
		_TyFinal AllP( P0 );
		AllP.AddRule( P1 );
		Show( "D: R0 = a ( b+ - \"bb\" ) <T1> ; R1 = \"abb\" <T2>", All, AllP, {
			{ "ab", "Token0 len 2 T1@2", "Token0 len 2" },
			{ "abb", "no token", "no token" },
			{ "abbb", "Token0 len 4 T1@4", "Token0 len 4" } } );
	}
	// E and E2: precedence first, then exclusion - see (2) above. The two rules match the same strings; only where
	//	the triggers are differs.
	// E: the first alternative crosses T1@2, the second T2@1. Crossing later wins so the first is chosen; the excluded
	//	"b" cancels it and the second is never considered: no token.
	// E2: now the second alternative crosses later ( T2@2 ), wins and isn't excluded: a token.
	// With "exclusion first, then precedence" E would give "Token0 len 3 T2@1".
	{
		_TyFinal R = l(u8'c') * ( l(u8'a') * T1 * ( l(u8'b') - l(u8'b') ) | T2 * l(u8'a') * l(u8'b') );
		_TyFinal R2 = l(u8'c') * ( T1 * l(u8'a') * ( l(u8'b') - l(u8'b') ) | l(u8'a') * T2 * l(u8'b') );
		_TyFinal P = l(u8'c') * ( l(u8'a') * ( l(u8'b') - l(u8'b') ) | l(u8'a') * l(u8'b') );
		R.SetAction( TyTestToken< s_ktidToken0 >() );
		R2.SetAction( TyTestToken< s_ktidToken0 >() );
		P.SetAction( TyTestToken< s_ktidToken0 >() );
		Show( "E: c ( a <T1> ( b - b ) | <T2> a b )", R, P, {
			{ "cab", "no token", "no token" } } );
		Show( "E2: c ( <T1> a ( b - b ) | a <T2> b )", R2, P, {
			{ "cab", "Token0 len 3 T2@2", "no token" } } );
	}
}

} // namespace ns_lexangtest

int main( int _argc, char ** _argv )
{
	(void)_argc;
	g_strProgramName = *_argv;
	try
	{
		ns_lexangtest::Run();
	}
	catch ( std::exception const & rexc )
	{
		fprintf( stderr, "%s: *** Exception: [%s]\n", g_strProgramName.c_str(), rexc.what() );
		return 2;
	}
	if ( ns_lexangtest::g_nFailures )
	{
		fprintf( stderr, "%s: %zu results were not as expected.\n", g_strProgramName.c_str(), ns_lexangtest::g_nFailures );
		return 1;
	}
	printf( "All results as expected.\n" );
	return 0;
}

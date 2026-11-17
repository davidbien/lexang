
//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangdfatest.cpp
// Unit tests of the creation of DFAs from regular expressions.
// For each regular expression (or set of rules) we create:
//	1) the DFA produced directly from the NFA ( no dead state, not optimized ) and
//	2) the optimized DFA
// and then match strings against both by walking the DFA's graph. This tests each of the regular expression operators,
//	the creation of the DFA from the NFA and the optimization of the DFA, without generating a lexical analyzer.

#include "lexangtest_gen.h"
#include "lexangtest_tag.h"
#include <random>
#include <sstream>
#include <functional>
#include <map>
#include "gtest/gtest.h"

std::string g_strProgramName;

namespace ns_lexangtest
{
__REGEXP_OP_USING_NAMESPACE

// The result of matching a string against a DFA: s_knNoMatch, s_knMatchNoToken ( see lexangtest_tag.h ) or:
static const int s_knAntiAccept = -2; // The string ends in an anti-accepting state: it was explicitly excluded.
// Otherwise the result is the token id of the rule which matched.

// A DFA created from a set of rules, either optimized or not.
struct TestDfa
{
	_TyDfa m_dfa;
	_TyDfaCtxt m_dctxt;
	TestDfa( _TyFinal const & _rFinal, bool _fOptimize, uint32_t _grfNFACreationOptions = 0 )
		: m_dctxt( m_dfa )
	{
		// We don't compress the transitions: we then have a single element of the alphabet on each transition.
		GenDfa( _rFinal, m_dfa, m_dctxt, _fOptimize, false, _grfNFACreationOptions );
		m_dfa._CreateRangeLookup();
	}
	size_t NStates() const
	{
		return (size_t)m_dfa.NStates();
	}
	// Match the entire string returning the result as described above.
	int Match( const char * _psz )
	{
		typedef _TyDfa::_TyGraph _TyGraph;
		typedef _TyDfa::_TyGraphNode _TyGraphNode;
		const _TyGraphNode * pgnCur = m_dctxt.m_pgnStart;
		for ( ; !!*_psz; ++_psz )
		{
			const _TyDfa::_TyAlphaIndex ai = m_dfa._LookupAlphaSetNum( (_TyDfa::_TyRangeEl)(unsigned char)*_psz );
			if ( ai < 0 )
				return s_knNoMatch; // Not in the alphabet.
			_TyGraph::_TyLinkPosIterConst	lpi( pgnCur->PPGLChildHead() );
			for ( ; !lpi.FIsLast() && ( *lpi != ai ); lpi.NextChild() )
				;
			if ( lpi.FIsLast() )
				return s_knNoMatch;
			pgnCur = lpi.PGNChild();
		}
		const size_t stState = (size_t)pgnCur->RElConst();
		if ( !m_dctxt.m_pssAccept->isbitset( stState ) )
			return s_knNoMatch;
		// Find the accept action for this state:
		for ( auto it = m_dctxt.m_partAccept.begin(); it != m_dctxt.m_partAccept.end(); ++it )
		{
			if ( it->first.RObject().isbitset( stState ) )
			{
				const int eaatType = (int)it->second.m_eaatType & ~(int)e_aatTrigger;
				if ( e_aatAntiAccepting == eaatType )
					return s_knAntiAccept;
				// A lookahead accept state is where the token of a lookahead rule ends - the rule itself has yet to match.
				if ( e_aatLookaheadAccept == eaatType )
					return s_knNoMatch;
				return !it->second.m_pSdpAction ? s_knMatchNoToken : (int)(*it->second.m_pSdpAction)->VGetTokenId();
			}
		}
		ADD_FAILURE() << "Accepting state [" << stState << "] is not in the accept partition.";
		return s_knNoMatch;
	}
};

// Both DFAs for a set of rules.
struct TestDfaPair
{
	TestDfa m_dfaUnoptimized;
	TestDfa m_dfaOptimized;
	TestDfaPair( _TyFinal const & _rFinal, uint32_t _grfNFACreationOptions = 0 )
		: m_dfaUnoptimized( _rFinal, false, _grfNFACreationOptions ),
			m_dfaOptimized( _rFinal, true, _grfNFACreationOptions )
	{
	}
	// Match the string against both DFAs - they must agree. Returns the result.
	int Match( const char * _psz )
	{
		const int nUnoptimized = m_dfaUnoptimized.Match( _psz );
		const int nOptimized = m_dfaOptimized.Match( _psz );
		EXPECT_EQ( nUnoptimized, nOptimized ) << "The unoptimized and optimized DFAs disagree on [" << _psz << "].";
		return nOptimized;
	}
	void ExpectMatches( std::initializer_list< const char * > _ilMatch, int _nExpect = s_knMatchNoToken )
	{
		for ( const char * psz : _ilMatch )
			EXPECT_EQ( Match( psz ), _nExpect ) << "String [" << psz << "].";
	}
	void ExpectNoMatches( std::initializer_list< const char * > _ilNoMatch )
	{
		for ( const char * psz : _ilNoMatch )
			EXPECT_LT( Match( psz ), 0 ) << "String [" << psz << "] should not be matched.";
	}
	// Match random strings over the given alphabet against both DFAs - they must agree.
	void ExpectRandomStringsAgree( const char * _pszAlphabet, size_t _nStrings, size_t _nMaxLen, unsigned _uSeed )
	{
		std::mt19937 rng( _uSeed );
		const size_t nAlpha = strlen( _pszAlphabet );
		size_t nMatched = 0;
		for ( size_t n = 0; n < _nStrings; ++n )
		{
			std::string str( rng() % ( _nMaxLen + 1 ), ' ' );
			for ( char & rc : str )
				rc = _pszAlphabet[ rng() % nAlpha ];
			const int nUnoptimized = m_dfaUnoptimized.Match( str.c_str() );
			const int nOptimized = m_dfaOptimized.Match( str.c_str() );
			if ( nUnoptimized != nOptimized )
			{
				ADD_FAILURE() << "The unoptimized [" << nUnoptimized << "] and optimized [" << nOptimized << "] DFAs disagree on [" << str << "].";
				return;
			}
			if ( nOptimized >= 0 )
				++nMatched;
		}
		// The test isn't testing much if nothing (or everything) matched.
		EXPECT_GT( nMatched, size_t(0) );
		EXPECT_LT( nMatched, _nStrings );
	}
};

TEST( LexangDfaOperators, Literal )
{
	TestDfaPair dfas( _TyFinal( l(u8'a') ) );
	dfas.ExpectMatches( { "a" } );
	dfas.ExpectNoMatches( { "", "b", "aa", "ab" } );
}

TEST( LexangDfaOperators, LiteralString )
{
	TestDfaPair dfas( _TyFinal( ls(u8"abc") ) );
	dfas.ExpectMatches( { "abc" } );
	dfas.ExpectNoMatches( { "", "a", "ab", "abcd", "abd", "xabc" } );
}

TEST( LexangDfaOperators, LiteralRange )
{
	TestDfaPair dfas( _TyFinal( lr(u8'b',u8'e') ) );
	dfas.ExpectMatches( { "b", "c", "d", "e" } );
	dfas.ExpectNoMatches( { "", "a", "f", "bc" } );
}

TEST( LexangDfaOperators, LiteralAnyInSet )
{
	TestDfaPair dfas( _TyFinal( lanyof(u8"xaz") ) );
	dfas.ExpectMatches( { "x", "a", "z" } );
	dfas.ExpectNoMatches( { "", "b", "y", "xa" } );
}

TEST( LexangDfaOperators, LiteralNotSet )
{
	TestDfaPair dfas( _TyFinal( lnot(u8"abc") ) );
	dfas.ExpectMatches( { "d", "z", "A", " ", "0" } );
	dfas.ExpectNoMatches( { "", "a", "b", "c", "dd" } );
}

TEST( LexangDfaOperators, Follows )
{
	TestDfaPair dfas( _TyFinal( l(u8'a') * l(u8'b') * lr(u8'0',u8'9') ) );
	dfas.ExpectMatches( { "ab0", "ab5", "ab9" } );
	dfas.ExpectNoMatches( { "", "a", "ab", "ab00", "ba0", "abx" } );
}

TEST( LexangDfaOperators, Or )
{
	TestDfaPair dfas( _TyFinal( ls(u8"ab") | ls(u8"cd") | l(u8'e') ) );
	dfas.ExpectMatches( { "ab", "cd", "e" } );
	dfas.ExpectNoMatches( { "", "a", "ad", "cb", "abcd", "ee" } );
}

TEST( LexangDfaOperators, ZeroOrMore )
{
	TestDfaPair dfas( _TyFinal( ~ls(u8"ab") ) );
	dfas.ExpectMatches( { "", "ab", "abab", "ababab" } );
	dfas.ExpectNoMatches( { "a", "b", "aba", "abb", "ba" } );
}

TEST( LexangDfaOperators, OneOrMore )
{
	TestDfaPair dfas( _TyFinal( ++ls(u8"ab") ) );
	dfas.ExpectMatches( { "ab", "abab", "ababab" } );
	dfas.ExpectNoMatches( { "", "a", "b", "aba", "abb", "ba" } );
}

TEST( LexangDfaOperators, ZeroOrOne )
{
	TestDfaPair dfas( _TyFinal( l(u8'x') * --ls(u8"ab") * l(u8'y') ) );
	dfas.ExpectMatches( { "xy", "xaby" } );
	dfas.ExpectNoMatches( { "", "x", "xay", "xababy", "xab" } );
}

TEST( LexangDfaOperators, NestedClosures )
{
	// ( a | b* c )+ d?
	TestDfaPair dfas( _TyFinal( ++( l(u8'a') | ~l(u8'b') * l(u8'c') ) * --l(u8'd') ) );
	dfas.ExpectMatches( { "a", "c", "bc", "bbbc", "aa", "abc", "bcabbc", "ad", "bcd", "cacad" } );
	dfas.ExpectNoMatches( { "", "b", "d", "ab", "add", "bd", "da" } );
	dfas.ExpectRandomStringsAgree( "abcd", 4000, 8, 1 );
}

// Excludes: r1 - r2 matches what r1 matches except for what r2 matches.
TEST( LexangDfaOperators, Excludes )
{
	// Any number of 'a's followed by any number of 'b's - but not "ab".
	TestDfaPair dfas( _TyFinal( ( ++l(u8'a') * ~l(u8'b') ) - ls(u8"ab") ) );
	dfas.ExpectMatches( { "a", "aa", "aab", "abb", "aabb" } );
	dfas.ExpectNoMatches( { "", "b", "ab", "ba" } );
	dfas.ExpectRandomStringsAgree( "ab", 2000, 6, 2 );
}

TEST( LexangDfaOperators, ExcludesKeyword )
{
	// A name which is not "xml" in any case - as the XML PITarget production.
	_TyFinal Name = ++lr(u8'a',u8'z') | ++lr(u8'A',u8'Z');
	_TyFinal NameMixed = ++( lr(u8'a',u8'z') | lr(u8'A',u8'Z') );
	TestDfaPair dfas( _TyFinal( NameMixed - ( ( l(u8'x') | l(u8'X') ) * ( l(u8'm') | l(u8'M') ) * ( l(u8'l') | l(u8'L') ) ) ) );
	dfas.ExpectMatches( { "x", "xm", "xmlx", "axml", "html", "XMLS" } );
	dfas.ExpectNoMatches( { "", "xml", "XML", "xMl", "Xml", "xml1" } );
	dfas.ExpectRandomStringsAgree( "xmlXML", 4000, 5, 3 );
}

// Completes: r1 + r2 matches a sequence matched by r1 up to and including the first occurrence of r2.
TEST( LexangDfaOperators, Completes )
{
	// As an XML comment: any characters up to the first "-->".
	TestDfaPair dfas( _TyFinal( ls(u8"<!--") * ( ~( lr(u8'a',u8'z') | l(u8'-') | l(u8'>') ) + ls(u8"-->") ) ) );
	dfas.ExpectMatches( { "<!---->", "<!--a-->", "<!--abc-->", "<!--a-b-->", "<!--a--b-->", "<!-->-->", "<!--->-->" } );
	dfas.ExpectNoMatches( { "", "<!--", "<!--a", "<!--a--", "<!--a-->b", "<!--a-->-->", "<!--a-->b-->" } );

	// Without the prefix so that random strings match often enough.
	TestDfaPair dfasNoPrefix( _TyFinal( ~( l(u8'a') | l(u8'-') | l(u8'>') ) + ls(u8"-->") ) );
	dfasNoPrefix.ExpectMatches( { "-->", "a-->", "--->", ">-->", "a-a--a-->", "->-->" } );
	dfasNoPrefix.ExpectNoMatches( { "", "--", "->", "-->a", "-->-->", "a-->-" } );
	dfasNoPrefix.ExpectRandomStringsAgree( "->a", 20000, 9, 4 );
}

TEST( LexangDfaOperators, CompletesWithVariableLengthEnd )
{
	// The completing expression needn't be a fixed string: digits up to the first "x" followed by one or more "y"s.
	TestDfaPair dfas( _TyFinal( ~( l(u8'0') | l(u8'x') | l(u8'y') ) + ( l(u8'x') * ++l(u8'y') ) ) );
	dfas.ExpectMatches( { "xy", "0xy", "00xy", "x0xy", "yxy", "xxy" } );
	dfas.ExpectNoMatches( { "", "x", "0x", "0y", "xy0", "xyxy" } );
	dfas.ExpectRandomStringsAgree( "0xy", 20000, 8, 7 );
}

// Several rules: the earliest rule wins when more than one matches the same string.
TEST( LexangDfaRules, RulePrecedence )
{
	_TyFinal KeywordIf = ls(u8"if");
	_TyFinal KeywordInt = ls(u8"int");
	_TyFinal Ident = lr(u8'a',u8'z') * ~( lr(u8'a',u8'z') | lr(u8'0',u8'9') );
	_TyFinal Number = ++lr(u8'0',u8'9');
	KeywordIf.SetAction( TyTestToken< s_ktidToken0 >() );
	KeywordInt.SetAction( TyTestToken< s_ktidToken1 >() );
	Ident.SetAction( TyTestToken< s_ktidToken2 >() );
	Number.SetAction( TyTestToken< s_ktidToken3 >() );
	_TyFinal All( KeywordIf );
	All.AddRule( KeywordInt );
	All.AddRule( Ident );
	All.AddRule( Number );
	TestDfaPair dfas( All );
	dfas.ExpectMatches( { "if" }, s_ktidToken0 );
	dfas.ExpectMatches( { "int" }, s_ktidToken1 );
	dfas.ExpectMatches( { "i", "in", "ifx", "inta", "x9", "i0" }, s_ktidToken2 );
	dfas.ExpectMatches( { "0", "123" }, s_ktidToken3 );
	dfas.ExpectNoMatches( { "", "9x", "if ", "-" } );
	dfas.ExpectRandomStringsAgree( "ifnt0", 4000, 5, 5 );
}

// The optimizer should produce the minimal DFA.
TEST( LexangDfaOptimize, MinimalNumberOfStates )
{
	{
		// (a|b)*abb - the example from the Dragon Book: the minimal DFA has 4 states.
		TestDfaPair dfas( _TyFinal( ~( l(u8'a') | l(u8'b') ) * ls(u8"abb") ) );
		EXPECT_EQ( dfas.m_dfaOptimized.NStates(), size_t(4) );
		EXPECT_GE( dfas.m_dfaUnoptimized.NStates(), size_t(4) );
		dfas.ExpectMatches( { "abb", "aabb", "babb", "abbabb", "ababb" } );
		dfas.ExpectNoMatches( { "", "ab", "abba", "bba" } );
		dfas.ExpectRandomStringsAgree( "ab", 4000, 10, 6 );
	}
	{
		// Redundant alternatives collapse: (a|a|a)(b|b) needs 3 states.
		TestDfaPair dfas( _TyFinal( ( l(u8'a') | l(u8'a') | l(u8'a') ) * ( l(u8'b') | l(u8'b') ) ) );
		EXPECT_EQ( dfas.m_dfaOptimized.NStates(), size_t(3) );
		dfas.ExpectMatches( { "ab" } );
		dfas.ExpectNoMatches( { "", "a", "b", "abb" } );
	}
	{
		// a(b|c)d | a(c|b)d : identical alternatives: 4 states.
		TestDfaPair dfas( _TyFinal( l(u8'a') * ( l(u8'b') | l(u8'c') ) * l(u8'd') | l(u8'a') * ( l(u8'c') | l(u8'b') ) * l(u8'd') ) );
		EXPECT_EQ( dfas.m_dfaOptimized.NStates(), size_t(4) );
		dfas.ExpectMatches( { "abd", "acd" } );
		dfas.ExpectNoMatches( { "", "ad", "abcd" } );
	}
}

// States which accept for different rules must not be merged by the optimizer.
TEST( LexangDfaOptimize, DifferentTokensAreNotMerged )
{
	_TyFinal A = l(u8'a') * l(u8'x');
	_TyFinal B = l(u8'b') * l(u8'x');
	A.SetAction( TyTestToken< s_ktidToken0 >() );
	B.SetAction( TyTestToken< s_ktidToken1 >() );
	_TyFinal All( A );
	All.AddRule( B );
	TestDfaPair dfas( All );
	dfas.ExpectMatches( { "ax" }, s_ktidToken0 );
	dfas.ExpectMatches( { "bx" }, s_ktidToken1 );
	dfas.ExpectNoMatches( { "", "a", "b", "x", "axx" } );
	// start, after 'a', after 'b', accept A, accept B.
	EXPECT_EQ( dfas.m_dfaOptimized.NStates(), size_t(5) );
}


TEST( LexangDfaOperators, Empty )
{
	// The empty expression matches only the empty string: a(|b)c.
	TestDfaPair dfas( _TyFinal( l(u8'a') * ( empty<_TyCTok>() | l(u8'b') ) * l(u8'c') ) );
	dfas.ExpectMatches( { "ac", "abc" } );
	dfas.ExpectNoMatches( { "", "a", "abbc", "c" } );
}

// Triggers can be ignored when creating the NFA: the rules then match as if the triggers were not present.
// This is used to reuse rules which contain triggers for matching alone.
TEST( LexangDfaOptions, IgnoreTriggers )
{
	_TyFinal Tag = l(u8'<') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * ++lr(u8'a',u8'z') 
									* t( TyTestTriggerString< s_ktidTriggerAEnd, s_ktidTriggerABegin >() ) * l(u8'>');
	TestDfaPair dfas( Tag, ( 1 << encoIgnoreTriggers ) );
	dfas.ExpectMatches( { "<a>", "<abc>" } );
	dfas.ExpectNoMatches( { "", "<>", "<abc", "abc>", "<a>>" } );
	// Without triggers the DFA is: start, after '<', in the name, accept.
	EXPECT_EQ( dfas.m_dfaOptimized.NStates(), size_t(4) );
}


// Lookahead: the DFA matches the token followed by the trailing context - finding where the token ends is the job of
//	the lexical analyzer and is tested in lexanglextest.cpp.
TEST( LexangDfaOperators, Lookahead )
{
	TestDfaPair dfas( _TyFinal( ++lr(u8'a',u8'z') / ls(u8"::") ) );
	dfas.ExpectMatches( { "a::", "abc::" } );
	dfas.ExpectNoMatches( { "", "a", "a:", "::", "a:::", "a::b" } );
}

// The token portion of a lookahead rule must not match the empty string.
TEST( LexangDfaOperators, LookaheadTokenMustNotBeEmpty )
{
	EXPECT_THROW( TestDfa( _TyFinal( ~l(u8'x') / ls(u8"xy") ), true ), regexp_lookahead_empty_token_exception );
	EXPECT_THROW( TestDfa( _TyFinal( --l(u8'x') / l(u8'y') ), true ), regexp_lookahead_empty_token_exception );
	EXPECT_NO_THROW( TestDfa( _TyFinal( ++l(u8'x') / ls(u8"xy") ), true ) );
	// The trailing context may match the empty string.
	EXPECT_NO_THROW( TestDfa( _TyFinal( ++l(u8'x') / ~l(u8'y') ), true ) );
}


// LogArray<> with the per-thread free list ( as used by _l_value<> and _l_data<> ): the first block of an array is reused
//	after the array is destroyed and growing to multiple blocks and shrinking back works as usual.
TEST( LogArrayFreeList, FirstBlockIsReusedAndArraysWork )
{
	typedef LogArray< std::string, 2, 4, true > TyLogArray;
	const void * pvFirstBlock = nullptr;
	{
		TyLogArray la;
		la.emplaceAtEnd( "a" );
		pvFirstBlock = &la[0];
	}
	{
		TyLogArray la;
		la.emplaceAtEnd( "b" );
		EXPECT_EQ( (const void *)&la[0], pvFirstBlock ); // The freed first block was reused.
	}
	// Grow well beyond a single block, shrink back to a single block, then clear - repeatedly.
	for ( size_t nRep = 0; nRep < 50; ++nRep )
	{
		TyLogArray la;
		const size_t knEls = 1 + nRep * 7;
		for ( size_t n = 0; n < knEls; ++n )
			la.emplaceAtEnd( std::to_string( n ) );
		ASSERT_EQ( la.NElements(), knEls );
		for ( size_t n = 0; n < knEls; ++n )
			ASSERT_EQ( la[n], std::to_string( n ) );
		if ( knEls > 3 )
		{
			la.SetSize( 3 );
			ASSERT_EQ( la.NElements(), size_t(3) );
			EXPECT_EQ( la[2], "2" );
			la.emplaceAtEnd( "x" );
			EXPECT_EQ( la[3], "x" );
		}
		TyLogArray laCopy( la );
		EXPECT_EQ( laCopy.NElements(), la.NElements() );
		// A copy to an array without the free list.
		LogArray< std::string, 2, 4 > laNoFreeList( la );
		EXPECT_EQ( laNoFreeList.NElements(), la.NElements() );
		if ( nRep % 2 )
			la.Clear();
	}
}


// Trigger ambiguity: the lexical analyzer consumes a character rather than crossing a trigger.
// This is what an untagged DFA cannot deal with - GenDfa() creates a tagged DFA for such rules unless told not to.
static std::vector< _TyDfaCtxt::_TyTriggerAmbiguity > GetTriggerAmbiguities( _TyFinal const & _rFinal )
{
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( _rFinal, dfa, dctxt, true, true, 0, etdkUntagged );
	std::vector< _TyDfaCtxt::_TyTriggerAmbiguity > rgta;
	dctxt.GetTriggerAmbiguities( rgta );
	return rgta;
}
static std::string StrExample( _TyDfaCtxt::_TyTriggerAmbiguity const & _rta )
{
	return std::string( _rta.m_rgExample.begin(), _rta.m_rgExample.end() );
}

TEST( LexangTriggerAmbiguity, MatchIsLost )
{
	// <a*<trigger>ab - for "<aab" the trigger must be crossed after the first 'a' but the analyzer consumes both 'a's.
	std::vector< _TyDfaCtxt::_TyTriggerAmbiguity > rgta = GetTriggerAmbiguities( _TyFinal( l(u8'<') * ~l(u8'a') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * l(u8'a') * l(u8'b') ) );
	ASSERT_FALSE( rgta.empty() );
	EXPECT_EQ( rgta[0].m_tidTrigger, s_ktidTriggerABegin );
	EXPECT_EQ( rgta[0].m_rngChar.first, (_TyDfaCtxt::_TyRangeEl)u8'a' );
	EXPECT_EQ( StrExample( rgta[0] ), "b" );
}

TEST( LexangTriggerAmbiguity, MatchIsChangedToAnotherToken )
{
	// Token0: <a+<trigger>ab, Token1: <a+b. For "<aab" the rules intend Token0 but the analyzer finds Token1.
	_TyFinal Rule0 = l(u8'<') * ++l(u8'a') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * l(u8'a') * l(u8'b');
	_TyFinal Rule1 = l(u8'<') * ++l(u8'a') * l(u8'b');
	Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
	Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
	_TyFinal All( Rule0 );
	All.AddRule( Rule1 );
	std::vector< _TyDfaCtxt::_TyTriggerAmbiguity > rgta = GetTriggerAmbiguities( All );
	ASSERT_FALSE( rgta.empty() );
	EXPECT_EQ( StrExample( rgta[0] ), "b" );
}

TEST( LexangTriggerAmbiguity, NothingLostIsNotReported )
{
	// As the XML CharData: the trigger could be crossed after any letter, but not crossing it loses nothing.
	EXPECT_TRUE( GetTriggerAmbiguities( _TyFinal( l(u8'<') * ++( ++lr(u8'a',u8'z') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) ) * l(u8'>') ) ).empty() );
	// The characters after the trigger can't continue the input before it.
	EXPECT_TRUE( GetTriggerAmbiguities( _TyFinal( l(u8'<') * ++lr(u8'a',u8'z') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * l(u8'=') * ++lr(u8'0',u8'9') ) ).empty() );
	// Triggers on both sides of a token's content, as used by the trigger tests.
	EXPECT_TRUE( GetTriggerAmbiguities( _TyFinal( l(u8'"') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * ~lr(u8'a',u8'z') 
		* t( TyTestTriggerString< s_ktidTriggerAEnd, s_ktidTriggerABegin >() ) * l(u8'"') ) ).empty() );
}

// The generator refuses to generate a lexical analyzer from an untagged DFA with an ambiguous trigger.
TEST( LexangTriggerAmbiguity, GeneratorThrows )
{
	_TyFinal Rule = l(u8'<') * ~l(u8'a') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * l(u8'a') * l(u8'b');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( Rule, dfa, dctxt, true, true, 0, etdkUntagged );
	_l_generator< _TyDfa, char > gen( egfdStandaloneGenerator, "_lexangtest_ambiguous_notgenerated.h", "LTAMBIG", true, "ns_ltambig", "stateambig", "char8_t", "u8'", "'" );
	EXPECT_THROW( gen.add_dfa( dfa, dctxt, "start" ), std::exception );
}

// Tagged DFAs - see _l_dfatag.h.
// We check the tagged DFA created for a set of rules against the reference of lexangtest_tag.h.
// We compare, for the longest prefix of a string that the rules match: the token, the length of the match and the
//	triggers fired with their positions, in order.

// A tagged DFA for a set of rules: we match by walking its graph and performing its tag operations.
struct TestTagDfa
{
	typedef _TyDfa::_TyGraph _TyGraph;
	typedef _TyDfa::_TyGraphNode _TyGraphNode;
	_TyDfa m_dfa;
	_TyDfaCtxt m_dctxt;
	bool m_fTagged;
	TestTagDfa( _TyFinal const & _rFinal, bool _fOptimize, ETriggerDfaKind _etdk = etdkTagged )
		: m_dctxt( m_dfa )
	{
		m_fTagged = GenDfa( _rFinal, m_dfa, m_dctxt, _fOptimize, false, 0, _etdk );
		m_dfa._CreateRangeLookup();
	}
	size_t NStates() const
	{
		return (size_t)m_dfa.NStates();
	}
	TagMatch Match( const char * _psz )
	{
		TagMatch tmLast;
		if ( !m_dfa.m_fTagged )
		{
			ADD_FAILURE() << "Not a tagged DFA.";
			return tmLast;
		}
		const size_t knLen = strlen( _psz );
		const _TyDfa::_TyAlphaIndex kaiFirstOpList = (_TyDfa::_TyAlphaIndex)( m_dfa.AlphabetSize() - m_dfa.m_nTriggers );
		std::vector< TyRgTagEvents > rgRegisters( m_dfa.m_nTagRegisters );
		auto fnRegister = [&]( unsigned short _nReg ) -> TyRgTagEvents &
		{
			return rgRegisters[ ( _TyDfa::s_knTagRegisterTemp == _nReg ) ? ( rgRegisters.size() - 1 ) : (size_t)_nReg ];
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
					if ( e_aatAccept == keaat )
					{
						tmLast.m_nToken = !it->second.m_pSdpAction ? s_knMatchNoToken : (int)( *it->second.m_pSdpAction )->VGetTokenId();
						tmLast.m_nLength = pos;
						// The register for the way in which the token is matched is register zero.
						for ( TagEvent const & rev : rgRegisters[ 0 ] )
						{
							TagEvent ev = { (int)m_dfa.m_rgTags[ (size_t)rev.m_nTrigger ].m_tidTrigger, rev.m_pos };
							tmLast.m_rgEvents.push_back( ev );
						}
						// Then the triggers crossed at the end of the token.
						if ( it->second.m_iTagFinish >= 0 )
						{
							for ( int iTag : m_dfa.m_rgTagFinishes[ (size_t)it->second.m_iTagFinish ] )
							{
								TagEvent ev = { (int)m_dfa.m_rgTags[ (size_t)iTag ].m_tidTrigger, pos };
								tmLast.m_rgEvents.push_back( ev );
							}
						}
					}
					else
					{
						EXPECT_LT( it->second.m_iTagFinish, 0 );
					}
					break;
				}
			}
			if ( pos == knLen )
				break;
			const _TyDfa::_TyAlphaIndex ai = m_dfa._LookupAlphaSetNum( (_TyDfa::_TyRangeEl)(unsigned char)_psz[ pos ] );
			if ( ai < 0 )
				break; // Not in the alphabet.
			_TyGraph::_TyLinkPosIterConst	lpi( pgnCur->PPGLChildHead() );
			for ( ; !lpi.FIsLast() && ( *lpi != ai ); lpi.NextChild() )
				;
			if ( lpi.FIsLast() )
				break;
			pgnCur = lpi.PGNChild();
			// A tag operation state has a single transition: on the pseudo-trigger for its list of operations.
			_TyGraph::_TyLinkPosIterConst	lpiOps( pgnCur->PPGLChildHead() );
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
					{
						EXPECT_EQ( rop.m_nDst, rop.m_nSrc );
						TagEvent ev = { rop.m_iTag, pos }; // The trigger is crossed before the character.
						fnRegister( rop.m_nDst ).push_back( ev );
					}
				}
				pgnCur = lpiOps.PGNChild();
				lpiOps.NextChild();
				EXPECT_TRUE( lpiOps.FIsLast() ) << "A tag operation state has more than one transition.";
			}
		}
		return tmLast;
	}
};

// The reference and the tagged DFAs - unoptimized and optimized - for a set of rules.
struct TestTagDfas
{
	TagReference m_ref;
	TestTagDfa m_dfaUnoptimized;
	TestTagDfa m_dfaOptimized;
	TestTagDfas( _TyFinal const & _rFinal )
		: m_ref( _rFinal ),
			m_dfaUnoptimized( _rFinal, false ),
			m_dfaOptimized( _rFinal, true )
	{
	}
	// Match the string by all three - they must agree. Returns the result.
	TagMatch Match( const char * _psz )
	{
		const TagMatch tmRef = m_ref.Match( _psz );
		EXPECT_EQ( m_dfaUnoptimized.Match( _psz ), tmRef ) << "The unoptimized tagged DFA disagrees with the reference on [" << _psz << "].";
		EXPECT_EQ( m_dfaOptimized.Match( _psz ), tmRef ) << "The optimized tagged DFA disagrees with the reference on [" << _psz << "].";
		return tmRef;
	}
	// Match random strings over the given alphabet. Returns the number of strings for which triggers fired.
	size_t NRandomStringsAgree( const char * _pszAlphabet, size_t _nStrings, size_t _nMaxLen, unsigned _uSeed, bool _fExpectMatches = true )
	{
		std::mt19937 rng( _uSeed );
		const size_t nAlpha = strlen( _pszAlphabet );
		size_t nMatched = 0, nWithEvents = 0;
		for ( size_t n = 0; n < _nStrings; ++n )
		{
			std::string str( rng() % ( _nMaxLen + 1 ), ' ' );
			for ( char & rc : str )
				rc = _pszAlphabet[ rng() % nAlpha ];
			const TagMatch tmRef = m_ref.Match( str.c_str() );
			const TagMatch tmUnoptimized = m_dfaUnoptimized.Match( str.c_str() );
			const TagMatch tmOptimized = m_dfaOptimized.Match( str.c_str() );
			if ( !( tmUnoptimized == tmRef ) || !( tmOptimized == tmRef ) )
			{
				ADD_FAILURE() << "On [" << str << "]: reference " << tmRef << " unoptimized " << tmUnoptimized << " optimized " << tmOptimized;
				return 0;
			}
			nMatched += ( tmRef.m_nToken >= 0 );
			nWithEvents += !tmRef.m_rgEvents.empty();
		}
		if ( _fExpectMatches )
			EXPECT_GT( nMatched, size_t(0) );
		return nWithEvents;
	}
};

// The example for which an untagged DFA fails: the trigger must be crossed before the last 'a'.
TEST( LexangTagDfa, TriggerCrossedWhereTheMatchRequires )
{
	_TyFinal Rule = l(u8'<') * ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'a') * l(u8'b');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "<ab" ), ExpectTagMatch( s_ktidToken0, 3, { { s_ktidTrigger1, 1 } } ) );
	EXPECT_EQ( dfas.Match( "<aab" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 2 } } ) );
	EXPECT_EQ( dfas.Match( "<aaaaab" ), ExpectTagMatch( s_ktidToken0, 7, { { s_ktidTrigger1, 5 } } ) );
	EXPECT_EQ( dfas.Match( "<b" ), ExpectTagMatch( s_knNoMatch, 0 ) );
	EXPECT_EQ( dfas.Match( "<aa" ), ExpectTagMatch( s_knNoMatch, 0 ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<ab", 3000, 8, 11 ), size_t(0) );
	// An untagged DFA is not created for these rules unless asked for.
	EXPECT_TRUE( TestTagDfa( Rule, true, etdkAuto ).m_fTagged );
	EXPECT_FALSE( TestTagDfa( Rule, true, etdkUntagged ).m_fTagged );
}

// Rules for which an untagged DFA suffices get one - unless a tagged DFA is asked for, which then fires the same triggers.
TEST( LexangTagDfa, UntaggedWhenSufficient )
{
	_TyFinal Tag = l(u8'<') * t( TyT< s_ktidTrigger1 >() ) * ++lr(u8'a',u8'z') * t( TyT< s_ktidTrigger2 >() )
									* --( l(u8'=') * t( TyT< s_ktidTrigger3 >() ) * ++lr(u8'0',u8'9') * t( TyT< s_ktidTrigger4 >() ) ) * l(u8'>');
	Tag.SetAction( TyTestToken< s_ktidToken0 >() );
	EXPECT_FALSE( TestTagDfa( Tag, true, etdkAuto ).m_fTagged );
	TestTagDfas dfas( Tag );
	EXPECT_EQ( dfas.Match( "<ab=12>" ), ExpectTagMatch( s_ktidToken0, 7, { { s_ktidTrigger1, 1 }, { s_ktidTrigger2, 3 }, { s_ktidTrigger3, 4 }, { s_ktidTrigger4, 6 } } ) );
	EXPECT_EQ( dfas.Match( "<ab>" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 1 }, { s_ktidTrigger2, 3 } } ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<a=1>", 20000, 8, 12 ), size_t(0) );
	// Each trigger is crossed at most once and in a known order: a position for each trigger suffices.
	EXPECT_TRUE( dfas.m_dfaOptimized.m_dfa.m_fTagSlots );
	// There is only ever one way in which the input may be matched: a single register plus the temporary.
	EXPECT_EQ( dfas.m_dfaOptimized.m_dfa.m_nTagRegisters, size_t(2) );
}

// A trigger within a loop: as with the XML CharData the trigger is crossed as late as possible.
TEST( LexangTagDfa, TriggerInLoop )
{
	_TyFinal Rule = l(u8'<') * ++( ++( l(u8'a') | l(u8'b') ) * t( TyT< s_ktidTrigger1 >() ) | l(u8'&') * l(u8'a') * l(u8';') ) * l(u8'>');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "<ab>" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 3 } } ) );
	EXPECT_EQ( dfas.Match( "<ab&a;b&a;>" ), ExpectTagMatch( s_ktidToken0, 11, { { s_ktidTrigger1, 3 }, { s_ktidTrigger1, 7 } } ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<ab&;>", 30000, 10, 13 ), size_t(0) );
	// A trigger which may be crossed more than once: the history of the triggers must be kept.
	EXPECT_FALSE( dfas.m_dfaOptimized.m_dfa.m_fTagSlots );
}

// A trigger within a loop where the position of each crossing depends upon what follows.
TEST( LexangTagDfa, AmbiguousTriggerInLoop )
{
	// ( a* <T1> a b )+ c
	_TyFinal Rule = ++( ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'a') * l(u8'b') ) * l(u8'c');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "abc" ), ExpectTagMatch( s_ktidToken0, 3, { { s_ktidTrigger1, 0 } } ) );
	EXPECT_EQ( dfas.Match( "aaabaabc" ), ExpectTagMatch( s_ktidToken0, 8, { { s_ktidTrigger1, 2 }, { s_ktidTrigger1, 5 } } ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "abc", 30000, 12, 14 ), size_t(0) );
	EXPECT_FALSE( dfas.m_dfaOptimized.m_dfa.m_fTagSlots );
}

// More than one rule: the rule decides which triggers fire.
TEST( LexangTagDfa, RulesWithDifferentTriggers )
{
	// Token0: <a+ <T1> ab		Token1: <a+ <T2> b		Token2: <a+
	_TyFinal Rule0 = l(u8'<') * ++l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'a') * l(u8'b');
	_TyFinal Rule1 = l(u8'<') * ++l(u8'a') * t( TyT< s_ktidTrigger2 >() ) * l(u8'b');
	_TyFinal Rule2 = l(u8'<') * ++l(u8'a');
	Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
	Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
	Rule2.SetAction( TyTestToken< s_ktidToken2 >() );
	_TyFinal All( Rule0 );
	All.AddRule( Rule1 );
	All.AddRule( Rule2 );
	TestTagDfas dfas( All );
	// "<aab" is matched by both of the first two rules: the first wins and its trigger is the one which fires.
	EXPECT_EQ( dfas.Match( "<aab" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 2 } } ) );
	EXPECT_EQ( dfas.Match( "<ab" ), ExpectTagMatch( s_ktidToken1, 3, { { s_ktidTrigger2, 2 } } ) );
	// No trigger fires for a token which crossed none - whatever was crossed along the way for the other rules.
	EXPECT_EQ( dfas.Match( "<aa" ), ExpectTagMatch( s_ktidToken2, 3 ) );
	EXPECT_EQ( dfas.Match( "<aac" ), ExpectTagMatch( s_ktidToken2, 3 ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<abc", 30000, 8, 15 ), size_t(0) );
}

// Triggers which an untagged DFA cannot represent: more than one trigger leaving a state and a trigger leaving an accepting state.
TEST( LexangTagDfa, TriggersNotRepresentableUntagged )
{
	{
		// x ( <T1> a | <T2> b ) y
		_TyFinal Rule = l(u8'x') * ( t( TyT< s_ktidTrigger1 >() ) * l(u8'a') | t( TyT< s_ktidTrigger2 >() ) * l(u8'b') ) * l(u8'y');
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		EXPECT_TRUE( TestTagDfa( Rule, true, etdkAuto ).m_fTagged );
		TestTagDfas dfas( Rule );
		EXPECT_EQ( dfas.Match( "xay" ), ExpectTagMatch( s_ktidToken0, 3, { { s_ktidTrigger1, 1 } } ) );
		EXPECT_EQ( dfas.Match( "xby" ), ExpectTagMatch( s_ktidToken0, 3, { { s_ktidTrigger2, 1 } } ) );
		EXPECT_GT( dfas.NRandomStringsAgree( "xaby", 3000, 5, 16 ), size_t(0) );
	}
	{
		// Token0: a+		Token1: a+ <T1> b+ <T2>
		_TyFinal Rule0 = ++l(u8'a');
		_TyFinal Rule1 = ++l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * ++l(u8'b') * t( TyT< s_ktidTrigger2 >() );
		Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
		Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
		_TyFinal All( Rule0 );
		All.AddRule( Rule1 );
		EXPECT_TRUE( TestTagDfa( All, true, etdkAuto ).m_fTagged );
		TestTagDfas dfas( All );
		EXPECT_EQ( dfas.Match( "aa" ), ExpectTagMatch( s_ktidToken0, 2 ) );
		EXPECT_EQ( dfas.Match( "aac" ), ExpectTagMatch( s_ktidToken0, 2 ) );
		// The second trigger is crossed at the end of the token.
		EXPECT_EQ( dfas.Match( "aabb" ), ExpectTagMatch( s_ktidToken1, 4, { { s_ktidTrigger1, 2 }, { s_ktidTrigger2, 4 } } ) );
		EXPECT_EQ( dfas.Match( "aabbab" ), ExpectTagMatch( s_ktidToken1, 4, { { s_ktidTrigger1, 2 }, { s_ktidTrigger2, 4 } } ) );
		EXPECT_GT( dfas.NRandomStringsAgree( "abc", 3000, 8, 17 ), size_t(0) );
	}
}

// Several triggers whose positions all depend upon what follows.
TEST( LexangTagDfa, SeveralAmbiguousTriggers )
{
	// < a* <T1> (a|b)* <T2> b* <T3> >
	_TyFinal Rule = l(u8'<') * ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * ~( l(u8'a') | l(u8'b') ) * t( TyT< s_ktidTrigger2 >() )
									* ~l(u8'b') * t( TyT< s_ktidTrigger3 >() ) * l(u8'>');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	// Each trigger is crossed as late as it can be: the first after the leading 'a's and the others at the end.
	EXPECT_EQ( dfas.Match( "<aabab>" ), ExpectTagMatch( s_ktidToken0, 7, { { s_ktidTrigger1, 3 }, { s_ktidTrigger2, 6 }, { s_ktidTrigger3, 6 } } ) );
	EXPECT_EQ( dfas.Match( "<>" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger1, 1 }, { s_ktidTrigger2, 1 }, { s_ktidTrigger3, 1 } } ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<ab>", 20000, 9, 18 ), size_t(0) );
	EXPECT_TRUE( dfas.m_dfaOptimized.m_dfa.m_fTagSlots );
}

// Alternatives which cross different triggers to reach the same place: the one crossing fewer triggers wins, then the
//	one whose trigger was used first within the rules.
TEST( LexangTagDfa, AlternativesWithDifferentTriggers )
{
	{
		// x ( <T1> <T2> | <T3> ) y
		_TyFinal Rule = l(u8'x') * ( t( TyT< s_ktidTrigger1 >() ) * t( TyT< s_ktidTrigger2 >() ) | t( TyT< s_ktidTrigger3 >() ) ) * l(u8'y');
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		TestTagDfas dfas( Rule );
		EXPECT_EQ( dfas.Match( "xy" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger3, 1 } } ) );
	}
	{
		// x ( <T2> | <T1> ) y - T2 is used first.
		_TyFinal Rule = l(u8'x') * ( t( TyT< s_ktidTrigger2 >() ) | t( TyT< s_ktidTrigger1 >() ) ) * l(u8'y');
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		TestTagDfas dfas( Rule );
		EXPECT_EQ( dfas.Match( "xy" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger2, 1 } } ) );
	}
	{
		// x ( a <T1> | a ) ( a <T2> | a )* y - crossing no trigger at all has precedence.
		_TyFinal Rule = l(u8'x') * ( l(u8'a') * t( TyT< s_ktidTrigger1 >() ) | l(u8'a') ) * ~( l(u8'a') * t( TyT< s_ktidTrigger2 >() ) | l(u8'a') ) * l(u8'y');
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		TestTagDfas dfas( Rule );
		EXPECT_EQ( dfas.Match( "xaaay" ), ExpectTagMatch( s_ktidToken0, 5 ) );
		(void)dfas.NRandomStringsAgree( "xay", 3000, 7, 19 );
	}
}

// A trigger within a loop which need not consume any input: the loop is not followed endlessly.
TEST( LexangTagDfa, TriggerInEmptyLoop )
{
	// x ( a* <T1> )* y
	_TyFinal Rule = l(u8'x') * ~( ~l(u8'a') * t( TyT< s_ktidTrigger1 >() ) ) * l(u8'y');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "xy" ), ExpectTagMatch( s_ktidToken0, 2 ) );
	EXPECT_EQ( dfas.Match( "xaay" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 3 } } ) );
	(void)dfas.NRandomStringsAgree( "xay", 3000, 7, 20 );
}

// Pattern exclusion along with triggers.
TEST( LexangTagDfa, ExcludesWithTriggers )
{
	// ( < [ab]* <T1> b > ) - "<ab>"
	_TyFinal Rule = ( l(u8'<') * ~( l(u8'a') | l(u8'b') ) * t( TyT< s_ktidTrigger1 >() ) * l(u8'b') * l(u8'>') ) - ls(u8"<ab>");
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "<bb>" ), ExpectTagMatch( s_ktidToken0, 4, { { s_ktidTrigger1, 2 } } ) );
	EXPECT_EQ( dfas.Match( "<ab>" ), ExpectTagMatch( s_knNoMatch, 0 ) );
	EXPECT_EQ( dfas.Match( "<aab>" ), ExpectTagMatch( s_ktidToken0, 5, { { s_ktidTrigger1, 3 } } ) );
	EXPECT_GT( dfas.NRandomStringsAgree( "<ab>", 20000, 7, 21 ), size_t(0) );
}

// Pattern exclusion within a loop which has a trigger: a way of matching which matches the excluded pattern doesn't
//	exclude the other ways - only the way of highest precedence decides, as it does for an untagged DFA.
TEST( LexangTagDfa, ExcludesWithinLoop )
{
	{
		// ( ( [ab]+ - "b" ) <T1> | c )+
		_TyFinal Piece = ++( l(u8'a') | l(u8'b') ) - l(u8'b');
		_TyFinal Rule = ++( Piece * t( TyT< s_ktidTrigger1 >() ) | l(u8'c') );
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		TestTagDfas dfas( Rule );
		// "ab" is not excluded - though were the trigger crossed after the 'a' the second piece, "b", would be.
		EXPECT_EQ( dfas.Match( "ab" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger1, 2 } } ) );
		EXPECT_EQ( dfas.Match( "abcba" ), ExpectTagMatch( s_ktidToken0, 5, { { s_ktidTrigger1, 2 }, { s_ktidTrigger1, 5 } } ) );
		// "b" on its own is excluded.
		EXPECT_EQ( dfas.Match( "b" ), ExpectTagMatch( s_knNoMatch, 0 ) );
		EXPECT_GT( dfas.NRandomStringsAgree( "abc", 20000, 8, 22 ), size_t(0) );
	}
	{
		// As the XML CharData: characters which don't contain "]]>", with a trigger after each run, and references.
		_TyFinal CharData = ++lnot(u8"<&") - ( ~lnot(u8"<&") * ls(u8"]]>") * ~lnot(u8"<&") );
		_TyFinal Rule = ++( CharData * t( TyT< s_ktidTrigger1 >() ) | l(u8'&') * l(u8'a') * l(u8';') );
		Rule.SetAction( TyTestToken< s_ktidToken0 >() );
		TestTagDfas dfas( Rule );
		EXPECT_EQ( dfas.Match( "ab]]cd" ), ExpectTagMatch( s_ktidToken0, 6, { { s_ktidTrigger1, 6 } } ) );
		EXPECT_EQ( dfas.Match( "a]&a;]>" ), ExpectTagMatch( s_ktidToken0, 7, { { s_ktidTrigger1, 2 }, { s_ktidTrigger1, 7 } } ) );
		EXPECT_EQ( dfas.Match( "ab]]>cd" ), ExpectTagMatch( s_knNoMatch, 0 ) );
		EXPECT_GT( dfas.NRandomStringsAgree( "a]>&;", 30000, 10, 23 ), size_t(0) );
		// There is only ever one way of matching to keep: the states of the copies of the character data are
		//	equivalent - their anti-accepting states included - so no register is needed for the way which crossed
		//	the trigger before the last character. A single register plus the temporary.
		EXPECT_EQ( dfas.m_dfaOptimized.m_dfa.m_nTagRegisters, size_t(2) );
		EXPECT_EQ( dfas.m_dfaOptimized.m_dfa.m_rgTagOpLists.size(), size_t(1) );
	}
}

// The optimizer must not merge states whose transitions perform different operations nor accepting states whose
//	tokens end with different triggers.
TEST( LexangTagDfa, OptimizerKeepsOperationsApart )
{
	// Token0: a <T1> x | b <T2> x - identical but for the trigger.
	_TyFinal Rule = l(u8'a') * t( TyT< s_ktidTrigger1 >() ) * l(u8'x') | l(u8'b') * t( TyT< s_ktidTrigger2 >() ) * l(u8'x');
	Rule.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfas( Rule );
	EXPECT_EQ( dfas.Match( "ax" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger1, 1 } } ) );
	EXPECT_EQ( dfas.Match( "bx" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger2, 1 } } ) );
	// Token0: a x <T1> | b x <T2> - the triggers are at the end of the token.
	_TyFinal RuleEnd = l(u8'a') * l(u8'x') * t( TyT< s_ktidTrigger1 >() ) | l(u8'b') * l(u8'x') * t( TyT< s_ktidTrigger2 >() );
	RuleEnd.SetAction( TyTestToken< s_ktidToken0 >() );
	TestTagDfas dfasEnd( RuleEnd );
	EXPECT_EQ( dfasEnd.Match( "ax" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger1, 2 } } ) );
	EXPECT_EQ( dfasEnd.Match( "bx" ), ExpectTagMatch( s_ktidToken0, 2, { { s_ktidTrigger2, 2 } } ) );
	// Without the triggers the optimizer merges the two alternatives.
	_TyFinal RuleNoTriggers = l(u8'a') * l(u8'x') | l(u8'b') * l(u8'x');
	EXPECT_LT( TestDfa( RuleNoTriggers, true ).NStates(), dfasEnd.m_dfaOptimized.NStates() );
}

// Rules built at random from a few pieces - each checked against the reference with random strings.
TEST( LexangTagDfa, RandomRules )
{
	std::mt19937 rng( 4242 );
	size_t nWithEvents = 0, nTagSlots = 0, nUntaggedSufficient = 0, nWithoutTriggers = 0, nWithExcludes = 0;
	const size_t knRules = 400;
	for ( size_t nRule = 0; nRule < knRules; ++nRule )
	{
		// Each rule is a sequence of pieces: a piece is a character, a set, a trigger or a group which is itself a
		//	sequence, alternatives or a sequence which excludes another, each possibly repeated.
		typedef std::shared_ptr< _TyFinal > TyPFinal;
		std::function< TyPFinal( int ) > fnPiece;
		std::function< TyPFinal( int ) > fnSequence = [&]( int _nDepth ) -> TyPFinal
		{
			TyPFinal pre = fnPiece( _nDepth );
			for ( size_t n = rng() % 4; n--; )
			{
				TyPFinal preNext = fnPiece( _nDepth );
				pre = std::make_shared< _TyFinal >( *pre * *preNext );
			}
			return pre;
		};
		fnPiece = [&]( int _nDepth ) -> TyPFinal
		{
			TyPFinal pre;
			switch ( rng() % ( ( _nDepth < 2 ) ? 9 : 5 ) )
			{
				case 0: pre = std::make_shared< _TyFinal >( l(u8'a') ); break;
				case 1: pre = std::make_shared< _TyFinal >( l(u8'b') ); break;
				case 2: pre = std::make_shared< _TyFinal >( l(u8'a') | l(u8'b') ); break;
				case 3: case 4:
					switch ( rng() % 3 )
					{
						case 0: pre = std::make_shared< _TyFinal >( t( TyT< s_ktidTrigger1 >() ) ); break;
						case 1: pre = std::make_shared< _TyFinal >( t( TyT< s_ktidTrigger2 >() ) ); break;
						default: pre = std::make_shared< _TyFinal >( t( TyT< s_ktidTrigger3 >() ) ); break;
					}
				break;
				case 5:
				{
					TyPFinal preFirst = fnSequence( _nDepth + 1 );
					TyPFinal preSecond = fnSequence( _nDepth + 1 );
					pre = std::make_shared< _TyFinal >( *preFirst | *preSecond );
				}
				break;
				case 6:
				{
					TyPFinal preFirst = fnSequence( _nDepth + 1 );
					TyPFinal preSecond = fnSequence( _nDepth + 1 );
					pre = std::make_shared< _TyFinal >( *preFirst - *preSecond );
					++nWithExcludes;
				}
				break;
				default: pre = fnSequence( _nDepth + 1 ); break;
			}
			switch ( rng() % 6 )
			{
				case 0: return std::make_shared< _TyFinal >( ~*pre );
				case 1: return std::make_shared< _TyFinal >( ++*pre );
				case 2: return std::make_shared< _TyFinal >( --*pre );
				default: return pre;
			}
		};
		// One or two rules, each beginning with 'c' so that no rule begins with a trigger.
		_TyFinal Rule0 = l(u8'c') * *fnSequence( 0 );
		Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
		_TyFinal All( Rule0 );
		if ( rng() % 2 )
		{
			_TyFinal Rule1 = l(u8'c') * *fnSequence( 0 );
			Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
			All.AddRule( Rule1 );
		}
		TestTagDfas dfas( All );
		if ( !dfas.m_dfaOptimized.m_fTagged )
		{
			++nWithoutTriggers; // The rules happen to have no triggers: there is then no tagged DFA.
			continue;
		}
		nWithEvents += !!dfas.NRandomStringsAgree( "abc", 500, 9, unsigned( nRule ), false );
		nTagSlots += dfas.m_dfaOptimized.m_dfa.m_fTagSlots;
		nUntaggedSufficient += !TestTagDfa( All, true, etdkAuto ).m_fTagged;
		if ( ::testing::Test::HasFailure() )
		{
			ADD_FAILURE() << "Rule number [" << nRule << "].";
			return;
		}
	}
	// The rules should exercise triggers, both kinds of register and both tagged and untagged DFAs.
	EXPECT_LT( nWithoutTriggers, knRules / 4 );
	EXPECT_GT( nWithExcludes, knRules / 8 );
	EXPECT_GT( nWithEvents, knRules / 2 );
	EXPECT_GT( nTagSlots, size_t(10) );
	EXPECT_LT( nTagSlots, knRules - 10 );
	EXPECT_GT( nUntaggedSufficient, size_t(5) );
	EXPECT_LT( nUntaggedSufficient, knRules - 5 );
}

} // namespace ns_lexangtest


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
#include <random>
#include <sstream>
#include "gtest/gtest.h"

std::string g_strProgramName;

namespace ns_lexangtest
{
__REGEXP_OP_USING_NAMESPACE

// The result of matching a string against a DFA.
static const int s_knNoMatch = -1; // The string is not matched.
static const int s_knAntiAccept = -2; // The string ends in an anti-accepting state: it was explicitly excluded.
static const int s_knMatchNoToken = 0; // The string is matched by a rule without an action.
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

} // namespace ns_lexangtest

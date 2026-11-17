
//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangtestgen.cpp
// Generates the lexical analyzers used by the lexang unit tests in lexanglextest.cpp.
// Each grammar is generated into its own header and namespace: _lexangtest_<grammar>.h, ns_lt<grammar>.

#include "lexangtest_gen.h"

std::string g_strProgramName;

namespace ns_lexangtest
{
__REGEXP_OP_USING_NAMESPACE

// Generate the header for a single DFA.
void
GenerateHeader( _TyDfa & _rDfa, _TyDfaCtxt & _rDfaCtxt, const char * _pszName )
{
	std::string strHeader = std::string( "_lexangtest_" ) + _pszName + ".h";
	std::string strPpBase = std::string( "LT" ) + _pszName;
	std::transform( strPpBase.begin(), strPpBase.end(), strPpBase.begin(), []( unsigned char _c ) { return (char)toupper( _c ); } );
	std::string strNamespace = std::string( "ns_lt" ) + _pszName;
	std::string strState = std::string( "state" ) + _pszName;
  _l_generator< _TyDfa, char >
    gen( egfdStandaloneGenerator, strHeader.c_str(), strPpBase.c_str(), true, strNamespace.c_str(), strState.c_str(), "char8_t", "u8'", "'");
  gen.add_dfa( _rDfa, _rDfaCtxt, "start" );
  gen.generate();
}

// basic: literals, ranges, strings, or, follows, zero or more, one or more, zero or one.
void
GenerateBasic()
{
	_TyFinal Spaces = ++( l(u8' ') | l(u8'\t') );
	_TyFinal Ident = ( lr(u8'a',u8'z') | l(u8'_') ) * ~( lr(u8'a',u8'z') | lr(u8'0',u8'9') | l(u8'_') );
	_TyFinal Number = ++lr(u8'0',u8'9') * --( l(u8'.') * ++lr(u8'0',u8'9') );
	_TyFinal KeywordIf = ls(u8"if");
	_TyFinal Arrow = ls(u8"->");
	_TyFinal Minus = l(u8'-');

	// The earlier rule wins when two rules match the same input: "if" is thus a keyword and not an identifier.
	KeywordIf.SetAction( TyTestToken< s_ktidToken0 >() );
	Ident.SetAction( TyTestToken< s_ktidToken1 >() );
	Number.SetAction( TyTestToken< s_ktidToken2 >() );
	Spaces.SetAction( TyTestToken< s_ktidToken3 >() );
	Arrow.SetAction( TyTestToken< s_ktidToken4 >() );
	Minus.SetAction( TyTestToken< s_ktidToken5 >() );
	_TyFinal All( KeywordIf );
	All.AddRule( Ident );
	All.AddRule( Number );
	All.AddRule( Spaces );
	All.AddRule( Arrow );
	All.AddRule( Minus );

	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "basic" );
}

// triggers: positions and strings recorded by triggers and saved in the token's value.
void
GenerateTriggers()
{
	_TyFinal Name = ++lr(u8'a',u8'z');
	_TyFinal Digits = ++lr(u8'0',u8'9');
	// A quoted string: the token's value is the string between the quotes - which may be empty.
	_TyFinal Quoted = l(u8'"') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * ~( lr(u8'a',u8'z') | l(u8' ') ) 
										* t( TyTestTriggerString< s_ktidTriggerAEnd, s_ktidTriggerABegin >() ) * l(u8'"');
	// A tag: <name> or <name=digits> - the token's values are the name and the digits, the latter being optional.
	_TyFinal Tag = l(u8'<') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * Name * t( TyTestTriggerString< s_ktidTriggerAEnd, s_ktidTriggerABegin >() )
										* --( l(u8'=') * t( TyTestTriggerPosition< s_ktidTriggerBBegin >() ) * Digits * t( TyTestTriggerString< s_ktidTriggerBEnd, s_ktidTriggerBBegin >() ) )
										* l(u8'>');
	_TyFinal Spaces = ++l(u8' ');
	_TyFinal Word = Name;

	Quoted.SetAction( TyTestTokenWithString< s_ktidToken0, s_ktidTriggerAEnd, s_ktidTriggerABegin >() );
	Tag.SetAction( TyTestTokenWithTwoStrings< s_ktidToken1, s_ktidTriggerAEnd, s_ktidTriggerABegin, s_ktidTriggerBEnd, s_ktidTriggerBBegin >() );
	Spaces.SetAction( TyTestToken< s_ktidToken2 >() );
	Word.SetAction( TyTestToken< s_ktidToken3 >() );
	_TyFinal All( Quoted );
	All.AddRule( Tag );
	All.AddRule( Spaces );
	All.AddRule( Word );

	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "triggers" );
}

// excludes: a rule which excludes some of what it would otherwise match.
void
GenerateExcludes()
{
	// A name which is not "xml".
	_TyFinal Name = ++lr(u8'a',u8'z') - ls(u8"xml");
	_TyFinal Spaces = ++l(u8' ');
	Name.SetAction( TyTestToken< s_ktidToken0 >() );
	Spaces.SetAction( TyTestToken< s_ktidToken1 >() );
	_TyFinal All( Name );
	All.AddRule( Spaces );

	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "excludes" );
}

// completes: a rule which matches up to and including the first occurrence of an ending expression.
void
GenerateCompletes()
{
	_TyFinal CommentChar = lr(u8'a',u8'z') | l(u8' ') | l(u8'-') | l(u8'>') | l(u8'<') | l(u8'!');
	_TyFinal Comment = ls(u8"<!--") * ( ~CommentChar + ls(u8"-->") );
	_TyFinal Word = ++lr(u8'a',u8'z');
	_TyFinal Spaces = ++l(u8' ');
	Comment.SetAction( TyTestToken< s_ktidToken0 >() );
	Word.SetAction( TyTestToken< s_ktidToken1 >() );
	Spaces.SetAction( TyTestToken< s_ktidToken2 >() );
	_TyFinal All( Comment );
	All.AddRule( Word );
	All.AddRule( Spaces );

	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "completes" );
}

// lookahead: rules which match only when followed by a trailing context which is not part of the token.
void
GenerateLookahead()
{
	_TyFinal Name = ++lr(u8'a',u8'z');
	// A name followed by '(' is a function name - the '(' is not part of the token.
	_TyFinal Function = Name / l(u8'(');
	// A name followed by "::" is a scope name.
	_TyFinal Scope = Name / ls(u8"::");
	_TyFinal Punct = l(u8'(') | l(u8')') | l(u8':');
	_TyFinal Spaces = ++l(u8' ');
	Function.SetAction( TyTestToken< s_ktidToken0 >() );
	Scope.SetAction( TyTestToken< s_ktidToken1 >() );
	Name.SetAction( TyTestToken< s_ktidToken2 >() );
	Punct.SetAction( TyTestToken< s_ktidToken3 >() );
	Spaces.SetAction( TyTestToken< s_ktidToken4 >() );
	_TyFinal All( Function );
	All.AddRule( Scope );
	All.AddRule( Name );
	All.AddRule( Punct );
	All.AddRule( Spaces );

	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookahead" );
}

// The rules of the "lookaheadambig" grammar - used on its own and as the second DFA of the "multi" grammar.
// A lookahead rule whose end ( before the trailing context ) coincides with the end of other, different, rules.
void
GetLookaheadAmbigRules( _TyFinal & _rAll )
{
	// Any number of 'a's then a 'b' when followed by 'c'.
	_TyFinal LookaheadRule = ( ~l(u8'a') * l(u8'b') ) / l(u8'c');
	_TyFinal RuleAB = ls(u8"ab");
	_TyFinal RuleAAB = ls(u8"aab");
	_TyFinal RuleC = l(u8'c');
	_TyFinal RuleA = l(u8'a');
	LookaheadRule.SetAction( TyTestToken< s_ktidToken0 >() );
	RuleAB.SetAction( TyTestToken< s_ktidToken1 >() );
	RuleAAB.SetAction( TyTestToken< s_ktidToken2 >() );
	RuleC.SetAction( TyTestToken< s_ktidToken3 >() );
	RuleA.SetAction( TyTestToken< s_ktidToken4 >() );
	_TyFinal All( LookaheadRule );
	All.AddRule( RuleAB );
	All.AddRule( RuleAAB );
	All.AddRule( RuleC );
	All.AddRule( RuleA );
	_rAll.AddRule( All );
}

void
GenerateLookaheadAmbig()
{
	_TyFinal Spaces = ++l(u8' ');
	Spaces.SetAction( TyTestToken< s_ktidToken7 >() );
	_TyFinal All( Spaces );
	GetLookaheadAmbigRules( All );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadambig" );
}

// lookaheadtwo: two lookahead rules which are both in progress over the same input.
void
GenerateLookaheadTwo()
{
	_TyFinal RuleA_BC = l(u8'a') / ls(u8"bc");
	_TyFinal RuleAB_C = ls(u8"ab") / l(u8'c');
	_TyFinal RuleB = l(u8'b');
	_TyFinal RuleC = l(u8'c');
	_TyFinal RuleA = l(u8'a');
	RuleA_BC.SetAction( TyTestToken< s_ktidToken0 >() );
	RuleAB_C.SetAction( TyTestToken< s_ktidToken1 >() );
	RuleB.SetAction( TyTestToken< s_ktidToken2 >() );
	RuleC.SetAction( TyTestToken< s_ktidToken3 >() );
	RuleA.SetAction( TyTestToken< s_ktidToken4 >() );
	_TyFinal All( RuleA_BC );
	All.AddRule( RuleAB_C );
	All.AddRule( RuleB );
	All.AddRule( RuleC );
	All.AddRule( RuleA );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadtwo" );
}

// lookaheadoverlap: the end of the token can match the beginning of the trailing context.
void
GenerateLookaheadOverlap()
{
	// One or more 'x's when followed by "xy".
	_TyFinal RuleXs = ++l(u8'x') / ls(u8"xy");
	_TyFinal RuleX = l(u8'x');
	_TyFinal RuleY = l(u8'y');
	RuleXs.SetAction( TyTestToken< s_ktidToken0 >() );
	RuleX.SetAction( TyTestToken< s_ktidToken1 >() );
	RuleY.SetAction( TyTestToken< s_ktidToken2 >() );
	_TyFinal All( RuleXs );
	All.AddRule( RuleX );
	All.AddRule( RuleY );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadoverlap" );
}

// lookaheadsplit: rules for which there is more than one place at which the token could end.
void
GenerateLookaheadSplit()
{
	// "a" or "ab" when followed by "bc" or "c": "abc" is matched as "a"+"bc" and as "ab"+"c".
	_TyFinal RuleSplit = ( l(u8'a') | ls(u8"ab") ) / ( ls(u8"bc") | l(u8'c') );
	// One or more 'a's when followed by any number of 'a's and then a 'b'.
	_TyFinal RuleAs = ++l(u8'a') / ( ~l(u8'a') * l(u8'b') );
	_TyFinal RuleA = l(u8'a');
	_TyFinal RuleB = l(u8'b');
	_TyFinal RuleC = l(u8'c');
	RuleSplit.SetAction( TyTestToken< s_ktidToken0 >() );
	RuleAs.SetAction( TyTestToken< s_ktidToken1 >() );
	RuleA.SetAction( TyTestToken< s_ktidToken2 >() );
	RuleB.SetAction( TyTestToken< s_ktidToken3 >() );
	RuleC.SetAction( TyTestToken< s_ktidToken4 >() );
	_TyFinal All( RuleSplit );
	All.AddRule( RuleAs );
	All.AddRule( RuleA );
	All.AddRule( RuleB );
	All.AddRule( RuleC );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadsplit" );
}

// lookaheadvar: trailing contexts of variable length and a lookahead rule containing triggers.
void
GenerateLookaheadVar()
{
	_TyFinal Name = ++lr(u8'a',u8'z');
	_TyFinal Digits = ++lr(u8'0',u8'9');
	_TyFinal Spaces = ++l(u8' ');
	// A name followed by any number of spaces and then '=' is a key.
	_TyFinal Key = Name / ( ~l(u8' ') * l(u8'=') );
	// "<name" when followed by '>': the token's value is the name.
	_TyFinal OpenTag = ( l(u8'<') * t( TyTestTriggerPosition< s_ktidTriggerABegin >() ) * Name * t( TyTestTriggerString< s_ktidTriggerAEnd, s_ktidTriggerABegin >() ) )
											/ l(u8'>');
	// Digits followed by a unit: "em" or '%'.
	_TyFinal Quantity = Digits / ( ls(u8"em") | l(u8'%') );
	// Note that '<' may not be a token itself: a trigger would then leave an accepting state - which is ambiguous.
	_TyFinal Punct = l(u8'=') | l(u8'>') | l(u8'%');
	Key.SetAction( TyTestToken< s_ktidToken0 >() );
	OpenTag.SetAction( TyTestTokenWithString< s_ktidToken1, s_ktidTriggerAEnd, s_ktidTriggerABegin >() );
	Quantity.SetAction( TyTestToken< s_ktidToken2 >() );
	Name.SetAction( TyTestToken< s_ktidToken3 >() );
	Digits.SetAction( TyTestToken< s_ktidToken4 >() );
	Spaces.SetAction( TyTestToken< s_ktidToken5 >() );
	Punct.SetAction( TyTestToken< s_ktidToken6 >() );
	_TyFinal All( Key );
	All.AddRule( OpenTag );
	All.AddRule( Quantity );
	All.AddRule( Name );
	All.AddRule( Digits );
	All.AddRule( Spaces );
	All.AddRule( Punct );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadvar" );
}

// lookaheadrandom: lookahead rules over the alphabet "ab" which the unit test checks against a simple reference
//	implementation of the same rules using random input.
void
GenerateLookaheadRandom()
{
	// One or more of 'a' or 'b' when followed by one or more 'b's and then an 'a'.
	_TyFinal RuleAny = ++( l(u8'a') | l(u8'b') ) / ( ++l(u8'b') * l(u8'a') );
	// One or more 'a's when followed by a 'b'.
	_TyFinal RuleAs = ++l(u8'a') / l(u8'b');
	_TyFinal RuleA = l(u8'a');
	_TyFinal RuleB = l(u8'b');
	RuleAny.SetAction( TyTestToken< s_ktidToken0 >() );
	RuleAs.SetAction( TyTestToken< s_ktidToken1 >() );
	RuleA.SetAction( TyTestToken< s_ktidToken2 >() );
	RuleB.SetAction( TyTestToken< s_ktidToken3 >() );
	_TyFinal All( RuleAny );
	All.AddRule( RuleAs );
	All.AddRule( RuleA );
	All.AddRule( RuleB );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, "lookaheadrandom" );
}

// lookaheadprecfwd, lookaheadprecrev: lookahead rules which are ambiguous with each other and with rules without
//	lookahead - they match the same input in its entirety. The same rules are generated in two orders so that the tests
//	can check that the order of the rules decides which matches.
//	Token0: "a" when followed by "bc".			Token1: "ab" when followed by "c".		( "abc" is matched by both. )
//	Token2: "abc".																															( "abc" again - always after Token0 and Token1. )
//	Token3: "ab" when followed by "cd".																					( Matches more of "abcd" than the above. )
//	Token4: "cd".														Token5: "c" when followed by "d".			( "cd" is matched by both. )
//	Token6: one or more 'd's when followed by 'e'.
//	Token8: one or more 'd's when followed by 'e' or 'b'.												( "dde" is matched by both. )
//	Token7: any single character of "abcde" - always last.
// lookaheadprecfwd has the rules in the order: Token0, Token1, Token2, Token3, Token4, Token5, Token6, Token8, Token7.
// lookaheadprecrev has the rules in the order: Token1, Token0, Token2, Token3, Token5, Token4, Token8, Token6, Token7.
void
GenerateLookaheadPrecedence( bool _fReverse )
{
	_TyFinal Rule0 = l(u8'a') / ls(u8"bc");
	_TyFinal Rule1 = ls(u8"ab") / l(u8'c');
	_TyFinal Rule2 = ls(u8"abc");
	_TyFinal Rule3 = ls(u8"ab") / ls(u8"cd");
	_TyFinal Rule4 = ls(u8"cd");
	_TyFinal Rule5 = l(u8'c') / l(u8'd');
	_TyFinal Rule6 = ++l(u8'd') / l(u8'e');
	_TyFinal Rule8 = ++l(u8'd') / ( l(u8'e') | l(u8'b') );
	_TyFinal Rule7 = lanyof(u8"abcde");
	Rule0.SetAction( TyTestToken< s_ktidToken0 >() );
	Rule1.SetAction( TyTestToken< s_ktidToken1 >() );
	Rule2.SetAction( TyTestToken< s_ktidToken2 >() );
	Rule3.SetAction( TyTestToken< s_ktidToken3 >() );
	Rule4.SetAction( TyTestToken< s_ktidToken4 >() );
	Rule5.SetAction( TyTestToken< s_ktidToken5 >() );
	Rule6.SetAction( TyTestToken< s_ktidToken6 >() );
	Rule8.SetAction( TyTestToken< s_ktidToken8 >() );
	Rule7.SetAction( TyTestToken< s_ktidToken7 >() );
	std::vector< _TyFinal * > rgpRules;
	if ( !_fReverse )
		rgpRules = { &Rule0, &Rule1, &Rule2, &Rule3, &Rule4, &Rule5, &Rule6, &Rule8, &Rule7 };
	else
		rgpRules = { &Rule1, &Rule0, &Rule2, &Rule3, &Rule5, &Rule4, &Rule8, &Rule6, &Rule7 };
	_TyFinal All( *rgpRules[0] );
	for ( size_t n = 1; n < rgpRules.size(); ++n )
		All.AddRule( *rgpRules[n] );
	_TyDfa dfa;
	_TyDfaCtxt dctxt( dfa );
	GenDfa( All, dfa, dctxt );
	GenerateHeader( dfa, dctxt, _fReverse ? "lookaheadprecrev" : "lookaheadprecfwd" );
}

// multi: more than one DFA in a single generated lexical analyzer - each has its own start state.
void
GenerateMulti()
{
	// The first DFA: words and numbers.
	_TyFinal Word = ++lr(u8'a',u8'z');
	_TyFinal Number = ++lr(u8'0',u8'9');
	Word.SetAction( TyTestToken< s_ktidToken5 >() );
	Number.SetAction( TyTestToken< s_ktidToken6 >() );
	_TyFinal AllFirst( Word );
	AllFirst.AddRule( Number );
	_TyDfa dfaFirst;
	_TyDfaCtxt dctxtFirst( dfaFirst );
	GenDfa( AllFirst, dfaFirst, dctxtFirst );

	// The second DFA: the same rules as the "lookaheadambig" grammar.
	_TyFinal Spaces = ++l(u8' ');
	Spaces.SetAction( TyTestToken< s_ktidToken7 >() );
	_TyFinal AllSecond( Spaces );
	GetLookaheadAmbigRules( AllSecond );
	_TyDfa dfaSecond;
	_TyDfaCtxt dctxtSecond( dfaSecond );
	GenDfa( AllSecond, dfaSecond, dctxtSecond );

  _l_generator< _TyDfa, char >
    gen( egfdStandaloneGenerator, "_lexangtest_multi.h", "LTMULTI", true, "ns_ltmulti", "statemulti", "char8_t", "u8'", "'");
  gen.add_dfa( dfaFirst, dctxtFirst, "startFirst" );
  gen.add_dfa( dfaSecond, dctxtSecond, "startSecond" );
  gen.generate();
}

} // namespace ns_lexangtest

int
main( int argc, char ** argv )
{
	__BIENUTIL_USING_NAMESPACE
	g_strProgramName = *argv;
	try
	{
		n_SysLog::InitSysLog( argv[0], 0, LOG_USER );
		ns_lexangtest::GenerateBasic();
		ns_lexangtest::GenerateTriggers();
		ns_lexangtest::GenerateExcludes();
		ns_lexangtest::GenerateCompletes();
		ns_lexangtest::GenerateLookahead();
		ns_lexangtest::GenerateLookaheadAmbig();
		ns_lexangtest::GenerateLookaheadTwo();
		ns_lexangtest::GenerateLookaheadOverlap();
		ns_lexangtest::GenerateLookaheadSplit();
		ns_lexangtest::GenerateLookaheadVar();
		ns_lexangtest::GenerateLookaheadRandom();
		ns_lexangtest::GenerateLookaheadPrecedence( false );
		ns_lexangtest::GenerateLookaheadPrecedence( true );
		ns_lexangtest::GenerateMulti();
		return 0;
	}
	catch ( const std::exception & rexc )
	{
		fprintf( stderr, "%s: *** Exception: [%s]\n", g_strProgramName.c_str(), rexc.what() );
		return 1;
	}
}

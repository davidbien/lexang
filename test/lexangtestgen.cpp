
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
	// Any number of 'x's when followed by "xy".
	_TyFinal RuleXs = ~l(u8'x') / ls(u8"xy");
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
		ns_lexangtest::GenerateMulti();
		return 0;
	}
	catch ( const std::exception & rexc )
	{
		fprintf( stderr, "%s: *** Exception: [%s]\n", g_strProgramName.c_str(), rexc.what() );
		return 1;
	}
}

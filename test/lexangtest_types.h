#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// lexangtest_types.h
// Types shared between the generator of the test lexical analyzers ( lexangtestgen.cpp ) and the unit tests which use the
//	generated lexical analyzers ( lexanglextest.cpp ): the lexical analyzer traits and the token and trigger identifiers.
// The generator instantiates the action objects with t_fInLexGen == true, the generated code with false.

#include "_l_ns.h"
#include "_l_types.h"
#include "_l_traits.h"
#include "_l_transport.h"
#include "_l_user.h"
#include "_l_axion.h"

namespace ns_lexangtest
{
__LEXOBJ_USING_NAMESPACE

// All of the test lexical analyzers read char8_t from fixed memory.
typedef char8_t TyTestChar;
typedef _l_transport_fixedmem< TyTestChar > TyTestTransport;
typedef _l_default_user_obj< TyTestChar > TyTestUserObj;
typedef _l_traits< TyTestTransport, TyTestUserObj > TyTestLexTraits;

// A token without any data other than the range of characters matched.
template < vtyTokenIdent t_ktid, bool t_fInLexGen = true >
using TyTestToken = _l_action_token< _l_trigger_noop< TyTestLexTraits, t_ktid, t_fInLexGen > >;

// A trigger which records the position at which it fired.
template < vtyTokenIdent t_ktid, bool t_fInLexGen = true >
using TyTestTriggerPosition = _l_trigger_position< TyTestLexTraits, t_ktid, t_fInLexGen >;
// A trigger which records the string between the position of the trigger t_ktidBegin and where it fires.
template < vtyTokenIdent t_ktid, vtyTokenIdent t_ktidBegin, bool t_fInLexGen = true >
using TyTestTriggerString = _l_trigger_string< TyTestLexTraits, t_ktid, t_ktidBegin, t_fInLexGen >;
// A token which saves the string of a single string trigger as its value.
template < vtyTokenIdent t_ktid, vtyTokenIdent t_ktidTriggerEnd, vtyTokenIdent t_ktidTriggerBegin, bool t_fInLexGen = true >
using TyTestTokenWithString = _l_action_token< _l_action_save_data_single< t_ktid, TyTestTriggerString< t_ktidTriggerEnd, t_ktidTriggerBegin, t_fInLexGen > > >;
// A token which saves the strings of two string triggers as its value.
template < vtyTokenIdent t_ktid, vtyTokenIdent t_ktidTriggerEnd0, vtyTokenIdent t_ktidTriggerBegin0,
					 vtyTokenIdent t_ktidTriggerEnd1, vtyTokenIdent t_ktidTriggerBegin1, bool t_fInLexGen = true >
using TyTestTokenWithTwoStrings = _l_action_token< _l_action_save_data_single< t_ktid,
	TyTestTriggerString< t_ktidTriggerEnd0, t_ktidTriggerBegin0, t_fInLexGen >, TyTestTriggerString< t_ktidTriggerEnd1, t_ktidTriggerBegin1, t_fInLexGen > > >;

// A token whose action rejects every token it matches - the matched input is then skipped.
template < class t_TyTraits, vtyTokenIdent t_ktid, bool t_fInLexGen >
struct TestRejectNoop : public _l_trigger_noop< t_TyTraits, t_ktid, t_fInLexGen >
{
	typedef _l_trigger_noop< t_TyTraits, t_ktid, t_fInLexGen > _TyBase;
	using typename _TyBase::_TyActionObjectBase;
	TestRejectNoop() = default;
	TestRejectNoop( _TyActionObjectBase * _paobNext )
		: _TyBase( _paobNext )
	{
	}
	void RenderActionType( ostream & _ros, const char * _pcTraitsName ) const
	{
		return StaticRenderActionType( _ros, _pcTraitsName );
	}
	void RenderActionType( stringstream & _ros, const char * _pcTraitsName ) const
	{
		return StaticRenderActionType( _ros, _pcTraitsName );
	}
	template < class t_TyOStream >
	static void StaticRenderActionType( t_TyOStream & _ros, const char * _pcTraitsName )
	{
		_ros << "ns_lexangtest::TestRejectNoop< " << _pcTraitsName << ", " << t_ktid << ", false >";
	}
	string VStrTypeName( const char * _pcTraitsName ) const
	{
		return StaticStrTypeName( _pcTraitsName );
	}
	static string StaticStrTypeName( const char * _pcTraitsName )
	{
		std::stringstream ss;
		StaticRenderActionType( ss, _pcTraitsName );
		return ss.str();
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		return false;
	}
};
template < vtyTokenIdent t_ktid, bool t_fInLexGen = true >
using TyTestTokenReject = _l_action_token< TestRejectNoop< TyTestLexTraits, t_ktid, t_fInLexGen > >;

// A trigger which appends its id and the position at which it fired to a log - see RgTestTriggerLog().
// The tests of the tagged DFAs use this to check exactly which triggers fired, where and in which order.
struct TestTriggerEvent
{
	vtyTokenIdent m_tid;
	vtyDataPosition m_pos;
	bool operator == ( TestTriggerEvent const & _r ) const
	{
		return ( m_tid == _r.m_tid ) && ( m_pos == _r.m_pos );
	}
};
inline std::vector< TestTriggerEvent > & RgTestTriggerLog()
{
	static std::vector< TestTriggerEvent > s_rgLog;
	return s_rgLog;
}
template < class t_TyTraits, vtyTokenIdent t_ktid, bool t_fInLexGen >
struct TestTriggerLog : public _l_trigger_noop< t_TyTraits, t_ktid, t_fInLexGen >
{
	typedef _l_trigger_noop< t_TyTraits, t_ktid, t_fInLexGen > _TyBase;
	using typename _TyBase::_TyActionObjectBase;
	TestTriggerLog() = default;
	TestTriggerLog( _TyActionObjectBase * _paobNext )
		: _TyBase( _paobNext )
	{
	}
	void RenderActionType( ostream & _ros, const char * _pcTraitsName ) const
	{
		return StaticRenderActionType( _ros, _pcTraitsName );
	}
	void RenderActionType( stringstream & _ros, const char * _pcTraitsName ) const
	{
		return StaticRenderActionType( _ros, _pcTraitsName );
	}
	template < class t_TyOStream >
	static void StaticRenderActionType( t_TyOStream & _ros, const char * _pcTraitsName )
	{
		_ros << "ns_lexangtest::TestTriggerLog< " << _pcTraitsName << ", " << t_ktid << ", false >";
	}
	string VStrTypeName( const char * _pcTraitsName ) const
	{
		return StaticStrTypeName( _pcTraitsName );
	}
	static string StaticStrTypeName( const char * _pcTraitsName )
	{
		std::stringstream ss;
		StaticRenderActionType( ss, _pcTraitsName );
		return ss.str();
	}
	template < class t_TyAnalyzer >
	bool action( t_TyAnalyzer & _rA )
	{
		TestTriggerEvent tte = { t_ktid, _rA.GetCurrentPosition() };
		RgTestTriggerLog().push_back( tte );
		return true;
	}
};
template < vtyTokenIdent t_ktid, bool t_fInLexGen = true >
using TyTestTriggerLog = TestTriggerLog< TyTestLexTraits, t_ktid, t_fInLexGen >;

// Trigger identifiers: these are shared by all of the grammars.
static const vtyTokenIdent s_ktidTriggerABegin = 1;
static const vtyTokenIdent s_ktidTriggerAEnd = 2;
static const vtyTokenIdent s_ktidTriggerBBegin = 3;
static const vtyTokenIdent s_ktidTriggerBEnd = 4;
// Further triggers used by the tests of the tagged DFAs.
static const vtyTokenIdent s_ktidTrigger1 = 11;
static const vtyTokenIdent s_ktidTrigger2 = 12;
static const vtyTokenIdent s_ktidTrigger3 = 13;
static const vtyTokenIdent s_ktidTrigger4 = 14;

// Token identifiers: each grammar uses some subset of these, the meaning of each is given by the grammar.
static const vtyTokenIdent s_ktidToken0 = 1000;
static const vtyTokenIdent s_ktidToken1 = 1001;
static const vtyTokenIdent s_ktidToken2 = 1002;
static const vtyTokenIdent s_ktidToken3 = 1003;
static const vtyTokenIdent s_ktidToken4 = 1004;
static const vtyTokenIdent s_ktidToken5 = 1005;
static const vtyTokenIdent s_ktidToken6 = 1006;
static const vtyTokenIdent s_ktidToken7 = 1007;
static const vtyTokenIdent s_ktidToken8 = 1008;

} // namespace ns_lexangtest

__BIENUTIL_BEGIN_NAMESPACE
// The base class of our trigger type - as is declared for each type of action object in _l_axion.h.
template < class t_TyTraits, __REGEXP_NAMESPACE vtyTokenIdent t_ktid, bool t_fInLexGen >
struct __map_to_base_class< ns_lexangtest::TestTriggerLog< t_TyTraits, t_ktid, t_fInLexGen > >
{
	typedef __REGEXP_NAMESPACE _l_action_object_base< typename t_TyTraits::_TyChar, t_fInLexGen > _TyBase;
};
__BIENUTIL_END_NAMESPACE

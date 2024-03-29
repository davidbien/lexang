#pragma once

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_lxgen.h

// Lexical analyzer generator.

// testing.
//#define LXGEN_OUTPUT_TRIGGERS

#include <ios>
#include <string>
#include <fstream>

__REGEXP_BEGIN_NAMESPACE

// EGeneratorFamilyDisposition:
// We allow families of generators but we need to declare each part specially since only the state machine need be written for the specializations.
enum EGeneratorFamilyDisposition
{
	egfdStandaloneGenerator, // This is a standalone generator - we will only have this generator with this character type.
	egfdBaseGenerator, // This is the base generator of a family of generators each recognizing a different character type.
	egfdSpecializedGenerator, // This is a "specialized" generator for which the "base" generator has already been declared (or will be declared - the order that different headers are generated doesn't matter).
	egfdEGeneratorFamilyDispositionCount // This at the end always.
};

// These to be used as ( 1 << egdoDontTemplatizeStates ).
enum EGeneratorDFAOptions : uint32_t
{
	egdoDontTemplatizeStates, 
		// Don't templatize the states of the state machine. This only works (of course) if there are no action or trigger function pointers associated with the states.
		// If action function pointers are necessary in the state machine then an error is thrown indicating that 
	egdoGeneratorDFAOptionsCount // This at the end always.
};

// _l_gen_action_info: Describes a given action token id.
template < class t_TyCharOut, class t_TyAllocator >
struct _l_gen_action_info
{
	typedef std::basic_string< t_TyCharOut, char_traits<t_TyCharOut>, t_TyAllocator > _TyStdStrOut;
	_l_gen_action_info() = default;
	_l_gen_action_info( _l_gen_action_info const & ) = default;
	_TyStdStrOut m_strActionName;	// Human readable name for the action.
	_TyStdStrOut m_strComment; // Comment to be placed in the generated code.
};

template < class t_TyDfa, class t_TyCharOut >
struct _l_gen_dfa
{
	typedef t_TyDfa _TyDfa;
	typedef t_TyCharOut	_TyCharOut;
	typedef typename _TyDfa::_TyDfaCtxt _TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator _TyAllocator;
	typedef basic_string< t_TyCharOut, char_traits<t_TyCharOut>, _TyAllocator >	_TyString;

	~_l_gen_dfa() = default;
	_l_gen_dfa() = delete;
	_l_gen_dfa( _l_gen_dfa const & ) = default;

	_l_gen_dfa( _TyDfa & _rDfa,
							_TyDfaCtxt & _rDfaCtxt,
							const t_TyCharOut * _pcStartStateName,
							uint32_t _grfGeneratorDFAOptions,
							_TyAllocator const & _rA )
		: m_rDfa( _rDfa ),
			m_rDfaCtxt( _rDfaCtxt ),
			m_grfGeneratorDFAOptions( _grfGeneratorDFAOptions ),
			m_sStartStateName( _pcStartStateName, _rA )
	{
		m_rDfa._CreateRangeLookup();
		m_rDfaCtxt.CreateAcceptPartLookup();
		VerifyThrowSz( !FDontTemplatizeStates() || !m_rDfa.m_nTriggers, 
			"Must have templatized states when triggers are present in the state machine. There are [%llu] triggers in the current DFA.", uint64_t(m_rDfa.m_nTriggers) );
	}
	bool FDontTemplatizeStates() const
	{
		return !!( ( 1ul << egdoDontTemplatizeStates ) & m_grfGeneratorDFAOptions );
	}
	_TyDfa & m_rDfa;
	_TyDfaCtxt & m_rDfaCtxt;
	_TyString m_sStartStateName;	// Special name for start state.
	uint32_t m_grfGeneratorDFAOptions;
};

template < class t_TyDfa, class t_TyCharOut >
struct _l_generator
{
	typedef t_TyDfa	_TyDfa;
	typedef t_TyCharOut _TyCharOut;
	typedef typename _TyDfa::_TyState _TyState;
	typedef typename _TyDfa::_TyDfaCtxt _TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator _TyAllocator;
	typedef typename _TyDfa::_TyNodeLookup _TyNodeLookup;
	typedef typename _TyDfa::_TyRange _TyRange;
	typedef typename _TyDfa::_TyGraph _TyGraph;
	typedef typename _TyDfa::_TyGraphNode _TyGraphNode;
	typedef typename _TyDfa::_TyChar _TyCharGen;
	typedef typename _TyDfa::_TyRangeEl _TyRangeEl;
	typedef typename _TyDfa::_TyAlphaIndex _TyAlphaIndex;
	typedef typename _TyDfa::_TyActionObjectBase _TyActionObjectBase;
	typedef typename _TyDfa::_TyAcceptAction _TyAcceptAction;
	typedef typename _TyAcceptAction::_TySetActionIds _TySetActionIds;

	typedef typename _TyDfaCtxt::_TyPartAcceptStates	_TyPartAcceptStates;
	typedef typename _TyDfaCtxt::_TyAcceptPartLookup	_TyAcceptPartLookup;

	typedef basic_string< t_TyCharOut, char_traits<t_TyCharOut>, _TyAllocator >	_TyString;
	typedef std::pair< _TyAlphaIndex, _TyAlphaIndex > _TyPrAI;

	typedef _l_gen_dfa< _TyDfa, t_TyCharOut >		_TyGenDfa;

  typedef typename _Alloc_traits< typename list< _TyGenDfa >::value_type, _TyAllocator >::allocator_type _TyDfaListAllocator;
	typedef list< _TyGenDfa, _TyDfaListAllocator > _TyDfaList;
	_TyDfaList m_lDfaGen;
	typename _TyDfaList::value_type *	m_pvtDfaCur{nullptr};

	EGeneratorFamilyDisposition m_egfdFamilyDisp; // Our disposition wrt to same generators with other character types.
	_TyString m_sfnHeader;			// The header file we are generating.
	_TyString m_sPpBase;				// Preprocessor base name ( for #defines ).
	bool m_fUseNamespaces;	// Whether to use namespaces when generating.
	_TyString m_sNamespace;			// The namespace into which we generate.
	_TyString m_sBaseStateName;	// The base name for state variables.
	_TyString m_sCharTypeName;		// The name of the character type for the analyzer we are generating,
	_TyString m_sCharTypeNameHumanReadable; // "Human readable" version of the character type.
	_TyString m_sStateProtoTypedef; // We name the vTyStateProto including the character type name on the end since we allow "families" of lexical analyzers for all character types.
	_TyString m_sVisibleCharPrefix;	// Prefix for a visible character const.
	_TyString m_sVisibleCharSuffix;	// Suffix for a visible character const.
	_TyPrAI m_praiTriggers; // The range of trigger alpha indices (not vtyActionIdent, btw.)
	vtyActionIdent m_aiStart;
	typename _TyDfa::_TyState m_stStart;

	bool m_fLookaheads;
	bool m_fTriggers;

	typedef _l_gen_action_info< _TyCharOut, _TyAllocator > _TyGenActionInfo;

	// We insert the actions into a map. They are ordered by the unique token/trigger id.
	// We map to the _TyGenActionInfo (name,comment,etc.) for the given action, and whether the action has been referenced by any DFA.
	typedef pair< _TyGenActionInfo, bool > _TyMAValue;
	typedef _ref< const _TyActionObjectBase > _TyRefActionObjectBase;
  typedef typename _Alloc_traits< typename map< _TyRefActionObjectBase, _TyMAValue,
                                                less< _TyRefActionObjectBase > >::value_type, _TyAllocator >::allocator_type _TyMapActionsAllocator;
	typedef map< _TyRefActionObjectBase, _TyMAValue, 
								less< _TyRefActionObjectBase >,_TyMapActionsAllocator > _TyMapActions;
	_TyMapActions	m_mapActions;	

	// Action info map - provide human readable identifiers instead of just numbers.
  typedef typename _Alloc_traits< typename map< vtyTokenIdent, _TyGenActionInfo >::value_type, _TyAllocator >::allocator_type _TyMapActionInfoAllocator;
	typedef std::map< vtyTokenIdent, _TyGenActionInfo, less< vtyTokenIdent >, _TyMapActionInfoAllocator > _TyMapActionInfo;
	_TyMapActionInfo m_mapActionInfo;

	_l_generator( EGeneratorFamilyDisposition _egfdFamilyDisp,
								const t_TyCharOut * _pcfnHeader,
								const t_TyCharOut *	_pcPpBase,
								bool _fUseNamespaces,
								const t_TyCharOut * _pcNamespace,
								const t_TyCharOut * _pcBaseStateName,
								const t_TyCharOut * _pcCharTypeName,
								const t_TyCharOut * _pcVisibleCharPrefix,
								const t_TyCharOut * _pcVisibleCharSuffix,
                _TyAllocator const & _rA = _TyAllocator() )
		: m_lDfaGen( _rA ),
			m_egfdFamilyDisp( _egfdFamilyDisp ),
			m_sfnHeader( _pcfnHeader, _rA ),
			m_sPpBase( _pcPpBase, _rA ),
			m_fUseNamespaces( _fUseNamespaces ),
			m_sNamespace( _pcNamespace, _rA ),
			m_sBaseStateName( _pcBaseStateName, _rA ),
			m_sCharTypeName( _pcCharTypeName, _rA ),
			m_sVisibleCharPrefix( _pcVisibleCharPrefix, _rA ),
			m_sVisibleCharSuffix( _pcVisibleCharSuffix, _rA ),
			m_aiStart( 0 ),
			m_stStart( 0 ),
			m_fLookaheads( false ),
			m_fTriggers( false ),
      m_mapActions( typename _TyMapActions::key_compare(), _rA ),
			m_mapActionInfo( typename _TyMapActionInfo::key_compare(), _rA )
	{
		if ( m_sCharTypeName == "char32_t" )
			m_sCharTypeNameHumanReadable = "Char32";
		else
		if ( m_sCharTypeName == "char16_t" )
			m_sCharTypeNameHumanReadable = "Char16";
		else
		if ( m_sCharTypeName == "char8_t" )
			m_sCharTypeNameHumanReadable = "Char8";
		else
		if ( m_sCharTypeName == "char" )
			m_sCharTypeNameHumanReadable = "Char";
		else
		if ( m_sCharTypeName == "wchar_t" )
			m_sCharTypeNameHumanReadable = "WChar";
		else
			VerifyThrowSz( false, "Unknown character type [%s].", m_sCharTypeName.c_str() );

		// We name the vTyStateProto including the character type name on the end since we allow "families" of lexical analyzers for all character types.
		m_sStateProtoTypedef = "vTyStateProto";
		m_sStateProtoTypedef += m_sCharTypeNameHumanReadable;
	}

  _TyAllocator  get_allocator()
  {
    return m_mapActions.get_allocator();
  }
	bool FIsStandaloneGenerator() const
	{
		return egfdStandaloneGenerator == m_egfdFamilyDisp;
	}
	bool FIsSpecializedGenerator() const
	{
		return egfdSpecializedGenerator == m_egfdFamilyDisp;
	}

enum EGeneratorDFAOptions : uint32_t
{
	egdoDontTemplatizeStates = 0x00000001, 
		// Don't templatize the states of the state machine. This only works (of course) if there are no action or trigger function pointers associated with the states.
		// If action function pointers are necessary in the state machine then an error is thrown indicating that 
	egdoGeneratorDFAOptionsCount // This at the end always.
};


	void add_dfa(	_TyDfa & _rDfa,
								_TyDfaCtxt & _rDfaCtxt,
								const t_TyCharOut * _pcStartStateName,
								uint32_t _grfGeneratorDFAOptions = 0 )
	{
		m_lDfaGen.push_back( _TyGenDfa( _rDfa, _rDfaCtxt, 
																		_pcStartStateName, _grfGeneratorDFAOptions,
                                    m_mapActions.get_allocator() ) );
		m_fLookaheads = m_fLookaheads || _rDfa.m_fHasLookaheads;
		m_fTriggers = m_fTriggers || !!_rDfa.m_nTriggers;
	}

	void add_action_info( vtyTokenIdent _tid, _TyGenActionInfo const & _rgai )
	{
		pair< typename _TyMapActionInfo::iterator, bool > pib = m_mapActionInfo.insert( _TyMapActionInfo::value_type( _tid, _rgai ) );
		VerifyThrowSz( pib.second, "TokenId[%d] has already been populated in the map.", _tid );
		// There is the possibility that the caller has duplicated a m_strActionName but if so the resultant code won't compile so no worries there.
	}

	void	generate()
	{
    _unique_actions();

    ofstream ofsHeader( m_sfnHeader.c_str() );

		_HeaderHeader( ofsHeader );

		ostringstream ossStateDefinitions; // Stream these to a string first because they reference the unique action objects.
		{ //B Generate state declarations.
			Assert( !m_aiStart );
			Assert( !m_stStart );
			for ( typename _TyDfaList::iterator lit = m_lDfaGen.begin();
						lit != m_lDfaGen.end(); ++lit )
			{
				m_pvtDfaCur = &*lit;

				m_pvtDfaCur->m_rDfa.GetTriggerUnsatAIRanges( &m_praiTriggers, nullptr );

				_GenStateDecls( ofsHeader );
				_GenStateDefinitions( ossStateDefinitions );

				m_aiStart += m_pvtDfaCur->m_rDfa.m_iMaxActions;
				m_stStart += m_pvtDfaCur->m_rDfa.NStates();
			}
		} //EB

		if ( !FIsSpecializedGenerator() )
			_HeaderBody( ofsHeader );

		ofsHeader << ossStateDefinitions.str();

		_HeaderFooter( ofsHeader );

    // Clear the associated rules:
    m_lDfaGen.clear();
	}

	// We expect completely unique actions at least at this point as we are translating to trigger/token numbers
	//	and using these trigger/token numbers in the implementation rather than the action ids. There are no action IDs
	//	in the impl - only trigger/token numbers/ids.
	// As we peruse the actions we look up any human-readable names that they might have been given.
	void	_unique_actions()
	{
		for ( typename _TyDfaList::iterator lit = m_lDfaGen.begin();
					lit != m_lDfaGen.end(); ++lit )
		{
			m_pvtDfaCur = &*lit;

			{
				typename _TyPartAcceptStates::iterator itAction = m_pvtDfaCur->m_rDfaCtxt.m_partAccept.begin();
				typename _TyPartAcceptStates::iterator itActionEnd = m_pvtDfaCur->m_rDfaCtxt.m_partAccept.end();
				for ( ; itAction != itActionEnd; ++itAction )
				{
					typename _TyPartAcceptStates::value_type & rvt = *itAction;
					rvt.second.AssertValid();
					if ( rvt.second.m_pSdpAction )
					{
						// We support non-templated states only when action object don't have an associated object, but just a token it.
						VerifyThrowSz( !m_pvtDfaCur->FDontTemplatizeStates() || (*rvt.second.m_pSdpAction)->FIsTokenIdOnly(), 
							"Must templatize states when any actions objects are present in the state machine as the callback member functions depend on the type of the lexical analyzer." );
						//Assert( !( e_aatTrigger & rvt.second.m_eaatType ) ); // Should see only non-trigger actions here.
						if ( !(*rvt.second.m_pSdpAction)->FIsTokenIdOnly() )
						{
							typename _TyMapActionInfo::const_iterator citAxnInfo = m_mapActionInfo.find( (*rvt.second.m_pSdpAction)->VGetTokenId() );
							_TyGenActionInfo gaiInfo;
							if ( m_mapActionInfo.end() == citAxnInfo )
							{
								// Just use the action id - leave the comment empty.
								PrintfStdStr( gaiInfo.m_strActionName, ( e_aatTrigger & rvt.second.m_eaatType ) ? "Trigger%d" : "Token%d", (*rvt.second.m_pSdpAction)->VGetTokenId() );
							}
							else
							{
								gaiInfo = citAxnInfo->second;
							}
							Trace( "Adding action [%s].", (*rvt.second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str() );
							std::pair< typename _TyMapActions::iterator, bool > pib = m_mapActions.insert( typename _TyMapActions::value_type( **rvt.second.m_pSdpAction,
								typename _TyMapActions::mapped_type( gaiInfo, false ) ) );
							// Apparently we can have duplicate actions here as well...
							//VerifyThrowSz( pib.second, "(type,TokenId)[%s,%d] are not unique", (*rvt.second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str(), (*rvt.second.m_pSdpAction)->VGetTokenId() );
						}
					}
					else
						Trace( "No action object for action id [%u].", rvt.second.GetOriginalActionId() );
				}
			}
			
			// Now the triggers:
			if ( m_pvtDfaCur->m_rDfa.m_pMapTriggers )
			{
				typename _TyDfa::_TyMapTriggers::iterator itTriggerEnd = m_pvtDfaCur->m_rDfa.m_pMapTriggers->end();
				for ( typename _TyDfa::_TyMapTriggers::iterator itTrigger = m_pvtDfaCur->m_rDfa.m_pMapTriggers->begin();
							itTrigger != itTriggerEnd; 
							++itTrigger )
				{
					typename _TyDfa::_TyMapTriggers::value_type & rvt = *itTrigger;
					Assert( e_aatTrigger == rvt.second.m_eaatType ); // Should see only triggers here.
					rvt.second.AssertValid();
					typename _TyMapActionInfo::const_iterator citAxnInfo = m_mapActionInfo.find( (*rvt.second.m_pSdpAction)->VGetTokenId() );
					_TyGenActionInfo gaiInfo;
					if ( m_mapActionInfo.end() == citAxnInfo )
					{
						// Just use the action id - leave the comment empty.
						PrintfStdStr( gaiInfo.m_strActionName, "Trigger%d", (*rvt.second.m_pSdpAction)->VGetTokenId() );
					}
					else
					{
						gaiInfo = citAxnInfo->second;
					}
					std::pair< typename _TyMapActions::iterator, bool > pib = m_mapActions.insert( typename _TyMapActions::value_type(	**rvt.second.m_pSdpAction, 
						typename _TyMapActions::mapped_type( gaiInfo, false ) ) );
					if ( pib.second )
						Trace( "Added trigger [%s].", (*rvt.second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str() );
					// REVIEW: Note that some of these triggers were added above so they may not be unique here.
					//VerifyThrowSz( pib.second, "(type,TokenId)[%s,%d] are not unique", (*rvt.second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str(), (*rvt.second.m_pSdpAction)->VGetTokenId() );
				}
			}
		}
	}

	void	_HeaderHeader( ostream & _ros )
	{
		_ros << "#pragma once\n\n";
		_ros << "// " << m_sfnHeader << "\n";
		_ros << "// Generated DFA.\n";
		_ros << "\n";
		_ros << "#include \"_l_lxobj.h\"\n";
		_ros << "#include \"_l_token.h\"\n";
		_ros << "\n";
		if ( m_fUseNamespaces )
		{
			if ( !FIsSpecializedGenerator() )
			{
				_ros << "#define __" << m_sPpBase << "_BEGIN_NAMESPACE namespace " << m_sNamespace << " {\n";
				_ros << "#define __" << m_sPpBase << "_END_NAMESPACE }\n";
				_ros << "#define __" << m_sPpBase << "_USING_NAMESPACE using namespace " << m_sNamespace << ";\n";
				_ros << "#define __" << m_sPpBase << "_NAMESPACE " << m_sNamespace << "\n";
				_ros << "\n";
			}
			_ros << "__" << m_sPpBase << "_BEGIN_NAMESPACE\n";
			_ros << "__LEXOBJ_USING_NAMESPACE\n";
			_ros << "\n";
		}

		if ( !FIsSpecializedGenerator() )
		{
			_ros << "template < class t_TyTraits >\nusing TGetAnalyzerBase = _l_analyzer< t_TyTraits"
						<< ( m_fLookaheads ? ", true" : ", false" )
						<< ( m_fTriggers ? ", true" : ", false" )
						<< " >;\n";
		}

		_ros << "typedef _l_state_proto< " << m_sCharTypeName << " > " << m_sStateProtoTypedef << ";\n";
		_ros << "\n";
	}

	void	_HeaderBody( ostream & _ros )
	{
		m_pvtDfaCur = &m_lDfaGen.front();

 		_ros << "\n";
		_ros << "template < class t_TyTraits >\n";
		_ros << "class _lexical_analyzer : public TGetAnalyzerBase<t_TyTraits>\n";
		_ros << "{\n";
		_ros << "\ttypedef TGetAnalyzerBase<t_TyTraits> _TyBase;\n";
		_ros << "\ttypedef _lexical_analyzer _TyThis;\n";
		_ros << "public:\n";
		_ros << "\ttypedef t_TyTraits _TyTraits;\n";
		_ros << "\ttypedef typename _TyTraits::_TyChar _TyChar;\n";

		// Declare the base GetActionObject() template that is not implemented.
		_ros << "\n\ttemplate < const int t_kiAction >\n\t_l_action_object_base< _TyChar, false > & GetActionObj();\n";


		// We generate all referenced unique actions, and link them in a singly-linked list.
		string strPreviousAction;
		typename _TyMapActions::iterator itMAEnd = m_mapActions.end();
		typename _TyMapActions::iterator itMA = m_mapActions.begin();
		for ( ; itMA != itMAEnd; ++itMA )
		{
			typename _TyMapActions::value_type & rvt = *itMA;
			if ( rvt.second.second )
			{
				// Define a constant so that we don't just have raw numbers lying around as much:
				_ros << "\n\tstatic constexpr vtyTokenIdent s_kti" << rvt.second.first.m_strActionName.c_str() << " = " << rvt.first->VGetTokenId() << ";\n";
				// Get a typedef for the action object for ease of use:
				_ros << "\tusing _TyAction" << rvt.second.first.m_strActionName.c_str() << " = ";
				rvt.first->RenderActionType( _ros, "t_TyTraits" );
				_ros << ";\n";
				// Define the object as a member of the lexical analyzer class:
				_ros << "\t_TyAction" << rvt.second.first.m_strActionName.c_str() << " m_axn" << rvt.second.first.m_strActionName.c_str();
				if ( strPreviousAction.length() )
					_ros << "{ " << strPreviousAction << " }";
				_ros << ";\n";
				{//B
					ostringstream oss;
					oss << "&m_axn"	<< rvt.second.first.m_strActionName.c_str();
					strPreviousAction = oss.str();
				}//EB
				_ros << "\ttemplate < >\n\t_l_action_object_base< _TyChar, false > & GetActionObj< s_kti" << rvt.second.first.m_strActionName.c_str() << " >()"
							" { return m_axn" << rvt.second.first.m_strActionName.c_str() << "; }\n";
				_ros << "\tbool Action" << rvt.second.first.m_strActionName.c_str() << "()\n";
				_ros << "\t{\n";
				_ros << "\t\treturn " << "m_axn" << rvt.second.first.m_strActionName.c_str()
								<< ".action( *this );\n";
				_ros << "\t}";
			}
		}

		_ros << "\n\t_lexical_analyzer()\n";
		if ( FIsStandaloneGenerator() )
		{
			// For a standalone generator we can reference the start state here because there is only one unique start state.
			_ros << "\t\t: _TyBase( (" << m_sStateProtoTypedef << "*) & " << m_pvtDfaCur->m_sStartStateName << "<t_TyTraits>, "; 
		}
		else
		{
			// For a base generator we cannot reference the start state as it is unclear which start state will be used by the impl depending on the character type being parsed.
			_ros << "\t\t: _TyBase( nullptr, "; 
		}
		if ( strPreviousAction.length() )
			_ros << strPreviousAction;
		else
			_ros << "nullptr;";
		_ros << " )\n\t{ }\n";

		_ros << "};\n\n";
		_ros << "template < class t_TyTraits >\n";
		_ros << "using TGetLexicalAnalyzer = _lexical_analyzer<t_TyTraits>;\n";
		_ros << "\n";
	}

	void	_HeaderFooter( ostream & _ros )
	{
		if ( m_fUseNamespaces )
		{
			_ros << "__" << m_sPpBase << "_END_NAMESPACE\n";
			_ros << "\n";
		}
	}

	void	_GenHeaderState(	ostream & _ros, _TyGraphNode * _pgn, 
													int _nOuts, bool _fAccept )
	{
		if ( !m_pvtDfaCur->FDontTemplatizeStates() )
			_ros	<< "template < class t_TyTraits >\n";
		_ros	<< "extern ";
		bool fIsTriggerAction, fIsTriggerGateway, fIsAntiAcceptingState;
		vtyTokenIdent tidTokenTrigger; // These get recorded if there is a trigger in the transitions.
		_TyRangeEl rgelTrigger; 
		_GenStateType( _ros, _pgn, _nOuts, _fAccept, fIsTriggerAction, fIsTriggerGateway, fIsAntiAcceptingState, tidTokenTrigger, rgelTrigger );
		if ( _pgn == m_pvtDfaCur->m_rDfaCtxt.m_pgnStart )
		{
			// Use the special start state name:
			_ros << "\t\t" << m_pvtDfaCur->m_sStartStateName << ";\n";
		}
		else
		{
			_ros << "\t\t" << m_sBaseStateName << "_" << ( _pgn->RElConst() + m_stStart ) << ";\n";
		}
	}

	bool	_FVisible( _TyRangeEl const & _r )
	{
		// There are some visible beyond 127 - but this takes care of the bulk.
 		return	( _r >= _TyRangeEl( ' ' ) ) && ( _r < _TyRangeEl( 127 ) ) && 
						( _r != _TyRangeEl( '\\' ) ) && ( _r != _TyRangeEl( '\'' ) );
	}

	void	_CharOut( ostream & _ros, _TyRangeEl const & _r )
	{
		// If it is a visible character we will print it nicely:
		if ( _FVisible( _r ) )
		{
			t_TyCharOut	zeroterm[ 2 ] = { t_TyCharOut( _r ), 0 };
			_TyString	sVisible( zeroterm );
			_ros << m_sVisibleCharPrefix << sVisible << m_sVisibleCharSuffix;
		}
		else
		{
			_ros << "0x0" << hex << unsigned( _r ) << dec;
		}
	}
	bool _FIsTrigger( _TyRangeEl const & _rre ) const
	{
		Assert(m_praiTriggers.first >= 0);
		Assert(m_praiTriggers.second >= 0);
		return ( _rre >= (_TyRangeEl)m_praiTriggers.first ) && ( _rre < (_TyRangeEl)m_praiTriggers.second );
	}
	void	_GenStateType(	ostream & _ros, _TyGraphNode * _pgn, 
												int _nOuts, bool _fAccept, bool & _rfIsTriggerAction, bool & _rfIsTriggerGateway, bool & _rIsAntiAcceptingState,
												vtyTokenIdent & _rtidTokenTrigger, _TyRangeEl & _rrgelTrigger )
	{
		const typename _TyPartAcceptStates::value_type *	pvtAction = 0;
		if ( _fAccept )
		{
			pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _pgn->RElConst() );
			if ( !!pvtAction && !!pvtAction->second.m_pSdpAction )
				Trace( "State[%zu] Action[%s] pvtAction[0x%zx]", size_t(_pgn->RElConst()), (*pvtAction->second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str(), pvtAction );
		}
		_rfIsTriggerAction = ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) );		
		// If this is an anti-accepting state then that status overrides all trigger out transitions 
		//	- since a trigger would fire on the same condition that the anti-accepting state is accepted.
		_rIsAntiAcceptingState = ( pvtAction && ( ( pvtAction->second.m_eaatType & ~e_aatTrigger ) == e_aatAntiAccepting ) );
		_rfIsTriggerGateway = false;
		_rtidTokenTrigger = (numeric_limits< vtyTokenIdent >::max)();
		_rrgelTrigger = 0;
#ifndef LXGEN_OUTPUT_TRIGGERS
		// We check for trigger links at the beginning of the the link list.
		// We should only ever see at most one. If we see an odd one then we expect the
		//	type of this node to be a trigger action, otherwise we expect this node to *not*
		//  be a trigger action.
		// We should never see that this node has any other action when there are trigger transitions
		//	leaving it.
		{//B
			typename _TyGraph::_TyLinkPosIterConst	lpi( _pgn->PPGLChildHead() );
			if ( !lpi.FIsLast() )
			{
				if ( _FIsTrigger( *lpi ) )
				{
					_TyRange r = m_pvtDfaCur->m_rDfa.LookupRange( *lpi );

					typename _TyDfa::_TyMapTriggerTransitionToTokenId::const_iterator cit = m_pvtDfaCur->m_rDfa.m_mapTriggerTransitionToTokenId.find( r.first );
					VerifyThrowSz( cit != m_pvtDfaCur->m_rDfa.m_mapTriggerTransitionToTokenId.end(), "Couldn't find trigger transition [0x%zx] in m_mapTriggerTransitionToTokenId.", size_t(r.first) );
					_rtidTokenTrigger = cit->second;
					_rrgelTrigger = r.first;

					Assert( r.second == r.first );
					_rfIsTriggerGateway = !( r.first % 2 ); // We are entering into a triggered state.
					VerifyThrowSz( _rfIsTriggerAction == !_rfIsTriggerGateway, "We expect odd triggers out of trigger states and even triggers into trigger states." );
					VerifyThrowSz( _rfIsTriggerAction || !_fAccept || _rIsAntiAcceptingState, "A trigger is leaving an accept state - this is an ambiguous situation.");
					VerifyThrowSz( !_rIsAntiAcceptingState || _rfIsTriggerGateway, "An action trigger ambiguous with an anti-accepting state.");
					if ( lpi.NextChild(), !lpi.FIsLast() )
						VerifyThrowSz( !_FIsTrigger( *lpi ), "More than one trigger leaving a state." );
				}
			}
		}//EB
		if ( _rfIsTriggerAction || _rfIsTriggerGateway )
		{
			--_nOuts;	// The zeroth transition is the trigger.
		}
#endif // !LXGEN_OUTPUT_TRIGGERS

		_ros	<< "_l_state< " << m_sCharTypeName << ", " 
					<< _nOuts << ", ";
		if ( pvtAction )
		{
			switch( pvtAction->second.m_eaatType & ~e_aatTrigger )
			{
				case e_aatAccept:
				{
					// Just an accept state:
					_ros << "true, false, 0";
				}
				break;
				case e_aatLookahead:
				{
					_ros << "true, true, 0";
				}
				break;
				case e_aatLookaheadAcceptAndAccept:
				case e_aatLookaheadAcceptAndLookahead:
				case e_aatLookaheadAccept:
				{
					_ros << "true, true, " 
						<< ( pvtAction->second.m_psrRelated ? 
									( pvtAction->second.m_psrRelated->size_bytes() / sizeof( vtyLookaheadVector ) ) :
									0 );
				}
				break;
				default:
				{
					_ros << "false, false, 0";
				}
				break;
			}

			if ( _rfIsTriggerAction || _rfIsTriggerGateway )
			{
				// Then we have some triggers:
				_ros << ", ";
				if ( _rfIsTriggerAction && pvtAction->second.m_psrTriggers )
				{
          Assert(pvtAction->second.m_psrTriggers->countsetbits());
					size_t stBits = pvtAction->second.m_psrTriggers->countsetbits();
					if ( ( stBits > 1 ) && ( _rtidTokenTrigger != (numeric_limits< vtyTokenIdent >::max)() ) )
					{
						// Then we are going to filter out the actions that do not match the input trigger transition and we are going to print a warning because this
						//	is very likely a bug:
						auto stEnd = pvtAction->second.m_psrTriggers->size();
						for ( auto stTrigger = pvtAction->second.m_psrTriggers->getfirstset(); 
									stEnd != stTrigger; stTrigger = pvtAction->second.m_psrTriggers->getnextset( stTrigger ) )
						{
							typename _TyDfa::_TyMapTriggers::iterator itTrigger = m_pvtDfaCur->m_rDfa.m_pMapTriggers->find( (vtyActionIdent)stTrigger );
							if ( (*itTrigger->second.m_pSdpAction)->VGetTokenId() != _rtidTokenTrigger )
							{
								n_SysLog::Log( eslmtError, "%s: Filtering out token [%d] since it doesn't match the input transition. This looks like a bug in NFA->DFA conversion.", 
									FUNCTION_PRETTY_NAME, (*itTrigger->second.m_pSdpAction)->VGetTokenId() );
								pvtAction->second.m_psrTriggers->clearbit( stTrigger );
							}
							// else we leave the bit.
						}
						Assert( 1 == pvtAction->second.m_psrTriggers->countsetbits() );
					}
					_ros << pvtAction->second.m_psrTriggers->countsetbits();
				}
				else
				{
					_ros << ( _rfIsTriggerAction ? "1" : "0" ); // No trigger actions are associated with a gateway.
				}
				_ros << " >";
			}
			else
			{
				_ros << ", 0 >";
			}
		}
		else
		{
			_ros << "false, false, 0, 0 >";
		}
	}

	void	_GenImpState(	ostream & _ros, _TyGraphNode * _pgn, 
											int _nOutsOrig, bool _fAccept )
	{
		_ros << "typedef ";
		bool fIsTriggerAction, fIsTriggerGateway, fIsAntiAcceptingState;
		vtyTokenIdent tidTokenTrigger; // These get recorded if there is a trigger in the transitions.
		_TyRangeEl rgelTrigger; 
		_GenStateType( _ros, _pgn, _nOutsOrig, _fAccept, fIsTriggerAction, fIsTriggerGateway, fIsAntiAcceptingState, tidTokenTrigger, rgelTrigger );
		_ros << " _Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart ) << ";\n";
		if ( !m_pvtDfaCur->FDontTemplatizeStates() )
			_ros << "template < class t_TyTraits > ";
		_ros << "inline\n";
		_ros << "_Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart ) << " ";
		const typename _TyPartAcceptStates::value_type *	pvtAction = 0;
		if ( _pgn == m_pvtDfaCur->m_rDfaCtxt.m_pgnStart )
		{
			_ros	<< m_pvtDfaCur->m_sStartStateName;
		}
		else
		{
			_ros	<< m_sBaseStateName << "_" << ( _pgn->RElConst() + m_stStart );
		}
		_ros << " = {\n#ifdef LXOBJ_STATENUMBERS\n\t" << ( _pgn->RElConst() + m_stStart ) << ",\n#endif //LXOBJ_STATENUMBERS\n\t";
		int _nOuts = _nOutsOrig;
		if ( _fAccept )
		{
			pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _pgn->RElConst() );
			Assert( !!pvtAction );
			if ( !!pvtAction->second.m_pSdpAction )
				Trace( "State[%zu] Action[%s] pvtAction[0x%zx]", size_t(_pgn->RElConst()), (*pvtAction->second.m_pSdpAction)->VStrTypeName( m_sCharTypeName.c_str() ).c_str(), pvtAction );
		}
#ifndef LXGEN_OUTPUT_TRIGGERS
		if ( fIsTriggerAction || fIsTriggerGateway )
		{
			--_nOuts;	// The zeroth transition is the trigger.
		}
#endif //!LXGEN_OUTPUT_TRIGGERS
		_ros	<< _nOuts << ", ";
		if ( fIsTriggerAction )
		{
			if ( pvtAction->second.m_psrTriggers )
			{
				Assert( pvtAction->second.m_psrTriggers->countsetbits() );
				_ros << pvtAction->second.m_psrTriggers->countsetbits();
			}
			else
			{
				_ros << "1";
			}
			_ros << ", ";
		}
		else
		{
			_ros << "0, ";
		}

		if ( _fAccept )
		{
			switch( pvtAction->second.m_eaatType & ~e_aatTrigger )
			{
				case e_aatAccept:
				{
					_ros << "kucAccept, ";
				}
				break;
				case e_aatLookahead:
				{
					_ros << "kucLookahead, ";
				}
				break;
				case e_aatLookaheadAccept:
				{
					_ros << "kucLookaheadAccept, ";
				}
				break;
				case e_aatLookaheadAcceptAndAccept:
				{
					_ros << "kucLookaheadAcceptAndAccept, ";
				}
				break;
				case e_aatLookaheadAcceptAndLookahead:
				{
					_ros << "kucLookaheadAcceptAndLookahead, ";
				}
				break;
				case e_aatAntiAccepting:
				{
					_ros << "kucAntiAccepting, ";
				}
				break;
				default:
				{
					_ros << "0, ";
				}
				break;
			}
		}
		else
		{
			_ros << "0, ";
		}
		if ( fIsTriggerAction || ( fIsTriggerGateway && !fIsAntiAcceptingState ) )
		{
			// Then the first transition is the trigger:
			_ros << "(" << m_sStateProtoTypedef << "*)( & ";
			if ( (*(_pgn->PPGLChildHead()))->PGNChild() == m_pvtDfaCur->m_rDfaCtxt.m_pgnStart )
			{
				_ros	<< m_pvtDfaCur->m_sStartStateName << "<t_TyTraits>";
			}
			else
			{
				_ros	<< m_sBaseStateName << "_" << 
					( (*(_pgn->PPGLChildHead()))->PGNChild()->RElConst() + m_stStart );
				_ros << "<t_TyTraits>";
			}
			_ros << " ), ";
		}
		else
		{
			_ros << "0, ";
		}

		// Offsets for accept and trigger transitions in the variable length struct:
		if ( pvtAction )
		{
			if (  ( pvtAction->second.m_eaatType & ~e_aatTrigger ) &&
					  ( e_aatAntiAccepting != 
							( pvtAction->second.m_eaatType & ~e_aatTrigger ) ) )
			{
				_ros << "\n\toffsetof( ";
				_ros << "_Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart );
				_ros << ", m_pmfnAccept ), ";
			}
			else
			{
				_ros << "\n\t0, "; 
			}
			if ( pvtAction->second.m_eaatType & e_aatTrigger )
			{
				_ros << "\n\toffsetof( ";
				_ros << "_Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart );
				_ros << ", m_rgpmfnTriggers )";
			}
			else
			{	
				_ros << " 0";
			}
			_ros << ", ";
			if ( !!pvtAction->second.m_pSdpAction )
				_ros << (*pvtAction->second.m_pSdpAction)->VGetTokenId();
			else
				_ros << "0";

		}
		else
		{
			_ros << "0, 0, vktidInvalidIdToken";
		}

		if ( _nOuts )
		{
			_ros << ",\n";
			_ros << "\t{\n";
			typename _TyGraph::_TyLinkPosIterNonConst	lpi( _pgn->PPGLChildHead() );
#ifndef LXGEN_OUTPUT_TRIGGERS
			if ( fIsTriggerAction || fIsTriggerGateway )
			{
				lpi.NextChild();	// Skip the trigger transition.
			}
#endif // !LXGEN_OUTPUT_TRIGGERS

			while ( !lpi.FIsLast() )
			{
				_TyRange r = m_pvtDfaCur->m_rDfa.LookupRange( *lpi );
				_ros	<< "\t\t{ ";
				_CharOut( _ros, r.first );
				 _ros << ", ";
				_CharOut( _ros, r.second );

				_ros << ", (" << m_sStateProtoTypedef << "*)( & ";
				if ( m_pvtDfaCur->m_rDfaCtxt.m_pgnStart == lpi.PGNChild() )
				{
					_ros << m_pvtDfaCur->m_sStartStateName;
					if ( !m_pvtDfaCur->FDontTemplatizeStates() )
						_ros << "<t_TyTraits>";
				}
				else
				{
					_ros << m_sBaseStateName << "_"  << ( lpi.PGNChild()->RElConst() + m_stStart );
					if ( !m_pvtDfaCur->FDontTemplatizeStates() )
						_ros << "<t_TyTraits>";
				}
				_ros << " ) }";

				bool fIsTrigger = _FIsTrigger( *lpi );
				lpi.NextChild();
				if ( !lpi.FIsLast() )
				{
					_ros << ",";
				}
				if ( fIsTrigger )
				{
					typename _TyDfa::_TyMapTriggerTransitionToTokenId::const_iterator cit = m_pvtDfaCur->m_rDfa.m_mapTriggerTransitionToTokenId.find( r.first );
					VerifyThrowSz( cit != m_pvtDfaCur->m_rDfa.m_mapTriggerTransitionToTokenId.end(), "Couldn't find trigger transition [0x%zx] in m_mapTriggerTransitionToTokenId.", size_t(r.first) );
					_ros << " /* TokenId[" << cit->second << "] */";
				}
				_ros << "\n";
			}
			_ros << "\t}";

			if ( _fAccept )
			{
				_GenActionMFnP( _ros, _pgn->RElConst() );
			}
			else
			{
				_ros << "\t};\n";
			}
		}
		else
		{
			if ( _fAccept )
			{
				_GenActionMFnP( _ros, _pgn->RElConst() );
			}
			else
			{
				_ros << " };\n";
			}
		}
	}

	void	_PrintActionMFnP( ostream & _ros, _TyActionObjectBase const & _raob )
	{
		typename _TyMapActions::iterator itUnique = m_mapActions.find( _raob );
		Assert( itUnique != m_mapActions.end() );
		typename _TyMapActions::value_type & rvtUnique = *itUnique;
		rvtUnique.second.second = true;	// referenced this action.
		
		_ros	<< "\n\tstatic_cast< typename TGetAnalyzerBase<t_TyTraits>::_TyPMFnAccept >( &TGetLexicalAnalyzer<t_TyTraits>::Action" 
					<< rvtUnique.second.first.m_strActionName.c_str() << " )";
	}

	// Generate the accept function pointer:	
	void _GenActionMFnP( ostream & _ros, _TyState _stAccept )
	{
		const typename _TyPartAcceptStates::value_type *	pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _stAccept );

		// Generate the accepting action, if any:
		if ( ( pvtAction->second.m_eaatType & ~e_aatTrigger ) &&
				 ( e_aatAntiAccepting != ( pvtAction->second.m_eaatType & ~e_aatTrigger ) ) )
		{
			_ros << ", ";
			if ( pvtAction->second.m_pSdpAction )
			{
				if ( !(*pvtAction->second.m_pSdpAction)->FIsTokenIdOnly() )
					_PrintActionMFnP( _ros, **( pvtAction->second.m_pSdpAction ) );
				else
					_ros << "nullptr /* ActionToken" << (*pvtAction->second.m_pSdpAction)->VGetTokenId() << " */";
					
			}
			else
			{
				_ros << "nullptr /* Action" << ( pvtAction->second.GetOriginalActionId() + m_aiStart ) << " */";
			}
		}

		switch( pvtAction->second.m_eaatType & ~e_aatTrigger )
		{
			case e_aatLookahead:
			{
				// We store the action identifier for the lookahead state -
				//	after disambiguation the mapping from lookahead suffixes to lookahead
				//	accepting states may be many to one.
				_ros << ", " << pvtAction->second.GetOriginalActionId() + m_aiStart;
			}
			break;
			case e_aatLookaheadAcceptAndAccept:
			case e_aatLookaheadAcceptAndLookahead:
			case e_aatLookaheadAccept:
			{
				if ( pvtAction->second.m_psrRelated )
				{
					// We store the bit vector of valid associated lookahead states- this
					//	vector is accumulated during disambiguation.
					if (	e_aatLookaheadAcceptAndLookahead == ( pvtAction->second.m_eaatType & ~e_aatTrigger ) )
					{
						// Then we need to store the action identifier:
						_ros << ", " << ( pvtAction->second.GetOriginalActionId() + m_aiStart );
					}
					else
					{
						// Action identifier is unused.
						_ros << ", -1";
					}
					_ros << ",\n\t{0x0" << hex;

					vtyLookaheadVector * pelRelated = pvtAction->second.m_psrRelated->begin();
					vtyLookaheadVector * pelEnd = pelRelated + 
						pvtAction->second.m_psrRelated->size_bytes() / sizeof( vtyLookaheadVector );
					for ( ; pelEnd != pelRelated; 
								( ++pelRelated == pelEnd ) || ( _ros << ", 0x0" ) )
					{
						_ros << *pelRelated;
					}
					_ros << dec << "}";
				}
				else
				{
					// Store the action id for the one and only related lookahead state.
					Assert( e_aatLookaheadAccept == 
                  ( pvtAction->second.m_eaatType & ~e_aatTrigger ) );
					_ros << ", " << ( pvtAction->second.m_aiRelated + m_aiStart );
				}
			}
			break;
		}

		// Now print the triggers:
		if ( pvtAction->second.m_eaatType & e_aatTrigger )
		{
			_ros << ",\n\t{";
			// Then generate the triggers:
			if ( pvtAction->second.m_psrTriggers )
			{
				_TySetActionIds bvCopyTriggers( *pvtAction->second.m_psrTriggers ); // better make a copy since this algorithm is destructive.
				size_t	stEnd = bvCopyTriggers.size();
				for ( auto stTrigger = bvCopyTriggers.getclearfirstset(); 
							stEnd != stTrigger;
						)
				{
					typename _TyDfa::_TyMapTriggers::iterator itTrigger = 
						m_pvtDfaCur->m_rDfa.m_pMapTriggers->find( (vtyActionIdent)stTrigger );
					
					_PrintActionMFnP( _ros, **( itTrigger->second.m_pSdpAction ) );

					stTrigger = bvCopyTriggers.getclearfirstset( stTrigger );
					if ( stEnd != stTrigger )
					{
						_ros << ",";
					}
				}
			}
			else
			{
				// Just a single trigger:
				_PrintActionMFnP( _ros, **( pvtAction->second.m_pSdpAction ) );
			}
			_ros << "\n\t}";
		}
		_ros << " };\n";
		
	}

	void	_GenStateDecls( ostream & _rosHeader )
	{
		typename _TyNodeLookup::iterator	nit = m_pvtDfaCur->m_rDfa.m_nodeLookup.begin();
		typename _TyNodeLookup::iterator	nitEnd = m_pvtDfaCur->m_rDfa.m_nodeLookup.end();
		for ( ; nit != nitEnd; ++nit )
		{
			_TyGraphNode *	pgn = static_cast< _TyGraphNode * >( *nit );
			int	nOuts = pgn->UChildren();	// We could record this earlier - like during both creation and optimization.
			bool	fAccept = m_pvtDfaCur->m_rDfaCtxt.m_pssAccept->isbitset( (size_t)pgn->RElConst() );// truncation ok here - we can't have a bitvector with > 4GB bits.
			_GenHeaderState( _rosHeader, pgn, nOuts, fAccept );
		}
		_rosHeader << "\n";
	}
	void	_GenStateDefinitions( ostream & _rosHeader )
	{
		typename _TyNodeLookup::iterator	nit = m_pvtDfaCur->m_rDfa.m_nodeLookup.begin();
		typename _TyNodeLookup::iterator	nitEnd = m_pvtDfaCur->m_rDfa.m_nodeLookup.end();
		for ( ; nit != nitEnd; ++nit )
		{
			_TyGraphNode *	pgn = static_cast< _TyGraphNode * >( *nit );
			int	nOuts = pgn->UChildren();	// We could record this earlier - like during both creation and optimization.
			bool	fAccept = m_pvtDfaCur->m_rDfaCtxt.m_pssAccept->isbitset( (size_t)pgn->RElConst() ); // truncation ok here - we can't have a bitvector with > 4GB bits.
			_GenImpState( _rosHeader, pgn, nOuts, fAccept );
		}
		_rosHeader << "\n";
	}
};

__REGEXP_END_NAMESPACE


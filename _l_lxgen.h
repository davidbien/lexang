#ifndef __L_LXGEN_H
#define __L_LXGEN_H

//          Copyright David Lawrence Bien 1997 - 2020.
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

// _l_gen_action_info: Describes a given action token id.
template < class t_TyCharOut, class t_TyAllocator >
struct _l_gen_action_info
{
	typedef std::basic_string< t_TyCharOut, char_traits<t_TyCharOut>, _TyAllocator > _tyStdStrOut;
	_l_gen_action_info() = default;
	_l_gen_action_info( _l_gen_action_info const & ) = deafult;
	_tyStdStrOut m_strActionName;	// Human readable name for the action.
	_tyStdStrOut m_strComment; // Comment to be placed in the generated code.
};

template < class t_TyDfa, class t_TyCharOut >
struct _l_gen_dfa
{
	typedef t_TyDfa _TyDfa;
	typedef t_TyCharOut	_TyCharOut;
	typedef typename _TyDfa::_TyDfaCtxt _TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator _TyAllocator;
	typedef basic_string< t_TyCharOut, char_traits<t_TyCharOut>, _TyAllocator >	_TyString;

	_l_gen_dfa( _TyDfa & _rDfa,
							_TyDfaCtxt & _rDfaCtxt,
							const t_TyCharOut * _pcStartStateName,
							_TyAllocator const & _rA )
		: m_rDfa( _rDfa ),
			m_rDfaCtxt( _rDfaCtxt ),
			m_sStartStateName( _pcStartStateName, _rA )
	{
		m_rDfa._CreateRangeLookup();
		m_rDfaCtxt.CreateAcceptPartLookup();
	}

	_TyDfa &			m_rDfa;
	_TyDfaCtxt &	m_rDfaCtxt;
	
	_TyString			m_sStartStateName;	// Special name for start state.
};

template < class t_TyDfa, class t_TyCharOut >
struct _l_generator
{
	typedef t_TyDfa	_TyDfa;
	typedef typename _TyDfa::_TyState				_TyState;
	typedef typename _TyDfa::_TyDfaCtxt			_TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator		_TyAllocator;
	typedef typename _TyDfa::_TyNodeLookup		_TyNodeLookup;
	typedef typename _TyDfa::_TyRange				_TyRange;
	typedef typename _TyDfa::_TyGraph				_TyGraph;
	typedef typename _TyDfa::_TyGraphNode		_TyGraphNode;
	typedef typename _TyDfa::_TyChar					_TyCharGen;
	typedef typename _TyDfa::_TyRangeEl			_TyRangeEl;
	typedef typename _TyDfa::_TyActionObjectBase	_TyActionObjectBase;

	typedef typename _TyDfaCtxt::_TyPartAcceptStates	_TyPartAcceptStates;
	typedef typename _TyDfaCtxt::_TyAcceptPartLookup	_TyAcceptPartLookup;

	typedef basic_string< t_TyCharOut, char_traits<t_TyCharOut>, _TyAllocator >	_TyString;

	typedef _l_gen_dfa< _TyDfa, t_TyCharOut >		_TyGenDfa;

  typedef typename _Alloc_traits< typename list< _TyGenDfa >::value_type, _TyAllocator >::allocator_type _TyDfaListAllocator;
	typedef list< _TyGenDfa, _TyDfaListAllocator > _TyDfaList;
	_TyDfaList m_lDfaGen;
	typename _TyDfaList::value_type *	m_pvtDfaCur{nullptr};

	_TyString m_sfnHeader;			// The header file we are generating.
	_TyString m_sfnImp;					// The implementation file we are generating.
	_TyString m_sPpBase;				// Preprocessor base name ( for #defines ).
	bool m_fUseNamespaces;	// Whether to use namespaces when generating.
	_TyString m_sNamespace;			// The namespace into which we generate.
	_TyString m_sMDAnalyzer;		// The type for the most-derived lexical analyzer.
	_TyString m_sBaseStateName;	// The base name for state variables.
	_TyString m_sCharTypeName;		// The name of the character type for the analyzer we are generating,
	_TyString m_sVisibleCharPrefix;	// Prefix for a visible character const.
	_TyString m_sVisibleCharSuffix;	// Suffix for a visible character const.
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
  typedef typename _Alloc_traits< typename map< vtyTokenIdent, _TyGenActionInfo >::value_type, _TyAllocator >::allocator_type _TyMapActionsAllocator;
	typedef std::map< vtyTokenIdent, _TyGenActionInfo, _TyMapActionsAllocator > _TyMapActionInfo;
	_TyMapActionInfo m_mapActionInfo;

	_l_generator( const t_TyCharOut * _pcfnHeader,
								const t_TyCharOut * _pcfnImp,
								const t_TyCharOut *	_pcPpBase,
								bool _fUseNamespaces,
								const t_TyCharOut * _pcNamespace,
								const t_TyCharOut *	_pcMDAnalyzer,
								const t_TyCharOut * _pcBaseStateName,
								const t_TyCharOut * _pcBaseActionName,
								const t_TyCharOut * _pcCharTypeName,
								const t_TyCharOut * _pcVisibleCharPrefix,
								const t_TyCharOut * _pcVisibleCharSuffix,
                _TyAllocator const & _rA = _TyAllocator() )
		: m_lDfaGen( _rA ),
			m_sfnHeader( _pcfnHeader, _rA ),
			m_sfnImp( _pcfnImp, _rA ),
			m_sPpBase( _pcPpBase, _rA ),
			m_fUseNamespaces( _fUseNamespaces ),
			m_sNamespace( _pcNamespace, _rA ),
			m_sMDAnalyzer( _pcMDAnalyzer, _rA ),
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
	}

  _TyAllocator  get_allocator()
  {
    return m_mapActions.get_allocator();
  }

	void add_dfa(	_TyDfa & _rDfa,
								_TyDfaCtxt & _rDfaCtxt,
								const t_TyCharOut * _pcStartStateName )
	{
		m_lDfaGen.push_back( _TyGenDfa( _rDfa, _rDfaCtxt, 
																		_pcStartStateName, 
                                    m_mapActions.get_allocator() ) );
		m_fLookaheads = m_fLookaheads || _rDfa.m_fHasLookaheads;
		m_fTriggers = m_fTriggers || !!_rDfa.m_nTriggers;
	}

	void add_action_info( vtyTokenIdent _tid, _TyGenActionInfo const & _rgai )
	{
		pair< _TyMapActionInfo::iterator, bool > pib = m_mapActionInfo.insert( _TyMapActionInfo::value_type( _tid, _rgai ) );
		VerifyThrowSz( pib.second, "TokenId[%d] has already been populated in the map.", _tid );
		// There is the possibility that the caller has duplicated a m_strActionName but if so the resultant code won't compile so no worries there.
	}

	void	generate()
	{
    _unique_actions();

    ofstream ofsHeader( m_sfnHeader.c_str() );
    ofstream	ofsImp( m_sfnImp.c_str() );

		_HeaderHeader( ofsHeader );
		_ImpHeader( ofsImp );

		for ( typename _TyDfaList::iterator lit = m_lDfaGen.begin();
					lit != m_lDfaGen.end(); ++lit )
		{
			m_pvtDfaCur = &*lit;

			_GenStates( ofsHeader, ofsImp );

			m_aiStart += m_pvtDfaCur->m_rDfa.m_iMaxActions;
			m_stStart += m_pvtDfaCur->m_rDfa.NStates();
		}

		// We generate a set of unique actions that satisty all DFA's:
		_GenActions( ofsHeader, ofsImp );

		_HeaderFooter( ofsHeader );
		_ImpFooter( ofsImp );

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
				typename _TyPartAcceptStates::iterator	itAction = m_pvtDfaCur->m_rDfaCtxt.m_partAccept.begin();
				typename _TyPartAcceptStates::iterator	itActionEnd = m_pvtDfaCur->m_rDfaCtxt.m_partAccept.end();
				for ( ; itAction != itActionEnd; ++itAction )
				{
					typename _TyPartAcceptStates::value_type & rvt = *itAction;
					Assert( !!rvt.second.m_pSdpAction || ( e_aatAntiAccepting == rvt.second.m_eaatType ) || ( rvt.second.m_eaatType ) );
					if ( rvt.second.m_pSdpAction )
					{
						Assert( !( e_aatTrigger & rvt.second.m_eaatType ) ); // Should see only non-trigger actions here.
						_TyMapActionInfo::const_iterator citAxnInfo = m_mapActionInfo.find( (*rvt.second.m_pSdpAction)->GetTokenId() );
						_TyGenActionInfo gaiInfo;
						if ( m_mapActionInfo.end() == citAxnInfo )
						{
							// Just use the action id - leave the comment empty.
							PrintfStdStr( gaiInfo.m_strActionName, "Token%d", (*rvt.second.m_pSdpAction)->GetTokenId() );
						}
						else
						{
							gaiInfo = citInfo->second;
						}
						std::pair< _TyMapActions::iterator, bool > pib = m_mapActions.insert(  typename _TyMapActions::value_type( **rvt.second.m_pSdpAction,
							typename _TyMapActions::mapped_type( gaiInfo, false ) ) );
						VerifyThrowSz( pib.second, "(type,TokenId)[%s,%d] are not unique", (*rvt.second.m_pSdpAction)->SzTypeName(), (*rvt.second.m_pSdpAction)->GetTokenId() );
					}
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
					Assert( !!rvt.second.m_pSdpAction );
					_TyMapActionInfo::const_iterator citAxnInfo = m_mapActionInfo.find( (*rvt.second.m_pSdpAction)->GetTokenId() );
					_TyGenActionInfo gaiInfo;
					if ( m_mapActionInfo.end() == citAxnInfo )
					{
						// Just use the action id - leave the comment empty.
						PrintfStdStr( gaiInfo.m_strActionName, "Trigger%d", (*rvt.second.m_pSdpAction)->GetTokenId() );
					}
					else
					{
						gaiInfo = citInfo->second;
					}
					std::pair< _TyMapActions::iterator, bool > pib = m_mapActions.insert( typename _TyMapActions::value_type(	**rvt.second.m_pSdpAction, 
						typename _TyMapActions::mapped_type( gaiInfo, false ) ) );
					VerifyThrowSz( pib.second, "(type,TokenId)[%s,%d] are not unique", (*rvt.second.m_pSdpAction)->SzTypeName(), (*rvt.second.m_pSdpAction)->GetTokenId() );
				}
			}
		}
	}

	void	_HeaderHeader( ostream & _ros )
	{
		_ros << "#ifndef __" << m_sPpBase << "_H\n";
		_ros << "#define __" << m_sPpBase << "_H\n";
		_ros << "\n";
		_ros << "// " << m_sfnHeader << "\n";
		_ros << "// Generated DFA.\n";
		_ros << "\n";
		_ros << "#include \"_l_lxobj.h\"\n";
		_ros << "\n";
		if ( m_fUseNamespaces )
		{
			_ros << "#define __" << m_sPpBase << "_BEGIN_NAMESPACE namespace " << m_sNamespace << " {\n";
			_ros << "#define __" << m_sPpBase << "_END_NAMESPACE }\n";
			_ros << "#define __" << m_sPpBase << "_USING_NAMESPACE using namespace " << m_sNamespace << ";\n";
			_ros << "#define __" << m_sPpBase << "_NAMESPACE " << m_sNamespace << "\n";
			_ros << "\n";
			_ros << "__" << m_sPpBase << "_BEGIN_NAMESPACE\n";
			_ros << "\n";
			_ros << "__LEXOBJ_USING_NAMESPACE\n";
			_ros << "\n";
		}

		_ros << "typedef _l_state_proto< " << m_sCharTypeName << " > _TyStateProto;\n";
		_ros << "typedef _l_transition< " << m_sCharTypeName << " > _TyTransition;\n";
		_ros << "typedef _l_analyzer< " << m_sCharTypeName
					<< ( m_fLookaheads ? ", true" : ", false" )
					<< ( m_fTriggers ? ", true" : ", false" )
					<< " > _TyAnalyzerBase;\n";
		_ros << "\n";
	}

	void	_HeaderFooter( ostream & _ros )
	{
		m_pvtDfaCur = &m_lDfaGen.front();

 		_ros << "\n";
		_ros << "struct _lexical_analyzer : public _TyAnalyzerBase\n";
		_ros << "{\n";
		_ros << "private:\n";
		_ros << "\ttypedef _TyAnalyzerBase		_TyBase;\n";
		_ros << "\ttypedef _lexical_analyzer	_TyThis;\n";
		_ros << "public:\n";
		_ros << "\t_lexical_analyzer()\n";
		_ros << "\t\t: _TyBase( (_TyStateProto*) & " << m_pvtDfaCur->m_sStartStateName << " )\n";
		_ros << "\t{ }\n";

		// Declare the base GetActionObject() template that is not implemented.
		_ros << "\ntemplate < const int t_iAction >\n\t_l_action_object_base< " << m_sCharTypeName << ", false > & GetActionObj();";

		// We generate all referenced unique actions:
		typename _TyMapActions::iterator itMAEnd = m_mapActions.end();
		typename _TyMapActions::iterator itMA = m_mapActions.begin();
		for ( ; itMA != itMAEnd; ++itMA )
		{
			typename _TyMapActions::value_type & rvt = *itMA;
			if ( rvt.second.second )
			{
				// Define a constant so that we don't just have raw numbers lying around as much:
				_ros << "\tstatic constexpr vtyTokenIdent s_kti" << rvt.second.first.m_strActionName.c_str() << ";\n"
				// Get a typedef for the action object for ease of use:
				_ros << "\tusing _tyAction" << rvt.second.first.m_strActionName.c_str() << " = ";
				rvt.first->RenderActionType( _ros, m_sCharTypeName.c_str() );
				_ros << ";\n";
				// Define the object as a member of the lexical analyzer class:
				_ros << "\t_tyAction" << rvt.second.first.m_strActionName.c_str() << " m_axn" << rvt.second.first.m_strActionName.c_str() << ";\n";
				_ros << "\ntemplate < >\n\t_l_action_object_base< " << m_sCharTypeName << ", false > & GetActionObj< " << rvt.first->GetTokenId() << " >()"
							" { return m_axn" << rvt.second.first.m_strActionName.c_str() << "; }\n";
				_ros << "\tbool Action" << rvt.second.first.m_strActionName.c_str() << "();\n";
			}
		}

		_ros << "};\n\n";
		_ros << "typedef _lexical_analyzer	_TyAnalyzer;\n";
		_ros << "typedef " << m_sMDAnalyzer <<  " _TyMDAnalyzer;\n";
		_ros << "\n";
		if ( m_fUseNamespaces )
		{
			_ros << "__" << m_sPpBase << "_END_NAMESPACE\n";
			_ros << "\n";
		}
		_ros << "#endif //__" << m_sPpBase << "_H\n";
	}

	void	_ImpHeader( ostream & _ros )
	{
		_ros << "\n";
		_ros << "// " << m_sfnImp << "\n";
		_ros << "// Generated DFA.\n";
		_ros << "\n";
		_ros << "#include \"" << m_sfnHeader << "\"\n";
		_ros << "#include \"syslogmgr.inl\"\n";
			_ros << "\n";
		if ( m_fUseNamespaces )
		{
			_ros << "__" << m_sPpBase << "_BEGIN_NAMESPACE\n";
			_ros << "\n";
			_ros << "__LEXOBJ_USING_NAMESPACE\n";
			_ros << "\n";
		}
	}

	void	_ImpFooter( ostream & _ros )
	{
		if ( m_fUseNamespaces )
		{
			_ros << "\n";
			_ros << "__" << m_sPpBase << "_END_NAMESPACE\n";
		}
	}

	void	_GenHeaderState(	ostream & _ros, _TyGraphNode * _pgn, 
													int _nOuts, bool _fAccept )
	{
		_ros	<< "extern	";
		_GenStateType( _ros, _pgn, _nOuts, _fAccept );
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

	void	_GenStateType(	ostream & _ros, _TyGraphNode * _pgn, 
												int _nOuts, bool _fAccept )
	{
		typename _TyPartAcceptStates::value_type *	pvtAction = 0;
		if ( _fAccept )
		{
			pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _pgn->RElConst() );
		}
		
#ifndef LXGEN_OUTPUT_TRIGGERS
		if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
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

			if ( pvtAction->second.m_eaatType & e_aatTrigger )
			{
				// Then we have some triggers:
				_ros << ", ";
				if ( pvtAction->second.m_psrTriggers )
				{
          Assert(pvtAction->second.m_psrTriggers->countsetbits());
					_ros << pvtAction->second.m_psrTriggers->countsetbits();
				}
				else
				{
					_ros << "1";
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
		_GenStateType( _ros, _pgn, _nOutsOrig, _fAccept );
		_ros << "\t\t_Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart ) << ";\n";

		_ros << "_Ty" << m_sBaseStateName << ( _pgn->RElConst() + m_stStart ) << "\t\t";

		typename _TyPartAcceptStates::value_type *	pvtAction = 0;

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
#ifndef LXGEN_OUTPUT_TRIGGERS
			if ( !!pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
			{
				--_nOuts;	// The zeroth transition is the trigger.
			}
#endif //!LXGEN_OUTPUT_TRIGGERS
		}
		_ros	<< _nOuts << ", ";
		if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
		{
			if ( pvtAction->second.m_psrTriggers )
			{
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
		if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
		{
			// Then the first transition is the trigger:
			_ros << "(_TyStateProto*)( & ";
			if ( (*(_pgn->PPGLChildHead()))->PGNChild() == m_pvtDfaCur->m_rDfaCtxt.m_pgnStart )
			{
				_ros	<< m_pvtDfaCur->m_sStartStateName;
			}
			else
			{
				_ros	<< m_sBaseStateName << "_" << 
					( (*(_pgn->PPGLChildHead()))->PGNChild()->RElConst() + m_stStart );
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
		}
		else
		{
			_ros << "0, 0";
		}

		if ( _nOuts )
		{
			_ros << ",\n";
			_ros << "\t{\n";
			typename _TyGraph::_TyLinkPosIterNonConst	lpi( _pgn->PPGLChildHead() );
#ifndef LXGEN_OUTPUT_TRIGGERS
			if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
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

				_ros << ", (_TyStateProto*)( & ";
				if ( m_pvtDfaCur->m_rDfaCtxt.m_pgnStart == lpi.PGNChild() )
				{
					_ros << m_pvtDfaCur->m_sStartStateName;
				}
				else
				{
					_ros << m_sBaseStateName << "_" 
							<< ( lpi.PGNChild()->RElConst() + m_stStart );
				}
				_ros << " ) }";

				lpi.NextChild();
				if ( !lpi.FIsLast() )
				{
					_ros << ",";
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
		
		_ros	<< "\n\tstatic_cast< _TyAnalyzerBase::_TyPMFnAccept >( &_TyAnalyzer::Action" 
					<< rvtUnique.second.first << " )";
	}

	// Generate the accept function pointer:
	void	_GenActionMFnP( ostream & _ros, _TyState _stAccept )
	{
		typename _TyPartAcceptStates::value_type *	pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _stAccept );

		// Generate the accepting action, if any:
		if ( ( pvtAction->second.m_eaatType & ~e_aatTrigger ) &&
				 ( e_aatAntiAccepting != ( pvtAction->second.m_eaatType & ~e_aatTrigger ) ) )
		{
			_ros << ", ";
			if ( pvtAction->second.m_pSdpAction )
			{
				_PrintActionMFnP( _ros, **( pvtAction->second.m_pSdpAction ) );
			}
			else
			{
				_ros << "0 /* Action" << ( pvtAction->second.GetOriginalActionId() + m_aiStart ) << " */";
			}
		}

		switch( pvtAction->second.m_eaatType & ~e_aatTrigger )
		{
			case e_aatLookahead:
			{
				// We store the action identifier for the lookahead state -
				//	after disambiguation the mapping from lookahead suffixes to lookahead
				//	accepting states may be many to one.
				_ros << ", " << pvtAction->second.GetOriginalActionId();
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
					if (	e_aatLookaheadAcceptAndLookahead == 
								( pvtAction->second.m_eaatType & ~e_aatTrigger ) )
					{
						// Then we need to store the action identifier:
						_ros << ", " << pvtAction->second.GetOriginalActionId();
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
					_ros << ", " << pvtAction->second.m_aiRelated;
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
				size_t	stEnd = pvtAction->second.m_psrTriggers->size();
				for ( auto stTrigger = pvtAction->second.m_psrTriggers->getclearfirstset(); 
							stEnd != stTrigger;
						)
				{
					typename _TyDfa::_TyMapTriggers::iterator itTrigger = 
						m_pvtDfaCur->m_rDfa.m_pMapTriggers->find( (vtyActionIdent)stTrigger );
					
					_PrintActionMFnP( _ros, **( itTrigger->second.m_pSdpAction ) );

					stTrigger = pvtAction->second.m_psrTriggers->getclearfirstset( stTrigger );
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

	void	_GenStates( ostream & _rosHeader, ostream & _rosImp )
	{
		typename _TyNodeLookup::iterator	nit = m_pvtDfaCur->m_rDfa.m_nodeLookup.begin();
		typename _TyNodeLookup::iterator	nitEnd = m_pvtDfaCur->m_rDfa.m_nodeLookup.end();
		for ( ; nit != nitEnd; ++nit )
		{
			_TyGraphNode *	pgn = static_cast< _TyGraphNode * >( *nit );
			int	nOuts = pgn->UChildren();	// We could record this earlier - like during both creation and optimization.
			bool	fAccept = m_pvtDfaCur->m_rDfaCtxt.m_pssAccept->isbitset( pgn->RElConst() );
			_GenHeaderState( _rosHeader, pgn, nOuts, fAccept );
			_GenImpState( _rosImp, pgn, nOuts, fAccept );
		}
		_rosImp << "\n";
		_rosHeader << "\n";
	}

	void	_GenActions( ostream & _rosHeader, ostream & _rosImp )
	{
		_rosImp << "\n";

		// Generate the action objects and the calls from the action methods:
		// We generate all referenced unique actions:
		typename _TyMapActions::iterator itMAEnd = m_mapActions.end();
		typename _TyMapActions::iterator itMA = m_mapActions.begin();
		for ( ; itMA != itMAEnd; ++itMA )
		{
			typename _TyMapActions::value_type & rvt = *itMA;
			if ( rvt.second.second )
			{
					_rosImp << "bool\t_TyAnalyzer::Action" << rvt.second.first.m_strActionName.c_str() << "()\n";
					_rosImp << "{\n";
					_rosImp << "\treturn " << "m_axn" << rvt.second.first.m_strActionName.c_str()
									<< ".action< _TyMDAnalyzer >( static_cast< _TyMDAnalyzer & >( *this ) );\n";
					_rosImp << "}\n\n";
			}
		}
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_LXGEN_H

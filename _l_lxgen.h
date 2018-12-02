#ifndef __L_LXGEN_H
#define __L_LXGEN_H

// _l_lxgen.h

// Lexical analyzer generator.

#include <fstream>

__REGEXP_BEGIN_NAMESPACE

template < class t_TyDfa, class t_TyCharOut >
struct _l_gen_dfa
{
	typedef t_TyDfa								_TyDfa;
	typedef typename _TyDfa::_TyDfaCtxt		_TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator	_TyAllocator;
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
	typedef list< _TyGenDfa, _TyAllocator >			_TyDfaList;
	_TyDfaList								m_lDfaGen;
	typename _TyDfaList::value_type *	m_pvtDfaCur;

	_TyString			m_sfnHeader;			// The header file we are generating.
	_TyString			m_sfnImp;					// The implementation file we are generating.
	_TyString			m_sPpBase;				// Preprocessor base name ( for #defines ).
	bool					m_fUseNamespaces;	// Whether to use namespaces when generating.
	_TyString			m_sNamespace;			// The namespace into which we generate.
	_TyString			m_sMDAnalyzer;		// The type for the most-derived lexical analyzer.
	_TyString			m_sBaseStateName;	// The base name for state variables.
	_TyString			m_sBaseActionName;	// The base name for action variables.
	_TyString			m_sCharTypeName;		// The name of the character type for the analyzer we are generating,
	_TyString			m_sVisibleCharPrefix;	// Prefix for a visible character const.
	_TyString			m_sVisibleCharSuffix;	// Suffix for a visible character const.
	_TyActionIdent			m_aiStart;
	typename _TyDfa::_TyState		m_stStart;

	bool					m_fLookaheads;
	bool					m_fTriggers;

	// We insert the actions into a map. They are ordered first by the typeinfo collating order
	//	and second by data in the action. We map to the unique action number for the given action,
	//	and whether the action has been referenced by any DFA.
	typedef pair< _TyActionIdent, bool >				_TyMAValue;
	typedef _ref< const _TyActionObjectBase >		_TyRefActionObjectBase;
	typedef map< _TyRefActionObjectBase, _TyMAValue, 
								less< _TyRefActionObjectBase >,_TyAllocator >	_TyMapActions;
	_TyMapActions	m_mapActions;	

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
			m_sBaseActionName( _pcBaseActionName, _rA ),
			m_sCharTypeName( _pcCharTypeName, _rA ),
			m_sVisibleCharPrefix( _pcVisibleCharPrefix, _rA ),
			m_sVisibleCharSuffix( _pcVisibleCharSuffix, _rA ),
			m_aiStart( 0 ),
			m_stStart( 0 ),
			m_fLookaheads( false ),
			m_fTriggers( false ),
      m_mapActions( typename _TyMapActions::key_compare(), _rA )  
	{
	}

  _TyAllocator  get_allocator()
  {
    return m_mapActions.get_allocator();
  }

	void	add_dfa(	_TyDfa & _rDfa,
									_TyDfaCtxt & _rDfaCtxt,
									const t_TyCharOut * _pcStartStateName )
	{
		m_lDfaGen.push_back( _TyGenDfa( _rDfa, _rDfaCtxt, 
																		_pcStartStateName, 
                                    m_mapActions.get_allocator() ) );
		m_fLookaheads = m_fLookaheads || _rDfa.m_fHasLookaheads;
		m_fTriggers = m_fTriggers || _rDfa.m_fHasTriggers;
	}

	void	generate()
	{
		_unique_actions();

	  ofstream	ofsHeader( m_sfnHeader.begin() );
		ofstream	ofsImp( m_sfnImp.begin() );

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

	void	_unique_actions()
	{
		m_aiStart = 0;
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
					if ( rvt.second.m_pSdpAction )
					{
						m_mapActions.insert( 
							typename _TyMapActions::value_type( **rvt.second.m_pSdpAction,
									typename _TyMapActions::mapped_type( m_aiStart + rvt.second.GetOriginalActionId(), false ) ) );
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
					m_mapActions.insert( 
						typename _TyMapActions::value_type(	**rvt.second.m_pSdpAction,
								typename _TyMapActions::mapped_type( m_aiStart + rvt.second.GetOriginalActionId(), false ) ) );
				}
			}

			m_aiStart += m_pvtDfaCur->m_rDfa.m_iMaxActions;
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
		_ros << "#include \"lexang/_l_lxobj.h\"\n";
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

		// We generate all referenced unique actions:
		typename _TyMapActions::iterator itMAEnd = m_mapActions.end();
		typename _TyMapActions::iterator itMA = m_mapActions.begin();
		for ( ; itMA != itMAEnd; ++itMA )
		{
			typename _TyMapActions::value_type & rvt = *itMA;
			if ( rvt.second.second )
			{
				_ros << "\tbool	Action" << rvt.second.first << "();\n";
			}
		}

		_ros << "};\n\n";
		_ros << "typedef _lexical_analyzer	_TyAnalyzer;\n";
		_ros << "typedef " << m_sMDAnalyzer <<  "\t_TyMDAnalyzer;\n";
		_ros << "\n";
		if ( m_fUseNamespaces )
		{
			_ros << "__" << m_sPpBase << "_END_NAMESPACE\n";
			_ros << "\n";
		}
		_ros << "#endif __" << m_sPpBase << "_H\n";
	}

	void	_ImpHeader( ostream & _ros )
	{
		_ros << "\n";
		_ros << "// " << m_sfnImp << "\n";
		_ros << "// Generated DFA.\n";
		_ros << "\n";
		_ros << "#include \"" << m_sfnHeader << "\"\n";
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
		
		if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
		{
			--_nOuts;	// The zeroth transition is the trigger.
		}

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
									( pvtAction->second.m_psrRelated->size_bytes() / sizeof( _TyLookaheadVector ) ) :
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
		_ros << " = { ";
		int _nOuts = _nOutsOrig;
		if ( _fAccept )
		{
			pvtAction = m_pvtDfaCur->m_rDfaCtxt.PVTGetAcceptPart( _pgn->RElConst() );
			if ( pvtAction->second.m_eaatType & e_aatTrigger )
			{
				--_nOuts;	// The zeroth transition is the trigger.
			}

			switch( pvtAction->second.m_eaatType & ~e_aatTrigger )
			{
				case e_aatAccept:
				{
					_ros << "kucAccept";
				}
				break;
				case e_aatLookahead:
				{
					_ros << "kucLookahead";
				}
				break;
				case e_aatLookaheadAccept:
				{
					_ros << "kucLookaheadAccept";
				}
				break;
				case e_aatLookaheadAcceptAndAccept:
				{
					_ros << "kucLookaheadAcceptAndAccept";
				}
				break;
				case e_aatLookaheadAcceptAndLookahead:
				{
					_ros << "kucLookaheadAcceptAndLookahead";
				}
				break;
				default:
				{
					_ros << "0";
				}
				break;
			}
		}
		else
		{
			_ros << "0";
		}
		_ros	<< ", " << _nOuts << ", ";
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
			_ros << "0, 0, ";
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
				_ros << ", m_pmfnAccept ),";
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
			if ( pvtAction && ( pvtAction->second.m_eaatType & e_aatTrigger ) )
			{
				lpi.NextChild();	// Skip the trigger transition.
			}

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
		typename _TyMapActions::iterator itUnique = 
			m_mapActions.find( _raob );
		assert( itUnique != m_mapActions.end() );
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
				 ( e_aatAntiAccepting != 
					 ( pvtAction->second.m_eaatType & ~e_aatTrigger ) ) )
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

					_TyLookaheadVector * pelRelated = pvtAction->second.m_psrRelated->begin();
					_TyLookaheadVector * pelEnd = pelRelated + 
						pvtAction->second.m_psrRelated->size_bytes() / sizeof( _TyLookaheadVector );
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
					assert( e_aatLookaheadAccept == 
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
				for ( size_t stTrigger = pvtAction->second.m_psrTriggers->getclearfirstset(); 
							stEnd != stTrigger;
						)
				{
					typename _TyDfa::_TyMapTriggers::iterator itTrigger = 
						m_pvtDfaCur->m_rDfa.m_pMapTriggers->find( (_TyActionIdent)stTrigger );
					
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
					rvt.first->Render( _rosImp, m_sCharTypeName.begin() );
					_rosImp << "\t" << m_sBaseActionName << rvt.second.first << ";\n";
					_rosImp << "bool\t_TyAnalyzer::Action" << rvt.second.first << "()\n";
					_rosImp << "{\n";
					_rosImp << "\treturn " << m_sBaseActionName << rvt.second.first
									<< ".action< _TyMDAnalyzer >( static_cast< _TyMDAnalyzer & >( *this ) );\n";
					_rosImp << "}\n\n";
			}
		}
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_LXGEN_H

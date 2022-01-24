#ifndef __L_DFACR_H
#define __L_DFACR_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_dfacr.h

// This module declares an object that can create a DFA from an NFA.

__REGEXP_BEGIN_NAMESPACE

template < class t_TyNfa, class t_TyDfa >
struct _create_dfa
{
private:
	typedef _create_dfa< t_TyNfa, t_TyDfa >	_TyThis;
protected:

	typedef	t_TyNfa	_TyNfa;
	typedef	typename t_TyNfa::_TyContext	_TyNfaCtxt;
	typedef	t_TyDfa	_TyDfa;
	typedef	typename _TyDfa::_TyContext			_TyDfaCtxt;
	typedef typename t_TyNfa::_TyAllocator	_TyAllocatorNfa;

	_TyNfa & m_rNfa;
	_TyNfaCtxt & m_rNfaCtxt;
	_TyDfa & m_rDfa;
	_TyDfaCtxt & m_rDfaCtxt;
	int	m_iDfaNodeLimit;

	typedef typename _TyNfa::_TyState			_TyState;

	typedef typename _TyNfa::_TySetStates	_TySetStatesNfa;
	typedef typename _TyNfa::_TySSCache		_TySSCacheNfa;
	typedef typename _TyNfa::_TyAlphabet	_TyAlphabetNfa;
	typedef _swap_object< _TySetStatesNfa >	_TySwapSSNfa;

	typedef typename _TyDfa::_TyAlphabet	_TyAlphabetDfa;
	typedef typename _TyDfa::_TyGraphNode	_TyGraphNodeDfa;
	typedef typename _TyDfa::_TyGraphLink	_TyGraphLinkDfa;
	typedef typename _TyDfa::_TyAcceptAction	_TyAcceptAction;

  typedef typename _Alloc_traits< typename unordered_map< _TySwapSSNfa, _TyGraphNodeDfa* >::value_type, _TyAllocatorNfa >::allocator_type _tySwapSSNfaAlloc;
  typedef unordered_map<	_TySwapSSNfa, _TyGraphNodeDfa*,
    hash< _TySwapSSNfa >, equal_to< _TySwapSSNfa >,
    _tySwapSSNfaAlloc > _TyLookupSS;
  typedef typename _Alloc_traits< typename forward_list< pair< _TyState, _TyState > >::value_type, _TyAllocatorNfa >::allocator_type _TyDfaAcceptingListAlloc;
  typedef forward_list< pair< _TyState, _TyState >, _TyDfaAcceptingListAlloc > _TyDfaAcceptingList;
  typedef typename _Alloc_traits< typename deque< const _TySetStatesNfa* >::value_type, _TyAllocatorNfa >::allocator_type _TyMapStateToSSAlloc;
  typedef deque< const _TySetStatesNfa*, _TyMapStateToSSAlloc > _TyMapStateToSS;

	// Lookahead disambiguating stuff:
  typedef typename _Alloc_traits< typename set< _TyState, less< _TyState > >::value_type, _TyAllocatorNfa >::allocator_type _TySetLDStatesAlloc;
  typedef set< _TyState, less< _TyState >, _TySetLDStatesAlloc > _TySetLDStates;
	typedef pair< _TySetLDStates, _TyAcceptAction >							_TyPairStatesAction;
	typedef _swap_object< typename _TyAcceptAction::_TySetActionIds >		_TySwapSetActionIds;
	typedef pair< _TySwapSetActionIds, _TySwapSetActionIds >		_TyPairSSActionIds;
	typedef less< _TyPairSSActionIds >													_TyCompareAmbigKey;
  typedef typename _Alloc_traits< typename map< _TyPairSSActionIds, _TyPairStatesAction,
                                                _TyCompareAmbigKey >::value_type, _TyAllocatorNfa >::allocator_type _TyAmbigAcceptAlloc;
  typedef map< _TyPairSSActionIds, _TyPairStatesAction, _TyCompareAmbigKey, _TyAmbigAcceptAlloc > _TyAmbigAccept;

	_TyLookupSS			m_ssLookup;
	_TyMapStateToSS	m_mapStateToSS;

	_TyState	m_sCur;	// The state of the DFA that we are currently processing.

	bool			m_fCreateDeadState;	// Should we create a dead state.

	// managed solely by create():
	_TySetStatesNfa * m_pssAcceptingNfa;
	_TySetStatesNfa * m_pssCur;

	// We store the accepting states for the DFA in a list for now:
	// The first in the pair is the original NFA state - the second
	//	is the corresponding DFA state:
	_TyDfaAcceptingList	m_lDfaAccepting;

	// The map of lookahead disambiguating action states.
	_TyAmbigAccept			m_mapAmbigAccept;

	bool								m_fAmbiguousTriggers;	// Any ambiguous triggers ?
	bool								m_fAmbiguousAntiAccepting;	// Any ambiguous anti-accepting states.

	static const bool	m_fAllowReject = false;

public:
	
	_create_dfa(	_TyNfa & _rNfa, _TyNfaCtxt & _rNfaCtxt,
								_TyDfa & _rDfa, _TyDfaCtxt & _rDfaCtxt,
								bool _fCreateDeadState,
								size_t _stHashSize = 3000 )
		:	m_rNfa( _rNfa ),
			m_rNfaCtxt( _rNfaCtxt ),
			m_rDfa( _rDfa ),
			m_rDfaCtxt( _rDfaCtxt ),
			m_sCur( 0 ),
			m_ssLookup( _stHashSize, typename _TyLookupSS::hasher(),
									typename _TyLookupSS::key_equal(), 
									_rNfa.get_allocator() ),
			m_mapStateToSS( _rNfa.get_allocator() ),
			m_fCreateDeadState( _fCreateDeadState ),
			m_lDfaAccepting( m_rNfa.get_allocator() ),
			m_mapAmbigAccept( _TyCompareAmbigKey(), m_rNfa.get_allocator() ),
			m_fAmbiguousTriggers( false ),
			m_fAmbiguousAntiAccepting( false )
	{
		Assert( !_rDfa.NStates() );
		Assert( !_rDfaCtxt.m_pgnStart );
	}

	// Returns false if we go over the node limit.
	bool	create( )
	{
    // We can't store iterators to a deque to which a push_back() or push_front() is called - so all bets are off with storing iterators.
    // We can however, in the presence of at least push_back() or push_front(),  store pointers/refs to elements. In fact as such we can just store an index to the element in question.
    // Get the accepting states from the NFA:

#if 0
    CMFDtor1_void< _TyNfa, size_t >
      releaseAcceptingNfa(&m_rNfa, &_TyNfa::_ReleaseSSCache, m_rNfa._STGetSSCache(m_pssAcceptingNfa));
#endif //0
    size_t stReleaseAccepting = m_rNfa._STGetSSCache(m_pssAcceptingNfa);
    auto fcReleaseAccept = [=]() // define the lambda for the releasing pssAccepting back to the cache.
    { 
      m_rNfa._ReleaseSSCache(stReleaseAccepting); 
    };
    typedef decltype(fcReleaseAccept) tyDeclFcReleaseAccept;
    _fcallobj< tyDeclFcReleaseAccept > fcoReleaseAccept(fcReleaseAccept);
    m_pssAcceptingNfa->clear();
    m_rNfaCtxt.GetAcceptingNodeSet(*m_pssAcceptingNfa);

		// Copy the alphabet set from the NFA to the DFA:
		{//B
			typename _TyAlphabetNfa::iterator itNfaAlpha = m_rNfa.m_setAlphabet.begin();
			if ( itNfaAlpha->empty() )
			{
				++itNfaAlpha;
			}
			m_rDfa.m_setAlphabet.insert( itNfaAlpha, m_rNfa.m_setAlphabet.end() );
		}//EB

		// Set up max original actions - we will be adding disambiguating actions:
		m_rDfa.m_iMaxActions = m_rNfa.m_iActionCur;

    CMFDtor1_void< _TyNfa, size_t >
      releaseCur(&m_rNfa, &_TyNfa::_ReleaseSSCache, m_rNfa._STGetSSCache(m_pssCur));
		m_pssCur->clear();

		// If we have a dead state then create it as the zeroth state -
		//	this keeps it in a known spot - allowing the optimizer to not
		//	process - afterwards we see if it has any connections and if so
		//	we add out transitions on every alphabet symbol.
    // REVIEW: <dbien>: pgnDead can be leaked on throw - until gets hooked up to graph.
    CMFDtor1_void< _TyDfa, _TyGraphNodeDfa * > mfdtorDestroyDeadNode;
		_TyGraphNodeDfa *	pgnDead = 0;
		if ( m_fCreateDeadState )
		{
			// The dead state is the zeroth state for the DFA - we have it correspond to the set
			//	of zero states in the NFA - this way no other DFA state can be in the same partition.
			m_rDfa._NewStartState( &pgnDead );
      mfdtorDestroyDeadNode.Reset( &m_rDfa, &_TyDfa::DestroySubGraph, pgnDead );
			{//B
				_TySetStatesNfa ssTemp( *m_pssCur ); // Gcc doesnt like the inline temporary.
				typename _TyLookupSS::value_type vtInsert( ssTemp, pgnDead ); // Insert a copy.
				pair< typename _TyLookupSS::iterator, bool > pibInserted = m_ssLookup.insert( vtInsert );	
				Assert( pibInserted.second );
				m_mapStateToSS.push_back( &(*pibInserted.first).first.RObject() );
			}//EB
			m_sCur++;  // We don't process the dead state.
		}

		m_rNfa.AllocClosureCache();
		
		// Create the initial state:
		Assert( m_pssCur->empty() );
		m_rNfa.Closure( m_rNfaCtxt.m_pgnStart, *m_pssCur, true );
		m_rDfa._NewStartState( &m_rDfaCtxt.m_pgnStart );
		_TySetStatesNfa const * pssInLookup = _NewDfaState( m_rDfaCtxt.m_pgnStart );

		_TySetStatesNfa * pssMove;
		CMFDtor1_void< _TyNfa, size_t >
			releaseMove( &m_rNfa, &_TyNfa::_ReleaseSSCache, m_rNfa._STGetSSCache(pssMove) );
		pssMove->clear();

		_TyGraphNodeDfa *	pgnCurDfa = m_rDfaCtxt.m_pgnStart;

		typedef typename _TyDfa::_TyAlphaIndex _TyAlphaIndex;
#ifdef __GNUC__ // <dbien>: can't get this to compile on VC14.
		const typename _TyDfa::_TyAlphaIndex iaMax = (numeric_limits< _TyAlphaIndex >::max)();
		if ( m_rDfa.m_setAlphabet.size() > iaMax )
			throw alpha_index_overflow("_create(): Alphabet size overflowed maximum alphabet index (__TyDfa::_TyAlphaIndex).");
#endif //__GNUC__
		typename _TyDfa::_TyAlphaIndex	aiStart = (typename _TyDfa::_TyAlphaIndex)(m_rDfa.m_setAlphabet.size()-1);
		typename _TyAlphabetDfa::iterator itAlphaBegin = m_rDfa.m_setAlphabet.begin();
		
		while ( m_sCur != m_rDfa.NStates() )
		{
			// For each input range - excluding the empty state:
			// Go backward through the set - this pushes the alphabet on in order.
			typename _TyDfa::_TyAlphaIndex	aiCur = aiStart;

			typename _TyAlphabetDfa::iterator itAlpha = m_rDfa.m_setAlphabet.end();
			--itAlpha;

			// If we have a trigger or unsatisfiable transition then we want that/them to be first:
			_TyGraphLinkDfa *	pglFirstAdded = 
				( m_rNfa.m_nTriggers || m_rNfa.m_nUnsatisfiableTransitions ) ? 
					0 : (_TyGraphLinkDfa*)1 ;
			//Assert( !m_rNfa.m_nTriggers || ( _TyNfa::ms_kreTrigger == itAlpha->first ) );

			bool	fContLoop = true;
			do
			{
				m_rNfa.ComputeSetMoveStates( *m_pssCur, *itAlpha, *pssMove );
				Assert( m_pssCur->empty() );
				m_rNfa.ComputeSetClosure( *pssMove, *m_pssCur );
				Assert( pssMove->empty() );

				if ( !m_pssCur->empty() )
				{
					// Then at a non-null state - first check if in the map ( this way
					//	we don't have to copy it first ):
					_TySwapSSNfa	sossLookup( *m_pssCur );	// takes possession of bitvec inside m_pssCur.
					typename _TyLookupSS::iterator itLookup = m_ssLookup.find( sossLookup );
					m_pssCur->swap( sossLookup );	// Swap back.
					if ( itLookup == m_ssLookup.end() )
					{
						_TyGraphNodeDfa *	pgnNew;
						m_rDfa._NewAcceptingState(	pgnCurDfa, aiCur, &pgnNew, 
																				pglFirstAdded ? 0 : &pglFirstAdded );
						(void)_NewDfaState( pgnNew );
					}
					else
					{
						m_rDfa._NewTransition(	pgnCurDfa, aiCur, (*itLookup).second, 
																		pglFirstAdded ? 0 : &pglFirstAdded );
					}
				}
				else
				{
					// If we have a "dead" state map to that here.
					if ( m_fCreateDeadState )
					{
	          if ( mfdtorDestroyDeadNode.FIsActive() )
	            mfdtorDestroyDeadNode.Reset(); // We are connecting to the dead node so it will be destroyed by graph destruction.
						m_rDfa._NewTransition(	pgnCurDfa, aiCur, pgnDead, 
																		pglFirstAdded ? 0 : &pglFirstAdded );
					}
				}

				if ( itAlpha != itAlphaBegin )
				{
					--aiCur;
					--itAlpha;
					// Then we need to restore m_pssCur:
					*m_pssCur = *pssInLookup;
				}
				else
				{
					fContLoop = false;
				}
			}
			while( fContLoop );

			if ( pglFirstAdded && ( m_rNfa.m_nTriggers || m_rNfa.m_nUnsatisfiableTransitions ) )
			{
				// Remove all trigger and unsatisfiable transition from the end of the child list
				//	and place at the beginning - this eases further processing.
				typename _TyDfa::_TyAlphaIndex	aiLimit = aiStart - ( m_rNfa.m_nTriggers + m_rNfa.m_nUnsatisfiableTransitions );
				while( pglFirstAdded->RElConst() > aiLimit )
				{
					_TyGraphLinkDfa *	pglMove = pglFirstAdded;
					if ( pglFirstAdded != *pgnCurDfa->PPGLChildHead() )
					{
						pglFirstAdded = pglFirstAdded->PGLGetPrevChild();
						pglMove->RemoveChild();
						pglMove->InsertChild( pgnCurDfa->PPGLChildHead() );
					}
					else
					{
						break;
					}
				}
			}

			if ( ++m_sCur != m_rDfa.NStates() )
			{
				// Then we need to update m_pssCur:
				pssInLookup = m_mapStateToSS[ (size_t)m_sCur ];
				Assert( !pssInLookup->empty() );
				*m_pssCur = *pssInLookup;
				pgnCurDfa = m_rDfa.PGNGetNode( m_sCur );
			}
		}

		m_rNfa.DeallocClosureCache();

		// If the dead state has any in transitions then we need to add out transitions:
		if ( m_fCreateDeadState && pgnDead->FParents() )
		{			
			typename _TyDfa::_TyAlphaIndex	aiCur = (typename _TyDfa::_TyAlphaIndex)m_rDfa.m_setAlphabet.size();
			aiCur -= m_rNfa.m_nTriggers + m_rNfa.m_nUnsatisfiableTransitions;

			while (	aiCur )
			{
				m_rDfa._NewTransition( pgnDead, --aiCur, pgnDead, 0 );
			}

			for ( size_t stUnsat = 0;
						stUnsat++ < m_rNfa.m_nUnsatisfiableTransitions;
					)
			{
				m_rDfa._NewTransition( pgnDead, (typename _TyDfa::_TyAlphaIndex)( m_rDfa.m_setAlphabet.size()-stUnsat ), pgnDead, 0 );
			}

			for ( size_t stTrigger = 0;
						stTrigger++ < m_rNfa.m_nTriggers;
					)
			{
				// First transition should be trigger transition:
				m_rDfa._NewTransition( pgnDead, (typename _TyDfa::_TyAlphaIndex)( m_rDfa.m_setAlphabet.size()-stTrigger-m_rNfa.m_nUnsatisfiableTransitions ), pgnDead, 0 );
			}
		}

		_ConstructAcceptPartition( );

		if ( m_fCreateDeadState )
		{
      // Even if we didn't add any transitions from the dead state, we will indicate that the graph has a dead state so that optimizaion will not barf.
      mfdtorDestroyDeadNode.Reset(); // From now on we let the DFA control the lifetime of the dead state.
			m_rDfa.m_fHasDeadState = true;
		}

		m_rDfa.m_fHasLookaheads = m_rNfa.m_fHasLookaheads;
		m_rDfa.m_nTriggers = m_rNfa.m_nTriggers;
		m_rDfa.m_nUnsatisfiableTransitions = m_rNfa.m_nUnsatisfiableTransitions;
		m_rDfa.ConsumeMapTokenIdToTriggerTransition( m_rNfa.m_mapTokenIdToTriggerTransition );

		return true;
	}

	_TySetStatesNfa const *
	_NewDfaState( _TyGraphNodeDfa * _pgn )
	{
    pair< typename _TyLookupSS::iterator, bool > pibInserted;
    { // BLOCK
		  _TySetStatesNfa ssTemp( *m_pssCur ); // gcc doesn't like the inline temporary.
		  typename _TyLookupSS::value_type vtInsert( ssTemp, _pgn ); // Insert a copy.
		  pibInserted = m_ssLookup.insert( vtInsert );
		  Assert( pibInserted.second );
    }

		_TySetStatesNfa const *	pssInLookup;
		m_mapStateToSS.push_back( pssInLookup = &(*pibInserted.first).first.RObject() );
		Assert( m_mapStateToSS.size() == m_rDfa.NStates() );
    typename _TySetStatesNfa::size_type stFirst = m_pssAcceptingNfa->FirstIntersection( *m_pssCur );
		if ( m_pssAcceptingNfa->size() != stFirst )
		{
			// If we have multiple intersections then the rule set is ambiguous:
      typename _TySetStatesNfa::size_type stNext = m_pssAcceptingNfa->NextIntersection( *m_pssCur, stFirst );
			if ( stNext != m_pssAcceptingNfa->size() )
			{
				// Then we have an ambiguous rule:
				// 1) If ambiguity is among normal accepting and lookahead states then:
				//	Choose the rule with the lowest action number - this should allow for
				//		catch-all rules after more specific rules.
				// 2) If, along the way, we find any triggers or lookahead accepting states then
				//		include those in the ambiguous state.
				// 3) We may have both (1) and (2) or either.
				typename _TyNfa::_TySetAcceptVT & rvtFirst = *m_rNfa.m_pSetAcceptStates->find( stFirst );

				typename _TyAcceptAction::_TySetActionIds	srAction( m_rDfa.m_iMaxActions, 
																										rvtFirst.second.get_allocator() );
				srAction.clear();
				srAction.setbit( rvtFirst.second.m_aiAction );
				srAction.setbit( m_rNfa.m_pSetAcceptStates->find( stNext )->second.m_aiAction );

				while (	( stNext = m_pssAcceptingNfa->NextIntersection( *m_pssCur, stNext ) ) !=
									m_pssAcceptingNfa->size() )
				{
					srAction.setbit( m_rNfa.m_pSetAcceptStates->find( stNext )->second.m_aiAction );
				}

				typename _TyAmbigAccept::iterator	itAmbig;

				m_rNfa.CreateActionIDLookup();	// If not already.

				bool	fFoundLookaheadAccept = false;
				// Bit vector of relations translated from action ids:
				typename _TyAcceptAction::_TySetActionIds	srFoundLARelations(	m_rDfa.m_iMaxActions, 
																															rvtFirst.second.get_allocator() );
				srFoundLARelations.clear();

				// Bit vector of found trigger action ids:
				bool	fFoundTrigger = false;
				typename _TyAcceptAction::_TySetActionIds	srFoundTriggers(	m_rDfa.m_iMaxActions, 
																														rvtFirst.second.get_allocator() );
				srFoundTriggers.clear();

				bool	fFoundAntiAccepting = false;

				// The first accepting or lookahead state found:
				typename _TyNfa::_TySetAcceptStates::value_type * pvtFirstAccept = 0;
				vtyActionIdent	aiActionMin = (vtyActionIdent)srAction.getclearfirstset();
				for ( vtyActionIdent aiAction = aiActionMin;
							srAction.size() != aiAction;
							aiAction = (vtyActionIdent)srAction.getclearfirstset( aiAction )
						)
				{
					Assert( srAction.size() != aiAction );
					typename _TyNfa::_TySetASByActionID::iterator it = 
						m_rNfa.m_pLookupActionID->find( aiAction );
					Assert( it != m_rNfa.m_pLookupActionID->end() );
					
					switch( it->second->second.m_eaatType )
					{
						case e_aatAccept:
						case e_aatLookahead:
						case e_aatAntiAccepting:
						{
							if ( !pvtFirstAccept )
							{
								pvtFirstAccept = it->second;
								if ( e_aatAntiAccepting == pvtFirstAccept->second.m_eaatType )
								{
									// We don't allow ambiguity with any other states when we find an anti-accept:
									m_fAmbiguousAntiAccepting = true;
									fFoundAntiAccepting = true;
									fFoundLookaheadAccept = false;
									fFoundTrigger = false;
								}
							}
						}
						break;
						case e_aatLookaheadAccept:
						{
							fFoundLookaheadAccept = true;
							srFoundLARelations.setbit( it->second->second.m_aiRelated );
						}
						break;
						case e_aatTrigger:
						{
							// Triggers always fire - unless creator sets a flag which then compares action ids:
							fFoundTrigger = true;
							srFoundTriggers.setbit( aiAction );
						}
						break;
						default:
						{
							Assert( 0 );
						}
						break;
					}

					if ( fFoundAntiAccepting )
					{
						break;
					}
				}

				if ( fFoundLookaheadAccept || fFoundTrigger )
				{
#if 0 // REVIEW: Might want to either enable or get rid of or just explain this code:
					// If we found a lookahead accept node that was ambiguous then don't use it:
					if ( e_aatLookahead == pvtFirstAccept->second.m_eaatType )
					{
						//pvtFirstAccept = 0;
					}
#endif //0

					// If we found ambiguous triggers then we need to check if any correspond to the same
					//	action - coalesce unique trigger actions at each state:
					if ( fFoundTrigger )
					{
						vtyActionIdent aiFirst = (vtyActionIdent)srFoundTriggers.getfirstset();
						bool	fMultiTrigger = false;
						if ( srFoundTriggers.getnextset( aiFirst ) != srFoundTriggers.size() )
						{
							typename _TyAcceptAction::_TySetActionIds	srTriggers( srFoundTriggers );
							srTriggers.clearbit( aiFirst );
							do
							{
								typename _TyNfa::_TySetASByActionID::value_type & rvtFirst = 
									*m_rNfa.m_pLookupActionID->find( aiFirst );
									
								for ( vtyActionIdent aiNext = aiFirst; 
											srTriggers.size() != ( aiNext = (vtyActionIdent)srTriggers.getnextset( aiNext ) ); )
								{
									if ( m_rNfa.m_pLookupActionID->
												find( aiNext )->second->second.m_pSdpAction->GetBaseP()->FIsSameAs(
													*( rvtFirst.second->second.m_pSdpAction->GetBaseP() ), true ) )
									{
										// Duplicate action:
										srTriggers.clearbit( aiNext );
										srFoundTriggers.clearbit( aiNext );
									}
									else
									{
										fMultiTrigger = true;
									}
								}
							}
							while( srTriggers.size() != 
											( aiFirst = (vtyActionIdent)srTriggers.getclearfirstset( aiFirst ) ) );
						}
						// Now if we coalesced a number of the same triggers into one trigger
						//	and we have no associated (lookahead)/accepting states then we can 
						//	just use the first state:
						if ( !fFoundLookaheadAccept && !fMultiTrigger && !pvtFirstAccept )
						{
							vtyActionIdent aiMin = (vtyActionIdent)srFoundTriggers.getfirstset();
							typename _TyNfa::_TySetASByActionID::value_type & rvtMin = 
								*m_rNfa.m_pLookupActionID->find( aiMin );
							m_lDfaAccepting.push_front(
								typename _TyDfaAcceptingList::value_type( rvtMin.second->first, _pgn->RElConst() ) );
							return pssInLookup;
						}
					}

					// This Dfa state gets a new action, which allows the completion of
					//	the any of the rules accumulated above.
					// It could be that other DFA states have this same completion criteria - if so
					//	we will group this state and those states in the same partition - this allows potential
					//	optimization of these states.
					// The completion criteria are embodied in ( srFoundLARelations, srFoundTriggers ).
					typename _TyAmbigAccept::key_type	ktFind( srFoundLARelations, srFoundTriggers );
					itAmbig = m_mapAmbigAccept.find( ktFind );
					srFoundLARelations.swap( ktFind.first );
					srFoundTriggers.swap( ktFind.second );

					if ( m_mapAmbigAccept.end() == itAmbig )
					{
						_TyAcceptAction aaNew( 
								pvtFirstAccept ? pvtFirstAccept->second.m_aiAction : m_rNfa.m_iActionCur++, 
								pvtFirstAccept ? pvtFirstAccept->second.m_pSdpAction.Ptr() : 0 );
						if ( fFoundLookaheadAccept )
						{
							if ( pvtFirstAccept )
							{
								Assert( e_aatAntiAccepting != aaNew.m_eaatType );
								aaNew.m_eaatType = e_aatLookahead == pvtFirstAccept->second.m_eaatType ? 
									e_aatLookaheadAcceptAndLookahead : e_aatLookaheadAcceptAndAccept;
							}
							else
							{
								aaNew.m_eaatType = e_aatLookaheadAccept;
							}
						}
						else
						{
							if ( pvtFirstAccept )
							{
								aaNew.m_eaatType = pvtFirstAccept->second.m_eaatType;
							}
							else
							{
								aaNew.m_eaatType = e_aatNone;
							}
						}
						// Now set the trigger flag if we found any:
						if ( fFoundTrigger )
						{
              Assert(!!srFoundTriggers.countsetbits());
              m_fAmbiguousTriggers = true;	// We have ambiguous triggers.
							(int&)aaNew.m_eaatType |= e_aatTrigger;
						}
						// The original action to which this corresponds:
						aaNew.m_aiRelated = pvtFirstAccept ? pvtFirstAccept->second.m_aiAction : -1;
						_TySetLDStates setAssoc( less< _TyState >(), m_rNfa.get_allocator() );

						{ // BLOCK // gcc doesn't like the inline temporaries:
							typename _TyAmbigAccept::key_type ktTemp( srFoundLARelations, srFoundTriggers );
							typename _TyAmbigAccept::mapped_type mtTemp( setAssoc, aaNew );
							typename _TyAmbigAccept::value_type vtTemp( ktTemp, mtTemp );
							itAmbig = m_mapAmbigAccept.insert( vtTemp ).first;
						}
						// Now ( since we have the info at this point ) if we have LA relations or
						//	triggers then allocate the respective objects:
						if ( fFoundLookaheadAccept )
						{
							itAmbig->second.second.m_psrRelated.template emplace
								< typename _TyAcceptAction::_TySetActionIds::size_type,
									typename _TyAcceptAction::_TySetActionIds::_TyAllocator const & >
								( 0, srFoundLARelations.get_allocator() );
						}
						if ( fFoundTrigger )
						{
							itAmbig->second.second.m_psrTriggers.template emplace
								< typename _TyAcceptAction::_TySetActionIds::size_type,
									typename _TyAcceptAction::_TySetActionIds::_TyAllocator const & >
								( 0, srFoundLARelations.get_allocator() );
						}
					}

					// Relate the current state to the ambiguous state created or found:
					itAmbig->second.first.insert( _pgn->RElConst() );
											
					return pssInLookup;
				}
				else
				{
					Assert( pvtFirstAccept );
					Assert( pvtFirstAccept->first == m_rNfa.m_pLookupActionID->find( aiActionMin )->second->first );
					// No need for a special diambiguating accepting state - just competition
					//	among normal accepting states and lookahead states:
					stFirst = (size_t)pvtFirstAccept->first;
				}

				if ( m_fAllowReject )
				{
					// Then need to set up the reject vector of accepting actions "directly rejectable to" 
					//	from this state ( indicating that the same amount of input is eaten - just a different
					//	action is taken - one that has a higher action id ).  For now we include the current action
					//	to uniquely identify
					// Lower action id rules - matching 
					//	less input - may be chosen after consulting these - and they, in turn may have a list of rules with
					//	higher action id's directly rejectable from them, etc.
					// This may uniquely identify an accepting state - we first check this when
					//	we are attempting to insert a lookahead disambiguating state - if the
					//	state is in the reject vector then we associate action generated below with that state.
					// We will insert the states uniquely identified by reject vector first into the accepting
					//	state partition. As we do so form a bitvector of states in this group - this allows
					//	us to determine, as we add lookahead disambiguating states, whether a state is already
					//	unique identified by membership in this class - in which case the partition of disambiguating
					//	action needs to be partitioned - copying the lookahead accept vector ( any action will be the same )
					//	to states that are present in the reject vector. Any remaining states remain int the lookahead
					//	accept vector.

					// If <aiAction> still points to the minimum then no action was generated above.
					// If <aiAction> is srAction.size() then no "direct rejection" is possible from this state -
					//	only default lesser input rejection - need generate no info here.
					// If <aiAction> in between then <itAmbig> points to the action generated above. We can
					//	copy the 
				}
			}
			
			m_lDfaAccepting.push_front( 
				typename _TyDfaAcceptingList::value_type( stFirst, _pgn->RElConst() ) );
		}
		return pssInLookup;
	}

	void	_ConstructAcceptPartition( )
	{
		// We create a map of NFA states to pair( sets of DFA accepting states, action object )'s:
		typedef typename _TyDfa::_TySetStates		_TySetStatesDfa;
		typedef typename _TyDfaCtxt::_TySwapSS		_TySwapSSDfa;
		typedef typename _TyDfa::_TyAcceptAction	_TyAcceptAction;
		typedef pair< _TySwapSSDfa, _TyAcceptAction >	_TyLMapEl;
    typedef typename _Alloc_traits< typename map< _TyState, _TyLMapEl, less< _TyState > >::value_type, _TyAllocatorNfa >::allocator_type _TyLocalMapAllocator;
    typedef map< _TyState, _TyLMapEl, less< _TyState >, _TyLocalMapAllocator >	_TyLocalMap;

		_TyLocalMap mapNfaState( less< _TyState >(), m_rNfa.get_allocator() );

		typename _TyLocalMap::iterator itNfaState;
		
		m_rDfaCtxt.CreateAcceptingNodeSet();
		typename _TyDfaAcceptingList::iterator itLEnd = m_lDfaAccepting.end();
		for ( typename _TyDfaAcceptingList::iterator itL = m_lDfaAccepting.begin();
					itL != itLEnd;
					++itL )
		{
			m_rDfaCtxt.m_pssAccept->setbit( (size_t)itL->second );

			if ( mapNfaState.end() == ( itNfaState = mapNfaState.find( itL->first ) ) )
			{
				// Find the action object if any:
				typename _TyNfa::_TySetAcceptIT itNfaAccept = m_rNfa.m_pSetAcceptStates->find( itL->first );
				Assert( m_rNfa.m_pSetAcceptStates->end() != itNfaAccept );
				
				_TySetStatesDfa	ssDfa(	static_cast< size_t >( m_rDfa.NStates() ), 
																m_rDfa.get_allocator() );
				ssDfa.clear();
				ssDfa.setbit( (size_t)itL->second );
				typename _TyLocalMap::value_type	vt( itL->first, _TyLMapEl( ssDfa, itNfaAccept->second ) );
				itNfaState = mapNfaState.insert( vt ).first;
			}
			else
			{
				// Action object already copied for this NFA state - set the bit for the next associated
				//	DFA state:
				itNfaState->second.first.RObject().setbit( (size_t)itL->second );
			}
			
			// If we have ambiguous anti-accepting states then we may have a trigger transition
			//	from an anti-accepting state to another accepting state - so in that case remove -
			//	or if creating a dead state move the head to the dead state.
			if ( m_fAmbiguousAntiAccepting && m_rNfa.m_nTriggers )
			{
				Trace( "Have m_fAmbiguousAntiAccepting and triggers." );
				typename _TyLocalMap::value_type & rvt = *itNfaState;
				if ( e_aatAntiAccepting == rvt.second.second.m_eaatType )
				{
					_TyGraphNodeDfa *	pgn = m_rDfa.PGNGetNode( itL->second );
					typedef std::pair< typename _TyDfa::_TyAlphaIndex, typename _TyDfa::_TyAlphaIndex > _TyPrAI;
					_TyPrAI praiTriggers;
					_TyPrAI praiUnsatisfiable;
					m_rDfaCtxt.RDfa().GetTriggerUnsatAIRanges( &praiTriggers, &praiUnsatisfiable );

					_TyGraphNodeDfa *	pgnDead = m_fCreateDeadState ? m_rDfa.PGNGetNode( 0 ) : 0;
					_TyGraphLinkDfa ** ppgl = pgn->PPGLChildHead();
					for ( ; !!*ppgl && ( (*ppgl)->RElConst() >= praiTriggers.first ) && ( (*ppgl)->RElConst() < praiTriggers.second );  )
					{
						_TyGraphLinkDfa * pglCur = *ppgl;
						ppgl = (*ppgl)->PPGLGetNextChild(); // Advance this here since we will be modifying it below.
						if ( m_fCreateDeadState )
						{
							// Ensure any triggers go to dead state:
							if ( pglCur->PGNChild() != pgnDead )
							{
								Assert( pglCur->PGNChild()->UParents() > 1 );
								pglCur->RemoveParent();
								pglCur->InsertParent( pgnDead->PPGLParentHead() );
								pglCur->SetChildNode( pgnDead );
							}
						}
						else
						{
							// Remove all trigger transitions:
							Assert( pglCur->PGNChild()->UParents() > 1 );
							m_rDfa._RemoveLink( pglCur );
						}
					}
					// We don't expect to see unsatisfiable transtions after any triggers - may have to re-evaluate our strategy if we do.
					Assert( !(*ppgl) || ( !( (*ppgl)->RElConst() >= praiUnsatisfiable.first ) && ( (*ppgl)->RElConst() < praiUnsatisfiable.second ) ) );
				}
			}
		}

		typedef typename _TyDfaCtxt::_TyPartAcceptStates	_TyPartAcceptStates;

		if ( m_fAllowReject )
		{
			// First we peruse the lookahead disambiguating states - we further partition
			//	the reject states by the intersection with the disambiguating states and copy
			//	the lookahead accept vector from them to the newly identified partitions - then
			//	removing that parition from the lookahead disambiguating states - to keep states
			//	from being represented twice - this may cause removal of the lookahead disambiguator entirely.
			// We will insert the states uniquely identified by reject vector first into the accepting
			//	state partition. As we do so form a bitvector of states in this group - this allows
			//	us to determine, as we add lookahead disambiguating states, whether a state is already
			//	unique identified by membership in this class - in which case the partition of disambiguating
			//	action needs to be partitioned - copying the lookahead accept vector ( any action will be the same )
			//	to states that are present in the reject vector. Any remaining states remain int the lookahead
			//	accept vector.
		}

		if ( !m_fAllowReject )	// Otherwise the lookahead disambiguating states added above with the
		{												//	reject states.
			// Start constructing the accepting state partition by inserting 
			//	the lookahead disambiguating actions and related states:
			typename _TyAmbigAccept::iterator itAmbigEnd = m_mapAmbigAccept.end();
			typename _TyAmbigAccept::iterator itAmbig = m_mapAmbigAccept.begin();
			for ( ; itAmbigEnd != itAmbig; ++itAmbig )
			{
				typename _TyAmbigAccept::value_type &	rvtAmbig = *itAmbig;
				_TySetStatesDfa	ssDfa(	static_cast< size_t >( m_rDfa.NStates() ), 
																m_rDfa.get_allocator() );
				ssDfa.clear();

				// For each related state of this action set a bit:
				typename _TySetLDStates::iterator itAssoc = rvtAmbig.second.first.begin();
				typename _TySetLDStates::iterator itAssocEnd = rvtAmbig.second.first.end();
				for ( ; itAssoc != itAssocEnd; ++itAssoc )
				{
					ssDfa.setbit( (size_t)*itAssoc);
				}

				// Update accepting state vector:
				*m_rDfaCtxt.m_pssAccept |= ssDfa;

				typename _TyPartAcceptStates::value_type	vt( ssDfa, rvtAmbig.second.second );
				pair< typename _TyPartAcceptStates::iterator, bool >	pib = m_rDfaCtxt.m_partAccept.insert( vt );
				Assert( pib.second );

				// Get the related action id vector from the key:
				if ( rvtAmbig.second.second.m_psrRelated )
				{
					pib.first->second.m_psrRelated->swap( 
						const_cast< typename _TyAcceptAction::_TySetActionIds & >( rvtAmbig.first.first.RObject() ) );
				}

				// Get the trigger vector from the key:
				if ( rvtAmbig.second.second.m_psrTriggers )
				{
					pib.first->second.m_psrTriggers->swap( 
						const_cast< typename _TyAcceptAction::_TySetActionIds & >( rvtAmbig.first.second.RObject() ) );
				}
			}
		}

		// Construct the rest of the accepting state partition for the DFA 
		//	from the NFA state map:
		typename _TyLocalMap::iterator	itNfaStateEnd = mapNfaState.end();
		for ( itNfaState = mapNfaState.begin(); 
					itNfaStateEnd != itNfaState;
					++itNfaState )
		{
			typename _TyPartAcceptStates::value_type	vt( itNfaState->second.first, 
																					itNfaState->second.second );
	#ifndef NDEBUG
			pair< typename _TyPartAcceptStates::iterator, bool >	pibDebug =
	#endif //!NDEBUG
			m_rDfaCtxt.m_partAccept.insert( vt );
	#ifndef NDEBUG
			Assert( pibDebug.second );
	#endif //!NDEBUG
		}

		if ( m_fAmbiguousTriggers )
		{
			// Copy the trigger actions to a separate data structure - this ensures creation
			//	even when ambiguity occurs:
			m_rDfa.m_pMapTriggers.template emplace
				< typename _TyDfa::_TyCompareAI const &, typename _TyDfa::_TyAllocator const & >
				( typename _TyDfa::_TyCompareAI(), m_rDfa.get_allocator() );
																	
			typename _TyNfa::_TySetAcceptIT itNfaAcceptEnd = m_rNfa.m_pSetAcceptStates->end();
			for ( typename _TyNfa::_TySetAcceptIT itNfaAccept = m_rNfa.m_pSetAcceptStates->begin();
						itNfaAccept != itNfaAcceptEnd;
						++itNfaAccept )
			{
				if ( e_aatTrigger == itNfaAccept->second.m_eaatType )
				{
					typename _TyDfa::_TyMapTriggers::value_type	vt( itNfaAccept->second.m_aiAction,
																									itNfaAccept->second );
					m_rDfa.m_pMapTriggers->insert( vt );
				}
			}
		}

	}

};


__REGEXP_END_NAMESPACE


#endif //__L_DFACR_H

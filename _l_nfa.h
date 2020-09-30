#ifndef __L_NFA_H
#define __L_NFA_H

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

#include <map>

__REGEXP_BEGIN_NAMESPACE

// _l_nfa.h

// There are two parts to the representation of an NFA - the dgraph
//	and the alphabet set. As we form the NFA from the regular expression
//	we refine the alphabet set.

// Actually there are some more parts too :-)

template < class t_TyChar > class _nfa_context_base;
template < class t_TyChar, class t_TyAllocator > class _nfa_context;

// class regexp_trigger_first:
// We throw this when we encounter a trigger as the first state in an NFA - it will fire all the time regardless of input - so it is dumb
#ifdef __NAMDDEXC_STDBASE
#pragma push_macro("std")
#undef std
#endif //__NAMDDEXC_STDBASE
class regexp_trigger_found_first_exception : public std::_t__Named_exception< __LEXANG_DEFAULT_ALLOCATOR >
{
  typedef std::_t__Named_exception< __LEXANG_DEFAULT_ALLOCATOR > _TyBase;
public:
  regexp_trigger_found_first_exception() : _TyBase("regexp_trigger_found_first_exception") {}
  regexp_trigger_found_first_exception(const string_type & __s) : _TyBase(__s) {}
};
#ifdef __NAMDDEXC_STDBASE
#pragma pop_macro("std")
#endif //__NAMDDEXC_STDBASE

template < class t_TyChar, class t_TyAllocator = allocator< char > >
class _nfa 
	:	public _fa_alloc_base< t_TyChar, t_TyAllocator >,
		public _alloc_base< char, t_TyAllocator >
{
private:
	typedef _fa_alloc_base< t_TyChar, t_TyAllocator > _TyBase;
	typedef _nfa< t_TyChar, t_TyAllocator > _TyThis;
	typedef _alloc_base< char, t_TyAllocator > _TyCharAllocBase;
protected:
	using _TyBase::m_iCurState;	
	using _TyBase::m_setAlphabet;
	using _TyBase::m_nodeLookup;
	using _TyBase::ms_kreTrigger;
	using _TyBase::ms_kreUnsatisfiableStart;
	using _TyBase::_STUpdateNodeLookup;
	using _TyBase::_STGetSSCache;
public:
	using _TyBase::NStates;

	typedef typename _TyCharAllocBase::size_type size_type;

	// Friends:
	template < class t__TyNfa, class t__TyDfa >
	friend struct _create_dfa;
	friend class _nfa_context< t_TyChar, t_TyAllocator >;

	typedef _nfa_context_base< t_TyChar > _TyNfaCtxtBase;
	typedef _nfa_context< t_TyChar, t_TyAllocator >	_TyNfaCtxt;
	typedef _TyNfaCtxt _TyContext;
	typedef typename _TyBase::_TyState _TyState;
	typedef typename _TyBase::_TyRange _TyRange;
	typedef typename _TyBase::_TyRangeEl _TyRangeEl;
	typedef typename _TyBase::_TySetStates _TySetStates;
	typedef typename _TyBase::_TyAcceptAction _TyAcceptAction;
	typedef typename _TyBase::_TyUnsignedChar _TyUnsignedChar;
	typedef typename _TyBase::_TySdpActionBase _TySdpActionBase;
	typedef typename _TyBase::_TyAlphabet _TyAlphabet;
	typedef typename _TyBase::_TySSCache _TySSCache;

	typedef __DGRAPH_NAMESPACE dgraph< _TyState, _TyRange, false, t_TyAllocator > _TyGraph;
	typedef typename _TyGraph::_TyGraphNode _TyGraphNode;
	typedef typename _TyGraph::_TyGraphLink _TyGraphLink;

	_TyGraph m_gNfa;

	typedef __DGRAPH_NAMESPACE _gr_select_value_modifiable< _TyGraphLink, 
								_fa_char_range_intersect< _TyRange > >	_TyLinkSelect;
	typedef typename _TyGraph::_TyGraphTraits:: 
		template _get_link_select_iter< _TyLinkSelect >::
                             _TyGraphFwdIterSelectConst	_TySelectIterConst;

	// We cache the selection iterator to keep from allocating/deallocating all the time:
	_TySelectIterConst	m_selit;

	bool m_fHaveEmpty;	// Have we already added the empty set to the alphabet lookup.

	// Cache for already computed closure by state:
	char * m_cpClosureCache;
	// bit vector indicating which cache sets are computed.
	_TySetStates m_ssClosureComputed;

	int m_iActionCur;

	typedef less< _TyState > _TyCompareStates;
	typedef typename _Alloc_traits< typename map< _TyState, _TyAcceptAction, _TyCompareStates >::value_type, t_TyAllocator >::allocator_type _TySetAcceptStatesAllocator;
	typedef map< _TyState, _TyAcceptAction, _TyCompareStates, _TySetAcceptStatesAllocator > _TySetAcceptStates;
	typedef typename _TySetAcceptStates::iterator _TySetAcceptIT;
	typedef typename _TySetAcceptStates::value_type _TySetAcceptVT;

	// Sometimes need to order the accept actions by action identifier:
	typedef less< _TyActionIdent > _TyCompareAI;
	typedef typename _Alloc_traits< typename map< _TyActionIdent, typename _TySetAcceptStates::value_type *, _TyCompareAI >::value_type, t_TyAllocator >::allocator_type _TySetASByActionIDAllocator;
	typedef map< _TyActionIdent, typename _TySetAcceptStates::value_type *, _TyCompareAI, _TySetASByActionIDAllocator > _TySetASByActionID;

	_sdpd< _TySetAcceptStates, t_TyAllocator > m_pSetAcceptStates;
	_sdpd< _TySetASByActionID, t_TyAllocator > m_pLookupActionID;
	bool m_fHasLookaheads;
	bool m_nTriggers;
	bool m_fHasFreeActions;
	int m_nUnsatisfiableTransitions;

	_nfa( _nfa const & ) = delete;
	_nfa operator = ( _nfa const & ) = delete;

	_nfa( t_TyAllocator const & _rAlloc = t_TyAllocator() )
		:	 _TyBase( _rAlloc ),
			_TyCharAllocBase( _rAlloc ),
			m_fHaveEmpty( false ),
			m_gNfa( typename _TyGraph::_TyAllocatorSet( _rAlloc, _rAlloc, _rAlloc ) ),
			m_selit( 0, 0, true, true, _rAlloc, _TyLinkSelect( _TyRange( 0, 0 ) ) ),
			m_ssClosureComputed( 0, _rAlloc ),
			m_cpClosureCache( 0 ),
			m_iActionCur( 0 ),
			m_pSetAcceptStates( get_allocator() ),
			m_pLookupActionID( get_allocator() ),
			m_fHasLookaheads( false ),
			m_nTriggers( 0 ),
			m_fHasFreeActions( false ),
			m_nUnsatisfiableTransitions( 0 )
	{
		m_pSetAcceptStates.template emplace< const _TyCompareStates &, const t_TyAllocator & >( _TyCompareStates(), get_allocator() );
		// m_pSetAcceptStates.template emplace< const _TyCompareStates &, const t_TyAllocator & >
		// 															( _TyCompareStates(), get_allocator() );
	}

	~_nfa()
	{
		DeallocClosureCache();
	}

	t_TyAllocator get_allocator() const _BIEN_NOTHROW	{ return _TyCharAllocBase::get_allocator(); }

	void	AllocClosureCache()
	{
		_TySetStates	ssComputed( NStates(), get_allocator() );
		m_ssClosureComputed.swap( ssComputed );
		m_ssClosureComputed.clear();

		assert( !m_cpClosureCache );

		_TyCharAllocBase::allocate_n( m_cpClosureCache, 
																	m_ssClosureComputed.size_bytes() * NStates() );
	}

	void	DeallocClosureCache()
	{
		if ( m_cpClosureCache )
		{
			_TyCharAllocBase::deallocate_n( m_cpClosureCache, m_ssClosureComputed.size_bytes() * NStates() );
			m_cpClosureCache = 0;
		}
		_TySetStates	ssComputed( 0, get_allocator() );
		m_ssClosureComputed.swap( ssComputed );
	}

	char *	PCGetClosureCache( size_type _st )
	{
		return m_cpClosureCache + m_ssClosureComputed.size_bytes() * _st;
	}

	_TyGraphNode *	PGNGetNode( _TyState _iState )
	{
		return static_cast< _TyGraphNode * >( m_nodeLookup[ _iState ] );
	}

	// Attempt to match the passed null terminated string:
	bool	MatchString(	_TyNfaCtxt & _rctxt, const t_TyChar * _pc,
											_TySetStates * _pssResultAccepting = 0 )
	{
		AllocClosureCache();

		_TySetStates * pssAccepting;
		CMFDtor1_void< _TyThis, size_t >
			releaseSSAccepting( this, &_TyThis::_ReleaseSSCache, _STGetSSCache(pssAccepting) );
		pssAccepting->clear();
		_rctxt.GetAcceptingNodeSet( *pssAccepting );

		// This is going to be pretty slow - but that's what you get matching with an NFA:
		_TySetStates * pssCur;
		CMFDtor1_void< _TyThis, size_t >
			releaseSSCur( this, &_TyThis::_ReleaseSSCache, _STGetSSCache(pssCur) );
		pssCur->clear();
		Closure( _rctxt.m_pgnStart, *pssCur, true );

		_TySetStates * pssMove;
		CMFDtor1_void< _TyThis, size_t >
			releaseSSMove( this, &_TyThis::_ReleaseSSCache, _STGetSSCache(pssMove) );
		pssMove->clear();

		while( *_pc && !pssCur->empty() )
		{
			assert( pssMove->empty() );
			ComputeSetMoveStates( *pssCur, _TyRange( *_pc, *_pc ), *pssMove );
			assert( pssCur->empty() );
			ComputeSetClosure( *pssMove, *pssCur );
			if ( !pssCur->empty() )
			{
				_pc++;
			}
		}

		DeallocClosureCache();

		if ( _pssResultAccepting )
		{
			// caller wants accepting states:
			return _pssResultAccepting->intersection( *pssCur, *pssAccepting );
		}
		else
		{
			// Just need to know if any intersections occur:
			return pssCur->FIntersects( *pssAccepting );
		}
	}

	void Dump( ostream & _ros, _TyNfaCtxt const & _rCtxt ) const
	{
		// Dump the alphabet and the graph:
		_TyBase::DumpAlphabet( _ros );
		_ros << "NFA:\n";	
		_ros << "Start state : {" << _rCtxt.m_pgnStart->RElConst() << "}.\n";
		if ( _rCtxt.m_pssAccept )
		{
			_ros << "Accepting states :";
			_TyBase::DumpStates( _ros, *_rCtxt.m_pssAccept );
			_ros << "\n";
		}
		else
		if ( _rCtxt.m_pgnAccept )
		{
			_ros << "Accepting state : {" << _rCtxt.m_pgnAccept->RElConst() << "}.\n";
		}

		m_gNfa.dump_node( _ros, _rCtxt.m_pgnStart, __DGRAPH_NAMESPACE e_doMapPointers );
	}

	void ToJSONStream( JsonValueLifeAbstractBase< t_TyChar > & _jvl, _TyNfaCtxt const & _rCtxt ) const
	{
		assert( _jvl.FAtObjectValue() ); // Will throw below if we aren't...
		// Dump the alphabet and the graph:
		{//B
			std::unique_ptr< JsonValueLifeAbstractBase< t_TyChar > > pjvlAlphabet;
			_jvl.NewSubValue( str_array_cast< t_TyChar >( "NFA:Alphabet" ), ejvtArray, pjvlAlphabet );
			_TyBase::AlphabetToJSON( *pjvlAlphabet );
		}//EB

		_jvl.WriteValue( str_array_cast< t_TyChar >( "NFA:StartState" ), _rCtxt.m_pgnStart->RElConst() );

		if ( !!_rCtxt.m_pssAccept )
		{
			std::unique_ptr< JsonValueLifeAbstractBase< t_TyChar > > pjvlStates;
			_jvl.NewSubValue( str_array_cast< t_TyChar >( "NFA:AcceptingStates" ), ejvtArray, pjvlStates );
			_TyBase::StatesToJSON( *pjvlStates, *_rCtxt.m_pssAccept );
		}
		else
		if ( !!_rCtxt.m_pgnAccept )
		{
			_jvl.WriteValue( str_array_cast< t_TyChar >( "NFA:AcceptingState" ), _rCtxt.m_pgnAccept->RElConst() );
		}

		// TODO:
		//m_gNfa.dump_node( _ros, _rCtxt.m_pgnStart );
	}

protected:	// accessed by _nfa_context:

	void	DestroySubGraph( _TyGraphNode * _pgn )
	{
		if ( !!_pgn )
			m_gNfa.destroy_node( _pgn );
	}

	void	CreateRangeNFA( _TyNfaCtxt & _rctxt, _TyRange const & _rr )
	{
    	assert( !_rctxt.m_pgnStart ); // throw-safety.
		_NewStartState( &_rctxt.m_pgnStart );
		_NewAcceptingState( _rctxt.m_pgnStart, _rr, &_rctxt.m_pgnAccept );
	}

	void	CreateStringNFA( _TyNfaCtxt & _rctxt, _TyUnsignedChar const * _pc )
	{
		if ( *_pc )
		{
			assert( !_rctxt.m_pgnStart );
			_NewStartState( &_rctxt.m_pgnStart );
			_rctxt.m_pgnAccept = _rctxt.m_pgnStart;

			// For each character in the string add a new state:
			while ( *_pc )
			{
				_NewAcceptingState( _rctxt.m_pgnAccept, _TyRange( *_pc, *_pc ), &_rctxt.m_pgnAccept );
				++_pc;
			}
		}
		else
		{
			// Create an empty NFA:
			CreateRangeNFA( _rctxt, _TyRange( 0, 0 ) );
		}
	}
	void	CreateFollowsNFA( _TyNfaCtxt & _rctxt1_Result, _TyNfaCtxt & _rctxt2 )
	{
		// Need to do a node-splice - easy since each has no siblings in the right direction:
		assert( !_rctxt1_Result.m_pgnAccept->FChildren() );
		assert( !_rctxt2.m_pgnStart->FParents() );

		(*(_rctxt2.m_pgnStart->PPGLChildHead()))->AppendChildListToTail( 
			_rctxt1_Result.m_pgnAccept->PPGLBChildHead() );

		*(_rctxt2.m_pgnStart->PPGLChildHead()) = 0;

		// Now move through and update the parent node in each link:
		for (	typename _TyGraph::_TyGraphLinkBaseBase ** ppglb = 
						_rctxt1_Result.m_pgnAccept->PPGLBChildHead();
					*ppglb; ppglb = (*ppglb)->PPGLBGetNextChild() )
		{
			(*ppglb)->SetParentNode( _rctxt1_Result.m_pgnAccept );
		}

		DestroySubGraph( _rctxt2.m_pgnStart );
		_rctxt2.m_pgnStart = 0;	// <_rctxt1_Result> now owns this NFA.
		_rctxt1_Result.m_pgnAccept = _rctxt2.m_pgnAccept;
	}
	void	CreateOrNFA( _TyNfaCtxt & _rctxt1_Result, _TyNfaCtxt & _rctxt2 )
	{
		_TyGraphNode *	pgnStart = 0;
		CMFDtor1_void< _TyThis, _TyGraphNode * >	
			dtorSubGraph( this, &_TyThis::DestroySubGraph, pgnStart );
		_NewStartState( &pgnStart );

		_NewAcceptingState( _rctxt2.m_pgnAccept, 
												_TyRange( 0, 0 ), &_rctxt2.m_pgnAccept );
		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt2.m_pgnStart );
		dtorSubGraph.Reset();
		_rctxt2.m_pgnStart = pgnStart;

		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt1_Result.m_pgnStart );
		_rctxt1_Result.m_pgnStart = pgnStart;
		_rctxt2.m_pgnStart = 0;	// now owned by _rctxt1_Result.

		_NewTransition( _rctxt1_Result.m_pgnAccept, 
										_TyRange( 0, 0 ), _rctxt2.m_pgnAccept );
		_rctxt1_Result.m_pgnAccept = _rctxt2.m_pgnAccept;
	}

	void	CreateExcludesNFA( _TyNfaCtxt & _rctxt1_Result, _TyNfaCtxt & _rctxt2 )
	{
	// Just like the or except don't connect the accepting states together:
		_TyGraphNode *	pgnStart = 0;
		CMFDtor1_void< _TyThis, _TyGraphNode * >	
			dtorSubGraph( this, &_TyThis::DestroySubGraph, pgnStart );
		_NewStartState( &pgnStart );

		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt2.m_pgnStart );
		dtorSubGraph.Reset();
		_rctxt2.m_pgnStart = pgnStart;

		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt1_Result.m_pgnStart );
		_rctxt1_Result.m_pgnStart = pgnStart;
		_rctxt2.m_pgnStart = 0;	// now owned by _rctxt1_Result.

		// We must disambiguate the non-accepting state - add to the accepting state partition:
		_TyAcceptAction	aa( m_iActionCur++, 0 );
		aa.m_eaatType = e_aatAntiAccepting;
		_TySetAcceptVT	vtAcceptState( _rctxt2.m_pgnAccept->RElConst(), aa );

#ifndef NDEBUG
		pair<_TySetAcceptIT,bool> pib =
#endif //!NDEBUG
		m_pSetAcceptStates->insert( vtAcceptState );
		assert( pib.second );
	}

	void	_Renumber(	_TyGraphNode * _pgnNew, _TyState _stAcceptOld, 
										_TyGraphNode ** _ppgnAcceptNew )
	{
		typename _TyGraph::iterator	gfit( _pgnNew, 0, false, true, get_allocator() );
		for( ; !gfit.FAtEnd(); ++gfit )
		{
			if ( !gfit.PGLCur() )
			{
				_TyGraphNode *	pgn = gfit.PGNCur();
				if ( _stAcceptOld == pgn->RElConst() )
				{
					*_ppgnAcceptNew = pgn;
				}
				pgn->RElNonConst() = m_iCurState++;
				assert( pgn->RElNonConst() == m_nodeLookup.size() );
				(void)_STUpdateNodeLookup( pgn );
			}
		}
	}

	void	CreateCompletesNFA( _TyNfaCtxt & _rctxt1_Result, _TyNfaCtxt & _rctxt2 )
	{
		_TyNfaCtxt *	pcNfaTemp;
		_rctxt1_Result.Clone( reinterpret_cast< _TyNfaCtxtBase ** >( &pcNfaTemp ) );
		CMFDtor1_void< _TyNfaCtxt, _TyNfaCtxtBase * >	
			dtorNfa2( &_rctxt1_Result, &_TyNfaCtxt::DestroyOther, pcNfaTemp );

		__DGRAPH_NAMESPACE _graph_copy_struct< _TyGraph, _TyGraph, t_TyAllocator >
			gcs( m_gNfa, m_gNfa, false, true, get_allocator() );
		gcs.m_pgnSrcCopyRoot = _rctxt1_Result.m_pgnStart;
		gcs.copy();
		pcNfaTemp->m_pgnStart = gcs.PGNTransferNewRoot();

		_Renumber( pcNfaTemp->m_pgnStart, 
			_rctxt1_Result.m_pgnAccept->RElConst(), &pcNfaTemp->m_pgnAccept );

		CreateFollowsNFA( _rctxt1_Result, _rctxt2 );
		CreateUnsatisfiableNFA( _rctxt2, 0 );
		CreateOrNFA( *pcNfaTemp, _rctxt2 );
		CreateZeroOrMoreNFA( *pcNfaTemp );
		CreateUnsatisfiableNFA( _rctxt2, 1 );
		CreateFollowsNFA( *pcNfaTemp, _rctxt2 );
		CreateUnsatisfiableNFA( _rctxt2, 1 );
		CreateFollowsNFA( *pcNfaTemp, _rctxt2 );

		CreateFollowsNFA( _rctxt1_Result, *pcNfaTemp );
	}

	void	CreateZeroOrMoreNFA( _TyNfaCtxt & _rctxt )
	{
		_TyGraphNode *	pgnStart = 0;
		CMFDtor1_void< _TyThis, _TyGraphNode * >	
			dtorSubGraph( this, &_TyThis::DestroySubGraph, pgnStart );
		_NewStartState( &pgnStart );

		_TyGraphNode *	pgnAccept;
		_NewAcceptingState( pgnStart, _TyRange( 0, 0 ), &pgnAccept );

		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt.m_pgnStart );
		dtorSubGraph.Reset();
		swap( _rctxt.m_pgnStart, pgnStart );

		_NewTransition( _rctxt.m_pgnAccept, _TyRange( 0, 0 ), pgnAccept );
		_NewTransition( _rctxt.m_pgnAccept, _TyRange( 0, 0 ), pgnStart );
		_rctxt.m_pgnAccept = pgnAccept;
	}
	void	CreateLookaheadNFA( _TyNfaCtxt & _rctxt1_Result, 
														_TyNfaCtxt & _rctxt2 )
	{
		_rctxt1_Result.m_pgnAltAccept = _rctxt1_Result.m_pgnAccept;
		CreateFollowsNFA( _rctxt1_Result, _rctxt2 );
	}

	void	CreateTriggerNFA( _TyNfaCtxt & _rctxt )
	{
    assert( !_rctxt.m_pgnStart ); // throw-safety.
    // If the current state is 0 then we are starting all productions with a trigger. This doesn't seem to make much sense and indeed the trigger fires regardless
    //  so I am going to throw an error if the current state is 0 when this trigger is encountered.
    if (!m_iCurState)
      throw regexp_trigger_found_first_exception();

		_NewStartState( &_rctxt.m_pgnStart );
		_NewAcceptingState( _rctxt.m_pgnStart, 
												_TyRange( 0, 0 ), 
												&_rctxt.m_pgnAltAccept );
		_NewAcceptingState(	_rctxt.m_pgnAltAccept, 
												_TyRange( ms_kreTrigger, ms_kreTrigger ), 
												&_rctxt.m_pgnAccept );
	}

	void	CreateUnsatisfiableNFA( _TyNfaCtxt & _rctxt, size_t _nUnsatisfiable )
	{
		CreateRangeNFA( _rctxt, 
			_TyRange( (_TyRangeEl)( ms_kreUnsatisfiableStart + _nUnsatisfiable ),
								(_TyRangeEl)( ms_kreUnsatisfiableStart + _nUnsatisfiable ) ) );
		m_nUnsatisfiableTransitions = max( int( _nUnsatisfiable+1 ), m_nUnsatisfiableTransitions );
	}

	void	AddTrigger( _TyState _st, const _TySdpActionBase * _pSdpAction )
	{
		assert( _pSdpAction );
		// Store the trigger accept state and its related action:
		++m_nTriggers;
		_TyAcceptAction	aa( m_iActionCur++, _pSdpAction );
		aa.m_eaatType = e_aatTrigger;
		_TySetAcceptVT	vtAcceptState( _st, aa );
#ifndef NDEBUG
		pair< _TySetAcceptIT, bool > pib =
#endif //!NDEBUG
		m_pSetAcceptStates->insert( vtAcceptState );
		assert( pib.second );
	}

	void	AddAction( _TyState _st, const _TySdpActionBase * _pSdpAction )
	{
		assert( _pSdpAction );
		// Store the accept state and its related action:
		m_fHasFreeActions = true;
		_TyAcceptAction	aa( m_iActionCur++, _pSdpAction );
		_TySetAcceptVT	vtAcceptState( _st, aa );
#ifndef NDEBUG
		pair< _TySetAcceptIT, bool > pib =
#endif //!NDEBUG
		m_pSetAcceptStates->insert( vtAcceptState );
		assert( pib.second );
	}

	void CreateAltRuleNFA( _TyNfaCtxt & _rctxt )
	{
		_TyGraphNode *	pgnStart = 0;
		CMFDtor1_void< _TyThis, _TyGraphNode * >	
			dtorSubGraph( this, &_TyThis::DestroySubGraph, pgnStart );
		_NewStartState( &pgnStart );

		_NewTransition( pgnStart, _TyRange( 0, 0 ), _rctxt.m_pgnStart );
		dtorSubGraph.Reset();
		_rctxt.m_pgnStart = pgnStart;
	}

	void	AddAlternativeNFA( _TyNfaCtxt & _rctxt_Result, _TyNfaCtxt & _rctxt2 )
	{
		_NewTransition( _rctxt_Result.m_pgnStart, _TyRange( 0, 0 ), _rctxt2.m_pgnStart );
		_rctxt2.m_pgnStart = 0;
	}

	void	_AddAccept( _TyNfaCtxt & _rCtxt )
	{
		assert( _rCtxt.m_pgnAccept );
		_TyAcceptAction	aa( m_iActionCur++, _rCtxt.m_pSdpAction );
		// If this accept state has a related lookahead intermediate accept state
		//	( the actual accept state for expression ) then relate the two
		//	via action identifier. Then when we are generating the final DFA
		//	we will associate the two in the state tables.
		_TySetAcceptIT	itLookaheadAccept;
		if ( _rCtxt.m_pgnAltAccept )
		{
			// The action remains associated with the lookahead state:
			_TyAcceptAction	aaInter( m_iActionCur++, 0 );
			// REVIEW: If we know that the lookahead pattern for this can be empty ( which
			//	sounds kind of silly, I know ) then we can change the precedence - which along
			//	with some special disambiguating code will appropriately ( in most cases )
			//	set up a special action. For now this is more generally correct:
			//	if ( _rCtxt.m_fLookaheadAllowsEmpty )
			//	{
			//		swap( aaInter.m_aiAction , aa.m_aiAction );	// The accept state has higher precedence during disambiguation.
			//	}
			aaInter.m_eaatType = e_aatLookaheadAccept;
			aaInter.m_aiRelated = aa.m_aiAction;
			_TySetAcceptVT	vtInter( _rCtxt.m_pgnAltAccept->RElConst(), aaInter );
			_rCtxt.m_pgnAltAccept = 0;
			pair< _TySetAcceptIT, bool >	pib = m_pSetAcceptStates->insert( vtInter );
			assert( pib.second );	// If this assert fires then we need some code similar to that of below ( though not sure if it will work ).
			itLookaheadAccept = pib.first;

			// Also relate the lookahead state:
			aa.m_eaatType = e_aatLookahead;
			aa.m_aiRelated = aaInter.m_aiAction;

			m_fHasLookaheads = true;	// This nfa has lookaheads.
		}

		_TySetAcceptVT	vtAcceptState( _rCtxt.m_pgnAccept->RElConst(), aa );
		pair< _TySetAcceptIT, bool >	pib = m_pSetAcceptStates->insert( vtAcceptState );
		if ( !pib.second )
		{
			assert( !pib.first->second.m_pSdpAction );
			// Then we have already set the action id for this action - use
			//	the action id as set - it disambiguates accepting states:
			assert( aa.m_eaatType != e_aatLookahead );	// This isn't currently supported ( i don't think ).

			vtAcceptState.second.m_aiAction = pib.first->second.m_aiAction;
			if ( aa.m_eaatType == e_aatLookahead )
			{
				// Update the lookahead accepting state's reference to this action:
				itLookaheadAccept->second.m_aiRelated = vtAcceptState.second.m_aiAction;
			}
			else
			{
				// By default the related and the action are the same ( this is an invariant ):
				vtAcceptState.second.m_aiRelated = vtAcceptState.second.m_aiAction;
			}
			m_pSetAcceptStates->erase( pib.first );
			pib = m_pSetAcceptStates->insert( vtAcceptState );
			assert( pib.second );
		}
		_rCtxt.m_pgnAccept = 0;
		_rCtxt.m_pSdpAction.Release();
	}

	void	_InitSetAcceptStates( _TyNfaCtxt & _rctxtAdd )
	{
		_AddAccept( _rctxtAdd );
	}

	void	CreateActionIDLookup()
	{
		if ( !m_pLookupActionID )
		{
			m_pLookupActionID.template emplace< const _TyCompareAI &, const t_TyAllocator & >
												( _TyCompareAI(), get_allocator() );
			_TySetAcceptIT itEnd = m_pSetAcceptStates->end();
			for ( _TySetAcceptIT it = m_pSetAcceptStates->begin();
						it != itEnd;
						++it )
			{
				typename _TySetASByActionID::value_type	vt( it->second.m_aiAction, &*it );
				m_pLookupActionID->insert( vt );
			}
		}
	}

	typedef void ( _TyThis:: * _TyPMFnComputeSingleSet )( const _TyGraphNode * _pgnStart,
																												_TyRange const & _rInput,
																												_TySetStates & _rsetResult );

	void	ComputeSetClosure(	_TySetStates & _rsetStart,
														_TySetStates & _rsetResult )
	{
		assert( _rsetResult.empty() );
		if ( _rsetStart.empty() )
			return;

		// Re-use the sets for efficiency:
		_TySetStates * pssCur;
		CMFDtor1_void< _TyThis, size_t >
			releaseSS( this, &_TyThis::_ReleaseSSCache, _STGetSSCache(pssCur) );
		// REVIEW:<dbien>: It seems that pssCur contains garbage, but that it *probably* doesn't matter in this case.
		// 	(i.e. unless we can encounter the current state during the selection iters perusal of the graph - which I am guess for regular expressions we cannot - so it doens't matter here)
		// However the cost of clearing it is negligible so let's do that:
		pssCur->clear();

		// Find a state in <_rsetStart>:
		_TyState nState;
		for ( nState = (_TyState)_rsetStart.getclearfirstset();
					_rsetStart.size() != nState;
					nState = (_TyState)_rsetStart.getclearfirstset( nState ) )
		{
			assert( nState < (_TyState)m_nodeLookup.size() );

			// We may have already computed the closure for this state:
			if ( m_ssClosureComputed.isbitset( nState ) )
			{
				_rsetResult.or_equals( (typename _TySetStates::_TyEl*)PCGetClosureCache( nState ) );
			}
			else
			{
				// Then we are going to compute the closure cache for this state:
				m_ssClosureComputed.setbit( nState );
				typename _TySetStates::_TyEl * pssVector = (typename _TySetStates::_TyEl*)PCGetClosureCache( nState );
				pssCur->swap_vector( pssVector );
				CMFDtor1_void< _TySetStates, typename _TySetStates::_TyEl *& >
					swapBack( pssCur, &_TySetStates::swap_vector, pssVector );
							
				pssCur->clear();

				m_selit.SetPGNBCur( const_cast< _TyGraphNode * >( PGNGetNode( nState ) ) );
				m_selit.Reset();

				pssCur->setbit( nState );	// add initial state.

				++m_selit;
				while( !m_selit.FAtEnd() )
				{
					// If we are currently at a node then add it:
					if ( !m_selit.PGLCur() )
					{
						const _TyGraphNode * _pgnCur = m_selit.PGNCur();
						// Check to see if we have computed the closure for this state already:
						if ( m_ssClosureComputed.isbitset( _pgnCur->RElConst() ) )
						{
							// Then closure already computed for this node:
							pssCur->or_equals( (typename _TySetStates::_TyEl*)PCGetClosureCache( _pgnCur->RElConst() ) );
							// Instruct the graph iterator not to iterate below this node:
							m_selit.SkipContext();
							continue;
						}
						else
						{
							pssCur->setbit( _pgnCur->RElConst() );
						}
					}
					++m_selit;
				}
				_rsetResult |= *pssCur;
			}
		}

		assert( _rsetStart.empty() );
	}

	void	ComputeSetMoveStates(	_TySetStates & _rsetStart,
															_TyRange const & _rInput,
															_TySetStates & _rsetResult )
	{
		assert( !_rsetStart.empty() );
		assert( _rsetResult.empty() );

		// Re-use the sets for efficiency:
		_TySetStates * pssCur;
		CMFDtor1_void< _TyThis, size_t >
			releaseSS( this, &_TyThis::_ReleaseSSCache, _STGetSSCache(pssCur) );

		// Find a state in <_rsetStart>:
		_TyState nState;
		for ( nState = (_TyState)_rsetStart.getclearfirstset();
					_rsetStart.size() != nState;
					nState = (_TyState)_rsetStart.getclearfirstset( nState ) )
		{
			assert( nState < (_TyState)m_nodeLookup.size() );
			pssCur->clear();
			ComputeMoveStates( PGNGetNode( nState ), _rInput, *pssCur );
			_rsetResult |= *pssCur;
		}

		assert( _rsetStart.empty() );
	}

	// Sometimes get internal compiler errors when this is the method below:
	// Compute the closure of a single state acccording to given range input:
	void	Closure(	const _TyGraphNode * _pgnStart,
									_TySetStates & _rsetResult,
									bool _fUseClosureCache )
	{
		assert( _rsetResult.empty() );

		m_selit.SetPGNBCur( const_cast< _TyGraphNode * >( _pgnStart ) );
		m_selit.Reset();

		_rsetResult.setbit( _pgnStart->RElConst() );	// add initial state.

		while( !(++m_selit).FAtEnd() )
		{
			// If we are currently at a node then add it:
			if ( !m_selit.PGLCur() )
			{
				const _TyGraphNode * _pgnCur = m_selit.PGNCur();
				_rsetResult.setbit( _pgnCur->RElConst() );
			}
		}

		if ( _fUseClosureCache )
		{
			// Copy the result into the closure cache:
			memcpy( PCGetClosureCache( _pgnStart->RElConst() ), 
							_rsetResult.begin(), _rsetResult.size_bytes() );
		}
	}

	// Compute the nodes reachable from <_pgnStart> on input <_rInput>.
	// No need to get a full blown iterator - just check direct descendents
	//	of this node.
	void	ComputeMoveStates(	const _TyGraphNode * _pgnStart,
														_TyRange const & _rInput,
														_TySetStates & _rsetResult )
	{
		typename _TyGraph::_TyLinkPosIterConst	lpi( _pgnStart->PPGLChildHead() );
		while( !lpi.FIsLast() )
		{
			if ( (*lpi).intersects( _rInput ) )
			{
				_rsetResult.setbit( lpi.PGNChild()->REl() );
			}
			lpi.NextChild();
		}
	}

  // Caller owns lifetime of result *_ppgn.
	void		_NewStartState( _TyGraphNode ** _ppgn )
	{
		*_ppgn = m_gNfa.create_node1( m_iCurState );
    size_t stNodeNumAdded = _STUpdateNodeLookup( *_ppgn );
    assert(_TyState(stNodeNumAdded) == m_iCurState);
		m_iCurState++;
	}

  // Caller does not own the lifetime of result *_ppgnAccept.
	void		_NewAcceptingState( _TyGraphNode * _pgnSrc, 
															_TyRange const & _r,
															_TyGraphNode ** _ppgnAccept )
	{
		_UpdateAlphabet( _r );

		_TyGraphLink * pgl = m_gNfa.template create_link1
      < _TyRange const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gNfa, &_TyGraph::destroy_link, pgl );
#else //__DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif //__DGRAPH_INSTANCED_ALLOCATORS

    *_ppgnAccept = m_gNfa.create_node1( m_iCurState );
		CMFDtor1_void< _TyGraph, _TyGraphNode * >
			endNode( &m_gNfa, &_TyGraph::destroy_node, *_ppgnAccept );

		_pgnSrc->AddChild(	**_ppgnAccept, *pgl, 
												*(_pgnSrc->PPGLBChildHead()),
												*((*_ppgnAccept)->PPGLBParentHead()) );
    // no throw here
		endNode.Reset();
		endLink.Reset();
    // can throw after this.
	
		size_t stNodeAdded = _STUpdateNodeLookup( *_ppgnAccept );
    assert(_TyState(stNodeAdded) == m_iCurState);
		m_iCurState++;
	}

	void _NewTransition( _TyGraphNode * _pgnSrc, 
						_TyRange const & _r,
						_TyGraphNode * _pgnSink )
	{
		_UpdateAlphabet( _r );

		_TyGraphLink *	pgl = m_gNfa.
      template create_link1< _TyRange const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gNfa, &_TyGraph::destroy_link, pgl );
#else //__DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif //__DGRAPH_INSTANCED_ALLOCATORS

		_pgnSrc->AddChild(	*_pgnSink, *pgl, 
							*(_pgnSrc->PPGLBChildHead()),
							*(_pgnSink->PPGLBParentHead()) );
    // no throwing here.
		endLink.Reset();
	}

	void _UpdateAlphabet( _TyRange const & _r )
	{
		// Special case empty:
		if ( !_r.first )
		{
			assert( !_r.second );
			if ( !m_fHaveEmpty )
			{
				m_setAlphabet.insert( m_setAlphabet.begin(), _r );
				m_fHaveEmpty = true;
			}
		}
		else
		{
			assert( _r.first <= _r.second );
			// Increase granularity and extent of alphabet set if required:
			typename _TyAlphabet::iterator itLower = m_setAlphabet.lower_bound( _r );
			typename _TyAlphabet::iterator itUpper = m_setAlphabet.upper_bound( _r );

			// Move from the low to the high, inserting records in the spaces:
			_TyRangeEl	cLow = _r.first;
			for( ; itLower != itUpper; )
			{
				assert( cLow <= _r.second );

				typename _TyAlphabet::value_type const & rvt = *itLower;
				if ( cLow < rvt.first )
				{
					m_setAlphabet.insert( itLower++, _TyRange( cLow, rvt.first-1 ) );
					cLow = rvt.second+1;
				}
				else
				{
					assert( cLow <= rvt.second );
					// If the low splits the current range then need to split
					if ( cLow > rvt.first )
					{
						swap( const_cast< _TyRange& >( rvt ).second, cLow );
						const_cast< _TyRange& >( rvt ).second--;
						if ( cLow > _r.second )
						{
							// Then we have split one range into three:
							m_setAlphabet.insert( ++itLower, _TyRange( rvt.second+1, _r.second ) );
							m_setAlphabet.insert( itLower, _TyRange( _r.second+1, cLow ) );
							assert( itLower == itUpper );
						}
						else
						{
							m_setAlphabet.insert( ++itLower, _TyRange( rvt.second+1, cLow++ ) );
						}
					}
					else
					{
						cLow = rvt.second+1;
						++itLower;
					}
				}
			}

			if ( cLow <= _r.second )
			{
				m_setAlphabet.insert( itLower, _TyRange( cLow, _r.second ) );
			}
		}
	}

};

template < class t_TyChar, class t_TyAllocator = allocator< char > >
class _nfa_context
	: public _nfa_context_base< t_TyChar >
{
private:
	typedef _nfa_context_base< t_TyChar >						_TyBase;
	typedef _nfa_context< t_TyChar, t_TyAllocator >	_TyThis;
	typedef typename _Alloc_traits< _TyThis, t_TyAllocator >::allocator_type	_TyAllocThis;
public:
	using _TyBase::m_rFaBase;

	typedef _nfa< t_TyChar, t_TyAllocator >	_TyNfa;
	typedef typename _TyNfa::_TyGraphNode _TyGraphNode;
	typedef typename _TyNfa::_TyGraphLink _TyGraphLink;
	typedef typename _TyNfa::_TySetStates _TySetStates;
	typedef typename _TyNfa::_TyUnsignedChar _TyUnsignedChar;
	typedef typename _TyBase::_TySdpActionBase _TySdpActionBase;
	typedef typename _TyBase::_TyRange _TyRange;
	typedef typename _TyBase::_TyState _TyState;
 
	_TyGraphNode * m_pgnStart;

	_sdpd< _TySetStates, t_TyAllocator > m_pssAccept;

	_TyGraphNode * m_pgnAccept;
	_dtorp< _TySdpActionBase > m_pSdpAction;	// The action associated with this _nfa_context.
	_TyGraphNode * m_pgnAltAccept;

	_nfa_context( _TyNfa & _rNfa )
		:	_TyBase( _rNfa ),
			m_pgnStart( 0 ),
			m_pgnAltAccept( 0 ),
			m_pssAccept( _rNfa.get_allocator() )
	{
	}

	_nfa_context( _TyThis const & _r )
		:	_TyBase( _r ),
			m_pgnStart( 0 ),
			m_pgnAltAccept( 0 ),
			m_pssAccept( _r.RNfa().get_allocator() )
	{
	}

	~_nfa_context()
	{
		if ( m_pgnStart )
		{
			// We own this node - destroy it:
			RNfa().DestroySubGraph( m_pgnStart );
		}
	}

	_TyNfa & RNfa() { return static_cast< _TyNfa & >( m_rFaBase ); }
	_TyNfa const & RNfa() const { return static_cast< const _TyNfa & >( m_rFaBase ); }

	void SetAction( const _TySdpActionBase * _pSdp, enum EActionType _eat = e_atNormal )
	{
		switch( _eat )
		{
			case e_atTrigger:
			{
				// Then we register this state with the NFA itself:
				// NOTE: It is the start state of the trigger that is 
				//	registered as the accepting state - this is because the trigger
				//	is executed on the transition to the trigger accept state.
				assert( _pSdp );
				RNfa().AddTrigger( m_pgnAltAccept->RElConst(), _pSdp );
				m_pgnAltAccept = 0;
			}
			break;
			case e_atFreeAction:
			{
				RNfa().AddAction( m_pgnAccept->RElConst(), _pSdp );
			}
			break;
			default:
			{
				assert( !m_pSdpAction.Ptr() );
				_pSdp->clone( &m_pSdpAction.PtrRef() ); // Copy the action.
			}
			break;
		}
	}

	void GetAcceptingNodeSet( _TySetStates & _rss )
	{
		if ( m_pgnAccept )
		{
			// Default context just has one accepting state - convert to a partition now:
			RNfa()._InitSetAcceptStates( *this );
		}

		if ( !m_pssAccept )
		{
			_CreateAcceptingNodeSet();
		}			
		_rss = *m_pssAccept;
	}
	
	// Clone() and DestroyOther() are used as a pair to maintain the lifetime of any cloned
	// _nfa_context. (*this)'s allocator is used because the library itself doesn't call new() or delete() it always allocates using allocators.
	void Clone( _TyBase ** _pp ) const
	{
		// Use the allocator that we have:
		_sdp< _TyThis, _TyAllocThis >	 pAllocate( RNfa().get_allocator() );
		pAllocate.allocate();
		*_pp = new( pAllocate ) _TyThis( *this );
		pAllocate.transfer();
	}

	void DestroyOther( _TyBase * _pb ) _BIEN_NOTHROW
	{
		_sdp< _TyThis, _TyAllocThis > pDeallocate( static_cast< _TyThis * >( _pb ), RNfa().get_allocator() );
		_pb->~_TyBase();	// Call virtual destructor.
	}

	void Dump( ostream & _ros ) const
	{
		// Dump the alphabet and the graph:
		RNfa().Dump( _ros, *this );
	}
	void ToJSONStream( JsonValueLifeAbstractBase< t_TyChar > & _jvl ) const
	{
		RNfa().ToJSONStream( _jvl, *this );
	}

	// Create NFA's - delegate to NFA container:
	void CreateEmptyNFA()
	{
		RNfa().CreateRangeNFA( *this, _TyRange( 0, 0 ) );
	}
	void CreateLiteralNFA( t_TyChar const & _rc )
	{
		RNfa().CreateRangeNFA( *this, _TyRange( _rc, _rc ) );
	}
	void CreateStringNFA( t_TyChar const * _pc )
	{
		RNfa().CreateStringNFA( *this, (_TyUnsignedChar*)_pc );
	}
	void CreateRangeNFA( _TyRange const & _rr )
	{
		RNfa().CreateRangeNFA( *this, _rr );
	}
	void CreateFollowsNFA( _TyBase & _rcb )
	{
		RNfa().CreateFollowsNFA( *this, static_cast< _TyThis & >( _rcb ) );
	}
	void CreateLookaheadNFA( _TyBase & _rcb )
	{
		// We need to save the intermediate accept state ( otherwise we won't know
		//	where it is ).
		RNfa().CreateLookaheadNFA( *this, static_cast< _TyThis & >( _rcb ) );
	}
	void CreateTriggerNFA()
	{
		// We need to save the intermediate accept state ( otherwise we won't know
		//	where it is ).
		RNfa().CreateTriggerNFA( *this );
	}
	void CreateOrNFA( _TyBase & _rcb )
	{
		RNfa().CreateOrNFA( *this, static_cast< _TyThis & >( _rcb ) );
	}
	void CreateExcludesNFA( _TyBase & _rcb )
	{
		RNfa().CreateExcludesNFA( *this, static_cast< _TyThis & >( _rcb ) );
	}
	void CreateCompletesNFA( _TyBase & _rcb )
	{
		RNfa().CreateCompletesNFA( *this, static_cast< _TyThis & >( _rcb ) );
	}
	void CreateZeroOrMoreNFA()
	{
		RNfa().CreateZeroOrMoreNFA( *this );
	}
	void CreateUnsatisfiableNFA( size_t _nUnsatisfiable )
	{
		RNfa().CreateUnsatisfiableNFA( *this, _nUnsatisfiable );
	}

	// We will be added more alternative rules to this NFA - set it up for that:
	void StartAddRules()
	{
		RNfa()._InitSetAcceptStates( *this );
		RNfa().CreateAltRuleNFA( *this ); 
	}

	void AddAlternativeNFA( _TyBase & _rcb )
	{
		// We add the accepting state and potential action to our container:
		_TyThis &	rc( static_cast< _TyThis & >( _rcb ) );
		RNfa()._AddAccept( rc );
		RNfa().AddAlternativeNFA( *this, rc );
	}

	void _CreateAcceptingNodeSet()
	{
		m_pssAccept.template emplace< _TyState, const t_TyAllocator & >( RNfa().NStates(), RNfa().get_allocator() );
		m_pssAccept->clear();

		// Copy the accepting states to the bit vector:
		typename _TyNfa::_TySetAcceptIT itEnd = RNfa().m_pSetAcceptStates->end();
		for ( typename _TyNfa::_TySetAcceptIT it = RNfa().m_pSetAcceptStates->begin();
					it != itEnd;
					++it )
		{
			m_pssAccept->setbit( it->first );
		}					
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_NFA_H

// Unused code - on the way out...

#if 0
	struct _select_not_state : public unary_function< _TyGraphLink const *, bool >
	{
		_TySetStates const & m_rss;

		_select_not_state( _TySetStates const & _rss )
			: m_rss( _rss )
		{
		}
		_select_not_state( _select_not_state const & _r )
			: m_rss( _r.m_rss )
		{
		}

		bool	operator()( _TyGraphLink const * _pgl )
		{
			return !m_rss.isbitset( _pgl->PGNChild()->RElConst() );
		}
	};

	struct _select_minus_closure : public unary_function< _TyGraphLink const *, bool >
	{
		bool	operator()( _TyGraphLink const * _pgl )
		{
			return _pgl->RElConst().contains( 0 ) || _pgl->RElConst().contains( ms_kreTrigger );
		}			
	};

	void	MinusClosure(	const _TyGraphNode * _pgnStart,
											_TySetStates & _rsetResult )
	{
		assert( _rsetResult.empty() );

		typedef _select_minus_closure	_TyMinusSelect;
		typedef typename _TyGraph::_TyGraphTraits:: 
			template _get_link_select_iter< _TyMinusSelect >::_TyGraphFwdIterSelectConst	_TyMinusSelectIter;

		// We cache the selection iterator to keep from allocating/deallocating all the time:
		_TyMinusSelectIter	selitMinus(	_pgnStart, 0, true, false, 
																		get_allocator(), _TyMinusSelect() );

		_rsetResult.setbit( _pgnStart->RElConst() );	// add initial state.

		while( !(++selitMinus).FAtEnd() )
		{
			// If we are currently at a node then add it:
			if ( !selitMinus.PGLCur() )
			{
				const _TyGraphNode * _pgnCur = selitMinus.PGNCur();
				_rsetResult.setbit( _pgnCur->RElConst() );
			}
		}
	}

	void	CreateExcludesNFA( _TyNfaCtxt & _rctxt1_Result, _TyNfaCtxt & _rctxt2 )
	{
		// The minus NFA is like the or NFA except:
		//	1) No new start state is added - the start state of _rctxt2 is the start state of the result.
		//		a) An epsilon transition is added to the start state of _rctxt1_Result.
		//	1) the accepting states are not linked together.
		//		a) the accepting state of _rctxt1_Result is the accepting state of the result.
		//		b) the accepting state of _rctxt2 is specifically a non-accepting state ( pattern exclusion ).
		//	2) An epsilon transition is added from all states of _rctxt2 to the start state of _rctxt1_Result,
		//		except those states in the reverse closure from the accepting state of _rctxt2 on all epsilon or
		//		trigger transitions.

		// Record the reverse epsilon and trigger closure for the exclusion accepting state:
		_TySetStates	ssReverse( NStates(), get_allocator() );
		ssReverse.clear();
		MinusClosure( _rctxt2.m_pgnAccept, ssReverse );

		// Record the forward epsilon closure from the exclusion start state:
		_TySetStates	ssForward( NStates(), get_allocator() );
		ssForward.clear();
		Closure( _rctxt2.m_pgnStart, ssForward, false );

		// If the closure of the start state of the exclusion pattern intersects the 
		//	reverse closure then we have a bogus minus pattern.
		if ( ssReverse.FIntersects( ssForward ) )
		{
			assert( 0 );
			//throw _bad_minus_pattern_exception();
		}

		// Now do a forward iteration from the start state of the exclusion pattern 
		//	recording those nodes not in the reverse closure:
		ssForward.clear();

		typedef _select_not_state	_TyForwardSelect;
		typedef typename _TyGraph::_TyGraphTraits:: 
			template _get_link_select_iter< _TyForwardSelect >::_TyGraphFwdIterSelectConst	_TyForwardSelectIter;

		// We cache the selection iterator to keep from allocating/deallocating all the time:
		_TyForwardSelectIter	selitForward(	_rctxt2.m_pgnStart, 0, true, true, 
																				get_allocator(), _TyForwardSelect( ssReverse ) );

		ssForward.setbit( _rctxt2.m_pgnStart->RElConst() );	// add initial state.

		while( !(++selitForward).FAtEnd() )
		{
			// If we are currently at a node then add it:
			if ( !selitForward.PGLCur() )
			{
				const _TyGraphNode * _pgnCur = selitForward.PGNCur();
				ssForward.setbit( _pgnCur->RElConst() );
			}
		}

		// Then for each node found create an epsilon transition to the start state
		//	of _rctxt1_Result:
		_TyState nState = -1;
		while( ssForward.size() != ( nState = (_TyState)ssForward.getclearfirstset( stState ) ) )
		{
			_NewTransition( PGNGetNode( nState ), _TyRange( 0, 0 ), _rctxt1_Result.m_pgnStart );
		}

		_rctxt1_Result.m_pgnStart = _rctxt2.m_pgnStart; 
		_rctxt2.m_pgnStart = 0;
	}
#endif //0

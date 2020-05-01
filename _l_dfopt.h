#ifndef __L_DFOPT_H
#define __L_DFOPT_H

// _l_dfopt.h

#include "_garcoll.h"

__REGEXP_BEGIN_NAMESPACE

template < class t_TySwapSS >
struct _partition_el
{
private:
	typedef _partition_el< t_TySwapSS >	_TyThis;
public:

	ptrdiff_t		first;
	t_TySwapSS	second;

	_partition_el(ptrdiff_t _first, t_TySwapSS const & _rss )
		: first( _first ),
			second( _rss )
	{
	}
	_partition_el( _TyThis const & _r )
		: first( _r.first ),
			second( _r.second )
	{
	}

	bool operator < ( _TyThis const & _r ) const
	{
		// Produce an ordering so that states are unique.
		if ( ( first < 0 ) && ( _r.first < 0 ) )
		{
			return second < _r.second;
		}
		return ( first < 0 ) ? false : ( _r.first < 0 ) ? true : ( first < _r.first );
	}
};

template < class t_TyPartitionClass >
struct _compare_partition_classes
#ifdef __LEXANG_USE_STLPORT
	: public binary_function< t_TyPartitionClass *, t_TyPartitionClass *, bool >
#endif //__LEXANG_USE_STLPORT
{
	size_t	m_stAlphabet;	// The number of characters in the alphabet.

	_compare_partition_classes( size_t _stAlphabet )
		: m_stAlphabet( _stAlphabet )
	{
	}

	bool	operator ()(	t_TyPartitionClass * const & _rpl, 
											t_TyPartitionClass * const & _rpr ) const _BIEN_NOTHROW
	{
		return (*_rpl).lessthan( (*_rpr), m_stAlphabet );
	}
};

template < class t_TyState, class t_TyPartitionEl, class t_TySetStates >
struct _partition_class
{
private:
	typedef _partition_class< t_TyState, t_TyPartitionEl, t_TySetStates >	_TyThis;
public:

	typedef t_TyState _TyState;
	typedef typename t_TySetStates::_TyAllocator	_TyAllocator;

	_TyState						m_nSingleState;
	t_TySetStates				m_ssMembers;	// The bit vector for members of this partition class.

	t_TyPartitionEl **	m_pppelTransitions;

	_partition_class(_TyState _nStates, _TyAllocator const & _rA )
		: m_ssMembers(_nStates, _rA ),
			m_nSingleState( -1 )
	{
		m_ssMembers.clear();
	}

	t_TySetStates & RSetStates()
	{
		return m_ssMembers;
	}

	void	clear()
	{
		clearNotSS();
		m_ssMembers.clear();
	}

	void	clearNotSS()
	{
		m_nSingleState = -1;
	}

	bool	empty()
	{
		return ( -1 == m_nSingleState ) && m_ssMembers.empty();
	}

	_TyState	NSingleState()
	{
		return m_nSingleState;
	}

	void	AddState( _TyState _iState )
	{
		m_nSingleState = ( -1 == m_nSingleState ) ? _iState : -2;
		m_ssMembers.setbit( _iState );
	}

	bool	lessthan(	_TyThis const & _r,
									size_t _stAlphabet ) const _BIEN_NOTHROW
	{
		// Move through the transition arrays until we find a difference:
		t_TyPartitionEl ** pppeCurThis = m_pppelTransitions;
		t_TyPartitionEl ** pppeEndThis = m_pppelTransitions + _stAlphabet;
		t_TyPartitionEl ** pppeCurR = _r.m_pppelTransitions;
		for ( ; pppeCurThis != pppeEndThis; pppeCurThis++, pppeCurR++ )
		{
			if ( *pppeCurThis < *pppeCurR )
			{
				return true;
			}
			if ( *pppeCurR < *pppeCurThis )
			{
				return false;
			}
		}
		return false;
	}

	t_TyPartitionEl ** begin() const
	{
		return m_pppelTransitions;
	}
};

template < class t_TyDfa, bool t_fPartDeadImmed >
struct _optimize_dfa
	: public _alloc_base< void *,	typename t_TyDfa::_TyAllocator >,
		public _alloc_base< _partition_class< typename t_TyDfa::_TyState, _partition_el< _swap_object< typename t_TyDfa::_TySetStates > >,
																					typename t_TyDfa::_TySetStates >,
												typename t_TyDfa::_TyAllocator >
{
private:
	typedef _optimize_dfa< t_TyDfa, t_fPartDeadImmed >	_TyThis;
	typedef _alloc_base< void *, typename t_TyDfa::_TyAllocator >	_TyAllocVPBase;
	typedef _alloc_base< _partition_class< typename t_TyDfa::_TyState, _partition_el< _swap_object< typename t_TyDfa::_TySetStates > >,
																					typename t_TyDfa::_TySetStates >,
												typename t_TyDfa::_TyAllocator >				_TyAllocPartClass;
protected:

	typedef	t_TyDfa	_TyDfa;
	typedef	typename _TyDfa::_TyContext	_TyDfaCtxt;
	typedef typename _TyDfa::_TyAllocator	_TyAllocator;
	typedef typename t_TyDfa::_TySetStates::size_type	size_type;

	_TyDfa &			m_rDfa;
	_TyDfaCtxt &	m_rDfaCtxt;
	size_type			m_stDfaStatesOrig;	// The number of DFA states before optimization.		

	typedef typename _TyDfa::_TyState				_TyState;
	typedef typename _TyDfa::_TySetStates		_TySetStates;
	typedef typename _TyDfa::_TySSCache			_TySSCache;
	typedef typename _TyDfa::_TyNodeLookup		_TyNodeLookup;
	typedef typename _TyDfa::_TyGraph				_TyGraph;
	typedef typename _TyDfa::_TyGraphNode		_TyGraphNode;
	typedef typename _TyGraph::_TyGraphLink	_TyGraphLink;
	typedef typename _TyGraph::_TyGraphLinkBaseBase	_TyGraphLinkBaseBase;
	typedef typename _TyDfaCtxt::_TyPartAcceptStates	_TyPartAcceptStates;

	// We need a set of states for the partition.
	// In order to minimize state creation ( i.e. only maintain actual number of
	//	states in the partition at any point in time ) we need reference counted
	//	objects. This allows insertion of the same object into the map - allows
	//	the set number -> state set mapping to remain correct for at least half
	//	of the elements of a split set.
	typedef _swap_object< _TySetStates > _TySwapSS;
	typedef _partition_el< _TySwapSS > _TyPartitionEl;
	typedef _gco< _TyPartitionEl, _TyAllocator, true, true > _TyGcoPE;
	typedef _gcr< _TyPartitionEl, _TyGcoPE > _TyGcrPE;
	typedef _gcp< _TyPartitionEl, _TyGcoPE > _TyGcpPE;
	typedef less< _TyGcrPE > _TyCompPE;
  typedef typename _Alloc_traits< typename set< _TyGcrPE, _TyCompPE >::value_type, _TyAllocator >::allocator_type _TyPartitionAlloc;
	typedef set< _TyGcrPE, _TyCompPE, _TyPartitionAlloc > _TyPartition;

	_TyPartition	m_partition;
	_TyGcpPE			m_gcppeSingleton;

	// We also need a mapping from set number to the partition element containing that set -
	//	since the number of states is fixed just need a simple array:
	typedef _TyPartitionEl *	_TyStateMapEl;
	_TyStateMapEl *						m_rgsmeMap;

	// As we compute the partition we need to accumulate the new sets ( relative to the current
	//	sets ). The number of unique sets in the partition could be as high as the number of states 
	//	the original DFA.
	typedef _partition_class< _TyState, _TyPartitionEl, _TySetStates >	_TyPartitionClass;

	_TyPartitionClass **	m_cachePartClasses;
	size_t								m_stUsedClassCache;
	size_t								m_stSizeClassCache;

	// This is the set of current classes.
	typedef _compare_partition_classes< _TyPartitionClass > _TyCompPartClases;
  typedef typename _Alloc_traits< typename set< _TyPartitionClass *, _TyCompPartClases >::value_type, _TyAllocator >::allocator_type _TySetPartClassesAlloc;
  typedef set< _TyPartitionClass *, _TyCompPartClases, _TySetPartClassesAlloc >	_TySetPartClasses;

	_TySetPartClasses	m_setPartClasses;

	// Lookup the representative for a node.
	_sdpn< _TyGraphNode *, _TyAllocator >	m_rgLookupRep;

public:
	
	_optimize_dfa( _TyDfa & _rDfa, _TyDfaCtxt & _rDfaCtxt )
		:	_TyAllocVPBase( _rDfa.get_allocator() ),
			_TyAllocPartClass( _rDfa.get_allocator() ),
			m_rDfa( _rDfa ),
			m_rDfaCtxt( _rDfaCtxt ),
			m_stDfaStatesOrig( m_rDfa.NStates() ),
			m_partition( _TyCompPE(), _rDfa.get_allocator() ),
			m_rgsmeMap( 0 ),
			m_cachePartClasses( 0 ),
			m_stUsedClassCache( 0 ),
			m_stSizeClassCache( 0 ),
			m_setPartClasses( _TyCompPartClases( _rDfa.AlphabetSize() ), 
												_rDfa.get_allocator() ),
			m_rgLookupRep( m_rDfa.get_allocator() )
	{
		_TyPartitionEl	peSingleton( INT_MAX, _TySetStates( 0, m_rDfa.get_allocator() ) );
		m_gcppeSingleton.template Create1< _TyPartitionEl const & >( peSingleton, m_rDfa.get_allocator() );

		_sdp< void *, _TyAllocator >	pvAllocMap( _TyAllocVPBase::get_allocator() );
		_TyAllocVPBase::allocate_n( pvAllocMap.PtrRef(), m_stDfaStatesOrig );
		void **	pvPartClassCache;
		_TyAllocVPBase::allocate_n( pvPartClassCache, m_stDfaStatesOrig + 1 );
		m_cachePartClasses = reinterpret_cast< _TyPartitionClass ** >( pvPartClassCache );
		m_rgsmeMap = reinterpret_cast< _TyStateMapEl * >( pvAllocMap.transfer() );
	}

	~_optimize_dfa()
	{
		if ( m_rgsmeMap )
		{
			_TyAllocVPBase::deallocate_n( reinterpret_cast< void ** >( m_rgsmeMap ), 
																		m_stDfaStatesOrig );
		}
		_DeallocPartClassCache();
	}

	_TyPartitionClass **
	_GetNewPartClass()
	{
		// Then need to allocate a new one:
		assert( m_stSizeClassCache < m_stDfaStatesOrig + 1 );	// Need room.
		assert( m_stSizeClassCache == m_stUsedClassCache );
		_sdp< _TyPartitionClass, _TyAllocator >	ppcNew( _TyAllocPartClass::get_allocator() );
		ppcNew.PtrRef() = _TyAllocPartClass::allocate_type( );
		new ( ppcNew ) _TyPartitionClass( m_stDfaStatesOrig, _TyAllocPartClass::get_allocator() );
		_dtorp< _TyPartitionClass >	dtorPC( ppcNew );	// throw-safety.
		void **	pvTransitions;
		_TyAllocVPBase::allocate_n( pvTransitions, m_rDfa.AlphabetSize() );
		ppcNew->m_pppelTransitions = reinterpret_cast< _TyPartitionEl ** >( pvTransitions );
		m_cachePartClasses[ m_stSizeClassCache ] = ppcNew.transfer();
		dtorPC.Reset();
		m_stSizeClassCache++;
		return &m_cachePartClasses[ m_stUsedClassCache++ ];
	}

	_TyPartitionClass **
	_GetPartClass()
	{
		_TyPartitionClass ** pppc;
		if ( m_stUsedClassCache < m_stSizeClassCache )
		{
			pppc = &m_cachePartClasses[ m_stUsedClassCache++ ];
		}
		else
		{
			pppc = _GetNewPartClass();
		}
		assert( (*pppc)->empty() );
		return pppc;
	}

	void
	_ReleasePartClass( _TyPartitionClass ** _pppc )
	{
		// Should always be releasing the last one ( otherwise we need
		//	a free bitvec ).
		assert( (*_pppc)->empty() );	// Should be empty.
		assert( _pppc == &m_cachePartClasses[ m_stUsedClassCache-1 ] );
		--m_stUsedClassCache;
	}

	void
	_DeallocPartClassCache()
	{
		if ( m_cachePartClasses )
		{
			// Then need to deallocate all of the caches:
			_TyPartitionClass ** pppcCur = m_cachePartClasses;
			_TyPartitionClass ** pppcEnd = m_cachePartClasses + m_stSizeClassCache;
			for ( ; pppcCur != pppcEnd; ++pppcCur )
			{
				_TyAllocVPBase::deallocate_n( (void**)((*pppcCur)->m_pppelTransitions), 
																			m_rDfa.AlphabetSize() );
				(*pppcCur)->~_TyPartitionClass();
				_TyAllocPartClass::deallocate_type( *pppcCur );
			}

			_TyAllocVPBase::deallocate_n( reinterpret_cast< void ** >( m_cachePartClasses ), 
																		m_stDfaStatesOrig + 1 );
			m_cachePartClasses = 0;
		}
	}

	// Clear the current set of partition classes - this also frees the cache:
	void
	_ClearPartClassSet()
	{
		m_setPartClasses.clear();
		m_stUsedClassCache = 0;
	}

	void
	_UpdateStateMap( _TyPartitionEl * _pel, _TySetStates & _rss )
	{
		if ( _pel->first >= 0 )
		{
			m_rgsmeMap[ _pel->first ] = _pel;
			_rss.clearbit( (typename _TySetStates::size_type)_pel->first );
		}	
		else
		{
      typename _TySetStates::size_type	stNextUpdate;
      typename _TySetStates::size_type	stFirstUpdate;
			int	iUpdates = 0;
			for ( stFirstUpdate = stNextUpdate = _rss.getclearfirstset();
						_rss.size() != stNextUpdate;
						stNextUpdate = _rss.getclearfirstset( stNextUpdate ) )
			{
				m_rgsmeMap[ stNextUpdate ] = _pel;
				iUpdates++;
			}
			assert( iUpdates );		
			if (1 == iUpdates)
				_pel->first = (_TyState)stFirstUpdate;
			else
				_pel->first = -1;
			//_pel->first = ( 1 == iUpdates ) ? stFirstUpdate : size_t( -1 );
		}
		assert( _rss.empty() );
	}

	void
	_InsertNewSS( _TySetStates & _rssInsert,
								_TyState _iSingleStateHint = -1 )
	{
		_TyPartitionEl	peInsert( _iSingleStateHint, _TySetStates( _rssInsert ) );

		_TyGcpPE	gcpPeInsert;
		gcpPeInsert.template Create1< _TyPartitionEl const & >( peInsert, m_rDfa.get_allocator() );

		// Now set the state->state set map pointers:
		_UpdateStateMap( gcpPeInsert, _rssInsert );

#ifndef NDEBUG
		pair< typename _TyPartition::iterator, bool >	pibDebug = 
#endif //!NDEBUG
		m_partition.insert( gcpPeInsert );
#ifndef NDEBUG
		assert( pibDebug.second );
#endif //!NDEBUG
	}

	// Return true if created a new DFA.
	// Return false if original DFA was optimal.
	bool	optimize( )
	{
		if ( !m_rDfa.m_fHasDeadState )
		{
			assert( 0 );	// Need a dead state to optimize.
			return false;
		}

		if (	m_rDfa.AlphabetSize() <= 1 )
		{
			m_rDfaCtxt.RemoveDeadState( );
			m_rDfaCtxt.CompressNodeLookup( 1 );
			return false;	// Need at least two significant tokens to optimize.
		}

		// First compress any trigger states that can be:
		m_rDfaCtxt.CompressTriggerAcceptPartitions();

		// Create the initial partition of accepting/non-accepting:
		_TySetStates	ssUtil( m_rDfa.NStates(), m_rDfa.get_allocator() );
		m_rDfaCtxt.GetAcceptingNodeSet( ssUtil );
		ssUtil.invert();
		if ( t_fPartDeadImmed || !m_rDfa.m_nodeLookup[ 0 ]->FChildren() )
		{
			ssUtil.clearbit( 0 ); // clear dead state.			
			m_rgsmeMap[ 0 ] = 0;
		}
		_InsertNewSS( ssUtil );
		assert( ssUtil.empty() );

		// Now if the dead state has any out transitions then we need to add it:
		if ( t_fPartDeadImmed && m_rDfa.m_nodeLookup[ 0 ]->FChildren() )
		{
			ssUtil.setbit( 0 );
			_InsertNewSS( ssUtil, 0 );
			assert( ssUtil.empty() );
		}

		// Added the non-accepting - now add the accepting state partition to the partition:
		typename _TyPartAcceptStates::iterator	itDfaPASEnd = m_rDfaCtxt.m_partAccept.end();
		for ( typename _TyPartAcceptStates::iterator	it = m_rDfaCtxt.m_partAccept.begin();
					it != itDfaPASEnd; ++it )
		{
			ssUtil = it->first;
			assert( !ssUtil.empty() );
			_InsertNewSS( ssUtil );
		}
		assert( ssUtil.empty() );

		// Apply partitioning algorithm:
		_Partition(	ssUtil );

		typename _TyPartition::iterator itUpper;
		itUpper = m_partition.upper_bound( m_gcppeSingleton );

		// No longer need the partition class cache:
		_DeallocPartClassCache();

		// Check the result to see if any un-optimized states:
		if ( m_partition.end() != itUpper )
		{
			// Then we have states to compress:
			_CompressStates( itUpper, ssUtil );
			return true;
		}
		else
		{
			// no optimization to be done:
			m_rDfaCtxt.RemoveDeadState( );
			m_rDfaCtxt.CompressNodeLookup( 1 );
			return false;
		}
	}

protected:

	void	_Partition( _TySetStates & _rssUtil )
	{
		assert( _rssUtil.empty() );

		typename _TyPartition::iterator	itCur = m_partition.upper_bound( m_gcppeSingleton );
		if ( itCur == m_partition.end() )
			return;	// already optmimal ( but not a very complex DFA ).

		do
		{
			// We don't follow the Aho/Sethi/Ullman algorithm completely here - seems to me that
			//	there is no reason to produce a new partition ( while maintaining the current partition ).
			// It seems to me that modification of the current partition with split partitions 
			//	should allow the algorithm to procede unhindered ( and faster - since the criteria
			//	of partitioning are more granular ).
			// It might be nice to skip the just split classes the second time around. This is because
			//	the criteria upon which the splitting depends ( i.e. the other classes ) will get
			//	more granular - may avoid passing through some sets that are not YET splittable.
			const _TyPartitionEl * ppelCur = *itCur;
      assert( -1 == ppelCur->first ); // should not encounter a singleton.
			_rssUtil = ppelCur->second;	// Copy the set of states - we will modify below.

			_TyPartitionClass **	pppc = _GetPartClass();	// Get a partition class from the cache.

			_TyState	iStateTest;
			for ( iStateTest = (_TyState)_rssUtil.getclearfirstset();
						_rssUtil.size() != iStateTest;
						iStateTest = _rssUtil.getclearfirstset( iStateTest ) )
			{
				// Since we know we have a transition on every state out of every node
				//	we can just iterate the links of the node for this state:
				// We also know that the links are stored in order - with the last alphabet
				//	element first ( though that doesn't matter - just as long as they are in the
				//	same order at each node ).
				_TyGraphNode * pgn = m_rDfa.PGNGetNode( iStateTest );
				typename _TyGraph::_TyLinkPosIterConst	lpi( pgn->PPGLChildHead() );

				_TyPartitionEl ** pppelPartClass = (*pppc)->begin();

				while( !lpi.FIsLast() )
				{
					*pppelPartClass++ = m_rgsmeMap[ lpi.PGNChild()->REl() ];
					lpi.NextChild();
				}

				// Now attempt to insert this new transition container into the set of unique
				//	transition sets of the current group of the partition:
				pair < typename _TySetPartClasses::iterator, bool >	pib = m_setPartClasses.insert( *pppc );
				if ( pib.second )
				{
					// Then a new partition of this group - get a new working cache:
					pppc = _GetPartClass();
				}
				else
				{
					assert( (*pppc)->empty() ); // not a new partition class.
				}

				// Indicate that this state is part of the partition class:
				(*pib.first)->AddState( iStateTest );
			}

			_ReleasePartClass( pppc );

			// See if we need to split the current group:
			if ( m_setPartClasses.size() > 1 )
			{
				_Split( itCur );
				// Reset the iterator upon the introduction of new groups.
				itCur = m_partition.upper_bound( m_gcppeSingleton );	
			}
			else
			{
				++itCur;	// Move to the next group of the partition to see if we can split that.
				assert( m_stUsedClassCache == 1 );
				m_cachePartClasses[ 0 ]->clear();
			}

			_ClearPartClassSet();
		}
		while( m_partition.end() != itCur );
	}

	void	_Split( typename _TyPartition::iterator	const & ritPartCur )
	{
		// Then the current group has been split:
		// The bitvectors in each partition class indicate the new set of groups.
		// We will remove the old group from the partition and then add each of the
		//	partition classes.
		_TyGcrPE	grInSet( *ritPartCur );
		m_partition.erase( ritPartCur );

		typename _TySetPartClasses::iterator itSpcCur = m_setPartClasses.begin();
		_TyPartitionClass *	ppc = *itSpcCur;
		grInSet->second.RObject() = ppc->RSetStates();	// Copy.
		grInSet->first = ppc->NSingleState();
		_UpdateStateMap( grInSet, ppc->RSetStates() );
		ppc->clearNotSS();
		pair< typename _TyPartition::iterator, bool >	pib = m_partition.insert( grInSet );
		assert( pib.second );
		assert( ppc->empty() );

		// Now for each of the remaining partition classes:
		for ( ; ++itSpcCur != m_setPartClasses.end(); )
		{
			ppc = *itSpcCur;
			_InsertNewSS( ppc->RSetStates(), ppc->NSingleState() );
			ppc->clearNotSS();
			assert( ppc->empty() );
		}
	}

	void	_CheckMoveHead( _TyGraphLink * _pgl )
	{
		// See if we are entering a representative - if not move to the representative:
		_TyState	iTransState;
		if ( !m_rgLookupRep[ (size_type)( iTransState = _pgl->PGNChild()->RElConst() ) ] )
		{
			// Then determine the representative for this node:
			if ( m_rgsmeMap[ iTransState ]->first >= 0 )
			{
				// singleton:
				m_rgLookupRep[ (size_type)iTransState ] = _pgl->PGNChild();
			}
			else
			{
				m_rgLookupRep[ (size_type)iTransState ] = m_rDfa.PGNGetNode( 
					m_rgsmeMap[ iTransState ]->second.RObject().getfirstset() );
			}
		}

		if ( m_rgLookupRep[ (size_type)iTransState ] != _pgl->PGNChild() )
		{
			// Then need to move the link:
			_pgl->RemoveParent();
			_pgl->InsertParent( m_rgLookupRep[ (size_type)iTransState ]->PPGLParentHead() );
			_pgl->SetChildNode( m_rgLookupRep[(size_type)iTransState ] );
		}
	}

	void	_CompressStates(	typename _TyPartition::iterator & _rcitUpper,
													_TySetStates & _rssUtil )
	{
		// Compress the graph in place.

		m_rDfaCtxt.RemoveDeadState( );

		// Now start compressing partition groups.
		// We will always choose the first member of each partition group as the
		//	representative for that group.
		// For each non-singleton representative we need to record a bitvector 
		//	of the existence of out transitions on each alphabet. This allows us to look
		//	up whether a link should be removed or merely have its endpoint moved -
		//	as we delete the extraneous states and transitions.
		// As we record the bit-vector we can move the links to the representative of each group.
		// (Pass 1) We make a pass over the non-representatives in the non-singleton
		//	parition groups. For each out transition we check whether the representative
		//	has an out transition on the given alphabet - if so then the out transition
		//	must be to a member of the same group as that of its non-representative partner, so:
		//		a) We can delete the link from the non-representative.
		//	If not then we can:
		//		a) Move the link's endpoints to the appropriate representatives of partition groups.
		//		b) Record that an out transition exists in the current parition group's representative bv.
		// At the end of this process the non-representatives will have no out transitions.
		// The only in transitions that need to be moved are those from singleton partition groups.
		// ( Also they will be the only ones that are left. )
		_TySetStates	ssOutOnAlpha( (_TyState)m_rDfa.AlphabetSize(), m_rDfa.get_allocator() );

		// Create a lookup which we will lazily fill with the representative state's graph node
		//	as we process:
		m_rgLookupRep.allocate( m_stDfaStatesOrig );
		memset( m_rgLookupRep.begin(), 0, m_stDfaStatesOrig * sizeof( _TyGraphNode * ) );

		typename _TyPartition::iterator itCur = _rcitUpper;

    typename _TySetStates::size_type	stNonReps = 1;	// Accumulate the number of non-reps we will be removing.

		typename _TyGraph::_TyLinkPosIterNonConst	lpi;

		do
		{
			const _TyPartitionEl * ppelCur = *itCur;
      assert( -1 == ppelCur->first ); // We shouldn't see any singletons.

			_rssUtil = ppelCur->second;	// Copy the set of states - we will modify below.

			_TyState iStateRep;
			_TyGraphNode *	pgnRep = m_rDfa.PGNGetNode( iStateRep = _rssUtil.getclearfirstset() );
			
			ssOutOnAlpha.clear();

			lpi = pgnRep->PPGLChildHead();
			while( !lpi.FIsLast() )
			{
				ssOutOnAlpha.setbit( *lpi );
				_CheckMoveHead( lpi.PGLCur() );
				lpi.NextChild();
			}

			// Now need to process the non-reps for this rep:
			_TyState	iStateNonRep;
			for ( iStateNonRep = _rssUtil.getclearfirstset( iStateRep );
						_rssUtil.size() != iStateNonRep;
						iStateNonRep = _rssUtil.getclearfirstset( iStateNonRep ) )
			{
				stNonReps++;

				_TyGraphNode *	pgnNonRep = m_rDfa.PGNGetNode( iStateNonRep );

				for ( lpi = pgnNonRep->PPGLChildHead();
							!lpi.FIsLast(); )
				{
					typename _TyDfa::_TyAlphaIndex	ai;
					_TyGraphLink *	pgl = lpi.PGLCur();
					if ( ssOutOnAlpha.isbitset( ai = pgl->RElConst() ) )
					{
						// Then a redundant link - remove:
						m_rDfa._RemoveLink( pgl );
					}
					else
					{
						// Then this link needs to have its head moved:
						pgl->RemoveChild();
						pgl->InsertChild( pgnRep->PPGLChildHead() );
						pgl->SetParentNode( pgnRep );
						ssOutOnAlpha.setbit( ai );

						// Check to see if the tail needs to be moved:
						_CheckMoveHead( pgl );
					}
				}
			}
		}
		while( m_partition.end() != ++itCur );

		// (Pass 2): Move all parents of non-kreps to rep:
		itCur = _rcitUpper;

		do
		{
			const _TyPartitionEl * ppelCur = *itCur;
			_rssUtil = ppelCur->second;	// Copy the set of states - we will modify below.

			_TyState iStateRep;
			_TyGraphNode *	pgnRep = m_rDfa.PGNGetNode( iStateRep = _rssUtil.getclearfirstset() );
			
			_TyState	iStateNonRep;
			for ( iStateNonRep = _rssUtil.getclearfirstset( iStateRep );
						_rssUtil.size() != iStateNonRep;
						iStateNonRep = _rssUtil.getclearfirstset( iStateNonRep ) )
			{
				_TyGraphNode *	pgnNonRep = m_rDfa.PGNGetNode( iStateNonRep );

				for ( lpi = pgnNonRep->PPGLParentHead();
							!lpi.FIsLast(); )
				{
					// This should from a representative ( we aren't checking but it should be a singleton ).
					_TyGraphLink *	pgl = lpi.PGLCur();
					assert( !( m_rgLookupRep[ (size_t)pgl->PGNParent()->RElConst() ] ) ||
									( m_rgLookupRep[(size_t)pgl->PGNParent()->RElConst() ] == pgl->PGNParent() ) );
					pgl->RemoveParent();
					pgl->InsertParent( pgnRep->PPGLParentHead() );
					pgl->SetChildNode( pgnRep );
				}
			}
		}
		while( m_partition.end() != ++itCur );

		// (Pass 3): Delete all non-representatives.
		// We have a bunch of orphans in the non-reps - destruct and deallocate them.
		itCur = _rcitUpper;

		do
		{
			const _TyPartitionEl * ppelCur = *itCur;
			_rssUtil = ppelCur->second;	// Copy the set of states - we will modify below.

			_TyState	iStateNonRep = _rssUtil.getclearfirstset();	// skip the representative state.
			while ( _rssUtil.size() != ( iStateNonRep = _rssUtil.getclearfirstset( iStateNonRep ) ) )
			{
				_TyGraphNode *	pgnNonRep = m_rDfa.PGNGetNode( iStateNonRep );
				assert( !pgnNonRep->FParents() );
				assert( !pgnNonRep->FChildren() );
        m_rDfa.m_gDfa.destroy_node( pgnNonRep );
				m_rDfa.m_nodeLookup[ iStateNonRep ] = 0;
			}
		}
		while( m_partition.end() != ++itCur );

		m_rDfaCtxt.CompressNodeLookup( stNonReps );
	
		// (Pass 5): Might be nice to sort all out transitions.
		m_rDfa.SortTransitions();
	}

	void	DumpPartition( ostream & _ros )
	{
		_ros << "Partition: {\n";
		for ( typename _TyPartition::iterator pit = m_partition.begin();
					pit != m_partition.end();
					( ++pit == m_partition.end() ) || ( _ros << ",\n" ) )
		{
			m_rDfa.DumpStates( _ros, (*pit)->second );
		}
		_ros << "\n} End Parition\n";
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_DFOPT_H

#ifndef __L_DFA_H
#define __L_DFA_H

// _l_dfa.h

// This module declares DFA types.

__REGEXP_BEGIN_NAMESPACE

class alpha_index_overflow : public _t__Named_exception< __L_DEFAULT_ALLOCATOR >
{
	typedef _t__Named_exception< __L_DEFAULT_ALLOCATOR > _TyBase;
public:
	alpha_index_overflow( const string_type & __s ) : _TyBase(__s) {}
};

template < class t_TyChar, class t_TyAllocator >
class _dfa_context;

template < class t_TyGraphLink >
struct _sort_dfa_link 
	: public binary_function< const t_TyGraphLink *, const t_TyGraphLink *, bool >
{
	bool	operator()( const t_TyGraphLink * const & _rpglL, 
										const t_TyGraphLink * const & _rpglR ) const _STLP_NOTHROW
	{
		return _rpglL->RElConst() < _rpglR->RElConst();
	}
};

template < class t_TyChar, class t_TyAllocator = allocator< t_TyChar > >
class _dfa : public _fa_alloc_base< t_TyChar, t_TyAllocator >
{
private:
	typedef _fa_alloc_base< t_TyChar, t_TyAllocator >		_TyBase;
	typedef _dfa< t_TyChar, t_TyAllocator >							_TyThis;
public:

	// Friends:
	template < class t__TyNfa, class t__TyDfa >
	friend struct _create_dfa;
	template < class t_TyDfa, bool f_tPartDeadImmed >
	friend struct _optimize_dfa;
	friend class _dfa_context< t_TyChar, t_TyAllocator >;

	typedef _dfa_context< t_TyChar, t_TyAllocator >	_TyDfaCtxt;
	typedef _TyDfaCtxt															_TyContext;

	typedef int	_TyAlphaIndex;	// We use an index into a range lookup table as the link element.
															// If the index is negative then it is an index into the CCRIndex.

	typedef __DGRAPH_NAMESPACE dgraph<  _TyState, _TyAlphaIndex, 
                                      false, t_TyAllocator >		_TyGraph;
	typedef typename _TyGraph::_TyGraphNode														_TyGraphNode;
	typedef typename _TyGraph::_TyGraphLink														_TyGraphLink;
	_TyGraph			m_gDfa;

	bool					m_fHasDeadState;

	// Char range lookup stuff:
	_sdpn< _TyRange, t_TyAllocator > m_rgrngLookup;

	// Compressed character range map - we allow multiple entries that compare equal -
	//	we check before inserting a new entry that it is not present in the equal range:
	typedef multimap< _TyRange, _TyAlphaIndex, less< _TyRange >, t_TyAllocator >	_TySetCompCharRange;
	typedef deque< typename _TySetCompCharRange::iterator, t_TyAllocator >				_TyDequeCCRIndex;
	
	_sdpd< _TySetCompCharRange, t_TyAllocator >	m_pSetCompCharRange;
	_sdpd< _TyDequeCCRIndex, t_TyAllocator >		m_pCCRIndex;

	bool																				m_fHasLookaheads;	// any lookaheads in the rules ?
	bool																				m_fHasTriggers;		// triggers ?
	size_t																			m_nUnsatisfiableTransitions; // # of unsatisfiable.
	_TyActionIdent															m_iMaxActions;

	// Triggers - we need an extra copy since ambiguity may have occurred:
	typedef less< _TyActionIdent >	_TyCompareAI;
	typedef map< _TyActionIdent, _TyAcceptAction, _TyCompareAI, t_TyAllocator >	_TyMapTriggers;
	_sdpd< _TyMapTriggers, t_TyAllocator >	m_pMapTriggers;

	_dfa( t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyBase( _rAlloc ),
			m_gDfa( typename _TyGraph::_TyAllocatorSet( _rAlloc, _rAlloc, _rAlloc ) ),
			m_fHasDeadState( false ),
			m_rgrngLookup( _rAlloc ),
			m_pSetCompCharRange( _rAlloc ),
			m_pCCRIndex( _rAlloc ),
			m_fHasLookaheads( false ),
			m_fHasTriggers( false ),
			m_nUnsatisfiableTransitions( 0 ),
			m_pMapTriggers( _rAlloc )
	{
	}

#ifdef _DEBUG
  ~_dfa()
  {
  }
#endif _DEBUG

	size_type	AlphabetSize() const	{ return m_setAlphabet.size(); }

	_TyGraphNode *	PGNGetNode( _TyState _iState )
	{
		return static_cast< _TyGraphNode * >( m_nodeLookup[ _iState ] );
	}

	_TyRange	LookupRange( _TyAlphaIndex _ai )
	{
		if ( _ai < 0 )
		{
			// Then a compressed range:
			return (*m_pCCRIndex)[ -( _ai + 1 ) ]->first;
		}
		else
		{
			return m_rgrngLookup[ _ai ];
		}
	}

	void	SortTransitions()
	{
		_sdpn< _TyGraphLink *, _TyAllocator >	rgpgl( get_allocator() );
		size_t	stAlphabet;
		rgpgl.allocate( stAlphabet = AlphabetSize() );	// Can't have more transitions than this.

		typedef _sort_dfa_link< _TyGraphLink >	_TySortDfaLinks;

		bool	fCheckUnsat = ( m_fHasTriggers + m_nUnsatisfiableTransitions );
		_TyAlphaIndex	aiLimit = (_TyAlphaIndex)( stAlphabet - 1 -
			( m_fHasTriggers + m_nUnsatisfiableTransitions ) );

    _TyGraph::_TyLinkPosIterNonConst	lpi;
		_TyNodeLookup::iterator itnlEnd = m_nodeLookup.end();
		_TyNodeLookup::iterator itnlCur = m_nodeLookup.begin();
		for ( ; itnlCur != itnlEnd; ++itnlCur )
		{
			_TyGraphNode *	pgn = static_cast< _TyGraphNode * >( *itnlCur );
			lpi = pgn->PPGLChildHead();
			size_type	stTransitions = 0;
			_TyGraphLink ** pppglCur = rgpgl.begin();
			for ( ; !lpi.FIsLast(); lpi.NextChild() )
			{
				*pppglCur++ = lpi.PGLCur();
			}

			if ( pppglCur != rgpgl.begin() )
			{
				sort( rgpgl.begin(), pppglCur, _TySortDfaLinks() );

				// Now link back in:
				_TyGraphLink **	ppglChildHead = pgn->PPGLChildHead();
				*ppglChildHead = 0;	// Clear list.
				_TyGraphLink ** pppglEnd = pppglCur;
				_TyGraphLink ** pppglBeg = rgpgl.begin();
				_TyGraphLink ** pppglUnsat = pppglEnd;
				if ( fCheckUnsat )
				{
					while(	( pppglCur-1 != pppglBeg ) && 
									( (*(pppglCur-1))->RElConst() > aiLimit ) )
					{
						pppglUnsat = --pppglCur;
					}
				}

				(*--pppglCur)->InsertChild( ppglChildHead );
				for ( ; pppglCur-- != pppglBeg; )
				{
					(*pppglCur)->InsertChildAssume( ppglChildHead );
				}

				while( pppglUnsat != pppglEnd )
				{
					(*--pppglEnd)->InsertChild( ppglChildHead );
				}
			}
			else
			{
				assert( pgn->FParents() );
			}
		}
	}

	void	CompressTransitions()
	{
		assert( !m_pSetCompCharRange );	// This is only done once - should be done just before generation.
		if ( !m_pSetCompCharRange )
		{
			m_pSetCompCharRange._STLP_TEMPLATE construct2< _TySetCompCharRange::key_compare const &,
																			t_TyAllocator const & >
																		( _TySetCompCharRange::key_compare(),
																			get_allocator() );
			m_pCCRIndex._STLP_TEMPLATE construct1< t_TyAllocator const & >( get_allocator() );

			_CreateRangeLookup();	// Create if not already.

			// The transitions are sorted - so compression is relatively easy:
			for ( _TyState sCur = 0; sCur < NStates(); ++sCur )
			{
				_TyGraph::_TyLinkPosIterNonConst	lpi( PGNGetNode( sCur )->PPGLChildHead() );

				for ( ; !lpi.FIsLast(); )
				{
					_TyGraphLink *	pgl = lpi.PGLCur();
					_TyGraphNode *	pgn = pgl->PGNChild();
					_TyRange r = m_rgrngLookup[ pgl->RElConst() ];
					_TyRange *	prConsecutive;
					for(	lpi.NextChild(); 
								!lpi.FIsLast() && 
									r.isconsecutiveright( *( prConsecutive = &( m_rgrngLookup[ *lpi ] ) ) ); )
					{
						_TyGraphLink *	pglRemove = lpi.PGLCur();
						if ( pglRemove->PGNChild() == pgn )
						{
							r.second = prConsecutive->second;
							_RemoveLink( pglRemove );
						}
						else
						{
							break;
						}
					}
					if ( r != m_rgrngLookup[ pgl->RElConst() ] )
					{
						// Then a unique range - see if we already have it:
						pair< _TySetCompCharRange::iterator, _TySetCompCharRange::iterator >
							pit = m_pSetCompCharRange->equal_range( r );
						_TySetCompCharRange::iterator	itFound;
						if ( ( itFound = find_if( pit.first, pit.second, 
										unary1st( bind2nd( equal_to< _TyRange >(), r ), 
															_TySetCompCharRange::value_type() ) ) ) == pit.second )
						{
							// Then a new range:
							_TySetCompCharRange::value_type	vt( r, (_TyAlphaIndex)m_pCCRIndex->size() );
							itFound = m_pSetCompCharRange->insert( vt );
							m_pCCRIndex->push_back( itFound );
						}

						pgl->RElNonConst() = -( itFound->second + 1 );
					}
				}
			}
		}
	}

	void	_CreateRangeLookup()
	{
		if ( !m_rgrngLookup )
		{
			m_rgrngLookup.allocate( m_setAlphabet.size() );
			_TyRange *	prngLookup = m_rgrngLookup.begin();
			for ( _TyAlphabet::iterator itAlpha = m_setAlphabet.begin();
						itAlpha != m_setAlphabet.end(); ++itAlpha, ++prngLookup )
			{
				*prngLookup = *itAlpha;
			}
		}
	}

	_TyAlphaIndex	_LookupAlphaSetNum( _TyRangeEl const & _rc )
	{
		_TyRange *	prngFound;
		prngFound = lower_bound(	m_rgrngLookup.begin(), m_rgrngLookup.end(), 
															_TyRange( _rc, _rc ) );
		if (	( prngFound != m_rgrngLookup.end() ) &&
					prngFound->contains( _rc ) )
		{
			size_type st = prngFound - m_rgrngLookup.begin();
			if (st > numeric_limits< _tyAlphaIndex >::(max)())
				throw alpha_index_overflow("_LookupAlphaSetNum(): Alpha index overflowed.");
			return prngFound - m_rgrngLookup.begin();
		}
		else
		{
			return -1;
		}
	}

	// Attempt to match the passed null terminated string:
	bool	MatchString(	_TyDfaCtxt const & _rctxt, const t_TyChar * _pc )
	{
		if ( m_fHasDeadState )
		{
			assert( 0 );	// ni - though easy.
		}

		_TySetStates * pssAccepting;
		CMFDtor1_void< _TyThis, typename _TySSCache::iterator const &, typename _TySSCache::iterator >
			releaseSSAccepting( this, &_TyThis::_ReleaseSSCache, _GetSSCache( pssAccepting ) );
		pssAccepting->clear();
		_rctxt.GetAcceptingNodeSet( *pssAccepting );

		const _TyGraphNode *	pgnCur = _rctxt.m_pgnStart;

		_CreateRangeLookup();	// Create the range lookup table.

		int	iAlphaset;

		for ( ; *_pc; ++_pc )
		{
			// Find the current input range in the alphabet set - if not there then invalid char:
			if ( ( iAlphaset = _LookupAlphaSetNum( *_pc ) ) >= 0 )
			{
				// See if we have a transition on this:
				typename _TyGraph::_TyLinkPosIterConst	lpi( pgnCur->PPGLChildHead() );
				for ( ; !lpi.FIsLast(); lpi.NextChild() )
				{
					if ( *lpi < 0 )
					{
						if ( LookupRange( *lpi ).contains( *_pc ) )
							break;
					}
					else
					{
						if ( *lpi == iAlphaset )
						{
							pgnCur = lpi.PGNChild();
							break;
						}
						else
						if ( *lpi > iAlphaset )
						{
							return false;
						}
					}
				}
				if ( lpi.FIsLast() )
				{
					return false;
				}
			}
			else
			{
				// character not in alphabet:
				return false;
			}
		}

		// If we are at an accepting state:
		return _rctxt.m_pssAccept->isbitset( pgnCur->RElConst() );
	}

	void	Dump( ostream & _ros, _TyDfaCtxt const & _rCtxt ) const
	{
		// Dump the alphabet and the graph:
		DumpAlphabet( _ros );
		_ros << "NFA:\n";	
		_ros << "Start state : {" << _rCtxt.m_pgnStart->RElConst() << "}.\n";
		_ros << "Accepting states :";
		DumpStates( _ros, *_rCtxt.m_pssAccept );
		_ros << "\n";
		m_gDfa.dump_node( _ros, _rCtxt.m_pgnStart );
	}

protected:

	void	DestroySubGraph( _TyGraphNode * _pgn )
	{
		m_gDfa.destroy_node( _pgn );
	}

	void	_RemoveLink( _TyGraphLink * _pgl )
	{
		_pgl->RemoveParent();
		_pgl->RemoveChild();
    m_gDfa.destroy_link( _pgl );
	}

  // Caller owns the lifetime of result *_ppgn.
	void		_NewStartState( _TyGraphNode ** _ppgn )
	{
    *_ppgn = m_gDfa.create_node1( m_iCurState );
		_UpdateNodeLookup( *_ppgn );
		m_iCurState++;
	} 

  // Caller does not own lifetime of results *_ppgnAccept and *_ppglAdded.
  // They are part of the graph.
	void		_NewAcceptingState( _TyGraphNode * _pgnSrc, 
															_TyAlphaIndex const & _r,
															_TyGraphNode ** _ppgnAccept,
															_TyGraphLink ** _ppglAdded )
	{
		_TyGraphLink *	pgl = m_gDfa._STLP_TEMPLATE create_link1
      < _TyAlphaIndex const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gDfa, &_TyGraph::destroy_link, pgl );
#else __DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif __DGRAPH_INSTANCED_ALLOCATORS

    *_ppgnAccept = m_gDfa.create_node1( m_iCurState );
		CMFDtor1_void< _TyGraph, _TyGraphNode * >
			endNode( &m_gDfa, &_TyGraph::destroy_node, *_ppgnAccept );

    // Add both new objects to the graph:
		_pgnSrc->AddChild(	**_ppgnAccept, *pgl, 
												*(_pgnSrc->PPGLBChildHead()),
												*((*_ppgnAccept)->PPGLBParentHead()) );
    // no throwing here.
		endNode.Reset();
		endLink.Reset();
    // throwing ok again.

		if ( _ppglAdded )
		{
			*_ppglAdded = pgl;
		}
	
		_UpdateNodeLookup( *_ppgnAccept );
		m_iCurState++;
	}

	void		_NewTransition( _TyGraphNode * _pgnSrc, 
													_TyAlphaIndex const & _r,
													_TyGraphNode * _pgnSink,
													_TyGraphLink ** _ppglAdded )
	{
		_TyGraphLink *	pgl = m_gDfa._STLP_TEMPLATE create_link1
      < _TyAlphaIndex const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gDfa, &_TyGraph::destroy_link, pgl );
#else __DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif __DGRAPH_INSTANCED_ALLOCATORS

		_pgnSrc->AddChild(	*_pgnSink, *pgl, 
												*(_pgnSrc->PPGLBChildHead()),
												*(_pgnSink->PPGLBParentHead()) );
    // no throwing here.
		endLink.Reset();

		if ( _ppglAdded )
		{
			*_ppglAdded = pgl;
		}
	}

};

template < class t_TyChar, class t_TyAllocator = allocator< t_TyChar > >
class _dfa_context
	: public _context_base< t_TyChar >
{
private:
	typedef _context_base< t_TyChar >								_TyBase;
	typedef _dfa_context< t_TyChar, t_TyAllocator >	_TyThis;
	typedef typename _Alloc_traits< _TyThis, t_TyAllocator >::allocator_type	_TyAllocThis;
public:

	typedef typename t_TyAllocator::size_type						size_type;
	typedef _dfa< t_TyChar, t_TyAllocator >							_TyDfa;
	typedef typename _TyDfa::_TyGraph										_TyGraph;
	typedef typename _TyDfa::_TyGraphNode								_TyGraphNode;
	typedef typename _TyDfa::_TyGraphLink								_TyGraphLink;
	typedef typename _TyDfa::_TySetStates								_TySetStates;
	typedef typename _TyDfa::_TyAcceptAction						_TyAcceptAction;
	typedef _swap_object< _TySetStates >								_TySwapSS;
	typedef less< _TySwapSS >														_TyCompareStates;
	typedef map<	_TySwapSS, _TyAcceptAction, 
								_TyCompareStates, t_TyAllocator >			_TyPartAcceptStates;
	typedef hash_map< _TyState, typename _TyPartAcceptStates::value_type *,
										hash< _TyState >, equal_to< _TyState >,
										t_TyAllocator >										_TyAcceptPartLookup;

	_TyGraphNode *															m_pgnStart;
	_sdpd< _TySetStates, t_TyAllocator >				m_pssAccept;
	_TyPartAcceptStates													m_partAccept;		// Accepting state partition.
	_sdpd< _TyAcceptPartLookup, t_TyAllocator >	m_pPartLookup;	// Accepting state partition lookup by state.

	_dfa_context( _TyDfa & _rDfa )
		: _TyBase( _rDfa ),
			m_pgnStart( 0 ),
			m_pssAccept( _rDfa.get_allocator() ),
			m_partAccept( _TyCompareStates(), _rDfa.get_allocator() ),
			m_pPartLookup( _rDfa.get_allocator() )
	{
	}

	~_dfa_context()
	{
		if ( m_pgnStart )
		{
			// We own this node - destroy it:
			RDfa().DestroySubGraph( m_pgnStart );
		}
	}

	_TyDfa & RDfa()							{ return static_cast< _TyDfa & >( m_rFaBase ); }
	_TyDfa const & RDfa() const { return static_cast< const _TyDfa & >( m_rFaBase ); }

	void	CreateAcceptingNodeSet()
	{
		m_pssAccept._STLP_TEMPLATE construct2< _TyState, const t_TyAllocator & >
			( RDfa().NStates(), RDfa().get_allocator() );
		m_pssAccept->clear();
	}

	void	GetAcceptingNodeSet( _TySetStates & _rss ) const
	{
		assert( m_pssAccept );
		_rss = *m_pssAccept;
	}

	void
	CreateAcceptPartLookup()
	{
		m_pPartLookup._STLP_TEMPLATE construct4< _TyAcceptPartLookup::size_type,
															const _TyAcceptPartLookup::hasher &,
															const _TyAcceptPartLookup::key_equal &,
															const t_TyAllocator & >
															( m_pssAccept->countsetbits() * 2,
															_TyAcceptPartLookup::hasher(),
															_TyAcceptPartLookup::key_equal(),
															RDfa().get_allocator() );

		// Get the accepting states from the NFA:
		_TySetStates * pssUtil;
		CMFDtor1_void< _TyDfa, _TyDfa::_TySSCache::iterator const &, _TyDfa::_TySSCache::iterator >
			releaseUtil(	&RDfa(), &_TyDfa::_ReleaseSSCache, 
										RDfa()._GetSSCache( pssUtil ) );

		// We will create a lookup of all the accepting states to their associated partition.
		_TyPartAcceptStates::iterator	itPAEnd = m_partAccept.end();
		for ( _TyPartAcceptStates::iterator itPA = m_partAccept.begin();
					itPA != itPAEnd; ++itPA )
		{
			_TyPartAcceptStates::value_type &	rvtPA = *itPA;

			*pssUtil = rvtPA.first;

			for ( size_type	stAccept = pssUtil->getclearfirstset();
						pssUtil->size() != stAccept;
						stAccept = pssUtil->getclearfirstset( stAccept ) )
			{
				_TyAcceptPartLookup::value_type	vt( stAccept, &rvtPA );
#ifndef NDEBUG
				pair< _TyAcceptPartLookup::iterator, bool >	pib = 
#endif !NDEBUG
				m_pPartLookup->insert( vt );
#ifndef NDEBUG
				assert( pib.second );
#endif !NDEBUG
			}
		}
	}

	typename _TyPartAcceptStates::value_type *
	PVTGetAcceptPart( _TyState _iState )
	{
		typename _TyAcceptPartLookup::iterator it = m_pPartLookup->find( _iState );
		return it == m_pPartLookup->end() ? 0 : ( it->second );
	}

	void DeallocAcceptPartLookup()
	{
		m_pPartLookup.destruct();
	}
	
	void	Dump( ostream & _ros ) const
	{
		// Dump the alphabet and the graph:
		RDfa().Dump( _ros, *this );
	}

	void	ProcessUnsatisfiableTranstitions()
	{
		assert( !RDfa().m_fHasDeadState );	// This should be removed first.

		if ( !RDfa().m_nUnsatisfiableTransitions )
			return;

		assert( !( RDfa().m_nUnsatisfiableTransitions % 2 ) );	// Should have an even number of unsats.

		// The algorithm works like this:
		// 1) Move through the states starting from zero looking for unsatifiable trailing contexts -
		//		which look like a state with a single odd unsatisfiable out transition.
		// 2) Starting at (1) search parents recursively for the set of first seen nodes whose out
		//		transitions contain no unsatifiable transitions. Record the links that enter these
		//		nodes. As we find nodes whose out transitions contain unsatisfiable transitions zero
		//		the node lookup for those nodes.
		// 3) Disconnect the child nodes from the links found. Reconnect to the state entered 
		//		into by the odd unsatisfiable transition(1).
		// 4) Disconnect the child of (1) and destroy the subgraph at the parent.
		// 5) search for more trailing contexts (1).

		_TyDfa::_TyAlphaIndex	aiLimit = (_TyDfa::_TyAlphaIndex)( RDfa().m_setAlphabet.size()-1-RDfa().m_nUnsatisfiableTransitions );

		size_t	stRemoved = 0;

		_TySetStates	ssFoundNodes( RDfa().NStates(), RDfa().get_allocator() );
		ssFoundNodes.clear();

		// If we have trigger transitions then they are first.
		for (_TyState nState = 0; nState < RDfa().NStates(); ++nState )
		{
			_TyGraphNode *	pgn = RDfa().PGNGetNode( nState );
			if ( pgn )
			{
				// Then check for a single odd unsat out trans:
				_TyGraphLink *	pglUnsat = *pgn->PPGLChildHead();
				if (	pglUnsat && !*pglUnsat->PPGLBGetNextChild() &&
							( pglUnsat->RElConst() > aiLimit ) &&
							!( ( pglUnsat->RElConst() - aiLimit ) % 2 ) )
				{
					// Then we have found the trailing context of a portion of the graph to be excised.
					_TyGraphNode *	pgnReconnect = pglUnsat->PGNChild();

					// Get a forward graph iterator iterating the parents:
					typename _TyGraph::iterator	gfit( pgn, 0, true, false, RDfa().get_allocator() );
					_TyGraphLink *	pglEntered = 0;
					while ( !gfit.FAtEnd() )
					{
						if ( gfit.PGLCur() )
						{
							// If this link enters into a node we have found below before then
							//	update the link:
							pglEntered = gfit.PGLCur();
							++gfit;
							if ( ssFoundNodes.isbitset( pglEntered->PGNParent()->RElConst() ) )
							{
								pglEntered->RemoveParent();
								pglEntered->InsertParent( pgnReconnect->PPGLParentHead() );
								pglEntered->SetChildNode( pgnReconnect );
								__DEBUG_STMT( pglEntered = 0 )
							}
						}
						else
						{
							// Then at a node - check to see if has any unsat out trans - they are at the beginning
							//	- but after any trigger transitions:
							_TyGraphLink *	pglFirst = *(gfit.PGNCur()->PPGLChildHead());
							if ( RDfa().m_fHasTriggers )
							{
								if ( pglFirst && ( pglFirst->RElConst() == aiLimit ) )
								{
									pglFirst = *pglFirst->PPGLGetNextChild();
								}
							}

							if ( pglFirst && ( pglFirst->RElConst() > aiLimit ) )
							{
								// Then found one - this node to be deleted:
								RDfa().m_nodeLookup[ gfit.PGNCur()->RElConst() ] = 0;
								++gfit;	// Move to next node or link.
								++stRemoved;
							}
							else
							{
								assert( pglEntered );
								assert( pglEntered->PGNParent() == gfit.PGNCur() );

								// This node to remain in graph - record the state:
								ssFoundNodes.setbit( gfit.PGNCur()->RElConst() );
								gfit.SkipContext();	// Skip to next context.
								// We can safely move the link:
								pglEntered->RemoveParent();
								pglEntered->InsertParent( pgnReconnect->PPGLParentHead() );
								pglEntered->SetChildNode( pgnReconnect );
								__DEBUG_STMT( pglEntered = 0 )
							}
						}
					}
					pglUnsat->RemoveParent();
					pglUnsat->SetChildNode( 0 );
					
					RDfa().m_gDfa.destroy_node( pgn );
				}
			}
		}

		if ( stRemoved )
		{
			CompressNodeLookup( stRemoved );
		}
	}

	void	RemoveDeadState( )
	{
		if ( RDfa().m_fHasDeadState )
		{
			// Remove dead state parents - then can call destroy on it:
			typename _TyGraph::_TyLinkPosIterNonConst	lpi( RDfa().PGNGetNode( 0 )->PPGLParentHead() );
			while( !lpi.FIsLast() )
			{
				RDfa()._RemoveLink( lpi.PGLCur() );
			}

			// Dead state is orphaned - but has a lot of children ( itself ).
			RDfa().m_gDfa.destroy_node( RDfa().PGNGetNode( 0 ) );
			RDfa().m_nodeLookup[ 0 ] = 0;

			RDfa().m_fHasDeadState = false;
		}
	}

	void	CompressNodeLookup( size_type _stNonReps )
	{
		size_type	stNewStates = RDfa().NStates() - _stNonReps;

		// We should compress the state numbers and the node lookup in the DFA.
		// At the same time set up the new accepting state set.
		_TySetStates	ssNewAccept( stNewStates, RDfa().get_allocator() );
		ssNewAccept.clear();

		// Create the accept partition lookup in the dfa context:
		CreateAcceptPartLookup();

		// Create a map from a pointer to an accept partition element to
		//	the new parition for that accept state:
		typedef map<	_TyPartAcceptStates::value_type *, _TySwapSS, 
									less< _TyPartAcceptStates::value_type * >,
									t_TyAllocator >	_TyMapToNewAccept;
		_TyMapToNewAccept	mapToNewAccept
#ifndef __GNUC__
		( less< typename _TyPartAcceptStates::value_type * >(), RDfa().get_allocator() )
#endif __GNUC__
		;

		// Move through and create the initial ( empty ) state sets:
		{ // SCOPE
			_TyPartAcceptStates::iterator	itPartEnd = m_partAccept.end();
			for ( _TyPartAcceptStates::iterator	it = m_partAccept.begin();
						it != itPartEnd; ++it )
			{
				_TyMapToNewAccept::value_type	vt( &(*it), _TySetStates( stNewStates, RDfa().get_allocator() ) );
				vt.second.RObject().clear();
				mapToNewAccept.insert( vt );
			}
		}
		
		_TyState	iStartSpace = -1;
		_TyState	iStartNS = -1;
		_TyState	iProcessedEmpty = 0;
		_TyState	iEmptyNodes = 0;
		_TyState iState;
		for ( iState = 0; iState < RDfa().NStates(); iState++ )
		{
			if ( RDfa().m_nodeLookup[ iState - iProcessedEmpty ] )
			{
				if ( iStartNS < 0 && iStartSpace >= 0 )
				{
					iStartNS = iState;
				}
				if ( m_pssAccept->isbitset( iState ) )
				{
					ssNewAccept.setbit( iState - iEmptyNodes );

					// Set the state in the appropriate accept partition:
					_TyMapToNewAccept::iterator itPart = mapToNewAccept.find( 
																									PVTGetAcceptPart( iState ) );
					assert( mapToNewAccept.end() != itPart );
					itPart->second.RObject().setbit( iState - iEmptyNodes );
				}
				if ( iEmptyNodes )
				{
					RDfa().PGNGetNode( iState - iProcessedEmpty )->RElNonConst() -= iEmptyNodes;
				}
			}
			else
			{
				if ( iStartSpace < 0 )
				{
					iStartSpace = iState;
				}
				else
				{
					if ( iStartNS > 0 )
					{
						// Then we need to process empty space:
						RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + ( iStartSpace - iProcessedEmpty ),
																				RDfa().m_nodeLookup.begin() + ( iStartNS - iProcessedEmpty ) );
						iStartNS = -1;
						iProcessedEmpty = iEmptyNodes;
						iStartSpace = iState;
					}
				}
				iEmptyNodes++;
			}
		}

		// If we didn't process the last non-space:
		if ( ( iStartSpace >= 0 ) && ( iStartNS > 0 ) )
		{
			RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + ( iStartSpace - iProcessedEmpty ),
																	RDfa().m_nodeLookup.begin() + ( iStartNS - iProcessedEmpty ) );
		}
		else
		// If there was empty space at the end:
		if ( ( iStartNS < 0 ) && ( iStartSpace >= 0 ) )
		{
			RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + ( iStartSpace - iProcessedEmpty ),
																	RDfa().m_nodeLookup.end() );
		}

		// Get rid of the lookup - it is at the old size:
		DeallocAcceptPartLookup();

		// Create the accept state partition from the old and then swap in:
		_TyPartAcceptStates	partAcceptNew
#ifndef __GNUC__
		( _TyCompareStates(), RDfa().get_allocator() )
#endif !__GNUC__
		;

		_TyMapToNewAccept::iterator itMapEnd = mapToNewAccept.end();
		for ( _TyMapToNewAccept::iterator	itMap = mapToNewAccept.begin();
					itMap != itMapEnd; ++itMap )
		{
			// Swap the bitvector in from <mapToNewAccept>:
			_TyPartAcceptStates::value_type	vt( itMap->second, itMap->first->second );
			pair< _TyPartAcceptStates::iterator, bool >	pibInsert = partAcceptNew.insert( vt );
			assert( pibInsert.second );
		}

		m_partAccept.swap( partAcceptNew );

		// Swap in the new accept states:
		m_pssAccept->swap( ssNewAccept );

		// Set the number of states in the DFA:
		assert( RDfa().m_nodeLookup.size() == stNewStates );
		RDfa().SetNumStates( stNewStates );

		RDfa()._ClearSSCache();	// Clear any SS cache's - they are at the old size.

		assert( RDfa().m_nodeLookup.size() == m_pssAccept->size() );
	}

	// Compress accept paritions that correspond to the same trigger actions.
	// This allows the optimizer to generate more optimal DFA's - since these
	//	states have no reason ( after disambiguation ) to have differring action
	//	ids.
	void	CompressTriggerAcceptPartitions()
	{
		typedef map<	_ref< const _TyAcceptAction >,
									pair< _TyPartAcceptStates::iterator, _TySwapSS >,
									less< _ref< const _TyAcceptAction > >,
									t_TyAllocator >	_TySetTriggerParts;

		_TySetTriggerParts	setTriggerParts
#ifndef __GNUC__
		(	_TySetTriggerParts::key_compare(), RDfa().get_allocator() )
#endif  !__GNUC__
		;

		_TyPartAcceptStates::iterator	itPartEnd = m_partAccept.end();
		_TyPartAcceptStates::iterator	itPart = m_partAccept.begin();
		while ( itPartEnd != itPart )
		{
			_TyPartAcceptStates::value_type & rvtPart = *itPart;

			// Only compress single trigger actions:
			if (	( e_aatTrigger == rvtPart.second.m_eaatType ) &&
						!rvtPart.second.m_psrTriggers )
			{
				_TySetTriggerParts::iterator	itSTP;
				if ( setTriggerParts.end() == ( itSTP = setTriggerParts.find( rvtPart.second ) ) )
				{
					setTriggerParts.insert( _TySetTriggerParts::value_type( rvtPart.second,
																	_TySetTriggerParts::mapped_type( 
																		itPart, _TySetStates( 0, RDfa().get_allocator() ) ) ) );
					++itPart;
				}
				else
				{
					// Already have this trigger action - update the set of states with that from
					//	this iterator - then remove this record.
					_TySetTriggerParts::value_type & rvtSTP = *itSTP;
					if ( rvtSTP.second.second.RObject().size() )
					{
						rvtSTP.second.second.RObject() |= rvtPart.first.RObject();
					}
					else
					{
						_TyPartAcceptStates::value_type & rvtFirst = *rvtSTP.second.first;
						_TySetStates	ssCopyFirst( rvtFirst.first.RObject() );
						ssCopyFirst |= rvtPart.first.RObject();
						rvtSTP.second.second.RObject().swap( ssCopyFirst );
					}
					_TyPartAcceptStates::iterator	itErase = itPart++;
					m_partAccept.erase( itErase );
				}
			}
			else
			{
				++itPart;
			}
		}

		// Now move through the compressed states updating:
		_TySetTriggerParts::iterator	itSTPEnd = setTriggerParts.end();
		_TySetTriggerParts::iterator	itSTP = setTriggerParts.begin();
		for ( ; itSTPEnd != itSTP; ++itSTP )
		{
			_TySetTriggerParts::value_type & rvtSTP = *itSTP;
			// If we compressed the trigger then update:
			if ( rvtSTP.second.second.RObject().size() )
			{
				_TyPartAcceptStates::value_type vtCopy( *rvtSTP.second.first );
				m_partAccept.erase( rvtSTP.second.first );
				const_cast< _TySetStates& >( vtCopy.first.RObject() ).
						swap( rvtSTP.second.second.RObject() );
				__DEBUG_COMMA_2( pair< _TyPartAcceptStates::iterator, bool > pib = )
				m_partAccept.insert( vtCopy );
				assert( pib.second );
			}
		}
	}


};

__REGEXP_END_NAMESPACE

#endif __L_DFA_H

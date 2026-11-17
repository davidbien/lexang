#ifndef __L_DFA_H
#define __L_DFA_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_dfa.h

// This module declares DFA types.

#include <unordered_map>
#include <algorithm>
#include <vector>
#include "_l_ns.h"

__REGEXP_BEGIN_NAMESPACE

class alpha_index_overflow : public _t__Named_exception< __L_DEFAULT_ALLOCATOR >
{
	typedef alpha_index_overflow _TyThis;
	typedef _t__Named_exception< __L_DEFAULT_ALLOCATOR > _TyBase;
public:
  alpha_index_overflow( const char * _pc )
    : _TyBase( _pc ) 
  {
  }
  alpha_index_overflow( const string_type & __s ) 
    : _TyBase( __s ) 
  {
  }
};

template < class t_TyChar, class t_TyAllocator >
class _dfa_context;

template < class t_TyGraphLink >
struct _sort_dfa_link 
{
	bool	operator()( const t_TyGraphLink * const & _rpglL, 
										const t_TyGraphLink * const & _rpglR ) const _BIEN_NOTHROW
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
protected:
	using _TyBase::m_iCurState;
	using _TyBase::m_setAlphabet;
	using _TyBase::_STUpdateNodeLookup;
	using _TyBase::ms_kreTriggerStart;
public:
	using _TyBase::m_nodeLookup;
	using _TyBase::get_allocator;
	using _TyBase::NStates;
	using _TyBase::_ReleaseSSCache;
	using _TyBase::DumpAlphabet;

	// Friends:
	template < class t__TyNfa, class t__TyDfa >
	friend struct _create_dfa;
	template < class t__TyNfa, class t__TyDfa >
	friend struct _create_dfa_tagged;
	template < class t_TyDfa, bool f_tPartDeadImmed >
	friend struct _optimize_dfa;
	friend class _dfa_context< t_TyChar, t_TyAllocator >;
	
	typedef t_TyAllocator _TyAllocator;

	typedef size_t size_type;
	typedef _dfa_context< t_TyChar, t_TyAllocator >	_TyDfaCtxt;
	typedef _TyDfaCtxt _TyContext;
	typedef typename _TyBase::_TyState _TyState;
	typedef typename _TyBase::_TyRange _TyRange;
	typedef typename _TyBase::_TyRangeEl _TyRangeEl;
	typedef typename _TyBase::_TyAcceptAction _TyAcceptAction;
	typedef typename _TyBase::_TyNodeLookup _TyNodeLookup;
	typedef typename _TyBase::_TyAlphabet _TyAlphabet;
	typedef typename _TyBase::_TySetStates _TySetStates;

	typedef int64_t	_TyAlphaIndex;	// We use an index into a range lookup table as the link element.
															// If the index is negative then it is an index into the CCRIndex.

	typedef __DGRAPH_NAMESPACE dgraph< _TyState, _TyAlphaIndex, false, t_TyAllocator > _TyGraph;
	typedef typename _TyGraph::_TyGraphNode _TyGraphNode;
	typedef typename _TyGraph::_TyGraphLink _TyGraphLink;
	_TyGraph m_gDfa;

	bool m_fHasDeadState;

	// Char range lookup stuff:
	_sdpn< _TyRange, t_TyAllocator > m_rgrngLookup;

	// Compressed character range map - use _TyRange::CanonicalLess() since we may have overlapping ranges.
  typedef typename _Alloc_traits< typename multimap< _TyRange, _TyAlphaIndex >::value_type, t_TyAllocator >::allocator_type _TySetCompCharRangeAllocator;
  typedef map< _TyRange, _TyAlphaIndex, _fa_char_range_canonical_less< _TyRange >, _TySetCompCharRangeAllocator > _TySetCompCharRange;
  typedef typename _Alloc_traits< typename deque< typename _TySetCompCharRange::iterator >::value_type, t_TyAllocator >::allocator_type _TyDequeCCRIndexAllocator;
  typedef deque< typename _TySetCompCharRange::iterator, _TyDequeCCRIndexAllocator > _TyDequeCCRIndex;
	
	_sdpd< _TySetCompCharRange, t_TyAllocator >	m_pSetCompCharRange;
	_sdpd< _TyDequeCCRIndex, t_TyAllocator > m_pCCRIndex;

	bool m_fHasLookaheads;	// any lookaheads in the rules ?
	// For each lookahead rule the DFA which matches its trailing context in reverse, by the action id of the rule's
	//	lookahead state - see _l_lkahd.h.
	typedef _dfa_lookahead_reverse< t_TyChar, t_TyAllocator > _TyLookaheadReverse;
	typedef std::shared_ptr< _TyLookaheadReverse > _TySpLookaheadReverse;
	typedef std::map< vtyActionIdent, _TySpLookaheadReverse > _TyMapLookaheadReverse;
	_TyMapLookaheadReverse m_mapLookaheadReverse;
	size_type m_nTriggers;		// # of triggers
	size_type m_nUnsatisfiableTransitions; // # of unsatisfiable.
	vtyActionIdent m_iMaxActions;

	// We consume the tokenid->rangeel map converting it to a rangeel->tokenid map as we consume.
	typedef typename _Alloc_traits< typename map< vtyTokenIdent, _TyRangeEl >::value_type, t_TyAllocator >::allocator_type _TyMapTokenIdToTriggerTransitionAllocator;
	typedef map< vtyTokenIdent, _TyRangeEl, less< vtyTokenIdent >, _TyMapTokenIdToTriggerTransitionAllocator > _TyMapTokenIdToTriggerTransition;
	typedef typename _Alloc_traits< typename map< _TyRangeEl, vtyTokenIdent >::value_type, t_TyAllocator >::allocator_type _TyMapTriggerTransitionToTokenIdAllocator;
	typedef map< _TyRangeEl, vtyTokenIdent, less< _TyRangeEl >, _TyMapTriggerTransitionToTokenIdAllocator > _TyMapTriggerTransitionToTokenId;
	_TyMapTriggerTransitionToTokenId m_mapTriggerTransitionToTokenId;

	// Triggers - we need an extra copy since ambiguity may have occurred:
	typedef less< vtyActionIdent > _TyCompareAI;
  typedef typename _Alloc_traits< typename map< vtyActionIdent, _TyAcceptAction, _TyCompareAI >::value_type, t_TyAllocator >::allocator_type _TyMapTriggersAllocator;
  typedef map< vtyActionIdent, _TyAcceptAction, _TyCompareAI, _TyMapTriggersAllocator >	_TyMapTriggers;
	_sdpd< _TyMapTriggers, t_TyAllocator >	m_pMapTriggers;

	// Tagged DFA - see _l_dfatag.h for an overview.
	// In a tagged DFA the triggers are not transitions between states: the lexical analyzer instead records the
	//	positions at which triggers are crossed in "registers" as it moves through the input, one register for each
	//	of the ways that the input read thus far might yet be matched. The triggers of the way in which the token was
	//	actually matched are then fired once the token has been found.
	// The transitions of a tagged DFA are all on characters with the exception of those leaving the "tag operation
	//	states": such a state has a single transition, on one of m_nTriggers pseudo-triggers, each of which corresponds
	//	to a list of operations upon the registers in m_rgTagOpLists. A transition on a character which requires
	//	operations to be performed enters such a state and its single transition leads to the destination.
	bool m_fTagged{false};
	// An operation on the registers. When m_iTag is negative: copy the register m_nSrc to m_nDst.
	// Otherwise record that the trigger m_iTag ( an index into m_rgTags ) was crossed at the current position
	//	in the register m_nDst.
	struct _TyTagOp
	{
		unsigned short m_nDst;
		unsigned short m_nSrc;
		int m_iTag;
		bool operator < ( _TyTagOp const & _r ) const
		{
			return ( m_nDst != _r.m_nDst ) ? ( m_nDst < _r.m_nDst ) : ( m_nSrc != _r.m_nSrc ) ? ( m_nSrc < _r.m_nSrc ) : ( m_iTag < _r.m_iTag );
		}
		bool operator == ( _TyTagOp const & _r ) const
		{
			return ( m_nDst == _r.m_nDst ) && ( m_nSrc == _r.m_nSrc ) && ( m_iTag == _r.m_iTag );
		}
	};
	// A temporary register is needed when registers are exchanged: this is its number within the operations - it
	//	becomes the last register: the register m_nTagRegisters-1.
	static constexpr unsigned short s_knTagRegisterTemp = 0xffff;
	typedef std::vector< _TyTagOp > _TyTagOpList;
	// The lists of operations - the list at index n is performed on the transition ms_kreTriggerStart + n.
	std::vector< _TyTagOpList > m_rgTagOpLists;
	// The triggers ( as indices into m_rgTags ) crossed between the last character of a token and the end of its rule
	//	for the accepting states that have such - indexed by _fa_accept_action<>::m_iTagFinish.
	std::vector< std::vector< int > > m_rgTagFinishes;
	// The tags: a tag is a trigger - all of the uses of a trigger within the rules are the same tag.
	struct _TyTag
	{
		vtyTokenIdent m_tidTrigger; // The token id of the trigger's action object.
		vtyActionIdent m_aiTrigger; // The action id in m_pMapTriggers of ( one of the uses of ) the trigger.
		// The position of this trigger in an order of the triggers in which they are always crossed, each at most once,
		//	however a token is matched. When there is no such order - a trigger may be crossed more than once, or two
		//	triggers may be crossed in either order - this is -1 for every tag. See m_fTagSlots.
		int m_nSlot;
	};
	std::vector< _TyTag > m_rgTags;
	// When true each trigger is crossed at most once and in a known order: the positions of the triggers may then be
	//	kept in a fixed set of registers. Otherwise the lexical analyzer must record the history of the triggers crossed.
	bool m_fTagSlots{false};
	size_t m_nTagRegisters{0}; // The number of registers required - including the temporary.

	_dfa( t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyBase( _rAlloc ),
			m_gDfa( typename _TyGraph::_TyAllocatorSet( _rAlloc, _rAlloc, _rAlloc ) ),
			m_fHasDeadState( false ),
			m_rgrngLookup( _rAlloc ),
			m_pSetCompCharRange( _rAlloc ),
			m_mapTriggerTransitionToTokenId( _rAlloc ),
			m_pCCRIndex( _rAlloc ),
			m_fHasLookaheads( false ),
			m_nTriggers( 0 ),
			m_nUnsatisfiableTransitions( 0 ),
			m_pMapTriggers( _rAlloc )
	{
	}

#ifdef _DEBUG
  ~_dfa()
  {
  }
#endif //_DEBUG

	size_type	AlphabetSize() const	
	{ 
		return m_setAlphabet.size(); 
	}
	_TyAlphaIndex AIGetLastSatisfiable() const
	{
		return (_TyAlphaIndex)( AlphabetSize() - 1 - ( m_nTriggers + m_nUnsatisfiableTransitions ) );
	}

	_TyGraphNode *	PGNGetNode( _TyState _iState )
	{
		return static_cast< _TyGraphNode * >( m_nodeLookup[ (size_t)_iState ] );
	}

	_TyRange	LookupRange( _TyAlphaIndex _ai ) const
	{
		if ( _ai < 0 )
		{
			// Then a compressed range:
			return (*m_pCCRIndex)[  (size_t)( -(_ai + 1) ) ]->first;
		}
		else
		{
			return m_rgrngLookup[ (size_t)_ai ];
		}
	}

	typedef std::pair< _TyAlphaIndex, _TyAlphaIndex > _TyPrAI;
	void GetTriggerUnsatAIRanges( _TyPrAI * _prpraiTriggers, _TyPrAI * _praiUnsatisfiable ) const
	{
		if ( _prpraiTriggers )
		{
			_prpraiTriggers->first = AlphabetSize() - m_nTriggers - m_nUnsatisfiableTransitions;
			_prpraiTriggers->second = _prpraiTriggers->first + m_nTriggers;
		}
		if ( _praiUnsatisfiable )
		{
			_praiUnsatisfiable->first = AlphabetSize() - m_nUnsatisfiableTransitions;
			_praiUnsatisfiable->second = _praiUnsatisfiable->first + m_nUnsatisfiableTransitions;
		}
	}

	void ConsumeMapTokenIdToTriggerTransition( _TyMapTokenIdToTriggerTransition const & _r )
	{
		Assert( !m_mapTriggerTransitionToTokenId.size() );
		typename _TyMapTokenIdToTriggerTransition::const_iterator cit = _r.begin();
		for ( ; _r.end() != cit; ++cit )
		{
			pair< typename _TyMapTriggerTransitionToTokenId::iterator, bool > pib = m_mapTriggerTransitionToTokenId.insert( typename _TyMapTriggerTransitionToTokenId::value_type( ms_kreTriggerStart + cit->second, cit->first ) );
			VerifyThrow( !!pib.second );
			pib = m_mapTriggerTransitionToTokenId.insert( typename _TyMapTriggerTransitionToTokenId::value_type( ms_kreTriggerStart + cit->second+1, cit->first ) );
		}
	}

	void	SortTransitions()
	{
		_sdpn< _TyGraphLink *, _TyAllocator >	rgpgl( get_allocator() );
		size_t	stAlphabet;
		rgpgl.allocate( stAlphabet = AlphabetSize() );	// Can't have more transitions than this.

		typedef _sort_dfa_link< _TyGraphLink >	_TySortDfaLinks;

		bool	fCheckUnsat = !!( m_nTriggers + m_nUnsatisfiableTransitions );
		_TyAlphaIndex	aiLimit = AIGetLastSatisfiable();

		typename _TyGraph::_TyLinkPosIterNonConst lpi;
		typename _TyNodeLookup::iterator itnlEnd = m_nodeLookup.end();
		typename _TyNodeLookup::iterator itnlCur = m_nodeLookup.begin();
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
				// Find the first trigger or unsatisfiable transition - they sort to the end. Every link may be one.
				_TyGraphLink ** pppglUnsat = pppglEnd;
				if ( fCheckUnsat )
				{
					while(	( pppglUnsat != pppglBeg ) &&
									( (*(pppglUnsat-1))->RElConst() > aiLimit ) )
					{
						--pppglUnsat;
					}
				}

				// We insert at the head so insert in reverse: the satisfiable transitions and then
				//	the triggers and unsatisfiables which thus end up first, each group in ascending order.
				for ( pppglCur = pppglUnsat; pppglCur != pppglBeg; )
				{
					(*--pppglCur)->InsertChild( ppglChildHead );
				}
				for ( pppglCur = pppglEnd; pppglCur != pppglUnsat; )
				{
					(*--pppglCur)->InsertChild( ppglChildHead );
				}
			}
			else
			{
				Assert( pgn->FParents() );
			}
		}
	}

	void	CompressTransitions()
	{
		Assert( !m_pSetCompCharRange );	// This is only done once - should be done just before generation.
		if ( !m_pSetCompCharRange )
		{
			m_pSetCompCharRange.template emplace< typename _TySetCompCharRange::key_compare const &,
															t_TyAllocator const & >
														( typename _TySetCompCharRange::key_compare(),
															get_allocator() );
			m_pCCRIndex.template emplace< t_TyAllocator const & >( get_allocator() );

			_CreateRangeLookup();	// Create if not already.

			// The transitions are sorted - so compression is relatively easy:
			for ( _TyState sCur = 0; sCur < NStates(); ++sCur )
			{
				typename _TyGraph::_TyLinkPosIterNonConst lpi( PGNGetNode( sCur )->PPGLChildHead() );

				for ( ; !lpi.FIsLast(); )
				{
					_TyGraphLink *	pgl = lpi.PGLCur();
					_TyGraphNode *	pgn = pgl->PGNChild();
					Assert(pgl->RElConst() >= 0);
					_TyRange r = m_rgrngLookup[ (size_t)pgl->RElConst() ];
					_TyRange *	prConsecutive;
					for(	lpi.NextChild(); 
								!lpi.FIsLast() && 
									r.isconsecutiveright( *( prConsecutive = &( m_rgrngLookup[(size_t)*lpi ] ) ) ); )
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
					Assert(pgl->RElConst() >= 0);
					if ( r != m_rgrngLookup[(size_t)pgl->RElConst() ] )
					{
						// Then a unique range - see if we already have it:
						typename _TySetCompCharRange::iterator itFound = m_pSetCompCharRange->find( r );
						if ( m_pSetCompCharRange->end() == itFound )
						{
							// Then a new range:
							typename _TySetCompCharRange::value_type	vt( r, (_TyAlphaIndex)m_pCCRIndex->size() );
							pair< typename _TySetCompCharRange::iterator, bool > pib = m_pSetCompCharRange->insert( vt );
							Assert( pib.second );
							itFound = pib.first;
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
			for ( typename _TyAlphabet::iterator itAlpha = m_setAlphabet.begin();
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
			if (st > (numeric_limits< _TyAlphaIndex >::max)())
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
			Assert( 0 );	// ni - though easy.
		}

    // Set up the release of the pssAccepting. There are a lot of local variables required for this.
    _TySetStates * pssAccepting;
    size_t stRelease = _STGetSSCache(pssAccepting);
    auto fcReleaseAccept = [=]() { _ReleaseSSCache(stRelease); }; // define the lambda for the releasing pssAccepting back to the cache.
    typedef decltype(fcReleaseAccept) tyDeclFcReleaseAccept;
    _fcallobj< tyDeclFcReleaseAccept > fcoReleaseAccept(fcReleaseAccept);
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
		return pssAccepting->isbitset( pgnCur->RElConst() );
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
		size_t stNodeAdded = _STUpdateNodeLookup( *_ppgn );
    Assert(_TyState(stNodeAdded) == m_iCurState);
		m_iCurState++;
	} 

  // Caller does not own lifetime of results *_ppgnAccept and *_ppglAdded.
  // They are part of the graph.
	void		_NewAcceptingState( _TyGraphNode * _pgnSrc, 
															_TyAlphaIndex const & _r,
															_TyGraphNode ** _ppgnAccept,
															_TyGraphLink ** _ppglAdded )
	{
		_TyGraphLink *	pgl = m_gDfa.template create_link1
      < _TyAlphaIndex const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gDfa, &_TyGraph::destroy_link, pgl );
#else //__DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif //__DGRAPH_INSTANCED_ALLOCATORS

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
	
		size_t stNodeAdded = _STUpdateNodeLookup( *_ppgnAccept );
    Assert(_TyState(stNodeAdded) == m_iCurState);
		m_iCurState++;
	}

	void		_NewTransition( _TyGraphNode * _pgnSrc, 
													_TyAlphaIndex const & _r,
													_TyGraphNode * _pgnSink,
													_TyGraphLink ** _ppglAdded )
	{
		_TyGraphLink *	pgl = m_gDfa.template create_link1
      < _TyAlphaIndex const & >( _r );
#ifdef __DGRAPH_INSTANCED_ALLOCATORS
		CMFDtor1_void< _TyGraph, _TyGraphLink * >
			endLink( &m_gDfa, &_TyGraph::destroy_link, pgl );
#else //__DGRAPH_INSTANCED_ALLOCATORS
    CFDtor1_void< _TyGraphLink * >
			endLink( &_TyGraph::destroy_link, pgl );
#endif //__DGRAPH_INSTANCED_ALLOCATORS

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
	typedef _context_base< t_TyChar > _TyBase;
	typedef _dfa_context< t_TyChar, t_TyAllocator > _TyThis;
	typedef typename _Alloc_traits< _TyThis, t_TyAllocator >::allocator_type _TyAllocThis;
public:
	using _TyBase::m_rFaBase;

  typedef typename _Alloc_traits< char, t_TyAllocator >::size_type size_type;
	typedef typename _TyBase::_TyState _TyState;
	typedef _dfa< t_TyChar, t_TyAllocator > _TyDfa;
	typedef typename _TyDfa::_TyGraph _TyGraph;
	typedef typename _TyDfa::_TyGraphNode _TyGraphNode;
	typedef typename _TyDfa::_TyGraphLink _TyGraphLink;
	typedef typename _TyDfa::_TySetStates _TySetStates;
	typedef typename _TyDfa::_TyAcceptAction _TyAcceptAction;
	typedef _swap_object< _TySetStates > _TySwapSS;
	typedef less< _TySwapSS > _TyCompareStates;
  typedef typename _Alloc_traits< typename map< _TySwapSS, _TyAcceptAction, _TyCompareStates >::value_type, t_TyAllocator >::allocator_type _TyPartAcceptStatesAllocator;
  typedef map< _TySwapSS, _TyAcceptAction, _TyCompareStates, _TyPartAcceptStatesAllocator > _TyPartAcceptStates;
  typedef typename _Alloc_traits< typename unordered_map< _TyState, typename _TyPartAcceptStates::value_type * >::value_type, t_TyAllocator >::allocator_type _TyAcceptPartLookupAllocator;
  typedef unordered_map< _TyState, typename _TyPartAcceptStates::value_type *,
                        hash< _TyState >, equal_to< _TyState >,
                      _TyAcceptPartLookupAllocator > _TyAcceptPartLookup;

	_TyGraphNode * m_pgnStart;
	_sdpd< _TySetStates, t_TyAllocator > m_pssAccept;
	_TyPartAcceptStates m_partAccept;		// Accepting state partition.
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

	_TyDfa & RDfa() { return static_cast< _TyDfa & >( m_rFaBase ); }
	_TyDfa const & RDfa() const { return static_cast< const _TyDfa & >( m_rFaBase ); }

	void	CreateAcceptingNodeSet()
	{
		m_pssAccept.template emplace< size_t, const t_TyAllocator & >
			( (size_t)RDfa().NStates(), RDfa().get_allocator() );
		m_pssAccept->clear();
	}

	void	GetAcceptingNodeSet( _TySetStates & _rss ) const
	{
		Assert( m_pssAccept );
		_rss = *m_pssAccept;
	}

	void
	CreateAcceptPartLookup()
	{
		m_pPartLookup.template emplace< typename _TyAcceptPartLookup::size_type,
															const typename _TyAcceptPartLookup::hasher &,
															const typename _TyAcceptPartLookup::key_equal &,
															const t_TyAllocator & >
															( m_pssAccept->countsetbits() * 2,
															typename _TyAcceptPartLookup::hasher(),
															typename _TyAcceptPartLookup::key_equal(),
															RDfa().get_allocator() );

		// Get the accepting states from the NFA:
		_TySetStates * pssUtil;
		CMFDtor1_void< _TyDfa, size_t >
			releaseUtil(	&RDfa(), &_TyDfa::_ReleaseSSCache, RDfa()._STGetSSCache(pssUtil) );

		// We will create a lookup of all the accepting states to their associated partition.
		typename _TyPartAcceptStates::iterator	itPAEnd = m_partAccept.end();
		for ( typename _TyPartAcceptStates::iterator itPA = m_partAccept.begin();
					itPA != itPAEnd; ++itPA )
		{
			typename _TyPartAcceptStates::value_type &	rvtPA = *itPA;

			*pssUtil = rvtPA.first;

			for (typename _TySetStates::size_type stAccept = pssUtil->getclearfirstset();
						pssUtil->size() != stAccept;
						stAccept = pssUtil->getclearfirstset( stAccept ) )
			{
				typename _TyAcceptPartLookup::value_type	vt( stAccept, &rvtPA );
#ifndef NDEBUG
				pair< typename _TyAcceptPartLookup::iterator, bool >	pib = 
#endif //!NDEBUG
				m_pPartLookup->insert( vt );
#ifndef NDEBUG
				Assert( pib.second );
#endif //!NDEBUG
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
		Assert( !RDfa().m_fHasDeadState );	// This should be removed first.

		if ( !RDfa().m_nUnsatisfiableTransitions )
		{
			size_t stRemoved;
			 // We should have no orphaned portions of the graphs.
			VerifyThrowSz( !( stRemoved = _STCheckPruneAlternateRoots( false ) ), "Found orphaned portion(s) or alternate roots and removed them ([%zu] nodes removed), but didn't expect to find orphaned portions.", stRemoved );
			return;
		}

		Assert( !( RDfa().m_nUnsatisfiableTransitions % 2 ) );	// Should have an even number of unsats.

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

		typedef std::pair< typename _TyDfa::_TyAlphaIndex, typename _TyDfa::_TyAlphaIndex > _TyPrAI;
		_TyPrAI praiTriggers, praiUnsatisfiable;
		RDfa().GetTriggerUnsatAIRanges( &praiTriggers, &praiUnsatisfiable );

    typename _TySetStates::size_type stRemoved = 0;

		_TySetStates ssFoundNodes( (size_t)RDfa().NStates(), RDfa().get_allocator() );
		ssFoundNodes.clear();

		// If we have trigger transitions then they are first.
		for (_TyState nState = 0; nState < RDfa().NStates(); ++nState )
		{
			_TyGraphNode *	pgn = RDfa().PGNGetNode( nState );
			if ( pgn )
			{
				// Then check for a single odd numbered unsat out trans:
				_TyGraphLink *	pglUnsat = *pgn->PPGLChildHead();
				if (	pglUnsat && !*pglUnsat->PPGLBGetNextChild() &&
							( pglUnsat->RElConst() >= praiUnsatisfiable.first ) &&
							FVerifyInline( pglUnsat->RElConst() < praiUnsatisfiable.second ) &&
							!!( ( pglUnsat->RElConst() - praiUnsatisfiable.first ) % 2 ) )
				{
					// Then we have found the trailing context of a portion of the graph to be excised.
					_TyGraphNode *	pgnReconnect = pglUnsat->PGNChild();

					// We ignore all unsatisfiable transitions found that don't match pglUnsat->RElConst()-1 or pglUnsat->RElConst().

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
							if ( ssFoundNodes.isbitset( (size_t)pglEntered->PGNParent()->RElConst() ) )
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
							for ( ; !!pglFirst && ( pglFirst->RElConst() >= praiTriggers.first ) && ( pglFirst->RElConst() < praiTriggers.second ); 
										pglFirst = *pglFirst->PPGLGetNextChild() )
									;

							if (	!!pglFirst && ( pglFirst->RElConst() >= praiUnsatisfiable.first ) && 
										( pglFirst->RElConst() < praiUnsatisfiable.second ) )
							{
								// We don't expect to encounter any other unsatisfiables and we want to know even in release whether or not we do:
								VerifySz( ( ( pglFirst->RElConst() == pglUnsat->RElConst()-1 ) || ( pglFirst->RElConst() == pglUnsat->RElConst() ) ), 
									"pglFirst->RElConst()[%zu] pglUnsat->RElConst()[%zu]", size_t(pglFirst->RElConst()), size_t(pglUnsat->RElConst()) );
								// Then found one - this node to be deleted.
								Assert( !!stRemoved || ( gfit.PGNCur() == pgn ) ); // First time through we should remove the root.
								RDfa().m_nodeLookup[ (size_t)gfit.PGNCur()->RElConst() ] = 0;
								_TyGraphNode * pgnRemoved = gfit.PGNCur();
								++gfit;	// Move to next node or link.
								++stRemoved;
								if ( 1 != stRemoved )// We don't check on the first node because we know there is only one child.
								{
									// Now remove links for any children of pgnRemoved who enter into nodes which do not contain any of the unsatifiable transactions we are interested in.
									// The first easy check is to see if it enters into a node which has already been zeroed - in which it should not be removed.
									// Note that we needn't remove the link entirely - just remove it from connecting back to the non-pruned graph - deletion will correctly delete unconnected links.
									// First move past any unsatisfiable transitions:
									_TyGraphLink * pglNext = *pglFirst->PPGLGetNextChild();
									for (; !!pglNext; pglNext = *pglNext->PPGLGetNextChild())
									{
										_TyGraphNode * pgnCheck = pglNext->PGNChild();
										if ( !RDfa().m_nodeLookup[ (size_t)pgnCheck->RElConst() ] )
										{
											// points at already removed node - leave the link because we need the to-be-removed subgraph to be connected for deletion purposes.
											continue;
										}
										_TyGraphLink *	pglFirstCheck = *pgnCheck->PPGLChildHead();
										for (; !!pglFirstCheck && (pglFirstCheck->RElConst() >= praiTriggers.first) && (pglFirstCheck->RElConst() < praiTriggers.second);
											pglFirstCheck = *pglFirstCheck->PPGLGetNextChild())
										{
											// REVIEW<dbien>: Want to see this scenario if it happens. It's likely that the connection to the trigger will keep this "to be excised" portion of the graph connected to the main graph and cause crashing issues.
											Assert( false ); 
										}
										Assert( !!pglFirstCheck ); // REVIEW<dbien>:Want to see if this ever fires and check out that scenario - could just remove this.
										if ( !!pglFirstCheck && ( ( pglFirstCheck->RElConst() != pglUnsat->RElConst()-1 ) && ( pglFirstCheck->RElConst() != pglUnsat->RElConst() ) ) )
										{
											// Then pglNext connects back to the main graph and should be removed from it's child's parent list:
											pglNext->RemoveParent();
											pglNext->SetChildNode( nullptr );
											// We leave it unconnected since it will be pruned and deletion correctly deletes links with only one connection.
										} // if
									} // for
								} // if
							} // if
							else
							{
								Assert( pglEntered );
								Assert( pglEntered->PGNParent() == gfit.PGNCur() );

								// This node to remain in graph - record the state:
								ssFoundNodes.setbit( (size_t)gfit.PGNCur()->RElConst() );
								gfit.SkipContext();	// Skip to next context.
								// We can safely move the link:
								pglEntered->RemoveParent();
								pglEntered->InsertParent( pgnReconnect->PPGLParentHead() );
								pglEntered->SetChildNode( pgnReconnect );
								__DEBUG_STMT( pglEntered = 0 )
							}
						}
					} // while ( !gfit.FAtEnd() )...
					pglUnsat->RemoveParent();
					pglUnsat->SetChildNode( 0 );
					
					Assert( !RDfa().m_nodeLookup[ (size_t)pgn->RElConst() ] ); // If not then when would we zero it? We're deleting the node below.
					RDfa().m_gDfa.destroy_node( pgn );
				}
			}
		}

		if ( stRemoved )
		{
			CompressNodeLookup( stRemoved );
		}

		// Now, for complex "completed by" expressions - i.e. ones that are completed by a variable length series of characters
		//	as opposed to a fix length set of characters, we may have orphaned sections of the DFA which need removal.
		(void)_STCheckPruneAlternateRoots( true );
	}

	// Pull this method out because MSVC is endlessly looping during the link.
	void _GetConnectedSet( _TyGraphNode *	_pgnRoot, _TySetStates & _bvConnected, bool _fClosedDirected, bool _fDirectionDown )
	{
		Assert( _bvConnected.empty() );
		typename _TyGraph::iterator	gfit( _pgnRoot, 0, _fClosedDirected, _fDirectionDown, RDfa().get_allocator() );
		for ( ; !gfit.FAtEnd(); ++gfit )
		{
			if ( !gfit.PGLCur() ) // We want scearios where we are at a node and not at any link of that node.
				_bvConnected.setbit( (size_t)gfit.PGNCur()->RElConst() );
		}
	}

	// This will check for nodes that are orphaned from the main DFA and then remove appropriately.
	// Return the number of nodes removed thusly.
	size_t _STCheckRemoveOrphanedSubgraphs( bool _fCallCompressNodeLookup )
	{
		// If a node is not connected to the root node (node 0) then it needs removal, as well once we have found such a set of nodes we must
		//	then test for connectedness betwen that set to determine which set of nodes needs deletion.
		// We will check for such orphaned portions for interconnectivity amongst themselves so that we may appropriately detroy the set of
		//	orphaned subgraphs.
		// Algorithm:
		// 1) First pass: Start at the root node and accumulate a bit vector of every node we encounter along the way - this is the set of connected nodes.
		// 2) Then start with the first disconnected node and do essentially the same thing - move through and accumulate a bitvector of nodes connected to it.
		//	a) Zero this set of nodes and then delete destroy the subgraph at the first disconnected node.
		//	b) If there are more disconnected nodes left then go to (2) and repeat until there are no disconnected nodes left.
		_TySetStates bvDisconnected( (size_t)RDfa().NStates(), RDfa().get_allocator() );
		bvDisconnected.clear();
		// We will iterate both parents and children to find connected nodes, however we should
		//	be able to just iterate down. Iterating both directions correctly detects multiple roots - even though
		//	multiple roots are not currently supported we will account for it for completeness purposes (and because it is easy to do so).
		// This method is removing orphaned subgraphs - not inaccessible roots.
		{ //B
			_TyGraphNode *	pgnRoot = RDfa().PGNGetNode( 0 );
			VerifyThrowSz( !!pgnRoot, "No root node?!" );
			_GetConnectedSet( pgnRoot, bvDisconnected, false, true );
			bvDisconnected.invert(); // now disconnected set.
		} //EB

		size_t stRemoved = 0;
		while ( !bvDisconnected.empty() )
		{
			_TyGraphNode *	pgnRootSG;
			{//B
				typename _TySetStates::size_type stRootSG;
				_TySetStates bvCopyDisconnected( bvDisconnected );
				// Move through and find the first non-null node in bvCopyDisconnected:
				for ( stRootSG = bvCopyDisconnected.getclearfirstset();
							( bvCopyDisconnected.size() != stRootSG ) && !( pgnRootSG = RDfa().PGNGetNode( stRootSG ) );
							( stRootSG = bvCopyDisconnected.getclearfirstset( stRootSG ) ) )
					;
				Assert( pgnRootSG );
			}//EB
			VerifyThrow( !!pgnRootSG );
			_TySetStates bvConnectedSG( (size_t)RDfa().NStates(), RDfa().get_allocator() );
			bvConnectedSG.clear();
			_GetConnectedSet( pgnRootSG, bvConnectedSG, false, true );
			bvDisconnected.and_not_equals( bvConnectedSG );
			// Now zero all the nodes we found:
			for (typename _TySetStates::size_type stRemove = bvConnectedSG.getclearfirstset();
						bvConnectedSG.size() != stRemove;
						( stRemove = bvConnectedSG.getclearfirstset( stRemove ) ), ++stRemoved )
				RDfa().m_nodeLookup[ stRemove ] = 0;
			// Remove the root:
			RDfa().m_gDfa.destroy_node( pgnRootSG );
		}
		if ( _fCallCompressNodeLookup && stRemoved )
		{
			CompressNodeLookup( stRemoved );
		}
		return stRemoved;
	}

	// This will check for nodes that are inaccessible from the root node of the DFA in a childwise traversal.
	// These are extraneous nodes as they cannot participate in the lexical analysis. These can be created by the
	//	 "completed by" algorithm of inserting "unsatisifiable" pairs into the NFA.
	// This is similar to _STCheckRemoveOrphanedSubgraphs() but we must clip any connections between roots and the main graph.
	// Return the number of nodes removed thusly.
	size_t _STCheckPruneAlternateRoots( bool _fCallCompressNodeLookup )
	{
		// Algorithm:
		// 1) Mark all nodes accessible in a downward (childwise) closed iteration from the root node.
		//	a) All nodes not such marked are not required in the resultant DFA. They won't hurt anything but they are detritus.
		// 3) Then move through all detritus nodes and remove any children connecting to nodes which are not detritus.
		// 4) Then perform "removal procedure" which we used in _STCheckRemoveOrphanedSubgraphs() to detroy connected subgraphs among the nodes to be removed.
		_TySetStates bvDetritus( (size_t)RDfa().NStates(), RDfa().get_allocator() );
		bvDetritus.clear();
		{ //B
			_TyGraphNode *	pgnRoot = RDfa().PGNGetNode( 0 );
			VerifyThrowSz( !!pgnRoot, "No root node?!" );
			_GetConnectedSet( pgnRoot, bvDetritus, true, true );
			bvDetritus.invert(); // now disconnected set.
		} //EB

		{ //B
			_TySetStates bvDetritusCopy( bvDetritus );
			// Move through and find the first non-null node in bvDetritusCopy:
			for ( typename _TySetStates::size_type stDetritus = bvDetritusCopy.getclearfirstset();
						( bvDetritusCopy.size() != stDetritus );
						( stDetritus = bvDetritusCopy.getclearfirstset( stDetritus ) ) )
			{
				_TyGraphNode *	pgnDetritus = RDfa().PGNGetNode( stDetritus );
				Assert( !!pgnDetritus ); // We can deal with it not being populated but we expect it populated.
				if ( !!pgnDetritus )
				{
					_TyGraphLink *	pglCur = *(pgnDetritus->PPGLChildHead());
					for ( ; !!pglCur; pglCur = *pglCur->PPGLGetNextChild() )
					{
						_TyGraphNode * pgnCheck = pglCur->PGNChild();
						if ( !bvDetritus.isbitset( (size_t)pgnCheck->RElConst() ) ) // yes bvDetritus.
						{
							// pruning shears...
							pglCur->RemoveParent();
							pglCur->SetChildNode( nullptr );
						}
					}
				}
			}
		} //EB

		// Now we should just be able to call _STCheckRemoveOrphanedSubgraphs since we would have just created at least one orphaned subgraph:
		return _STCheckRemoveOrphanedSubgraphs( _fCallCompressNodeLookup );
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

			// Dead state is orphaned - but has a lot of children ( itself ) - assert that is the case:
#if ASSERTSENABLED
			{//B
				typename _TyGraph::_TyLinkPosIterConst	lpi( RDfa().PGNGetNode( 0 )->PPGLChildHead() );
				_TyGraphNode * pgnDead = RDfa().PGNGetNode( 0 );
				while( !lpi.FIsLast() )
					Assert( lpi.PGLCur()->PGNChild() == pgnDead );
			} //EB
#endif //ASSERTSENABLED

			RDfa().m_gDfa.destroy_node( RDfa().PGNGetNode( 0 ) );
			RDfa().m_nodeLookup[ 0 ] = 0;

			RDfa().m_fHasDeadState = false;
		}
	}

	void CompressNodeLookup( typename _TySetStates::size_type _stNonReps )
	{
    typename _TySetStates::size_type	stNewStates = (size_t)RDfa().NStates() - _stNonReps;

		// We should compress the state numbers and the node lookup in the DFA.
		// At the same time set up the new accepting state set.
		_TySetStates	ssNewAccept( stNewStates, RDfa().get_allocator() );
		ssNewAccept.clear();

		// Create the accept partition lookup in the dfa context:
		CreateAcceptPartLookup();

		// Create a map from a pointer to an accept partition element to
		//	the new parition for that accept state:
    typedef typename _Alloc_traits< typename map< typename _TyPartAcceptStates::value_type *, _TySwapSS,
                                                  less< typename _TyPartAcceptStates::value_type * > >::value_type, t_TyAllocator >::allocator_type _TyMapToNewAcceptAllocator;
		typedef map<	typename _TyPartAcceptStates::value_type *, _TySwapSS, 
						less< typename _TyPartAcceptStates::value_type * >,
            _TyMapToNewAcceptAllocator >	_TyMapToNewAccept;
		_TyMapToNewAccept mapToNewAccept( less< typename _TyPartAcceptStates::value_type * >(), RDfa().get_allocator() );

		// Move through and create the initial ( empty ) state sets:
		{ // SCOPE
			typename _TyPartAcceptStates::iterator	itPartEnd = m_partAccept.end();
			for ( typename _TyPartAcceptStates::iterator	it = m_partAccept.begin();
						it != itPartEnd; ++it )
			{
				typename _TyMapToNewAccept::value_type	vt( &(*it), _TySetStates( stNewStates, RDfa().get_allocator() ) );
				vt.second.RObject().clear();
				mapToNewAccept.insert( vt );
			}
		}
		
		_TyState iStartSpace = -1;
		_TyState iStartNS = -1;
		_TyState iProcessedEmpty = 0;
		_TyState iEmptyNodes = 0;
		_TyState iState;
		for ( iState = 0; iState < RDfa().NStates(); iState++ )
		{
			if ( RDfa().m_nodeLookup[ size_t( iState - iProcessedEmpty ) ] )
			{
				if ( iStartNS < 0 && iStartSpace >= 0 )
				{
					iStartNS = iState;
				}
				if ( m_pssAccept->isbitset( (size_t)iState ) )
				{
					ssNewAccept.setbit( size_t( iState - iEmptyNodes ) );

					// Set the state in the appropriate accept partition:
					typename _TyMapToNewAccept::iterator itPart = mapToNewAccept.find( 
																									PVTGetAcceptPart( iState ) );
					Assert( mapToNewAccept.end() != itPart );
					itPart->second.RObject().setbit( size_t( iState - iEmptyNodes ) );
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
						RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + size_t( iStartSpace - iProcessedEmpty ),
																				RDfa().m_nodeLookup.begin() + size_t( iStartNS - iProcessedEmpty ) );
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
			RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + size_t( iStartSpace - iProcessedEmpty ),
																	RDfa().m_nodeLookup.begin() + size_t( iStartNS - iProcessedEmpty ) );
		}
		else
		// If there was empty space at the end:
		if ( ( iStartNS < 0 ) && ( iStartSpace >= 0 ) )
		{
			RDfa().m_nodeLookup.erase(	RDfa().m_nodeLookup.begin() + size_t( iStartSpace - iProcessedEmpty ),
																	RDfa().m_nodeLookup.end() );
		}

		// Get rid of the lookup - it is at the old size:
		DeallocAcceptPartLookup();

		// Create the accept state partition from the old and then swap in:
		_TyPartAcceptStates	partAcceptNew( _TyCompareStates(), RDfa().get_allocator() );

		typename _TyMapToNewAccept::iterator itMapEnd = mapToNewAccept.end();
		for ( typename _TyMapToNewAccept::iterator	itMap = mapToNewAccept.begin();
					itMap != itMapEnd; ++itMap )
		{
			// Swap the bitvector in from <mapToNewAccept>:
			typename _TyPartAcceptStates::value_type	vt( itMap->second, itMap->first->second );
			pair< typename _TyPartAcceptStates::iterator, bool >	pibInsert = partAcceptNew.insert( vt );
			Assert( pibInsert.second );
		}

		m_partAccept.swap( partAcceptNew );

		// Swap in the new accept states:
		m_pssAccept->swap( ssNewAccept );

		// Set the number of states in the DFA:
		Assert( RDfa().m_nodeLookup.size() == stNewStates );
		RDfa().SetNumStates( stNewStates );

		RDfa()._ClearSSCache();	// Clear any SS cache's - they are at the old size.

		Assert( RDfa().m_nodeLookup.size() == m_pssAccept->size() );
	}

	// Trigger ambiguity:
	// At runtime a trigger transition is taken only when no character transition from the current state matches the
	//	current character - the lexical analyzer always prefers consuming a character to crossing a trigger. When the
	//	character transitions of a state and those of the state(s) reached by crossing its trigger(s) share a character
	//	the trigger is never crossed on that character. Often that loses nothing: for instance for ++( [a-z] <trigger> )
	//	every input matched by crossing the trigger early is matched just as well by not crossing it - the trigger just
	//	fires fewer times. But it can lose a match: for instance a* <trigger> a b against "aab" - the trigger must be
	//	crossed after the first 'a' but the analyzer consumes both 'a's and then can't match the 'b'.
	// This finds the latter: a state X, a character c that X consumes and that a state Y beyond X's trigger(s) also
	//	consumes, such that there is some input following c which, starting from Y, the analyzer accepts as a token but
	//	which, starting from X ( as the analyzer actually proceeds ), it doesn't accept or accepts as a different token.
	// We explore pairs of states ( alternative, actual ) moving forward in lock step on the elements of the alphabet,
	//	moving each as the analyzer does: by the state's character transition if it has one, else by following its trigger(s).
	// The alphabet must not have been compressed further than CompressTransitions() does - we use the alphabet's ranges.
	typedef typename _TyDfa::_TyRange _TyRange;
	typedef typename _TyDfa::_TyRangeEl _TyRangeEl;
	struct _TyTriggerAmbiguity
	{
		_TyState m_stState; // The state X: the analyzer consumes m_rngChar here.
		_TyState m_stStateTrigger; // The state Y beyond X's trigger(s) which also consumes m_rngChar.
		vtyTokenIdent m_tidTrigger; // The token id of the first trigger crossed from X - vktidInvalidIdToken if unknown.
		_TyRange m_rngChar; // The character(s) consumed by both.
		std::vector< _TyRangeEl > m_rgExample; // An example of the input following m_rngChar which is lost or changed.
	};
	void GetTriggerAmbiguities( std::vector< _TyTriggerAmbiguity > & _rrgta )
	{
		_rrgta.clear();
		_TyDfa & rDfa = RDfa();
		rDfa._CreateRangeLookup();
		const size_t knStates = (size_t)rDfa.NStates();
		auto fIsTrigger = []( _TyRange const & _rr ) { return ( _rr.first >= _TyDfa::ms_kreTriggerStart ) && ( _rr.first < _TyDfa::ms_kreUnsatisfiableStart ); };
		auto fIsChar = []( _TyRange const & _rr ) { return _rr.first < _TyDfa::ms_kreTriggerStart; };
		// The atoms of the alphabet: the character ranges in the alphabet. Each transition is on a range which is a union of atoms.
		std::vector< _TyRange > rgAtoms;
		for ( size_t n = 0; n < rDfa.AlphabetSize(); ++n )
		{
			_TyRange const & rr = rDfa.m_rgrngLookup[ n ];
			if ( fIsChar( rr ) && !( !rr.first && !rr.second ) )
				rgAtoms.push_back( rr );
		}
		const size_t knAtoms = rgAtoms.size();
		const int64_t kiNone = -1;
		// For each state: its character transition on each atom, its trigger transition and the token of the accepting state.
		std::vector< int64_t > rgCharNext( knStates * knAtoms, kiNone );
		std::vector< int64_t > rgTriggerNext( knStates, kiNone );
		std::vector< vtyTokenIdent > rgTriggerTid( knStates, vktidInvalidIdToken );
		std::vector< const void * > rgAccept( knStates, nullptr ); // The accept partition element - null if not accepting or anti-accepting.
		for ( auto itPart = m_partAccept.begin(); itPart != m_partAccept.end(); ++itPart )
		{
			const bool kfAnti = ( e_aatAntiAccepting == ( itPart->second.m_eaatType & ~e_aatTrigger ) );
			_TySetStates const & rss = itPart->first.RObject();
			for ( size_t st = rss.getfirstset(); rss.size() != st; st = rss.getnextset( st ) )
				if ( st < knStates )
					rgAccept[ st ] = kfAnti ? nullptr : &*itPart;
		}
		for ( size_t st = 0; st < knStates; ++st )
		{
			_TyGraphNode * pgn = rDfa.PGNGetNode( (_TyState)st );
			if ( !pgn )
				continue;
			typename _TyGraph::_TyLinkPosIterNonConst lpi( pgn->PPGLChildHead() );
			for ( ; !lpi.FIsLast(); lpi.NextChild() )
			{
				_TyRange r = rDfa.LookupRange( *lpi );
				const int64_t kiChild = (int64_t)lpi.PGNChild()->RElConst();
				if ( fIsTrigger( r ) )
				{
					rgTriggerNext[ st ] = kiChild;
					auto cit = rDfa.m_mapTriggerTransitionToTokenId.find( r.first );
					if ( rDfa.m_mapTriggerTransitionToTokenId.end() != cit )
						rgTriggerTid[ st ] = cit->second;
				}
				else
				if ( fIsChar( r ) )
				{
					for ( size_t a = 0; a < knAtoms; ++a )
						if ( ( rgAtoms[ a ].first >= r.first ) && ( rgAtoms[ a ].second <= r.second ) )
							rgCharNext[ st * knAtoms + a ] = kiChild;
				}
			}
		}
		// The state the analyzer moves to from _st on atom _a: the character transition of the first state along the
		//	chain of trigger transitions from _st that has one.
		auto iEffNext = [&]( int64_t _st, size_t _a ) -> int64_t
		{
			for ( size_t nSteps = 0; ( kiNone != _st ) && ( nSteps <= knStates ); ++nSteps, _st = rgTriggerNext[ (size_t)_st ] )
			{
				const int64_t kiNext = rgCharNext[ (size_t)_st * knAtoms + _a ];
				if ( kiNone != kiNext )
					return kiNext;
			}
			return kiNone;
		};
		// The token the analyzer finds when the input ends at _st: that of the last accepting state along the chain of
		//	trigger transitions - which the analyzer follows when no character transition matches.
		auto pvAcceptAt = [&]( int64_t _st ) -> const void *
		{
			const void * pv = nullptr;
			for ( size_t nSteps = 0; ( kiNone != _st ) && ( nSteps <= knStates ); ++nSteps, _st = rgTriggerNext[ (size_t)_st ] )
			{
				if ( m_pssAccept->isbitset( (size_t)_st ) )
					pv = rgAccept[ (size_t)_st ]; // null for an anti-accepting state: no token.
			}
			return pv;
		};
		// Explore from ( _stAlt, _stActual ): return true and the example if some input is lost or changed.
		std::unordered_map< uint64_t, std::pair< uint64_t, size_t > > mapVisited; // pair -> ( predecessor pair, atom ).
		std::vector< uint64_t > rgQueue;
		auto fFindLoss = [&]( int64_t _stAlt, int64_t _stActual, std::vector< _TyRangeEl > & _rrgExample ) -> bool
		{
			auto key = []( int64_t _p, int64_t _q ) { return ( uint64_t( _p ) << 32 ) | uint64_t( uint32_t( _q ) ); };
			mapVisited.clear();
			rgQueue.clear();
			const uint64_t kuStart = key( _stAlt, _stActual );
			mapVisited[ kuStart ] = { kuStart, 0 };
			rgQueue.push_back( kuStart );
			for ( size_t nQueue = 0; nQueue < rgQueue.size(); ++nQueue )
			{
				const uint64_t ku = rgQueue[ nQueue ];
				const int64_t kp = int64_t( ku >> 32 );
				const int64_t kq = ( uint32_t( ku ) == 0xffffffff ) ? kiNone : int64_t( uint32_t( ku ) );
				if ( kp == kq )
					continue; // Identical from here on.
				const void * pvAlt = pvAcceptAt( kp );
				if ( !!pvAlt && ( pvAlt != pvAcceptAt( kq ) ) )
				{
					// Lost or changed: reconstruct the example input.
					_rrgExample.clear();
					for ( uint64_t kuCur = ku; kuCur != kuStart; )
					{
						auto & rpr = mapVisited[ kuCur ];
						_rrgExample.push_back( rgAtoms[ rpr.second ].first );
						kuCur = rpr.first;
					}
					std::reverse( _rrgExample.begin(), _rrgExample.end() );
					return true;
				}
				for ( size_t a = 0; a < knAtoms; ++a )
				{
					const int64_t kpNext = iEffNext( kp, a );
					if ( kiNone == kpNext )
						continue; // The alternative can't continue - nothing to lose.
					const int64_t kqNext = ( kiNone == kq ) ? kiNone : iEffNext( kq, a );
					const uint64_t kuNext = key( kpNext, kqNext );
					if ( mapVisited.insert( { kuNext, { ku, a } } ).second )
						rgQueue.push_back( kuNext );
				}
			}
			return false;
		};
		for ( size_t stX = 0; stX < knStates; ++stX )
		{
			if ( kiNone == rgTriggerNext[ stX ] )
				continue;
			std::vector< bool > rgfVisited( knStates, false );
			rgfVisited[ stX ] = true;
			for ( int64_t stY = rgTriggerNext[ stX ]; ( kiNone != stY ) && !rgfVisited[ (size_t)stY ]; stY = rgTriggerNext[ (size_t)stY ] )
			{
				rgfVisited[ (size_t)stY ] = true;
				for ( size_t a = 0; a < knAtoms; ++a )
				{
					const int64_t kiActual = rgCharNext[ stX * knAtoms + a ];
					const int64_t kiAlt = rgCharNext[ (size_t)stY * knAtoms + a ];
					if ( ( kiNone == kiActual ) || ( kiNone == kiAlt ) )
						continue;
					_TyTriggerAmbiguity ta;
					if ( fFindLoss( kiAlt, kiActual, ta.m_rgExample ) )
					{
						ta.m_stState = (_TyState)stX;
						ta.m_stStateTrigger = (_TyState)stY;
						ta.m_tidTrigger = rgTriggerTid[ stX ];
						ta.m_rngChar = rgAtoms[ a ];
						_rrgta.push_back( std::move( ta ) );
					}
				}
			}
		}
	}

	// Return whether the triggers of this ( untagged ) DFA are transitions that the lexical analyzer is able to take:
	//	at most a single trigger leaves any state, a trigger doesn't leave an accepting state, etc.
	// These are the conditions that _l_generator<> requires of an untagged DFA. If not then _rstrWhy describes the
	//	first problem found. The transitions must not have been compressed.
	bool FCheckTriggerStructure( std::string & _rstrWhy )
	{
		_TyDfa & rDfa = RDfa();
		if ( !rDfa.m_nTriggers )
			return true;
		rDfa._CreateRangeLookup();
		const size_t knStates = (size_t)rDfa.NStates();
		typename _TyDfa::_TyPrAI praiTriggers;
		rDfa.GetTriggerUnsatAIRanges( &praiTriggers, nullptr );
		auto fIsTrigger = [&praiTriggers]( typename _TyDfa::_TyAlphaIndex _ai ) { return ( _ai >= praiTriggers.first ) && ( _ai < praiTriggers.second ); };
		std::vector< const typename _TyPartAcceptStates::value_type * > rgpvtAccept( knStates, nullptr );
		for ( auto itPart = m_partAccept.begin(); itPart != m_partAccept.end(); ++itPart )
		{
			_TySetStates const & rss = itPart->first.RObject();
			for ( size_t st = rss.getfirstset(); rss.size() != st; st = rss.getnextset( st ) )
				if ( st < knStates )
					rgpvtAccept[ st ] = &*itPart;
		}
		for ( size_t st = 0; st < knStates; ++st )
		{
			_TyGraphNode * pgn = rDfa.PGNGetNode( (_TyState)st );
			if ( !pgn )
				continue;
			typename _TyGraph::_TyLinkPosIterConst lpi( pgn->PPGLChildHead() );
			if ( lpi.FIsLast() || !fIsTrigger( *lpi ) )
				continue; // Any triggers are first.
			const typename _TyPartAcceptStates::value_type * pvtAccept = rgpvtAccept[ st ];
			const bool kfAccept = m_pssAccept->isbitset( st );
			const bool kfIsTriggerAction = !!pvtAccept && !!( pvtAccept->second.m_eaatType & e_aatTrigger );
			const bool kfIsAntiAccepting = !!pvtAccept && ( e_aatAntiAccepting == ( pvtAccept->second.m_eaatType & ~e_aatTrigger ) );
			const bool kfIsTriggerGateway = !( rDfa.LookupRange( *lpi ).first % 2 );
			const char * pszWhy = nullptr;
			if ( kfIsTriggerAction == kfIsTriggerGateway )
				pszWhy = "a trigger is crossed while another trigger is in the midst of being crossed";
			else
			if ( !kfIsTriggerAction && kfAccept && !kfIsAntiAccepting )
				pszWhy = "a trigger leaves an accepting state";
			else
			if ( kfIsAntiAccepting && !kfIsTriggerGateway )
				pszWhy = "a trigger is ambiguous with an excluded pattern";
			else
			if ( lpi.NextChild(), ( !lpi.FIsLast() && fIsTrigger( *lpi ) ) )
				pszWhy = "more than one trigger leaves a state";
			if ( pszWhy )
			{
				PrintfStdStr( _rstrWhy, "state %zu: %s", st, pszWhy );
				return false;
			}
		}
		return true;
	}

	// Return whether the lexical analyzer finds the same tokens, firing the triggers where the rules have them, by
	//	crossing the triggers of this ( untagged ) DFA as transitions. If not then a tagged DFA is required - see
	//	_l_dfatag.h - and _rstrWhy describes the first problem found. The transitions must not have been compressed.
	bool FTriggersUnambiguous( std::string & _rstrWhy )
	{
		if ( !RDfa().m_nTriggers )
			return true;
		if ( !FCheckTriggerStructure( _rstrWhy ) )
			return false;
		std::vector< _TyTriggerAmbiguity > rgta;
		GetTriggerAmbiguities( rgta );
		if ( rgta.empty() )
			return true;
		PrintfStdStr( _rstrWhy, "state %zu: the characters 0x%llx-0x%llx may be consumed before or after the trigger with token id %d and consuming them before it loses a match",
			(size_t)rgta[0].m_stState, (unsigned long long)rgta[0].m_rngChar.first, (unsigned long long)rgta[0].m_rngChar.second, (int)rgta[0].m_tidTrigger );
		return false;
	}

	// Compress accept paritions that correspond to the same trigger actions.
	// This allows the optimizer to generate more optimal DFA's - since these
	//	states have no reason ( after disambiguation ) to have differring action
	//	ids.
	void	CompressTriggerAcceptPartitions()
	{
  typedef typename _Alloc_traits< typename map< _ref< const _TyAcceptAction >, pair< typename _TyPartAcceptStates::iterator, _TySwapSS >,
                                                less< _ref< const _TyAcceptAction > > >::value_type, t_TyAllocator >::allocator_type _TySetTriggerPartsAllocator;
		typedef map<  _ref< const _TyAcceptAction >, pair< typename _TyPartAcceptStates::iterator, _TySwapSS >,
									less< _ref< const _TyAcceptAction > >, _TySetTriggerPartsAllocator >	_TySetTriggerParts;

		_TySetTriggerParts	setTriggerParts( typename _TySetTriggerParts::key_compare(), RDfa().get_allocator() );

		typename _TyPartAcceptStates::iterator	itPartEnd = m_partAccept.end();
		typename _TyPartAcceptStates::iterator	itPart = m_partAccept.begin();
		while ( itPartEnd != itPart )
		{
			typename _TyPartAcceptStates::value_type & rvtPart = *itPart;

			// Only compress single trigger actions:
			if (	( e_aatTrigger == rvtPart.second.m_eaatType ) &&
						!rvtPart.second.m_psrTriggers )
			{
				typename _TySetTriggerParts::iterator	itSTP;
				if ( setTriggerParts.end() == ( itSTP = setTriggerParts.find( rvtPart.second ) ) )
				{
					setTriggerParts.insert( typename _TySetTriggerParts::value_type( rvtPart.second,
											typename _TySetTriggerParts::mapped_type( itPart, _TySetStates( 0, RDfa().get_allocator() ) ) ) );
					++itPart;
				}
				else
				{
					// Already have this trigger action - update the set of states with that from
					//	this iterator - then remove this record.
					typename _TySetTriggerParts::value_type & rvtSTP = *itSTP;
					if ( rvtSTP.second.second.RObject().size() )
					{
						rvtSTP.second.second.RObject() |= rvtPart.first.RObject();
					}
					else
					{
						typename _TyPartAcceptStates::value_type & rvtFirst = *rvtSTP.second.first;
						_TySetStates ssCopyFirst( rvtFirst.first.RObject() );
						ssCopyFirst |= rvtPart.first.RObject();
						rvtSTP.second.second.RObject().swap( ssCopyFirst );
					}
					typename _TyPartAcceptStates::iterator itErase = itPart++;
					m_partAccept.erase( itErase );
				}
			}
			else
			{
				++itPart;
			}
		}

		// Now move through the compressed states updating:
		typename _TySetTriggerParts::iterator	itSTPEnd = setTriggerParts.end();
		typename _TySetTriggerParts::iterator	itSTP = setTriggerParts.begin();
		for ( ; itSTPEnd != itSTP; ++itSTP )
		{
			typename _TySetTriggerParts::value_type & rvtSTP = *itSTP;
			// If we compressed the trigger then update:
			if ( rvtSTP.second.second.RObject().size() )
			{
				typename _TyPartAcceptStates::value_type vtCopy( *rvtSTP.second.first );
				m_partAccept.erase( rvtSTP.second.first );
				const_cast< _TySetStates& >( vtCopy.first.RObject() ).
						swap( rvtSTP.second.second.RObject() );
#if ASSERTSENABLED
				pair< typename _TyPartAcceptStates::iterator, bool > pib =
#endif //ASSERTSENABLED
				m_partAccept.insert( vtCopy );
				Assert( pib.second );
			}
		}
	}


};

__REGEXP_END_NAMESPACE

#endif //__L_DFA_H

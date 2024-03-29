#ifndef __L_FABAS_H
#define __L_FABAS_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_fabas.h

// Base classes for finite automata.

#include <set>
#include <deque>
#include "_simpbv.h"
#include "jsonstrm.h"
#include "_l_ns.h"

__REGEXP_BEGIN_NAMESPACE

template < class t__TyNfa, class t__TyDfa > 
struct _create_dfa;

template < class t_TyDfa, bool f_tPartDeadImmed = true >
struct _optimize_dfa;

enum	EActionActionType
{
	e_aatNone = 0,
	e_aatAccept = 1,
		// A normal action ( i.e. doesn't involve lookahead.
	e_aatLookahead,
		// The non-accepting lookahead state. This is the accepting state of the follows
		//	NFA ( which is identical to the lookahead NFA ).
	e_aatLookaheadAccept,
		// The accepting lookahead state.
		// This is the intermediate state of the follows NFA.
	e_aatLookaheadAcceptAndAccept,
		// Both a lookahead accept state and an accept state. If the lookahead suffix
		//	is not seen then the action for this state is performed.
	e_aatLookaheadAcceptAndLookahead,
		// Both a lookahead accept state and a lookahead state.
	e_aatAntiAccepting,
		// This is an explicit non-accepting state - used for pattern exclusion.
	e_aatTrigger = 0x80000000,
		// Trigger modifier - this action is also a trigger ( may be just a trigger ).
};

template <	class t_TyActionIdent, class t_TySdpActionBase, class t_TyAllocator >
struct _fa_accept_action
{
private:
	typedef _fa_accept_action _TyThis;
public:

	typedef _simple_bitvec< vtyLookaheadVector, t_TyAllocator > _TySetActionIds;

	EActionActionType m_eaatType;
	t_TyActionIdent m_aiAction;
	_dtorp< t_TySdpActionBase > m_pSdpAction;
	t_TyActionIdent m_aiRelated;
	_sdpd< _TySetActionIds, t_TyAllocator >	m_psrRelated;	// If this is non-null then a bit-vector of related action identifiers.
	_sdpd< _TySetActionIds, t_TyAllocator >	m_psrTriggers;	// If this is non-null then a bit-vector of trigger action identifiers.

	_fa_accept_action() = delete;
	_fa_accept_action(	t_TyActionIdent _aiAction, 
						const t_TySdpActionBase * _pSdpAction, 
						t_TyAllocator const & _rAlloc = t_TyAllocator() )
		:	m_eaatType( e_aatAccept ),
			m_aiAction( _aiAction ),
			m_aiRelated( _aiAction ),
			m_psrRelated( _rAlloc ),
			m_psrTriggers( _rAlloc )
	{
		if ( !!_pSdpAction )
		{
			_pSdpAction->clone( &m_pSdpAction.PtrRef() );
		}
	}

	_fa_accept_action( _TyThis const & _r )
		:	m_eaatType( _r.m_eaatType ),
			m_aiAction( _r.m_aiAction ),
			m_aiRelated( _r.m_aiRelated ),
			m_psrRelated( _r.get_allocator() ),
			m_psrTriggers( _r.get_allocator() )
	{
		if ( !!_r.m_pSdpAction )
			_r.m_pSdpAction->clone( &m_pSdpAction.PtrRef() );
		if ( !!_r.m_psrRelated )
			m_psrRelated.template emplace< _TySetActionIds const & >( *_r.m_psrRelated );
		if ( !!_r.m_psrTriggers )
			m_psrTriggers.template emplace< _TySetActionIds const & >( *_r.m_psrTriggers );
	}
	void AssertValid() const
	{
#if ASSERTSENABLED
		if ( !!( e_aatTrigger & m_eaatType ) )
		{
			//Assert( !!m_pSdpAction ); // Should have an action.
			Assert( !m_psrTriggers );
			if ( !!m_psrTriggers )
				Assert( m_psrTriggers->countsetbits() );
		}
#endif //ASSERTSENABLED
	}

	t_TyActionIdent	GetOriginalActionId() const
	{
		return m_eaatType == e_aatLookahead ? m_aiAction : m_aiRelated;
	}


	t_TyAllocator get_allocator() const _BIEN_NOTHROW
	{
		return m_psrRelated.get_allocator();
	}

	bool operator < ( const _TyThis & _r ) const _BIEN_NOTHROW
	{
		if ( m_eaatType == _r.m_eaatType )
		{
			switch ( m_eaatType )
			{
				case e_aatTrigger:
				{
					if ( !!m_psrTriggers == !!_r.m_psrTriggers )
					{
						if ( !!m_psrTriggers )
						{
							return *m_psrTriggers < *_r.m_psrTriggers;
						}
						else
						{
							// For triggers we don't care about the action id - just the action:
							return **m_pSdpAction < **_r.m_pSdpAction;
						}
					}
					else
					{
						return !!m_psrTriggers < !!_r.m_psrTriggers;
					}
				}
				break;
				default:
				{
					// For other actions we merely compare action ids:
					Assert( m_aiAction != _r.m_aiAction );
					return m_aiAction < _r.m_aiAction;
				}
				break;
			}
		}
		else
		{
			return m_eaatType < _r.m_eaatType;
		}
	}
};

template < class t_TyChar, class t_TyAllocator >
class _fa_alloc_base : public _fa_base< t_TyChar >
{
private:
	typedef _fa_alloc_base< t_TyChar, t_TyAllocator >	_TyThis;
	typedef _fa_base< t_TyChar >				_TyBase;
protected:
	using _TyBase::m_iCurState;
public:

	// Friends:
	template < class t__TyNfa, class t__TyDfa > 
	friend struct _create_dfa;
	template < class t_TyDfa, bool f_tPartDeadImmed >
	friend struct _optimize_dfa;

	typedef t_TyAllocator	_TyAllocator;
	typedef typename _Alloc_traits< char, _TyAllocator >::size_type size_type;
	typedef typename _TyBase::_TyRange _TyRange;
	typedef typename _TyBase::_TySdpActionBase _TySdpActionBase;

	typedef less< _TyRange > _TyCompareRange;
	typedef typename _Alloc_traits< typename set < _TyRange, _TyCompareRange >::value_type, t_TyAllocator >::allocator_type _TyAlphabetAllocator;
	typedef set< _TyRange, _TyCompareRange, _TyAlphabetAllocator >	_TyAlphabet;

	typedef _fa_accept_action< vtyActionIdent, _TySdpActionBase, __L_DEFAULT_ALLOCATOR > _TyAcceptAction;

  // REVIEW: <dbien>: This is kind of bogus - shouldn't really 
  //  reference an internal impll class.
	typedef __DGRAPH_NAMESPACE _graph_node_base	_TyLexanGraphNodeBase;

  typedef typename _Alloc_traits< typename deque< _TyLexanGraphNodeBase * >::value_type, t_TyAllocator >::allocator_type _TyNodeLookupAllocator;
  typedef deque< _TyLexanGraphNodeBase *, _TyNodeLookupAllocator >	_TyNodeLookup;

	// Type for set of states:
	typedef _simple_bitvec< unsigned int, t_TyAllocator >		_TySetStates;

	// Type for state set cache:
  typedef typename _Alloc_traits< typename deque< _swap_object< _TySetStates > >::value_type, t_TyAllocator >::allocator_type _TySSCacheAllocator;
  typedef deque< _swap_object< _TySetStates >, _TySSCacheAllocator >	_TySSCache;

	_TyAlphabet	m_setAlphabet;

	_TyNodeLookup	m_nodeLookup;

	_TySSCache		m_ssCache;	// state set cache.
	unsigned int	m_uiUnusedCacheBitVec;
	static const int	ms_kiMaxSetCache = CHAR_BIT * sizeof( unsigned int );

	_fa_alloc_base( _fa_alloc_base const & ) = delete;
	_fa_alloc_base & operator = ( _fa_alloc_base const & ) = delete;
	~_fa_alloc_base() = default;

	_fa_alloc_base( t_TyAllocator const & _rAlloc = t_TyAllocator() )
		: _TyBase(),
			m_setAlphabet( _TyCompareRange(), _rAlloc ),
			m_nodeLookup( _rAlloc ),
			m_ssCache( _rAlloc ),
			m_uiUnusedCacheBitVec( 0 )
	{
	}

	t_TyAllocator get_allocator() const _BIEN_NOTHROW	{ return m_setAlphabet.get_allocator(); }

	void DumpStates( ostream & _ros, _TySetStates const & _r ) const
	{
		_ros << "{";
		bool	fYet = false;
		for ( typename _TySetStates::size_type stCur = 0;
					stCur < _r.size();
					++stCur )
		{
			if ( _r.isbitset( stCur ) )
			{
				if ( fYet )
				{
					_ros << ",";
				}
				else
				{
					fYet = true;
				}
				_ros << stCur;
			}
		}
		_ros << "}";
	}
	void DumpAlphabet( ostream & _ros ) const
	{
		// Dump the alphabet:
		_ros << "Alphabet : {";
		for (	typename _TyAlphabet::const_iterator it = m_setAlphabet.begin();
					it != m_setAlphabet.end();
					( ++it == m_setAlphabet.end() ) || ( _ros << "," ) )
		{
			_ros << *it;
		}
		_ros << "}.\n";
	}

	template < class t_tyJsonValueLife >
	void StatesToJSON( t_tyJsonValueLife & _jvl, _TySetStates const & _r ) const
	{
		Assert( _jvl.FAtArrayValue() );
		if ( _jvl.FAtArrayValue() )
		{
			for ( typename _TySetStates::size_type stCur = 0;
					stCur < _r.size(); ++stCur )
			{
				if ( _r.isbitset( stCur ) )
					_jvl.WriteValue( stCur );
			}
		}
	}
	template < class t_tyJsonOutputStream >
	void AlphabetToJSON( JsonValueLife< t_tyJsonOutputStream > & _jvl  ) const
	{
		Assert( _jvl.FAtArrayValue() );
		if ( _jvl.FAtArrayValue() )
		{
			for (	typename _TyAlphabet::const_iterator it = m_setAlphabet.begin(); it != m_setAlphabet.end(); ++it )
			{
				JsonValueLife< t_tyJsonOutputStream > jvl( _jvl, ejvtArray );
				it->ToJSONStream( jvl );
			}
		}
	}
	void AlphabetToJSON( JsonValueLifeAbstractBase< t_TyChar > & _jvl  ) const
	{
		Assert( _jvl.FAtArrayValue() );
		if ( _jvl.FAtArrayValue() )
		{
			for (	typename _TyAlphabet::const_iterator it = m_setAlphabet.begin();
						it != m_setAlphabet.end(); ++it )
			{
				std::unique_ptr< JsonValueLifeAbstractBase< t_TyChar > > pjvl;
				_jvl.NewSubValue( ejvtArray, pjvl );
				it->ToJSONStream( *pjvl );
			}
		}
	}

protected:

	void _ClearSSCache()
	{
		m_ssCache.clear();
		m_uiUnusedCacheBitVec = 0;
		Assert(	m_uiUnusedCacheBitVec == ( ( 1 << m_ssCache.size() ) - 1 ) );
	}

  // So a deque invalidates all iterators when a push_back or push_front is performed.
  // However the address of the elements of the deque, so they claim, is not changed.
  // We'll just record the index and then use that on release - makes it easy - and ignores if they reallocate the deque.
  // Hmmm: I am disappointed that deque iterators are invalidated by push...() - this is the major usage case and this is why you used deque.
  // Anyway the most typesafe way seems to be to just use the element point which is claimed to not change as long as you don't do such and such which I will not do (I never remove elements - it is a cache).
  size_t _STGetSSCache(_TySetStates *& _rpss )
	{
		if ( m_uiUnusedCacheBitVec )
		{
			size_t stUnused = _bv_get_clear_first_set( m_uiUnusedCacheBitVec );
			_rpss = &m_ssCache[stUnused].RObject();
			return stUnused;
		}
		else
		{
			// Need a new cache:
			Assert( m_ssCache.size()+1 < ms_kiMaxSetCache );
			_TySetStates ss( static_cast< size_t >( m_iCurState ), get_allocator() );
			m_ssCache.push_back( ss );
			_rpss = &m_ssCache.back().RObject();
			return m_ssCache.size()-1;
		}
	}

  // Provide the element to be released. It is an exception situation if the element is outside of the range of current elements.
	void _ReleaseSSCache(size_t _stRelease)
	{
		Assert(_stRelease < m_ssCache.size());
		Assert(!(m_uiUnusedCacheBitVec & (1 << _stRelease)));
		m_uiUnusedCacheBitVec |= ( 1 << _stRelease );
	}

	size_t _STUpdateNodeLookup( _TyLexanGraphNodeBase * _pgnb )
	{
		m_nodeLookup.push_back( _pgnb );
		return m_nodeLookup.size() - 1;
	}
};

__REGEXP_END_NAMESPACE

#endif //__L_FABAS_H

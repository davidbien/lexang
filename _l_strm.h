#pragma once

#include "_fdobjs.h"

// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

// Transports for the stream objects.

// _l_transport_base: base for all transports.
// Templatize by the character type contained in the file.
template < class t_TyChar >
class _l_transport_base
{
  typedef _l_transport_base _TyThis;
public:
  typedef t_TyChar _TyChar;
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar > _TyValue;
};

// _l_transport_fd:
// Transport using a file descriptor.
template < class t_TyChar >
class _l_transport_fd : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_fd _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;

  _l_transport_fd( const char * _pszFileName )
  {
    errno = 0;
    int fd = open( _pszFileName, O_RDONLY );
    if ( -1 == fd )
      THROWNAMEDEXCEPTIONERRNO( errno, "_l_transport_fd::_l_transport_fd(): open of [%s] failed.", _pszFileName );
    m_fd = fd;
    m_fOwnFd = true;
  }
  // Attach to an fd like STDIN - in which case you would set _fUseSeek to false and _fOwnFd to false.
  _l_transport_fd( int _fd, bool _fUseSeek = false, bool _fOwnFd = false )
    : m_fd( _fd)
      m_fOwnFd( _fOwnFd ),
      m_fUseSeek( _fUseSeek )
  {
  }
  ~_l_transport_fd()
  {
    if ( m_fOwnFd && ( -1 != m_fd ) )
    {
      int fd = m_fd;
      m_fd = -1;
      errno = 0;
      int iCloseResult = close( fd );
      Assert( !iCloseResult );
      if ( -1 == iCloseResult )
        LOGSYSLOGERRNO( eslmtError, errno, "~_l_transport_fd(): close() returned an error.")
    }
  }
  // No need to provide a bunch of utility methods here...

  void 

protected:
  off_t m_posAttached; // The position the fd was at when we were attached to it.
  int m_fd{-1};
  bool m_fOwnFd{false};
  bool m_fUseSeek{true}; // For STDIN, or a pipe (for instance) we wouldn't use seek and we then need to convert the token data to strings before deallocating the token data itself (which we then of course must also save...)
};

// _l_transport_pv:
// Transport that uses a piece of memory.
template < class t_TyChar >
class _l_transport_pv : public _l_transport_base< t_TyChar >
{
  typedef _l_transport_pv _TyThis;
  typedef _l_transport_base< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;
};

// _l_transport_mapped
// Transport that uses mapped memory.
template < class t_TyChar >
class _l_transport_mapped : public _l_transport_pv< t_TyChar >
{
  typedef _l_transport_mapped _TyThis;
  typedef _l_transport_pv< t_TyChar > _TyBase;
public:
  using _TyBase::_TyChar;
  using _TyBase::_TyData;
  using _TyBase::_TyValue;
};

// _l_stream: An input stream for the lexicographical analyzer.
template < class t_TyTransport, class t_TyChar >
class _l_stream
{
typedef _l_stream _TyThis;
public:
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar > _TyValue;

  _l_stream() = default;

  // Construct the transport object appropriately.
  template < class... t_tysArgs >
  _tyT & emplaceTransport( t_tysArgs&&... _args )
  {
    m_opttpImpl.emplace( std::forward< t_tysArgs >( _args )... );
  }

  // Read the Convert the strings present in this _TyValue in-place to the character type t_tyCharConvertTo.
  template < class t_tyCharConvertTo >
  void ReadAndConvertStrings( _TyValue & _rv )
  {
    
  }

protected:
  typedef optional< t_TyTransport > _tyOptTransport;
  _tyOptTransport m_opttpImpl; // The "transport" that implements the stream. Make it optional so that the user can emplace construct the stream after declaration of an _l_stream object.
  _TyChar m_posCur; // The current position in the stream. This is kept maintained with any transports that also have a current position.
};

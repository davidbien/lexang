#ifndef __L_CHRTR_H__
#define __L_CHRTR_H__

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _l_chrtr.h

// Character traits for lexical analyzer.

#include <limits.h>
#include <stdint.h>

__REGEXP_BEGIN_NAMESPACE

template < class t_TyCharType >
struct _l_char_type_map;

template <>
struct _l_char_type_map< char >
{
  typedef char          _TyChar;
  typedef unsigned char _TyUnsigned;
  typedef signed char   _TySigned;
  typedef short         _TyLarger;

  static constexpr char ms_kcMin = CHAR_MIN;
  static constexpr char ms_kcMax = CHAR_MAX;

  static constexpr char ms_kcSurrogateFirst = 0;
  static constexpr char ms_kcSurrogateLast = 0;
  static constexpr bool ms_kfHasSurrogates = false; // no surrogates present in UTF-8 or we at least are not checking for them.
};

template <>
struct _l_char_type_map< unsigned char >
{
  typedef char            _TyChar;
  typedef unsigned char   _TyUnsigned;
  typedef signed char     _TySigned;
  typedef unsigned short  _TyLarger;

  static constexpr unsigned char ms_kcMin = 0;
  static constexpr unsigned char ms_kcMax = UCHAR_MAX;

  // Provide a large set of triggers and unsatisifiable transitions:
  static constexpr _TyLarger ms_knTriggerStart = ms_kcMax + 1;
  static constexpr _TyLarger ms_knTriggerLast = ( USHRT_MAX - ( ms_knTriggerStart ) ) / 2; // inclusive.
  static constexpr _TyLarger ms_knUnsatisfiableStart = ms_knTriggerLast + 1;
  static constexpr _TyLarger ms_knUnsatisfiableLast = USHRT_MAX;
};

// REVIEW: <dbien>: Interesting that this override is here - it doesn't define ms_knTrigger or ms_knUnsatisfiableStart so it isn't used.
// Removing for now.
#if 0
template <>
struct _l_char_type_map< signed char >
{
  typedef char          _TyChar;
  typedef unsigned char _TyUnsigned;
  typedef signed char   _TySigned;
  typedef signed short  _TyLarger;

  static constexpr signed char ms_kcMin = SCHAR_MIN;
  static constexpr signed char ms_kcMax = SCHAR_MAX;
};
#endif //0

#if 1 //def WIN32
template <>
struct _l_char_type_map< wchar_t >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef int32_t       _TySigned;
  static_assert( sizeof( _TySigned ) == sizeof ( _TyUnsigned ) );
  typedef uint64_t      _TyLarger;
  static_assert( sizeof( _TyLarger ) > sizeof ( _TyUnsigned ) );

#if 0 // These are defined incorrectly/differently under gcc, use USHRT_MAX
  static constexpr wchar_t ms_kcMin = WCHAR_MIN;
  static constexpr wchar_t ms_kcMax = WCHAR_MAX;
#else //0
  // REVIEW: <dbien>: Note that this is incorrect for windows.
#ifdef WIN32
#error Under Windows wchar_t is an unsigned short so these constants are wrong.
#endif //WIN32
  static constexpr wchar_t ms_kcMin = 0;
  static constexpr wchar_t ms_kcMax = 0x10FFFF;
#endif //1

  // Provide a large set of triggers and unsatisifiable transitions:
  static constexpr _TyLarger ms_knTriggerStart = 0x0200000;
  static constexpr _TyLarger ms_knTriggerLast = ( UINT64_MAX - ( ms_knTriggerStart + 1 ) ) / 2; // inclusive.
  static constexpr _TyLarger ms_knUnsatisfiableStart = ms_knTriggerLast + 1;
  static constexpr _TyLarger ms_knUnsatisfiableLast = UINT64_MAX;

  static constexpr wchar_t ms_kcSurrogateFirst = 0xd800;
  static constexpr wchar_t ms_kcSurrogateLast = 0xdfff;
  static constexpr bool ms_kfHasSurrogates = true;
};
#if 1
template <>
struct _l_char_type_map< int32_t >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef int32_t       _TySigned;
  static_assert( sizeof( _TySigned ) == sizeof ( _TyUnsigned ) );
  typedef int64_t      _TyLarger;
  static_assert( sizeof( _TyLarger ) > sizeof ( _TyUnsigned ) );

  // static constexpr int32_t ms_kcMin = INT32_MIN;
  // static constexpr int32_t ms_kcMax = INT32_MAX;
};
#else //0
template <>
struct _l_char_type_map< signed short >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef signed short  _TySigned;
  typedef signed long   _TyLarger;

  static constexpr signed short ms_kcMin = SHRT_MIN;
  static constexpr signed short ms_kcMax = SHRT_MAX;
};

template <>
struct _l_char_type_map< signed int >
{
  typedef unsigned int      _TyChar;
  typedef unsigned int      _TyUnsigned;
  typedef signed int        _TySigned;
	typedef signed long long  _TyLarger;

  static constexpr signed int ms_kcMin = INT_MIN;
  static constexpr signed int ms_kcMax = INT_MAX;
};

template <>
struct _l_char_type_map< unsigned int >
{
  static constexpr unsigned int ms_kcMin = 0;
  static constexpr unsigned int ms_kcMax = UINT_MAX;

  typedef unsigned int        _TyChar;
  typedef unsigned int        _TyUnsigned;
  typedef signed int          _TySigned;
	typedef unsigned long long  _TyLarger;

#if 0 // This needs to be worked on - could just use a ulong - only need 3 non-characters
  static constexpr _TyLarger ms_knTrigger = UINT_MAX - 2;
  static constexpr _TyLarger ms_knUnsatisfiableStart = UINT_MAX - 1;
#endif //0
};
#endif //0
#else // LINUX/MAC
template <>
struct _l_char_type_map< wchar_t >
{
  typedef wchar_t _TyChar;
  typedef wchar_t _TyUnsigned;
  typedef int32_t _TySigned;
  static_assert( sizeof( _TySigned ) == sizeof ( _TyUnsigned ) );
  typedef uint64_t _TyLarger;

#if 0 // These are defined incorrectly/differently under gcc, use UINT32_MAX (I thikn they are signed under linux).
  static constexpr wchar_t ms_kcMin = WCHAR_MIN;
  static constexpr wchar_t ms_kcMax = WCHAR_MAX;
#else //0
  static constexpr wchar_t ms_kcMin = 0;
  static constexpr wchar_t ms_kcMax = UINT32_MAX;
#endif //0

  static constexpr _TyLarger ms_knTrigger = UINT32_MAX + 1;
  static constexpr _TyLarger ms_knUnsatisfiableStart = UINT32_MAX + 2;
};
template <>
struct _l_char_type_map< int32_t >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef int32_t       _TySigned;
  typedef int64_t   _TyLarger;

  static constexpr int32_t ms_kcMin = INT32_MIN;
  static constexpr int32_t ms_kcMax = INT32_MAX;
};
#endif

__REGEXP_END_NAMESPACE

#endif //__L_CHRTR_H__

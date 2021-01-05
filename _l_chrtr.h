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

template <>
struct _l_char_type_map< char32_t >
{
  typedef char32_t       _TyChar;
  typedef char32_t       _TyUnsigned;
  typedef int32_t       _TySigned;
  static_assert( sizeof( _TySigned ) == sizeof ( _TyUnsigned ) );
  typedef uint64_t      _TyLarger;
  static_assert( sizeof( _TyLarger ) > sizeof ( _TyUnsigned ) );

  static constexpr char32_t ms_kcMin = 0;
  static constexpr char32_t ms_kcMax = 0x10FFFF;

  // Provide a large set of triggers and unsatisifiable transitions:
  static constexpr _TyLarger ms_knTriggerStart = 0x0200000;
  static constexpr _TyLarger ms_knTriggerLast = ( UINT64_MAX - ( ms_knTriggerStart + 1 ) ) / 2; // inclusive.
  static constexpr _TyLarger ms_knUnsatisfiableStart = ms_knTriggerLast + 1;
  static constexpr _TyLarger ms_knUnsatisfiableLast = UINT64_MAX;

  static constexpr char32_t ms_kcSurrogateFirst = 0xd800;
  static constexpr char32_t ms_kcSurrogateLast = 0xdfff;
  static constexpr bool ms_kfHasSurrogates = true;
};

#if 0
template <>
struct _l_char_type_map< int32_t >
{
  typedef char32_t       _TyChar;
  typedef char32_t       _TyUnsigned;
  typedef int32_t       _TySigned;
  static_assert( sizeof( _TySigned ) == sizeof ( _TyUnsigned ) );
  typedef int64_t      _TyLarger;
  static_assert( sizeof( _TyLarger ) > sizeof ( _TyUnsigned ) );

  // static constexpr int32_t ms_kcMin = INT32_MIN;
  // static constexpr int32_t ms_kcMax = INT32_MAX;
};
#endif //0

__REGEXP_END_NAMESPACE

#endif //__L_CHRTR_H__

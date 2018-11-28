#ifndef __L_CHRTR_H__
#define __L_CHRTR_H__

// _l_chrtr.h

// Character traits for lexical analyzer.

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

  static const char ms_kcMin = CHAR_MIN;
  static const char ms_kcMax = CHAR_MAX;
};

template <>
struct _l_char_type_map< unsigned char >
{
  typedef char            _TyChar;
  typedef unsigned char   _TyUnsigned;
  typedef signed char     _TySigned;
  typedef unsigned short  _TyLarger;

  static const unsigned char ms_kcMin = 0;
  static const unsigned char ms_kcMax = UCHAR_MAX;

  static const _TyLarger ms_kucTrigger = UCHAR_MAX + 1;
  static const _TyLarger ms_kucUnsatisfiableStart = UCHAR_MAX + 2;
};

template <>
struct _l_char_type_map< signed char >
{
  typedef char          _TyChar;
  typedef unsigned char _TyUnsigned;
  typedef signed char   _TySigned;
  typedef signed short  _TyLarger;

  static const signed char ms_kcMin = SCHAR_MIN;
  static const signed char ms_kcMax = SCHAR_MAX;
};

template <>
struct _l_char_type_map< wchar_t >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef signed short  _TySigned;
  typedef unsigned long _TyLarger;

#if 0 // These are defined incorrectly/differently under gcc, use USHRT_MAX
  static const wchar_t ms_kcMin = WCHAR_MIN;
  static const wchar_t ms_kcMax = WCHAR_MAX;
#else 0
  static const wchar_t ms_kcMin = 0;
  static const wchar_t ms_kcMax = USHRT_MAX;
#endif 0

  static const _TyLarger ms_kucTrigger = USHRT_MAX + 1;
  static const _TyLarger ms_kucUnsatisfiableStart = USHRT_MAX + 2;
};
template <>
struct _l_char_type_map< signed short >
{
  typedef wchar_t       _TyChar;
  typedef wchar_t       _TyUnsigned;
  typedef signed short  _TySigned;
  typedef signed long   _TyLarger;

  static const signed short ms_kcMin = SHRT_MIN;
  static const signed short ms_kcMax = SHRT_MAX;
};

template <>
struct _l_char_type_map< signed int >
{
  typedef unsigned int      _TyChar;
  typedef unsigned int      _TyUnsigned;
  typedef signed int        _TySigned;
	typedef signed long long  _TyLarger;

  static const signed int ms_kcMin = INT_MIN;
  static const signed int ms_kcMax = INT_MAX;
};

template <>
struct _l_char_type_map< unsigned int >
{
  static const unsigned int ms_kcMin = 0;
  static const unsigned int ms_kcMax = UINT_MAX;

  typedef unsigned int        _TyChar;
  typedef unsigned int        _TyUnsigned;
  typedef signed int          _TySigned;
	typedef unsigned long long  _TyLarger;

#if 0 // This needs to be worked on - could just use a ulong - only need 3 non-characters
  static const _TyLarger ms_kucTrigger = UINT_MAX - 2;
  static const _TyLarger ms_kucUnsatisfiableStart = UINT_MAX - 1;
#endif 0
};

__REGEXP_END_NAMESPACE

#endif __L_CHRTR_H__

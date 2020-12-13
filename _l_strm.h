#pragma once

// _l_strm.h
// Stream objects for use with the lexical analyzer objects.
// dbien
// 13DEC2020

template < class t_TyChar >
class _l_stream
{
typedef _l_stream _TyThis;
public:
  typedef _l_data< t_TyChar > _TyData;
  typedef _l_value< t_TyChar
  _l_stream() = default;

  template < class t_tyCharConvertTo >
  void ConvertStrings( _TyValue & _rv )
  {
    Assert( _rv.F
  }
protected:

};
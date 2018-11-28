#ifndef __L_NS_H___
#define __L_NS_H___

// _l_ns.h

// Lexical analyzer namespace stuff.

#include "bienutil/bienutil.h"

// Choose namespace:
#if !defined( __STL_USE_NAMESPACES ) && !defined( __REGEXP_USE_NAMESPACE )
#define __REGEXP_GLOBALNAMESPACE
#endif

#ifdef __REGEXP_GLOBALNAMESPACE
#define __REGEXP_BEGIN_NAMESPACE
#define __REGEXP_END_NAMESPACE
#define __REGEXP_USING_NAMESPACE
#define __REGEXP_NAMESPACE
#else __REGEXP_GLOBALNAMESPACE
#ifndef __REGEXP_USE_NAMESPACE
#define __REGEXP_USE_NAMESPACE ns_re
#endif __REGEXP_USE_NAMESPACE
#define __REGEXP_BEGIN_NAMESPACE namespace __REGEXP_USE_NAMESPACE { __BIENUTIL_USING_NAMESPACE
#define __REGEXP_END_NAMESPACE }
#define __REGEXP_USING_NAMESPACE using namespace __REGEXP_USE_NAMESPACE;
#define __REGEXP_NAMESPACE __REGEXP_USE_NAMESPACE::
#endif __REGEXP_GLOBALNAMESPACE

#if 0  // always put the regexp ops in a namespace
#if !defined( __STL_USE_NAMESPACES ) && !defined( __REGEXP_OP_USE_NAMESPACE )
#error here
//#error Can't put the regular expression operators in the global namespace.
#endif !__STL_USE_NAMESPACES
#endif 0

#ifndef __REGEXP_OP_USE_NAMESPACE
#define __REGEXP_OP_USE_NAMESPACE ns_re_op
#endif __REGEXP_OP_USE_NAMESPACE
#define __REGEXP_OP_BEGIN_NAMESPACE namespace __REGEXP_OP_USE_NAMESPACE { __BIENUTIL_USING_NAMESPACE __REGEXP_USING_NAMESPACE
#define __REGEXP_OP_END_NAMESPACE }
#define __REGEXP_OP_USING_NAMESPACE using namespace __REGEXP_OP_USE_NAMESPACE;
#define __REGEXP_OP_NAMESPACE __REGEXP_OP_USE_NAMESPACE::

#if !defined( __STL_USE_NAMESPACES ) && !defined( __LEXOBJ_USE_NAMESPACE )
#define __REGEXP_GLOBALNAMESPACE
#endif

#ifdef __LEXOBJ_GLOBALNAMESPACE
#define __LEXOBJ_BEGIN_NAMESPACE
#define __LEXOBJ_END_NAMESPACE
#define __LEXOBJ_USING_NAMESPACE
#define __LEXOBJ_NAMESPACE
#else __LEXOBJ_GLOBALNAMESPACE
#ifndef __LEXOBJ_USE_NAMESPACE
#define __LEXOBJ_USE_NAMESPACE ns_lxo
#endif __LEXOBJ_USE_NAMESPACE
#define __LEXOBJ_BEGIN_NAMESPACE namespace __LEXOBJ_USE_NAMESPACE { __BIENUTIL_USING_NAMESPACE
#define __LEXOBJ_END_NAMESPACE }
#define __LEXOBJ_USING_NAMESPACE using namespace __LEXOBJ_USE_NAMESPACE;
#define __LEXOBJ_NAMESPACE __LEXOBJ_USE_NAMESPACE::
#endif __LEXOBJ_GLOBALNAMESPACE

#ifndef __L_DEFAULT_ALLOCATOR
#define __L_DEFAULT_ALLOCATOR	allocator< char >
#endif __L_DEFAULT_ALLOCATOR

#endif __L_NS_H___

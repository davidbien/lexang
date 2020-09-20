# lexang
### A set of templates that allow generation of a lexicographical analyzer given a set of regular expressions.
### Meant to be somewhat of a replacement for the good old unix lex command.

## Mechanism:
Currently the regular expression are written in C++ code via overridden global operators. Compilation generates the type structure that describes a given regular expression. A set of such regular expressions are converted into an NFA (nondiscrete finite automata) and then this NFA is convered to a DFA and optionally optimized to the smallest possible DFA.

## Regular Expressions:
### Regular expressions are represented using a namespace in which several global operators are overridden.
#### Literals:
  **Single character:** Represented by the method **literal(char)**.  
  Example: **literal('x')** matches the character **'x'**, **literal(L'x')** matches the wchar_t **'x'**.  
  **String of characters:** Represented by the method **litstr(char*)**.  
  Example: **litstr("string")** matches the string **"string"**, **litstr(L"string")** matches the wchar_t **"string"**.  
  **Range of characters:** Represented by the method **litrange(char,char)**.  
  Example: **litrange('a','z')** matches any character in the range 'a' to 'z' inclusive. 
#### Unary operations:
  **Zero or more**: Represented by **operator ~( regexp )**.  
  Example: **~literal('a')** will match "", "a", "aa", etc.  
  **One or more**: Represented by **operator ++( regexp )**.  
  Example: **++literal('a')** will match "a", "aa", "aaa", etc.  
  **Zero or one**: Represented by **operator --( regexp )**.  
  Example: **--literal('a')** will match "" and "a" only.  
#### Binary operations:
  **Follows**: Represented by **operator \*( regexp, regexp )**.  
  Example: **literal('a') * literal('b')** will match "ab".  
  **Or**: Represented by the bitwise OR **operator |( regexp, regexp )**.  
  Example: **literal('a') | literal('b')** will match "a" or "b".  
#### Special operations:
  **Excludes**: Represented by **operator -( regexp, regexp )**.  
  Example: **++literal('a') * ~literal('b') - litstr('ab')** will match any number of 'a's followed by 0 or more 'b's except for the string "ab".  
  **Completes**: Represented by the **operator +( regexp, regexp )**.  
  Example: **++litrange('a','z') + 'z'** will match any sequence of characters 'a' - 'z' but will stop at the first encountered 'z' character.  
  **Lookahead**: Represented by **operator /( regexp, regexp )**.  
  Example: **++litrange('a','z') / litstr("--")** will match any sequence of characters 'a' to 'z' when followed by the string "--", not including the "--".

### Complex example of regular expression usage:
This encodes the start of the XML regular expressions as specified some years ago (like 20 years ago).
<pre>
typedef wchar_t	_TyCharTokens;
typedef _TyDefaultAllocator  _TyAllocator;
typedef _regexp_final< _TyCharTokens, _TyAllocator >		_TyFinal;
typedef _regexp_trigger< _TyCharTokens, _TyAllocator >	_TyTrigger;
#define l(x)	literal< _TyCharTokens >(x)
#define ls(x)	litstr< _TyCharTokens >(x)
#define lr(x,y)	litrange< _TyCharTokens >(x,y)

_TyFinal Char =	l(0x09) | l(0x0a) | l(0x0d) | lr(0x020,0xd7ff) | lr(0xe000,0xfffd); // [2].
_TyFinal S = ++( l(0x20) | l(0x09) | l(0x0d) | l(0x0a) ); // [3]
_TyFinal Eq = --S * l(L'=') * --S; // [25].
_TyFinal BaseChar = lr(0x0041,0xd7a3);	// [85].
_TyFinal Ideographic = lr(0x4e00,0x9fa5) | l(0x3007) | lr(0x3021,0x3029); // [86]
_TyFinal CombiningChar = lr(0x0300,0x309a);	// [87]
_TyFinal Digit = lr(0x0030,0x0039) | lr(0x0660,0x0669); // [88]
_TyFinal Extender = l(0x00b7) | l(0x02d0);	// [89].
_TyFinal Letter = BaseChar | Ideographic;	// [84].
_TyFinal NameChar = Letter | Digit | l(L'.') | l(L'-') | l(L'_') | l(L':') | CombiningChar | Extender; // [4]

_TyFinal Name = ( Letter | l(L'_') | l(L':') ) * ~NameChar;	// [5]
_TyFinal PITarget = Name - ( ( ( l(L'x') | l(L'X') ) * ( l(L'm') | l(L'M') ) * ( l(L'l') | l(L'L') ) ) );
_TyFinal NCNameChar = Letter | Digit | l(L'.') | l(L'-') | l(L'_') | CombiningChar | Extender;	// namespace support
_TyFinal NCName = ( Letter | l(L'_') ) * ~NCNameChar;
_TyFinal Prefix = NCName;
_TyFinal LocalPart = NCName;

_TyFinal QName = Prefix * --( l(L':') * LocalPart );
_TyFinal DefaultAttName = ls(L"xmlns") * t( _TyAction104() );
_TyFinal PrefixedAttName = ls(L"xmlns:") * t( _TyAction105() ) * NCName * t( _TyAction117() );
_TyFinal NSAttName = PrefixedAttName | DefaultAttName;
_TyFinal EntityRef = l(L'&') * Name * l(L';'); // [49]
_TyFinal CharRef = ls(L"&#") * ++lr(L'0',L'9') * l(L';') 
                 | ls(L"&#x") * ++( lr(L'0',L'9') | lr(L'A',L'F') | lr(L'a',L'f') ) * l(L';'); // [66]
_TyFinal Reference = EntityRef | CharRef;	// [67]
_TyFinal AVCharNoAmperLessDouble = l(0x09) | l(0x0a) | l(0x0d) |	// Char - '&' - '<' - '"'
                                   lr(0x020,0x021) | lr(0x023,0x025) | lr(0x027,0x03b) | lr(0x03d,0xd7ff) 
                                   | lr(0xe000,0xfffd);
_TyFinal AVCharNoAmperLessSingle = l(0x09) | l(0x0a) | l(0x0d) |	// Char - '&' - '<' - '\''
                                   lr(0x020,0x025) | lr(0x028,0x03b) | lr(0x03d,0xd7ff) 
                                   | lr(0xe000,0xfffd);
_TyFinal AttValue = l(L'\"') * ~( AVCharNoAmperLessDouble | Reference ) * l(L'\"')	// [10]
                  | l(L'\'') * ~( AVCharNoAmperLessSingle | Reference ) * l(L'\'');
_TyFinal Attribute = NSAttName * Eq * AttValue // [41]
                   | QName * Eq * AttValue;

_TyFinal PI = ls(L"<?")	* PITarget * ( ls(L"?>") | ( S * ( ~Char + ls(L"?>") ) ) );
_TyFinal CharNoMinus =	l(0x09) | l(0x0a) | l(0x0d) // [2].
                     | lr(0x020,0x02c) | lr(0x02e,0xd7ff) | lr(0xe000,0xfffd);
_TyFinal Comment = ls(L"<!--") * ~( CharNoMinus | ( l(L'-') * CharNoMinus ) ) * ls(L"-->");
_TyFinal MixedBegin = l(L'(') * --S * ls(L"#PCDATA") * t( _TyAction25() );
_TyFinal Mixed = MixedBegin * ~( --S * l(L'|') * --S * Name ) * --S * ls(L")*") |
                 MixedBegin * --S * l(L')'); // [51].
</pre>
  



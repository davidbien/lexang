# Lexical Analyzer Generator Template Library
### A set of templates that allow generation of a lexical analyzer given a set of regular expressions.  
### Meant to be somewhat of a replacement for the good old unix lex command.
Note that this project depends on the repository [https://github.com/davidbien/dgraph](https://github.com/davidbien/dgraph) and [https://github.com/davidbien/bienutil](https://github.com/davidbien/bienutil).

## Mechanism:
The regular expression are written in C++ code via overridden global operators declared in a namespace. Compilation generates the type structure that describes a given regular expression. A set of such regular expressions are converted into an NFA (nondiscrete finite automata) and then this NFA is converted to a DFA (discrete finite automata) and optionally optimized to the smallest possible DFA.  
Most if not all of the algorithms I gleaned from the Dragon Book [https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools](https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools).

## Regular Expressions:
### Regular expressions are represented using a namespace in which several global operators are overridden.
Note that the usual C++ operator precedence is enforced by the compiler. I have tried to map regular expression operations to C++ operators accordingly.
#### Literals:
  **Single character:** Represented by the method **literal(char)**.  
  Example: **literal('x')** matches the character **'x'**, **literal(L'x')** matches the wchar_t **'x'**.  
  **String of characters:** Represented by the method **litstr(char\*)**.  
  Example: **litstr("string")** matches the string **"string"**, **litstr(L"string")** matches the wchar_t **"string"**.  
  **Range of characters:** Represented by the method **litrange(char,char)**.  
  Example: **litrange('a','z')** matches any character in the range 'a' to 'z' inclusive.  
  **Any character in string:** Represented by the method **litanyinset("string")**.  
  Example: **litanyinset("aeiou")** matches any of the characters 'a','e','i','o','u'.  
  **Any character not in string:** Represented by the method **litnotset(string)**.  
  Note that this method **may** match surrogates for UTF-32.  
  Example: **litnotset("aeiou")** matches any character that isn't among "aeiou" including any surrogates.  
  **Any character not in string excluding surrogates:** Represented by **litnotset_no_surrogates("string")**.  
  Note that this only has any effect for UTF-32 - surrogates in the ranges U+D800 to U+DFFF are not matched.  
  Example: **litnotset_no_surrogates("aeiou")** matches any character that isn't among "aeiou" excluding any surrogates.  

  Note that, writh respect to surrogates, the parser doesn't attempt to discern if UTF-8 and UTF-16 sequences represent
  valid sequences. Also surrogates will be matched in UTF-32 unless otherwise prevented.

#### Unary operations:
  **Zero or more**: Represented by **operator ~( regexp )**.  
  Example: **~literal('a')** will match "", "a", "aa", etc.  
  **One or more**: Represented by **operator ++( regexp )**.  
  Example: **++literal('a')** will match "a", "aa", "aaa", etc.  
  **Zero or one**: Represented by **operator \-\-( regexp )**.  
  Example: **\-\-literal('a')** will match "" and "a" only.  
#### Binary operations:
  **Follows**: Represented by **operator \*( regexp, regexp )**.  
  Example: **literal('a') * literal('b')** will match "ab".  
  **Or**: Represented by the bitwise OR **operator |( regexp, regexp )**.  
  Example: **literal('a') | literal('b')** will match "a" or "b".  
#### Special operations:
  **Excludes**: Represented by **operator -( regexp, regexp )**.  
  Example: **++literal('a') * ~literal('b') - litstr('ab')** will match any number of 'a's followed by 0 or more 'b's except for the string "ab".  
  **Completes**: Represented by the **operator +( regexp, regexp )**.  
  Example: **++litrange('a','z') + "zzz"** will match any sequence of characters 'a' - 'z' but will stop when "zzz" is encountered.  
  **Lookahead**: Represented by **operator /( regexp, regexp )**.  
  Example: **++litrange('a','z') / litstr("\-\-")** will match any sequence of characters 'a' to 'z' when followed by the string "\-\-", not including the "\-\-".

### Complex example of regular expression usage:
This encodes the start of the XML regular expressions as specified in [https://www.w3.org/TR/xml/](https://www.w3.org/TR/xml/).
  Source: [https://github.com/davidbien/xmlp/blob/master/xmlpgen_utf8.cpp](https://github.com/davidbien/xmlp/blob/master/xmlpgen_utf8.cpp).
  There are separate versions for UTF-8, UTF-16, and UTF-32 - necessarily.  

    typedef char8_t _TyCTok;
    typedef _regexp_final< _TyCTok, _TyAllocator > _TyFinal;
    typedef _regexp_trigger< _TyCTok, _TyAllocator > _TyTrigger;

    // Define some simple macros to make the productions more readable.
    #define l(x) literal<_TyCTok>(x)
    #define ls(x) litstr<_TyCTok>(x)
    #define lr(x,y)	litrange<_TyCTok>(x,y)
    #define lanyof(x) litanyinset<_TyCTok>(x)
    #define lnot(x) litnotset<_TyCTok>(x)
    #define t(a) _TyTrigger(a)

    // Utility and multiple use non-triggered productions:
    _TyFinal S = ++( l(0x20) | l(0x09) | l(0x0d) | l(0x0a) ); //[3]
    _TyFinal Eq = --S * l(u8'=') * --S; //[25].
    _TyFinal Char =	l(0x09) | l(0x0a) | l(0x0d) | lr(0x20,0xFF); //[2].

    _TyFinal NameStartChar = l(u8':') | lr(u8'A',u8'Z') | l(u8'_') | lr(u8'a',u8'z') |lr(0xC0,0xFF);//[4]
    _TyFinal NameChar = NameStartChar | l(u8'-') | l(u8'.') | lr(u8'0',u8'9') | lr(0x80,0xBF); //[4a]
    _TyFinal Name = NameStartChar * ~NameChar; //[5]

    // namespace support:
    _TyFinal NameStartCharNoColon = lr(u8'A',u8'Z') | l(u8'_') | lr(u8'a',u8'z') | lr(0xC0,0xFF);//[4ns]
    _TyFinal NameCharNoColon = NameStartCharNoColon | l(u8'-') | l(u8'.') | lr(u8'0',u8'9') 
      | lr(0x80,0xBF); //[4ans]
    _TyFinal NCName = NameStartCharNoColon * ~NameCharNoColon;
    _TyFinal Prefix = NCName;
    _TyFinal LocalPart = NCName;

    // Qualified name, first triggers: transmit a signal to the lexer so that position can be recorded.
    _TyFinal QName = t(TyGetTriggerPrefixBegin<_TyLexT>()) * Prefix * t(TyGetTriggerPrefixEnd<_TyLexT>())
            * --( l(u8':') * t( TyGetTriggerLocalPartBegin<_TyLexT>() ) 
            * LocalPart * t( TyGetTriggerLocalPartEnd<_TyLexT>() ) ); //[7]

    _TyFinal QName = Prefix * --( l(L':') * LocalPart );
    _TyFinal EntityRef = l(L'&') * Name * l(L';'); // [49]
    _TyFinal CharRef = ls(L"&#") * ++lr(L'0',L'9') * l(L';') 
                    | ls(L"&#x") * ++( lr(L'0',L'9') | lr(L'A',L'F') | lr(L'a',L'f') ) * l(L';'); //[66]
    _TyFinal Reference = EntityRef | CharRef;	// [67]
    _TyFinal AVCharNoAmperLessDouble = l(0x09) | l(0x0a) | l(0x0d)	// Char - '&' - '<' - '"'
                                | lr(0x020,0x021) | lr(0x023,0x025) | lr(0x027,0x03b) | lr(0x03d,0xd7ff)
                                | lr(0xe000,0xfffd);
    _TyFinal AVCharNoAmperLessSingle = l(0x09) | l(0x0a) | l(0x0d) |	// Char - '&' - '<' - '\''
                                      lr(0x020,0x025) | lr(0x028,0x03b) | lr(0x03d,0xd7ff) 
                                      | lr(0xe000,0xfffd);
    _TyFinal AttValue = l(L'\"') * ~( AVCharNoAmperLessDouble | Reference ) * l(L'\"')	//[10]
                      | l(L'\'') * ~( AVCharNoAmperLessSingle | Reference ) * l(L'\'');
    _TyFinal Attribute = QName * Eq * AttValue;

    _TyFinal PI = ls(L"<?")	* PITarget * ( ls(L"?>") | ( S * ( ~Char + ls(L"?>") ) ) );
    _TyFinal CharNoMinus =	l(0x09) | l(0x0a) | l(0x0d) // [2].
                        | lr(0x020,0x02c) | lr(0x02e,0xd7ff) | lr(0xe000,0xfffd);
    _TyFinal Comment = ls(L"&lt;!--") * ~( CharNoMinus | ( l(L'-') * CharNoMinus ) ) * ls(L"-->");
    _TyFinal MixedBegin = l(L'(') * --S * ls(L"#PCDATA");
    _TyFinal Mixed = MixedBegin * ~( --S * l(L'|') * --S * Name ) * --S * ls(L")*") |
                    MixedBegin * --S * l(L')'); // [51].

## Actions and triggers.
### All Actions and Triggers have unique "action identifiers" of type vtyActionIdent (int32_t) which uniquely identify the action or trigger in question.
### Actions: Are associated with final productions and serve to communicate the token to the analyzer.
### Triggers: Triggers are placed inline within a regular expression and serve to communicate positions within a token to the analyzer.



  

## Conversion to NFA, DFA and optimized DFA:
### Regular expressions are represented using a namespace in which several global operators are overridden.

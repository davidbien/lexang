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

    // Attribute value and character data productions:
    _TyFinal EntityRef = l(u8'&') * t( TyGetTriggerEntityRefBegin<_TyLexT>() ) 	// [49]
                                  * Name * t( TyGetTriggerEntityRefEnd<_TyLexT>() ) * l(u8';');
    _TyFinal CharRefDecData = ++lr(u8'0',u8'9');
    _TyFinal CharRefHexData = ++( lr(u8'0',u8'9') | lr(u8'A',u8'F') | lr(u8'a',u8'f') );
    _TyFinal CharRefDec = ls(u8"&#") * t( TyGetTriggerCharDecRefBegin<_TyLexT>() ) * CharRefDecData 
                                     * t( TyGetTriggerCharDecRefEnd<_TyLexT>()  ) * l(u8';');
    _TyFinal CharRefHex = ls(u8"&#x") * t( TyGetTriggerCharHexRefBegin<_TyLexT>() ) * CharRefHexData 
                                      * t( TyGetTriggerCharHexRefEnd<_TyLexT>() ) * l(u8';');
    _TyFinal CharRef = CharRefDec | CharRefHex; // [66]
    _TyFinal Reference = EntityRef | CharRef;	// [67]
    
    _TyFinal _AVCharRangeNoAmperLessDouble =	t(TyGetTriggerCharDataBegin<_TyLexT>()) 
          * ++lnot(u8"&<\"") * t(TyGetTriggerCharDataEnd<_TyLexT>());
    _TyFinal _AVCharRangeNoAmperLessSingle =	t(TyGetTriggerCharDataSingleQuoteBegin<_TyLexT>()) 
          * ++lnot(u8"&<\'") * t(TyGetTriggerCharDataSingleQuoteEnd<_TyLexT>());
    _TyFinal AttCharDataNoDoubleQuoteOutputValidate = ~lnot(u8"&<\"");
    _TyFinal AttCharDataNoSingleQuoteOutputValidate = ~lnot(u8"&<\'");
    // We need only record whether single or double quotes were used as a convenience to any 
    //  reader/writer system that may want to duplicate the current manner of quoting.
    // No need to record a single quote trigger as the lack of double quote is adequate.
    _TyFinal AttValue =	l(u8'\"') * t( TyGetTriggerAttValueDoubleQuote<_TyLexT>() ) // [10]
                      * ~( _AVCharRangeNoAmperLessDouble | Reference )  * l(u8'\"') 
                      | l(u8'\'') * ~( _AVCharRangeNoAmperLessSingle | Reference ) * l(u8'\''); 
    _TyFinal Attribute = QName * Eq * AttValue;

    // The various types of tags:
    _TyFinal STag = l(u8'<') * QName * t(TyGetTriggerSaveTagName<_TyLexT>()) 
      * ~( S * Attribute * t(TyGetTriggerSaveAttributes<_TyLexT>()) ) * --S * l(u8'>'); // [12]
    _TyFinal ETag = ls(u8"</") * QName * --S * l(u8'>');// [13]
    _TyFinal EmptyElemTag = l(u8'<') * QName * t(TyGetTriggerSaveTagName<_TyLexT>()) 
      * ~( S * Attribute * t(TyGetTriggerSaveAttributes<_TyLexT>()) ) * --S * ls(u8"/>");// [14]								

## Actions: Tokens and Triggers
### Tokens and Triggers are Actions that have unique "action identifiers" of type vtyActionIdent(int32_t) which uniquely identify the Token or Trigger in question.
### Tokens: Are associated with final productions and serve to communicate the token found to the analyzer. Examples:
    static const vtyActionIdent s_knTokenSTag = 1000;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTokenSTag = _l_action_token< _l_action_save_data_single< s_knTokenSTag, 
      TyGetTriggerSaveTagName<t_TyLexTraits,t_fInLexGen>, 
      TyGetTriggerSaveAttributes<t_TyLexTraits,t_fInLexGen> > >;

    static const vtyActionIdent s_knTokenETag = 1001;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTokenETag = _l_action_token< _l_action_save_data_single< s_knTokenETag, 
      TyGetTriggerPrefixEnd<t_TyLexTraits,t_fInLexGen>, 
      TyGetTriggerLocalPartEnd<t_TyLexTraits,t_fInLexGen> > >;

    static const vtyActionIdent s_knTokenEmptyElemTag = 1002;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTokenEmptyElemTag = _l_action_token< _l_action_save_data_single< s_knTokenEmptyElemTag, 
      TyGetTriggerSaveTagName<t_TyLexTraits,t_fInLexGen>, 
      TyGetTriggerSaveAttributes<t_TyLexTraits,t_fInLexGen> > >;

    STag.SetAction( TyGetTokenSTag<_TyLexT>() );
    ETag.SetAction( TyGetTokenETag<_TyLexT>() );
    EmptyElemTag.SetAction( TyGetTokenEmptyElemTag<_TyLexT>() );

### Triggers: Are used to communicate positions within a Token to the analyzer. This obviates re-parsing of a found token in most cases by finding all relevant positions while parsing the token. Note that Triggers cannot be used at the start of a final production as they would signal every time. If a Trigger is used at the start of a final production then an exception is thrown during analyzer generation. Examples:
    static const vtyActionIdent s_knTriggerPrefixBegin = 18;
    static const vtyActionIdent s_knTriggerPrefixEnd = 19;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTriggerPrefixBegin = _l_trigger_position< t_TyLexTraits, 
                                                         s_knTriggerPrefixBegin, t_fInLexGen >;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTriggerPrefixEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerPrefixEnd, 
                                                     s_knTriggerPrefixBegin, t_fInLexGen >;

    static const vtyActionIdent s_knTriggerLocalPartBegin = 20;
    static const vtyActionIdent s_knTriggerLocalPartEnd = 21;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTriggerLocalPartBegin = _l_trigger_position< t_TyLexTraits, 
                                                            s_knTriggerLocalPartBegin, t_fInLexGen >;
    template < class t_TyLexTraits, bool t_fInLexGen = true >
    using TyGetTriggerLocalPartEnd = _l_trigger_string< t_TyLexTraits, s_knTriggerLocalPartEnd, 
                                                        s_knTriggerLocalPartBegin, t_fInLexGen >;


## Conversion to NFA, DFA and optimized DFA:
### To produce the lexical analyzer(s) (potentially one for each type of input character UTF-8, 16 or 32) the final productions are converted first to a NFA (non-discrete finite automata) then to a DFA (discrete finite automata) and then this DFA is optimized to most compact form, getting rid of redundancies. For the XML parser I produce 

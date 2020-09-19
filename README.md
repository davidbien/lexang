# lexang
### A set of templates that allow generation of a lexicographical analyzer given a set of regular expressions.
### Meant to be somewhat of a replacement for the good old unix lex command.

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
  Example: **++litrange('a','z') / '--'** will match any sequence of characters 'a' to 'z' when followed by the string "--".  

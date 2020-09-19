# lexang: A set of templates that allow generations of a lexicographical analyzer given a set of regular expression.
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
  **Zero or more**: Represented by operator ~( regexp ).  
  Example: **~literal('a')** will match "", "a", "aa", etc.  
  

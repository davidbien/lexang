# lexang: A set of templates that allow generations of a lexicographical analyzer given a set of regular expression.
### Meant to be somewhat of a replacement for the good old unix lex command.

## Regular Expressions:
### Regular expressions are represented using a namespace in which several global operators are overridden.
#### Literals:
  Single character: Represented by the method literal(char).
  Example: literal('x') matches the character 'x', literal(L'x') matches the wchar_t 'x'.
#### Unary operations:
  Zero or more: operator ~();

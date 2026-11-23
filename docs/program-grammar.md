# Declarations and basic statements

`--ast` uses `program` as its start rule. `expression` comes from the [expression grammar](expression-grammar.md) and is shared across both AST modes.

## EBNF

```ebnf
program           = { declaration } EOF ;
declaration       = IDENTIFIER ":" ( variable_tail | function_tail ) ;
variable_decl     = IDENTIFIER ":" variable_tail ;
variable_tail     = primitive_type [ "=" expression ] ";"
                  | array_type [ "=" initializer ] ";" ;
variable_type     = primitive_type | array_type ;
array_type        = "array" "[" expression "]" variable_type ;
primitive_type    = "integer" | "boolean" | "char" | "string" ;
return_type       = primitive_type | "void" ;
parameter_type    = primitive_type | "array" "[" "]" variable_type ;
initializer       = expression | "{" initializer { "," initializer } "}" ;
function_tail     = "function" return_type "(" [ parameters ] ")"
                    ( ";" | "=" block ) ;
parameters        = parameter { "," parameter } ;
parameter         = IDENTIFIER ":" parameter_type ;
block             = "{" { statement } "}" ;
statement         = block | variable_decl | return_statement | expression ";" ;
return_statement  = "return" [ expression ] ";" ;
```

## Parsing decisions

- A top-level name and colon form a shared prefix. The next keyword selects a function or variable declaration. After a function signature, `;` ends a prototype and `=` introduces the definition’s body.
- A statement beginning with an identifier can be a local declaration or an expression. The parser looks one token beyond the identifier and selects the declaration rule when it sees `:`. Both lookahead tokens skip comments. `return` and `{` select their own statement rules immediately.
- Each variable array dimension has an expression for its size. In a parameter, the outer array dimension uses `[]`. Any inner dimensions carry sizes. Array initializer lists contain at least one item and may contain nested lists. Constant sizes for globals, positive dimensions, and initializer length and type checks belong to semantic analysis.
- `void` is a return type. Matching a function’s declared type to its `return` statements, resolving names, and requiring `main` in a complete executable are later checks.

## AST and diagnostics

The program stores declarations in source order. A function node stores its name token, return type, parameters, and an optional block. Initializers, array dimensions, expression statements, and return values point into the same expression array. Nodes keep their source spans and reserved type annotation fields. Block and initializer nesting have a limit of 256 levels.

This increment reports the first error and prints the AST only after a successful parse. The next statement increment adds `if`/`else`, `for`, and `print`, followed by panic-mode recovery within blocks and between declarations.

The declaration forms follow [Appendix B of Douglas Thain’s compiler textbook](https://dthain.github.io/books/compiler/chapterB.pdf). The unsized outer parameter dimension follows the clarification in [the B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor#questions-and-clarifications-for-2023).

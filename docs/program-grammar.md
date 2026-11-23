# Declarations and statements

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
statement         = closed_statement | open_statement ;
closed_statement  = simple_statement
                  | if_header closed_statement "else" closed_statement
                  | for_header closed_statement ;
open_statement    = if_header statement
                  | if_header closed_statement "else" open_statement
                  | for_header open_statement ;
simple_statement  = block | variable_decl | return_statement | print_statement
                  | expression ";" ;
if_header         = "if" "(" expression ")" ;
for_header        = "for" "(" [ expression ] ";" [ expression ] ";" [ expression ] ")" ;
return_statement  = "return" [ expression ] ";" ;
print_statement   = "print" [ expression { "," expression } ] ";" ;
```

## Parsing decisions

- A top-level name and colon form a shared prefix. The next keyword selects a function or variable declaration. After a function signature, `;` ends a prototype and `=` introduces the definition’s body.
- A statement beginning with an identifier can be a local declaration or an expression. The parser looks one token beyond the identifier and selects the declaration rule when it sees `:`. Both lookahead tokens skip comments. `{`, `return`, `if`, `for`, and `print` each select their own rule immediately.
- A closed statement cannot take another `else`. An open statement ends in a branch that can. The parser factors the shared `if` prefix by parsing the entire then branch first, then consuming `else` if it remains in lookahead. This resolves the overlap between an optional `else` and the enclosing statement’s following tokens by binding `else` to the nearest unmatched `if`. Braces delimit their own statement list.
- Each `for` clause uses its following `;` or `)` to select the empty alternative. The loop body can be one statement or a block. After `print`, `;` selects the empty list. Otherwise the parser reads an expression and uses commas to continue the list.
- Each variable array dimension has an expression for its size. In a parameter, the outer array dimension uses `[]`. Any inner dimensions carry sizes. Array initializer lists contain at least one item and may contain nested lists. Constant sizes for globals, positive dimensions, and initializer length and type checks belong to semantic analysis.
- `void` is a return type. Matching a function’s declared type to its `return` statements, resolving names, and requiring `main` in a complete executable are later checks.

## AST and diagnostics

The program stores declarations in source order. A function node stores its name token, return type, parameters, and an optional block. Initializers, array sizes, conditions, print arguments, and return values point into the same expression array. An `IfStmt` stores its condition and branches. A `ForStmt` stores its optional clauses and body. A `PrintStmt` stores its arguments in source order. Nodes keep their source spans and reserved type annotation fields. Statement and initializer nesting are limited to 256 levels.

## Panic-mode recovery

The program parser catches an error in a declaration or block statement, records its source location, and discards the expression nodes built by that failed construct. It then skips tokens to a synchronization point and resumes:

- A semicolon ends the discarded construct. Semicolons inside a broken `for` header are skipped until the header closes or recovery reaches a statement boundary.
- A closing brace at the current block depth is left for the block to consume. During synchronization, a braced region can be skipped as a unit. An unmatched top-level closing brace is consumed.
- A new `name:` declaration or the statement keywords `if`, `for`, `print`, and `return` can also provide a restart point. Declaration lookahead accounts for parentheses so that a parameter in a malformed signature stays inside that signature.
- At EOF, an unclosed block reports its missing `}` and retains statements already parsed. If an `if` condition is immediately followed by `{` where `)` was expected, the parser reports the missing `)` and uses the brace to start the then branch.

The parser also records invalid lexer tokens encountered while skipping a damaged construct. Recovery is best effort when delimiters are missing. A failed construct may be omitted, and the partial tree can retain a block with an inferred closing boundary. The EBNF above describes valid inputs only.

`parse_program_with_recovery` returns the partial tree and its diagnostics together. The `--ast` command uses that result and returns a failure status when it contains any diagnostics. The `parse_program_file` API still throws the first diagnostic for callers that require a complete tree, and `--expr-ast` still stops after its first error.

The declaration and statement forms follow [Appendix B of Douglas Thain’s compiler textbook](https://dthain.github.io/books/compiler/chapterB.pdf). Optional loop clauses are described in [Chapter 11](https://dthain.github.io/books/compiler/chapter11.pdf). The unsized outer parameter dimension and empty `print` follow [the B-Minor language clarifications](https://dthain.github.io/compilers-fa23/bminor#questions-and-clarifications-for-2023).

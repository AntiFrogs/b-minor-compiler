# B-Minor expression grammar

The `--expr-ast` entry point parses one expression from a token stream. The lexer supplies terminal kinds and spellings. The parser skips comment tokens before selecting each next token.

## EBNF

```ebnf
expression_file = expression EOF ;
expression      = assignment ;
assignment      = logical_or [ "=" assignment ] ;
logical_or      = logical_and { "||" logical_and } ;
logical_and     = equality { "&&" equality } ;
equality        = relation { ("==" | "!=") relation } ;
relation        = additive { ("<" | "<=" | ">" | ">=") additive } ;
additive        = multiplicative { ("+" | "-") multiplicative } ;
multiplicative  = power { ("*" | "/" | "%") power } ;
power           = prefix [ "^" power ] ;
prefix          = { "+" | "-" | "!" } postfix ;
postfix         = call_or_index [ "++" | "--" ] ;
call_or_index   = primary { "(" [ arguments ] ")" | "[" expression "]" } ;
arguments       = expression { "," expression } ;
primary         = IDENTIFIER | INTEGER | BOOLEAN | STRING | CHARACTER
                | "(" expression ")" ;
```

## Precedence and lookahead

Assignment and exponentiation associate to the right. The other binary levels associate to the left. Prefix operators bind more tightly than exponentiation, and calls and subscripts bind more tightly than postfix increment or decrement. The implementation uses C-style precedence between ordering comparisons and equality, and between `&&` and `||`.

The recursive-descent parser handles the binary levels with precedence climbing. Given an operator at precedence `p`, it parses the right operand with minimum precedence `p + 1` for a left-associative operator and `p` for a right-associative one. The grammar has no left-recursive productions. The parser uses one lookahead token to distinguish primary expressions and postfix suffixes. After a call’s `(`, a `)` selects the empty argument list. The other expression starters select an argument. Commas and closing delimiters end the nested expression so the enclosing rule can consume the delimiter.

## AST and diagnostics

- Each node has a kind, a start location, an exclusive ending byte offset, and an initially empty type annotation. Leaf nodes and operator nodes also retain the exact token, including its own location.
- Binary and assignment children are left then right. A call stores the callee first and arguments in source order. A subscript stores its base and index. Parentheses control grouping and extend the enclosed node’s source span.
- Child IDs index a flat array of nodes, so growing the tree cannot invalidate a child ID. A long left-associative chain can be parsed and printed without recursive AST traversal. The recursive parser has a limit of 256 nested expression levels and reports a diagnostic when it is exceeded.
- The parser checks expression structure. Checks for assignable destinations, callable values, operand types, and valid indices belong to semantic analysis. The standalone entry point reports the first lexical or syntactic error. The [program parser](program-grammar.md) recovers at declaration and statement boundaries.

The operator vocabulary and the order of postfix, prefix, and arithmetic operators follow [Appendix B of Douglas Thain’s compiler textbook](https://dthain.github.io/books/compiler/chapterB.pdf). Prefix `+` follows the scanner and parser clarification in [the B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor#questions-and-clarifications-for-2023).

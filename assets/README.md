# Phase 1 test inputs

Run the checks for the feature you just added and rerun earlier checks that the change could affect. Files under `valid/` are valid source inputs for the Phase 1 front end. Some are snippets without a `main` function. Files under `invalid/` are deliberately broken. `expressions/` contains standalone inputs for `--expr-ast`, including malformed inputs under `expressions/invalid/`. `dialect/` contains a case to check against your grammar decisions.

## Automated checks for the current version

Follow the [build instructions](../README.md#build-on-linux), then run these commands from the repository root:

```sh
sh tests/test-source-copy.sh
sh tests/test-core-lexer.sh
sh tests/test-literals-comments.sh
sh tests/test-expression-parser.sh
sh tests/test-program-parser.sh
sh tests/test-statements.sh
sh tests/test-parser-recovery.sh
python3 tests/test-ansi-highlighting.py
python3 tests/test-html-highlighting.py
```

The copy script exercises every fixture, including malformed ones. The lexer scripts check core tokens, literals, comments, positions, and lexical recovery. The parser scripts check expression and program ASTs, diagnostics, and partial program recovery. The ANSI and HTML scripts check source preservation and contextual styles.

The three `io/` files cover a zero-byte input, a line comment that reaches end of file without a newline, and CRLF line endings. The attributes file alongside them keeps Git from normalizing the CRLF fixture.

## Checks by milestone

Paths in the table are relative to `assets/`. Use the token and AST displays when those features are available. Exact diagnostic wording and AST formatting can vary.

| Milestone | Input | Expected check |
|---|---|---|
| Core tokens and positions | `valid/token-boundaries.bminor`, `io/crlf.bminor` | `returnValue` is one identifier, `return` is a keyword, and `_count` and `x1` are identifiers. Each of `<=`, `>=`, `==`, `!=`, `&&`, and `\|\|` is one token. In the CRLF file, `second` starts at line 3, column 1. |
| Literals and comments | `valid/literals-and-comments.bminor`, `io/no-final-newline.bminor` | Check an integer, both boolean values, escaped quotes and backslashes, an escaped character, and an empty string. Comment markers inside `marker` belong to one string token. Both real comment forms are recognized, including the line comment at EOF. The multiline comment ends on line 12, and `marker` starts at line 13, column 1. |
| Lexical diagnostics | `invalid/lexer/unknown-character.bminor` | Emit `INVALID` for `@` at line 1, column 20. Scanning continues to the declaration on line 2. |
| Lexical diagnostics at EOF | `invalid/lexer/unterminated-string.bminor`, `invalid/lexer/unterminated-block-comment.bminor` | Report the unclosed literal or comment and terminate normally. The opening string quote is at line 1, column 19, and the block comment starts at line 2, column 1. |
| Literal recovery | `invalid/lexer/literal-recovery.bminor` | Report four errors for the unclosed string, empty character, two-character literal, and unclosed character. Keep scanning `after_string` and `after_char` and emit `EOF`. |
| Expression AST | `expressions/precedence.bminor`, `expressions/assignment.bminor`, `expressions/postfix.bminor`, `expressions/logic-and-literals.bminor` | Check multiplication before addition, left-associative subtraction, right-associative assignment and power, grouping, calls, subscripts, postfix increment, and boolean expressions. |
| Expression diagnostics | `expressions/invalid/` | Report a missing operand or closing delimiter at the unexpected token or EOF. Exit without printing a partial AST. |
| Declarations and basic statements | `valid/declarations.bminor`, `valid/function-bodies.bminor` | Recognize prototypes and definitions, array types and initializers, array parameters, nested blocks, local declarations, expression statements, and both return forms. |
| Expressions in programs | `valid/precedence.bminor` | Check grouped arithmetic and operator precedence in variable initializers and return statements. |
| Control flow and `print` | `valid/statements.bminor`, `valid/factorial.bminor`, `valid/control-flow.bminor`, `valid/highlighting.bminor` | Recognize `for`, `if`/`else`, and `print`. Check the nearest-`if` rule for nested branches, optional loop clauses, argument order, and the recursive call used as the right operand of `*`. |
| Parser recovery | `invalid/parser/missing-expression.bminor`, `invalid/parser/missing-right-paren.bminor`, `invalid/parser/multiple-errors.bminor`, `invalid/parser/truncated-block.bminor` | Report the missing expression, closing parenthesis, or closing brace. Keep the top-level `after` declaration in the first two files. Report both bad declarations and retain `first_ok` and `second_ok` in `multiple-errors`. Retain the completed return when `truncated-block` ends at EOF. |
| Recovery within blocks | `invalid/parser/block-recovery.bminor` | Report three errors, retain the valid `print` statements and the final assignment and return, and reach the top-level `after` declaration. The invalid loop and its body are omitted from the AST. |
| AST-aware highlighting | `valid/highlighting.bminor`, `valid/highlight-roles.bminor`, `valid/factorial.bminor`, `invalid/lexer/unknown-character.bminor` | Style function prototypes, definitions, and calls while keeping variables in the default style, even when they share a function’s spelling. Mark `@` while still highlighting later tokens. |
| ANSI output | All fixtures | Verify colors and resets, then compare the unstyled text with the original. Check CRLF line endings, indentation, and files without a final newline. |
| HTML output | All fixtures | Verify the decoded code block matches the source. Open HTML generated from `valid/highlighting.bminor` and `invalid/lexer/unknown-character.bminor` to check that `<tag>&` displays literally and `@` is visibly marked. The page works without JavaScript. |

### Dialect check

The current lexer emits `-` as a separate operator in both `x-1` and `-1`, as specified in the [lexical rules](../docs/lexical-rules.md#match-order). Once the parser is available, verify that `x-1` parses the same way as `x - 1`.

The Phase 1 handout is the project reference. The array, prototype, and statement fixtures also follow the forms in Douglas Thain’s B-Minor language overview.

[B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor)

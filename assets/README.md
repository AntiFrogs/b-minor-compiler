# Phase 1 test inputs

Run the checks for the feature you just added and rerun earlier checks that the change could affect. Files under `valid/` are valid source inputs for the Phase 1 front end. Some are snippets without a `main` function. Files under `invalid/` are deliberately broken. `dialect/` contains a case to check against your grammar decisions.

## Automated checks for the current version

Follow the [build instructions](../README.md#build-on-linux), then run these commands from the repository root:

```sh
sh tests/test-source-copy.sh
sh tests/test-core-lexer.sh
sh tests/test-literals-comments.sh
```

The copy script exercises every fixture, including malformed ones. The lexer scripts check core tokens, literals, comments, positions, and lexical recovery. The parser and renderer rows below describe checks for later versions.

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
| Expression AST | `valid/precedence.bminor`, `valid/factorial.bminor` | In `plain`, `+` has `3 * 4` as its right operand. In `grouped`, `*` has `2 + 3` as its left operand. In `chained`, the left operand of the outer subtraction is `20 - 5`. In `powered`, `/` has `2 ^ 3` as its left operand. In `factorial`, the recursive call is the right operand of `*` and takes `n - 1` as its argument. |
| Declarations and statements | `valid/declarations.bminor`, `valid/statements.bminor` | Recognize the function prototype and definition, fixed-size arrays, initializer list, and indexed access. In `statements`, recognize a `for`, nested `if/else` blocks, a `print` with three arguments, and postfix `++` and `--`. AST nodes have source positions and a slot for later type information. |
| Parser recovery | `invalid/parser/missing-expression.bminor`, `invalid/parser/missing-right-paren.bminor`, `invalid/parser/multiple-errors.bminor`, `invalid/parser/truncated-block.bminor` | Report the missing expression, the unclosed condition, or the missing closing brace. Recover the top-level `after` declaration in the first two files. Report both bad statements and retain `first_ok` and `second_ok` in `multiple-errors`. Stop at EOF in `truncated-block`. |
| AST-aware highlighting | `valid/highlighting.bminor`, `valid/factorial.bminor`, `invalid/lexer/unknown-character.bminor` | Style `double_value` at its definition and call as a function name. Style `value` as a variable or parameter. Style comments and literals, and mark `@` while still highlighting the rest of the input. |
| ANSI output | `valid/highlighting.bminor`, `io/no-final-newline.bminor` | Verify colors and resets, then compare the unstyled text with the original. Check the indentation and that the EOF fixture still has no final newline. |
| HTML output | `valid/highlighting.bminor`, `invalid/lexer/unknown-character.bminor` | Open the page in a browser. The literal text `<tag>&` should be visible, indentation should match the input, and the invalid token should be visibly marked in the second file. The page should work without JavaScript. |

### Dialect check

The current lexer emits `-` as a separate operator in both `x-1` and `-1`, as specified in the [lexical rules](../docs/lexical-rules.md#match-order). Once the parser is available, verify that `x-1` parses the same way as `x - 1`.

The Phase 1 handout is the project reference. The array, prototype, and statement fixtures also follow the forms in Douglas Thain’s B-Minor language overview.

[B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor)

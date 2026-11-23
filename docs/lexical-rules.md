# B-Minor lexical rules

## Core token patterns

The lexer scans ASCII token spellings. These regular expressions apply to the implemented token kinds:

```text
identifier       [A-Za-z_][A-Za-z0-9_]*
decimal integer  [0-9]+
whitespace       [ \t\r\n]+
```

- Keywords are `array`, `boolean`, `char`, `else`, `for`, `function`, `if`, `integer`, `print`, `return`, `string`, `void`, and `while`. `while` is reserved in the reference spelling list. Statement support is a parser decision.
- Boolean literals are `true` and `false`.
- Two-character operators are `++`, `--`, `<=`, `>=`, `==`, `!=`, `&&`, and `||`.
- Single-character operators are `!`, `^`, `*`, `/`, `%`, `+`, `-`, `<`, `>`, and `=`.
- Delimiters are `(`, `)`, `[`, `]`, `{`, `}`, `,`, `:`, and `;`.

## Match order

Consume the longest word before checking it against the keyword and boolean spellings. This makes `return` a keyword and `returnValue` a single identifier. Consume both bytes of a two-character operator when present before considering a single-character operator. Consume all consecutive decimal digits as one integer token.

Signs are separate operator tokens. `x-1` becomes `IDENTIFIER`, `OPERATOR`, `INTEGER`, and `-1` becomes `OPERATOR`, `INTEGER`. A later parser determines which uses are unary or binary. The token keeps the original digits, so scanning avoids an integer conversion and its range limits.

## Positions and recovery

Every token contains its kind, the exact source slice, the filename, a zero-based byte offset, and a 1-based starting line and column. The source text and filename must outlive the lexer and its tokens. The CLI owns the input while it prints the token stream.

Spaces and tabs advance one column each. LF, CR, and CRLF advance one line and reset the column to 1. Whitespace advances the cursor. The lexer emits an empty `EOF` token at the final cursor position.

When no core rule matches, the lexer consumes one byte and returns an `INVALID` token at that location. The CLI reports the error and continues, so the stream includes subsequent tokens and `EOF`.

String and character literals, escapes, comments, and their unterminated-input diagnostics are planned for the next lexer milestone. This document will gain their formal rules with that implementation.

## References

- The Phase 1 handout, pages 7–9, defines the required categories, positions, matching priority, and error recovery.
- The extra spellings follow [Appendix B of Douglas Thain’s compiler textbook](https://dthain.github.io/books/compiler/chapterB.pdf). The separate sign decision follows the scanning clarification in [the B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor#questions-and-clarifications-for-2023).

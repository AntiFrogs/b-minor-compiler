# B-Minor lexical rules

## Token patterns

The lexer recognizes these patterns over source bytes:

```text
identifier       [A-Za-z_][A-Za-z0-9_]*
decimal integer  [0-9]+
whitespace       [ \t\r\n]+
string           "(?:[\x20-\x21\x23-\x5B\x5D-\x7E]|\\[\x20-\x7E])*"
character        '(?:[\x20-\x26\x28-\x5B\x5D-\x7E]|\\[\x20-\x7E])'
line comment     //[^\r\n]*
block comment    /\*(?:[^*]|\*+[^*/])*\*+/
```

- Keywords are `array`, `boolean`, `char`, `else`, `for`, `function`, `if`, `integer`, `print`, `return`, `string`, `void`, and `while`. `while` is reserved in the reference spelling list. Statement support is a parser decision.
- Boolean literals are `true` and `false`.
- Two-character operators are `++`, `--`, `<=`, `>=`, `==`, `!=`, `&&`, and `||`.
- Single-character operators are `!`, `^`, `*`, `/`, `%`, `+`, `-`, `<`, `>`, and `=`.
- Delimiters are `(`, `)`, `[`, `]`, `{`, `}`, `,`, `:`, and `;`.

## Match order

Consume the longest word before checking it against the keyword and boolean spellings. This makes `return` a keyword and `returnValue` a single identifier. Consume both bytes of a two-character operator when present before considering a single-character operator. Consume all consecutive decimal digits as one integer token.

Signs are separate operator tokens. `x-1` becomes `IDENTIFIER`, `OPERATOR`, `INTEGER`, and `-1` becomes `OPERATOR`, `INTEGER`. A later parser determines which uses are unary or binary. The token keeps the original digits, so scanning avoids an integer conversion and its range limits.

At a slash, check for `//` and `/*` before recognizing division. A line comment ends at CR, LF, or EOF, with the line ending excluded from its token. A block comment ends at its first `*/`. An internal `/*` stays part of that comment. Comment tokens include their delimiters and original text for highlighting.

## Quoted literals

A string contains zero or more printable ASCII characters or escapes. A character literal contains exactly one. A backslash consumes the next printable ASCII byte as one character, so `\"` and `\'` can represent the active quote and `\\` represents a backslash. Comment delimiters inside a literal are ordinary text. The scanner keeps the raw spelling, including the quotes and escapes, for later decoding. The escape spelling follows the textbook form, which gives `\n` and `\0` special meanings and uses the following character for other escapes.

## Positions and recovery

Every token contains its kind, the exact source slice, the filename, a zero-based byte offset, and a 1-based starting line and column. The source text and filename must outlive the lexer and its tokens. The CLI owns the input while it prints the token stream.

Spaces and tabs advance one column each. LF, CR, and CRLF advance one line and reset the column to 1, including inside block comments. The lexer skips whitespace and emits an empty `EOF` token at the final cursor position.

An `INVALID` token carries a structured reason and covers the offending source span. An unknown byte outside a literal or comment consumes one byte. A closed literal with a disallowed byte or the wrong character count consumes through its closing quote. An unclosed literal stops at the first line ending or EOF, so scanning can resume on the next line. An unclosed block comment consumes through EOF. Each diagnostic points to the beginning of its invalid span. The CLI continues to emit subsequent tokens and `EOF`.

## References

- The Phase 1 handout, pages 7–9, defines the required categories, positions, matching priority, and error recovery.
- The extra spellings follow [Appendix B of Douglas Thain’s compiler textbook](https://dthain.github.io/books/compiler/chapterB.pdf). The separate sign decision follows the scanning clarification in [the B-Minor language overview](https://dthain.github.io/compilers-fa23/bminor#questions-and-clarifications-for-2023).

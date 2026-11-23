# B-Minor Phase 1

The Phase 1 front end now parses expressions into an abstract syntax tree (AST). The lexer also exposes the complete token stream, including comments, and reports lexical errors with their source positions.

## Build on Linux

Use a C++17 compiler. From the project root, run these commands with GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp src/lexer.cpp src/parser.cpp -o build/bminor
```

## Run

Copy a file verbatim to standard output:

```sh
./build/bminor assets/valid/factorial.bminor
```

Dump tokens from a source file:

```sh
./build/bminor --tokens assets/valid/token-boundaries.bminor
./build/bminor --tokens assets/valid/literals-and-comments.bminor
```

Each line gives the filename, 1-based line and column, token kind, and quoted source spelling. For example:

```text
assets/valid/token-boundaries.bminor:1:1 IDENTIFIER "returnValue"
```

Token mode prints comments and exact literal spellings, followed by an `EOF` token. It escapes quotes and backslashes for display and shows nonprintable and non-ASCII bytes as `\xHH`. A malformed literal, unclosed block comment, or unknown byte produces an `INVALID` token on standard output and a diagnostic on standard error. The process exits with a nonzero status after scanning an input with lexical errors. Try `./build/bminor --tokens assets/invalid/lexer/literal-recovery.bminor` to see the scanner recover on later lines.

Copy mode preserves the input bytes, including line endings and a missing final newline. An empty file produces empty output in copy mode and one `EOF` token in token mode.

Parse one expression and print its AST:

```sh
./build/bminor --expr-ast assets/expressions/precedence.bminor
```

Use a file containing exactly one expression without a trailing semicolon. The parser skips comments. Indentation shows child nodes, and `@line:column` shows the start of each node’s source span. If parsing fails, the command reports the first error to standard error, exits with a nonzero status, and produces no AST. Parsing full B-Minor files and recovering between statements are later milestones.

## Run the regression tests

Build first, then run from the project root:

```sh
sh tests/test-source-copy.sh
sh tests/test-core-lexer.sh
sh tests/test-literals-comments.sh
sh tests/test-expression-parser.sh
```

`test-source-copy.sh` compares all source fixtures byte for byte and checks file and usage errors. `test-core-lexer.sh` covers the core token kinds and positions. `test-literals-comments.sh` checks escaped literals, comment boundaries, and recovery after malformed input. `test-expression-parser.sh` compares AST shapes and checks source positions and diagnostics.

## Source layout

`src/lexer.h` defines token and location types, and `src/lexer.cpp` scans source text. `src/parser.h` defines the expression AST, and `src/parser.cpp` implements the expression grammar. `src/main.cpp` handles file I/O, command-line options, and output.

See [lexical rules](docs/lexical-rules.md), [expression grammar](docs/expression-grammar.md), and [test inputs](assets/README.md) for checks by milestone.

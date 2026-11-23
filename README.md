# B-Minor Phase 1

The Phase 1 lexer recognizes B-Minor tokens, including string and character literals and both forms of comments. It reports lexical errors with their source positions and continues to the end of the file. The token stream is ready for the parser milestone.

## Build on Linux

Use a C++17 compiler. From the project root, run these commands with GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp src/lexer.cpp -o build/bminor
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

## Run the regression tests

Build first, then run from the project root:

```sh
sh tests/test-source-copy.sh
sh tests/test-core-lexer.sh
sh tests/test-literals-comments.sh
```

`test-source-copy.sh` compares all source fixtures byte for byte and checks file and usage errors. `test-core-lexer.sh` covers the core token kinds and positions. `test-literals-comments.sh` checks escaped literals, comment boundaries, and recovery after malformed input.

## Source layout

`src/lexer.h` defines the token and location types. `src/lexer.cpp` scans source text, and `src/main.cpp` handles file I/O, command-line options, and token output.

See [lexical rules](docs/lexical-rules.md) for the token definitions and [test inputs](assets/README.md) for checks by milestone.

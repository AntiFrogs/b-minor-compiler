# B-Minor Phase 1

This version adds a token dump for core B-Minor syntax. It recognizes identifiers, keywords, decimal integers, booleans, operators, delimiters, and whitespace. The lexer reports invalid bytes with their source positions and continues to the end of the file. String literals, character literals, and comments are the next lexer milestone.

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

Dump tokens from a source file that uses the supported token kinds:

```sh
./build/bminor --tokens assets/valid/token-boundaries.bminor
```

Each line gives the filename, 1-based line and column, token kind, and quoted source spelling. For example:

```text
assets/valid/token-boundaries.bminor:1:1 IDENTIFIER "returnValue"
```

Token mode prints an `EOF` token after the last input byte. Invalid bytes produce `INVALID` tokens on standard output and diagnostics on standard error. The dump displays nonprintable and non-ASCII bytes as `\xHH`. The command exits with a nonzero status if it encountered any invalid bytes. Usage and file errors also go to standard error and return a nonzero status.

Copy mode preserves the input bytes, including line endings and a missing final newline. An empty file produces empty output in copy mode and one `EOF` token in token mode.

## Run the regression tests

Build first, then run from the project root:

```sh
sh tests/test-source-copy.sh
sh tests/test-core-lexer.sh
```

The first script compares all source fixtures byte for byte and checks file and usage errors. The second checks the operator set, token boundaries and positions, separate sign tokens, and recovery after invalid input.

## Source layout

`src/lexer.h` defines the token and location types. `src/lexer.cpp` scans source text, and `src/main.cpp` handles file I/O, command-line options, and token output.

See [lexical rules](docs/lexical-rules.md) for the token definitions and [test inputs](assets/README.md) for checks by milestone.

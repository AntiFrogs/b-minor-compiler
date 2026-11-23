# B-Minor Phase 1

The Phase 1 front end parses B-Minor declarations and statements into an abstract syntax tree (AST) and uses it to highlight source in the terminal or a standalone HTML page. It also exposes the token stream and can parse a standalone expression.

## Build on Linux

Use a C++17 compiler. From the project root, run these commands with GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -pedantic src/*.cpp -o build/bminor
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

Use a file containing exactly one expression without a trailing semicolon.

Parse declarations and function bodies with the program entry point:

```sh
./build/bminor --ast assets/valid/declarations.bminor
./build/bminor --ast assets/valid/statements.bminor
```

`--ast` accepts global variables, arrays, function prototypes, and definitions. Function bodies support blocks, local declarations, expression statements, `return`, `if`/`else`, `for`, and `print`. Each `for` clause is optional, and `print` takes a comma-separated list of expressions or an empty list.

Both AST modes skip comments. Indentation shows child nodes, and `@line:column` shows the start of each node’s source span. The expression mode reports its first error and exits without a tree. The program mode reports errors to standard error and writes a partial tree to standard output. Both modes return a nonzero exit status on errors.

Try a file with two malformed declarations:

```sh
./build/bminor --ast assets/invalid/parser/multiple-errors.bminor
```

Expect two diagnostics, a nonzero exit status, and a tree containing `first_ok` and `second_ok`.

### Highlight source in the terminal

```sh
./build/bminor --ansi assets/valid/highlighting.bminor
./build/bminor --ansi assets/invalid/lexer/unknown-character.bminor
```

`--ansi` writes styled source to standard output, including when redirected. Function names at prototypes, definitions, and direct calls are gold. Variables use the terminal’s default foreground. Diagnostic tokens are red and underlined. On malformed input, the command still writes the highlighted source and returns a nonzero status. Diagnostics go to standard error.

The output preserves indentation, line endings, and the presence or absence of a final newline. Most ASCII control bytes display as `\xHH`. The original copy mode can reproduce arbitrary bytes. See [highlighting rules](docs/highlighting.md) for the full style mapping.

### Generate a standalone HTML page

```sh
./build/bminor --html assets/valid/highlighting.bminor > build/highlighting.html
```

Open `build/highlighting.html` in a browser. It uses the same source classifications as `--ansi`, with embedded CSS and a preformatted code block. It works offline without JavaScript. The shell redirection saves the page. On malformed input, the command still generates HTML, writes diagnostics to standard error, and returns a nonzero status.

## Run the regression tests

Build first, then run from the project root:

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

`test-source-copy.sh` compares all source fixtures byte for byte and checks file and usage errors. `test-core-lexer.sh` covers core token kinds and positions. `test-literals-comments.sh` checks literals, comments, and lexical recovery. `test-expression-parser.sh` and `test-program-parser.sh` check expression and declaration ASTs and diagnostics. `test-statements.sh` checks that all valid program fixtures parse, and covers branch binding, optional loop clauses, print arguments, and malformed statements.

`test-parser-recovery.sh` checks multiple diagnostics and the contents of partial trees. The Python 3 highlighting tests check source preservation, styles, HTML escaping, and output after malformed input. The compiler itself only requires standard C++17.

## Source layout

`src/lexer.h` defines token and location types, and `src/lexer.cpp` scans source text. `src/parser.h` defines the AST, and `src/parser.cpp` implements the grammar. `src/dump.cpp` renders the AST as text. `src/highlight.cpp` assigns source styles, and `src/ansi.cpp` and `src/html.cpp` render them. `src/main.cpp` handles file I/O, command-line options, and diagnostics.

See the [lexical rules](docs/lexical-rules.md), [expression grammar](docs/expression-grammar.md), [declaration and statement grammar](docs/program-grammar.md), and [test inputs](assets/README.md).

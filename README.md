# B-Minor Phase 1

The initial command-line version reads a B-Minor source file and copies its contents to standard output. Usage and file errors go to standard error and produce a nonzero exit status.

## Build on Linux

Use a C++17 compiler. From the project root, run these commands with GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp -o build/bminor
```

## Run

Pass the path to an existing B-Minor source file:

```sh
./build/bminor path/to/example.bminor
```

An empty file succeeds with empty output. The program preserves the original line endings and the presence or absence of a final newline.

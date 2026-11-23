#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/program-parser.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
actual="$temp_dir/ast"
errors="$temp_dir/errors"

check_tree() {
    ./build/bminor --ast "$1" > "$actual"
    diff -u - "$actual"
}

expect_line() {
    if ! grep -Fqx -e "$1" "$actual"
    then
        printf 'Expected AST line: %s\n' "$1" >&2
        cat "$actual" >&2
        exit 1
    fi
}

check_error() {
    if ./build/bminor --ast "$1" > "$actual" 2> "$errors"
    then
        printf 'Expected a program parse error for %s\n' "$1" >&2
        exit 1
    fi
    grep -q '^Program @' "$actual"
    printf '%s\n' "$2" > "$temp_dir/expected"
    diff -u "$temp_dir/expected" "$errors"
}

check_tree assets/valid/declarations.bminor <<'TREE'
Program @1:1
  FunctionPrototype "read_item" @1:1
    ReturnType "integer" @1:21
    Parameter "index" @1:30
      Type "integer" @1:37
  VarDecl "values" @3:1
    Type "integer" @3:9
      ArrayDimension @3:9
        Literal INTEGER "3" @3:16
    InitializerList @3:29
      Literal INTEGER "7" @3:30
      Literal INTEGER "11" @3:33
      Literal INTEGER "13" @3:37
  VarDecl "scratch" @4:1
    Type "integer" @4:10
      ArrayDimension @4:10
        Literal INTEGER "2" @4:17
  FunctionDefinition "read_item" @6:1
    ReturnType "integer" @6:21
    Parameter "index" @6:30
      Type "integer" @6:37
    Block @6:48
      ReturnStmt @7:5
        SubscriptExpr @7:12
          Identifier "values" @7:12
          Identifier "index" @7:19
TREE
printf 'PASS declarations, arrays, function prototypes, and definitions\n'

./build/bminor --ast assets/valid/function-bodies.bminor > "$actual"
expect_line '    BinaryExpr "+" @1:17'
expect_line '      BinaryExpr "*" @1:21'
expect_line '      ArrayDimension @2:9'
expect_line '      ArrayDimension @2:19'
expect_line '      InitializerList @2:40'
expect_line '      InitializerList @2:48'
expect_line '      Type "integer" @4:33'
expect_line '        ArrayDimension @4:33'
expect_line '      VarDecl "current" @7:5'
expect_line '        SubscriptExpr @7:37'
expect_line '      Block @8:5'
expect_line '        VarDecl "next" @9:9'
expect_line '        ExprStmt @10:9'
expect_line '          AssignExpr "=" @10:9'
expect_line '      ExprStmt @12:5'
expect_line '        CallExpr @12:5'
expect_line '      ReturnStmt @13:5'
expect_line '        Identifier "current" @13:12'
expect_line '  FunctionDefinition "reset" @16:1'
expect_line '      ReturnStmt @17:5'
printf 'PASS nested arrays, array parameters, local declaration lookahead, and basic statements\n'

check_tree assets/io/empty.bminor <<'TREE'
Program @1:1
TREE
printf 'PASS empty program\n'

source=assets/invalid/parser/missing-expression.bminor
check_error "$source" "$source:1:19: error: expected expression, found \";\""
source=assets/invalid/parser/truncated-block.bminor
check_error "$source" "$source:3:1: error: expected '}' after block, found end of input"
source=assets/invalid/lexer/unknown-character.bminor
check_error "$source" "$source:1:20: error: invalid byte \"@\""

source="$temp_dir/invalid.bminor"
printf '%s\n' 'x integer;' > "$source"
check_error "$source" "$source:1:3: error: expected ':' after declaration name, found \"integer\""
printf '%s\n' 'x: array [3 integer;' > "$source"
check_error "$source" "$source:1:13: error: expected ']' after array dimension, found \"integer\""
printf '%s\n' 'x: integer = {1};' > "$source"
check_error "$source" "$source:1:14: error: expected expression for scalar initializer, found \"{\""
printf '%s\n' 'f: function void (x: array [1] integer);' > "$source"
check_error "$source" "$source:1:29: error: expected ']' for unsized array parameter, found \"1\""
printf '%s\n' 'f: function void () = { x: integer = 1 }' > "$source"
check_error "$source" "$source:1:40: error: expected ';' after variable declaration, found \"}\""
printf 'PASS source-located lexical and declaration diagnostics\n'

#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/parser-recovery.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
tree="$temp_dir/ast"
errors="$temp_dir/errors"

recover() {
    if ./build/bminor --ast "$1" > "$tree" 2> "$errors"
    then
        printf 'Expected a failure status for %s\n' "$1" >&2
        exit 1
    fi
    grep -q '^Program @' "$tree"
}

source=assets/invalid/parser/multiple-errors.bminor
recover "$source"
diff -u - "$errors" <<EXPECTED
$source:1:22: error: expected expression, found ";"
$source:3:23: error: expected expression, found ";"
EXPECTED
diff -u - "$tree" <<'TREE'
Program @1:1
  VarDecl "first_ok" @2:1
    Type "integer" @2:11
    Literal INTEGER "1" @2:21
  VarDecl "second_ok" @4:1
    Type "integer" @4:12
    Literal INTEGER "2" @4:22
TREE
printf 'PASS independent declaration errors and preserved declarations\n'

source=assets/invalid/parser/block-recovery.bminor
recover "$source"
diff -u - "$errors" <<EXPECTED
$source:2:27: error: expected expression, found ";"
$source:5:34: error: expected expression, found ";"
$source:8:16: error: expected ';' after for initializer, found "i"
EXPECTED
diff -u - "$tree" <<'TREE'
Program @1:1
  FunctionDefinition "recover" @1:1
    ReturnType "integer" @1:19
    Parameter "i" @1:28
      Type "integer" @1:31
    Block @1:42
      PrintStmt @3:5
        Literal STRING "\"after first error\"" @3:11
      IfStmt @4:5
        Literal BOOLEAN "true" @4:9
        Block @4:15
          PrintStmt @6:9
            Identifier "i" @6:15
      ExprStmt @11:5
        AssignExpr "=" @11:5
          Identifier "i" @11:5
          BinaryExpr "+" @11:9
            Identifier "i" @11:9
            Literal INTEGER "1" @11:13
      ReturnStmt @12:5
        Identifier "i" @12:12
  VarDecl "after" @14:1
    Type "integer" @14:8
    Literal INTEGER "7" @14:18
TREE
printf 'PASS nested block and malformed for header synchronization\n'

source=assets/invalid/parser/missing-right-paren.bminor
recover "$source"
printf '%s:2:14: error: expected %s after if condition, found "{"\n' "$source" "')'" | diff -u - "$errors"
grep -Fqx '      IfStmt @2:5' "$tree"
grep -Fqx '          ReturnStmt @3:9' "$tree"
grep -Fqx '      ReturnStmt @5:5' "$tree"
grep -Fqx '  VarDecl "after" @7:1' "$tree"

source=assets/invalid/parser/truncated-block.bminor
recover "$source"
printf '%s:3:1: error: expected %s after block, found end of input\n' "$source" "'}'" | diff -u - "$errors"
grep -Fqx '  FunctionDefinition "incomplete" @1:1' "$tree"
grep -Fqx '      ReturnStmt @2:5' "$tree"
printf 'PASS missing control delimiter and partial block at EOF\n'

source="$temp_dir/mixed.bminor"
cat > "$source" <<'SOURCE'
broken: integer = 1 @ #;
missing_semicolon: integer = 2
next: integer = 3;
bad_array: array [2] integer = {1, ;
tail: integer = 4;
}
last: integer = 5;
SOURCE
recover "$source"
diff -u - "$errors" <<EXPECTED
$source:1:21: error: invalid byte "@"
$source:1:23: error: invalid byte "#"
$source:3:1: error: expected ';' after variable declaration, found "next"
$source:4:36: error: expected expression, found ";"
$source:6:1: error: expected declaration name, found "}"
EXPECTED
grep -Fqx '  VarDecl "next" @3:1' "$tree"
grep -Fqx '  VarDecl "tail" @5:1' "$tree"
grep -Fqx '  VarDecl "last" @7:1' "$tree"
test "$(grep -c '^  VarDecl ' "$tree")" -eq 3
printf 'PASS lexical errors during synchronization, declaration lookahead, and unmatched delimiters\n'

source="$temp_dir/signature.bminor"
cat > "$source" <<'SOURCE'
bad: function void (first integer, ignored: integer) = {
    local: integer = 9;
}
good: integer = 6;
SOURCE
recover "$source"
printf '%s:1:27: error: expected %s after parameter name, found "integer"\n' "$source" "':'" | diff -u - "$errors"
diff -u - "$tree" <<'TREE'
Program @1:1
  VarDecl "good" @4:1
    Type "integer" @4:7
    Literal INTEGER "6" @4:17
TREE
printf 'PASS malformed function signature and following declaration\n'

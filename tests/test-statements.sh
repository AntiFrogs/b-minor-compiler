#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/statements.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
actual="$temp_dir/ast"
errors="$temp_dir/errors"

check_error() {
    if ./build/bminor --ast "$1" > "$actual" 2> "$errors"
    then
        printf 'Expected a statement parse error for %s\n' "$1" >&2
        exit 1
    fi
    grep -q '^Program @' "$actual"
    printf '%s\n' "$2" > "$temp_dir/expected"
    diff -u "$temp_dir/expected" "$errors"
}

for source in assets/valid/*.bminor
do
    ./build/bminor --ast "$source" > "$actual"
    grep -q '^Program @' "$actual"
done
printf 'PASS full-program parsing for every valid program fixture\n'

./build/bminor --ast assets/valid/control-flow.bminor > "$actual"
diff -u - "$actual" <<'TREE'
Program @1:1
  FunctionDefinition "choose" @1:1
    ReturnType "integer" @1:18
    Parameter "outer" @1:27
      Type "boolean" @1:34
    Parameter "inner" @1:43
      Type "boolean" @1:50
    Block @1:61
      IfStmt @2:5
        Identifier "outer" @2:9
        IfStmt @3:9
          Identifier "inner" @3:13
          PrintStmt @3:20
            Literal STRING "\"inner\"" @3:26
          PrintStmt @4:14
            Literal STRING "\"near\"" @4:20
        IfStmt @5:10
          Identifier "inner" @5:14
          PrintStmt @6:9
            Literal STRING "\"alternate\"" @6:15
          PrintStmt @8:9
      ReturnStmt @9:5
        Literal INTEGER "0" @9:12
  FunctionDefinition "loops" @12:1
    ReturnType "void" @12:17
    Parameter "n" @12:23
      Type "integer" @12:26
    Block @12:37
      VarDecl "i" @13:5
        Type "integer" @13:8
        Literal INTEGER "0" @13:18
      ForStmt @14:5
        ForCondition @14:12
          BinaryExpr "<" @14:12
            Identifier "i" @14:12
            Identifier "n" @14:16
        ForUpdate @14:19
          PostfixExpr "++" @14:19
            Identifier "i" @14:19
        PrintStmt @14:24
          Identifier "i" @14:30
      ForStmt @15:5
        ForInitializer @15:10
          AssignExpr "=" @15:10
            Identifier "i" @15:10
            Literal INTEGER "0" @15:14
        ReturnStmt @15:19
      ForStmt @16:5
        ForUpdate @16:13
          PostfixExpr "++" @16:13
            Identifier "i" @16:13
        Block @16:18
      ForStmt @17:5
        Block @17:14
          PrintStmt @18:9
          ReturnStmt @19:9
TREE
printf 'PASS nearest-if binding, else-if, and omitted for clauses\n'

./build/bminor --ast assets/valid/statements.bminor > "$actual"
awk '/PrintStmt @9:13$/ { remaining = 4 } remaining > 0 { print; --remaining }' "$actual" > "$temp_dir/print"
diff -u - "$temp_dir/print" <<'TREE'
              PrintStmt @9:13
                Literal STRING "\"skipped: \"" @9:19
                Identifier "i" @9:32
                Literal STRING "\"\\n\"" @9:35
TREE
printf 'PASS print argument order and source positions\n'

source=assets/invalid/parser/missing-right-paren.bminor
check_error "$source" "$source:2:14: error: expected ')' after if condition, found \"{\""
source="$temp_dir/invalid.bminor"
printf '%s\n' 'f: function void () = {' '    for (i = 0 i < 1; i++) return;' '}' > "$source"
check_error "$source" "$source:2:16: error: expected ';' after for initializer, found \"i\""
printf '%s\n' 'f: function void () = {' '    print 1,;' '}' > "$source"
check_error "$source" "$source:2:13: error: expected expression, found \";\""
printf '%s\n' 'f: function void () = {' '    print 1' '}' > "$source"
check_error "$source" "$source:3:1: error: expected ';' after print statement, found \"}\""
printf 'PASS malformed if, for, and print diagnostics\n'

source="$temp_dir/deeply-nested.bminor"
awk 'BEGIN {
    print "f: function void () = {"
    for (i = 0; i < 260; ++i) print "    if (true)"
    print "    return;"
    print "}"
}' > "$source"
check_error "$source" "$source:257:5: error: statement nesting limit exceeded, found \"if\""
printf 'PASS statement nesting guard\n'

#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/expression-parser.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
actual="$temp_dir/ast"
errors="$temp_dir/errors"

check_tree() {
    ./build/bminor --expr-ast "$1" > "$actual"
    diff -u - "$actual"
}

check_error() {
    if ./build/bminor --expr-ast "$1" > "$actual" 2> "$errors"
    then
        printf 'Expected a parse error for %s\n' "$1" >&2
        exit 1
    fi
    test ! -s "$actual"
    printf '%s\n' "$2" > "$temp_dir/expected"
    diff -u "$temp_dir/expected" "$errors"
}

check_tree assets/expressions/precedence.bminor <<'TREE'
BinaryExpr "-" @1:1
  BinaryExpr "-" @1:1
    BinaryExpr "+" @1:1
      Literal INTEGER "2" @1:1
      BinaryExpr "*" @1:5
        Literal INTEGER "3" @1:5
        Literal INTEGER "4" @1:9
    BinaryExpr "-" @1:13
      Literal INTEGER "5" @1:14
      Literal INTEGER "6" @1:18
  Literal INTEGER "7" @1:23
TREE
printf 'PASS arithmetic precedence, left associativity, and grouping\n'

check_tree assets/expressions/assignment.bminor <<'TREE'
AssignExpr "=" @1:1
  Identifier "a" @1:1
  AssignExpr "=" @1:5
    Identifier "b" @1:5
    BinaryExpr "^" @1:9
      PrefixExpr "-" @1:9
        Literal INTEGER "2" @1:10
      BinaryExpr "^" @1:14
        Literal INTEGER "3" @1:14
        Literal INTEGER "4" @1:18
TREE
printf 'PASS right-associative assignment and power, and prefix precedence\n'

check_tree assets/expressions/postfix.bminor <<'TREE'
BinaryExpr "+" @1:1
  PostfixExpr "++" @1:1
    SubscriptExpr @1:1
      Identifier "items" @1:1
      CallExpr @1:7
        Identifier "index" @1:7
        BinaryExpr "+" @1:24
          Literal INTEGER "1" @1:24
          Literal INTEGER "2" @1:28
  CallExpr @1:36
    Identifier "size" @1:36
TREE
printf 'PASS calls, subscripts, postfix operators, and comment skipping\n'

check_tree assets/expressions/logic-and-literals.bminor <<'TREE'
BinaryExpr "||" @1:1
  PrefixExpr "!" @1:1
    CallExpr @1:2
      Identifier "done" @1:2
  BinaryExpr "&&" @1:12
    CallExpr @1:12
      Identifier "check" @1:12
      Literal STRING "\"A\"" @1:18
      Literal CHARACTER "'\\n'" @1:23
      Literal BOOLEAN "true" @1:29
    BinaryExpr "==" @1:38
      BinaryExpr "<" @1:38
        Identifier "lower" @1:38
        Identifier "upper" @1:46
      Literal BOOLEAN "true" @1:55
TREE
printf 'PASS logical precedence, comparisons, and literal arguments\n'

source=assets/expressions/invalid/missing-operand.bminor
check_error "$source" "$source:1:5: error: expected expression, found \")\""
source=assets/expressions/invalid/missing-call-close.bminor
check_error "$source" "$source:2:1: error: expected ')' after arguments, found end of input"
source=assets/expressions/invalid/missing-subscript-close.bminor
check_error "$source" "$source:1:7: error: expected ']' after subscript, found \")\""

source="$temp_dir/invalid.bminor"
printf '%s\n' 'f(1,)' > "$source"
check_error "$source" "$source:1:5: error: expected expression, found \")\""
printf '%s\n' '1 2' > "$source"
check_error "$source" "$source:1:3: error: expected end of input after expression, found \"2\""
printf '%s\n' '1 + @' > "$source"
check_error "$source" "$source:1:5: error: invalid byte \"@\""
printf 'PASS diagnostics for malformed expressions and lexical errors\n'

source="$temp_dir/nesting.bminor"
awk 'BEGIN {
    for (i = 0; i < 260; ++i) printf "("
    printf "1"
    for (i = 0; i < 260; ++i) printf ")"
    print ""
}' > "$source"
check_error "$source" "$source:1:258: error: expression nesting limit exceeded, found \"(\""

source="$temp_dir/long-chain.bminor"
awk 'BEGIN {
    printf "1"
    for (i = 1; i < 384; ++i) printf "+1"
    print ""
}' > "$source"
./build/bminor --expr-ast "$source" > "$actual"
test "$(wc -l < "$actual")" -eq 767
printf 'PASS nesting guard and long left-associative chains\n'

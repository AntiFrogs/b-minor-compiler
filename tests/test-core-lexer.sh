#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/core-lexer.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
tokens="$temp_dir/tokens"
errors="$temp_dir/errors"

expect_line() {
    if ! grep -Fqx -e "$2" "$1"
    then
        printf 'Expected line: %s\n' "$2" >&2
        cat "$1" >&2
        exit 1
    fi
}

source=assets/valid/token-boundaries.bminor
./build/bminor --tokens "$source" > "$tokens"
expect_line "$tokens" "$source:1:1 IDENTIFIER \"returnValue\""
expect_line "$tokens" "$source:1:12 DELIMITER \":\""
expect_line "$tokens" "$source:1:32 IDENTIFIER \"_count\""
expect_line "$tokens" "$source:1:49 IDENTIFIER \"x1\""
expect_line "$tokens" "$source:2:5 KEYWORD \"return\""
expect_line "$tokens" "$source:2:19 OPERATOR \"<=\""
expect_line "$tokens" "$source:2:21 IDENTIFIER \"x1\""
expect_line "$tokens" "$source:2:31 OPERATOR \"||\""
expect_line "$tokens" "$source:4:1 EOF \"\""
printf 'PASS identifiers, keywords, and token boundaries\n'

source="$temp_dir/operators.bminor"
{
    column=1
    for operator in '++' '--' '<=' '>=' '==' '!=' '&&' '||' '!' '^' '*' '/' '%' '+' '-' '<' '>' '='
    do
        printf '%s:1:%s OPERATOR "%s"\n' "$source" "$column" "$operator"
        column=$((column + ${#operator} + 1))
    done
    printf '%s:2:1 EOF ""\n' "$source"
} > "$temp_dir/expected"
printf '%s\n' '++ -- <= >= == != && || ! ^ * / % + - < > =' > "$source"
./build/bminor --tokens "$source" > "$tokens"
diff -u "$temp_dir/expected" "$tokens"
printf 'PASS all core operators and longest match\n'

source="$temp_dir/literals.bminor"
printf 'true false trueValue\t042\rnext\r\nend' > "$source"
./build/bminor --tokens "$source" > "$tokens"
expect_line "$tokens" "$source:1:1 BOOLEAN \"true\""
expect_line "$tokens" "$source:1:6 BOOLEAN \"false\""
expect_line "$tokens" "$source:1:12 IDENTIFIER \"trueValue\""
expect_line "$tokens" "$source:1:22 INTEGER \"042\""
expect_line "$tokens" "$source:2:1 IDENTIFIER \"next\""
expect_line "$tokens" "$source:3:1 IDENTIFIER \"end\""
expect_line "$tokens" "$source:3:4 EOF \"\""

source=assets/dialect/adjacent-sign.bminor
./build/bminor --tokens "$source" > "$tokens"
expect_line "$tokens" "$source:2:25 IDENTIFIER \"x\""
expect_line "$tokens" "$source:2:26 OPERATOR \"-\""
expect_line "$tokens" "$source:2:27 INTEGER \"1\""
expect_line "$tokens" "$source:3:25 OPERATOR \"-\""
expect_line "$tokens" "$source:3:26 INTEGER \"1\""

source=assets/io/crlf.bminor
./build/bminor --tokens "$source" > "$tokens"
expect_line "$tokens" "$source:3:1 IDENTIFIER \"second\""
source=assets/io/empty.bminor
./build/bminor --tokens "$source" > "$tokens"
printf '%s:1:1 EOF ""\n' "$source" > "$temp_dir/expected"
diff -u "$temp_dir/expected" "$tokens"
printf 'PASS literals, signs, whitespace positions, and EOF\n'

source=assets/invalid/lexer/unknown-character.bminor
if ./build/bminor --tokens "$source" > "$tokens" 2> "$errors"
then
    printf 'Expected --tokens to fail for %s\n' "$source" >&2
    exit 1
fi
expect_line "$tokens" "$source:1:20 INVALID \"@\""
expect_line "$tokens" "$source:2:1 IDENTIFIER \"after\""
expect_line "$tokens" "$source:3:1 EOF \"\""
expect_line "$errors" "$source:1:20: error: invalid byte \"@\""

source="$temp_dir/invalid-bytes.bminor"
printf '\000\377&|x' > "$source"
if ./build/bminor --tokens "$source" > "$tokens" 2> "$errors"
then
    printf 'Expected --tokens to fail for %s\n' "$source" >&2
    exit 1
fi
expect_line "$tokens" "$source:1:1 INVALID \"\x00\""
expect_line "$tokens" "$source:1:2 INVALID \"\xFF\""
expect_line "$tokens" "$source:1:3 INVALID \"&\""
expect_line "$tokens" "$source:1:4 INVALID \"|\""
expect_line "$tokens" "$source:1:5 IDENTIFIER \"x\""
expect_line "$tokens" "$source:1:6 EOF \"\""
test "$(wc -l < "$errors")" -eq 4
printf 'PASS invalid bytes, diagnostics, and recovery\n'

#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/literals-comments.XXXXXX)
trap 'rm -rf "$temp_dir"' 0
tokens="$temp_dir/tokens"
errors="$temp_dir/errors"
expected="$temp_dir/expected"

expect_line() {
    if ! grep -Fqx -e "$2" "$1"
    then
        printf 'Expected line: %s\n' "$2" >&2
        cat "$1" >&2
        exit 1
    fi
}

scan_invalid() {
    if ./build/bminor --tokens "$1" > "$tokens" 2> "$errors"
    then
        printf 'Expected lexical errors in %s\n' "$1" >&2
        exit 1
    fi
}

source=assets/valid/literals-and-comments.bminor
./build/bminor --tokens "$source" > "$tokens"
grep -E ' (STRING|CHARACTER|LINE_COMMENT|BLOCK_COMMENT|EOF) |:13:1 IDENTIFIER ' "$tokens" > "$temp_dir/selected"
cat > "$expected" <<'TOKENS'
assets/valid/literals-and-comments.bminor:1:17 STRING "\"A quote: \\\" and a backslash: \\\\\""
assets/valid/literals-and-comments.bminor:2:17 CHARACTER "'\\n'"
assets/valid/literals-and-comments.bminor:3:15 CHARACTER "'\\''"
assets/valid/literals-and-comments.bminor:4:14 CHARACTER "'Z'"
assets/valid/literals-and-comments.bminor:8:22 STRING "\"\""
assets/valid/literals-and-comments.bminor:10:1 LINE_COMMENT "// return 99; this is a comment."
assets/valid/literals-and-comments.bminor:11:1 BLOCK_COMMENT "/* This comment spans\x0A   two lines. */"
assets/valid/literals-and-comments.bminor:13:1 IDENTIFIER "marker"
assets/valid/literals-and-comments.bminor:13:18 STRING "\"// and /* are string contents */\""
assets/valid/literals-and-comments.bminor:14:1 EOF ""
TOKENS
diff -u "$expected" "$temp_dir/selected"

source="$temp_dir/escapes.bminor"
cat > "$source" <<'SOURCE'
'\0' '\q' '\\' '"'
SOURCE
./build/bminor --tokens "$source" > "$tokens"
cat > "$expected" <<TOKENS
$source:1:1 CHARACTER "'\\\\0'"
$source:1:6 CHARACTER "'\\\\q'"
$source:1:11 CHARACTER "'\\\\\\\\'"
$source:1:16 CHARACTER "'\\"'"
$source:2:1 EOF ""
TOKENS
diff -u "$expected" "$tokens"
printf 'PASS quoted literals, escapes, and their exact spellings\n'

source="$temp_dir/comment-boundaries.bminor"
printf '%s\r\n' '1/**/ /2 // comment with "quotes"' '/* first' '   /* second */3' > "$source"
./build/bminor --tokens "$source" > "$tokens"
cat > "$expected" <<TOKENS
$source:1:1 INTEGER "1"
$source:1:2 BLOCK_COMMENT "/**/"
$source:1:7 OPERATOR "/"
$source:1:8 INTEGER "2"
$source:1:10 LINE_COMMENT "// comment with \\"quotes\\""
$source:2:1 BLOCK_COMMENT "/* first\\x0D\\x0A   /* second */"
$source:3:16 INTEGER "3"
$source:4:1 EOF ""
TOKENS
diff -u "$expected" "$tokens"

source=assets/io/no-final-newline.bminor
./build/bminor --tokens "$source" > "$tokens"
expect_line "$tokens" "$source:1:21 LINE_COMMENT \"// comment ends at EOF\""
expect_line "$tokens" "$source:1:43 EOF \"\""
printf 'PASS comment boundaries, division, CRLF locations, and comment at EOF\n'

source=assets/invalid/lexer/unterminated-string.bminor
scan_invalid "$source"
printf '%s:1:19: error: unterminated string literal\n' "$source" > "$expected"
diff -u "$expected" "$errors"
expect_line "$tokens" "$source:1:19 INVALID \"\\\"unfinished\""
expect_line "$tokens" "$source:1:30 EOF \"\""

source=assets/invalid/lexer/unterminated-block-comment.bminor
scan_invalid "$source"
printf '%s:2:1: error: unterminated block comment\n' "$source" > "$expected"
diff -u "$expected" "$errors"
expect_line "$tokens" "$source:2:1 INVALID \"/* unfinished\x0A   after: integer = 7;\x0A\""
expect_line "$tokens" "$source:4:1 EOF \"\""
printf 'PASS unclosed string and block comment at EOF\n'

source=assets/invalid/lexer/literal-recovery.bminor
scan_invalid "$source"
cat > "$expected" <<ERRORS
$source:1:22: error: unterminated string literal
$source:3:20: error: character literal must contain exactly one character
$source:4:19: error: character literal must contain exactly one character
$source:5:18: error: unterminated character literal
ERRORS
diff -u "$expected" "$errors"
expect_line "$tokens" "$source:2:1 IDENTIFIER \"after_string\""
expect_line "$tokens" "$source:6:1 IDENTIFIER \"after_char\""
expect_line "$tokens" "$source:7:1 EOF \"\""

source="$temp_dir/malformed-escapes.bminor"
printf '"a\000b" "bad\\\r\nok' > "$source"
scan_invalid "$source"
cat > "$expected" <<ERRORS
$source:1:1: error: literal contains a byte outside printable ASCII
$source:1:7: error: unterminated string literal
ERRORS
diff -u "$expected" "$errors"
expect_line "$tokens" "$source:2:1 IDENTIFIER \"ok\""
expect_line "$tokens" "$source:2:3 EOF \"\""
printf 'PASS malformed literals and recovery on the following line\n'

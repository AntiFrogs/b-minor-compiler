#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
temp_dir=$(mktemp -d build/source-copy.XXXXXX)
trap 'rm -rf "$temp_dir"' 0

for source in assets/io/*.bminor assets/valid/*.bminor assets/invalid/*/*.bminor assets/dialect/*.bminor
do
    ./build/bminor "$source" > "$temp_dir/copy"
    cmp "$source" "$temp_dir/copy"
    printf 'PASS copy %s\n' "$source"
done

if ./build/bminor > "$temp_dir/output" 2> "$temp_dir/error"
then
    printf 'Expected an error when no input path is supplied\n' >&2
    exit 1
fi
test ! -s "$temp_dir/output"
grep -Fq 'Usage: bminor' "$temp_dir/error"

missing=assets/io/does-not-exist.bminor
if ./build/bminor "$missing" > "$temp_dir/output" 2> "$temp_dir/error"
then
    printf 'Expected a file error for %s\n' "$missing" >&2
    exit 1
fi
test ! -s "$temp_dir/output"
grep -Fq "$missing" "$temp_dir/error"
printf 'PASS copy mode diagnostics\n'

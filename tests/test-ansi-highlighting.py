from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
PROGRAM = ROOT / "build/bminor"
ASSETS = ROOT / "assets"
SGR = re.compile(rb"\x1b\[[0-9;]+m")
RESET = b"\x1b[0m"


def highlight(source):
    return subprocess.run(
        [PROGRAM, "--ansi", source],
        cwd=ROOT,
        capture_output=True,
        timeout=10,
    )


def styled(code, text):
    return b"\x1b[" + code + b"m" + text + RESET


def assert_balanced_styles(output, path):
    active = False
    for match in SGR.finditer(output):
        if match[0] == RESET:
            assert active, f"Unexpected reset in {path}"
            active = False
        else:
            assert not active, f"Style changed before reset in {path}"
            active = True
    assert not active, f"Missing final reset in {path}"


def main():
    for source in sorted(ASSETS.rglob("*.bminor")):
        result = highlight(source)
        assert result.returncode in (0, 1), (source, result.returncode)
        assert bool(result.stderr) == (result.returncode == 1), source
        assert_balanced_styles(result.stdout, source)
        assert SGR.sub(b"", result.stdout) == source.read_bytes(), source
        if source.parent == ASSETS / "valid":
            assert result.returncode == 0, (source, result.stderr)
    print("PASS source bytes and balanced styles across all fixtures")

    result = highlight(ASSETS / "valid/highlight-roles.bminor")
    output = result.stdout
    function = styled(b"33", b"repeat")
    assert result.returncode == 0 and output.count(function) == 4, output
    assert output.count(function + b":") == 2, output
    assert b"(" + function + b")(" in output, output
    assert function + b" " + styled(b"2;3;90", b"/* before arguments */") + b" (" in output, output
    assert b"\n        repeat: " in output and styled(b"1;34", b"print") + b" repeat;" in output
    print("PASS AST-based prototypes, definitions, and calls with same-spelled variables")

    sample = highlight(ASSETS / "valid/highlighting.bminor").stdout
    for code, token in [
        (b"1;34", b"function"),
        (b"36", b"integer"),
        (b"33", b"double_value"),
        (b"38;5;208", b"3"),
        (b"32", b'"<tag>&"'),
        (b"32", rb"'\n'"),
        (b"37", b"*"),
        (b"2;3;90", b'/* Render <tag> & keep "quotes". */'),
    ]:
        assert styled(code, token) in sample, (code, token)
    literals = highlight(ASSETS / "valid/literals-and-comments.bminor").stdout
    assert styled(b"38;5;208", b"true") in literals
    assert styled(b"2;3;90", b"// return 99; this is a comment.") in literals
    print("PASS keyword, type, literal, operator, and comment styles")

    lexical = highlight(ASSETS / "invalid/lexer/unknown-character.bminor")
    assert lexical.returncode == 1 and b'invalid byte "@"' in lexical.stderr
    assert styled(b"4;31", b"@") in lexical.stdout
    assert b"after: " + styled(b"36", b"integer") in lexical.stdout

    syntax = highlight(ASSETS / "invalid/parser/multiple-errors.bminor")
    assert syntax.returncode == 1 and syntax.stderr.count(b"expected expression") == 2
    assert syntax.stdout.count(styled(b"4;31", b";")) == 2
    assert b"first_ok: " + styled(b"36", b"integer") in syntax.stdout
    assert b"second_ok: " + styled(b"36", b"integer") in syntax.stdout
    print("PASS lexical and syntax error styling with continued highlighting")

    with TemporaryDirectory(prefix="ansi-highlighting-", dir=ROOT / "build") as temp:
        input_file = Path(temp) / "controls.bminor"
        comment = b"/* caf\xc3\xa9\t\x1b[32m\x07\x00\x7f */"
        input_file.write_bytes(comment + b"\r\n")
        result = highlight(input_file)
        assert result.returncode == 0, result.stderr
        assert SGR.sub(b"", result.stdout) == rb"/* caf" + b"\xc3\xa9\t" + rb"\x1B[32m\x07\x00\x7F */" + b"\r\n"
    print("PASS visible ASCII control bytes and preserved UTF-8, tabs, and CRLF")


if __name__ == "__main__":
    main()

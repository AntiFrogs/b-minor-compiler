from html.parser import HTMLParser
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
PROGRAM = ROOT / "build/bminor"
ASSETS = ROOT / "assets"


class HighlightedPage(HTMLParser):
    def __init__(self, html):
        super().__init__(convert_charrefs=True)
        self.tags = []
        self.in_code = False
        self.span_class = None
        self.span_text = []
        self.source_parts = []
        self.spans = []
        self.feed(html)
        self.close()
        assert not self.in_code and self.span_class is None
        assert self.tags.count("pre") == self.tags.count("code") == 1
        assert self.tags.count("style") == 1
        assert set(self.tags) <= {"html", "head", "meta", "title", "style", "body", "pre", "code", "span"}

    def handle_starttag(self, tag, attrs):
        self.tags.append(tag)
        if self.in_code:
            assert tag == "span" and self.span_class is None
            assert len(attrs) == 1 and attrs[0][0] == "class"
            self.span_class = attrs[0][1]
        elif tag == "code":
            self.in_code = True

    def handle_endtag(self, tag):
        if tag == "span":
            assert self.in_code and self.span_class is not None
            self.spans.append((self.span_class, "".join(self.span_text)))
            self.span_class = None
            self.span_text.clear()
        elif tag == "code":
            assert self.in_code and self.span_class is None
            self.in_code = False

    def handle_data(self, text):
        if self.in_code:
            self.source_parts.append(text)
            if self.span_class is not None:
                self.span_text.append(text)

    @property
    def source(self):
        return "".join(self.source_parts)


def render(source):
    result = subprocess.run(
        [PROGRAM, "--html", source],
        cwd=ROOT,
        capture_output=True,
        timeout=10,
    )
    assert result.returncode in (0, 1), (source, result.returncode)
    assert bool(result.stderr) == (result.returncode == 1), (source, result.stderr)
    markup = result.stdout.decode("utf-8")
    assert markup.startswith("<!doctype html>\n"), source
    assert markup.endswith("</code></pre>\n</body>\n</html>\n"), source
    return result, markup, HighlightedPage(markup)


def main():
    for source in sorted(ASSETS.rglob("*.bminor")):
        result, _, page = render(source)
        assert page.source.encode("utf-8") == source.read_bytes(), source
        if source.parent == ASSETS / "valid":
            assert result.returncode == 0, (source, result.stderr)
    print("PASS standalone documents and source round trips across all fixtures")

    _, markup, page = render(ASSETS / "valid/highlighting.bminor")
    assert "&lt;tag&gt;&amp;" in markup
    assert "&quot;" in markup and "&#39;" in markup
    assert ("str", '"<tag>&"') in page.spans
    assert ("comment", '/* Render <tag> & keep "quotes". */') in page.spans
    assert ("str", r"'\n'") in page.spans
    assert ("kw", "function") in page.spans
    assert ("type", "integer") in page.spans
    assert ("op", "*") in page.spans
    assert ("lit", "3") in page.spans
    assert ("lit", "true") in render(ASSETS / "valid/literals-and-comments.bminor")[2].spans
    print("PASS escaped markup and matching token styles")

    _, markup, page = render(ASSETS / "valid/highlight-roles.bminor")
    assert page.spans.count(("fn", "repeat")) == 4
    assert markup.count('<span class="fn">repeat</span>:') == 2
    assert '(<span class="fn">repeat</span>)(' in markup
    assert '<span class="fn">repeat</span> <span class="comment">/* before arguments */</span> (' in markup
    assert "\n        repeat: " in markup and '<span class="kw">print</span> repeat;' in markup
    print("PASS AST-based function styles and default variable style")

    result, _, page = render(ASSETS / "invalid/lexer/unknown-character.bminor")
    assert result.returncode == 1 and b'invalid byte "@"' in result.stderr
    assert ("error", "@") in page.spans and "after: integer = 9;" in page.source
    result, _, page = render(ASSETS / "invalid/parser/multiple-errors.bminor")
    assert result.returncode == 1 and result.stderr.count(b"expected expression") == 2
    assert page.spans.count(("error", ";")) == 2
    assert "first_ok: integer = 1;" in page.source and "second_ok: integer = 2;" in page.source
    print("PASS error styles, diagnostics, and later source text")

    with TemporaryDirectory(prefix="html-highlighting-", dir=ROOT / "build") as temp:
        source = Path(temp) / "example.bminor"
        leading = b"\n\r\n\t"
        comment = '/* </code></pre><script>alert("&")</script> café 🌱 */'.encode("utf-8")
        source.write_bytes(leading + comment + b"\r\n")
        result, markup, page = render(source)
        assert result.returncode == 0 and page.source.encode("utf-8") == source.read_bytes()
        assert "&lt;script&gt;" in markup and "&#13;\n" in markup
        assert "<code>\n&#13;\n\t" in markup

        source.write_bytes(b"/* \x00\x1b\x7f\xff \xed\xa0\x80 \xf4\x90\x80\x80 */")
        result, _, page = render(source)
        assert result.returncode == 0
        assert page.source == r"/* \x00\x1B\x7F\xFF \xED\xA0\x80 \xF4\x90\x80\x80 */"
    print("PASS leading blank lines, HTML-like comments, and visible invalid bytes")


if __name__ == "__main__":
    main()

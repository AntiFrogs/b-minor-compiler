#include "highlight.h"

#include <ostream>

namespace bminor {
namespace {

constexpr std::string_view document_start = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>B-Minor source</title>
<style>
  body { margin: 0; background: #111827; color: #f1f5f9; }
  pre {
    margin: 0;
    padding: 1.25rem;
    overflow-x: auto;
    white-space: pre;
    tab-size: 4;
    font: 0.9375rem/1.6 ui-monospace, monospace;
  }
  code { font: inherit; }
  .kw { color: #82aaff; font-weight: bold; }
  .type { color: #67e8f9; }
  .fn { color: #f5cf75; }
  .lit { color: #f9a76a; }
  .str { color: #ace39c; }
  .op { color: #cbd5e1; }
  .comment { color: #a6afbd; font-style: italic; }
  .error { color: #fda4af; text-decoration: underline; }
</style>
</head>
<body>
<pre><code>)HTML";

std::string_view css_class(HighlightStyle style) {
    switch (style) {
        case HighlightStyle::Keyword: return "kw";
        case HighlightStyle::Type: return "type";
        case HighlightStyle::Function: return "fn";
        case HighlightStyle::Integer:
        case HighlightStyle::Boolean: return "lit";
        case HighlightStyle::StringOrCharacter: return "str";
        case HighlightStyle::Operator: return "op";
        case HighlightStyle::Comment: return "comment";
        case HighlightStyle::Error: return "error";
        case HighlightStyle::Plain:
        case HighlightStyle::Identifier: return {};
    }
    return {};
}

std::string_view html_entity(unsigned char byte) {
    switch (byte) {
        case '&': return "&amp;";
        case '<': return "&lt;";
        case '>': return "&gt;";
        case '"': return "&quot;";
        case '\'': return "&#39;";
        case '\r': return "&#13;";
        default: return {};
    }
}

// Return the width of one complete UTF-8 character, or zero for an invalid sequence.
std::size_t utf8_width(std::string_view text) {
    const auto first = static_cast<unsigned char>(text.front());
    std::size_t width = 0;
    if (first >= 0xc2 && first <= 0xdf) {
        width = 2;
    } else if (first >= 0xe0 && first <= 0xef) {
        width = 3;
    } else if (first >= 0xf0 && first <= 0xf4) {
        width = 4;
    }
    if (width == 0 || text.size() < width) {
        return 0;
    }

    for (std::size_t index = 1; index < width; ++index) {
        const auto continuation = static_cast<unsigned char>(text[index]);
        if (continuation < 0x80 || continuation > 0xbf) {
            return 0;
        }
    }
    const auto second = static_cast<unsigned char>(text[1]);
    if ((first == 0xe0 && second < 0xa0) || (first == 0xed && second > 0x9f)
        || (first == 0xf0 && second < 0x90) || (first == 0xf4 && second > 0x8f)) {
        return 0;
    }
    return width;
}

void write_html_text(std::ostream& output, std::string_view text) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::size_t start = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto byte = static_cast<unsigned char>(text[index]);
        const auto entity = html_entity(byte);
        if (!entity.empty()) {
            output << text.substr(start, index - start) << entity;
            start = ++index;
            continue;
        }

        if (byte >= 0x80) {
            const auto width = utf8_width(text.substr(index));
            if (width != 0) {
                index += width;
                continue;
            }
        } else if (byte == '\t' || byte == '\n' || (byte >= 0x20 && byte < 0x7f)) {
            ++index;
            continue;
        }

        output << text.substr(start, index - start);
        output << "\\x" << hex[byte >> 4] << hex[byte & 0x0f];
        start = ++index;
    }
    output << text.substr(start);
}

}  // namespace

void render_html(std::ostream& output, std::string_view source,
                 const std::vector<HighlightSpan>& spans) {
    output << document_start;
    visit_highlighted_source(source, spans, [&](std::string_view text, HighlightStyle style) {
        const auto name = css_class(style);
        if (!name.empty()) {
            output << "<span class=\"" << name << "\">";
        }
        write_html_text(output, text);
        if (!name.empty()) {
            output << "</span>";
        }
    });
    output << "</code></pre>\n</body>\n</html>\n";
}

}  // namespace bminor

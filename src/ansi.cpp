#include "highlight.h"

#include <ostream>

namespace bminor {
namespace {

std::string_view ansi_style(HighlightStyle style) {
    switch (style) {
        case HighlightStyle::Keyword: return "\x1b[1;34m";
        case HighlightStyle::Type: return "\x1b[36m";
        case HighlightStyle::Function: return "\x1b[33m";
        case HighlightStyle::Integer:
        case HighlightStyle::Boolean: return "\x1b[38;5;208m";
        case HighlightStyle::StringOrCharacter: return "\x1b[32m";
        case HighlightStyle::Operator: return "\x1b[37m";
        case HighlightStyle::Comment: return "\x1b[2;3;90m";
        case HighlightStyle::Error: return "\x1b[4;31m";
        case HighlightStyle::Plain:
        case HighlightStyle::Identifier: return {};
    }
    return {};
}

void write_terminal_text(std::ostream& output, std::string_view text) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::size_t start = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto byte = static_cast<unsigned char>(text[index]);
        const bool control = byte < 0x20 && byte != '\t' && byte != '\r' && byte != '\n';
        if (control || byte == 0x7f) {
            output << text.substr(start, index - start);
            output << "\\x" << hex[byte >> 4] << hex[byte & 0x0f];
            start = index + 1;
        }
    }
    output << text.substr(start);
}

}  // namespace

void render_ansi(std::ostream& output, std::string_view source,
                 const std::vector<HighlightSpan>& spans) {
    constexpr std::string_view reset = "\x1b[0m";
    visit_highlighted_source(source, spans, [&](std::string_view text, HighlightStyle category) {
        const auto style = ansi_style(category);
        if (!style.empty()) {
            output << style;
        }
        write_terminal_text(output, text);
        if (!style.empty()) {
            output << reset;
        }
    });
}

}  // namespace bminor

#ifndef BMINOR_HIGHLIGHT_H
#define BMINOR_HIGHLIGHT_H

#include <cstddef>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace bminor {

struct ProgramParseResult;

enum class HighlightStyle {
    Plain,
    Keyword,
    Type,
    Identifier,
    Function,
    Integer,
    Boolean,
    StringOrCharacter,
    Operator,
    Comment,
    Error,
};

struct HighlightSpan {
    std::size_t offset;
    std::size_t length;
    HighlightStyle style;
};

// The parse result must come from the same source. Spans are ordered and disjoint.
std::vector<HighlightSpan> classify_source(std::string_view source, std::string_view filename,
                                           const ProgramParseResult& parsed);

template <typename Writer>
void visit_highlighted_source(std::string_view source, const std::vector<HighlightSpan>& spans,
                              Writer write) {
    std::size_t cursor = 0;
    for (const auto& span : spans) {
        if (cursor < span.offset) {
            write(source.substr(cursor, span.offset - cursor), HighlightStyle::Plain);
        }
        write(source.substr(span.offset, span.length), span.style);
        cursor = span.offset + span.length;
    }
    if (cursor < source.size()) {
        write(source.substr(cursor), HighlightStyle::Plain);
    }
}

void render_ansi(std::ostream& output, std::string_view source,
                 const std::vector<HighlightSpan>& spans);
void render_html(std::ostream& output, std::string_view source,
                 const std::vector<HighlightSpan>& spans);

}  // namespace bminor

#endif

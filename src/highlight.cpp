#include "highlight.h"

#include "parser.h"

#include <unordered_map>

namespace bminor {
namespace {

using StylesByOffset = std::unordered_map<std::size_t, HighlightStyle>;

StylesByOffset contextual_styles(const ProgramParseResult& parsed) {
    StylesByOffset styles;
    for (const auto& declaration : parsed.program.declarations) {
        if (const auto* function = std::get_if<FunctionDeclaration>(&declaration)) {
            styles[function->name.location.offset] = HighlightStyle::Function;
        }
    }

    const auto& expressions = parsed.program.expressions;
    for (const auto& expression : expressions) {
        if (expression.kind == ExpressionKind::Call) {
            const auto& callee = expressions[expression.children.front()];
            if (callee.kind == ExpressionKind::Identifier && callee.token) {
                styles[callee.token->location.offset] = HighlightStyle::Function;
            }
        }
    }

    for (const auto& diagnostic : parsed.diagnostics) {
        styles[diagnostic.token.location.offset] = HighlightStyle::Error;
    }
    return styles;
}

bool is_type_name(std::string_view word) {
    return word == "integer" || word == "boolean" || word == "char"
        || word == "string" || word == "void";
}

HighlightStyle lexical_style(const Token& token) {
    switch (token.kind) {
        case TokenKind::Identifier: return HighlightStyle::Identifier;
        case TokenKind::Keyword:
            return is_type_name(token.lexeme) ? HighlightStyle::Type : HighlightStyle::Keyword;
        case TokenKind::Integer: return HighlightStyle::Integer;
        case TokenKind::Boolean: return HighlightStyle::Boolean;
        case TokenKind::String:
        case TokenKind::Character: return HighlightStyle::StringOrCharacter;
        case TokenKind::Operator: return HighlightStyle::Operator;
        case TokenKind::LineComment:
        case TokenKind::BlockComment: return HighlightStyle::Comment;
        case TokenKind::Invalid: return HighlightStyle::Error;
        case TokenKind::Delimiter:
        case TokenKind::EndOfFile: return HighlightStyle::Plain;
    }
    return HighlightStyle::Plain;
}

}  // namespace

std::vector<HighlightSpan> classify_source(std::string_view source, std::string_view filename,
                                           const ProgramParseResult& parsed) {
    const auto styles = contextual_styles(parsed);
    std::vector<HighlightSpan> spans;
    Lexer lexer(source, filename);

    for (auto token = lexer.next(); token.kind != TokenKind::EndOfFile; token = lexer.next()) {
        const auto offset = token.location.offset;
        const auto contextual = styles.find(offset);
        const auto style = contextual != styles.end() ? contextual->second : lexical_style(token);
        if (style != HighlightStyle::Plain) {
            spans.push_back({offset, token.lexeme.size(), style});
        }
    }
    return spans;
}

}  // namespace bminor

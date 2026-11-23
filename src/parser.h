#ifndef BMINOR_PARSER_H
#define BMINOR_PARSER_H

#include "lexer.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bminor {

enum class ExpressionKind {
    Identifier,
    Literal,
    Prefix,
    Postfix,
    Binary,
    Assignment,
    Call,
    Subscript,
};

using ExpressionId = std::size_t;

struct SourceSpan {
    SourceLocation start;
    std::size_t end_offset;
};

struct Expression {
    ExpressionKind kind;
    SourceSpan span;
    std::optional<Token> token;
    std::vector<ExpressionId> children;
    std::optional<std::string> type_annotation;
};

struct ExpressionTree {
    std::vector<Expression> nodes;
    ExpressionId root;
};

struct ParseError {
    Token token;
    std::string_view message;
};

// The source and filename must outlive the returned tree and any ParseError.
ExpressionTree parse_expression_file(std::string_view source, std::string_view filename);
std::string_view expression_kind_name(ExpressionKind kind);

}  // namespace bminor

#endif

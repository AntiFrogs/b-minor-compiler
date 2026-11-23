#include "lexer.h"

#include <algorithm>
#include <iterator>

namespace bminor {
namespace {

constexpr std::string_view keywords[] = {
    "array", "boolean", "char", "else", "for", "function", "if",
    "integer", "print", "return", "string", "void", "while",
};

constexpr std::string_view two_byte_operators[] = {
    "++", "--", "<=", ">=", "==", "!=", "&&", "||",
};

constexpr std::string_view one_byte_operators = "!^*/%+-<>=";
constexpr std::string_view delimiters = "()[]{},:;";

bool is_digit(char byte) {
    return byte >= '0' && byte <= '9';
}

bool is_identifier_start(char byte) {
    return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || byte == '_';
}

bool is_identifier_continue(char byte) {
    return is_identifier_start(byte) || is_digit(byte);
}

bool is_whitespace(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}

TokenKind classify_word(std::string_view word) {
    if (word == "true" || word == "false") {
        return TokenKind::Boolean;
    }
    if (std::find(std::begin(keywords), std::end(keywords), word) != std::end(keywords)) {
        return TokenKind::Keyword;
    }
    return TokenKind::Identifier;
}

}  // namespace

std::string_view token_kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Identifier: return "IDENTIFIER";
        case TokenKind::Keyword: return "KEYWORD";
        case TokenKind::Integer: return "INTEGER";
        case TokenKind::Boolean: return "BOOLEAN";
        case TokenKind::Operator: return "OPERATOR";
        case TokenKind::Delimiter: return "DELIMITER";
        case TokenKind::Invalid: return "INVALID";
        case TokenKind::EndOfFile: return "EOF";
    }
    return "UNKNOWN";
}

Lexer::Lexer(std::string_view source, std::string_view filename)
    : source_(source), filename_(filename) {}

Token Lexer::next() {
    while (offset_ < source_.size() && is_whitespace(source_[offset_])) {
        advance();
    }

    const SourceLocation start{filename_, offset_, line_, column_};
    if (offset_ == source_.size()) {
        return make_token(TokenKind::EndOfFile, start);
    }

    const char byte = source_[offset_];
    if (is_identifier_start(byte)) {
        do {
            advance();
        } while (offset_ < source_.size() && is_identifier_continue(source_[offset_]));

        const auto word = source_.substr(start.offset, offset_ - start.offset);
        return {classify_word(word), word, start};
    }

    if (is_digit(byte)) {
        do {
            advance();
        } while (offset_ < source_.size() && is_digit(source_[offset_]));
        return make_token(TokenKind::Integer, start);
    }

    const auto pair = source_.substr(offset_, 2);
    const auto operators_end = std::end(two_byte_operators);
    if (std::find(std::begin(two_byte_operators), operators_end, pair) != operators_end) {
        advance();
        advance();
        return make_token(TokenKind::Operator, start);
    }

    advance();
    if (one_byte_operators.find(byte) != std::string_view::npos) {
        return make_token(TokenKind::Operator, start);
    }
    if (delimiters.find(byte) != std::string_view::npos) {
        return make_token(TokenKind::Delimiter, start);
    }
    return make_token(TokenKind::Invalid, start);
}

void Lexer::advance() {
    const char byte = source_[offset_++];
    if (byte == '\r' || byte == '\n') {
        if (byte == '\r' && offset_ < source_.size() && source_[offset_] == '\n') {
            ++offset_;
        }
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
}

Token Lexer::make_token(TokenKind kind, SourceLocation start) const {
    return {kind, source_.substr(start.offset, offset_ - start.offset), start};
}

}  // namespace bminor

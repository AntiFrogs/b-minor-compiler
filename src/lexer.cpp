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

bool is_line_break(char byte) {
    return byte == '\n' || byte == '\r';
}

bool is_whitespace(char byte) {
    return byte == ' ' || byte == '\t' || is_line_break(byte);
}

bool is_printable_ascii(char byte) {
    return byte >= 0x20 && byte <= 0x7e;
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
        case TokenKind::String: return "STRING";
        case TokenKind::Character: return "CHARACTER";
        case TokenKind::Operator: return "OPERATOR";
        case TokenKind::Delimiter: return "DELIMITER";
        case TokenKind::LineComment: return "LINE_COMMENT";
        case TokenKind::BlockComment: return "BLOCK_COMMENT";
        case TokenKind::Invalid: return "INVALID";
        case TokenKind::EndOfFile: return "EOF";
    }
    return "UNKNOWN";
}

std::string_view lexical_error_message(LexicalError error) {
    switch (error) {
        case LexicalError::None: return "";
        case LexicalError::InvalidByte: return "invalid byte";
        case LexicalError::UnterminatedString: return "unterminated string literal";
        case LexicalError::UnterminatedCharacter: return "unterminated character literal";
        case LexicalError::InvalidLiteralByte: return "literal contains a byte outside printable ASCII";
        case LexicalError::InvalidCharacterLength: return "character literal must contain exactly one character";
        case LexicalError::UnterminatedBlockComment: return "unterminated block comment";
    }
    return "invalid token";
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
    if (byte == '"' || byte == '\'') {
        return scan_literal(start, byte);
    }

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
    if (pair == "//") {
        return scan_line_comment(start);
    }
    if (pair == "/*") {
        return scan_block_comment(start);
    }

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
    return make_token(TokenKind::Invalid, start, LexicalError::InvalidByte);
}

Token Lexer::scan_literal(SourceLocation start, char quote) {
    const bool is_character = quote == '\'';
    bool contains_invalid_byte = false;
    std::size_t character_count = 0;
    advance();

    while (offset_ < source_.size() && !is_line_break(source_[offset_])) {
        if (source_[offset_] == quote) {
            advance();
            if (contains_invalid_byte) {
                return make_token(TokenKind::Invalid, start, LexicalError::InvalidLiteralByte);
            }
            if (is_character && character_count != 1) {
                return make_token(TokenKind::Invalid, start, LexicalError::InvalidCharacterLength);
            }
            return make_token(is_character ? TokenKind::Character : TokenKind::String, start);
        }

        if (source_[offset_] == '\\') {
            advance();
            if (offset_ == source_.size() || is_line_break(source_[offset_])) {
                break;
            }
        }
        if (!is_printable_ascii(source_[offset_])) {
            contains_invalid_byte = true;
        }
        advance();
        ++character_count;
    }

    const auto error = is_character ? LexicalError::UnterminatedCharacter
                                    : LexicalError::UnterminatedString;
    return make_token(TokenKind::Invalid, start, error);
}

Token Lexer::scan_line_comment(SourceLocation start) {
    while (offset_ < source_.size() && !is_line_break(source_[offset_])) {
        advance();
    }
    return make_token(TokenKind::LineComment, start);
}

Token Lexer::scan_block_comment(SourceLocation start) {
    advance();
    advance();
    while (offset_ < source_.size()) {
        if (source_.compare(offset_, 2, "*/") == 0) {
            advance();
            advance();
            return make_token(TokenKind::BlockComment, start);
        }
        advance();
    }
    return make_token(TokenKind::Invalid, start, LexicalError::UnterminatedBlockComment);
}

void Lexer::advance() {
    const char byte = source_[offset_++];
    if (is_line_break(byte)) {
        if (byte == '\r' && offset_ < source_.size() && source_[offset_] == '\n') {
            ++offset_;
        }
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
}

Token Lexer::make_token(TokenKind kind, SourceLocation start, LexicalError error) const {
    return {kind, source_.substr(start.offset, offset_ - start.offset), start, error};
}

}  // namespace bminor

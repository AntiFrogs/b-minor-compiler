#ifndef BMINOR_LEXER_H
#define BMINOR_LEXER_H

#include <cstddef>
#include <string_view>

namespace bminor {

enum class TokenKind {
    Identifier,
    Keyword,
    Integer,
    Boolean,
    Operator,
    Delimiter,
    Invalid,
    EndOfFile,
};

struct SourceLocation {
    std::string_view filename;
    std::size_t offset;
    std::size_t line;
    std::size_t column;
};

struct Token {
    TokenKind kind;
    std::string_view lexeme;
    SourceLocation location;
};

std::string_view token_kind_name(TokenKind kind);

class Lexer {
public:
    // Keep the source and filename alive while using the lexer and its tokens.
    Lexer(std::string_view source, std::string_view filename);

    Token next();

private:
    void advance();
    Token make_token(TokenKind kind, SourceLocation start) const;

    std::string_view source_;
    std::string_view filename_;
    std::size_t offset_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
};

}  // namespace bminor

#endif

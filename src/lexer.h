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
    String,
    Character,
    Operator,
    Delimiter,
    LineComment,
    BlockComment,
    Invalid,
    EndOfFile,
};

enum class LexicalError {
    None,
    InvalidByte,
    UnterminatedString,
    UnterminatedCharacter,
    InvalidLiteralByte,
    InvalidCharacterLength,
    UnterminatedBlockComment,
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
    LexicalError error = LexicalError::None;
};

std::string_view token_kind_name(TokenKind kind);
std::string_view lexical_error_message(LexicalError error);

class Lexer {
public:
    // Keep the source and filename alive while using the lexer and its tokens.
    Lexer(std::string_view source, std::string_view filename);

    Token next();

private:
    Token scan_literal(SourceLocation start, char quote);
    Token scan_line_comment(SourceLocation start);
    Token scan_block_comment(SourceLocation start);
    void advance();
    Token make_token(TokenKind kind, SourceLocation start,
                     LexicalError error = LexicalError::None) const;

    std::string_view source_;
    std::string_view filename_;
    std::size_t offset_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
};

}  // namespace bminor

#endif

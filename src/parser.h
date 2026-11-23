#ifndef BMINOR_PARSER_H
#define BMINOR_PARSER_H

#include "lexer.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
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

struct ArrayDimension {
    SourceSpan span;
    std::optional<ExpressionId> size;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct TypeSyntax {
    SourceSpan span;
    Token base;
    std::vector<ArrayDimension> dimensions;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct Initializer {
    SourceSpan span;
    std::variant<ExpressionId, std::vector<Initializer>> value;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct VariableDeclaration {
    SourceSpan span;
    Token name;
    TypeSyntax type;
    std::optional<Initializer> initializer;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct Parameter {
    SourceSpan span;
    Token name;
    TypeSyntax type;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct ReturnStatement {
    SourceSpan span;
    std::optional<ExpressionId> value;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct ExpressionStatement {
    SourceSpan span;
    ExpressionId expression;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct Statement;

struct Block {
    SourceSpan span;
    std::vector<Statement> statements;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct Statement {
    std::variant<Block, VariableDeclaration, ReturnStatement, ExpressionStatement> node;
};

struct FunctionDeclaration {
    SourceSpan span;
    Token name;
    TypeSyntax return_type;
    std::vector<Parameter> parameters;
    std::optional<Block> body;
    std::optional<std::string> type_annotation = std::nullopt;
};

using Declaration = std::variant<VariableDeclaration, FunctionDeclaration>;

struct Program {
    SourceSpan span;
    std::vector<Declaration> declarations;
    std::vector<Expression> expressions;
    std::optional<std::string> type_annotation = std::nullopt;
};

struct ParseError {
    Token token;
    std::string_view message;
};

// The source and filename must outlive the returned AST and any ParseError.
ExpressionTree parse_expression_file(std::string_view source, std::string_view filename);
Program parse_program_file(std::string_view source, std::string_view filename);
std::string_view expression_kind_name(ExpressionKind kind);

}  // namespace bminor

#endif

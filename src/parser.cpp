#include "parser.h"

#include <utility>

namespace bminor {
namespace {

constexpr std::size_t maximum_nesting = 256;

struct InfixOperator {
    std::string_view spelling;
    int precedence;
    bool right_associative = false;
};

constexpr InfixOperator infix_operators[] = {
    {"=", 1, true},
    {"||", 2},
    {"&&", 3},
    {"==", 4}, {"!=", 4},
    {"<", 5}, {"<=", 5}, {">", 5}, {">=", 5},
    {"+", 6}, {"-", 6},
    {"*", 7}, {"/", 7}, {"%", 7},
    {"^", 8, true},
};

const InfixOperator* find_infix_operator(const Token& token) {
    if (token.kind == TokenKind::Operator) {
        for (const auto& op : infix_operators) {
            if (op.spelling == token.lexeme) {
                return &op;
            }
        }
    }
    return nullptr;
}

std::size_t end_of(const Token& token) {
    return token.location.offset + token.lexeme.size();
}

class Parser {
public:
    Parser(std::string_view source, std::string_view filename)
        : lexer_(source, filename), current_(next_significant_token()) {}

    ExpressionTree parse();

private:
    ExpressionId parse_expression(int minimum_precedence, std::size_t nesting);
    ExpressionId parse_prefix(std::size_t nesting);
    ExpressionId parse_postfix(std::size_t nesting);
    ExpressionId parse_call(ExpressionId callee, std::size_t nesting);
    ExpressionId parse_primary(std::size_t nesting);

    Token next_significant_token();
    Token advance();
    bool matches(std::string_view spelling) const;
    Token expect(std::string_view spelling, std::string_view message);
    [[noreturn]] void fail(std::string_view message) const;
    ExpressionId add(ExpressionKind kind, SourceSpan span, std::optional<Token> token,
                     std::vector<ExpressionId> children = {});

    Lexer lexer_;
    Token current_;
    std::vector<Expression> nodes_;
};

ExpressionTree Parser::parse() {
    const auto root = parse_expression(1, 0);
    if (current_.kind != TokenKind::EndOfFile) {
        fail("expected end of input after expression");
    }
    return {std::move(nodes_), root};
}

ExpressionId Parser::parse_expression(int minimum_precedence, std::size_t nesting) {
    if (nesting > maximum_nesting) {
        fail("expression nesting limit exceeded");
    }

    auto left = parse_prefix(nesting);
    while (const auto* op = find_infix_operator(current_)) {
        if (op->precedence < minimum_precedence) {
            break;
        }

        const auto token = advance();
        const int right_precedence = op->precedence + (op->right_associative ? 0 : 1);
        const auto right = parse_expression(right_precedence, nesting + 1);
        const auto kind = token.lexeme == "=" ? ExpressionKind::Assignment : ExpressionKind::Binary;
        const SourceSpan span{nodes_[left].span.start, nodes_[right].span.end_offset};
        left = add(kind, span, token, {left, right});
    }
    return left;
}

ExpressionId Parser::parse_prefix(std::size_t nesting) {
    std::vector<Token> prefix;
    while (matches("+") || matches("-") || matches("!")) {
        prefix.push_back(advance());
    }

    auto operand = parse_postfix(nesting);
    for (auto op = prefix.rbegin(); op != prefix.rend(); ++op) {
        const SourceSpan span{op->location, nodes_[operand].span.end_offset};
        operand = add(ExpressionKind::Prefix, span, *op, {operand});
    }
    return operand;
}

ExpressionId Parser::parse_postfix(std::size_t nesting) {
    auto operand = parse_primary(nesting);
    while (matches("(") || matches("[")) {
        if (matches("(")) {
            operand = parse_call(operand, nesting);
        } else {
            advance();
            const auto index = parse_expression(1, nesting + 1);
            const auto close = expect("]", "expected ']' after subscript");
            const SourceSpan span{nodes_[operand].span.start, end_of(close)};
            operand = add(ExpressionKind::Subscript, span, std::nullopt, {operand, index});
        }
    }

    if (matches("++") || matches("--")) {
        const auto op = advance();
        const SourceSpan span{nodes_[operand].span.start, end_of(op)};
        operand = add(ExpressionKind::Postfix, span, op, {operand});
    }
    return operand;
}

ExpressionId Parser::parse_call(ExpressionId callee, std::size_t nesting) {
    advance();
    std::vector<ExpressionId> children{callee};
    if (!matches(")")) {
        children.push_back(parse_expression(1, nesting + 1));
        while (matches(",")) {
            advance();
            children.push_back(parse_expression(1, nesting + 1));
        }
    }
    const auto close = expect(")", "expected ')' after arguments");
    return add(ExpressionKind::Call, {nodes_[callee].span.start, end_of(close)},
               std::nullopt, std::move(children));
}

ExpressionId Parser::parse_primary(std::size_t nesting) {
    switch (current_.kind) {
        case TokenKind::Identifier: {
            const auto token = advance();
            return add(ExpressionKind::Identifier, {token.location, end_of(token)}, token);
        }
        case TokenKind::Integer:
        case TokenKind::Boolean:
        case TokenKind::String:
        case TokenKind::Character: {
            const auto token = advance();
            return add(ExpressionKind::Literal, {token.location, end_of(token)}, token);
        }
        default:
            break;
    }

    if (matches("(")) {
        const auto open = advance();
        const auto expression = parse_expression(1, nesting + 1);
        const auto close = expect(")", "expected ')' after grouped expression");
        nodes_[expression].span = {open.location, end_of(close)};
        return expression;
    }
    fail("expected expression");
}

Token Parser::next_significant_token() {
    auto token = lexer_.next();
    while (token.kind == TokenKind::LineComment || token.kind == TokenKind::BlockComment) {
        token = lexer_.next();
    }
    return token;
}

Token Parser::advance() {
    const auto previous = current_;
    current_ = next_significant_token();
    return previous;
}

bool Parser::matches(std::string_view spelling) const {
    return (current_.kind == TokenKind::Operator || current_.kind == TokenKind::Delimiter)
        && current_.lexeme == spelling;
}

Token Parser::expect(std::string_view spelling, std::string_view message) {
    if (!matches(spelling)) {
        fail(message);
    }
    return advance();
}

void Parser::fail(std::string_view message) const {
    throw ParseError{current_, message};
}

ExpressionId Parser::add(ExpressionKind kind, SourceSpan span, std::optional<Token> token,
                         std::vector<ExpressionId> children) {
    nodes_.push_back({kind, span, token, std::move(children), std::nullopt});
    return nodes_.size() - 1;
}

}  // namespace

ExpressionTree parse_expression_file(std::string_view source, std::string_view filename) {
    return Parser(source, filename).parse();
}

std::string_view expression_kind_name(ExpressionKind kind) {
    switch (kind) {
        case ExpressionKind::Identifier: return "Identifier";
        case ExpressionKind::Literal: return "Literal";
        case ExpressionKind::Prefix: return "PrefixExpr";
        case ExpressionKind::Postfix: return "PostfixExpr";
        case ExpressionKind::Binary: return "BinaryExpr";
        case ExpressionKind::Assignment: return "AssignExpr";
        case ExpressionKind::Call: return "CallExpr";
        case ExpressionKind::Subscript: return "SubscriptExpr";
    }
    return "UnknownExpr";
}

}  // namespace bminor

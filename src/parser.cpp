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

std::size_t end_of(const Statement& statement) {
    return std::visit([](const auto& node) { return node.span.end_offset; }, statement.node);
}

enum class RecoveryScope { Program, Block };

struct RecoveryPoint {
    std::size_t offset;
    std::size_t expression_count;
    std::size_t braces;
    std::size_t parentheses;
};

class Parser {
public:
    Parser(std::string_view source, std::string_view filename)
        : lexer_(source, filename), current_(next_significant_token()) {}

    ExpressionTree parse_expression_file();
    ProgramParseResult parse_program();

private:
    Declaration parse_declaration();
    VariableDeclaration parse_variable(Token name);
    FunctionDeclaration parse_function(Token name);
    Parameter parse_parameter();
    TypeSyntax parse_variable_type(bool array_parameter = false);
    Token parse_base_type(bool allow_void);
    Initializer parse_initializer(std::size_t nesting);
    Block parse_block(std::size_t nesting);
    Statement parse_statement(std::size_t nesting);
    IfStatement parse_if(std::size_t nesting);
    ForStatement parse_for(std::size_t nesting);
    PrintStatement parse_print();

    ExpressionId parse_expression(int minimum_precedence, std::size_t nesting);
    std::optional<ExpressionId> parse_optional_expression(std::string_view delimiter);
    void parse_expression_list(std::vector<ExpressionId>& expressions,
                               std::string_view delimiter, std::size_t nesting);
    ExpressionId parse_prefix(std::size_t nesting);
    ExpressionId parse_postfix(std::size_t nesting);
    ExpressionId parse_call(ExpressionId callee, std::size_t nesting);
    ExpressionId parse_primary(std::size_t nesting);

    Token next_significant_token();
    const Token& peek();
    Token advance();
    bool matches(std::string_view spelling) const;
    bool matches_keyword(std::string_view spelling) const;
    Token expect(std::string_view spelling, std::string_view message);
    Token expect_identifier(std::string_view message);
    [[noreturn]] void fail(std::string_view message) const;
    RecoveryPoint recovery_point() const;
    void recover(ParseError error, RecoveryPoint point, RecoveryScope scope);
    void synchronize(RecoveryPoint point, RecoveryScope scope);
    void discard_token();
    void report(ParseError error);
    bool starts_declaration();
    ExpressionId add(ExpressionKind kind, SourceSpan span, std::optional<Token> token,
                     std::vector<ExpressionId> children = {});

    Lexer lexer_;
    Token current_;
    std::optional<Token> lookahead_;
    std::vector<Expression> nodes_;
    std::vector<ParseError> diagnostics_;
    std::size_t braces_ = 0;
    std::size_t parentheses_ = 0;
    std::optional<std::size_t> for_header_depth_;
};

ExpressionTree Parser::parse_expression_file() {
    const auto root = parse_expression(1, 0);
    if (current_.kind != TokenKind::EndOfFile) {
        fail("expected end of input after expression");
    }
    return {std::move(nodes_), root};
}

ProgramParseResult Parser::parse_program() {
    const auto start = current_.location;
    auto end_offset = start.offset;
    std::vector<Declaration> declarations;
    while (current_.kind != TokenKind::EndOfFile) {
        const auto point = recovery_point();
        try {
            declarations.push_back(parse_declaration());
            end_offset = std::visit([](const auto& declaration) {
                return declaration.span.end_offset;
            }, declarations.back());
        } catch (const ParseError& error) {
            recover(error, point, RecoveryScope::Program);
        }
    }
    return {{{start, end_offset}, std::move(declarations), std::move(nodes_)},
            std::move(diagnostics_)};
}

Declaration Parser::parse_declaration() {
    const auto name = expect_identifier("expected declaration name");
    expect(":", "expected ':' after declaration name");
    if (matches_keyword("function")) {
        return parse_function(name);
    }
    return parse_variable(name);
}

VariableDeclaration Parser::parse_variable(Token name) {
    auto type = parse_variable_type();
    std::optional<Initializer> initializer;
    if (matches("=")) {
        advance();
        if (type.dimensions.empty() && matches("{")) {
            fail("expected expression for scalar initializer");
        }
        initializer = parse_initializer(0);
    }
    const auto end = expect(";", "expected ';' after variable declaration");
    return {{name.location, end_of(end)}, name, std::move(type), std::move(initializer)};
}

FunctionDeclaration Parser::parse_function(Token name) {
    advance();
    const auto base = parse_base_type(true);
    TypeSyntax return_type{{base.location, end_of(base)}, base, {}};

    expect("(", "expected '(' before function parameters");
    std::vector<Parameter> parameters;
    if (!matches(")")) {
        parameters.push_back(parse_parameter());
        while (matches(",")) {
            advance();
            parameters.push_back(parse_parameter());
        }
    }
    expect(")", "expected ')' after function parameters");

    if (matches(";")) {
        const auto end = advance();
        return {{name.location, end_of(end)}, name, std::move(return_type),
                std::move(parameters), std::nullopt};
    }
    expect("=", "expected ';' or '=' after function signature");
    auto body = parse_block(0);
    const SourceSpan span{name.location, body.span.end_offset};
    return {span, name, std::move(return_type), std::move(parameters), std::move(body)};
}

Parameter Parser::parse_parameter() {
    const auto name = expect_identifier("expected parameter name");
    expect(":", "expected ':' after parameter name");
    auto type = parse_variable_type(true);
    const SourceSpan span{name.location, type.span.end_offset};
    return {span, name, std::move(type)};
}

TypeSyntax Parser::parse_variable_type(bool array_parameter) {
    const auto start = current_.location;
    std::vector<ArrayDimension> dimensions;
    while (matches_keyword("array")) {
        const auto prefix = advance();
        expect("[", "expected '[' after 'array'");
        const bool unsized = array_parameter && dimensions.empty();
        std::optional<ExpressionId> size;
        if (!unsized) {
            size = parse_expression(1, 0);
        }
        const auto end = expect("]", unsized ? "expected ']' for unsized array parameter"
                                              : "expected ']' after array dimension");
        dimensions.push_back({{prefix.location, end_of(end)}, size});
    }
    const auto base = parse_base_type(false);
    return {{start, end_of(base)}, base, std::move(dimensions)};
}

Token Parser::parse_base_type(bool allow_void) {
    if (matches_keyword("integer") || matches_keyword("boolean") || matches_keyword("char")
        || matches_keyword("string") || (allow_void && matches_keyword("void"))) {
        return advance();
    }
    fail(allow_void ? "expected primitive return type or 'void'" : "expected primitive type");
}

Initializer Parser::parse_initializer(std::size_t nesting) {
    if (!matches("{")) {
        const auto expression = parse_expression(1, 0);
        return {nodes_[expression].span, expression};
    }
    if (nesting >= maximum_nesting) {
        fail("initializer nesting limit exceeded");
    }

    const auto open = advance();
    std::vector<Initializer> items;
    items.push_back(parse_initializer(nesting + 1));
    while (matches(",")) {
        advance();
        items.push_back(parse_initializer(nesting + 1));
    }
    const auto close = expect("}", "expected '}' after array initializer");
    return {{open.location, end_of(close)}, std::move(items)};
}

Block Parser::parse_block(std::size_t nesting) {
    const auto open = expect("{", "expected '{' to start block");
    std::vector<Statement> statements;
    while (!matches("}") && current_.kind != TokenKind::EndOfFile) {
        const auto point = recovery_point();
        try {
            statements.push_back(parse_statement(nesting + 1));
        } catch (const ParseError& error) {
            recover(error, point, RecoveryScope::Block);
        }
    }
    if (current_.kind == TokenKind::EndOfFile) {
        report({current_, "expected '}' after block"});
        return {{open.location, current_.location.offset}, std::move(statements)};
    }
    const auto close = advance();
    return {{open.location, end_of(close)}, std::move(statements)};
}

Statement Parser::parse_statement(std::size_t nesting) {
    if (nesting >= maximum_nesting) {
        fail("statement nesting limit exceeded");
    }
    if (matches("{")) {
        return {parse_block(nesting)};
    }
    if (matches_keyword("if")) {
        return {parse_if(nesting)};
    }
    if (matches_keyword("for")) {
        return {parse_for(nesting)};
    }
    if (matches_keyword("print")) {
        return {parse_print()};
    }
    if (matches_keyword("return")) {
        const auto start = advance().location;
        const auto value = parse_optional_expression(";");
        const auto end = expect(";", "expected ';' after return statement");
        return {ReturnStatement{{start, end_of(end)}, value}};
    }
    if (starts_declaration()) {
        const auto name = advance();
        advance();
        return {parse_variable(name)};
    }
    const auto expression = parse_expression(1, 0);
    const auto end = expect(";", "expected ';' after expression statement");
    return {ExpressionStatement{{nodes_[expression].span.start, end_of(end)}, expression}};
}

IfStatement Parser::parse_if(std::size_t nesting) {
    const auto start = advance().location;
    expect("(", "expected '(' after 'if'");
    const auto outer_parentheses = parentheses_ - 1;
    const auto condition = parse_expression(1, 0);
    if (matches("{")) {
        report({current_, "expected ')' after if condition"});
        parentheses_ = outer_parentheses;
    } else {
        expect(")", "expected ')' after if condition");
    }

    auto then_branch = std::make_unique<Statement>(parse_statement(nesting + 1));
    std::unique_ptr<Statement> else_branch;
    if (matches_keyword("else")) {
        advance();
        else_branch = std::make_unique<Statement>(parse_statement(nesting + 1));
    }
    const auto end = end_of(else_branch ? *else_branch : *then_branch);
    return {{start, end}, condition, std::move(then_branch), std::move(else_branch)};
}

ForStatement Parser::parse_for(std::size_t nesting) {
    const auto start = advance().location;
    expect("(", "expected '(' after 'for'");
    for_header_depth_ = parentheses_;
    const auto initializer = parse_optional_expression(";");
    expect(";", "expected ';' after for initializer");
    const auto condition = parse_optional_expression(";");
    expect(";", "expected ';' after for condition");
    const auto update = parse_optional_expression(")");
    expect(")", "expected ')' after for update");
    for_header_depth_.reset();

    auto body = std::make_unique<Statement>(parse_statement(nesting + 1));
    const SourceSpan span{start, end_of(*body)};
    return {span, initializer, condition, update, std::move(body)};
}

PrintStatement Parser::parse_print() {
    const auto start = advance().location;
    std::vector<ExpressionId> arguments;
    parse_expression_list(arguments, ";", 0);
    const auto end = expect(";", "expected ';' after print statement");
    return {{start, end_of(end)}, std::move(arguments)};
}

std::optional<ExpressionId> Parser::parse_optional_expression(std::string_view delimiter) {
    if (matches(delimiter)) {
        return std::nullopt;
    }
    return parse_expression(1, 0);
}

void Parser::parse_expression_list(std::vector<ExpressionId>& expressions,
                                   std::string_view delimiter, std::size_t nesting) {
    if (matches(delimiter)) {
        return;
    }
    expressions.push_back(parse_expression(1, nesting));
    while (matches(",")) {
        advance();
        expressions.push_back(parse_expression(1, nesting));
    }
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
    parse_expression_list(children, ")", nesting + 1);
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

const Token& Parser::peek() {
    if (!lookahead_) {
        lookahead_ = next_significant_token();
    }
    return *lookahead_;
}

Token Parser::advance() {
    const auto previous = current_;
    if (previous.kind == TokenKind::Delimiter) {
        if (previous.lexeme == "{") {
            ++braces_;
        } else if (previous.lexeme == "}" && braces_ != 0) {
            --braces_;
        } else if (previous.lexeme == "(") {
            ++parentheses_;
        } else if (previous.lexeme == ")" && parentheses_ != 0) {
            --parentheses_;
        }
    }
    current_ = peek();
    lookahead_.reset();
    return previous;
}

bool Parser::matches_keyword(std::string_view spelling) const {
    return current_.kind == TokenKind::Keyword && current_.lexeme == spelling;
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

Token Parser::expect_identifier(std::string_view message) {
    if (current_.kind != TokenKind::Identifier) {
        fail(message);
    }
    return advance();
}

void Parser::fail(std::string_view message) const {
    throw ParseError{current_, message};
}

RecoveryPoint Parser::recovery_point() const {
    return {current_.location.offset, nodes_.size(), braces_, parentheses_};
}

void Parser::recover(ParseError error, RecoveryPoint point, RecoveryScope scope) {
    report(error);
    nodes_.resize(point.expression_count);
    synchronize(point, scope);
    braces_ = point.braces;
    parentheses_ = point.parentheses;
    for_header_depth_.reset();
}

void Parser::synchronize(RecoveryPoint point, RecoveryScope scope) {
    const bool in_block = scope == RecoveryScope::Block;
    // Blocks handle errors in their own statement lists. Unclosed braces at
    // this checkpoint therefore belong to a discarded initializer.
    const bool started_in_initializer = braces_ > point.braces;
    bool skipped_braced_region = false;

    while (current_.kind != TokenKind::EndOfFile) {
        const bool at_scope = braces_ == point.braces;
        if (at_scope && matches("}")) {
            if (!in_block) {
                advance();
            }
            return;
        }
        if (at_scope || (started_in_initializer && !skipped_braced_region)) {
            const bool in_for_header = for_header_depth_ && parentheses_ >= *for_header_depth_;
            if (matches(";") && !in_for_header) {
                advance();
                return;
            }
            if (current_.location.offset > point.offset) {
                if (parentheses_ <= point.parentheses && starts_declaration()) {
                    return;
                }
                if (in_block && (matches_keyword("if") || matches_keyword("for")
                                 || matches_keyword("print") || matches_keyword("return"))) {
                    return;
                }
            }
            if (at_scope && matches("{")) {
                skipped_braced_region = true;
            }
        }

        const bool closes_skipped_region = skipped_braced_region && matches("}")
                                        && braces_ > point.braces && braces_ - point.braces == 1;
        discard_token();
        if (closes_skipped_region) {
            if (matches(";")) {
                advance();
            }
            return;
        }
    }
}

void Parser::discard_token() {
    if (current_.kind == TokenKind::Invalid) {
        report({current_, {}});
    }
    advance();
}

void Parser::report(ParseError error) {
    if (error.token.kind == TokenKind::Invalid) {
        error.message = lexical_error_message(error.token.error);
    }
    if (!diagnostics_.empty()) {
        const auto& previous = diagnostics_.back();
        if (previous.token.location.offset == error.token.location.offset
            && previous.message == error.message) {
            return;
        }
    }
    diagnostics_.push_back(error);
}

bool Parser::starts_declaration() {
    if (current_.kind != TokenKind::Identifier) {
        return false;
    }
    const auto& next = peek();
    return next.kind == TokenKind::Delimiter && next.lexeme == ":";
}

ExpressionId Parser::add(ExpressionKind kind, SourceSpan span, std::optional<Token> token,
                         std::vector<ExpressionId> children) {
    nodes_.push_back({kind, span, token, std::move(children), std::nullopt});
    return nodes_.size() - 1;
}

}  // namespace

ExpressionTree parse_expression_file(std::string_view source, std::string_view filename) {
    return Parser(source, filename).parse_expression_file();
}

Program parse_program_file(std::string_view source, std::string_view filename) {
    auto result = parse_program_with_recovery(source, filename);
    if (!result.diagnostics.empty()) {
        throw result.diagnostics.front();
    }
    return std::move(result.program);
}

ProgramParseResult parse_program_with_recovery(std::string_view source, std::string_view filename) {
    return Parser(source, filename).parse_program();
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

#include "dump.h"

#include "parser.h"

#include <ostream>
#include <string>

namespace bminor {
namespace {

void write_start(std::ostream& output, std::size_t depth, std::string_view label) {
    output << std::string(depth * 2, ' ') << label;
}

void write_position(std::ostream& output, const SourceSpan& span) {
    output << " @" << span.start.line << ':' << span.start.column << '\n';
}

void write_node(std::ostream& output, std::size_t depth, std::string_view label,
                const SourceSpan& span, std::string_view spelling = {}) {
    write_start(output, depth, label);
    if (!spelling.empty()) {
        output.put(' ');
        write_lexeme(output, spelling);
    }
    write_position(output, span);
}

void write_expression(std::ostream& output, const std::vector<Expression>& nodes,
                      ExpressionId root, std::size_t root_depth) {
    struct PendingNode {
        ExpressionId id;
        std::size_t depth;
    };
    std::vector<PendingNode> pending{{root, root_depth}};
    while (!pending.empty() && output) {
        const auto [id, depth] = pending.back();
        pending.pop_back();

        const auto& node = nodes[id];
        write_start(output, depth, expression_kind_name(node.kind));
        if (node.token) {
            if (node.kind == ExpressionKind::Literal) {
                output << ' ' << token_kind_name(node.token->kind);
            }
            output.put(' ');
            write_lexeme(output, node.token->lexeme);
        }
        write_position(output, node.span);

        for (auto child = node.children.rbegin(); child != node.children.rend(); ++child) {
            pending.push_back({*child, depth + 1});
        }
    }
}

class ProgramWriter {
public:
    ProgramWriter(std::ostream& output, const std::vector<Expression>& expressions)
        : output_(output), expressions_(expressions) {}

    void write(const Program& program) {
        write_node(output_, 0, "Program", program.span);
        for (const auto& declaration : program.declarations) {
            std::visit([this](const auto& node) { write(node, 1); }, declaration);
        }
    }

private:
    void write(const VariableDeclaration& declaration, std::size_t depth) {
        write_node(output_, depth, "VarDecl", declaration.span, declaration.name.lexeme);
        write_type(declaration.type, depth + 1, "Type");
        if (declaration.initializer) {
            write(*declaration.initializer, depth + 1);
        }
    }

    void write(const FunctionDeclaration& declaration, std::size_t depth) {
        const auto label = declaration.body ? "FunctionDefinition" : "FunctionPrototype";
        write_node(output_, depth, label, declaration.span, declaration.name.lexeme);
        write_type(declaration.return_type, depth + 1, "ReturnType");
        for (const auto& parameter : declaration.parameters) {
            write_node(output_, depth + 1, "Parameter", parameter.span, parameter.name.lexeme);
            write_type(parameter.type, depth + 2, "Type");
        }
        if (declaration.body) {
            write(*declaration.body, depth + 1);
        }
    }

    void write_type(const TypeSyntax& type, std::size_t depth, std::string_view label) {
        write_node(output_, depth, label, type.span, type.base.lexeme);
        for (const auto& dimension : type.dimensions) {
            write_node(output_, depth + 1, "ArrayDimension", dimension.span);
            if (dimension.size) {
                write_expression(output_, expressions_, *dimension.size, depth + 2);
            }
        }
    }

    void write(const Initializer& initializer, std::size_t depth) {
        if (const auto* expression = std::get_if<ExpressionId>(&initializer.value)) {
            write_expression(output_, expressions_, *expression, depth);
        } else {
            write_node(output_, depth, "InitializerList", initializer.span);
            for (const auto& item : std::get<std::vector<Initializer>>(initializer.value)) {
                write(item, depth + 1);
            }
        }
    }

    void write(const Block& block, std::size_t depth) {
        write_node(output_, depth, "Block", block.span);
        for (const auto& statement : block.statements) {
            write(statement, depth + 1);
        }
    }

    void write(const Statement& statement, std::size_t depth) {
        std::visit([this, depth](const auto& node) { write(node, depth); }, statement.node);
    }

    void write(const ReturnStatement& statement, std::size_t depth) {
        write_node(output_, depth, "ReturnStmt", statement.span);
        if (statement.value) {
            write_expression(output_, expressions_, *statement.value, depth + 1);
        }
    }

    void write(const ExpressionStatement& statement, std::size_t depth) {
        write_node(output_, depth, "ExprStmt", statement.span);
        write_expression(output_, expressions_, statement.expression, depth + 1);
    }

    void write(const IfStatement& statement, std::size_t depth) {
        write_node(output_, depth, "IfStmt", statement.span);
        write_expression(output_, expressions_, statement.condition, depth + 1);
        write(*statement.then_branch, depth + 1);
        if (statement.else_branch) {
            write(*statement.else_branch, depth + 1);
        }
    }

    void write(const ForStatement& statement, std::size_t depth) {
        write_node(output_, depth, "ForStmt", statement.span);
        write_clause(statement.initializer, depth + 1, "ForInitializer");
        write_clause(statement.condition, depth + 1, "ForCondition");
        write_clause(statement.update, depth + 1, "ForUpdate");
        write(*statement.body, depth + 1);
    }

    void write_clause(std::optional<ExpressionId> expression, std::size_t depth, std::string_view label) {
        if (expression) {
            write_node(output_, depth, label, expressions_[*expression].span);
            write_expression(output_, expressions_, *expression, depth + 1);
        }
    }

    void write(const PrintStatement& statement, std::size_t depth) {
        write_node(output_, depth, "PrintStmt", statement.span);
        for (const auto argument : statement.arguments) {
            write_expression(output_, expressions_, argument, depth + 1);
        }
    }

    std::ostream& output_;
    const std::vector<Expression>& expressions_;
};

}  // namespace

void write_lexeme(std::ostream& output, std::string_view lexeme) {
    constexpr char hex[] = "0123456789ABCDEF";
    output.put('"');
    for (char character : lexeme) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20 || byte > 0x7e) {
            output << "\\x" << hex[byte >> 4] << hex[byte & 0x0f];
        } else {
            if (character == '"' || character == '\\') {
                output.put('\\');
            }
            output.put(character);
        }
    }
    output.put('"');
}

void write_expression_tree(std::ostream& output, const ExpressionTree& tree) {
    write_expression(output, tree.nodes, tree.root, 0);
}

void write_program_tree(std::ostream& output, const Program& program) {
    ProgramWriter(output, program.expressions).write(program);
}

}  // namespace bminor

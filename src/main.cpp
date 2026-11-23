#include "lexer.h"
#include "parser.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class Mode { Copy, Tokens, ExpressionAst };

bool transfer_source(std::istream& source, std::ostream& destination,
                     const char* path, std::string_view destination_name) {
    std::array<char, 4096> buffer;
    while (source && destination) {
        source.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        destination.write(buffer.data(), source.gcount());
    }

    if (!destination.flush()) {
        std::cerr << "Error: could not write to " << destination_name << ".\n";
        return false;
    }

    if (source.bad() || !source.eof()) {
        std::cerr << "Error: could not read source file '" << path << "'.\n";
        return false;
    }
    return true;
}

void write_location(std::ostream& output, const bminor::SourceLocation& location) {
    output << location.filename << ':' << location.line << ':' << location.column;
}

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

void write_lexical_diagnostic(const bminor::Token& token) {
    write_location(std::cerr, token.location);
    std::cerr << ": error: " << bminor::lexical_error_message(token.error);
    if (token.error == bminor::LexicalError::InvalidByte) {
        std::cerr.put(' ');
        write_lexeme(std::cerr, token.lexeme);
    }
    std::cerr.put('\n');
}

bool flush_output() {
    if (!std::cout.flush()) {
        std::cerr << "Error: could not write to standard output.\n";
        return false;
    }
    return true;
}

bool dump_tokens(std::string_view text, std::string_view filename) {
    bminor::Lexer lexer(text, filename);
    bool valid = true;

    while (std::cout) {
        const auto token = lexer.next();
        write_location(std::cout, token.location);
        std::cout << ' ' << bminor::token_kind_name(token.kind) << ' ';
        write_lexeme(std::cout, token.lexeme);
        std::cout.put('\n');

        if (token.kind == bminor::TokenKind::Invalid) {
            write_lexical_diagnostic(token);
            valid = false;
        }
        if (token.kind == bminor::TokenKind::EndOfFile) {
            break;
        }
    }

    return flush_output() && valid;
}

void write_expression_tree(const bminor::ExpressionTree& tree) {
    struct PendingNode {
        bminor::ExpressionId id;
        std::size_t depth;
    };
    std::vector<PendingNode> pending{{tree.root, 0}};
    while (!pending.empty() && std::cout) {
        const auto [id, depth] = pending.back();
        pending.pop_back();

        const auto& node = tree.nodes[id];
        std::cout << std::string(depth * 2, ' ') << bminor::expression_kind_name(node.kind);
        if (node.token) {
            if (node.kind == bminor::ExpressionKind::Literal) {
                std::cout << ' ' << bminor::token_kind_name(node.token->kind);
            }
            std::cout.put(' ');
            write_lexeme(std::cout, node.token->lexeme);
        }
        std::cout << " @" << node.span.start.line << ':' << node.span.start.column << '\n';

        for (auto child = node.children.rbegin(); child != node.children.rend(); ++child) {
            pending.push_back({*child, depth + 1});
        }
    }
}

bool dump_expression(std::string_view text, std::string_view filename) {
    try {
        write_expression_tree(bminor::parse_expression_file(text, filename));
        return flush_output();
    } catch (const bminor::ParseError& error) {
        if (error.token.kind == bminor::TokenKind::Invalid) {
            write_lexical_diagnostic(error.token);
        } else {
            write_location(std::cerr, error.token.location);
            std::cerr << ": error: " << error.message << ", found ";
            if (error.token.kind == bminor::TokenKind::EndOfFile) {
                std::cerr << "end of input";
            } else {
                write_lexeme(std::cerr, error.token.lexeme);
            }
            std::cerr.put('\n');
        }
        return false;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string_view argument = argc > 1 ? argv[1] : "";
    Mode mode = Mode::Copy;
    if (argument == "--tokens") {
        mode = Mode::Tokens;
    } else if (argument == "--expr-ast") {
        mode = Mode::ExpressionAst;
    }
    const int path_index = mode == Mode::Copy ? 1 : 2;
    if (argc != path_index + 1) {
        std::cerr << "Usage: bminor [--tokens | --expr-ast] <source-file>\n";
        return EXIT_FAILURE;
    }

    const char* path = argv[path_index];
    std::ifstream source(path, std::ios::binary);
    if (!source) {
        std::cerr << "Error: could not open source file '" << path << "'.\n";
        return EXIT_FAILURE;
    }

    if (mode == Mode::Copy) {
        return transfer_source(source, std::cout, path, "standard output") ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    std::ostringstream buffer;
    if (!transfer_source(source, buffer, path, "source buffer")) {
        return EXIT_FAILURE;
    }

    const std::string text = buffer.str();
    const bool success = mode == Mode::Tokens ? dump_tokens(text, path) : dump_expression(text, path);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

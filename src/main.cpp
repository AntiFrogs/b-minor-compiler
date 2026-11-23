#include "lexer.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

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
            write_location(std::cerr, token.location);
            std::cerr << ": error: invalid byte ";
            write_lexeme(std::cerr, token.lexeme);
            std::cerr.put('\n');
            valid = false;
        }
        if (token.kind == bminor::TokenKind::EndOfFile) {
            break;
        }
    }

    if (!std::cout.flush()) {
        std::cerr << "Error: could not write to standard output.\n";
        return false;
    }
    return valid;
}

}  // namespace

int main(int argc, char* argv[]) {
    const bool token_mode = argc > 1 && std::string_view(argv[1]) == "--tokens";
    const int path_index = token_mode ? 2 : 1;
    if (argc != path_index + 1) {
        std::cerr << "Usage: bminor [--tokens] <source-file>\n";
        return EXIT_FAILURE;
    }

    const char* path = argv[path_index];
    std::ifstream source(path, std::ios::binary);
    if (!source) {
        std::cerr << "Error: could not open source file '" << path << "'.\n";
        return EXIT_FAILURE;
    }

    if (!token_mode) {
        return transfer_source(source, std::cout, path, "standard output") ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    std::ostringstream buffer;
    if (!transfer_source(source, buffer, path, "source buffer")) {
        return EXIT_FAILURE;
    }

    const std::string text = buffer.str();
    return dump_tokens(text, path) ? EXIT_SUCCESS : EXIT_FAILURE;
}

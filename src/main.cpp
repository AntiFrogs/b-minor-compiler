#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: bminor <source-file>\n";
        return EXIT_FAILURE;
    }

    const char* path = argv[1];
    std::ifstream source(path, std::ios::binary);
    if (!source) {
        std::cerr << "Error: could not open source file '" << path << "'.\n";
        return EXIT_FAILURE;
    }

    std::array<char, 4096> buffer;
    while (source && std::cout) {
        source.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::cout.write(buffer.data(), source.gcount());
    }

    if (!std::cout.flush()) {
        std::cerr << "Error: could not write to standard output.\n";
        return EXIT_FAILURE;
    }

    if (source.bad() || !source.eof()) {
        std::cerr << "Error: could not read source file '" << path << "'.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

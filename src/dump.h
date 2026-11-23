#ifndef BMINOR_DUMP_H
#define BMINOR_DUMP_H

#include <iosfwd>
#include <string_view>

namespace bminor {

struct ExpressionTree;
struct Program;

void write_lexeme(std::ostream& output, std::string_view lexeme);
void write_expression_tree(std::ostream& output, const ExpressionTree& tree);
void write_program_tree(std::ostream& output, const Program& program);

}  // namespace bminor

#endif

#include <iostream>
#include "scanner.h"
#include "parser.h"
#include "printer.h"
#include "codegen.h"

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cout << "wrong number of args" << std::endl;
        exit(1);
    }

    Scanner scanner;
    std::vector<Token> tokens = scanner.scan_source(std::string(argv[1]));
    if (scanner.failed()) exit(1);

    Parser parser(tokens);
    std::vector<Stmt*> statements = parser.parse();
    if (parser.failed()) exit(1);

    // Printer printer;
    // printer.print(statements);

    // Codegen codegen;
    // std::cout << codegen.gen(statements);

    return 0;
}
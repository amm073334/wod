#include <iostream>
#include "scanner.h"
#include "parser.h"
#include "printer.h"
#include "typechecker.h"
#include "codegen.h"
#include "targetopt.h"

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cout << "wrong number of args" << std::endl;
        exit(1);
    }

    Scanner scanner;
    std::vector<Token> tokens = scanner.scan_source(std::string(argv[1]));
    if (scanner.failed()) exit(1);

    Parser parser(tokens);
    std::vector<Stmt*> statements = parser.parse();
    if (parser.failed()) exit(1);

    Typechecker typechecker;
    Environment* env = typechecker.typecheck(statements);
    if (typechecker.failed()) exit(1);

    Codegen codegen;
    GameData gd = codegen.gen(statements);

    for (CommonEvent& cev : gd.cevs) targopt_label(cev);
    gd.write(argv[2]);

    delete env;
    return 0;
}
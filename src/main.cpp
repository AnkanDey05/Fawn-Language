#include "cli/CLI.hpp"
#include "CodeRunner/CodeRunner.hpp"
#include <iostream>


int main(int argc, char* argv[]){
    auto opts = parseArgs(argc, argv);
    CodeRunner runner;

    switch (opts.mode) {
        case Mode::RUNFILE: runner.runFile(opts.filepath);
                    break;
        case Mode::CHECK: runner.checkFile(opts.filepath);
                    break;
        case Mode::REPL: runner.runREPL();
                    break;
        case Mode::VERSION: runner.printVersion();
                    break;
        case Mode::AST: runner.dumpAST(opts.filepath);
                    break;
        case Mode::TOKEN: runner.dumpTokens(opts.filepath);
                    break;
        default: std::cerr<<"Something That you typed is not implimented yet\n";
                return 0;
    }
    return 0;
}
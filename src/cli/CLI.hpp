#pragma once 


#include <string>
enum class Mode {RUNFILE, CHECK, TOKEN, AST, HELP, VERSION, REPL};

struct CLIOptions{
    Mode mode = Mode::REPL;
    std::string filepath = "";
};

CLIOptions parseArgs(int argc, char* argv[]);
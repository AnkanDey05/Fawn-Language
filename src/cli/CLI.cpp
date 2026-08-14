#include "CLI.hpp"
#include <stdexcept>
#include <string>

CLIOptions parseArgs(int argc, char **argv)
{
    CLIOptions opts;
    if (argc < 2) 
    {opts.mode = Mode::REPL;
    return opts;
    }

    std::string arg1 = argv[1];

    if (arg1 == "--help") {
        opts.mode = Mode::HELP;
        return opts;
    }
    if (arg1 == "--version" || arg1 == "--v") {
        opts.mode = Mode::VERSION;
        return opts;
    }
    if (arg1 == "--check" || arg1 == "--token" || arg1 == "--ast") {
        if (argc < 3) {
            throw std::runtime_error("Missing File to complete this command \n");
        }
        
        if (arg1 == "--check") opts.mode = Mode::CHECK;
        else if (arg1 == "--token") opts.mode = Mode::TOKEN;
        else opts.mode = Mode::AST;
        
        opts.filepath = argv[2];
        return opts;
    }
    if (arg1.starts_with("--")) {
        throw std::runtime_error("Unknow command use --help if stuck \n");
    }
    opts.mode = Mode::RUNFILE;
    opts.filepath = arg1;
    return opts;
}
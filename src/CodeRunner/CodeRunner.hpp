#pragma once 

#include <string>
#include "../io/FileReader.hpp"


class CodeRunner{

    FileReader filemanager;

    public:
        void runFile(const std::string& path);
        void checkFile(const std::string& path);
        void runREPL();
        void printVersion();
        void dumpAST(const std::string& path);
        void dumpTokens(const std::string& path);
};
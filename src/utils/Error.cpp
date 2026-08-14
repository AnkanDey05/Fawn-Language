#include "Error.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

std::string red(const std::string& s)  { return "\033[31m" + s + "\033[0m"; }
std::string blue(const std::string& s) { return "\033[36m" + s + "\033[0m"; }
std::string bold(const std::string& s) { return "\033[1m" + s + "\033[0m"; }


static std::string kindToString(const ErrorKind kind){
        switch (kind) {
            case ErrorKind::Syntax: return "Syntax";
            case ErrorKind::Reference: return "Reference";
            case ErrorKind::Runtime: return "Runtime";
            case ErrorKind::Type: return "Type";
            default: return "Unknown";
        }
    }


ErrorReporter::ErrorReporter(const std::string& filename, const std::string& source): filename(filename) 
{
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        sourcelines.push_back(line);
    }
}

void ErrorReporter::flush() {
    for (auto& err : errors) {

        std::cerr << red("\nError") << "[" << kindToString(err.kind) << "]: " 
                  << err.message << "\n";
        

        std::cerr << filename << ":" << err.line << "\n";
        
        if (err.line > 0 && err.line <= sourcelines.size()) {
            const std::string lineNumber = std::to_string(err.line);
            const size_t gutterWidth = std::max<size_t>(4, std::to_string(sourcelines.size()).size());
            const std::string padding(gutterWidth - lineNumber.size(), ' ');

            std::cerr << red(std::string(gutterWidth, ' ') + " | ") << "\n";
            std::cerr << red(padding + lineNumber + " | ")
                      << sourcelines[err.line - 1] << "\n";
            if (err.column > 0) {
                std::cerr << red(std::string(gutterWidth, ' ') + " | ")
                          << std::string(err.column - 1, ' ')
                          << red("^^^^ ") << err.message << "\n";
            } else {
                std::cerr << red(std::string(gutterWidth, ' ') + " | ") << "\n";
            }
        } else {
            std::cerr << blue("   = ")
                      << "source line unavailable" << "\n";
        }
        
        // Hint
        if (!err.hint.empty()) {
            std::cerr <<blue("Hint: ") << err.hint << "\n";
        }
        std::cerr << "\n";
    }
    errors.clear();
}

void ErrorReporter::report(const Error& error)
{
    errors.push_back(error);
}
bool ErrorReporter::haserror() const
{
    return !errors.empty();
}

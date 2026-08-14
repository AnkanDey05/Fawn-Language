#pragma once 
#pragma once 
#include "../common/Value.hpp"
#include <cstddef>
#include <string>
#include <vector>


class ReturnException {
public:
    Value value;
    ReturnException(Value val) : value(std::move(val)) {}
};

class FatalError {};

enum class ErrorKind{Syntax, Type, Runtime, Reference};

struct Error{
    ErrorKind kind;
    std::string message ;
    size_t line{};
    std::string hint = "";
    size_t column{};
};

class ErrorReporter{
    std::string filename;
    std::vector<std::string> sourcelines;
    std::vector<Error> errors;

    public:
        ErrorReporter(const std::string& filename, const std::string& source);
        void report(const Error& error);
        bool haserror() const;
        void flush();

};

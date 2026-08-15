#pragma once
#include "../common/Token.hpp"
#include "../common/Value.hpp"
#include "../utils/Error.hpp"
#include <memory>
#include <unordered_map>
#include <string>

struct VarEntry {
    Value value;
    std::string declaredType;
    bool isDynamic;
    bool isConst = false;
    size_t line = 0;
};

class Env {
    std::unordered_map<std::string, VarEntry> values;
    std::shared_ptr<Env> enclosing;
    ErrorReporter* reporter;

    public:
    explicit Env(ErrorReporter* reporter = nullptr) : enclosing(nullptr), reporter(reporter) {}
    Env(std::shared_ptr<Env> enclosing) : enclosing(enclosing), reporter(enclosing ? enclosing->reporter : nullptr) {}

    void define(const Token& nameToken, Value val, const std::string& declaredType, bool isDynamic, bool isConst = false);
    void define(const std::string& name, Value val, const std::string& declaredType, bool isDynamic, bool isConst = false);
    void assign(const Token& nameToken, Value val);
    Value get(const Token& nameToken);
    bool isConst(const Token& nameToken);
    bool isDefinedInCurrentScope(const std::string& name) const;
};

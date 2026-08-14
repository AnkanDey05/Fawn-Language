#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "../runtime/Environment.hpp"
#include "../ast/Stmt.hpp"
#include "../ast/Expr.hpp"
#include "../common/Value.hpp"

using NativeFunction = std::function<Value(std::vector<Value>, size_t)>;

class Interpreter
{
    public:
        std::shared_ptr<Env>globals;
        std::shared_ptr<Env> environment;

        explicit Interpreter(ErrorReporter* reporter);

        void interpret(const std::vector<std::unique_ptr<Stmt>>& statements);

    private:
        ErrorReporter* reporter;
        size_t functionDepth = 0;
        size_t loopDepth = 0;
        size_t functionLoopBase = 0;
        std::unordered_map<std::string, FuncStmt*> functions;
        std::unordered_map<std::string, NativeFunction> nativeFunctions;
        void execute(Stmt* statement);
        Value evaluate(Expr* expr);
        void registerNatives();
        Value executeListMethod(ListType list, const std::string& method, 
                                std::vector<std::unique_ptr<Expr>>& args, size_t line);
        Value executeStringMethod(const std::string& str, const std::string& method,
                                  std::vector<std::unique_ptr<Expr>>& args, size_t line);
};

struct BreakException {};
struct ContinueException {};

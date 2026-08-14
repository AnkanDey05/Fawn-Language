#pragma once 
#include "Node.hpp"
#include "Expr.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class Stmt : public Node 
{
    public:
        Stmt(size_t line) : Node(line) {}
        virtual ~Stmt() = default;
};

class OutStmt : public Stmt {
    public:
    std::vector<std::unique_ptr<Expr>> values;
    OutStmt(size_t line, std::vector<std::unique_ptr<Expr>> values)
        : Stmt(line), values(std::move(values)) {}
};

class InStmt : public Stmt {
    public:
    Token target;
    InStmt(size_t line, Token target)
        : Stmt(line), target(std::move(target)) {}
};


class VarDecStmt : public Stmt
{   
    public:
    Token type;
    Token name;
    std::unique_ptr<Expr> initializer; 
    bool isConst = false;
    VarDecStmt(size_t line, Token type, Token name, std::unique_ptr<Expr> init, bool isConst) :
        Stmt(line), type(type), name(name), initializer(std::move(init)), isConst(isConst) {}
};

class ListDecStmt : public Stmt {
    public:
    Token name;
    std::unique_ptr<Expr> size;
    Token elementType;
    std::vector<std::unique_ptr<Expr>> elements;
    std::unique_ptr<Expr> initExpr;
    bool isConst;

    ListDecStmt(size_t line, Token name, std::unique_ptr<Expr> size, Token elemType, 
                std::vector<std::unique_ptr<Expr>> elems, bool isConst = false, 
                std::unique_ptr<Expr> initExpr = nullptr)
        : Stmt(line), name(std::move(name)), size(std::move(size)), elementType(std::move(elemType)), 
          elements(std::move(elems)), initExpr(std::move(initExpr)), isConst(isConst) {}
};



class IfStmt : public Stmt 
{
    public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch; 
    IfStmt(size_t line, std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenB, std::unique_ptr<Stmt> elseB)
        : Stmt(line), condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
};

class WhileStmt : public Stmt
{
    public:
    std::unique_ptr<Expr> condition; 
    std::unique_ptr<Stmt> body;
    WhileStmt(size_t line, std::unique_ptr<Expr>cond, std::unique_ptr<Stmt> body) : Stmt(line), condition(std::move(cond)), body(std::move(body))
        {}
};

class ForStmt : public Stmt 
{
    public:
    Token varName;
    std::unique_ptr<Expr> from;
    std::unique_ptr<Expr> to;
    std::unique_ptr<Expr> step; 
    std::unique_ptr<Stmt> body;
    ForStmt(size_t line, Token var, std::unique_ptr<Expr> from, std::unique_ptr<Expr> to,
            std::unique_ptr<Expr> step, std::unique_ptr<Stmt> body)
        : Stmt(line), varName(var), from(std::move(from)), to(std::move(to)),
          step(std::move(step)), body(std::move(body)) {}
};

struct Param 
{
    Token type;
    Token name;
    std::shared_ptr<Expr> defaultValue = nullptr;
};

class FuncStmt : public Stmt
{
    public:
    Token name;
    std::vector<Param> params;
    Token returnType;
    std::unique_ptr<Stmt> body;
    FuncStmt(size_t line, Token name, std::vector<Param> params, Token retType, std::unique_ptr<Stmt> body)
        : Stmt(line), name(name), params(std::move(params)), returnType(retType), body(std::move(body)) {}
};

class ReturnStmt : public Stmt {
    public:
    std::unique_ptr<Expr> value; 
    ReturnStmt(size_t line, std::unique_ptr<Expr> value) : Stmt(line), value(std::move(value)) {}
};

class BreakStmt : public Stmt 
{ 
    public: BreakStmt(size_t line) : Stmt(line) {} 
};

class ContinueStmt : public Stmt 
{ 
    public: ContinueStmt(size_t line) : Stmt(line) {} 
};

class ExitStmt : public Stmt {
    public:
    std::unique_ptr<Expr> code; 
    ExitStmt(size_t line, std::unique_ptr<Expr> code) : Stmt(line), code(std::move(code)) {}
};

class ExprStmt : public Stmt { 
    public:
    std::unique_ptr<Expr> expr;
    ExprStmt(size_t line, std::unique_ptr<Expr> expr) : Stmt(line), expr(std::move(expr)) {}
};

class BlockStmt : public Stmt {
    public:
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt(size_t line, std::vector<std::unique_ptr<Stmt>> stmts)
        : Stmt(line), statements(std::move(stmts)) {}
};
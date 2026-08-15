#pragma once 
#include "Node.hpp"
#include "../common/Token.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class Expr : public Node 
{
    public:
        Expr(size_t line) : Node(line) {}
        virtual ~Expr() = default;
};

class LiteralExpr : public Expr 
{
    public:
    Token valueToken;

        LiteralExpr(size_t line, Token token) : Expr(line), valueToken(token) {}
        ~LiteralExpr() = default;
};

class BinaryExpr : public Expr 
{   
    public:
    std::unique_ptr<Expr> left;
    Token operatorToken;
    std::unique_ptr<Expr> right;
    
    

    BinaryExpr(size_t line, std::unique_ptr<Expr>l,Token op, std::unique_ptr<Expr>r) :Expr(line), left(std::move(l)), operatorToken(op), right(std::move(r)) {}
    ~BinaryExpr() = default;
};

class VariableExpr :  public Expr 
{
    public:
        Token name;
    
    

    VariableExpr(size_t line, Token varName):Expr(line), name(varName) {}

};

class UnaryExpr : public Expr
{
    public:
    Token operatorToken;
    std::unique_ptr<Expr> operand;

        UnaryExpr(size_t line, Token op, std::unique_ptr<Expr> opr) :Expr(line), operatorToken(op), operand(std::move(opr)) {}
};

class AssignExpr : public Expr
{
    public:
    Token name;
    std::unique_ptr<Expr> value;
        AssignExpr(size_t line, Token name, std::unique_ptr<Expr> val) :Expr(line), name(name) , value(std::move(val)) {}
};

class CallExpr : public Expr 
{
    public:
    Token callee;
    std::vector<std::unique_ptr<Expr>> arguments;

        CallExpr(size_t line, Token callee, std::vector<std::unique_ptr<Expr>> args) : Expr(line), callee(callee), arguments(std::move(args)) {}
};

class IndexExpr : public Expr
{
    public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
        IndexExpr(size_t line, std::unique_ptr<Expr> target, std::unique_ptr<Expr> index)
            : Expr(line), target(std::move(target)), index(std::move(index)) {}
};

class IndexAssignExpr : public Expr
{
    public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
        IndexAssignExpr(size_t line, std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value)
            : Expr(line), target(std::move(target)), index(std::move(index)), value(std::move(value)) {}
};

class MemberCallExpr : public Expr
{
    public:
    std::unique_ptr<Expr> target;
    Token methodName;
    std::vector<std::unique_ptr<Expr>> arguments;
        MemberCallExpr(size_t line, std::unique_ptr<Expr> target, Token method, std::vector<std::unique_ptr<Expr>> args)
            : Expr(line), target(std::move(target)), methodName(method), arguments(std::move(args)) {}
};

class IncrementExpr : public Expr
{
    public:
    Token name;
    bool isIncrement; 
        IncrementExpr(size_t line, Token name, bool isInc)
            : Expr(line), name(name), isIncrement(isInc) {}
};

class TernaryExpr : public Expr
{
    public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueExpr;
    std::unique_ptr<Expr> falseExpr;
        TernaryExpr(size_t line, std::unique_ptr<Expr> cond, std::unique_ptr<Expr> t, std::unique_ptr<Expr> f)
            : Expr(line), condition(std::move(cond)), trueExpr(std::move(t)), falseExpr(std::move(f)) {}
};

class CompoundAssignExpr : public Expr
{
    public:
    Token name;
    Token op;
    std::unique_ptr<Expr> value;
        CompoundAssignExpr(size_t line, Token name, Token op, std::unique_ptr<Expr> val)
            : Expr(line), name(name), op(op), value(std::move(val)) {}
};

class CompoundIndexAssignExpr : public Expr
{
    public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    Token op;
    std::unique_ptr<Expr> value;
        CompoundIndexAssignExpr(size_t line, std::unique_ptr<Expr> target, std::unique_ptr<Expr> index, Token op, std::unique_ptr<Expr> value)
            : Expr(line), target(std::move(target)), index(std::move(index)), op(op), value(std::move(value)) {}
};

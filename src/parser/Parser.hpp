#pragma once 

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include "../common/Token.hpp"
#include "../ast/Stmt.hpp"
#include "../ast/Expr.hpp"
#include "../utils/Error.hpp"

class ParserError : public std::runtime_error
{
    public:
    ParserError(const std::string& errorMessage) : std::runtime_error(errorMessage) {}
};
class Parser 
{
    private:
        const std::vector<Token> &m_source;
        size_t m_current{0};
        ErrorReporter* m_reporter{nullptr};


        // Helpers --- 

        bool isAtEnd()const;
        const Token& peek() const;
        const Token& peekNext()const;
        const Token& previous()const;
        const Token& advance();
        bool match(std::initializer_list<TokenType> types);
        bool check(TokenType type) const;
        void error(const Token& token, const std::string& message, const std::string& hint = "");
        Token consume(TokenType type, const std::string& errorMessage, const std::string& hint = "");

        // Statements ---
    public:
        std::unique_ptr<Stmt> parseStatement();
        std::unique_ptr<Stmt>parseOutStatement();
        std::unique_ptr<Stmt>parseInStatement();
        std::unique_ptr<Stmt> parseVarDecStatement(bool isConst);
        std::unique_ptr<Stmt>parseListDecStatement(bool isConst);
        std::unique_ptr<Stmt> parseIfStatement();
        std::unique_ptr<Stmt> parseWhileStatement();
        std::unique_ptr<Stmt>parseForStatement();
        std::unique_ptr<Stmt> parseFuncDecStatement();
        std::vector<Param> parseParamList();
        std::unique_ptr<Expr> parseCall();
        std::unique_ptr<Stmt> parseBreakStatement();
        std::unique_ptr<Stmt> parseContinueStatement();
        std::unique_ptr<Stmt> parseExitStatement();
        std::unique_ptr<Stmt> parseExpressionStatement();
        std::unique_ptr<Stmt> parseReturnStatement();
        std::vector<std::unique_ptr<Stmt>> parseBlock();

        // Expressions ---

        std::unique_ptr<Expr> parseExpression();
        std::unique_ptr<Expr> parseAssignment();
        std::unique_ptr<Expr> parseTernary();
        std::unique_ptr<Expr> parseOr();
        std::unique_ptr<Expr> parseAnd();
        std::unique_ptr<Expr> parseEquality();
        std::unique_ptr<Expr> parseComparison();
        std::unique_ptr<Expr> parseTerm();
        std::unique_ptr<Expr> parseUnary();
        std::unique_ptr<Expr> parseFactor();
        std::unique_ptr<Expr> parsePrimary();
    
        
        explicit Parser(const std::vector<Token> &tokens, ErrorReporter* reporter = nullptr) 
            : m_source(tokens), m_current(0), m_reporter(reporter) {};
        std::vector<std::unique_ptr<Stmt>> parse();
};
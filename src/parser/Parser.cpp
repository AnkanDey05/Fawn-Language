#include "Parser.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>


// Helper Functions ---

bool Parser::isAtEnd() const
{
    return peek().m_type == TokenType::EndOfFile;
}
const Token& Parser::peek() const
{
    return m_source[m_current];
}
const Token& Parser::peekNext() const
{
    if ((m_current + 1 >= m_source.size())) 
    {
        return m_source[m_source.size() - 1];    
    }
    return m_source[m_current + 1];
}
const Token& Parser::previous() const
{
    if (m_current == 0) {
        return m_source[m_current];
    }
    return m_source[m_current - 1];
}
const Token& Parser::advance()
{
    if (!isAtEnd()) {
        m_current++;
    }
    return previous();
}
bool Parser::match(std::initializer_list<TokenType> types)
{
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}
bool Parser::check(TokenType type) const
{
    if (isAtEnd()) {
        return false;
    }
    return peek().m_type == type;
}

void Parser::error(const Token& token, const std::string& message, const std::string& hint)
{
    if (m_reporter) {
        m_reporter->report({ErrorKind::Syntax, message, token.m_line, hint, token.m_column});
    }
    throw ParserError(message + " (got '" + token.m_lexeme + "' at line " + std::to_string(token.m_line) + ")");
}

Token Parser::consume(TokenType type, const std::string& errorMessage, const std::string& hint)
{
    if (check(type)) {
        return advance();
    }
    error(peek(), errorMessage, hint);
    return peek();
}


// expressions ---

std::unique_ptr<Expr> Parser::parsePrimary()
{
    if (match({TokenType::Number, TokenType::String, TokenType::True, TokenType::False, TokenType::Null})) {
        return std::make_unique<LiteralExpr>(m_current > 0 ? previous().m_line : 0, previous());
    }
    if (match({TokenType::Identifier})) {
        return std::make_unique<VariableExpr>(m_current > 0 ? previous().m_line : 0, previous());
    }
    if (match({TokenType::TypeKeyword})) {
        return std::make_unique<VariableExpr>(previous().m_line, previous());
    }
    if (match({TokenType::LeftParen})) {
        auto expr = parseExpression();
        consume(TokenType::RightParen, "Expected ')' after expression.", "Add a matching closing parenthesis ')'");
        return expr;
    }
    if (match({TokenType::If})) {
        auto cond = parseOr();
        consume(TokenType::Then, "Expected 'then' in inline if-expression.", "Inline if-expression format: if condition then expr else expr");
        auto trueExpr = parseExpression();
        consume(TokenType::Else, "Expected 'else' in inline if-expression.", "Inline if-expression format: if condition then expr else expr");
        auto falseExpr = parseExpression();
        return std::make_unique<TernaryExpr>(previous().m_line, std::move(cond), std::move(trueExpr), std::move(falseExpr));
    }
    error(peek(), "Expected an expression.", "Check for a missing value, variable, or literal.");
    return nullptr;
}


std::unique_ptr<Expr> Parser::parseFactor()
{
    auto expr = parseUnary();

    while (match({TokenType::Slash, TokenType::Star, TokenType::Modulo})) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary()
{
    if (match({TokenType::Not, TokenType::Minus})) {
        Token op = previous();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(op.m_line, op, std::move(operand));
    }
    if (match({TokenType::Increment, TokenType::Decrement})) {
        bool isInc = previous().m_type == TokenType::Increment;
        Token name = consume(TokenType::Identifier, "Expected variable name after '" + previous().m_lexeme + "'.", "Increment/decrement ('++' / '--') can only be applied to variables.");
        return std::make_unique<IncrementExpr>(name.m_line, name, isInc);
    }
    return parseCall();
}

std::unique_ptr<Expr> Parser::parseTerm()
{
    auto expr = parseFactor();

    while (match({TokenType::Plus, TokenType::Minus})) {
        Token op = previous();
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison()
{
    auto expr = parseTerm();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality()
{
    auto expr = parseComparison();

    while (match({TokenType::EqualEqual, TokenType::NotEqual, TokenType::EqualEqualEqual})) {
        Token op = previous();
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAnd()
{
    auto expr = parseEquality();

    while (match({TokenType::And})) {
        Token op = previous();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseOr()
{
    auto expr = parseAnd();

    while (match({TokenType::Or})) {
        Token op = previous();
        auto right = parseAnd();
        expr = std::make_unique<BinaryExpr>(op.m_line, std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTernary()
{
    auto expr = parseOr();

    if (match({TokenType::QuestionMark})) {
        auto trueExpr = parseTernary();
        consume(TokenType::Colon, "Expected ':' in ternary expression.", "Ternary expression format: condition ? true_expr : false_expr");
        auto falseExpr = parseTernary();
        return std::make_unique<TernaryExpr>(previous().m_line, std::move(expr), std::move(trueExpr), std::move(falseExpr));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAssignment()
{
    auto expr = parseTernary();

    if (match({TokenType::Equal, TokenType::PlusEqual, TokenType::MinusEqual, TokenType::StarEqual, TokenType::SlashEqual, TokenType::ModuloEqual})) {
        Token opToken = previous();
        auto value = parseAssignment();

        if (opToken.m_type == TokenType::Equal) {
            if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
                Token name = varExpr->name;
                return std::make_unique<AssignExpr>(opToken.m_line, name, std::move(value));
            }
            if (auto* indexExpr = dynamic_cast<IndexExpr*>(expr.get())) {
                return std::make_unique<IndexAssignExpr>(
                    opToken.m_line,
                    std::move(indexExpr->target),
                    std::move(indexExpr->index),
                    std::move(value));
            }
            error(peek(), "Invalid assignment target.", "You can only assign values to variables or list indices (e.g. x = 5 or arr[0] = 5).");
        } else {
            if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
                Token name = varExpr->name;
                return std::make_unique<CompoundAssignExpr>(opToken.m_line, name, opToken, std::move(value));
            }
            if (auto* indexExpr = dynamic_cast<IndexExpr*>(expr.get())) {
                return std::make_unique<CompoundIndexAssignExpr>(
                    opToken.m_line,
                    std::move(indexExpr->target),
                    std::move(indexExpr->index),
                    opToken,
                    std::move(value));
            }
            error(peek(), "Invalid compound assignment target.", "Compound operators (+=, -=, etc.) can only target variables or list indices.");
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseExpression()
{
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseCall()
{
    auto expr = parsePrimary();

    while (true) {
        if (match({TokenType::LeftParen})) {
            std::vector<std::unique_ptr<Expr>> arguments;
            while (match({TokenType::NewLine})) {}
            if (!check(TokenType::RightParen)) {
                do {
                    while (match({TokenType::NewLine})) {}
                    arguments.push_back(parseExpression());
                    while (match({TokenType::NewLine})) {}
                } while (match({TokenType::Comma}));
                while (match({TokenType::NewLine})) {}
            }
            Token paren = consume(TokenType::RightParen, "Expected ')' after arguments.", "Check for a missing closing parenthesis ')' in function call.");
            if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
                expr = std::make_unique<CallExpr>(paren.m_line, varExpr->name, std::move(arguments));
            } else {
                error(peek(), "Can only call functions by name.", "Function calls must target a valid function identifier.");
            }
        } else if (match({TokenType::LeftBracket})) {
            auto index = parseExpression();
            Token bracket = consume(TokenType::RightBracket, "Expected ']' after index.", "Check for a missing closing bracket ']' in array/string indexing.");
            expr = std::make_unique<IndexExpr>(bracket.m_line, std::move(expr), std::move(index));
        } else if (match({TokenType::Dot})) {
            // Member call:
            Token method = consume(TokenType::Identifier, "Expected method name after '.'.", "Use a valid method name after '.', e.g. str.caps() or list.put()");
            consume(TokenType::LeftParen, "Expected '(' after method name.", "Call methods using parentheses, e.g. list.put(item)");
            std::vector<std::unique_ptr<Expr>> arguments;
            while (match({TokenType::NewLine})) {}
            if (!check(TokenType::RightParen)) {
                do {
                    while (match({TokenType::NewLine})) {}
                    arguments.push_back(parseExpression());
                    while (match({TokenType::NewLine})) {}
                } while (match({TokenType::Comma}));
                while (match({TokenType::NewLine})) {}
            }
            consume(TokenType::RightParen, "Expected ')' after method arguments.", "Close method call with a matching parenthesis ')'");
            expr = std::make_unique<MemberCallExpr>(method.m_line, std::move(expr), method, std::move(arguments));
        } else if (check(TokenType::Increment) || check(TokenType::Decrement)) {
            // Postfix x++ / x--
            Token op = advance();
            if (auto* varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
                expr = std::make_unique<IncrementExpr>(op.m_line, varExpr->name, op.m_type == TokenType::Increment);
            } else {
                error(op, "'++' / '--' can only be used on variables.", "Postfix increment/decrement must target a variable name.");
            }
        } else {
            break;
        }
    }

    return expr;
}


// Statements ---

std::unique_ptr<Stmt> Parser::parseStatement()
{
    while (match({TokenType::NewLine})) {}
    if (isAtEnd()) return nullptr;
    
    // Func and variablees  

    if (match({TokenType::Func}))
        return parseFuncDecStatement();
    if (match({TokenType::Const})) {
        if (check(TokenType::List)) 
        {
            advance();
            return parseListDecStatement(true);
        }
        if (check(TokenType::TypeKeyword)) 
        {
            return parseVarDecStatement(true);
        }
        error(previous(), "Expected a type or 'list' after 'const'.", "Format: const int x = 10 or const list nums = {1, 2}");
    }
    if (check(TokenType::TypeKeyword) && peekNext().m_type == TokenType::LeftParen) {
        return parseExpressionStatement();
    }
    if (match({TokenType::TypeKeyword, TokenType::Var}))
        return parseVarDecStatement(false);
    if (match({TokenType::List}))
        return parseListDecStatement(false);
    
    // I/O streams

    if (match({TokenType::Out}))
        return parseOutStatement();
    if (match({TokenType::In}))
        return parseInStatement();

    // Control flow

    if (match({TokenType::If}))
        return parseIfStatement();
    if (match({TokenType::LeftBrace}))
        return std::make_unique<BlockStmt>(previous().m_line, parseBlock());
    if (match({TokenType::While}))
        return parseWhileStatement();
    if (match({TokenType::For}))
        return parseForStatement();
    if (match({TokenType::Break}))
        return parseBreakStatement();
    if (match({TokenType::Continue}))
        return parseContinueStatement();
    if (match({TokenType::Return}))
        return parseReturnStatement();
    if (match({TokenType::Exit}))
        return parseExitStatement();

    return parseExpressionStatement();
}


std::unique_ptr<Stmt> Parser::parseOutStatement()
{   
    auto line  = previous().m_line;
    std::vector<std::unique_ptr<Expr>> values;
    while (match({TokenType::ShiftLeft})) {
        if (match({TokenType::End})) {
            values.push_back(std::make_unique<LiteralExpr>(
            line, Token(TokenType::End, "end", line)));
        } else {
            values.push_back(parseExpression());
        }
    }
    if (values.empty()) {
        error(previous(), "Expected '<<' after 'out'/'print'.", "Print statements use format: out << expression << end");
    }
    return std::make_unique<OutStmt>(line, std::move(values));
}

std::unique_ptr<Stmt> Parser::parseInStatement()
{
    size_t line = previous().m_line;
    consume(TokenType::ShiftRight, "Expected '>>' after 'in'.", "Input statements use format: in >> variable_name");
    Token target = consume(TokenType::Identifier, "Expected variable name after 'in >>'.", "Provide a variable identifier to store input.");
    return std::make_unique<InStmt>(line, std::move(target));
}

std::vector<std::unique_ptr<Stmt>> Parser::parseBlock()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        if (match({TokenType::NewLine})) {
            continue;
        }
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    consume(TokenType::RightBrace, "Unclosed block: expected '}'.", "Add a matching closing curly brace '}'");
    return statements;
}

std::unique_ptr<Stmt> Parser::parseVarDecStatement(bool isConst)
{
    Token type(TokenType::Null, "", previous().m_line);
    if (isConst) {
        if (check(TokenType::TypeKeyword) || check(TokenType::Var)) {
            type = advance();
        } else {
            error(previous(), "Expected type after 'const'.", "Specify a type like int, float, string, bool, or var after 'const'");
        }
    } else {
        type = previous();
    }
    Token name = consume(TokenType::Identifier, "Expected variable name in declaration.", "Provide a valid variable identifier.");
    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::Equal})) {
        initializer = parseExpression();
    } else if (isConst) {
        error(name, "Const variable must be initialized.", "Provide an initial value when declaring a const variable, e.g. const int x = 10.");
    }
    return std::make_unique<VarDecStmt>(type.m_line, type, name, std::move(initializer), isConst);
}

std::unique_ptr<Stmt> Parser::parseListDecStatement(bool isConst)
{
    size_t line = previous().m_line; // the 'list' token
    Token name = consume(TokenType::Identifier, "Expected list name.", "Provide a name for the list, e.g. list nums = {1, 2, 3}");

    std::unique_ptr<Expr> size = nullptr;
    if (match({TokenType::LeftBracket})) {
        size = parseExpression();
        consume(TokenType::RightBracket, "Expected ']' after list capacity size.", "Format: list[capacity] nums = {...}");
    }

    Token elementType(TokenType::Null, "", line); // default: untyped
    if (match({TokenType::Colon})) {
        elementType = consume(TokenType::TypeKeyword, "Expected element type after ':'.", "Format: list nums: int = {1, 2, 3}");
    }

    std::vector<std::unique_ptr<Expr>> elements;
    std::unique_ptr<Expr> initExpr = nullptr;

    if (match({TokenType::Equal})) {
        while (match({TokenType::NewLine})) {}
        consume(TokenType::LeftBrace, "Expected '{' to start list literal.", "Enclose list elements in curly braces '{...}'");
        while (match({TokenType::NewLine})) {}
        if (!check(TokenType::RightBrace)) {
            do {
                while (match({TokenType::NewLine})) {}
                elements.push_back(parseExpression());
                while (match({TokenType::NewLine})) {}
            } while (match({TokenType::Comma}));
        }
        while (match({TokenType::NewLine})) {}
        consume(TokenType::RightBrace, "Expected '}' after list elements.", "Close list initializer with '}'");
    }

    return std::make_unique<ListDecStmt>(line, name, std::move(size), elementType, std::move(elements), isConst, std::move(initExpr));
}




std::unique_ptr<Stmt> Parser::parseIfStatement()
{
    auto line = previous().m_line;
    auto con = parseExpression();
    while (match({TokenType::NewLine})) {}

    std::unique_ptr<Stmt> thenBlock = nullptr;

    if (match({TokenType::Then})) {
        // One-liner If:
        if (check(TokenType::LeftBrace)) {
            error(previous(), "Cannot use '{' after 'then'.", "Use 'if condition { ... }' for block statements.");
        }
        auto stmt = parseStatement();
        std::vector<std::unique_ptr<Stmt>> stmts;
        stmts.push_back(std::move(stmt));
        thenBlock = std::make_unique<BlockStmt>(line, std::move(stmts));
    } else {
        // Block:
        consume(TokenType::LeftBrace, "Expected '{' or 'then' after condition.", "Use 'if condition { ... }'");
        auto thenstmt = parseBlock();
        thenBlock = std::make_unique<BlockStmt>(line, std::move(thenstmt));
    }

    // skip newline before if /elif
    while (match({TokenType::NewLine})) {}
    std::unique_ptr<Stmt> elseBlock = nullptr;

    if (match({TokenType::Elif})) {
        elseBlock = parseIfStatement();
    } else if (match({TokenType::Else})) {
        while (match({TokenType::NewLine})) {}
        if (match({TokenType::Then})) {
            error(previous(), "'else' does not need 'then'.", "Use 'else <stmt>' or 'else { ... }'");
        }
        if (check(TokenType::LeftBrace)) {
            //Else Block 
            advance();
            auto elseStatment = parseBlock();
            elseBlock = std::make_unique<BlockStmt>(previous().m_line, std::move(elseStatment));
        } else {
            //One line Else
            auto stmt = parseStatement();
            std::vector<std::unique_ptr<Stmt>> stmts;
            stmts.push_back(std::move(stmt));
            elseBlock = std::make_unique<BlockStmt>(previous().m_line, std::move(stmts));
        }
    }

    return std::make_unique<IfStmt>(line, std::move(con), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<Stmt> Parser::parseWhileStatement()
{
    auto line = previous().m_line;
    auto con = parseExpression();
    // Skip newlines before opening brace
    while (match({TokenType::NewLine})) {}
    consume(TokenType::LeftBrace, "Expected '{' after condition.", "Format: while condition { ... }");
    auto bodystmt = parseBlock();
    auto bodyBlock = std::make_unique<BlockStmt>(line, std::move(bodystmt));
    return std::make_unique<WhileStmt>(line, std::move(con), std::move(bodyBlock));
}

std::unique_ptr<Stmt> Parser::parseForStatement()
{
    auto line = previous().m_line;
    auto name = consume(TokenType::Identifier, "Expected variable name after 'for'.", "Format: for i in 1 -> 10 { ... }");
    consume(TokenType::In, "Expected 'in' after variable name.", "Format: for i in 1 -> 10 { ... }");
    auto from = parseExpression();
    consume(TokenType::Arrow, "Expected '->' between loop range numbers.", "Format: for i in 1 -> 10 { ... }");
    auto to = parseExpression();
    std::unique_ptr<Expr> step = nullptr;
    if (match({TokenType::Step})) {
        step = parseExpression();
    }
    // Skip newlines before opening brace
    while (match({TokenType::NewLine})) {}
    consume(TokenType::LeftBrace, "Expected '{' after loop range.", "Enclose loop body in curly braces '{ ... }'");
    auto bodystmt = parseBlock();
    auto bodyBlock = std::make_unique<BlockStmt>(previous().m_line, std::move(bodystmt));

    return std::make_unique<ForStmt>(line, name, std::move(from), std::move(to), std::move(step), std::move(bodyBlock));
}

std::vector<Param> Parser::parseParamList()
{
    std::vector<Param> paramlist;
    bool hadDefault = false;

    while (match({TokenType::NewLine})) {}
    if (!check(TokenType::RightParen)) {
        do {
            while (match({TokenType::NewLine})) {}
            Token type(TokenType::Null, "", previous().m_line);
            if (check(TokenType::TypeKeyword) || check(TokenType::Var) || check(TokenType::List)) {
                type = advance();
            } else {
                error(peek(), "Expected parameter type.", "Specify a parameter type like int, float, string, bool, or var.");
            }
            Token name = consume(TokenType::Identifier, "Expected parameter name.", "Provide a parameter identifier.");
            std::shared_ptr<Expr> defaultValue = nullptr;
            if (match({TokenType::Equal})) {
                defaultValue = parseExpression();
                hadDefault = true;
            } else if (hadDefault) {
                error(name, "Parameter '" + name.m_lexeme + "' must have a default value.", "All parameters after a default value must also have default values.");
            }
            paramlist.push_back({type, name, defaultValue});
            while (match({TokenType::NewLine})) {}
        } while (match({TokenType::Comma}));
        while (match({TokenType::NewLine})) {}
    }

    return paramlist;
}

std::unique_ptr<Stmt> Parser::parseFuncDecStatement()
{
    auto line = previous().m_line;
    auto name = consume(TokenType::Identifier, "Expected function name after 'fn'.", "Format: fn function_name() { ... }");
    consume(TokenType::LeftParen, "Expected '(' after function name.", "Enclose function parameters in parentheses '()'");
    auto params = parseParamList();
    consume(TokenType::RightParen, "Expected ')' after parameters.", "Close parameter list with ')'");
    Token type(TokenType::Null, "", previous().m_line);
    if (match({TokenType::Colon})) {
        type = consume(TokenType::TypeKeyword, "Expected return type after ':'.", "Specify return type like int, float, string, or bool.");
    }
    // Skip newlines before opening brace
    while (match({TokenType::NewLine})) {}
    consume(TokenType::LeftBrace, "Expected '{' before function body.", "Enclose function body in curly braces '{ ... }'");
    auto bodystmt = parseBlock();
    auto bodyBlock = std::make_unique<BlockStmt>(previous().m_line, std::move(bodystmt));

    return std::make_unique<FuncStmt>(line, name, std::move(params), type, std::move(bodyBlock));
}

std::unique_ptr<Stmt> Parser::parseBreakStatement()
{
    size_t line = previous().m_line;
    return std::make_unique<BreakStmt>(line);
}

std::unique_ptr<Stmt> Parser::parseContinueStatement()
{
    size_t line = previous().m_line;
    return std::make_unique<ContinueStmt>(line);
}

std::unique_ptr<Stmt> Parser::parseExitStatement()
{
    size_t line = previous().m_line;
    std::unique_ptr<Expr> code = nullptr;

    if (!check(TokenType::NewLine) && !check(TokenType::RightBrace) && !isAtEnd()) {
        code = parseExpression();
    }

    return std::make_unique<ExitStmt>(line, std::move(code));
}

std::unique_ptr<Stmt> Parser::parseReturnStatement()
{
    size_t line = previous().m_line;
    std::unique_ptr<Expr> value = nullptr;

    if (!check(TokenType::NewLine) && !check(TokenType::RightBrace) && !isAtEnd()) {
        value = parseExpression();
    }

    return std::make_unique<ReturnStmt>(line, std::move(value));
}

std::unique_ptr<Stmt> Parser::parseExpressionStatement()
{
    auto expr = parseExpression();
    return std::make_unique<ExprStmt>(expr->m_line, std::move(expr));
}


std::vector<std::unique_ptr<Stmt>> Parser::parse()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        if (match({TokenType::NewLine})) {
            continue;
        }
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    return statements;
}

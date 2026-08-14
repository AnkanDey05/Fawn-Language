#include "CodeRunner.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <iomanip>
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../interpreter/Interpreter.hpp"
#include "../common/Config.hpp"
#include "../ast/Expr.hpp"
#include "../ast/Stmt.hpp"
#include "../common/Token.hpp"
#include "../utils/Error.hpp"

void CodeRunner::runFile(const std::string& path) {
    std::string source = filemanager.read(path);
    ErrorReporter reporter(path, source);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    try {
        auto stmt = parser.parse();
        Interpreter intrp(&reporter);
        intrp.interpret(stmt);
        if (reporter.haserror()) reporter.flush();
    } catch (ParserError& error) {
        std::cerr << "Error: " << error.what() << "\n";
    }
}

void CodeRunner::checkFile(const std::string& path)
{
    std::string source  = filemanager.read(path);

    try {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();
    } catch (ParserError& error) {
        std::cout<< "Error: " << error.what() << "\n";
    }
}

void CodeRunner::printVersion()
{
    std::cout<< Fawn::NAME << " v"<< Fawn::VERSION_MAJOR<<"."<<Fawn::VERSION_MINOR<<"."<<Fawn::VERSION_PATCH;
}

void CodeRunner::runREPL()
{
    ErrorReporter reporter("<repl>", "");
    Interpreter interp(&reporter);
    std::string line;
    printVersion();
    std::cout<<" REPL Mode\n";
    while (true) {
        std::cout<<Fawn::PROMPT<< " ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        try {
        Lexer lexer(line);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto stmts = parser.parse();
        interp.interpret(stmts);
        if (reporter.haserror()) reporter.flush();
        } catch (std::runtime_error& error) {
            std::cerr<< "Error: " << error.what() << "\n";
            continue;
        }
    }    
}


static const char* tokenTypeToString(TokenType type)
{
    switch (type)
    {
        // Literals
        case TokenType::Number:           return "Number";
        case TokenType::String:           return "String";
        case TokenType::Identifier:       return "Identifier";

        // Keywords / types
        case TokenType::TypeKeyword:             return "Type";
        case TokenType::Func:             return "Func";
        case TokenType::Out:              return "Out";
        case TokenType::If:               return "If";
        case TokenType::Elif:             return "Elif";
        case TokenType::Else:             return "Else";
        case TokenType::While:            return "While";
        case TokenType::For:              return "For";
        case TokenType::In:               return "In";
        case TokenType::Step:             return "Step";
        case TokenType::Print:            return "Print";
        case TokenType::Read:             return "Read";
        case TokenType::Var:              return "Var";
        case TokenType::Const:            return "Const";
        case TokenType::List:             return "List";
        case TokenType::True:             return "True";
        case TokenType::False:            return "False";
        case TokenType::Null:             return "Null";
        case TokenType::End:              return "End";
        case TokenType::Return:           return "Return";
        case TokenType::Break:            return "Break";
        case TokenType::Continue:         return "Continue";
        case TokenType::Escape:           return "Escape";
        case TokenType::Exit:             return "Exit";

        // Operators
        case TokenType::Arrow:            return "Arrow";
        case TokenType::Plus:             return "Plus";
        case TokenType::Minus:            return "Minus";
        case TokenType::Star:             return "Star";
        case TokenType::Slash:            return "Slash";
        case TokenType::Modulo:           return "Modulo";
        case TokenType::Increment:        return "Increment";
        case TokenType::Decrement:        return "Decrement";

        case TokenType::Equal:            return "Equal";
        case TokenType::EqualEqual:       return "EqualEqual";
        case TokenType::EqualEqualEqual:  return "EqualEqualEqual";
        case TokenType::NotEqual:         return "NotEqual";

        case TokenType::Greater:          return "Greater";
        case TokenType::GreaterEqual:     return "GreaterEqual";
        case TokenType::Less:             return "Less";
        case TokenType::LessEqual:        return "LessEqual";

        case TokenType::And:              return "And";
        case TokenType::Or:               return "Or";
        case TokenType::Not:              return "Not";

        case TokenType::ShiftLeft:        return "ShiftLeft";
        case TokenType::ShiftRight:       return "ShiftRight";

        // Symbols
        case TokenType::LeftParen:        return "LeftParen";
        case TokenType::RightParen:       return "RightParen";
        case TokenType::LeftBrace:        return "LeftBrace";
        case TokenType::RightBrace:       return "RightBrace";
        case TokenType::LeftBracket:      return "LeftBracket";
        case TokenType::RightBracket:     return "RightBracket";
        case TokenType::Comma:            return "Comma";
        case TokenType::Colon:            return "Colon";
        case TokenType::Dot:              return "Dot";
        case TokenType::QuestionMark:     return "QuestionMark";

        // Misc
        case TokenType::NewLine:          return "NewLine";
        case TokenType::EndOfFile:        return "EndOfFile";
        case TokenType::Invalid:          return "Invalid";
        default: return "Unknown";
    }

    return "Unknown";
}

static void dumpExpr(const Expr* expr, int depth = 0)
{
    if (!expr)
        return;

    const std::string indent(depth * 2, ' ');

    // Literal
    if (const auto* node = dynamic_cast<const LiteralExpr*>(expr))
    {
        std::cout
            << indent
            << "Literal("
            << node->valueToken.m_lexeme
            << ")\n";
    }

    // Variable
    else if (const auto* node = dynamic_cast<const VariableExpr*>(expr))
    {
        std::cout
            << indent
            << "Variable("
            << node->name.m_lexeme
            << ")\n";
    }

    // Binary
    else if (const auto* node = dynamic_cast<const BinaryExpr*>(expr))
    {
        std::cout
            << indent
            << "Binary("
            << node->operatorToken.m_lexeme
            << ")\n";

        dumpExpr(node->left.get(), depth + 1);
        dumpExpr(node->right.get(), depth + 1);
    }

    // Unary
    else if (const auto* node = dynamic_cast<const UnaryExpr*>(expr))
    {
        std::cout
            << indent
            << "Unary("
            << node->operatorToken.m_lexeme
            << ")\n";

        dumpExpr(node->operand.get(), depth + 1);
    }

    // Assignment
    else if (const auto* node = dynamic_cast<const AssignExpr*>(expr))
    {
        std::cout
            << indent
            << "Assign("
            << node->name.m_lexeme
            << ")\n";

        dumpExpr(node->value.get(), depth + 1);
    }

    // Function call
    else if (const auto* node = dynamic_cast<const CallExpr*>(expr))
    {
        std::cout
            << indent
            << "Call("
            << node->callee.m_lexeme
            << ")\n";

        for (const auto& arg : node->arguments)
        {
            dumpExpr(arg.get(), depth + 1);
        }
    }

    // Array indexing
    else if (const auto* node = dynamic_cast<const IndexExpr*>(expr))
    {
        std::cout
            << indent
            << "Index:\n";

        std::cout << indent << "  Target:\n";
        dumpExpr(node->target.get(), depth + 2);

        std::cout << indent << "  Index:\n";
        dumpExpr(node->index.get(), depth + 2);
    }

    // Array index assignment
    else if (const auto* node =
                 dynamic_cast<const IndexAssignExpr*>(expr))
    {
        std::cout
            << indent
            << "IndexAssign:\n";

        std::cout << indent << "  Target:\n";
        dumpExpr(node->target.get(), depth + 2);

        std::cout << indent << "  Index:\n";
        dumpExpr(node->index.get(), depth + 2);

        std::cout << indent << "  Value:\n";
        dumpExpr(node->value.get(), depth + 2);
    }

    // Member call
    else if (const auto* node =
                 dynamic_cast<const MemberCallExpr*>(expr))
    {
        std::cout
            << indent
            << "MemberCall("
            << node->methodName.m_lexeme
            << ")\n";

        std::cout << indent << "  Target:\n";
        dumpExpr(node->target.get(), depth + 2);

        if (!node->arguments.empty())
        {
            std::cout << indent << "  Arguments:\n";

            for (const auto& arg : node->arguments)
            {
                dumpExpr(arg.get(), depth + 2);
            }
        }
    }

    // Increment / decrement
    else if (const auto* node =
                 dynamic_cast<const IncrementExpr*>(expr))
    {
        std::cout
            << indent
            << (node->isIncrement ? "Increment(" : "Decrement(")
            << node->name.m_lexeme
            << ")\n";
    }

    // Ternary
    else if (const auto* node =
                 dynamic_cast<const TernaryExpr*>(expr))
    {
        std::cout << indent << "Ternary:\n";

        std::cout << indent << "  Condition:\n";
        dumpExpr(node->condition.get(), depth + 2);

        std::cout << indent << "  True:\n";
        dumpExpr(node->trueExpr.get(), depth + 2);

        std::cout << indent << "  False:\n";
        dumpExpr(node->falseExpr.get(), depth + 2);
    }

    else
    {
        std::cout
            << indent
            << "Unknown expr node\n";
    }
}


static void dumpStmt(const Stmt* stmt, int depth = 0)
{
    if (!stmt)
        return;

    const std::string indent(depth * 2, ' ');

    // Variable declaration
    if (const auto* node =
            dynamic_cast<const VarDecStmt*>(stmt))
    {
        std::cout
            << indent
            << "VarDecl("
            << (node->isConst ? "const, " : "")
            << "type="
            << node->type.m_lexeme
            << ", name="
            << node->name.m_lexeme
            << ")\n";

        if (node->initializer)
        {
            std::cout << indent << "Initializer:\n";
            dumpExpr(node->initializer.get(), depth + 1);
        }
    }

    // List declaration
    else if (const auto* node =
                 dynamic_cast<const ListDecStmt*>(stmt))
    {
        std::cout
            << indent
            << "ListDecl("
            << (node->isConst ? "const, " : "")
            << "name="
            << node->name.m_lexeme;

        if (node->elementType.m_type != TokenType::Null)
        {
            std::cout
                << ", type="
                << node->elementType.m_lexeme;
        }

        std::cout << ")\n";

        if (node->size)
        {
            std::cout << indent << "Size:\n";
            dumpExpr(node->size.get(), depth + 1);
        }

        if (!node->elements.empty())
        {
            std::cout << indent << "Elements:\n";

            for (const auto& element : node->elements)
            {
                dumpExpr(element.get(), depth + 1);
            }
        }
    }

    // Block
    else if (const auto* node =
                 dynamic_cast<const BlockStmt*>(stmt))
    {
        std::cout << indent << "Block:\n";

        for (const auto& child : node->statements)
        {
            dumpStmt(child.get(), depth + 1);
        }
    }

    // If
    else if (const auto* node =
                 dynamic_cast<const IfStmt*>(stmt))
    {
        std::cout << indent << "If:\n";

        std::cout << indent << "Condition:\n";
        dumpExpr(node->condition.get(), depth + 1);

        std::cout << indent << "Then:\n";
        dumpStmt(node->thenBranch.get(), depth + 1);

        if (node->elseBranch)
        {
            std::cout << indent << "Else:\n";
            dumpStmt(node->elseBranch.get(), depth + 1);
        }
    }

    // While
    else if (const auto* node =
                 dynamic_cast<const WhileStmt*>(stmt))
    {
        std::cout << indent << "While:\n";

        std::cout << indent << "Condition:\n";
        dumpExpr(node->condition.get(), depth + 1);

        std::cout << indent << "Body:\n";
        dumpStmt(node->body.get(), depth + 1);
    }

    // For
    else if (const auto* node =
                 dynamic_cast<const ForStmt*>(stmt))
    {
        std::cout
            << indent
            << "For(var="
            << node->varName.m_lexeme
            << ")\n";

        std::cout << indent << "From:\n";
        dumpExpr(node->from.get(), depth + 1);

        std::cout << indent << "To:\n";
        dumpExpr(node->to.get(), depth + 1);

        if (node->step)
        {
            std::cout << indent << "Step:\n";
            dumpExpr(node->step.get(), depth + 1);
        }

        std::cout << indent << "Body:\n";
        dumpStmt(node->body.get(), depth + 1);
    }

    // Function
    else if (const auto* node =
                 dynamic_cast<const FuncStmt*>(stmt))
    {
        std::cout
            << indent
            << "Func(name="
            << node->name.m_lexeme
            << ")\n";

        std::cout << indent << "Params:\n";

        for (const auto& param : node->params)
        {
            std::cout
                << indent
                << "  "
                << param.type.m_lexeme
                << " "
                << param.name.m_lexeme;

            if (param.defaultValue)
            {
                std::cout << " = ";
                dumpExpr(param.defaultValue.get(), 0);
            }
            else
            {
                std::cout << '\n';
            }
        }

        if (node->returnType.m_type != TokenType::Null)
        {
            std::cout
                << indent
                << "ReturnType: "
                << node->returnType.m_lexeme
                << '\n';
        }

        std::cout << indent << "Body:\n";
        dumpStmt(node->body.get(), depth + 1);
    }

    // Return
    else if (const auto* node =
                 dynamic_cast<const ReturnStmt*>(stmt))
    {
        std::cout << indent << "Return:\n";

        if (node->value)
            dumpExpr(node->value.get(), depth + 1);
    }

    // Break
    else if (dynamic_cast<const BreakStmt*>(stmt))
    {
        std::cout << indent << "Break\n";
    }

    // Continue
    else if (dynamic_cast<const ContinueStmt*>(stmt))
    {
        std::cout << indent << "Continue\n";
    }

    // Exit
    else if (const auto* node =
                 dynamic_cast<const ExitStmt*>(stmt))
    {
        std::cout << indent << "Exit:\n";

        if (node->code)
            dumpExpr(node->code.get(), depth + 1);
    }

    // Expression statement
    else if (const auto* node =
                 dynamic_cast<const ExprStmt*>(stmt))
    {
        std::cout << indent << "ExprStmt:\n";
        dumpExpr(node->expr.get(), depth + 1);
    }

    // Out
    else if (const auto* node =
                 dynamic_cast<const OutStmt*>(stmt))
    {
        std::cout << indent << "Out:\n";

        for (const auto& value : node->values)
        {
            dumpExpr(value.get(), depth + 1);
        }
    }

    // In
    else if (const auto* node =
                 dynamic_cast<const InStmt*>(stmt))
    {
        std::cout
            << indent
            << "In(target="
            << node->target.m_lexeme
            << ")\n";
    }

    else
    {
        std::cout
            << indent
            << "Unknown stmt node\n";
    }
}


void CodeRunner::dumpTokens(const std::string& path)
{
    std::string source = filemanager.read(path);

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::cout
        << std::left
        << std::setw(7)  << "Line"
        << std::setw(24) << "Type"
        << "Lexeme\n";

    std::cout
        << "------------------------------------------------------------\n";

    for (const auto& token : tokens)
    {
        std::string lexeme = token.m_lexeme;

        // Make whitespace visible.
        if (lexeme == "\n")
            lexeme = "\\n";
        else if (lexeme == "\t")
            lexeme = "\\t";
        else if (lexeme == "\r")
            lexeme = "\\r";

        std::cout
            << std::left
            << std::setw(7)
            << token.m_line
            << std::setw(24)
            << tokenTypeToString(token.m_type)
            << lexeme
            << '\n';
    }

    std::cout
        << "\n=================================\n";
}


void CodeRunner::dumpAST(const std::string& path)
{
    std::string source = filemanager.read(path);

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);

    try
    {
        auto statements = parser.parse();

        std::cout
            << "\n========== AST ==========\n\n";

        for (const auto& statement : statements)
        {
            dumpStmt(statement.get());
        }

        std::cout
            << "\n==============================\n";
    }
    catch (const ParserError& error)
    {
        std::cerr
            << "AST dump failed: "
            << error.what()
            << '\n';

        throw;
    }
}

#include "Interpreter.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "../utils/Error.hpp"
#include "../io/Keyboard.hpp"

Interpreter::Interpreter(ErrorReporter* reporter)
    : reporter(reporter)
{
    globals = std::make_shared<Env>(reporter);
    environment = globals;
    registerNatives();
}



void Interpreter::interpret(const std::vector<std::unique_ptr<Stmt>>& statements)
{
    for (const auto& stmt : statements) {
        try {
            execute(stmt.get());
        } catch (const std::runtime_error& error) {
            reporter->report({ErrorKind::Runtime, error.what(), stmt->m_line,
                              "Fix the invalid value or operation."});
            return;
        } catch (FatalError&) {
            // Error already queued via reporter->report() at the throw site.
            return;
        }
    }
}

void Interpreter::execute(Stmt* statement)
{   
    // out
    if (auto* outstmt = dynamic_cast<OutStmt*>(statement)) {
        for (auto& valueExpr : outstmt->values) {
            if (auto* lit = dynamic_cast<LiteralExpr*>(valueExpr.get())) {
                if (lit->valueToken.m_type == TokenType::End) {
                    std::cout<<"\n";
                    continue;
                }
            }
            Value val = evaluate(valueExpr.get());
            std::cout<< val.stringify();
        }
        return;
    }
    // in 
    if (auto* instmt = dynamic_cast<InStmt*>(statement)) {
        std::string input;
        std::getline(std::cin, input);

        Value val;

        try {
             if (input.find('.') != std::string::npos) {
            val = Value(std::stof(input));
        } else {
            val = Value(std::stoi(input));
        }
        } catch (std::exception&) {
             val = Value(input);
        }

        environment->assign(instmt->target, val);
        return;
    }

    // vaiable declearration 

    if (auto* varDecl = dynamic_cast<VarDecStmt*>(statement)) 
    {
    Value value; // defaults to Null if uninitialized
    if (varDecl->initializer) {
        value = evaluate(varDecl->initializer.get());
        std::string declType = varDecl->type.m_lexeme;
        if (declType == "float" && value.isInt()) value = Value(static_cast<float>(value.asInt()));
        else if (declType == "int" && value.isFloat()) value = Value(static_cast<int>(value.asFloat()));
    } else {
        // Set type-appropriate defaults for uninitialized typed vars
        std::string t = varDecl->type.m_lexeme;
        if (t == "int") value = Value(0);
        else if (t == "float") value = Value(0.0f);
        else if (t == "string") value = Value(std::string(""));
        else if (t == "bool") value = Value(false);
    }
    bool isDynamic = (varDecl->type.m_lexeme == "var");
    std::string declaredType = isDynamic ? "" : varDecl->type.m_lexeme;

    if (!isDynamic && varDecl->initializer) {
        std::string actual = value.getTypeAsString();
        bool matches =
            (declaredType == "int" && value.isInt()) ||
            (declaredType == "float" && value.isFloat()) ||
            (declaredType == "string" && value.isString()) ||
            (declaredType == "bool" && value.isBool());
        if (!matches) {
            reporter->report({ErrorKind::Type,
                              "Cannot initialize statically-typed variable '" + varDecl->name.m_lexeme +
                                  "' (type '" + declaredType + "') with value of type '" + actual + "'",
                              varDecl->name.m_line,
                              "Provide an initial value of type '" + declaredType + "'.", varDecl->name.m_column});
            throw FatalError();
        }
    }

    environment->define(varDecl->name.m_lexeme, value,declaredType, isDynamic, varDecl->isConst);
    return;
    }

    // list declaration
    if (auto* listDecl = dynamic_cast<ListDecStmt*>(statement)) {
        std::vector<Value> elems;

        if (listDecl->initExpr) {
            Value val = evaluate(listDecl->initExpr.get());
            if (val.isList()) {
                elems = *val.asList(); // Copies the contents of the right-hand list
            } else {
                throw std::runtime_error("Expected a list value after '=' at line " + std::to_string(statement->m_line));
            }
        } else {
            for (auto& elemExpr : listDecl->elements) {
                elems.push_back(evaluate(elemExpr.get()));
            }
        }

        if (listDecl->elementType.m_type != TokenType::Null) {
            std::string expected = listDecl->elementType.m_lexeme;
            for (size_t i = 0; i < elems.size(); i++) {
                std::string actual = elems[i].getTypeAsString();
                bool matches =
                    (expected == "int" && elems[i].isInt()) ||
                    (expected == "float" && elems[i].isFloat()) ||
                    (expected == "string" && elems[i].isString()) ||
                    (expected == "bool" && elems[i].isBool());
                if (!matches) {
                    throw std::runtime_error("List element at index " + std::to_string(i) +
                        " has type '" + actual + "' but list expects '" + expected +
                        "' at line " + std::to_string(statement->m_line));
                }
            }
        }

        if (listDecl->size) {
            int fixedSize = evaluate(listDecl->size.get()).asInt();
            elems.resize(fixedSize);
        }

        Value listVal = Value::makeList(std::move(elems));
        std::string declaredType = listDecl->elementType.m_lexeme == "var" ? "" : listDecl->elementType.m_lexeme;
        environment->define(listDecl->name.m_lexeme, listVal, declaredType, true, listDecl->isConst);
        return;
    }



    // exprssion 
    if (auto* exprStmt = dynamic_cast<ExprStmt*>(statement)) {
    evaluate(exprStmt->expr.get());
    return;
    }

    // block
    if (auto* blockstmt = dynamic_cast<BlockStmt*>(statement)) {
        auto prev = environment;
        environment = std::make_shared<Env>(prev);
            try {
        for (auto& stmt : blockstmt->statements) {
            execute(stmt.get());
        }
        } catch (std::runtime_error& error) {
            environment = prev; 
            throw error;
        } catch (BreakException&) {
            environment = prev;
            throw;
        } catch (ContinueException&) {
            environment = prev;
            throw;
        } catch (ReturnException&) {
            environment = prev;
            throw;
        }
        environment = prev;
        return;
    }

    //if
    if (auto* ifstmt = dynamic_cast<IfStmt*>(statement)) {
        Value cond  = evaluate(ifstmt->condition.get());
        if (cond.isTruthy()) {
            execute(ifstmt->thenBranch.get());
        } else if (ifstmt->elseBranch) {
            execute(ifstmt->elseBranch.get());
        }
        return;
    }
    //  while
    if (auto* whilestmt = dynamic_cast<WhileStmt*>(statement)) {
       while (evaluate(whilestmt->condition.get()).isTruthy()) 
       {
        try {
            loopDepth++;
            execute(whilestmt->body.get());
            loopDepth--;
        } catch (BreakException&) {
            loopDepth--;
            break;
        } catch (ContinueException&) {
            loopDepth--;
            // just continue to next iteration
        } catch (...) {
            loopDepth--;
            throw;
        }
       }
       return;
    }
    // for
    if (auto* forstmt = dynamic_cast<ForStmt*>(statement)) {
        Value from = evaluate(forstmt->from.get());
        Value to = evaluate(forstmt->to.get());
        Value step = forstmt->step ?  evaluate(forstmt->step.get()) : Value(1);

        int from_value = from.asInt();
        int to_value = to.asInt();
        int step_value = step.asInt();

        if (step_value ==  0) {
            throw std::runtime_error("For loop step cannot be zero, at line " + std::to_string(statement->m_line));
        }
        auto prev  = environment;
        try {
            bool broken = false;
            if (step_value > 0) {
                for (int i = from_value; i <= to_value && !broken; i += step_value) {
                    environment = std::make_shared<Env>(prev);
                    std::string declaredType = forstmt->varName.m_lexeme == "var" ? "" : forstmt->varName.m_lexeme;
                    environment->define(forstmt->varName.m_lexeme, Value(i),declaredType, forstmt->varName.m_lexeme == "var");
                    try {
                        loopDepth++;
                        execute(forstmt->body.get());
                        loopDepth--;
                    } catch (BreakException&) {
                        loopDepth--;
                        broken = true;
                    } catch (ContinueException&) {
                        loopDepth--;
                        // just continue to next iteration
                    } catch (...) {
                        loopDepth--;
                        throw;
                    }
                }
            } else {
                for (int i = from_value; i >= to_value && !broken; i += step_value) {
                    environment = std::make_shared<Env>(prev);
                    std::string declaredType = forstmt->varName.m_lexeme == "var" ? "" : forstmt->varName.m_lexeme;
                    environment->define(forstmt->varName.m_lexeme, Value(i),declaredType, forstmt->varName.m_lexeme == "var");
                    try {
                        loopDepth++;
                        execute(forstmt->body.get());
                        loopDepth--;
                    } catch (BreakException&) {
                        loopDepth--;
                        broken = true;
                    } catch (ContinueException&) {
                        loopDepth--;
                        // just continue to next iteration
                    } catch (...) {
                        loopDepth--;
                        throw;
                    }
                }
            }
            environment = prev;
            return;
            } catch (...) {
                environment = prev;
                throw;
            }
            }
    //return
    if (auto* returnstmt  = dynamic_cast<ReturnStmt*>(statement)) {
        if (functionDepth == 0) {
            reporter->report({ErrorKind::Runtime, "'return' can only be used inside a function",
                              statement->m_line, "Move this statement into a function body."});
            return;
        }
        Value val;
        if (returnstmt->value) {
            val = evaluate(returnstmt->value.get());
        }
        throw ReturnException(val);
    }
    // function
    if (auto* funcstmt  = dynamic_cast<FuncStmt*>(statement)) {
        functions[funcstmt->name.m_lexeme] = funcstmt;
        return;
    }
    //  break 
    if (dynamic_cast<BreakStmt*>(statement)) {
        if (loopDepth <= functionLoopBase) {
            reporter->report({ErrorKind::Runtime, "'break' can only be used inside a loop",
                              statement->m_line, "Move this statement into a while or for loop."});
            return;
        }
        throw BreakException();
    }
    //  continue 
    if (dynamic_cast<ContinueStmt*>(statement)) {
        if (loopDepth <= functionLoopBase) {
            reporter->report({ErrorKind::Runtime, "'continue' can only be used inside a loop",
                              statement->m_line, "Move this statement into a while or for loop."});
            return;
        }
        throw ContinueException();
    }
    // exit
    if (auto* exitstmt = dynamic_cast<ExitStmt*>(statement)) {
        int code = 0;
        if (exitstmt->code) {
            code = evaluate(exitstmt->code.get()).asInt();
        }
        std::exit(code);
    }

    throw std::runtime_error("Unknown statement type at line " + std::to_string(statement->m_line));
}
// array methods 
Value Interpreter::executeListMethod(ListType list, const std::string& method,
                                     std::vector<std::unique_ptr<Expr>>& args, size_t line)
{
    // put
    if (method == "put") {
        if (args.size() != 1) throw std::runtime_error("put() takes exactly 1 argument at line " + std::to_string(line));
        list->push_back(evaluate(args[0].get()));
        return Value(list);
    }
    // place
    if (method == "place") {
        if (args.size() != 2) throw std::runtime_error("place() takes exactly 2 arguments at line " + std::to_string(line));
        int idx = evaluate(args[0].get()).asInt();
        Value val = evaluate(args[1].get());
        if (idx < 0 || idx > static_cast<int>(list->size())) {
            throw std::runtime_error("Index out of bounds in place() at line " + std::to_string(line));
        }
        list->insert(list->begin() + idx, val);
        return Value(list);
    }
    // pull
    if (method == "pull") {
        if (list->empty()) throw std::runtime_error("Cannot pull from empty list at line " + std::to_string(line));
        int idx = static_cast<int>(list->size()) - 1; // default: last
        if (args.size() == 1) {
            idx = evaluate(args[0].get()).asInt();
        }
        if (idx < 0 || idx >= static_cast<int>(list->size())) {
            throw std::runtime_error("Index out of bounds in pull() at line " + std::to_string(line));
        }
        Value val = (*list)[idx];
        list->erase(list->begin() + idx);
        return val;
    }
    // stip
    if (method == "strip") {
        if (args.size() != 1) throw std::runtime_error("strip() takes exactly 1 argument at line " + std::to_string(line));
        Value target = evaluate(args[0].get());
        for (auto it = list->begin(); it != list->end(); ++it) {
            if (*it == target) {
                list->erase(it);
                return Value(list);
            }
        }
        throw std::runtime_error("Value not found in list for strip() at line " + std::to_string(line));
    }
    //shuffle
    if (method == "shuffle") {
        if (!args.empty()) throw std::runtime_error("shuffle() takes no arguments at line " + std::to_string(line));
        static std::random_device rd;
        static std::mt19937 g(rd());
        std::shuffle(list->begin(), list->end(), g);
        return Value(list);
    }
    // flush
    if (method == "flush") {
        if (!args.empty()) throw std::runtime_error("flush() takes no arguments at line " + std::to_string(line));
        list->clear();
        return Value(list);
    }
    // clone
    if (method == "clone") {
        if (!args.empty()) throw std::runtime_error("clone() takes no arguments at line " + std::to_string(line));
        return Value(list);
    }
    // freequncy
    if (method == "freq") {
        if (args.size() != 1) throw std::runtime_error("freq() takes exactly 1 argument at line " + std::to_string(line));
        Value target = evaluate(args[0].get());
        int count = 0;
        for (auto& elem : *list) {
            if (elem == target) count++;
        }
        return Value(count);
    }
    // find
    if (method == "find") {
        if (args.size() != 1) throw std::runtime_error("find() takes exactly 1 argument at line " + std::to_string(line));
        Value target = evaluate(args[0].get());
        for (size_t i = 0; i < list->size(); i++) {
            if ((*list)[i] == target) return Value(static_cast<int>(i));
        }
        return Value(-1);
    }
    // flip
    if (method == "flip") {
        if (!args.empty()) throw std::runtime_error("flip() takes no arguments at line " + std::to_string(line));
        std::reverse(list->begin(), list->end());
        return Value::makeList(*list);
    }
    // sort
    if (method == "sort") {
        if (!args.empty()) throw std::runtime_error("sort() takes no arguments at line " + std::to_string(line));
        std::sort(list->begin(), list->end(), [line](const Value& a, const Value& b) {
            return a < b;
        });
        return Value(list);
    }
    // size
    if (method == "size") {
        if (!args.empty()) throw std::runtime_error("size() takes no arguments at line " + std::to_string(line));
        return Value(static_cast<int>(list->size()));
    }

    throw std::runtime_error("Unknown list method '" + method + "' at line " + std::to_string(line));
}

Value Interpreter::evaluate(Expr* expr)
{

    // Literal --- 

    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        const auto& tok = lit->valueToken;
        switch (tok.m_type) 
        {
            case TokenType::Number:
                if (tok.m_lexeme.find('.') != std::string::npos) {
                return Value(std::stof(tok.m_lexeme));
                }
             return Value(std::stoi(tok.m_lexeme));   
            case TokenType::String:
                return Value(tok.m_lexeme);
            case TokenType::End:
                return Value();
            case TokenType::True:
                return Value(true);
            case TokenType::False:
                return Value(false);
            case TokenType::Null:
                return Value();
            default:
                throw std::runtime_error("Unknown literal type at line " + std::to_string(tok.m_line));
        }
    }

    // Variable--- 

    if (auto* var = dynamic_cast<VariableExpr*>(expr)) 
    {
    return environment->get(var->name);
    }

    if (auto* assign = dynamic_cast<AssignExpr*>(expr)) {
        Value value = evaluate(assign->value.get());
        environment->assign(assign->name, value);
        return value; // assignment is an expression — it evaluates to the assigned value
    }

    // array
    if (auto* indexExpr = dynamic_cast<IndexExpr*>(expr)) {
        Value target = evaluate(indexExpr->target.get());
        Value index = evaluate(indexExpr->index.get());
        if (target.isList()) {
            auto list = target.asList();
            int idx = index.asInt();
            if (idx < 0 || idx >= static_cast<int>(list->size())) {
                throw std::runtime_error("Index " + std::to_string(idx) + " out of bounds (list size: " +
                    std::to_string(list->size()) + ") at line " + std::to_string(expr->m_line));
            }
            return (*list)[idx];
        }
        if (target.isString()) {
            int idx = index.asInt();
            std::string str = target.asString();
            if (idx < 0 || idx >= static_cast<int>(str.size())) {
                throw std::runtime_error("Index out of bounds for string at line " + std::to_string(expr->m_line));
            }
            return Value(std::string(1, str[idx]));
        }
        throw std::runtime_error("Cannot index into non-list/non-string value at line " + std::to_string(expr->m_line));
    }

    //array assign 
    if (auto* idxAssign = dynamic_cast<IndexAssignExpr*>(expr)) {
        Value target = evaluate(idxAssign->target.get());
        Value index = evaluate(idxAssign->index.get());
        Value value = evaluate(idxAssign->value.get());
        if (!target.isList()) {
            throw std::runtime_error("Cannot index-assign into non-list value at line " + std::to_string(expr->m_line));
        }
        auto list = target.asList();
        int idx = index.asInt();
        if (idx < 0 || idx >= static_cast<int>(list->size())) {
            throw std::runtime_error("Index out of bounds for assignment at line " + std::to_string(expr->m_line));
        }
        (*list)[idx] = value;
        return value;
    }

    // membercall
    if (auto* memberCall = dynamic_cast<MemberCallExpr*>(expr)) {
        Value target = evaluate(memberCall->target.get());
        if (target.isList()) {
            return executeListMethod(target.asList(), memberCall->methodName.m_lexeme,
                                     memberCall->arguments, expr->m_line);
        }
        if (target.isString()) {
            return executeStringMethod(target.asString(), memberCall->methodName.m_lexeme,
                                       memberCall->arguments, expr->m_line);
        }
        throw std::runtime_error("Method calls are only supported on lists and strings at line " + std::to_string(expr->m_line));
    }

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
    Value left = evaluate(bin->left.get());
    Value right = evaluate(bin->right.get());

        switch (bin->operatorToken.m_type) {
            case TokenType::Plus:  return left + right;
            case TokenType::Minus: return left - right;
            case TokenType::Star:  return left * right;
            case TokenType::Slash: return left / right;
            case TokenType::Modulo: return left % right;
            case TokenType::EqualEqual: return Value(left == right);
            case TokenType::NotEqual:   return Value(left != right);
            case TokenType::Less: return Value(left < right);
            case TokenType::Greater: return Value(left > right);
            case TokenType::EqualEqualEqual: return Value(left.strictEquals(right));
            case TokenType::GreaterEqual: return Value(left >= right);
            case TokenType::LessEqual: return Value(left <= right);
            case TokenType::And: return left && right;
            case TokenType::Or:  return left || right;
            default:
                throw std::runtime_error("Unknown binary operator at line " + std::to_string(bin->m_line));
        }
    }
// function call--- 

    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        // check if usr defined
        auto it = functions.find(call->callee.m_lexeme);
        if (it != functions.end()) {
            FuncStmt* func = it->second;

            size_t requiredParams = 0;
            for (auto& p : func->params) {
                if (!p.defaultValue) requiredParams++;
                else break;
            }

            if (call->arguments.size() < requiredParams || call->arguments.size() > func->params.size()) {
                throw std::runtime_error("Function '" + func->name.m_lexeme + "' expects " +
                    std::to_string(requiredParams) + "-" + std::to_string(func->params.size()) +
                    " arguments but got " + std::to_string(call->arguments.size()) +
                    " at line " + std::to_string(call->m_line));
            }

            std::vector<Value> argValues;
            for (auto& arg : call->arguments) {
                argValues.push_back(evaluate(arg.get()));
            }

            // filling with default values
            for (size_t i = argValues.size(); i < func->params.size(); i++) {
                if (func->params[i].defaultValue) {
                    argValues.push_back(evaluate(func->params[i].defaultValue.get()));
                }
            }

            // coercion
            auto callEnv = std::make_shared<Env>(globals);
            for (size_t i = 0; i < func->params.size(); i++) {
                Value val = argValues[i];
                std::string ptype = func->params[i].type.m_lexeme;
                std::string declaredType = (ptype == "var") ? "" : ptype;

                if (ptype == "int" && val.isFloat()) val = Value(static_cast<int>(val.asFloat()));
                else if (ptype == "int" && val.isBool()) val = Value(val.asBool() ? 1 : 0);
                else if (ptype == "float" && val.isInt()) val = Value(static_cast<float>(val.asInt()));
                else if (ptype == "string" && !val.isString()) val = Value(val.stringify());
                else if (ptype == "bool" && !val.isBool()) val = Value(val.isTruthy());

                callEnv->define(func->params[i].name.m_lexeme, val , declaredType, ptype == "var", false);
            }

            auto prev = environment;
            const size_t previousLoopBase = functionLoopBase;
            functionDepth++;
            functionLoopBase = loopDepth;
            environment = callEnv;

            Value result;
            try {
                execute(func->body.get());
            } catch (ReturnException& ret) {
                result = ret.value;
            } catch (...) {
                environment = prev;
                functionLoopBase = previousLoopBase;
                functionDepth--;
                throw;
            }

            environment = prev;
            functionLoopBase = previousLoopBase;
            functionDepth--;

            if (func->returnType.m_type != TokenType::Null) {
                std::string expected = func->returnType.m_lexeme;

                if (expected == "int" && result.isFloat()) result = Value(static_cast<int>(result.asFloat()));
                else if (expected == "int" && result.isBool()) result = Value(result.asBool() ? 1 : 0);
                else if (expected == "float" && result.isInt()) result = Value(static_cast<float>(result.asInt()));
                else if (expected == "string" && !result.isString()) result = Value(result.stringify());
                else if (expected == "bool" && !result.isBool()) result = Value(result.isTruthy());

                bool matches =
                    (expected == "int" && result.isInt()) ||
                    (expected == "float" && result.isFloat()) ||
                    (expected == "string" && result.isString()) ||
                    (expected == "bool" && result.isBool());

                if (!matches) {
                    throw std::runtime_error("Function '" + func->name.m_lexeme + "' declared return type '" + expected +
                        "' but returned '" + result.getTypeAsString() + "' at line " + std::to_string(call->m_line));
                }
            }

            return result;
        }
        // native function check
        auto nativeIt = nativeFunctions.find(call->callee.m_lexeme);
        if (nativeIt != nativeFunctions.end()) {
            std::vector<Value> argValues;
            for (auto& arg : call->arguments) {
                argValues.push_back(evaluate(arg.get()));
            }
            return nativeIt->second(argValues, call->m_line);
        }

        throw std::runtime_error("Undefined function '" + call->callee.m_lexeme + "' at line " + std::to_string(call->m_line));
    }

    // Unary ---
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        Value operand = evaluate(unary->operand.get());
        switch (unary->operatorToken.m_type) {
            case TokenType::Minus:
                if (operand.isInt()) return Value(-operand.asInt());
                if (operand.isFloat()) return Value(-operand.asFloat());
                throw std::runtime_error("Cannot negate non-numeric value at line " + std::to_string(expr->m_line));
            case TokenType::Not:
                return Value(!operand.isTruthy());
            default:
                throw std::runtime_error("Unknown unary operator at line " + std::to_string(expr->m_line));
        }
    }

    // increment / decrement
    if (auto* incExpr = dynamic_cast<IncrementExpr*>(expr)) {
        Value current = environment->get(incExpr->name);
        Value result;
        if (current.isInt()) {
            result = Value(current.asInt() + (incExpr->isIncrement ? 1 : -1));
        } else if (current.isFloat()) {
            result = Value(current.asFloat() + (incExpr->isIncrement ? 1.0f : -1.0f));
        } else {
            throw std::runtime_error("'++' / '--' can only be used on numeric values at line " + std::to_string(expr->m_line));
        }
        environment->assign(incExpr->name, result);
        return result;
    }

    // ternarry 
    if (auto* ternary = dynamic_cast<TernaryExpr*>(expr)) {
        Value cond = evaluate(ternary->condition.get());
        if (cond.isTruthy()) {
            return evaluate(ternary->trueExpr.get());
        }
        return evaluate(ternary->falseExpr.get());
    }

    throw std::runtime_error("Unknown expression type at line " + std::to_string(expr->m_line));
}

// Native Functions --- 

void Interpreter::registerNatives()
{
    nativeFunctions["keyPressed"] = [](std::vector<Value> args, size_t line) -> Value {
        if (!args.empty()) {
            throw std::runtime_error("keyPressed() takes no arguments at line " + std::to_string(line));
        }
        return Value(Keyboard::keyPressed());
    };

    nativeFunctions["getKey"] = [](std::vector<Value> args, size_t line) -> Value {
        if (!args.empty()) {
            throw std::runtime_error("getKey() takes no arguments at line " + std::to_string(line));
        }
        return Value(Keyboard::getKey());
    };
    // print
    nativeFunctions["print"] = [](std::vector<Value> args, size_t line) -> Value
    {
        if(args.size() == 0) throw std::runtime_error("print() needs atleast one argument at line " + std::to_string(line));
        unsigned int current_arg = 0;
        while (current_arg < args.size()) {
            std::cout<< args[current_arg].stringify();
            current_arg++;
        }
        return Value{};
    };
    //println
    nativeFunctions["println"] = [](std::vector<Value> args, size_t line) -> Value
    {
        if(args.size() == 0) throw std::runtime_error("print() needs atleast one argument at line " + std::to_string(line));
        unsigned int current_arg = 0;
        while (current_arg < args.size()) {
            std::cout<< args[current_arg].stringify();
            current_arg++;
        }
        std::cout<<"\n";
        return Value{};
    };
    // absolutee
    nativeFunctions["abs"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("abs() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isInt()) return Value(std::abs(args[0].asInt()));
        if (args[0].isFloat()) return Value(std::abs(args[0].asFloat()));
        throw std::runtime_error("abs() requires a numeric argument at line " + std::to_string(line));
    };

    // least
    nativeFunctions["least"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 2) throw std::runtime_error("least() takes exactly 2 arguments at line " + std::to_string(line));
        if (args[0].isInt() && args[1].isInt()) return Value(std::min(args[0].asInt(), args[1].asInt()));
        return Value(std::min(args[0].asFloatPromoted(), args[1].asFloatPromoted()));
    };

    // most
    nativeFunctions["most"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 2) throw std::runtime_error("most() takes exactly 2 arguments at line " + std::to_string(line));
        if (args[0].isInt() && args[1].isInt()) return Value(std::max(args[0].asInt(), args[1].asInt()));
        return Value(std::max(args[0].asFloatPromoted(), args[1].asFloatPromoted()));
    };

    // exp
    nativeFunctions["exp"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 2) throw std::runtime_error("exp() takes exactly 2 arguments at line " + std::to_string(line));
        if (args[0].isInt() && args[1].isInt()) {
            return Value(static_cast<int>(std::pow(args[0].asInt(), args[1].asInt())));
        }
        return Value(static_cast<float>(std::pow(args[0].asFloatPromoted(), args[1].asFloatPromoted())));
    };

    // root
    nativeFunctions["sqrt"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("sqrt() takes exactly 1 argument at line " + std::to_string(line));
        return Value(static_cast<float>(std::sqrt(args[0].asFloatPromoted())));
    };

    // random
    nativeFunctions["random"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 2) throw std::runtime_error("random() takes exactly 2 arguments at line " + std::to_string(line));
        int lo = args[0].asInt();
        int hi = args[1].asInt();
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(lo, hi);
        return Value(dist(rng));
    };

    // typeof
    nativeFunctions["type_of"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("type_of() takes exactly 1 argument at line " + std::to_string(line));
        return Value(args[0].getTypeAsString());
    };

    //len
    nativeFunctions["len"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("len() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isString()) return Value(static_cast<int>(args[0].asString().size()));
        if (args[0].isList()) return Value(static_cast<int>(args[0].asList()->size()));
        throw std::runtime_error("len() requires a string or list argument at line " + std::to_string(line));
    };

    //round
    nativeFunctions["round"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.empty() || args.size() > 2) throw std::runtime_error("round() takes 1 or 2 arguments at line " + std::to_string(line));
        if (args[0].isInt() && args.size() == 1) return args[0];
        float val = args[0].asFloatPromoted();
        if (args.size() == 2) {
            int places = args[1].asInt();
            float factor = static_cast<float>(std::pow(10, places));
            return Value(std::round(val * factor) / factor);
        }
        return Value(static_cast<int>(std::round(val)));
    };

    //floor
    nativeFunctions["floor"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("floor() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isInt()) return args[0];
        if (args[0].isFloat()) return Value(static_cast<int>(std::floor(args[0].asFloat())));
        throw std::runtime_error("floor() requires a numeric argument at line " + std::to_string(line));
    };

    //ceil
    nativeFunctions["ceil"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("ceil() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isInt()) return args[0];
        if (args[0].isFloat()) return Value(static_cast<int>(std::ceil(args[0].asFloat())));
        throw std::runtime_error("ceil() requires a numeric argument at line " + std::to_string(line));
    };

    nativeFunctions["sleep"] = [](std::vector<Value>args, size_t line) -> Value
    {
        if (args.size() != 1) throw std::runtime_error("sleep() only takes one argument at line " + std::to_string(line));
        if (!args[0].isInt()) throw std::runtime_error("sleep() only accepts intezer value at line " + std::to_string(line));
        std::this_thread::sleep_for(std::chrono::milliseconds(args[0].asInt()));
        return Value();
    };

    // clock
    nativeFunctions["clock"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() > 1) throw std::runtime_error("clock() takes 0 or 1 arguments at line " + std::to_string(line));

        if (args.empty()) {
            // Return milliseconds since epoch
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            return Value(static_cast<int>(ms % 2147483647)); // fit in int
        }

        std::string flag = args[0].asString();
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&t);
        char buf[64];

        if (flag == "time") {
            std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
        } else if (flag == "date") {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
        } else if (flag == "full") {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
        } else {
            throw std::runtime_error("clock() flag must be 'time', 'date', or 'full' at line " + std::to_string(line));
        }
        return Value(std::string(buf));
    };

    // Type converrsions ---

    // int
    nativeFunctions["int"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("int() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isInt()) return args[0];
        if (args[0].isFloat()) return Value(static_cast<int>(args[0].asFloat()));
        if (args[0].isBool()) return Value(args[0].asBool() ? 1 : 0);
        if (args[0].isString()) {
            try { return Value(std::stoi(args[0].asString())); }
            catch (...) { throw std::runtime_error("Cannot convert '" + args[0].asString() + "' to int at line " + std::to_string(line)); }
        }
        throw std::runtime_error("Cannot convert " + args[0].getTypeAsString() + " to int at line " + std::to_string(line));
    };

    // float
    nativeFunctions["float"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("float() takes exactly 1 argument at line " + std::to_string(line));
        if (args[0].isFloat()) return args[0];
        if (args[0].isInt()) return Value(static_cast<float>(args[0].asInt()));
        if (args[0].isString()) {
            try { return Value(std::stof(args[0].asString())); }
            catch (...) { throw std::runtime_error("Cannot convert '" + args[0].asString() + "' to float at line " + std::to_string(line)); }
        }
        throw std::runtime_error("Cannot convert " + args[0].getTypeAsString() + " to float at line " + std::to_string(line));
    };

    // string
    nativeFunctions["string"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("string() takes exactly 1 argument at line " + std::to_string(line));
        return Value(args[0].stringify());
    };
    // bool
    nativeFunctions["bool"] = [](std::vector<Value> args, size_t line) -> Value {
        if (args.size() != 1) throw std::runtime_error("bool() takes exactly 1 argument at line " + std::to_string(line));
        return Value(args[0].isTruthy());
    };
}

// String Methods ---

Value Interpreter::executeStringMethod(const std::string& str, const std::string& method,
                                       std::vector<std::unique_ptr<Expr>>& args, size_t line)
{
    // caps
    if (method == "caps") {
        if (!args.empty()) throw std::runtime_error("caps() takes no arguments at line " + std::to_string(line));
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return Value(result);
    }
    // small
    if (method == "small") {
        if (!args.empty()) throw std::runtime_error("small() takes no arguments at line " + std::to_string(line));
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return Value(result);
    }
    //empty
    if (method == "empty") {
        if (!args.empty()) throw std::runtime_error("empty() takes no arguments at line " + std::to_string(line));
        return Value(str.empty());
    }
    if (method == "ends") {
        if (args.size() != 1) throw std::runtime_error("ends() takes one argumen at line " + std::to_string(line));
        std::string substr = evaluate(args[0].get()).asString();
        return Value(str.ends_with(substr));
    }
    //slice
    if (method == "slice") {
        if (args.size() != 1) throw std::runtime_error("slice() takes exactly 1 argument at line " + std::to_string(line));
        std::string delim = evaluate(args[0].get()).asString();
        std::vector<Value> parts;
        size_t start = 0;
        size_t pos;
        while ((pos = str.find(delim, start)) != std::string::npos) {
            parts.push_back(Value(str.substr(start, pos - start)));
            start = pos + delim.size();
        }
        parts.push_back(Value(str.substr(start)));
        return Value::makeList(std::move(parts));
    }
    //strip
    if (method == "strip") {
        if (!args.empty()) throw std::runtime_error("strip() takes no arguments at line " + std::to_string(line));
        std::string result = str;
        size_t start = result.find_first_not_of(" \t\n\r");
        size_t end = result.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return Value(std::string(""));
        return Value(result.substr(start, end - start + 1));
    }
    // has
    if (method == "has") {
        if (args.size() != 1) throw std::runtime_error("has() takes exactly 1 argument at line " + std::to_string(line));
        std::string substr = evaluate(args[0].get()).asString();
        return Value(str.find(substr) != std::string::npos);
    }
    // starts with
    if (method == "begins") {
        if (args.size() != 1) throw std::runtime_error("starts_with() only takes one argument at line " + std::to_string(line));
        std::string substr  = evaluate(args[0].get()).asString();
        return Value(str.starts_with(substr));
    }
    
    // swap
    if (method == "swap") {
        if (args.size() != 2) throw std::runtime_error("swap() takes exactly 2 arguments at line " + std::to_string(line));
        std::string oldStr = evaluate(args[0].get()).asString();
        std::string newStr = evaluate(args[1].get()).asString();
        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(oldStr, pos)) != std::string::npos) {
            result.replace(pos, oldStr.size(), newStr);
            pos += newStr.size();
        }
        return Value(result);
    }
    // size
    if (method == "size") {
        if (!args.empty()) throw std::runtime_error("size() takes no arguments at line " + std::to_string(line));
        return Value(static_cast<int>(str.size()));
    }

    if (method == "at") {
        if (args.size() != 1) throw std::runtime_error("at() takes exactly 1 argument at line " + std::to_string(line));
        const int index = evaluate(args[0].get()).asInt();
        if (index < 0 || index >= static_cast<int>(str.size())) {
            throw std::runtime_error("at() index is out of bounds at line " + std::to_string(line));
        }
        return Value(std::string(1, str[static_cast<size_t>(index)]));
    }

    if (method == "cut") {
        if (args.size() != 2) throw std::runtime_error("cut() takes exactly 2 arguments at line " + std::to_string(line));
        const int start = evaluate(args[0].get()).asInt();
        const int count = evaluate(args[1].get()).asInt();
        if (start < 0 || count < 0 || start > static_cast<int>(str.size())) {
            throw std::runtime_error("cut() received an invalid start or count at line " + std::to_string(line));
        }
        return Value(str.substr(static_cast<size_t>(start), static_cast<size_t>(count)));
    }

    if (method == "locate") {
        if (args.size() != 1) throw std::runtime_error("locate() takes exactly 1 argument at line " + std::to_string(line));
        const std::string part = evaluate(args[0].get()).asString();
        const size_t position = str.find(part);
        return Value(position == std::string::npos ? -1 : static_cast<int>(position));
    }

    if (method == "append") {
        if (args.size() != 1) throw std::runtime_error("add() takes exactly 1 argument at line " + std::to_string(line));
        return Value(str + evaluate(args[0].get()).asString());
    }

    if (method == "insert_at") {
        if (args.size() != 2) throw std::runtime_error("insert_at() takes exactly 2 arguments at line " + std::to_string(line));
        const int index = evaluate(args[0].get()).asInt();
        if (index < 0 || index > static_cast<int>(str.size())) {
            throw std::runtime_error("insert_at() index is out of bounds at line " + std::to_string(line));
        }
        std::string result = str;
        result.insert(static_cast<size_t>(index), evaluate(args[1].get()).asString());
        return Value(result);
    }

    if (method == "erase_at") {
        if (args.size() != 2) throw std::runtime_error("erase_at() takes exactly 2 arguments at line " + std::to_string(line));
        const int index = evaluate(args[0].get()).asInt();
        const int count = evaluate(args[1].get()).asInt();
        if (index < 0 || count < 0 || index > static_cast<int>(str.size())) {
            throw std::runtime_error("erase_at() received an invalid index or count at line " + std::to_string(line));
        }
        std::string result = str;
        result.erase(static_cast<size_t>(index), static_cast<size_t>(count));
        return Value(result);
    }

    if (method == "trim") {
        if (!args.empty()) throw std::runtime_error("trim() takes no arguments at line " + std::to_string(line));
        const size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return Value(std::string(""));
        return Value(str.substr(start, str.find_last_not_of(" \t\n\r") - start + 1));
    }

    if (method == "upper" || method == "lower") {
        if (!args.empty()) throw std::runtime_error(method + "() takes no arguments at line " + std::to_string(line));
        std::string result = str;
        const bool makeUpper = method == "upper";
        std::transform(result.begin(), result.end(), result.begin(), [makeUpper](unsigned char c) {
            return static_cast<char>(makeUpper ? std::toupper(c) : std::tolower(c));
        });
        return Value(result);
    }

    if (method == "split_by") {
        if (args.size() != 1) throw std::runtime_error("split_by() takes exactly 1 argument at line " + std::to_string(line));
        const std::string delimiter = evaluate(args[0].get()).asString();
        if (delimiter.empty()) throw std::runtime_error("split_by() delimiter cannot be empty at line " + std::to_string(line));
        std::vector<Value> parts;
        size_t start = 0;
        size_t position;
        while ((position = str.find(delimiter, start)) != std::string::npos) {
            parts.emplace_back(str.substr(start, position - start));
            start = position + delimiter.size();
        }
        parts.emplace_back(str.substr(start));
        return Value::makeList(std::move(parts));
    }

    if (method == "repeat") {
        if (args.size() != 1) throw std::runtime_error("repeat() takes exactly 1 argument at line " + std::to_string(line));
        const int count = evaluate(args[0].get()).asInt();
        if (count < 0) throw std::runtime_error("repeat() count cannot be negative at line " + std::to_string(line));
        std::string result;
        for (int i = 0; i < count; ++i) result += str;
        return Value(result);
    }

    if (method == "pad_left" || method == "pad_right" || method == "pad_center") {
        if (args.size() != 1 && args.size() != 2) {
            throw std::runtime_error(method + "() takes a width and optional fill character at line " + std::to_string(line));
        }
        const int width = evaluate(args[0].get()).asInt();

        if (width < 0) throw std::runtime_error(method + "() width cannot be negative at line " + std::to_string(line));
        const std::string fill = args.size() == 2 ? evaluate(args[1].get()).asString() : " ";

        if (fill.size() != 1) throw std::runtime_error(method + "() fill must be one character at line " + std::to_string(line));

        const int padding = std::max(0, width - static_cast<int>(str.size()));

        if (method == "pad_left") {
            return Value(std::string(static_cast<size_t>(padding), fill[0]) + str);
        }
        if (method == "pad_right") {
            return Value(str + std::string(static_cast<size_t>(padding), fill[0]));
        }
        const int left = padding / 2;
        return Value(std::string(static_cast<size_t>(left), fill[0]) + str + std::string(static_cast<size_t>(padding - left), fill[0]));
    }

    throw std::runtime_error("Unknown string method '" + method + "' at line " + std::to_string(line));
}

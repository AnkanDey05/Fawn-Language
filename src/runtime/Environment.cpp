#include "Environment.hpp"
#include <string>

void Env::define(const std::string& name, Value val,const std::string& declaredType, bool isDynamic, bool isConst)
{
    values[name] = VarEntry{val, declaredType, isDynamic, isConst};
}

Value Env::get(const Token& nameToken)
{
    std::string name = nameToken.m_lexeme;

    auto it = values.find(name);
    if (it != values.end()) {
        return it->second.value; 
    }
    if (enclosing != nullptr) {
        return enclosing->get(nameToken);
    }

    reporter->report({ErrorKind::Reference, "Undefined variable '" + name + "'",
                      nameToken.m_line, "Declare the variable before using it.", nameToken.m_column});
    throw FatalError();
}

void Env::assign(const Token& nameToken, Value val)
{
    std::string name = nameToken.m_lexeme;
    auto it = values.find(name);
    if (it != values.end()) {
        if (it->second.isConst) {
            reporter->report({ErrorKind::Runtime, "Cannot reassign const variable '" + name + "'",
                              nameToken.m_line, "Remove 'const' or assign the value only once.", nameToken.m_column});
            throw FatalError();
        }
        if (!it->second.isDynamic) {
            if (it->second.declaredType == "float" && val.isInt()) val = Value(static_cast<float>(val.asInt()));
            else if (it->second.declaredType == "int" && val.isFloat()) val = Value(static_cast<int>(val.asFloat()));
            std::string actual = val.getTypeAsString();
            bool matches =
                (it->second.declaredType == "int" && val.isInt()) ||
                (it->second.declaredType == "float" && val.isFloat()) ||
                (it->second.declaredType == "string" && val.isString()) ||
                (it->second.declaredType == "bool" && val.isBool());
            if (!matches) {
                reporter->report({ErrorKind::Type,
                                  "Cannot assign '" + actual + "' to statically-typed variable '" + name +
                                      "' (expected '" + it->second.declaredType + "')",
                                  nameToken.m_line,
                                  "Assign a value with type '" + it->second.declaredType + "'.", nameToken.m_column});
                throw FatalError();
            }
        }
        it->second.value = val;
        return;
    }
    if (enclosing != nullptr) { enclosing->assign(nameToken, val); return; }
    reporter->report({ErrorKind::Reference, "Cannot reassign undefined variable '" + name + "'",
                      nameToken.m_line, "Declare the variable before assigning to it.", nameToken.m_column});
    throw FatalError();
}

#pragma once 
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
struct NullValue
{
    bool operator==(const NullValue&) const { return true; }
};

struct Value;
using ListType = std::shared_ptr<std::vector<Value>>;

struct Value
{
    using VarientType = std::variant<int, float, std::string, bool, NullValue, ListType>;
    VarientType data;

    // Constructors ---

    Value() : data(NullValue{}) {}
    Value(int i) : data(i) {}
    Value(float f) : data(f) {}
    Value(bool b) : data(b) {}
    Value(std::string s) : data(s) {}
    Value(const char* c) : data(std::string(c)) {}
    Value(ListType l) : data(l) {}
    
    static Value makeList(std::vector<Value> elems = {}) {
        return Value(std::make_shared<std::vector<Value>>(std::move(elems)));
    }

    // Type checker ---

    bool isNull()const {return std::holds_alternative<NullValue>(data);}
    bool isInt()const {return std::holds_alternative<int>(data);}
    bool isFloat()const {return std::holds_alternative<float>(data);}
    bool isString()const {return std::holds_alternative<std::string>(data);}
    bool isBool() const {return std::holds_alternative<bool>(data);}
    bool isList() const {return std::holds_alternative<ListType>(data);}
    
    // Getters ---

    int asInt() const {
        if (auto* val = std::get_if<int>(&data)) return *val;
        throw std::runtime_error("Value is not an 'int'");
    }

    float asFloat() const {
        if (!isFloat()) {
            throw std::runtime_error("Value Is not an type 'float'");
        }
        return std::get<float>(data);
    }
    
    std::string asString() const {
        if (!isString()) 
        {
            throw std::runtime_error("Value is not a type 'string'");
        }
        return std::get<std::string>(data);
    }
    
    bool asBool() const {
        if (!isBool()) 
        {
            throw std::runtime_error("Value Is not an type 'boolean'");
        }
        return std::get<bool>(data);
    }

    ListType asList() const {
        if (!isList()) {
            throw std::runtime_error("Value is not a type 'list'");
        }
        return std::get<ListType>(data);
    }

    bool isTruthy() const {
        if (isNull()) return false;
        if (isBool()) return asBool();
        if (isInt()) return asInt() != 0;
        if (isFloat()) return asFloat() != 0.0f;
        if (isString()) return asString() != "";
        return true;
    }

    std::string stringify() const {
        if (isInt())
        {
            return std::to_string(std::get<int>(data));
        }
        if (isFloat()) 
        {
            std::string s = std::to_string(std::get<float>(data));
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last = s.find_last_not_of('0');
                if (last == dot) last++;
                s = s.substr(0, last + 1);
            }
            return s;
        }
        if (isBool()) 
        {
            return std::get<bool>(data) ? "true" : "false";
        }
        if (isString()) 
        {
            return std::get<std::string>(data);
        }
        if (isNull()) {
            return "Null";
        }
        if (isList()) {
            std::string result = "{";
            auto& list = *std::get<ListType>(data);
            for (size_t i = 0; i < list.size(); i++) {
                if (i > 0) result += ", ";
                if (list[i].isString()) result += "\"" + list[i].stringify() + "\"";
                else result += list[i].stringify();
            }
            result += "}";
            return result;
        }
        return "unknown";
    }

    std::string getTypeAsString()const
    {
        if (isInt())
        {
            return "int";
        }
        if (isFloat()) 
        {
            return "float";
        }
        if (isBool()) 
        {
            return "boolean";
        }
        if (isString()) 
        {
            return "string";
        }
        if (isNull()) {
            return "Null";
        }
        if (isList()) {
            return "list";
        }
        return "unknown";
    }

    // Mixed Types

    float asFloatPromoted() const {
        if (isInt()) return static_cast<float>(std::get<int>(data));
        if (isFloat()) return std::get<float>(data);
        throw std::runtime_error("Cannot promote"+ getTypeAsString()+ " to a float:" );
    }

    Value operator+(const Value& right) const {
        if (this->isString() || right.isString()) {
            return Value(this->stringify() + right.stringify());
        }
        if (this->isInt() && right.isInt()) {
            return Value(this->asInt() + right.asInt());
        }

        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return Value(this->asFloatPromoted() + right.asFloatPromoted());
        }

        throw std::runtime_error("Invalid operand types for '+' operation, " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    Value operator-(const Value& right) const {
        if (this->isInt() && right.isInt()) {
            return Value(this->asInt() - right.asInt());
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return Value(this->asFloatPromoted() - right.asFloatPromoted());
        }
        throw std::runtime_error("Invalid operand types for '-' operation, " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    Value operator* (const Value& right) const {
        if (this->isInt() && right.isInt()) {
            return Value(this->asInt() * right.asInt());
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return Value(this->asFloatPromoted() * right.asFloatPromoted());
        }
        throw std::runtime_error("Invalid operand types for '*' operation, " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    Value operator/ (const Value& right) const {
        if (this->isInt() && right.isInt()) {
            if (right.asInt() == 0) throw std::runtime_error("Division by zero.");
            return Value(this->asInt() / right.asInt());
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            if (right.asFloatPromoted() == 0.0f) throw std::runtime_error("Division by zero.");
            return Value(this->asFloatPromoted() / right.asFloatPromoted());
        }
        throw std::runtime_error("Invalid operand types for '/' operation.");
    }

    Value operator% (const Value& right) const {
        if (this->isInt() && right.isInt()) {
            if (right.asInt() == 0) throw std::runtime_error("Modulo by zero.");
            return Value(this->asInt() % right.asInt());
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            if (right.asFloatPromoted() == 0.0f) throw std::runtime_error("Modulo by zero.");
            return Value(static_cast<float>(std::fmod(this->asFloatPromoted(), right.asFloatPromoted())));
        }
        throw std::runtime_error("Invalid operand types for '%' operation."); // Fixed typo
    }

    bool operator== (const Value& right) const {
        if (this->isBool() && right.isBool()) return this->asBool() == right.asBool();
        if (this->isNull() && right.isNull()) return true;
        
        if (this->isString() || right.isString()) {
            return this->stringify() == right.stringify();
        }
        if (this->isInt() && right.isInt()) {
            return this->asInt() == right.asInt();
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return this->asFloatPromoted() == right.asFloatPromoted();
        }
        return false; 
    }
    bool operator!= (const Value& right) const {
        return !(*this == right); 
    }

    bool strictEquals(const Value& right) const 
    {
        return this->data == right.data; 
    }

    Value operator&&(const Value& right) const {
        return Value(this->isTruthy() && right.isTruthy());
    }

    Value operator||(const Value& right) const {
        return Value(this->isTruthy() || right.isTruthy());
    }
    
    bool operator<(const Value& right) const {
        if (this->isString() && right.isString()) {
            return this->asString() < right.asString();
        }
        if (this->isInt() && right.isInt()) {
            return this->asInt() < right.asInt();
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return this->asFloatPromoted() < right.asFloatPromoted();
        }
        throw std::runtime_error("Invalid operand types for '<' operation: " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    bool operator>(const Value& right) const {
        if (this->isString() && right.isString()) {
            return this->asString() > right.asString();
        }
        if (this->isInt() && right.isInt()) {
            return this->asInt() > right.asInt();
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return this->asFloatPromoted() > right.asFloatPromoted();
        }
        throw std::runtime_error("Invalid operand types for '>' operation: " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    bool operator<=(const Value& right) const {
        if (this->isString() && right.isString()) {
            return this->asString() <= right.asString();
        }
        if (this->isInt() && right.isInt()) {
            return this->asInt() <= right.asInt();
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return this->asFloatPromoted() <= right.asFloatPromoted();
        }
        throw std::runtime_error("Invalid operand types for '<=' operation: " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }

    bool operator>=(const Value& right) const {
        if (this->isString() && right.isString()) {
            return this->asString() >= right.asString();
        }
        if (this->isInt() && right.isInt()) {
            return this->asInt() >= right.asInt();
        }
        if ((this->isInt() || this->isFloat()) && (right.isInt() || right.isFloat())) {
            return this->asFloatPromoted() >= right.asFloatPromoted();
        }
        throw std::runtime_error("Invalid operand types for '>=' operation: " + this->getTypeAsString() + " and " + right.getTypeAsString());
    }
};
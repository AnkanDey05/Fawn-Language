#pragma once
#include "../common/Token.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Lexer {
private:
  inline static const std::unordered_map<std::string, TokenType> keywords = {
    {"fn", TokenType::Func},    {"out", TokenType::Out},
    {"if", TokenType::If},        {"elif", TokenType::Elif},
    {"else", TokenType::Else},    {"while", TokenType::While},
    {"for", TokenType::For},      {"in", TokenType::In},
    {"step", TokenType::Step},    {"var", TokenType::Var},
    {"const", TokenType::Const}, {"list", TokenType::List},
    {"end", TokenType::End},     {"true", TokenType::True},
    {"false", TokenType::False}, {"null", TokenType::Null},
    {"and", TokenType::And},     {"or", TokenType::Or},
    {"not", TokenType::Not},     {"int", TokenType::TypeKeyword},
    {"float", TokenType::TypeKeyword}, {"string", TokenType::TypeKeyword},
    {"bool", TokenType::TypeKeyword}, {"return", TokenType::Return},
    {"break",  TokenType::Break}, {"exit",   TokenType::Exit},
    {"then",   TokenType::Then}, {"continue", TokenType::Continue},};
  ;
  std::string m_source;
  size_t m_line = 1;
  size_t m_column = 1;
  size_t m_current{};
  Token scanNumber();
  Token scanString();
  Token scanWord();
  Token scanOperatorOrSymbols();
  void skipComment();
  bool match(const char expected);

public:
  explicit Lexer(const std::string &source)
      : m_source(source), m_line(1), m_current(0) {};
  std::vector<Token> tokenize();
  char peek() const;
  char peekNext() const;
  char advance();
  bool isAtEnd() const; 
};

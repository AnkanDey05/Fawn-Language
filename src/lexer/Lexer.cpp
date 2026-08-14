#include "Lexer.hpp"
#include <cctype>
#include <string>
#include <vector>

// Helpers 

bool Lexer::isAtEnd() const { return m_current >= m_source.size(); }
char Lexer::peek() const {
  if (isAtEnd()) {
    return '\0';
  }
  return m_source[m_current];
}
char Lexer::peekNext() const {
    if (isAtEnd()) {
        return '\0';
    }
    if (m_current +1 == m_source.size()) {
        return '\0';
    }
    return m_source[m_current +1 ];
}
char Lexer::advance() {
  if (isAtEnd()) {
    return '\0';
  }
  if (m_source[m_current] == '\n') {
    m_line++;
    m_column = 1;
  } else {
    m_column++;
  }
  return m_source[m_current++];
}
bool Lexer::match(const char expected){
   if (isAtEnd()){
        return false;
    }
    if (peek() != expected){
        return false;
    }
    advance();

    return true;
}


// Scaning for tokens 

Token Lexer::scanString(){
    std::string result;    
    const size_t startColumn = m_column - 1;

    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            return Token(TokenType::Invalid, "", m_line, startColumn); 
        }
        if (peek() == '\\') {
            advance(); 
            char esc = advance(); 
            switch (esc) {
                case 'n': result.push_back('\n'); break;
                case 't': result.push_back('\t'); break;
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case 'e': result.push_back('\x1B'); break; 
                case 'r': result.push_back('\r'); break;
                default: result.push_back(esc); break; 
            }
        } else {
            result.push_back(advance());
        }
    }

    if (peek() == '"') {
        advance();
    } else {
        return Token(TokenType::Invalid, result, m_line, startColumn);
    }

    return Token(TokenType::String, result, m_line, startColumn);
}
void Lexer::skipComment(){
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}
Token Lexer::scanNumber() {
  std::string result{};
  const size_t startColumn = m_column;
  while (isdigit(static_cast<unsigned char>(peek()))) {
    result.push_back(advance());
  }
   if (peek() == '.' && isdigit(static_cast<unsigned char>(peekNext()))) {
        result.push_back(advance()); // consume '.'
        while (isdigit(static_cast<unsigned char>(peek()))) {
            result.push_back(advance());
        }
    }
  return Token(TokenType::Number, result, m_line, startColumn);
}
Token Lexer::scanWord() {
  std::string result;
  const size_t startColumn = m_column;
  while (isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
    result.push_back(advance());
  }
  auto match = keywords.find(result);
  if (match != keywords.end()) {
    return Token(match->second, result, m_line, startColumn);
  }
  return Token(TokenType::Identifier, result, m_line, startColumn);
}
Token Lexer::scanOperatorOrSymbols()
{
    const size_t startColumn = m_column;
    char ch = advance();
    switch (ch) {
            case '(': return Token(TokenType::LeftParen, "(", m_line, startColumn);
            case ')': return Token(TokenType::RightParen, ")", m_line, startColumn);
            case '{': return Token(TokenType::LeftBrace, "{", m_line, startColumn);
            case '}': return Token(TokenType::RightBrace, "}", m_line, startColumn);
            case '[': return Token(TokenType::LeftBracket, "[", m_line, startColumn);
            case ']': return Token(TokenType::RightBracket, "]", m_line, startColumn);
            case ',': return Token(TokenType::Comma, ",", m_line, startColumn);
            case '.': return Token(TokenType::Dot, ".", m_line, startColumn);
            case ':': return Token(TokenType::Colon, ":", m_line, startColumn);
            case '%': return Token(TokenType::Modulo, "%", m_line, startColumn);
            case '*': return Token(TokenType::Star, "*", m_line, startColumn);

            case '+':
                if (match('+')) {  return (Token(TokenType::Increment, "++", m_line)); }
                else { return(Token(TokenType::Plus, "+", m_line)); }

            case '-':
                if (match('-')) {  
                   return Token(TokenType::Decrement, "--", m_line); }
                else if (match('>')) {  return Token(TokenType::Arrow, "->", m_line); }
                else { return Token(TokenType::Minus, "-", m_line); }

            case '=':
                if (match('=')) {
                    
                    if (match('=')) {
                        
                        return (Token(TokenType::EqualEqualEqual, "===", m_line));
                    } else {
                        return (Token(TokenType::EqualEqual, "==", m_line));
                    }
                } else {
                    return (Token(TokenType::Equal, "=", m_line));
                }

            case '<':
                if (match('=')) { return (Token(TokenType::LessEqual, "<=", m_line)); }
                else if (match('<')) {  return (Token(TokenType::ShiftLeft, "<<", m_line)); }
                else { return (Token(TokenType::Less, "<", m_line)); };

            case '>':
                if (match('=')) {  return (Token(TokenType::GreaterEqual, ">=", m_line)); }
                else if (match('>')) {  return (Token(TokenType::ShiftRight, ">>", m_line)); }
                else { return (Token(TokenType::Greater, ">", m_line)); }

            case '!':
                if (match('=')) {  return (Token(TokenType::NotEqual, "!=", m_line)); }
                else { return (Token(TokenType::Not, "!", m_line)); } 

            case '?': return (Token(TokenType::QuestionMark, "?", m_line));

            case '/': return (Token(TokenType::Slash, "/", m_line));

            default:
                return (Token(TokenType::Invalid, std::string(1, ch), m_line));
            }  
}


// Making Tokens 

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> result;
  while (!isAtEnd()) {
    char ch = peek();
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      advance();
    } else if (ch == '\n') {
        result.push_back(Token(TokenType::NewLine, "\\n", m_line));
        advance();
    } else if (std::isdigit(static_cast<unsigned char>(ch))) {
      result.push_back(scanNumber());
    } else if (std::isalpha(static_cast<unsigned char>(ch)) || peek() == '_') {
      result.push_back(scanWord());
    }
    else if (ch == '"') {
        advance();
        result.push_back(scanString());
    }
    else if (peek() == '/' && peekNext() == '*') {
        // multiline comment
        advance(); // consume '/'
        advance(); // consume '*'
        while (!isAtEnd()) {
            if (peek() == '*' && peekNext() == '/') {
                advance(); // consume '*'
                advance(); // consume '/'
                break;
            }
            advance();
        }
        continue;
    }
    else if (peek() == '/' && peekNext() == '/') {
        skipComment();
        continue;
    }
    else {
            result.push_back(scanOperatorOrSymbols());
        }
    }
    result.push_back(Token(TokenType::EndOfFile, "", m_line));
    return result;
  }

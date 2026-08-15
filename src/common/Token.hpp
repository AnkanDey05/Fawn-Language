#pragma once
#include <cstddef>
#include <string>
#include <string_view>

enum class TokenType
{
    // Literals
    Number,
    String,
    Identifier,

    // Keywords
    TypeKeyword,
    Func,
    Out,
    If,
    Elif,
    Else,
    While,
    For,
    In,
    Step,
    Print,
    Read,
    Var,
    Const,
    List,
    True,
    False,
    Null,
    End,

    // Operators
    Arrow,
    Plus,
    Minus,
    Star,
    Slash,
    Modulo,
    Increment,
    Decrement,

    Equal,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    ModuloEqual,
    EqualEqual,
    EqualEqualEqual,
    NotEqual,

    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    And,
    Or,
    Not,
    Colon,

    // Symbols
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Escape,
    Exit,
    Break,
    Continue,
    QuestionMark,

    Comma,
    Dot,

    ShiftLeft,   
    ShiftRight,  
    Return,
    Then,
    NewLine,
    EndOfFile,
    Invalid
};

struct Token{
    TokenType m_type;
    std::string m_lexeme;
    size_t m_line = 1;
    size_t m_column = 1;
    Token(TokenType type, std::string_view lexeme, size_t line, size_t column = 1)
        : m_type(type), m_lexeme(lexeme), m_line(line), m_column(column) {};
};

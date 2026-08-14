#pragma once

#include <cstddef>
class Node
{   public:
    size_t m_line{};
    Node(size_t line) : m_line(line) {}; 
    virtual ~Node() = default;
};
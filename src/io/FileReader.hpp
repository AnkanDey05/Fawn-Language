#pragma once 

#include <filesystem>
#include <string>
class FileReader{
    public:
        static std::string read(const std::filesystem::path& filepath);
};
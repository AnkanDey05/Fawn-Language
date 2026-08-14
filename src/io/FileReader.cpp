#include "FileReader.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace fs  = std::filesystem;
std::string FileReader::read(const std::filesystem::path &filepath){
    auto resolved  = std::filesystem::absolute(filepath);
    if (!fs::exists(resolved)) {
        throw std::runtime_error("File Path did not exist\n");
    }
    if (!fs::is_regular_file(filepath)) {
        throw std::runtime_error("Not a Valid File" + filepath.string() +"\n");
    }
    if (resolved.extension() != ".fw") {
        throw std::runtime_error("The file is not a Fawn script \n");
    }
    std::ifstream file(resolved);
    if (!file.is_open()) {
        throw std::runtime_error("Could Not Open the File" +  filepath.string() + "\n");
    }
    std::stringstream buffer;
    buffer<< file.rdbuf();
    return buffer.str();
}
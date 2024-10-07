#include <Logger.hpp>

#include <fstream>
#include <iostream>

Logger::Logger(const std::string& file_path) :
    _file_path{"../" + file_path}
    {}

void Logger::write_log(const std::string& message) {
    std::ofstream file;
    file.open(_file_path, std::ios::app);
    if(!file.is_open()) {
        std::cerr << "The file could not be opened for writing: " + _file_path  << std::endl;
    } else {
        file << message;
        file.close();
    }
}

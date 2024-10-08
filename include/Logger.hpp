#pragma once

#include <string>

class Logger {
public:
    Logger(const std::string& file_path);
    bool write_log(const std::string& message);
private:
    std::string _file_path;
};
#pragma once

#include <fstream>
#include <iostream>
#include <string>

class Logger {
   public:
    Logger(const std::string& file_path);        // set path to file
    bool write_log(const std::string& message);  // write log to file
   private:
    std::string _file_path;
};
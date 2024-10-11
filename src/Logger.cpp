#include <Logger.hpp>

Logger::Logger(const std::string& file_path) : _file_path{"../" + file_path} {}

bool Logger::write_log(const std::string& message) {
    std::ofstream file;

    file.open(_file_path, std::ios::app);
    bool result = file.is_open();
    if (!result) {
        std::cerr << "The file could not be opened for writing: " + _file_path << std::endl;
    } else {
        file << message;
        file.close();
    }
    return result;
}

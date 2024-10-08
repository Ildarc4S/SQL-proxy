#include "ProxyServer.hpp"

#include <fstream>

bool fileExists(const std::string& path) {
    std::ifstream file("../" + path);
    return file.good(); // Проверяем, можно ли открыть файл
}

int main(int argc, char* argv[]) {
    if(argc != 6){
        std::cout << "Вы указали не все параметры, остальные будут выставлены значениями по умолчанию\n" << 
            "{executable} <proxy_server_host> <proxy_server_port> <sql_server_host> <sql_server_port> <log_file_path>";

    } else {
        int proxy_server_port = atoi(argv[2]);
        int sql_server_port = atoi(argv[4]);

        if(fileExists(argv[5])) {
            ProxyServer proxy(argv[1], proxy_server_port, argv[3], sql_server_port, argv[5]);
            proxy.start();
        } else {
            std::cout << "Некорректный файл";
        }
    
    }

    return 0;
}
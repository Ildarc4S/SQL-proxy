#include "ProxyServer.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    if(argc != 6){
        std::cout << "Вы указали некорректные параметры:\n" << 
            "{executable} <proxy_server_host> <proxy_server_port> <sql_server_host> <sql_server_port> <log_file_path>";
    } else {
        int proxy_server_port = atoi(argv[2]);
        int sql_server_port = atoi(argv[4]);

        ProxyServer proxy(argv[1], proxy_server_port, argv[3], sql_server_port, argv[5]);
        
        proxy.start();
    }

    return 0;
}
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>  
#include <string>

#include "ProxyServer.hpp"

bool fileExists(const std::string& path) {
    std::ifstream file("../" + path);
    return file.good(); 
}

bool isValidIPAddress(const std::string& ip) {
    bool correct_address = true;
    std::regex ip_regex("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    if (!std::regex_match(ip, ip_regex)) {
        correct_address = false;
    }

    std::stringstream ss(ip);
    std::string part_ip;
    while (std::getline(ss, part_ip, '.')) {
        int num = std::stoi(part_ip);
        if (num < 0 || num > 255) {
            correct_address = false;
        }
    }
    return correct_address;
}

bool validateInput(int argc, char* argv[], std::string& errorMsg) {
    bool correct_params = true;
    if (argc != 6) {
        errorMsg =
            "You have not specified all the parameters: \n"
            "\"" + std::string(argv[0]) + "\" <proxy_server_host> <proxy_server_port> <sql_server_host> <sql_server_port>"
            "<log_file_path>";
        correct_params = false;
    } else {

    if (!isValidIPAddress(argv[1])) {
        errorMsg = "Invalid IP address of the proxy server: " + std::string(argv[1]);
        correct_params = false;
    }

    if (!isValidIPAddress(argv[3])) {
        errorMsg = "Invalid IP address of the SQL server: " + std::string(argv[3]);
        correct_params = false;
    }

    int proxy_server_port = atoi(argv[2]);
    int sql_server_port = atoi(argv[4]);

    if (proxy_server_port <= 0 || sql_server_port <= 0) {
        errorMsg = "Ports must be positive numbers!s";
        correct_params = false;
    }

    if (!fileExists(argv[5])) {
        errorMsg = "No such file or directory: " + std::string(argv[5]);
        correct_params = false;
    }
    }
    return correct_params;
}

int main(int argc, char* argv[]) {
    std::string errorMsg;
    if (!validateInput(argc, argv, errorMsg)) {
        std::cout << errorMsg << std::endl;
    } else {
        ProxyServer proxy(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), argv[5]);
        proxy.start();
    }
    return 0;
}

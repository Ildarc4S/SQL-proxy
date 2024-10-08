#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <PacketParser.hpp>
#include <Logger.hpp>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1024

struct Client {
    int sockfd; // Файловый дескриптор сокета
    char buffer[BUFFER_SIZE]; // Буфер для данных
    int buffer_size; // Размер данных в буфере
    bool authenticated = false; // Флаг авторизации
};

class ProxyServer {
public:
    ProxyServer(const std::string& server_ip, int server_port, 
                const std::string& sql_servre_ip, int sql_server_port, 
                const std::string& log_file_path);
    void start();
private:
    bool initializeSocket();
    bool acceptClient();
    bool handleRequest(int client_socket);
    bool sendMessage(int socket_fd, char* message, size_t len);
    bool forwardToServer(int client_socket_fd, Client& client);
    void closeSocket(int socket);
    bool logSQLToFile(const char* message, size_t len);
    void addSocketToEpoll(int socket_fd);

    std::string _proxy_server_ip;
    int _proxy_server_port;

    std::string _sql_server_ip;
    int _sql_server_port;
    int _sql_server_fd;

    int _server_socket_fd;
    int _epoll_fd;

    std::map<int, Client> clients; // Карта для хранения клиентов

    PacketParser _packetParser;
    Logger _logger;
};

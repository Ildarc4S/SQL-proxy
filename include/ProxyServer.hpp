#pragma once

#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

struct Client {
    int sockfd; // Файловый дескриптор сокета
    char buffer[BUFFER_SIZE]; // Буфер для данных
    int buffer_size; // Размер данных в буфере
    bool authenticated = false; // Флаг авторизации
    int pg_sockfd; // Файловый дескриптор сокета для PostgreSQL-сервера
};

class ProxyServer {
public:
    ProxyServer(const std::string& server_ip, int server_port);
    void start();

private:
    void initializeSocket();
    void acceptClient();
    void handleRequest(int client_socket);
    void sendMessage(int sockfd, char* message, size_t len);
    void forwardToServer(int client_socket, Client& client);
    void closeSocket(int socket);
    void logSQLToFile(const char* message, int message_size);
    std::string _proxy_server_ip;
    int _proxy_server_port;
    int _server_socket;
    int _epoll_fd;
    std::map<int, Client> clients; // Карта для хранения клиентов
};

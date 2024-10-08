#include "ProxyServer.hpp"
#include "SocketManager.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <fstream>
#include <signal.h>

volatile int is_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nПолучен сигнал SIGINT. Завершение работы сервера..." << std::endl;
        is_running = false;
    }
}

ProxyServer::ProxyServer(const std::string& server_ip, int server_port, 
                        const std::string& sql_servre_ip, int sql_server_port, 
                        const std::string& log_file_path) : 
    _proxy_server_ip{server_ip}, 
    _proxy_server_port{server_port}, 
    _sql_server_ip{sql_servre_ip},
    _sql_server_port{sql_server_port},
    _server_socket_fd{-1},
    _logger{log_file_path}
     {}

void ProxyServer::start() {
    signal(SIGINT, signalHandler);
    is_running = initializeSocket();
    if(is_running) {
        std::cout << "Прокси-сервер запущен на порту " << _proxy_server_port << std::endl;
    }

    _epoll_fd = SocketManager::epollCreate();
    if (_epoll_fd == -1) {
        is_running = false;
    }

    if (SocketManager::addSocketToEpoll(_server_socket_fd, _epoll_fd) == -1) {
        is_running = false;
    }

    while (is_running) {
        std::vector<epoll_event> events(MAX_CLIENTS);
        int ev_count =  SocketManager::epollWait(_epoll_fd, events, MAX_CLIENTS, is_running);
        if (ev_count == -1) {
            is_running = false;
        }

        for (int i = 0; i < ev_count && is_running; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == _server_socket_fd) {
                    is_running = acceptClient();
                } else {
                    is_running = handleRequest(events[i].data.fd);
                }
            } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                closeSocket(events[i].data.fd);
            }
        }
    }

    close(_epoll_fd);
    close(_server_socket_fd);
    clients.clear(); 
}

bool ProxyServer::initializeSocket() {
    bool func_result = true;

    _server_socket_fd = SocketManager::creatteSocket();
    if (_server_socket_fd < 0) {
        func_result = false;
    }

    int opt = 1;
    if (setsockopt(_server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Ошибка при установке параметров сокета: " << std::strerror(errno) << std::endl;
        func_result = false;
    }

    struct sockaddr_in address;
    SocketManager::createConnectorAddress(address, _proxy_server_port);

    
    if (SocketManager::bindSocket(_server_socket_fd, address) < 0) {
        func_result = false;
    }

    if (SocketManager::listenSocket(_server_socket_fd, 512) < 0) {
        std::cerr << "Ошибка при прослушивании сокета: " << std::strerror(errno) << std::endl;
        func_result = false;
    }

    return func_result;
}

bool ProxyServer::acceptClient() {
    bool func_result = true;

    struct sockaddr_in address;
    int client_fd = SocketManager::acceptConnection(_server_socket_fd, address);
    if (client_fd == -1) {
        func_result = false;
    }

    // Создание сокета для PostgreSQL-сервера
    _sql_server_fd = SocketManager::creatteSocket();
    if (_sql_server_fd == -1) {
        close(client_fd);
        func_result = false;
    }

    // Подключение к PostgreSQL-серверу
    if (SocketManager::connectServer(_sql_server_fd, _sql_server_ip, _sql_server_port) == -1) {
        close(client_fd);
        //close(_sql_server_fd);
        func_result = false;
    }

    // Добавление нового клиента в карту
    clients[client_fd] = {client_fd, "", 0, false};

    if (SocketManager::addSocketToEpoll(client_fd, _epoll_fd) == -1) {
        is_running = false;
    }
    
    std::cout << "Новый клиент подключился: " << inet_ntoa(address.sin_addr) << std::endl;

    return func_result;
}

bool ProxyServer::handleRequest(int client_socket) {
    bool func_result = true;
    Client& client = clients[client_socket];
    // Читаем данные от клиента
    client.buffer_size = recv(client_socket, client.buffer, BUFFER_SIZE, 0);
    if (client.buffer_size <= 0) {
        closeSocket(client_socket);
        clients.erase(client_socket);
        return true;
    }


    if (!client.authenticated) {
        // Отправка данных на PostgreSQL
        func_result = sendMessage(_sql_server_fd, client.buffer, client.buffer_size);

        // Получаем ответ от PostgreSQL-сервера
        client.buffer_size = recv(_sql_server_fd, client.buffer, BUFFER_SIZE, 0);
        if (client.buffer_size == 0) {
            closeSocket(client_socket);
            func_result = false;
        }
        
        func_result = sendMessage(client_socket, client.buffer, client.buffer_size); // Send AuthenticationOk
        client.authenticated = true;
    } else {
        // Перенаправление запросов после авторизации
        func_result = logSQLToFile(client.buffer, client.buffer_size);
        func_result &= forwardToServer(client_socket, client);
    }

    return func_result;
}

bool ProxyServer::sendMessage(int socket_fd, char* message, size_t len) {
    bool result = true;
    if (SocketManager::sendMessage(socket_fd, message, len) == -1) {
        closeSocket(socket_fd);
    }
    return result;
}

bool ProxyServer::forwardToServer(int client_socket, Client& client) {
    bool func_result = true;
    func_result = sendMessage(_sql_server_fd, client.buffer, client.buffer_size);
    client.buffer_size = SocketManager::recvMessage(_sql_server_fd, client.buffer, BUFFER_SIZE);

    if (client.buffer_size > 0) {
        if(client.buffer[0] == 'E') {
            func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
            client.buffer_size = recv(_sql_server_fd, client.buffer, BUFFER_SIZE, 0);
            func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
        } else {
            func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
        }
    } else {
        closeSocket(client_socket);
    }
    return func_result;
}

void ProxyServer::closeSocket(int socket) {
    std::cout << "Клиент отключился: " << socket << std::endl;
    close(socket);
    clients.erase(socket); // Удаляем клиента из карты
}

bool ProxyServer::logSQLToFile(const char* message, size_t len) {
    return _logger.write_log(_packetParser.parsePacket(message, len));
}

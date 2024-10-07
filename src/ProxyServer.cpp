#include "ProxyServer.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <fstream>
#include <signal.h>

volatile sig_atomic_t is_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nПолучен сигнал SIGINT. Завершение работы сервера..." << std::endl;
        is_running = false;
    }
}

void ProxyServer::addSocketToEpoll(int socket_fd) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        std::cerr << "Ошибка при добавлении сокета в epoll: " << std::strerror(errno) << std::endl;
        is_running = false;
        return;
    }
}



ProxyServer::ProxyServer(const std::string& server_ip, int server_port, 
                        const std::string& sql_servre_ip, int sql_server_port, 
                        const std::string& log_file_path) : 
    _proxy_server_ip{server_ip}, 
    _proxy_server_port{server_port}, 
    _sql_server_ip{sql_servre_ip},
    _sql_server_port{sql_server_port},
    _server_socket{-1},
    _logger{log_file_path}
     {}

void ProxyServer::start() {
    signal(SIGINT, signalHandler);

    initializeSocket();
    std::cout << "Прокси-сервер запущен на порту " << _proxy_server_port << std::endl;

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        std::cerr << "Ошибка при создании epoll: " << std::strerror(errno) << std::endl;
        is_running = false;
    }

    addSocketToEpoll(_server_socket);

    while (is_running) {
        std::vector<epoll_event> events(MAX_CLIENTS);
        int ev_count = epoll_wait(_epoll_fd, events.data(), MAX_CLIENTS, -1);
        if (ev_count == -1) {
            if (is_running) {
                std::cerr << "Ошибка epoll_wait: " << std::strerror(errno) << std::endl;
                is_running = false;
            }
            continue;
        }

        for (int i = 0; i < ev_count && is_running; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == _server_socket) {
                    acceptClient();
                } else {
                    handleRequest(events[i].data.fd);
                }
            } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                closeSocket(events[i].data.fd);
            }
        }
    }

    close(_epoll_fd);
    close(_server_socket);
    clients.clear(); 
}

void ProxyServer::initializeSocket() {
    _server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_socket < 0) {
        std::cerr << "Ошибка при создании сокета: " << std::strerror(errno) << std::endl;
        return;
    }

    int opt = 1;
    if (setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Ошибка при установке параметров сокета: " << std::strerror(errno) << std::endl;
        return;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_proxy_server_port);

    if (bind(_server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Ошибка при привязке сокета: " << std::strerror(errno) << std::endl;
        return;
    }

    if (listen(_server_socket, 3) < 0) {
        std::cerr << "Ошибка при прослушивании сокета: " << std::strerror(errno) << std::endl;
        return;
    }
}

void ProxyServer::acceptClient() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd = accept(_server_socket, (struct sockaddr*)&address, &addrlen);
    if (client_fd == -1) {
        std::cerr << "Ошибка при принятии клиента: " << std::strerror(errno) << std::endl;
        return;
    }

    // Создание сокета для PostgreSQL-сервера
    int sql_servere_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sql_servere_sockfd == -1) {
        std::cerr << "Ошибка при создании сокета PostgreSQL: " << std::strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    // Подключение к PostgreSQL-серверу
    struct sockaddr_in sql_server_addres;
    sql_server_addres.sin_family = AF_INET;
    sql_server_addres.sin_addr.s_addr = inet_addr(_sql_server_ip.c_str()); // Замените на IP-адрес PostgreSQL-сервера
    sql_server_addres.sin_port = htons(_sql_server_port);
    if (connect(sql_servere_sockfd, (struct sockaddr*)&sql_server_addres, sizeof(sql_server_addres)) == -1) {
        std::cerr << "Ошибка при подключении к PostgreSQL: " << std::strerror(errno) << std::endl;
        close(client_fd);
        close(sql_servere_sockfd);
        return;
    }

    // Добавление нового клиента в карту
    clients[client_fd] = {client_fd, "", 0, false, sql_servere_sockfd};

    addSocketToEpoll(client_fd);
    
    std::cout << "Новый клиент подключился: " << inet_ntoa(address.sin_addr) << std::endl;
}

void ProxyServer::handleRequest(int client_socket) {
    Client& client = clients[client_socket];
    // Читаем данные от клиента
    client.buffer_size = recv(client_socket, client.buffer, BUFFER_SIZE, 0);
    if (client.buffer_size <= 0) {
        closeSocket(client_socket);
        clients.erase(client_socket);
        return;
    }

    if (!client.authenticated) {
        // Отправка данных на PostgreSQL
        sendMessage(client.sql_servere_sockfd, client.buffer, client.buffer_size);

        // Получаем ответ от PostgreSQL-сервера
        client.buffer_size = recv(client.sql_servere_sockfd, client.buffer, BUFFER_SIZE, 0);
        if (client.buffer_size == 0) {
            closeSocket(client_socket);
            return;
        }
        // Отправка AuthenticationOk
        sendMessage(client_socket, client.buffer, client.buffer_size);
        client.authenticated = true;
    } else {
        // Перенаправление запросов после авторизации
        logSQLToFile(client.buffer, client.buffer_size);
        forwardToServer(client_socket, client);
    }
}

void ProxyServer::sendMessage(int sockfd, char* message, size_t len) {
    message[len] = '\0';
    if (send(sockfd, message, len, 0) == -1) {
        std::cerr << "Ошибка отправки сообщения: " << std::strerror(errno) << std::endl;
        closeSocket(sockfd);
    }
}

void ProxyServer::forwardToServer(int client_socket, Client& client) {

        sendMessage(client.sql_servere_sockfd, client.buffer, client.buffer_size);
        client.buffer_size = recv(client.sql_servere_sockfd, client.buffer, BUFFER_SIZE, 0);

        if (client.buffer_size > 0) {
            if(client.buffer[0] == 'E') {
                sendMessage(client_socket, client.buffer, client.buffer_size);
                client.buffer_size = recv(client.sql_servere_sockfd, client.buffer, BUFFER_SIZE, 0);
                sendMessage(client_socket, client.buffer, client.buffer_size);
            } else {
                sendMessage(client_socket, client.buffer, client.buffer_size);
            }
        } else {
            closeSocket(client_socket);
        }
}

void ProxyServer::closeSocket(int socket) {
    close(socket);
    clients.erase(socket); // Удаляем клиента из карты
}

void ProxyServer::logSQLToFile(const char* message, int message_size) {
    _logger.write_log(_packetParser.parsePacket(message, message_size));
}

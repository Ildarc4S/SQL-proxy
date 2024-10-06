#include "../include/ProxyServer.hpp"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <fstream>

ProxyServer::ProxyServer(const std::string& server_ip, int server_port)
    : _proxy_server_ip(server_ip), _proxy_server_port(server_port), _server_socket(-1) {}

void ProxyServer::start() {
    initializeSocket();
    std::cout << "Прокси-сервер запущен на порту " << _proxy_server_port << std::endl;

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _server_socket;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_socket, &event) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    

    while (true) {
        std::vector<epoll_event> events(MAX_CLIENTS);
        int ev_count = epoll_wait(_epoll_fd, events.data(), MAX_CLIENTS, -1);
        if (ev_count == -1) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < ev_count; i++) {
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

    close(_server_socket);
}

void ProxyServer::initializeSocket() {
    _server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_socket < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_proxy_server_port);

    if (bind(_server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(_server_socket, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

void ProxyServer::acceptClient() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd = accept(_server_socket, (struct sockaddr*)&address, &addrlen);
    if (client_fd == -1) {
        perror("accept failed");
        return;
    }

    // Создание сокета для PostgreSQL-сервера
    int pg_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (pg_sockfd == -1) {
        perror("socket failed");
        close(client_fd);
        return;
    }

    // Подключение к PostgreSQL-серверу
    struct sockaddr_in pg_address;
    pg_address.sin_family = AF_INET;
    pg_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Замените на IP-адрес PostgreSQL-сервера
    pg_address.sin_port = htons(5432);
    if (connect(pg_sockfd, (struct sockaddr*)&pg_address, sizeof(pg_address)) == -1) {
        perror("connect failed");
        close(client_fd);
        close(pg_sockfd);
        return;
    }

    // Добавление нового клиента в карту
    clients[client_fd] = {client_fd, "", 0, false, pg_sockfd};

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }
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
    std::cout << client.buffer_size << ":\n";
    std::cout << client.buffer << "-\n";

    if (!client.authenticated) {
        // Отправка данных на PostgreSQL
        sendMessage(client.pg_sockfd, client.buffer, client.buffer_size);

        // Получаем ответ от PostgreSQL-сервера
        client.buffer_size = recv(client.pg_sockfd, client.buffer, BUFFER_SIZE, 0);
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
        perror("Ошибка отправки сообщения");
        closeSocket(sockfd);
    }
}

void ProxyServer::forwardToServer(int client_socket, Client& client) {

        sendMessage(client.pg_sockfd, client.buffer, client.buffer_size);
        client.buffer_size = recv(client.pg_sockfd, client.buffer, BUFFER_SIZE, 0);
        std::cout << client.buffer <<  " " << client.buffer_size << "\n";

        if (client.buffer_size > 0) {
            if(client.buffer[0] == 'E') {
                std::cout << client.buffer << "\n";
                sendMessage(client_socket, client.buffer, client.buffer_size);
                close(client_socket);
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
    std::ofstream file;
    file.open("../logs/a.txt", std::ios::app);
    file << message << '\n';
    file.close();
}

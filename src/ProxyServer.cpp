#include "ProxyServer.hpp"

bool ProxyServer::is_running = true;

void ProxyServer::signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nA completion signal has been received. Shutting down the server!!!" << std::endl;
        is_running = false;
    }
}

ProxyServer::ProxyServer(const std::string& server_ip, int server_port, const std::string& sql_servre_ip,
                         int sql_server_port, const std::string& log_file_path)
    : _proxy_server_ip{server_ip},
      _proxy_server_port{server_port},
      _sql_server_ip{sql_servre_ip},
      _sql_server_port{sql_server_port},
      _server_socket_fd{-1},
      _logger{log_file_path} {}

ProxyServer::~ProxyServer() {
    for (const auto& client : clients) {
        close(client.first);
    }
    clients.clear();
    if (_server_socket_fd != -1) {
        close(_server_socket_fd);
    }
    if (_epoll_fd != -1) {
        close(_epoll_fd);
    }
}

void ProxyServer::start() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    is_running = initializeSocket();
    if (is_running) {
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
        int ev_count = SocketManager::epollWait(_epoll_fd, events, MAX_CLIENTS);
        if (ev_count == -1) {
            is_running = false;
        }

        for (int i = 0; i < ev_count && is_running; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == _server_socket_fd) {
                    acceptClient();
                } else {
                    handleRequest(events[i].data.fd);
                }
            } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                closeSocket(events[i].data.fd);
                std::cout << "1\n";
            }
        }
    }
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
        func_result = false;
    }

    return func_result;
}

void ProxyServer::acceptClient() {
    bool func_result = true;

    struct sockaddr_in address;
    int client_fd = SocketManager::acceptConnection(_server_socket_fd, address);
    if (client_fd == -1) {
        func_result = false;
    }

    // creating socket for PostgreSQL server
    int _sql_server_fd = SocketManager::creatteSocket();
    if (_sql_server_fd == -1) {
        close(client_fd);
        func_result = false;
    }

    // connecting to  PostgreSQL server
    if (SocketManager::connectServer(_sql_server_fd, _sql_server_ip, _sql_server_port) == -1) {
        close(client_fd);
        func_result = false;
    }

    // add a new client to map
    clients[client_fd] = {client_fd, _sql_server_fd, "", 0, false};

    if (SocketManager::addSocketToEpoll(client_fd, _epoll_fd) == -1) {
        func_result = false;
    }

    if (func_result) {
        std::cout << "A new client has connected: " << inet_ntoa(address.sin_addr) << std::endl;
    } else {
        std::cout << "A new client is not connected: " << inet_ntoa(address.sin_addr) << std::endl;
        if (client_fd != -1) {
            closeSocket(client_fd);
        }
        if (_sql_server_fd != -1) {
            closeSocket(_sql_server_fd);
        }
    }
}

void ProxyServer::handleRequest(int client_socket) {
    bool func_result = true;
    Client& client = clients[client_socket];

    client.buffer_size = SocketManager::recvMessage(client_socket, client.buffer, BUFFER_SIZE);
    if (client.buffer_size <= 0) {
        func_result = false;
    }

    if (func_result && !client.authenticated) {
        func_result = sendMessage(client.sql_server_fd, client.buffer, client.buffer_size);

        if (func_result) {
            client.buffer_size = SocketManager::recvMessage(client.sql_server_fd, client.buffer, BUFFER_SIZE);
            if (client.buffer_size <= 0) {
                func_result = false;
            }
        }
        if (func_result) {
            func_result =
                sendMessage(client_socket, client.buffer, client.buffer_size);  // Send AuthenticationOk
            client.authenticated = true;
        }
    } else if (func_result) {
        if (!logSQLToFile(client.buffer, client.buffer_size)) {
            func_result = false;
        }
        if (!forwardToServer(client_socket, client)) {
            func_result = false;
        }
    }

    if (!func_result) {
        closeSocket(client_socket);
    }
}

bool ProxyServer::sendMessage(int socket_fd, char* message, size_t len) {
    bool result = true;
    if (SocketManager::sendMessage(socket_fd, message, len) == -1) {
        std::cout << "1\n";
        closeSocket(socket_fd);
    }
    return result;
}

bool ProxyServer::checkClientMessage(int client_socket, Client& client) {
    bool func_result = true;
    if (client.buffer[0] == 'E') {
        func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
        if (func_result) {
            client.buffer_size = SocketManager::recvMessage(client.sql_server_fd, client.buffer, BUFFER_SIZE);
            if (client.buffer_size > 0) {
                func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
            } else {
                func_result = false;
            }
        }
    } else {
        func_result = sendMessage(client_socket, client.buffer, client.buffer_size);
    }
    return func_result;
}

bool ProxyServer::forwardToServer(int client_socket, Client& client) {
    bool func_result = true;
    func_result = sendMessage(client.sql_server_fd, client.buffer, client.buffer_size);

    if (func_result) {
        client.buffer_size = SocketManager::recvMessage(client.sql_server_fd, client.buffer, BUFFER_SIZE);
        if (client.buffer_size > 0) {
            func_result = checkClientMessage(client_socket, client);
        } else {
            func_result = false;
        }
    } else {
        func_result = false;
    }
    return func_result;
}

void ProxyServer::closeSocket(int socket_fd) {
    std::cout << "Client has disconnected: " << socket_fd << std::endl;
    close(socket_fd);
    clients.erase(socket_fd);
}

bool ProxyServer::logSQLToFile(const char* message, size_t len) {
    return _logger.write_log(_packetParser.parsePacket(message, len));
}

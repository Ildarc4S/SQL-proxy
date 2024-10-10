#include "SocketManager.hpp"

int SocketManager::setSocketOption(int server_socket_fd, int level, int optname) {
    int opt = 1;
    int result = setsockopt(server_socket_fd, level, optname, &opt, sizeof(opt));
    if (result) {
        std::cerr << "Error setting socket parameters:" << std::strerror(errno) << std::endl;
    }
    return result;
}

void SocketManager::createConnectorAddress(struct sockaddr_in& address, int server_port) {
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);
}

int SocketManager::bindSocket(int server_fd, const sockaddr_in& address) {
    int result = bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    if (result < 0) {
        std::cerr << "Error binding the socket:" << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::listenSocket(int server_fd, int count) {
    int result = listen(server_fd, count);
    if (result < 0) {
        std::cerr << "Error listening to the socket: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::sendMessage(int server_fd, char* message, size_t len) {
    message[len] = '\0';
    int result = send(server_fd, message, len, 0);
    if (result == -1) {
        std::cerr << "Error sending the message: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::recvMessage(int server_fd, char* buffer, int buffer_size) {
    return recv(server_fd, buffer, buffer_size, 0);
}

int SocketManager::creatteSocket() {
    int result = socket(AF_INET, SOCK_STREAM, 0);
    if (result == -1) {
        std::cerr << "Error creating the socket: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::connectServer(int server_fd, std::string& server_ip, int server_port) {
    struct sockaddr_in server_addres;
    server_addres.sin_family = AF_INET;
    server_addres.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addres.sin_port = htons(server_port);

    int result = connect(server_fd, (struct sockaddr*)&server_addres, sizeof(server_addres));
    if (result == -1) {
        std::cerr << "Error connecting to the server: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::acceptConnection(int server_fd, struct sockaddr_in& address) {
    socklen_t addrlen = sizeof(address);

    int result = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (result == -1) {
        std::cerr << "Error in accepting a client: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::addSocketToEpoll(int socket_fd, int epoll_fd) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = socket_fd;

    int result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event);
    if (result == -1) {
        std::cerr << "Error adding socket to epoll: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::epollCreate() {
    int result = epoll_create1(0);
    if (result == -1) {
        std::cerr << "Error creating the epoll: " << std::strerror(errno) << std::endl;
    }
    return result;
}

int SocketManager::epollWait(int epoll_fd, std::vector<epoll_event>& events, int max_clients) {
    int result = epoll_wait(epoll_fd, events.data(), max_clients, -1);
    if (result == -1) {
        std::cerr << "Error epoll_wait: " << std::strerror(errno) << std::endl;
    }
    return result;
}
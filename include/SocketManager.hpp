#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class SocketManager {
   public:
    SocketManager() = default;
    static int setSocketOption(int server_socket_fd, int level, int optname);
    static int creatteSocket();
    static int connectServer(int server_fd, std::string& server_ip, int server_port);
    static int bindSocket(int server_fd, const sockaddr_in& address);
    static int listenSocket(int server_fd, int count);
    static int sendMessage(int server_fd, char* message, size_t len);
    static int recvMessage(int server_fd, char* buffer, int buffer_size);
    static int acceptConnection(int server_fd, struct sockaddr_in& address);
    static int addSocketToEpoll(int socket_fd, int epoll_fd);
    static int epollCreate();
    static int epollWait(int epoll_fd, std::vector<epoll_event>& events, int max_clients);
    static void createConnectorAddress(struct sockaddr_in& address, int server_port);
};
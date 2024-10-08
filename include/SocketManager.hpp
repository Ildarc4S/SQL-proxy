#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <vector>
#include <string>

class SocketManager {
public:
    SocketManager() = default;
    static int creatteSocket();
    static int connectServer(int server_fd, std::string& server_ip, int server_port);
    static int bindSocket(int server_fd, const sockaddr_in& address);
    static int listenSocket(int server_fd, int count);
    static int sendMessage(int server_fd, char* message, size_t len);
    static int recvMessage(int server_fd, char* buffer, int buffer_size);
    static int acceptConnection(int server_fd, struct sockaddr_in& address);
    static int addSocketToEpoll(int socket_fd, int epoll_fd);
    static int epollCreate();
    static int epollWait(int epoll_fd, std::vector<epoll_event>& events, int max_clients, bool flug);
    static void createConnectorAddress(struct sockaddr_in& address,int server_port);
};
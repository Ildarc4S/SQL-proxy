#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class SocketManager {
public:
    SocketManager() = default;
    int createSocket(int _domain, int _type, int _protocol);
    bool bindSocket(int _sockfd, const sockaddr_in& _address);
    bool listenSocket();
    int acceptConnection();
};
#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "SocketManager.hpp"

#include "PacketParser.hpp"
#include "Logger.hpp"

#define BUFFER_SIZE 2048
#define MAX_CLIENTS 1024

struct Client {
    int sockfd; // Client file descriptor
    int sql_server_fd; // SQL server file Descriptor
    char buffer[BUFFER_SIZE]; // Buffer for data
    int buffer_size; // Buffer size
    bool authenticated; // Authorization flag

};

class ProxyServer {
public:
    // constructor accepting parameters for configuring the proxy server
    ProxyServer(const std::string& server_ip, int server_port, 
                const std::string& sql_servre_ip, int sql_server_port, 
                const std::string& log_file_path);
    void start(); // run server
    ~ProxyServer(); // destruct all 
private:
    bool initializeSocket(); // method to initialize the socket
    void acceptClient(); // method to accept new clients
    void handleRequest(int client_socket); // method to handle requests from clients and wrtie logs
    bool sendMessage(int socket_fd, char* message, size_t len); // method to send a message to socket
    bool forwardToServer(int client_socket_fd, Client& client); // method to forward data from the client to the SQL server
    bool checkClientMessage(int client_socket, Client& client); // method to check messages from the client
    void closeSocket(int socket_fd); // method to close socket
    bool logSQLToFile(const char* message, size_t len); // method to log SQL queries to file
    void addSocketToEpoll(int socket_fd); // method to add socket to epoll for event tracking
    static void signalHandler(int signal); // method to check signals
    static bool is_running; // end the program at the signal

    std::string _proxy_server_ip;
    int _proxy_server_port;

    std::string _sql_server_ip;
    int _sql_server_port;

    int _server_socket_fd;
    int _epoll_fd;

    std::map<int, Client> clients; // map to store clients

    PacketParser _packetParser;
    Logger _logger;

};

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>
#include <map>

#define PROXY_PORT 5005 // Порт прокси-сервера
#define PG_PORT 5432 // Порт PostgreSQL-сервера
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

using namespace std;

struct Client {
    int sockfd; // Файловый дескриптор сокета
    char buffer[BUFFER_SIZE]; // Буфер для данных
    int buffer_size; // Размер данных в буфере
    bool authenticated; // Флаг авторизации
    int pg_sockfd; // Файловый дескриптор сокета для PostgreSQL-сервера
};

// Карта для хранения клиентов
map<int, Client> clients;

// Функция для отправки сообщения
void send_message(int sockfd, const char *message, size_t len) {
    if (send(sockfd, message, len, 0) == -1) {
        cerr << "Ошибка отправки сообщения: " << strerror(errno) << endl;
        close(sockfd);
        clients.erase(sockfd); // Удаляем клиента из карты
    }
}

// Функция для обработки запроса
void handle_request(int sockfd) {
    Client &client = clients[sockfd];

    // Читаем данные от клиента
    client.buffer_size = recv(sockfd, client.buffer, BUFFER_SIZE, 0);
    if (client.buffer_size == -1) {
        cerr << "Ошибка приема данных: " << strerror(errno) << endl;
        close(sockfd);
        clients.erase(sockfd); // Удаляем клиента из карты
        return;
    } else if (client.buffer_size == 0) {
        // Клиент отключился
        close(sockfd);
        clients.erase(sockfd); // Удаляем клиента из карты
        return;
    }

    // Проверяем, авторизован ли клиент
    if (!client.authenticated) {
            send_message(client.pg_sockfd, client.buffer, client.buffer_size);

            // Получаем ответ от PostgreSQL-сервера
            client.buffer_size = recv(client.pg_sockfd, client.buffer, BUFFER_SIZE, 0);
            if (client.buffer_size == -1) {
                cerr << "Ошибка приема данных от PostgreSQL-сервера: " << strerror(errno) << endl;
                close(sockfd);
                clients.erase(sockfd); // Удаляем клиента из карты
                return;
            } 
            else if (client.buffer_size == 0) {
                cerr << "11PostgreSQL-сервер отключился." << endl;
                close(sockfd);
                clients.erase(sockfd); // Удаляем клиента из карты
                return;
            }

                // Отправка AuthenticationOk
                send_message(sockfd, client.buffer, client.buffer_size);
                client.authenticated = true;
            
    } else {
        // Перенаправление запросов после авторизации
        
        send_message(client.pg_sockfd, client.buffer, client.buffer_size);
        client.buffer_size = recv(client.pg_sockfd, client.buffer, BUFFER_SIZE, 0);
        if (client.buffer_size > 0) {
            send_message(sockfd, client.buffer, client.buffer_size);
        } 
        else {
            cerr << "22PostgreSQL-сервер отключился." << endl;
            close(sockfd);
            close(client.pg_sockfd); // Закрываем сокет PostgreSQL
            clients.erase(sockfd); // Удаляем клиента из карты
        }
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int epoll_fd;
    struct epoll_event event;

    // Создание сокета для прокси-сервера
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Включение опции SO_REUSEADDR
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Заполнение структуры адреса для прокси-сервера
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PROXY_PORT);

    // Привязка сокета к адресу
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Прослушивание соединения
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Создание epoll-файлового дескриптора
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    // Добавление серверного сокета в epoll
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    cout << "Прокси-сервер запущен на порту " << PROXY_PORT << endl;

    // Цикл обработки событий
    while (true) {
        vector<epoll_event> events(MAX_CLIENTS);
        int ev_count = epoll_wait(epoll_fd, events.data(), MAX_CLIENTS, -1);
        if (ev_count == -1) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < ev_count; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == server_fd) {
                    // Новое соединение
                    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    if (client_fd == -1) {
                        perror("accept failed");
                        continue;
                    }

                    // Создание сокета для PostgreSQL-сервера
                    int pg_sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (pg_sockfd == -1) {
                        perror("socket failed");
                        close(client_fd);
                        continue;
                    }

                    // Подключение к PostgreSQL-серверу
                    struct sockaddr_in pg_address;
                    pg_address.sin_family = AF_INET;
                    pg_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Замените на IP-адрес PostgreSQL-сервера
                    pg_address.sin_port = htons(PG_PORT);
                    if (connect(pg_sockfd, (struct sockaddr *)&pg_address, sizeof(pg_address)) == -1) {
                        perror("connect failed");
                        close(client_fd);
                        close(pg_sockfd);
                        continue;
                    }

                    // Добавление нового клиента в карту
                    clients[client_fd] = {client_fd, "", 0, false, pg_sockfd};

                    // Добавление клиентского сокета в epoll
                    event.events = EPOLLIN;
                    event.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        perror("epoll_ctl failed");
                        exit(EXIT_FAILURE);
                    }

                    cout << "Новый клиент подключился: " << inet_ntoa(address.sin_addr) << endl;

                } else {
                    // Получение данных от клиента
                    handle_request(events[i].data.fd);
                }
            } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                // Обработка ошибок
                cerr << "Ошибка соединения: " << strerror(errno) << endl;
                close(events[i].data.fd);
                clients.erase(events[i].data.fd); // Удаляем клиента из карты
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    return 0;
}

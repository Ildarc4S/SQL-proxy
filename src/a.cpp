#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <thread>
#include <vector>
#include <random>
#include <chrono>

const int PORT = 5011; // Стандартный порт PostgreSQL
const char* HOST = "127.0.0.1"; // Адрес хоста (локальный)
const int NUM_THREADS = 10; // Количество потоков
const int REQUESTS_PER_THREAD = 1000; // Запросов на поток

// Функция для отправки запроса к PostgreSQL
void sendRequest(int sockfd, const std::string& query) {
    send(sockfd, query.c_str(), query.size(), 0);
    char buffer[1024];
    recv(sockfd, buffer, sizeof(buffer), 0);
}

// Функция для запуска потока
void threadFunction(int threadId) {
    // Создание сокета
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка: Не удалось создать сокет." << std::endl;
        return;
    }

    // Настройка адреса сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Разрешение имени хоста
    struct hostent* server = gethostbyname(HOST);
    if (server == NULL) {
        std::cerr << "Ошибка: Не удалось разрешить имя хоста." << std::endl;
        close(sockfd);
        return;
    }
    memcpy((char*)&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Подключение к серверу
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Ошибка: Не удалось подключиться к серверу." << std::endl;
        close(sockfd);
        return;
    }

    // Генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 100);

    // Отправка запросов
    for (int i = 0; i < REQUESTS_PER_THREAD; ++i) {
        // Генерируем случайный запрос
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(distrib(gen)) + ";";
        sendRequest(sockfd, query);
    }

    // Закрытие соединения
    close(sockfd);
}

int main() {
    // Запускаем потоки
    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread(threadFunction, i));
    }

    // Ожидаем завершения потоков
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Завершено за: " << duration.count() << " миллисекунд" << std::endl;

    return 0;
}

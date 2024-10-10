# SQL Proxy Server  

SQL Proxy Server — это прокси-сервер для PostgreSQL, который перехватывает и логирует SQL-запросы от клиентов. Он позволяет управлять подключениями к базе данных, обрабатывать несколько клиентов одновременно и предоставляет возможность анализа выполненных запросов.

**Основные возможности:**
* Перехват и логирование SQL-запросов в текстовый файл (один запрос на строку).
* Поддержка большого числа подключений без создания отдельного потока на каждое соединение.
* Управление подключением к базе данных PostgreSQL.
* Использование Bracket Socket API для обработки сетевых соединений.

## Структура проекта:

```text
.
├── bin
├── include
│   ├── Logger.hpp
│   ├── PacketParser.hpp
│   ├── ProxyServer.hpp
│   └── SocketManager.hpp
├── logs
│   └── sql.logs
├── src
│   ├── Logger.cpp
│   ├── PacketParser.cpp
│   ├── ProxyServer.cpp
│   ├── SocketManager.cpp
│   └── main.cpp
├── test
│   └── TestProxyServer.cpp
└── CMakeLists.txt
```
## Установка пакетов (Ubuntu, при необходимости)
1. **Установка PostgreSQL:**
    ```bash
    sudo apt-get update
    sudo apt-get install postgresql
    ```
2. **Установка PostgreSQL:**
    ```bash
    sudo apt-get install postgresql-client
    ```
3. **Установка компилятора GCC:**
    ```bash
    sudo apt-get install gcc
    ```
4. **Установка CMake:**
    ```bash
    sudo apt-get install cmake
    ```
## Установка и запуск прокси сервера

### Требования:
* PostgreSQL сервер
* Компилятор C++ (например, GCC)
* CMake (для сборки проекта)
* SSL должен быть отключен!  
[Установка пакетов](#установка-пакетов-ubuntu-при-необходимости)

### Установка:

1. **Клонирование репозитория:**
   ```bash
   git clone https://github.com/вашрепозиторий.git
   cd вашрепозиторий
    ```
2. **Сборка проекта:**
    ```bash 
    mkdir build 
    cd build 
    cmake .. 
    make
    ```
3. **Запуск SQL прокси сервера:**  
    ```bash 
    ./../bin/ProxyServer <proxy_server_host> <proxy_server_port> <sql_server_host> <sql_server_port> <log_file_path>
    ```
    * `<proxy_server_host>` - адрес прокси-сервера (например, 127.0.0.1)
    * `<proxy_server_port>` - порт прокси-сервера (например, 5432)
    * `<sql_server_host>` - адрес PostgreSQL-сервера (например, 127.0.0.1)
    * `<sql_server_port>` - порт PostgreSQL-сервера (например, 5432)
    * `<log_file_path>` - путь к файлу для логирования запросов (например, `../logs/sql.log`)

## Тестирование

**Тестовая программа: Включает систему для проверки:**
* Корректности перехвата и логирования SQL-запросов.
* Функциональности управления подключением к базе данных.
* Нагрузочного тестирования прокси-сервера:  
Позволяет оценить производительность под нагрузкой в течение заданного времени, используя многопоточность для имитации работы нескольких клиентов одновременно.

### Требования:
* Компилятор C++ (например, GCC)
* CMake (для сборки проекта)
* Установлен psql!  
[Установка пакетов](#установка-пакетов-ubuntu-при-необходимости)

1. **Сборка теста:**
    ```bash
    bash make test
    ```
2. **Запуск теста:**
    ```bash 
    ./../bin/TestProxyServer <db_name> <db_user> <db_password> <db_host> <db_port> <db_query> <time_seconds> <clients_count>
    ```
    **Аргументы для запуска теста:**
   * `<db_name>` - имя базы данных PostgreSQL
   * `<db_user>` - имя пользователя базы данных
   * `<db_password>` - пароль пользователя базы данных
   * `<db_host>` - адрес PostgreSQL-сервера (например, 127.0.0.1)
   * `<db_port>` - порт PostgreSQL-сервера (например, 5432)
   * `<db_query>` - SQL-запрос, который будет отправлен на сервер (например, SELECT * FROM users)
   * `<time_seconds>` - продолжительность теста в секундах (например, 10)
   * `<clients_count>` - количество виртуальных клиентов, которые будут выполнять запросы (например, 5)


## 5. Пример использования с пояснениями
### Пример использования:
```bash 
./../bin/testproxyserver mydatabase myuser my_password 127.0.0.1 5432 "SELECT * FROM users" 10 5
```


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
2. **Установка PostgreSQL клиента:**
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
5. **Установка Valgrind**
    ```bash
    sudo apt-get install valgrind
    ```

## Установка и запуск прокси-сервера

### Требования:
* PostgreSQL сервер
* Компилятор C++ (например, GCC)
* CMake (для сборки проекта)
* SSL должен быть отключен!  
[Установка пакетов](#установка-пакетов-ubuntu-при-необходимости)

### Установка:

1. **Клонирование репозитория:**
   ```bash
   git clone [https://github.com/вашрепозиторий.git](https://github.com/Ildarc4S/SQL-proxy.git)
   cd SQL-proxy
    ```
2. **Сборка проекта:**
    ```bash 
    mkdir build 
    cd build 
    cmake .. 
    make
    ```
3. **Запуск SQL прокси-сервера:**  
    ```bash 
    ./../bin/ProxyServer <proxy_server_host> <proxy_server_port> <sql_server_host> <sql_server_port> <log_file_path>
    ```
    * `<proxy_server_host>` - адрес PostgreSQL прокси-сервера (например, 127.0.0.1)
    * `<proxy_server_port>` - порт PostgreSQL прокси-сервера (например, 5432)
    * `<sql_server_host>` - адрес PostgreSQL-сервера (например, 127.0.0.1)
    * `<sql_server_port>` - порт PostgreSQL-сервера (например, 5432)
    * `<log_file_path>` - путь к файлу относительно корневой директории проекта для логирования запросов  (например, `logs/sql.log`)
4. **Отключение SQL прокси-сервера:**  
## Тестирование

**Тестовая программа: Включает систему для проверки:**
* Корректности перехвата и логирования SQL-запросов.
* Функциональности управления подключением к базе данных.
* Нагрузочного тестирования прокси-сервера:  
Позволяет оценить производительность под нагрузкой в течение заданного времени, используя многопоточность для имитации работы нескольких клиентов одновременно.
(sysbench зависает, поэтому решил свой вариант теста сделать)

### Требования:
* Компилятор C++ (например, GCC)
* CMake (для сборки проекта)
* Установлен psql клиент  
[Установка пакетов](#установка-пакетов-ubuntu-при-необходимости)

1. **Сборка теста:**
    ```bash
    make test
    ```
2. **Запуск теста:**
    ```bash 
    ./../bin/TestProxyServer <db_name> <db_user> <db_password> <db_host> <db_port> <db_query> <clients_count> <operations_count>
    ```
    **Аргументы для запуска теста:**
   * `<db_name>` - имя базы данных PostgreSQL
   * `<db_user>` - имя пользователя базы данных
   * `<db_password>` - пароль пользователя базы данных
   * `<db_host>` - адрес PostgreSQL прокси-сервера (например, 127.0.0.1)
   * `<db_port>` - порт PostgreSQL прокси-сервера (например, 5112)
   * `<db_query>` - SQL-запрос, который будет отправлен на сервер (например, SELECT * FROM users)
   * `<clients_count>` - количество виртуальных клиентов, которые будут выполнять запросы (например, 5)
    * `<operations_count>` - количество выполняемых запросов одним клиентом.

Так же можете воспользоваться psql клиентом для подключения и проверки работы прокси сервера:
```bash
psql "host=proxy_host port=proxy_port user=your_user dbname=your_db sslmode=disable"
```
* `proxy_host` - адрес PostgreSQL-сервера.
* `proxy_port` - порт PostgreSQL прокси-сервера.
* `your_user` - имя пользователя базы данных.
* `your_db` - имя базы данных PostgreSQL.
* `disable` - отключение SSL.

**Для проверки утечек памяти воспользуйтесь valgrind**
```bash
valgrind ./../bin/TestProxyServer <db_name> <db_user> <db_password> <db_host> <db_port> <db_query> <time_seconds> <clients_count>
```

##  Пример использования
### 1. Создание пользователя и базы данных для тестирования
Перед запуском прокси-сервера создайте пользователя и базу данных, которые будете использовать для тестирования.
1. Подключитесь к серверу PostgreSQL с помощью psql:
    ```bash
    sudo -u postgres psql
    ```
2. Создадим тестовую базу данных и тестового пользователя::
    ```bash
    CREATE USER test_user WITH PASSWORD 'test_password';
    CREATE DATABASE test_db;
    GRANT ALL PRIVILEGES ON DATABASE test_db TO test_user;
    ```
3. Создайте таблицу для тестирования и записей в нее:
    ```bash
    \c test_db
    CREATE TABLE users (
        id SERIAL PRIMARY KEY,
        name VARCHAR(100),
        email VARCHAR(100)
    ); 
    INSERT INTO users (name, email) VALUES ('Alice', 'alice@example.com');
    INSERT INTO users (name, email) VALUES ('Bob', 'bob@example.com');
    ```
4. Выход из psql:
    ```bash
    \q
    ```
### 2. Запуск прокси-сервера
Запустите SQL Proxy-Server после [сборки проекта](#установка), указав необходимые параметры:  

```bash 
./../bin/ProxyServer 127.0.0.1 5002 127.0.0.1 5432 logs/sql.log
```
* `127.0.0.1` - адрес прокси-сервера.
* `50022` - порт прокси-сервера.
* `127.0.0.1` - адрес PostgreSQL-сервера.
* `5432` - порт PostgreSQL-сервера.
* `logs/sql.log` - путь к файлу для логирования запросов.

### 3. Использование тестирующей программы
После [сборки тестовой программы](#тестирование) можете запустить её для проверки функциональности прокси-сервера:
```bash
./../bin/TestProxyServer test_db test_user test_password 127.0.0.1 5002 "SELECT * FROM users" 10 5
```
* `test_db` - имя базы данных PostgreSQL.
* `test_user` - имя пользователя базы данных.
* `test_password` - пароль пользователя базы данных.
* `127.0.0.1` - адрес PostgreSQL-сервера.
* `5002` - порт PostgreSQL прокси-сервера.
* `"SELECT * FROM users"` - SQL-запрос, который будет отправлен на сервер.
* `5` - количество виртуальных клиентов, которые будут выполнять запросы.
* `10` - количество выполняемых запросов одним клиентом.

### 4. Тестирование через подключение клиента с помощью psql
Вы можете протестировать подключение к прокси-серверу PostgreSQL с помощью клиента psql:
```bash
psql "host=127.0.0.1 port=5002 user=test_user dbname=test_db sslmode=disable"
```
* `127.0.0.1` - адрес PostgreSQL-сервера.
* `5002` - порт PostgreSQL прокси-сервера.
* `test_user` - имя пользователя базы данных.
* `test_db` - имя базы данных PostgreSQL.
* `disable` - отключение SSL.

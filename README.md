# SQL Proxy Server

## Описание

SQL Proxy Server — это прокси-сервер для PostgreSQL, который перехватывает и логирует SQL-запросы от клиентов. Он позволяет управлять подключениями к базе данных и предоставляет возможность анализа выполненных запросов.

## Структура проекта
```bash
.
├── CMakeLists.txt
├── README.md
├── bin
│   └── ProxyServer
├── include
│   ├── Logger.hpp
│   ├── PacketParser.hpp
│   ├── ProxyServer.hpp
│   └── SocketManager.hpp
├── logs
│   └── test.txt
├── src
│   ├── Logger.cpp
│   ├── PacketParser.cpp
│   ├── ProxyServer
│   ├── ProxyServer.cpp
│   ├── SocketManager.cpp
│   └── main.cpp
└── test
    └── test_proxy_server.cpp
```

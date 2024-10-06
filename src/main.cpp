#include "../include/ProxyServer.hpp"

int main() {
    ProxyServer proxy("127.0.0.1", 5011);
    proxy.start();
    return 0;
}
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::atomic<int> successful_clients{0};  // successful client counter

void runClient(int client_id, int time, const std::string& db_name, const std::string& db_user,
               const std::string& db_password, const std::string& db_host, const std::string& db_port,
               const std::string& query) {
    bool success = true;
    try {
        std::cout << "Client " << client_id << " connected to database." << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(time);

        while (std::chrono::steady_clock::now() < end_time) {
            std::string command = "psql \"host=" + db_host + " port=" + db_port + " user=" + db_user +
                                  " dbname=" + db_name + " password=" + db_password +
                                  " sslmode=disable\" -c \"" + query + "\" ";

            int result = system(command.c_str());
            if (result == 0) {
                std::cout << "Client " << client_id << " executed query successfully." << std::endl;
            } else {
                std::cerr << "Client " << client_id << " failed to execute query." << std::endl;
                success = false;
            }
        }
        if (success) {
            successful_clients++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Client " << client_id << " error: " << e.what() << std::endl;
    }
}

bool isValidIPAddress(const std::string& ip) {
    bool correct_address = true;
    std::regex ip_regex("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$");
    if (!std::regex_match(ip, ip_regex)) {
        correct_address = false;
    }

    std::stringstream ss(ip);
    std::string part_ip;
    while (std::getline(ss, part_ip, '.')) {
        int num = std::stoi(part_ip);
        if (num < 0 || num > 255) {
            correct_address = false;
        }
    }
    return correct_address;
}

int main(int argc, char* argv[]) {
    if (argc != 9) {
        std::cerr << "Usage: " << argv[0]
                  << " <db_name> <db_user> <db_password> <db_host> <db_port> <db_query> <time_seconds> "
                     "<clients_count>"
                  << std::endl;
    } else {
        std::string db_name = argv[1];
        std::string db_user = argv[2];
        std::string db_password = argv[3];
        std::string db_host = argv[4];
        std::string db_port = argv[5];
        std::string db_query = argv[6];
        bool continue_programm = true;

        int time = std::stoi(argv[7]);
        if (time <= 0) {
            std::cerr << "<time_seconds> must be a positive integer." << std::endl;
            continue_programm = false;
        }

        int num_clients = atoi(argv[8]);
        if (num_clients <= 0) {
            std::cerr << "<clients_count> must be a positive integer." << std::endl;
            continue_programm = false;
        }

        if (!isValidIPAddress(db_host)) {
            std::cerr << "<db_host> is not a valid IP address." << std::endl;
            continue_programm = false;
        }

        if (continue_programm) {
            std::vector<std::thread> clients;
            for (int i = 0; i < num_clients; ++i) {
                clients.emplace_back(runClient, i, time, db_name, db_user, db_password, db_host, db_port,
                                     db_query);
            }

            for (auto& client : clients) {
                client.join();
            }

            // Output the total number of successful clients
            std::cout << "Total successful clients: " << successful_clients.load() << " out of "
                      << num_clients << std::endl;
        }
    }
    return 0;
}
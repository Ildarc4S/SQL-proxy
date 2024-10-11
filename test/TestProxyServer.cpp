#include <atomic>
#include <cstdlib>
#include <future>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::atomic<int> successful_clients{0};

void runClient(int client_id, int operations_count, const std::string& db_name, const std::string& db_user,
               const std::string& db_password, const std::string& db_host, const std::string& db_port,
               const std::string& query) {
    bool success = false;
    try {
        std::cout << "Client " << client_id << " connected to database." << std::endl;

        for (int i = 0; i < operations_count; ++i) {
            std::string command = "psql \"host=" + db_host + " port=" + db_port + " user=" + db_user +
                                  " dbname=" + db_name + " password=" + db_password +
                                  " sslmode=disable\" -c \"" + query + "\" ";

            auto command_start_time = std::chrono::steady_clock::now();
            system(command.c_str());
            auto command_end_time = std::chrono::steady_clock::now();

            if ((command_end_time - command_start_time) <=
                std::chrono::seconds(200)) {  // 200 - time to execute the command
                std::cout << "Client " << client_id << " executed query successfully!" << std::endl;
                success = true;
            } else {
                std::cerr << "Client " << client_id << " failed to execute query!" << std::endl;
            }
        }
        if (success) {
            successful_clients++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Client " << client_id << " error: " << e.what() << "!" << std::endl;
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
                  << " <db_name> <db_user> <db_password> <db_host> <db_port> <db_query> <clients_count> "
                     "<operations_count>"
                  << std::endl;
    } else {
        bool correct_input = true;
        std::string db_name = argv[1];
        std::string db_user = argv[2];
        std::string db_password = argv[3];
        std::string db_host = argv[4];
        std::string db_port = argv[5];
        std::string db_query = argv[6];

        int num_clients = atoi(argv[7]);
        if (num_clients <= 0) {
            std::cerr << "<clients_count> must be a positive integer." << std::endl;
            correct_input = false;
        }

        int operations_count = atoi(argv[8]);
        if (operations_count <= 0) {
            std::cerr << "<operations_count> must be a positive integer." << std::endl;
            correct_input = false;
        }

        if (!isValidIPAddress(db_host)) {
            std::cerr << "<db_host> is not a valid IP address." << std::endl;
            correct_input = false;
        }

        if (correct_input) {
            std::vector<std::future<void>> futures;
            for (int i = 0; i < num_clients; ++i) {
                futures.emplace_back(std::async(std::launch::async, runClient, i, operations_count, db_name,
                                                db_user, db_password, db_host, db_port, db_query));
            }

            for (auto& future : futures) {
                future.wait();
            }

            std::cout << "Total successful clients: " << successful_clients.load() << " out of "
                      << num_clients << std::endl;
        }
    }
    return 0;
}
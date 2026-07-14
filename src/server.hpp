#pragma once

#include <string>
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

class Server {
public:
    Server(const std::string& host, int port);
    ~Server();

    // Start the TCP server and enter connection loop
    void start();

    // Stop the TCP server and close socket connections
    void stop();

private:
    std::string host_;
    int port_;
    int server_fd_;
    std::atomic<bool> is_running_;
    std::vector<std::thread> client_threads_;
    std::mutex threads_mutex_;

    // Handle an individual client connection
    void handle_client(int client_fd);
};

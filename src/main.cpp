#include "server.hpp"
#include <iostream>
#include <csignal>
#include <memory>

// Global pointer to the server instance for signal handling
std::unique_ptr<Server> global_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Server] Shutdown signal received. Stopping server..." << std::endl;
        if (global_server) {
            global_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Default host and port
    std::string host = "127.0.0.1";
    int port = 6379;

    // Quick command-line arguments parse (e.g. ./gedis-server <port>)
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
            return 1;
        }
    }

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "[Gedis] Initializing Redis Clone (Gedis)..." << std::endl;
    global_server = std::make_unique<Server>(host, port);
    global_server->start();

    std::cout << "[Gedis] Exiting server cleanly. Goodbye!" << std::endl;
    return 0;
}

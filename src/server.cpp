#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

Server::Server(const std::string& host, int port)
    : host_(host), port_(port), server_fd_(-1), is_running_(false) {}

Server::~Server() {
    stop();
}

void Server::start() {
    // 1. Create a socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[Error] Failed to create socket" << std::endl;
        return;
    }

    // 2. Allow immediate reuse of port
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[Error] setsockopt SO_REUSEADDR failed" << std::endl;
        close(server_fd_);
        return;
    }

    // 3. Bind to specified address & port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) <= 0) {
        std::cerr << "[Error] Invalid address or address not supported: " << host_ << std::endl;
        close(server_fd_);
        return;
    }

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[Error] Bind failed on " << host_ << ":" << port_ << std::endl;
        close(server_fd_);
        return;
    }

    // 4. Start listening for incoming connections
    if (listen(server_fd_, SOMAXCONN) < 0) {
        std::cerr << "[Error] Listen failed" << std::endl;
        close(server_fd_);
        return;
    }

    is_running_ = true;
    std::cout << "[Server] Listening on " << host_ << ":" << port_ << "..." << std::endl;

    // 5. Accept loop
    while (is_running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (is_running_) {
                std::cerr << "[Error] Accept failed" << std::endl;
            }
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        std::cout << "[Server] Connection accepted from " << client_ip << ":" << client_port << std::endl;

        // Spawn a thread to handle the connected client concurrently
        std::thread client_thread(&Server::handle_client, this, client_fd);
        client_thread.detach(); // Detach to let it run independently
    }
}

void Server::stop() {
    if (is_running_) {
        is_running_ = false;
        close(server_fd_);
        server_fd_ = -1;
        std::cout << "[Server] Stopped listening." << std::endl;
    }
}

void Server::handle_client(int client_fd) {
    char buffer[1024];
    while (is_running_) {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read < 0) {
            std::cerr << "[Server] Read error on client socket " << client_fd << std::endl;
            break;
        } else if (bytes_read == 0) {
            std::cout << "[Server] Client disconnected (socket " << client_fd << ")" << std::endl;
            break;
        }

        std::cout << "[Server] Received raw command from socket " << client_fd << ": " << buffer;

        // Milestone 2 Dummy reply: echo back pong for now.
        std::string reply = "+PONG\r\n";
        send(client_fd, reply.c_str(), reply.length(), 0);
    }
    close(client_fd);
}

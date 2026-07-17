#include "server.hpp"
#include "resp.hpp"
#include <iostream>
#include <algorithm>
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
    std::string input_buffer;

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

        // Append newly received bytes to the client stream buffer
        input_buffer.append(buffer, bytes_read);

        // Process all complete RESP commands in the buffer
        while (true) {
            auto [obj, consumed] = parse_resp(input_buffer);
            if (consumed == 0) {
                break; // Incomplete command, wait for more data
            }

            // Route the parsed command and get the response
            std::string reply = handle_command(obj);
            send(client_fd, reply.c_str(), reply.length(), 0);

            // Erase processed bytes from buffer
            input_buffer.erase(0, consumed);
        }
    }
    close(client_fd);
}

std::string Server::handle_command(const RespObject& command) {
    if (command.type != RespType::Array) {
        return make_error("ERR Protocol error: expected array of commands").serialize();
    }

    if (command.array_val.empty()) {
        return make_error("ERR Protocol error: empty command array").serialize();
    }

    // Extract command name (first element of array)
    const auto& cmd_name_obj = command.array_val[0];
    if (cmd_name_obj.type != RespType::BulkString) {
        return make_error("ERR Protocol error: command name must be a bulk string").serialize();
    }

    // Convert command name to uppercase
    std::string cmd_name = cmd_name_obj.str_val;
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::toupper);

    if (cmd_name == "PING") {
        // If PING has an argument, return it as a bulk string. Otherwise return simple string "+PONG\r\n".
        if (command.array_val.size() > 1) {
            const auto& arg = command.array_val[1];
            if (arg.type == RespType::BulkString) {
                return make_bulk_string(arg.str_val).serialize();
            } else {
                return make_bulk_string("").serialize();
            }
        }
        return make_simple_string("PONG").serialize();
    } else if (cmd_name == "ECHO") {
        if (command.array_val.size() < 2) {
            return make_error("ERR wrong number of arguments for 'echo' command").serialize();
        }
        const auto& arg = command.array_val[1];
        if (arg.type != RespType::BulkString) {
            return make_error("ERR echo argument must be a bulk string").serialize();
        }
        return make_bulk_string(arg.str_val).serialize();
    } else if (cmd_name == "SET") {
        if (command.array_val.size() < 3) {
            return make_error("ERR wrong number of arguments for 'set' command").serialize();
        }
        const auto& key_obj = command.array_val[1];
        const auto& val_obj = command.array_val[2];
        if (key_obj.type != RespType::BulkString || val_obj.type != RespType::BulkString) {
            return make_error("ERR set arguments must be bulk strings").serialize();
        }

        store_.set(key_obj.str_val, val_obj.str_val);
        return make_simple_string("OK").serialize();
    } else if (cmd_name == "GET") {
        if (command.array_val.size() < 2) {
            return make_error("ERR wrong number of arguments for 'get' command").serialize();
        }
        const auto& key_obj = command.array_val[1];
        if (key_obj.type != RespType::BulkString) {
            return make_error("ERR get argument must be a bulk string").serialize();
        }

        std::string value;
        if (store_.get(key_obj.str_val, value)) {
            return make_bulk_string(value).serialize();
        }
        return make_null().serialize();
    } else if (cmd_name == "DEL") {
        if (command.array_val.size() < 2) {
            return make_error("ERR wrong number of arguments for 'del' command").serialize();
        }
        long long deleted_count = 0;
        for (size_t i = 1; i < command.array_val.size(); ++i) {
            const auto& key_obj = command.array_val[i];
            if (key_obj.type == RespType::BulkString) {
                if (store_.del(key_obj.str_val)) {
                    deleted_count++;
                }
            }
        }
        return make_integer(deleted_count).serialize();
    } else if (cmd_name == "EXISTS") {
        if (command.array_val.size() < 2) {
            return make_error("ERR wrong number of arguments for 'exists' command").serialize();
        }
        long long exists_count = 0;
        for (size_t i = 1; i < command.array_val.size(); ++i) {
            const auto& key_obj = command.array_val[i];
            if (key_obj.type == RespType::BulkString) {
                if (store_.exists(key_obj.str_val)) {
                    exists_count++;
                }
            }
        }
        return make_integer(exists_count).serialize();
    }

    return make_error("ERR unknown command '" + cmd_name_obj.str_val + "'").serialize();
}

#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <memory>
#include <mutex>
#include <map>
#include <algorithm>

using asio::ip::tcp;

// Global variables
inline std::vector<std::shared_ptr<tcp::socket>> clients;
inline std::mutex clients_mutex;
inline std::map<std::shared_ptr<tcp::socket>, std::string> client_names;

// Function declarations
void run_server(unsigned short port);
void run_client(const std::string& host, unsigned short port);
void broadcast(const std::string& message);
void handle_client(std::shared_ptr<tcp::socket> client);

// Broadcast message to all clients
inline void broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        try {
            asio::write(*client, asio::buffer(message + "\n"));
        }
        catch (...) {
            // Ignore write errors
        }
    }
}

// Handle individual client
inline void handle_client(std::shared_ptr<tcp::socket> client) {
    try {
        asio::streambuf buf;

        // First, read username
        asio::read_until(*client, buf, "\n");
        std::istream is(&buf);
        std::string username;
        std::getline(is, username);
        if (username.empty()) username = "Unknown";

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client_names[client] = username;
        }

        broadcast(username + " has joined the chat.");

        while (true) {
            asio::read_until(*client, buf, "\n");
            std::string message;
            std::getline(is, message);
            if (message.empty()) continue;

            std::string formatted = username + ": " + message;
            std::cout << formatted << std::endl;
            broadcast(formatted);
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
        client_names.erase(client);
        std::cout << "A client disconnected\n";
        broadcast("A user has left the chat.");
    }
}

// Run the server
inline void run_server(unsigned short port) {
    try {
        asio::io_context io;

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
        std::cout << "Server running on port " << port << std::endl;

        while (true) {
            auto socket = std::make_shared<tcp::socket>(io);
            acceptor.accept(*socket);

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.push_back(socket);
            }

            std::thread(handle_client, socket).detach();
            std::cout << "New client connected\n";
        }
    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

// Run the client
inline void run_client(const std::string& host, unsigned short port) {
    try {
        asio::io_context io;

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        tcp::socket socket(io);
        asio::connect(socket, endpoints);

        std::cout << "Connected to server " << host << ":" << port << std::endl;

        // Ask for username
        std::cout << "Enter your username: ";
        std::string username;
        std::getline(std::cin, username);
        if (username.empty()) username = "Unknown";
        asio::write(socket, asio::buffer(username + "\n"));

        // Thread to read messages from server
        std::thread reader([&socket]() {
            try {
                asio::streambuf buf;
                while (true) {
                    asio::read_until(socket, buf, "\n");
                    std::istream is(&buf);
                    std::string message;
                    std::getline(is, message);
                    if (!message.empty()) {
                        std::cout << message << std::endl;
                    }
                }
            }
            catch (...) {
                std::cout << "Disconnected from server." << std::endl;
            }
            });

        // Main thread sends messages to server
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty()) {
                asio::write(socket, asio::buffer(line + "\n"));
            }
        }

        reader.join();

    }
    catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

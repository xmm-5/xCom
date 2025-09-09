#include <iostream>
#include <string>
#include "chat.h" // include all chat logic

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
            << "Server: xCom server <port>\n"
            << "Client: xCom client <host> <port>\n";
        return 0;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        unsigned short port = (argc >= 3) ? std::stoi(argv[2]) : 12345;
        run_server(port);
    }
    else if (mode == "client") {
        if (argc < 4) {
            std::cout << "Usage: xCom client <host> <port>\n";
            return 0;
        }
        std::string host = argv[2];
        unsigned short port = std::stoi(argv[3]);
        run_client(host, port);
    }
    else {
        std::cout << "Unknown mode: " << mode << std::endl;
    }

    return 0;
}

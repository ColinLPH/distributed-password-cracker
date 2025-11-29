#include "parse_args.h"
#include "network.h"

int main(int argc, char* argv[]) {
    Args args;

    if(parse_args(argc, argv, args) != 0) {
        return -1;
    }
    print_args(args);

    try {
        auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in server_addr {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(args.server_port);

        if (inet_pton(AF_INET, args.serverIP.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid server IP address");
        }

        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Failed to connect to server");
        }

        /**
         * wait to receive CONACK
         * send WORKREQ
         * wait to receive WORK
         */

        std::cout << "Connected to server, waiting for CONACK.\n";
        std::vector<uint8_t> buffer;
        ssize_t ret = recv_full_packet(sockfd, buffer);
        if (ret <= 0) {
            throw std::runtime_error("Failed to receive CONACK packet");
        }
        Packet packet;
        if (deserialize(buffer.data(), buffer.size(), packet) != 0) {
            throw std::runtime_error("Failed to deserialize CONACK packet");
        }
        if (packet.header.flags != CONACK) {
            throw std::runtime_error("Expected CONACK packet");
        }
        std::cout << "Received CONACK from server.\n";
        std::cout << "Hash: " << std::string(packet.payload.begin(), packet.payload.end()) << "\n";

        close(sockfd);

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}

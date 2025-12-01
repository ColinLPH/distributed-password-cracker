#include <memory>
#include <atomic>

#include "parse_args.h"
#include "network.h"
#include "worker.h"

enum NODE_STATUS
{
    IDLE,
    READY,
    WORKING,
    CLOSING
};

int main(int argc, char *argv[])
{
    Args args;

    if (parse_args(argc, argv, args) != 0)
    {
        return -1;
    }
    print_args(args);

    try
    {
        auto sockfd = connect_to_server(args);
        std::cout << "Connected to server, waiting for CONACK.\n";

        auto password_found = std::make_shared<std::atomic<bool>>(false);
        auto work_done = std::make_shared<std::atomic<uint16_t>>(0);
        auto shared_hash_info = std::make_shared<hash_info>();

        while (!password_found->load(std::memory_order_relaxed))
        {
            std::vector<uint8_t> buffer;
            ssize_t ret = recv_full_packet(sockfd, buffer); // could do: add server timeout
            if (ret <= 0)
            {
                close(sockfd);
                throw std::runtime_error("Received error or connection closed");
            }
            Packet packet;
            if (deserialize(buffer.data(), buffer.size(), packet) != 0)
            {
                close(sockfd);
                throw std::runtime_error("Failed to deserialize packet");
            }

            switch (packet.header.flags)
            {
            case CONACK:
                std::cout << "Received CONACK from server.\n";
                *shared_hash_info = parse_hash_info(std::string(packet.payload.begin(), packet.payload.end()));
                print_hash_info(*shared_hash_info);
                if (send_workreq(sockfd, DEFAULT_RETRIES, args.threads) < 0)
                {
                    close(sockfd);
                    throw std::runtime_error("Failed to send WORKREQ to server");
                }
                std::cout << "Sent WORKREQ to server.\n";
                break;
            case WORK:
            {
                std::cout << "Received WORK packet from server.\n";

                // Convert payload bytes to string
                std::string payload_str(packet.payload.begin(), packet.payload.end());

                // Split by spaces into a vector of strings
                std::vector<std::string> prefixes;
                std::istringstream iss(payload_str);
                std::string token;
                while (iss >> token)
                {
                    prefixes.push_back(token);
                }
                
                std::cout << "Work size: " << static_cast<int>(packet.header.work_size) << "\n";
                std::cout << "Checkpoint interval: " << static_cast<int>(packet.header.checkpoint_interval) << "\n";
                // Debug: print prefixes
                std::cout << "Parsed " << prefixes.size() << " prefixes:\n";
                for (const auto &p : prefixes)
                {
                    std::cout << "[" << p << "] ";
                }
                std::cout << "\n";

                // Launch worker threads
                // std::vector<std::thread> thread_pool;
                // thread_pool.reserve(prefixes.size());
                // work_done->store(0, std::memory_order_relaxed);
                // for (int i = 0; i < prefixes.size(); ++i)
                // {
                //     thread_pool.emplace_back([&, i](){
                        
                //     });
                // }

                // for (auto &t : thread_pool)
                // {
                //     if (t.joinable())
                //         t.join();
                // }
                break;
            }
            case KILL:
                std::cout << "Received KILL packet from server. Exiting.\n";
                password_found->store(true, std::memory_order_relaxed);
                break;
            default:
                std::cout << "Received unexpected packet with flag: " << static_cast<int>(packet.header.flags) << "\n";
                break;
            }
        }

        close(sockfd);
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }
}

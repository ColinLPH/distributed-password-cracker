#include <iostream>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "parse_args.h"
#include "partition.h"
#include "network.h"

int main(int argc, char* argv[]) {
    Args args;

    if(parse_args(argc, argv, args) != 0)
    {
        return -1;
    }
    print_args(args);

    auto partitions = create_partitions(DEFAULT_PREFIX_LEN);
    bool password_found = false;

    try {
        auto listen_fd = create_listen_socket(args.port);
        std::cout << "Server listening on port " << args.port << "\n";

        auto epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw std::runtime_error("Error creating epoll instance");
            ::close(listen_fd);
        }

        epoll_event event{};
        event.data.fd = listen_fd;
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
            throw std::runtime_error("Error adding listen socket to epoll");
            ::close(listen_fd);
            ::close(epoll_fd);
        }

        std::vector<epoll_event> events(MAX_EPOLL_EVENTS);

        while (true)
        {
            int n = epoll_wait(epoll_fd, events.data(), MAX_EPOLL_EVENTS, -1);
            if (n < 0) {
                throw std::runtime_error("epoll_wait failed");
            }

            for (size_t i = 0; i < n; i++)
            {
                if (events[i].data.fd == listen_fd)
                {
                    // Handle new connection
                    while (true) {
                        sockaddr_in client_addr{};
                        socklen_t client_len = sizeof(client_addr);
                        int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                        if (client_fd == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break; // No more incoming connections
                            } else {
                                throw std::runtime_error("Error accepting connection");
                            }
                        }

                        std::cout << "Accepted connection from "
                                  << inet_ntoa(client_addr.sin_addr) << ":"
                                  << ntohs(client_addr.sin_port) << "\n";

                        // Make the client socket non-blocking
                        if (!make_fd_non_blocking(client_fd)) {
                            ::close(client_fd);
                            throw std::runtime_error("Error making client socket non-blocking");
                        }

                        // Add the new client socket to epoll
                        epoll_event client_event{};
                        client_event.data.fd = client_fd;
                        client_event.events = EPOLLIN | EPOLLET;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                            ::close(client_fd);
                            throw std::runtime_error("Error adding client socket to epoll");
                        }
                    }
                } else {
                    // handle data from client
                }
            }
        }


    } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

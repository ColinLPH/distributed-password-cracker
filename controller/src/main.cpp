#include <iostream>
#include <unistd.h>

#include "network.h"
#include "parse_args.h"
#include "partition.h"

int main(int argc, char *argv[])
{
    Args args;

    if (parse_args(argc, argv, args) != 0)
    {
        return -1;
    }
    print_args(args);

    auto partitions = create_partitions(DEFAULT_PREFIX_LEN);
    size_t part_index = 0;
    bool password_found = false;

    try
    {
        Fd listen_fd(create_listen_socket(args.port));
        if (!make_fd_non_blocking(listen_fd.get()))
        {
            throw std::runtime_error("Error making listen socket non-blocking");
        }

        std::vector<pollfd> pollfds;
        std::vector<Fd> client_fds;

        pollfd listen_entry{};
        listen_entry.fd = listen_fd.get();
        listen_entry.events = POLLIN;
        pollfds.push_back(listen_entry);

        std::cout << "Server listening on port " << args.port << "\n";

        while (!password_found)
        {

            int n = poll(pollfds.data(), pollfds.size(), -1);
            if (n < 0)
            {
                throw std::runtime_error("poll() failed");
            }

            for (size_t i = 0; i < pollfds.size(); i++)
            {
                auto &pfd = pollfds[i];

                // 1. Handle new incoming connections
                if (pfd.fd == listen_fd.get() && (pfd.revents & POLLIN))
                {

                    while (true)
                    {
                        sockaddr_in client_addr{};
                        socklen_t len = sizeof(client_addr);

                        int raw_fd =
                            accept(listen_fd.get(), (sockaddr *)&client_addr, &len);

                        if (raw_fd < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            throw std::runtime_error("Error accepting connection");
                        }

                        Fd client_fd(raw_fd);

                        if (!make_fd_non_blocking(client_fd.get()))
                        {
                            throw std::runtime_error("Error making client socket non-blocking");
                        }

                        std::cout << "Accepted connection from "
                                  << inet_ntoa(client_addr.sin_addr) << ":"
                                  << ntohs(client_addr.sin_port) << "\n";

                        pollfd entry{};
                        entry.fd = client_fd.get();
                        entry.events = POLLIN;
                        pollfds.push_back(entry);

                        client_fds.push_back(std::move(client_fd));

                        if (send_conack(entry.fd, DEFAULT_RETRIES, args) != 0)
                        {
                            std::cerr << "Failed to send CONACK to client (fd: " << entry.fd << ")\n";
                            ::close(entry.fd);
                            entry.fd = -1; // Mark for removal
                        }
                    }
                }

                // 2. Handle client data
                else if (pfd.fd != listen_fd.get() && (pfd.revents & POLLIN))
                {
                    std::vector<uint8_t> buffer;
                    ssize_t n = recv_full_packet(pfd.fd, buffer); // Use our robust helper
                    if (n <= 0)
                    {
                        std::cout << "Client disconnected (fd: " << pfd.fd << ")\n";
                        ::close(pfd.fd);
                        pfd.fd = -1; // Mark for removal
                        continue;
                    }

                    Packet pkt;
                    int rc = deserialize(buffer.data(), buffer.size(), pkt);
                    if (rc != 0)
                    {
                        std::cerr << "Failed to deserialize packet from client (fd: " << pfd.fd << "), error code: " << rc << "\n";
                        ::close(pfd.fd);
                        pfd.fd = -1;
                        continue;
                    }

                    // Handle the packet
                    switch (pkt.header.flags)
                    {
                    case WORKREQ:
                    {
                        // handle_WORKREQ(pkt, client_fd, partitions, password_found);
                        std::cout << "Received WORKREQ packet from fd " << pfd.fd << "\n";
                        // cast payload to uint8_t
                        auto num_threads = pkt.payload[0];
                        auto prefixes = generate_work_prefixes(partitions, part_index, num_threads);
                        if (send_work(pfd.fd, DEFAULT_RETRIES, args, prefixes) != 0)
                        {
                            std::cerr << "Failed to send WORK packet to client (fd: " << pfd.fd << ")\n";
                            ::close(pfd.fd);
                            pfd.fd = -1;
                        }
                        break;
                    }
                    case WORKFIN:
                        // handle_WORKFIN(pkt, client_fd, password_found);
                        std::cout << "Received WORKFIN packet from fd " << pfd.fd << "\n";
                        break;
                    case CHECK:
                        // handle_CHECK(pkt, client_fd, password_found);
                        std::cout << "Received CHECK packet from fd " << pfd.fd << "\n";
                        break;
                    case PWDFND:
                        // handle_PWDFND(pkt, client_fd);
                        std::cout << "Received PWDFND packet from fd " << pfd.fd << "\n";
                        break;
                    default:
                        std::cerr << "Unknown packet flag: " << static_cast<int>(pkt.header.flags) << "\n";
                        break;
                    }
                }
            }

            // Remove closed entries from pollfds
            pollfds.erase(
                std::remove_if(
                    pollfds.begin(),
                    pollfds.end(),
                    [](const pollfd &p)
                    { return p.fd == -1; }),
                pollfds.end());

            // Remove closed RAII client FDs that no longer appear in pollfds
            client_fds.erase(
                std::remove_if(
                    client_fds.begin(),
                    client_fds.end(),
                    [&](const Fd &fd)
                    {
                        // Keep only if fd.get() still exists in pollfds
                        return std::none_of(
                            pollfds.begin(),
                            pollfds.end(),
                            [&](const pollfd &p)
                            { return p.fd == fd.get(); });
                    }),
                client_fds.end());
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

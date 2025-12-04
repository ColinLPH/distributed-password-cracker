#include <iostream>
#include <unistd.h>
#include <chrono>
#include <unordered_map>

#include "network.h"
#include "parse_args.h"
#include "partition.h"

void print_checkpoint_info(int client_fd, const Packet &pkt)
{
    std::cout << "Client " << client_fd << " Checkpoint  Info:\n";
    std::cout << "  Interval: " << pkt.header.checkpoint_interval << "\n";
    std::cout << "  Last Prefix: " << std::string(pkt.payload.begin(), pkt.payload.end()) << "\n";
}

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
    bool start_time_set = false;
    std::chrono::steady_clock::time_point start_time, end_time;

    int connects = 0;
    int work_requests = 0;
    int checkpoints = 0;
    int total_pkts = 0;

    try
    {
        Fd listen_fd(create_listen_socket(args.port));
        if (!make_fd_non_blocking(listen_fd.get()))
        {
            throw std::runtime_error("Error making listen socket non-blocking");
        }

        std::vector<pollfd> pollfds;
        std::vector<Fd> client_fds;
        std::unordered_map<int, std::chrono::steady_clock::time_point> last_activity;

        pollfd listen_entry{};
        listen_entry.fd = listen_fd.get();
        listen_entry.events = POLLIN;
        pollfds.push_back(listen_entry);

        std::cout << "Server listening on port " << args.port << "\n";

        while (!password_found)
        {

            int n = poll(pollfds.data(), pollfds.size(), 1000);
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

                        last_activity[client_fd.get()] = std::chrono::steady_clock::now();

                        pollfd entry{};
                        entry.fd = client_fd.get();
                        entry.events = POLLIN;
                        pollfds.push_back(entry);

                        client_fds.push_back(std::move(client_fd));
                        ++total_pkts;
                        ++connects;

                        if (send_conack(entry.fd, DEFAULT_RETRIES, args) != 0)
                        {
                            std::cerr << "Failed to send CONACK to client (fd: " << entry.fd << ")\n";
                            ::close(entry.fd);
                            entry.fd = -1; // Mark for removal
                        }
                        ++total_pkts;
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
                    last_activity[pfd.fd] = std::chrono::steady_clock::now();
                    Packet pkt;
                    int rc = deserialize(buffer.data(), buffer.size(), pkt);
                    if (rc != 0)
                    {
                        std::cerr << "Failed to deserialize packet from client (fd: " << pfd.fd << "), error code: " << rc << "\n";
                        ::close(pfd.fd);
                        pfd.fd = -1;
                        continue;
                    }
                    ++total_pkts;
                    // Handle the packet
                    switch (pkt.header.flags)
                    {
                    case WORKREQ:
                    {
                        std::cout << "Received WORKREQ packet from fd " << pfd.fd << "\n";
                        ++work_requests;
                        auto num_threads = pkt.payload[0];
                        auto prefixes = generate_work_prefixes(partitions, part_index, num_threads);
                        if (send_work(pfd.fd, DEFAULT_RETRIES, args, prefixes) != 0)
                        {
                            std::cerr << "Failed to send WORK packet to client (fd: " << pfd.fd << ")\n";
                            ::close(pfd.fd);
                            pfd.fd = -1;
                        }
                        ++total_pkts;
                        if (!start_time_set)
                        {
                            start_time = std::chrono::steady_clock::now();
                            start_time_set = true;
                        }
                        break;
                    }
                    case WORKFIN:
                    {
                        std::cout << "Received WORKFIN packet from fd " << pfd.fd << "\n";
                        std::string last_prefix(pkt.payload.begin(), pkt.payload.end());
                        if (update_prefix(last_prefix, partitions) != 0)
                        {
                            std::cerr << "Failed to update prefix from WORKFIN packet: " << last_prefix
                                      << " (fd: " << pfd.fd << ")\n";
                        }
                        break;
                    }
                    case CHECK:
                    {
                        print_checkpoint_info(pfd.fd, pkt); // Could do: optimize sending next work based on work remaining
                        std::string last_prefix_chk(pkt.payload.begin(), pkt.payload.end());
                        if (update_prefix(last_prefix_chk, partitions) != 0)
                        {
                            std::cerr << "Failed to update prefix from CHECK packet: " << last_prefix_chk
                                      << " (fd: " << pfd.fd << ")\n";
                        }

                        ++checkpoints;
                        break;
                    }
                    case PWDFND:
                    {
                        std::cout << "Received PWDFND packet from fd " << pfd.fd << "\n";
                        password_found = true;
                        std::string found_password(pkt.payload.begin(), pkt.payload.end());
                        std::cout << "Password found: " << found_password << "\n";
                        end_time = std::chrono::steady_clock::now();
                        for (auto &entry : pollfds)
                        {
                            if (entry.fd != -1 && entry.fd != listen_fd.get())
                            {
                                std::cout << "Active client fd: " << entry.fd << "\n";
                                if (send_kill(entry.fd, DEFAULT_RETRIES) != 0)
                                {
                                    std::cerr << "Failed to send KILL packet to client (fd: " << entry.fd << ")\n";
                                }
                                ++total_pkts;
                            }
                        }
                        break;
                    }
                    default:
                        std::cerr << "Unknown packet flag: " << static_cast<int>(pkt.header.flags) << "\n";
                        break;
                    }
                }
            }

            auto now = std::chrono::steady_clock::now();
            pollfds.erase(
                std::remove_if(
                    pollfds.begin(),
                    pollfds.end(),
                    [&](pollfd &pfd)
                    {
                        if (pfd.fd == listen_fd.get())
                            return false; // keep listening socket

                        bool remove = false;

                        // Check for timeout
                        auto it = last_activity.find(pfd.fd);

                        if (it != last_activity.end())
                        {
                            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();

                            if (duration > args.timeout)
                            {
                                std::cout << "Client fd " << pfd.fd << " timed out after " << duration << "s\n";
                                ::close(pfd.fd);
                                last_activity.erase(it);
                                remove = true;
                            }
                        }

                        // Already marked for removal (e.g., disconnected)
                        if (pfd.fd == -1)
                            remove = true;

                        return remove;
                    }),
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

        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        double elapsed_sec = elapsed_ms / 1000.0;
        std::cout << "Total elapsed time: " << elapsed_sec << " seconds\n";
        std::cout << "Total connections: " << connects << "\n";
        std::cout << "Total work requests: " << work_requests << "\n";
        std::cout << "Total checkpoints: " << checkpoints << "\n";
        std::cout << "Estimated total candidates tried: " << checkpoints * args.checkpoint_interval << "\n";
        std::cout << "Total packets processed: " << total_pkts << "\n";
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

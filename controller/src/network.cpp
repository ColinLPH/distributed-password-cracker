#include "network.h"

bool make_fd_non_blocking(int fd)
{
    auto flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int create_listen_socket(int port)
{
    auto sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw std::runtime_error("Error creating socket");
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        ::close(sock);
        throw std::runtime_error("Error binding socket");
    }

    if (!make_fd_non_blocking(sock))
    {
        ::close(sock);
        throw std::runtime_error("Error making socket non-blocking");
    }

    if (::listen(sock, SOMAXCONN) == -1)
    {
        ::close(sock);
        throw std::runtime_error("Error listening on socket");
    }

    return sock;
}

ssize_t serialize(const Packet &packet, std::vector<uint8_t> &buffer)
{
    buffer.clear();

    buffer.push_back(packet.header.flags);
    buffer.push_back(packet.header.data_len);
    buffer.insert(buffer.end(), packet.payload.begin(), packet.payload.end());

    return static_cast<ssize_t>(buffer.size());

}

int deserialize(const uint8_t *buffer, size_t len, Packet &result)
{
    if (len < HEADER_SIZE)
    {
        std::cout << "buffer too short for header\n";
        return -1;
    }

    result.header.flags = buffer[0];
    result.header.data_len = buffer[1];

    size_t expected_len = HEADER_SIZE + result.header.data_len;
    if (len < expected_len)
    {
        std::cout << "buffer shorter than expected length\n";
        return -1;
    }

    result.payload.resize(result.header.data_len);
    if (result.header.data_len > 0)
    {
        std::copy(buffer + HEADER_SIZE, buffer + expected_len, result.payload.begin());
    }

    return 0;
}

int send_all(int fd, const uint8_t *data, size_t len)
{
    size_t total_sent = 0;
    while (total_sent < len)
    {
        ssize_t sent = ::send(fd, data + total_sent, len - total_sent, 0);
        if (sent <= 0)
        {
            return -1; // Error or connection closed
        }
        total_sent += sent;
    }
    return total_sent;
}

int recv_all(int fd, uint8_t *buffer, size_t len)
{
    size_t total_received = 0;
    while (total_received < len)
    {
        ssize_t received = ::recv(fd, buffer + total_received, len - total_received, 0);
        if (received <= 0)
        {
            return -1; // Error or connection closed
        }
        total_received += received;
    }
    return total_received;
}

ssize_t recv_full_packet(int fd, std::vector<uint8_t> &buffer)
{
    uint8_t header_buf[HEADER_SIZE];

    // Receive header first
    ssize_t n = recv_all(fd, header_buf, HEADER_SIZE);
    if (n <= 0)
        return n; // error or connection closed
    if (n != HEADER_SIZE)
        return -1; // incomplete header

    uint8_t flags = header_buf[0];
    uint8_t data_len = header_buf[1];

    // Resize buffer to hold header + payload
    buffer.resize(HEADER_SIZE + data_len);
    buffer[0] = flags;
    buffer[1] = data_len;

    // If there is a payload, receive it
    if (data_len > 0)
    {
        n = recv_all(fd, buffer.data() + HEADER_SIZE, data_len);
        if (n <= 0)
            return n; // error or connection closed
        if (static_cast<size_t>(n) != data_len)
            return -2; // incomplete payload
    }

    return static_cast<ssize_t>(buffer.size()); // total bytes read
}

void handle_packet(const Packet &pkt, int client_fd,
                   const std::vector<std::unique_ptr<Partition>> &partitions,
                   bool &password_found)
{
    switch (pkt.header.flags)
    {
    case WORKREQ:
        // handle_WORKREQ(pkt, client_fd, partitions, password_found);
        std::cout << "Received WORKREQ packet from fd " << client_fd << "\n";
        break;
    case WORKFIN:
        // handle_WORKFIN(pkt, client_fd, password_found);
        std::cout << "Received WORKFIN packet from fd " << client_fd << "\n";
        break;
    case CHECK:
        // handle_CHECK(pkt, client_fd, password_found);
        std::cout << "Received CHECK packet from fd " << client_fd << "\n";
        break;
    case PWDFND:
        // handle_PWDFND(pkt, client_fd);
        std::cout << "Received PWDFND packet from fd " << client_fd << "\n";
        break;
    default:
        std::cerr << "Unknown packet flag: " << static_cast<int>(pkt.header.flags) << "\n";
        break;
    }
}

int send_conack(int client_fd, int retries, Args &args)
{
    Packet pkt;
    pkt.header.flags = CONACK;
    pkt.header.data_len = static_cast<uint8_t>(args.hash.size());
    pkt.payload.insert(pkt.payload.end(), args.hash.begin(), args.hash.end());
    std::vector<uint8_t> buffer;
    ssize_t ret = serialize(pkt, buffer);
    if (ret < 0)
    {
        std::cerr << "Failed to serialize CONACK packet\n";
        return -1;
    }

    for (int attempt = 0; attempt < retries; ++attempt)
    {
        int n = send_all(client_fd, buffer.data(), buffer.size());
        if (n == static_cast<int>(buffer.size()))
        {
            return 0; // Success
        }
        std::cerr << "Failed to send CONACK, attempt " << (attempt + 1) << "\n";
    }
    return -1; // Failed after retries
}

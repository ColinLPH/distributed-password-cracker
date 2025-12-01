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
    buffer.reserve(HEADER_SIZE + packet.payload.size());

    buffer.push_back(packet.header.flags);
    buffer.push_back(packet.header.data_len);

    uint16_t net_work_size = htons(packet.header.work_size);
    uint16_t net_checkpoint = htons(packet.header.checkpoint_interval);

    buffer.push_back((net_work_size >> 8) & 0xFF);
    buffer.push_back(net_work_size & 0xFF);
    buffer.push_back((net_checkpoint >> 8) & 0xFF);
    buffer.push_back(net_checkpoint & 0xFF);

    buffer.insert(buffer.end(), packet.payload.begin(), packet.payload.end());

    return buffer.size();
}


int deserialize(const uint8_t *buffer, size_t len, Packet &result)
{
    if (len < HEADER_SIZE) {
        std::cerr << "buffer too short for header\n";
        return -1;
    }

    result.header.flags    = buffer[0];
    result.header.data_len = buffer[1];

    // Extract raw big-endian values
    uint16_t raw_work_size =
        (uint16_t(buffer[2]) << 8) | buffer[3];
    uint16_t raw_checkpoint =
        (uint16_t(buffer[4]) << 8) | buffer[5];

    result.header.work_size = ntohs(raw_work_size);
    result.header.checkpoint_interval = ntohs(raw_checkpoint);

    size_t expected_len = HEADER_SIZE + result.header.data_len;

    if (len < expected_len) {
        std::cerr << "buffer shorter than expected payload length\n";
        return -1;
    }

    result.payload.resize(result.header.data_len);

    if (result.header.data_len > 0) {
        std::copy(buffer + HEADER_SIZE,
                  buffer + expected_len,
                  result.payload.begin());
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
    uint16_t work_size = (header_buf[2] << 8) | header_buf[3];
    uint16_t checkpoint_interval = (header_buf[4] << 8) | header_buf[5];

    buffer.resize(HEADER_SIZE + data_len);
    memcpy(buffer.data(), header_buf, HEADER_SIZE);

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

int send_conack(int client_fd, int retries, const Args &args)
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

int send_work(int client_fd, int retries, const Args &args, const std::vector<std::string> &prefixes)
{
    Packet pkt;
    pkt.header.flags = WORK;
    pkt.header.work_size = static_cast<uint16_t>(args.work_size);
    pkt.header.checkpoint_interval = static_cast<uint16_t>(args.checkpoint_interval);
    std::cout << "Preparing to send WORK packet with work_size: " << pkt.header.work_size
              << " and checkpoint_interval: " << pkt.header.checkpoint_interval << "\n";

    size_t total_size = 0;
    if (!prefixes.empty())
    {
        for (const auto &prefix : prefixes)
        {
            total_size += prefix.size();
        }
        total_size += prefixes.size() - 1;
    }

    if (total_size > 255)
    {
        std::cerr << "Error: total payload too large for data_len\n";
        return -1;
    }
    pkt.header.data_len = static_cast<uint8_t>(total_size);

    pkt.payload.reserve(total_size);
    for (size_t i = 0; i < prefixes.size(); ++i)
    {
        const auto &prefix = prefixes[i];
        pkt.payload.insert(pkt.payload.end(), prefix.begin(), prefix.end());
        if (i != prefixes.size() - 1)
        {
            pkt.payload.push_back(' ');
        }
    }

    std::vector<uint8_t> buffer;
    ssize_t ret = serialize(pkt, buffer);
    if (ret < 0)
    {
        std::cerr << "Failed to serialize WORK packet\n";
        return ret;
    }
    for (int attempt = 0; attempt < retries; ++attempt)
    {
        int n = send_all(client_fd, buffer.data(), buffer.size());
        if (n == static_cast<int>(buffer.size()))
        {
            return 0; // Success
        }
        std::cerr << "Failed to send WORK, attempt " << (attempt + 1) << "\n";
    }
    return 0;
}

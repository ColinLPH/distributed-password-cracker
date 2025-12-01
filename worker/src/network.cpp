#include "network.h"

int connect_to_server(const Args &args)
{
    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args.server_port);

    if (inet_pton(AF_INET, args.serverIP.c_str(), &server_addr.sin_addr) <= 0)
    {
        close(sockfd);
        throw std::runtime_error("Invalid server IP address");
    }

    if (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Failed to connect to server");
    }

    return sockfd;
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


ssize_t send_all(int fd, const uint8_t *data, size_t len)
{
    ssize_t total_sent = 0;
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

ssize_t recv_all(int fd, uint8_t *buffer, size_t len)
{
    ssize_t total_received = 0;
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

int send_workreq(int server_fd, int retries, int num_threads)
{
    Packet workreq_packet;
    workreq_packet.header.flags = WORKREQ;
    workreq_packet.header.data_len = 1;
    workreq_packet.payload.push_back(static_cast<uint8_t>(num_threads));

    std::vector<uint8_t> serialized_packet;
    if (serialize(workreq_packet, serialized_packet) < 0)
    {
        std::cerr << "Failed to serialize WORKREQ packet.\n";
        return -1; // Serialization failed
    }

    for (int attempt = 0; attempt < retries; ++attempt)
    {
        ssize_t n = send_all(server_fd, serialized_packet.data(), serialized_packet.size());
        if (n == static_cast<ssize_t>(serialized_packet.size()))
        {
            return 0; // Successfully sent
        }
        // Optionally add a delay here before retrying
    }

    std::cerr << "Failed to send WORKREQ after " << retries << " attempts.\n";
    return -1; // Failed to send after retries
}

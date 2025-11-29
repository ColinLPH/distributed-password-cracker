#include "network.h"

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

ssize_t send_all(int fd, const uint8_t* data, size_t len)
{
    ssize_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = ::send(fd, data + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // Error or connection closed
        }
        total_sent += sent;
    }
    return total_sent;
}

ssize_t recv_all(int fd, uint8_t* buffer, size_t len)
{
    ssize_t total_received = 0;
    while (total_received < len) {
        ssize_t received = ::recv(fd, buffer + total_received, len - total_received, 0);
        if (received <= 0) {
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
    if (data_len > 0) {
        n = recv_all(fd, buffer.data() + HEADER_SIZE, data_len);
        if (n <= 0)
            return n; // error or connection closed
        if (static_cast<size_t>(n) != data_len)
            return -2; // incomplete payload
    }

    return static_cast<ssize_t>(buffer.size()); // total bytes read
}


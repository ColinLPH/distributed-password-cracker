#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <iostream>

constexpr size_t HEADER_SIZE = 2;

enum Header_Flags : uint8_t {
    CONACK,
    WORK,
    KILL,
    REQLOG,
    WORKLOG,
    WORKREQ,
    CHECK,
    PWDFND
};

struct Header {
    uint8_t flags;
    uint8_t data_len;
};

struct Packet {
    Header header;
    std::vector<uint8_t> payload;
};

ssize_t send_all(int fd, const uint8_t* data, size_t len);
ssize_t recv_all(int fd, uint8_t* buffer, size_t len);
ssize_t recv_full_packet(int fd, std::vector<uint8_t> &buffer);

ssize_t serialize(const Packet &packet, std::vector<uint8_t> &buffer);
int deserialize(const uint8_t *buf, size_t len, Packet &result);

#endif // NETWORK_H

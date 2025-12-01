#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <iostream>

#include "parse_args.h"

constexpr int DEFAULT_RETRIES = 3;
constexpr size_t HEADER_SIZE = 6;

enum Header_Flags : uint8_t {
    CONACK = 0,
    WORK,
    KILL,
    REQLOG,
    WORKLOG,
    WORKREQ,
    WORKFIN,
    CHECK,
    PWDFND
};

struct Header {
    uint8_t flags;
    uint8_t data_len;
    uint16_t work_size;
    uint16_t checkpoint_interval;
};

struct Packet {
    Header header;
    std::vector<uint8_t> payload;
};

int connect_to_server(const Args &args);

ssize_t send_all(int fd, const uint8_t* data, size_t len);
ssize_t recv_all(int fd, uint8_t* buffer, size_t len);
ssize_t recv_full_packet(int fd, std::vector<uint8_t> &buffer);

ssize_t serialize(const Packet &packet, std::vector<uint8_t> &buffer);
int deserialize(const uint8_t *buf, size_t len, Packet &result);

int send_workreq(int server_fd, int retries, int num_threads);
int send_workfin();
int send_check();
int send_pwdfind();

#endif // NETWORK_H

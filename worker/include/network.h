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
#include <mutex>

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

ssize_t threadsafe_send_all(int fd, const uint8_t *data, size_t len);
int send_workreq(int server_fd, int retries, int num_threads);
int send_workfin(int server_fd, int retries, std::string &last_prefix);
int send_check(int server_fd, int retries, uint16_t work_done, uint16_t work_size, std::string &last_prefix);
int send_pwdfind(int server_fd, int retries, const std::string &found_password);

#endif // NETWORK_H

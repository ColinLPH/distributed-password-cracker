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
#include <algorithm>
#include <cstring>
#include <poll.h>
#include <iostream>

#include "partition.h"
#include "parse_args.h"

constexpr int DEFAULT_RETRIES = 3;
constexpr int MAX_EPOLL_EVENTS = 100;

constexpr size_t HEADER_SIZE = 6;

class Fd {
    public:
        Fd(int fd = -1) : fd_(fd) {}
        ~Fd() {
            if (fd_ != -1) {
                ::close(fd_);
            }
        }
        
        Fd(const Fd&) = delete;
        Fd& operator=(const Fd&) = delete;

        Fd(Fd&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }
        Fd& operator=(Fd&& other) noexcept {
            if (this != &other) {
                if (fd_ != -1) ::close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const { return fd_; }

    private:
        int fd_;
};

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

bool make_fd_non_blocking(int fd);
int create_listen_socket(int port);

int send_all(int fd, const uint8_t* data, size_t len);
int recv_all(int fd, uint8_t* buffer, size_t len);
ssize_t recv_full_packet(int fd, std::vector<uint8_t> &buffer);

ssize_t serialize(const Packet &packet, std::vector<uint8_t> &buffer);
int deserialize(const uint8_t *buffer, size_t len, Packet &result);

int send_conack(int client_fd, int retries, const Args &args);
int send_work(int client_fd, int retries, const Args &args, const std::vector<std::string> &prefixes);
int send_kill(int client_fd, int retries);

#endif // NETWORK_H
